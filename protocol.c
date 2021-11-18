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

struct termios oldtio,newtio;

int send_trama(int fd, char * buf, int length){
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

  /*
  VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
  leitura do(s) pr髕imo(s) caracter(es)
  */

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

int llclose(int fd){

  sleep(1);

  send_disc(fd);

  sleep(1);

  //repor serial port
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
}
