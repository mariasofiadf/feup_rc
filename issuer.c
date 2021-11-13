/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>


#include "header.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"

volatile int STOP=FALSE;

//emissor ligado a ttyS10 e  o recetor ligado a ttyS11

int flag = 1, try = 0;



int wait_ua_message(int fd, char * buf[]){
    char rcv;
    int finished = 0;
    char a, c;
    enum state state = START; 
    while(!flag){
        if(read(fd,&rcv,1) <= 0)
          usleep(100000);
        switch (state)
        {
        case START:
            if(rcv == FLAG)
                state = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(rcv == A_ISSUER){
                a = rcv;
                state = A_RCV;
            }
            else if(rcv != FLAG)
                state = START;
            break;
        case A_RCV:
            if(rcv == UA){
                c = rcv;
                state = C_RCV;
            }              
            else if(rcv == FLAG)
                state = FLAG_RCV;
            else
                state = START;
            break;
        case C_RCV:
            if(rcv == FLAG)
                state = FLAG_RCV;
            else if( rcv == (a ^ c))
                state = BCC_OK;
            else
                state = START;
            break;
        case BCC_OK:
            if(rcv == FLAG){
              try = 3;
              flag = 1;
              printf("[issuer] Received UA message\n");
              return 0; //Success
            }
            else
                state = START;
            break;
        default:
            break;
        }
    }
    return 1; //Failed
}

int send_set_message(int fd, char * buf){
  char a = A_ISSUER, flag = FLAG, c = SET;
  if(write(fd,&flag,1) <= 0)
    return 1;
  if(write(fd,&a,1) <= 0)
     return 1;
  if(write(fd,&c,1) <= 0)
    return 1;
  char BCC = a ^ c;
  if(write(fd,&BCC,1) <= 0)
    return 1;
  if(write(fd,&flag,1) <= 0)
    return 1;
  printf("[issuer] Sent SET message\n");
  return 0;
}


void sig_handler(int signum){
  flag = 1;
  try++;
}

int llopen (int fd, char * buf[]){

  signal(SIGALRM,sig_handler); // Register signal handler
 
  while (try < 3)
  {
    if(flag){
      alarm(3);
      flag = 0;
      send_set_message(fd, buf);
      if(wait_ua_message(fd, buf) == 0)
        return 0;
    }
  }

  return -1;
}

struct termios oldtio,newtio;

int inner_open(char** argv){
  int fd,c, res;
  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

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

  printf("New termios structure set\n");

  return fd;
}


int main(int argc, char** argv)
  {
  char buf[255];
  int i, sum = 0, speed = 0;

  if ( (argc < 2) ) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
  */

  int fd = inner_open(argv);

  if(llopen(fd, buf) < 0){
      perror("[issuer] Connect failed!");       
      return -1;
  }


  sleep(1);

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
  perror("tcsetattr");
  exit(-1);
  }




  close(fd);
  return 0;
}