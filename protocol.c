#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>

#include "header.h"
#include "transmitter.h"
#include "receiver.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"

//COMMON

volatile int STOP=FALSE;



int send_trama(int fd, unsigned char * buf, int length){
  int i = 0;
  while(i < length){
    write(fd,&buf[i],1);
    i++;
  }
}

int port_connect(char* port){
  int fd,c, res;
  fd = open(port, O_RDWR | O_NOCTTY );
  if (fd <0) {perror(port); exit(-1); }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
  perror("tcgetattr");
  exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0; /* blocking read until n chars received */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
  perror("tcsetattr");
  exit(-1);
  }

    if(DEBUG)
        printf("New termios structure set\n");

  return fd;
}

int llopen(char *port, int mode)
{

    int fd = port_connect(port);
    if(fd < 0)
        return 1;
    if(DEBUG)
        printf("Connected to port\n");

    if(mode == TRANSMITTER)
        llopen_transmitter(fd);
    else
        llopen_receiver(fd);

    return fd;
}


int stuffing(char * data, char * stuf_data, unsigned int data_size){
    unsigned int j = 0;

    for(unsigned int i=0; i < data_size; i++)
    {
        if(data[i]== 0x7e){
            stuf_data[j]=0x7d;
            j++;
            stuf_data[j]=0x5e;

        }
        else if(data[i]==0x7d){

            stuf_data[j]=0x7d;
            j++;
            stuf_data[j]=0x5d;
        }
        else stuf_data[j]=data[i];
        j++;
    }

    return j;
}


int destuffing(char * stuf_data, char * destuf_data, unsigned int data_size){
    unsigned int j = 0;

    for(unsigned int i=0; i < data_size;i++)
    {
        if(stuf_data[i]== 0x7d && stuf_data[i+1] == 0x5e){
            destuf_data[j]=0x7e;
            i++;
        }
        else if(stuf_data[i]==0x7d && stuf_data[i+1] == 0x5d){
            destuf_data[j]=0x7d;
            i++;
        }
        else destuf_data[j]= stuf_data[i];
        j++;
    }

    return j;
}


int send_ua(int fd)
{
    unsigned char set_message[5];
    set_message[0] = FLAG;
    set_message[1] = A_ISSUER;
    set_message[2] = UA;
    set_message[3] = A_ISSUER ^ UA;
    set_message[4] = FLAG;

    send_trama(fd, set_message, 6);

    if(DEBUG)
        printf("Sent UA message\n");

    return 0;
}


int send_disc(int fd){

  char disc_message[5];
  disc_message[0] = FLAG;
  disc_message[1] = A_ISSUER;
  disc_message[2] = DISC;
  disc_message[3] = A_ISSUER^DISC;
  disc_message[4] = FLAG;

  send_trama(fd, disc_message, 6);

  if(DEBUG)
    printf("Sent DISC message. \n");

  return 0;
}



int wait_disc(int fd)
{
    unsigned char rcv = "";
    int finished = 0;
    unsigned char a, c;
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
            if (rcv == DISC)
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
    if(DEBUG)
        printf("Received DISC message\n");
    return 0;
}
