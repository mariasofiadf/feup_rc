/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "header.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"

volatile int STOP=FALSE;


int wait_ua_message(int fd, char * buf[], struct trama_s * trama_ua){
  read(fd,trama_ua, sizeof(trama_ua));
  printf("[issuer] received trama: F: %x A: %x C: %x BCC: %x\n", trama_ua->F, trama_ua->A, trama_ua->C, trama_ua->BCC);
  if((trama_ua->A ^ trama_ua->C) != trama_ua->BCC){
    printf("[issuer] BCC invalid\n");
    return 1;
  }
}

int send_set_message(int fd, char * buf, struct trama_s * trama_set){
  trama_set->A = A_ISSUER;
  trama_set->F = FLAG;
  trama_set->C = SET;
  trama_set->BCC = trama_set->A ^ trama_set->C;

  if(write(fd,trama_set,sizeof(trama_set)) <= 0)
     return 1;
  printf("[issuer] send trama-> F: %x A: %x C: %x BCC: %x\n", trama_set->F, trama_set->A, trama_set->C, trama_set->BCC);
    
}

int issuer_connect (int fd, char * buf[]){
    
  struct trama_s * trama = malloc(sizeof(struct trama_s));
  send_set_message(fd, buf, trama);

  wait_ua_message(fd, buf, trama);

  free(trama);
    
  return 0;
}

int main(int argc, char** argv)
  {
  int fd,c, res;
  struct termios oldtio,newtio;
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

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 1; /* blocking read until 5 chars received */



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



  if(issuer_connect(fd, buf)){
      perror("[issuer] Connect failed!");       
      return -1;
  }


  /*
  //for (i = 0; i < 255; i++) {

  gets(buf);

  //buf[25] = '\n';

  

  res = write(fd,buf,strlen(buf)+1);
  //printf("%d bytes written\n", res);

  for(int i = 0; i < strlen(buf); i++) {
  write(fd,buf[i],1);
  }


  int x = 0;
  while (STOP==FALSE) { 
  res = read(fd,&buf[x],1); 
  if (buf[x]=='\0') STOP=TRUE;
  x++;
  }

  printf("%s\n", buf, res);

  */



  sleep(1);

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
  perror("tcsetattr");
  exit(-1);
  }




  close(fd);
  return 0;
}