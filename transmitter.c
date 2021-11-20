#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>

#include "header.h"


u_int8_t C_COUNT = C_ZERO;

//ISSUER

int flag = 1, try = 0;

void sig_handler(int signum){
  flag = 1;
  try++;
}

int llopen_transmitter (int fd){

  signal(SIGALRM,sig_handler); // Register signal handler
 
  while (try < RETRANSMISSIONMAX)
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

  unsigned char set_message[5];
  set_message[0] = FLAG;
  set_message[1] = A_ISSUER;
  set_message[2] = SET;
  set_message[3] = A_ISSUER^SET;
  set_message[4] = FLAG;

  send_trama(fd, set_message, 6);

  if(DEBUG)
    printf("Sent SET message. \n");

  return 0;
}

int wait_ua(int fd){
    unsigned char rcv;
    int finished = 0;
    unsigned char a, c;
    enum state state = START; 
    flag = 0;
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
              try = RETRANSMISSIONMAX;
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


int wait_rr(int fd){
    unsigned char rcv;
    int finished = 0;
    unsigned char a, c;
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
            if(rcv == RR_ONE || rcv == RR_ZERO || rcv == REJ_ZERO || rcv == REJ_ONE){
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
            if((c == RR_ZERO && C_COUNT==C_ZERO)||(c == RR_ONE && C_COUNT==C_ONE))
              return 1;
            else if((c == REJ_ZERO && C_COUNT == C_ZERO) || c == REJ_ONE && C_COUNT == C_ONE)
              return -1;
            if(C_COUNT == C_ZERO)
              C_COUNT = C_ONE;
            else C_COUNT = C_ZERO;
            if(rcv == FLAG){
              try = RETRANSMISSIONMAX;
              flag = 1;
              if(DEBUG)
                printf("Received RR message\n");
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

int send_info(int fd, unsigned char * buffer, int length){

  unsigned char i_message[6+length*2];
  i_message[0] = FLAG;
  i_message[1] = A_ISSUER;
  i_message[2] = C_COUNT; //C_ZERO || C_ONE

  //i_message[3] = A_ISSUER^C_ZERO; //BCC1
  unsigned char bcc2 = 0;

  int to_stuff_size = length+2;
  unsigned char to_stuff[to_stuff_size];
  to_stuff[0] = A_ISSUER^C_COUNT; //BCC1

  
  for( int i=0; i < length; i++){
    bcc2 = bcc2 ^ buffer[i];
    to_stuff[i+1] = buffer[i];
    //i_message[i+4] = buffer[i];  
  }
  
  //FORCE BCC2 ERRORS TO CREATE REJ ON RECEIVER
  if((random() % 10) == 0){ //10% chance of error
    bcc2 = !bcc2;
  }


  to_stuff[length+1]=bcc2;

  //printf("BCC2:%d\t", bcc2);

  unsigned char stuffed[(to_stuff_size)*2];

  int stuffed_size = stuffing(to_stuff, stuffed,length+2);

  //i_message[length+4] = bcc2;
  for(int i = 0; i < stuffed_size; i++){
    
    i_message[i+3] = stuffed[i];
  }
  
  //printf("BCC1:%d\n", i_message[3]);
  i_message[stuffed_size+3] = FLAG;


  send_trama(fd, i_message, stuffed_size+4);

  if(DEBUG)
    printf("Sent %i information bytes\tC_COUNT: %d \n", length,C_COUNT);

}


int llwrite(int fd, unsigned char * buffer, int length){

  signal(SIGALRM,sig_handler); // Register signal handler
  flag = 1; try = 0;
  while (try < RETRANSMISSIONMAX)
  {
    if(flag){
      alarm(3);
      flag = 0;
      send_info(fd, buffer, length); //send_info
      int r = wait_rr(fd);
      if( r == 0) //wait_rr
        return 0;
      else if( r == -1){
        flag = 1;
      }
    }
  }


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



int llclose(int fd){

  flag = 1; try = 0;

  signal(SIGALRM,sig_handler); // Register signal handler
 
  while (try < RETRANSMISSIONMAX)
  {
    if(flag){
      alarm(3);
      flag = 0;
      send_disc(fd);
      if(wait_disc(fd) == 0)
        try = RETRANSMISSIONMAX;
    }
  }

  send_ua(fd);

  sleep(1);

  //repor serial port
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
}