
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>
#include <string.h>

#include "protocol.h"

#define MAX_DATA 200

enum mode{
    TRANSMITTER,
    RECEIVER
};

enum mode mode; // TRANSMITTER|RECEIVER
char filename[100], port[100];


void printUsage(){
    printf("Usage:  app receiver|transmitter serialPort filename\n");
    printf("\tex: app receiver /dev/ttyS0 myfile.txt\n");
    printf("\tex: app t /dev/ttyS0 myfile.txt\n");
}

int readArgs(int arc, char**argv){
    if(arc != 4)
        return 1;
    if(!strcmp(argv[1],"receiver") || !strcmp(argv[1],"r"))
        mode = RECEIVER;
    else if (!strcmp(argv[1],"transmitter") || !strcmp(argv[1], "t"))
        mode = TRANSMITTER;
    else
        return 1;
        
    strcpy(port, argv[2]);
    strcpy(filename,argv[3]);
    
    return 0;
}

int transmitter(){ 
    int file = open(filename, O_RDONLY); 
    int PACKET_SIZE = MAX_DATA+4;

    unsigned char dpacket[PACKET_SIZE];
    unsigned char * ptr = &dpacket;

    unsigned char data[MAX_DATA];

    int fd = llopen(port, mode);

    int r = 0;
    while((r = read(file, ptr+4, MAX_DATA)) > 0){
        dpacket[0] = 1;
        dpacket[1] = MAX_DATA >> 8;
        dpacket[2] = MAX_DATA & 0x00ff;
        dpacket[3] = 1;
        PACKET_SIZE = r + 4;
        llwrite(fd,&dpacket,PACKET_SIZE);
        //usleep(1000000);
        memset(&dpacket, '\0', MAX_DATA+4);
    }

    llclose(fd);

    close(file);
}

int receiver(){
    int file = open(filename, O_WRONLY | O_CREAT, 0644); //Give permission to read and write to owner

    unsigned char dpacket[MAX_DATA+4];

    unsigned char * ptr = &dpacket;

    int fd = llopen(port, mode);

    int r=0;
    while ( (r = llread(fd, &dpacket,MAX_DATA+4)) >= 0)
    {
        /* code */
        if(r > 0){
            write(file, ptr+4, r-4);
            memset(&dpacket, '\0', MAX_DATA+4);
        }
    }

    close(file);
}

int main(int arc, char**argv){
    if(readArgs(arc, argv))
        printUsage();

    if(mode == RECEIVER)
        receiver();
    else if(mode == TRANSMITTER)
        transmitter();
    else return 1;

    return 0;

}