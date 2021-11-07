/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "header.h"

#define BAUDRATE B38400

volatile int STOP=FALSE;

int wait_set_message(int fd, char * buf[], struct trama_s * trama_set ){
    read(fd,trama_set,sizeof(trama_set));
    printf("[receiver] received trama: F: %x A: %x C: %x BCC: %x \n", trama_set->F, trama_set->A, trama_set->C, trama_set->BCC);
    if((trama_set->A ^ trama_set->C) != trama_set->BCC){
        printf("[receiver] BCC invalid\n");
    }
}

int send_ua_message(int fd, char * buf, struct trama_s * trama_ua){
    trama_ua->C = UA;
    trama_ua->BCC = trama_ua->A ^ trama_ua->C;
    if(write(fd,trama_ua,sizeof(trama_ua)) <= 0)
        return 1;
    printf("[receiver] sent trama-> F: %x A: %x C: %x BCC: %x\n", trama_ua->F, trama_ua->A, trama_ua->C, trama_ua->BCC);
    
}

int receiver_connect (int fd, char * buf[]){

    struct trama_s * trama = malloc(sizeof(struct trama_s));

    wait_set_message(fd, buf, trama);

    send_ua_message(fd, buf, trama);

    free(trama);

    return 0;
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

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

    newtio.c_cc[VTIME] = 0; // inter-character timer unused /
    newtio.c_cc[VMIN] = 1; // blocking read until 5 chars received /



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
    */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
    }

    if(receiver_connect(fd, buf)){
        perror("[receiver] Connect failed!");       
        return -1;
    }

    /*
    printf("New termios structure set\n");
    int i = 0;
    while (STOP==FALSE) { // loop for input /
        res = read(fd,&buf[i],1); // returns after 5 chars have been input /
        //buf[i] = 0; / so we can printf... /
        if (buf[i]=='\0') STOP=TRUE;
        i++;
    }
    printf("%s\n", buf);

    res = write(fd,buf,255);
    for(int i = 0; i < strlen(buf); i++){
        write(fd,buf[i],1);
    }
    */    

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}

