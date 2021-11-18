#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>

#include "header.h"

int flag = 1, try = 0;

//ISSUER

void sig_handler(int signum){
  flag = 1;
  try++;
}

int llopen_transmitter (int fd){

  signal(SIGALRM,sig_handler); // Register signal handler
 
  while (try < 3)
  {
    if(flag){
      alarm(3);
      flag = 0;
      send_set(fd);
      if(wait_ua(fd) == 0)
        return 0;
    }
  }

  return -1;
}

int send_set(int fd){

  char * set_message = malloc(5);
  set_message[0] = FLAG;
  set_message[1] = A_ISSUER;
  set_message[2] = SET;
  set_message[3] = A_ISSUER^SET;
  set_message[4] = FLAG;

  send_trama(fd, set_message, 6);

  if(DEBUG)
    printf("Sent SET message. \n");

  free(set_message);
  return 0;
}

int wait_ua(int fd){
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
              if(DEBUG)
                printf("Received UA message\n");
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


int llwrite(int fd, unsigned char * buffer, int length){
  unsigned char * i_message = malloc(6+length);
  i_message[0] = FLAG;
  i_message[1] = A_ISSUER;
  i_message[2] = C_ZERO;
  i_message[3] = A_ISSUER^C_ZERO;
  unsigned char bcc2;
  int i = 0;
  
  while(i < length){
    i_message[i + 4] = buffer[i];
    i++;
    if(i > 0)
    {
      bcc2 = i_message[i-1] ^ i_message[i];
    }
  }
  i_message[i + 4] = bcc2;
  i_message[i + 5] = FLAG;
  send_trama(fd, i_message, length + 6);

  if(DEBUG)
    printf("Sent %s message. \n", buffer);

  free(i_message);
}


int send_disc(int fd){

  char * disc_message = malloc(5);
  disc_message[0] = FLAG;
  disc_message[1] = A_ISSUER;
  disc_message[2] = DISC;
  disc_message[3] = A_ISSUER^DISC;
  disc_message[4] = FLAG;

  send_trama(fd, disc_message, 6);

  if(DEBUG)
    printf("Sent DISC message. \n");

  free(disc_message);
  return 0;
}


