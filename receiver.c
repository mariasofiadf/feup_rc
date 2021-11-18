#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>

#include "header.h"

//RECEIVER

int llopen_receiver(int fd)
{
    wait_set(fd);

    send_ua(fd);

    return 0;
}

int wait_set(int fd)
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

int send_ua(int fd)
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
            send_disc(fd);
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