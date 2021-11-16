/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "header.h"

#define BAUDRATE B38400

volatile int STOP = FALSE;

//emissor ligado a ttyS10 e  o recetor ligado a ttyS11

int wait_set_message(int fd)
{
    char rcv;
    int finished = 0;
    char a, c;
    enum state state = START;
    while (!finished)
    {
        if (read(fd, &rcv, 1) <= 0)
            usleep(100000);
        switch (state)
        {
        case START:
            if (rcv == FLAG)
                state = FLAG_RCV;
            break;
        case FLAG_RCV:
            if (rcv == A_ISSUER)
            {
                a = rcv;
                state = A_RCV;
            }
            else if (rcv != FLAG)
                state = START;
            break;
        case A_RCV:
            if (rcv == SET)
            {
                c = rcv;
                state = C_RCV;
            }
            else if (rcv == FLAG)
                state = FLAG_RCV;
            else
                state = START;
            break;
        case C_RCV:
            if (rcv == FLAG)
                state = FLAG_RCV;
            else if (rcv == (a ^ c))
                state = BCC_OK;
            else
                state = START;
            break;
        case BCC_OK:
            if (rcv == FLAG)
            {
                finished = 1;
            }
            else
                state = START;
            break;
        default:
            break;
        }
    }
    printf("[receiver] Received SET message\n");
    return 0;
}

int send_ua_message(int fd)
{
    char *set_message = malloc(5);
    set_message[0] = FLAG;
    set_message[1] = A_ISSUER;
    set_message[2] = UA;
    set_message[3] = A_ISSUER ^ UA;
    set_message[4] = FLAG;

    send_trama(fd, set_message, 6);

    printf("[receiver] Sent UA message\n");

    free(set_message);
    return 0;
}

int llopen(char *porta, int mode)
{

    int fd = inner_open(porta);

    llopen_receiver(fd);

    return fd;
}

struct termios oldtio, newtio;

int inner_open(char *porta)
{
    int fd, c, res;

    printf("%s\n", porta);

    fd = open(porta, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(porta);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1)
    { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME] = 1; // inter-character timer unused /
    newtio.c_cc[VMIN] = 0;  // blocking read until 5 chars received /
    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
    */
    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }
    return fd;
}

int llopen_receiver(int fd)
{

    wait_set_message(fd);

    send_ua_message(fd);

    return 0;
}

int send_disc_message(int fd)
{

    char *disc_message = malloc(5);
    disc_message[0] = FLAG;
    disc_message[1] = A_ISSUER;
    disc_message[2] = DISC;
    disc_message[3] = A_ISSUER ^ DISC;
    disc_message[4] = FLAG;

    send_trama(fd, disc_message, 6);

    printf("[receiver] Sent DISC message. \n");

    free(disc_message);
    return 0;
}

int llread(int fd, char *buffer)
{

    char rcv;
    int finished = 0, count = 0;
    char a, c, bcc2;
    enum state state = START;
    char *data = malloc(255);
    while (!finished)
    {
        if (read(fd, &rcv, 1) <= 0)
            usleep(10000);
        switch (state)
        {
        case START:
            //printf("START\n");
            if (rcv == FLAG)
                state = FLAG_RCV;
            break;
        case FLAG_RCV:
            //printf("FLAG_RCV\n");
            if (rcv == A_ISSUER)
            {
                a = rcv;
                state = A_RCV;
            }
            else if (rcv != FLAG)
                state = START;
            break;
        case A_RCV:
            //printf("A_RCV\n");
            if (rcv == C_ZERO)
            {
                c = rcv;
                state = C_RCV;
            }
            else if (rcv == FLAG)
                state = FLAG_RCV;
            else if (rcv == DISC)
                state = DISC_ST;
            else
                state = START;
            break;
        case C_RCV:
            //printf("C_RCV\n");
            if (rcv == FLAG)
                state = FLAG_RCV;
            else if (rcv == (a ^ c))
                state = BCC_OK;
            else
                state = START;
            break;
        case BCC_OK:
            //printf("BCC_OK: %s\n", &rcv);
            if (rcv == FLAG)
            {
                state = DATA_RCV;
            }
            else
            {
                data[count] = rcv;
                count++;
                if (count > 0)
                {
                    bcc2 = data[count - 1] ^ data[count];
                }
            }
            break;
        case DATA_RCV:
            if (data[count - 1] == bcc2)
            {
                data[count - 1] = '\0';
                state = STOP_ST;
                //printf("BCC2_OK\n");
            }

            //finished = 1;
            break;
        case DISC_ST:
            //printf("DISC_ST\n");
            send_disc_message(fd);
            finished = 1;
            break;
        case STOP_ST:
            printf("[receiver] Received %s message. \n", data);
            state = START;
            break;
        default:
            break;
        }
    }
    sprintf(buffer, data);

    free(data);
    return 0;
}

int main(int argc, char **argv)
{
    int fd, c, res;
    struct termios oldtio, newtio;
    char buf[255];

    if ((argc < 2))
    {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */

    if ((fd = llopen(argv[1], RECEIVER)) < 0)
    {
        perror("[receiver] Connect failed!");
        return -1;
    }

    llread(fd, buf);

    sleep(1);
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
    return 0;
}
