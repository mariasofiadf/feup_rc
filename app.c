
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>
#include <string.h>

#include "protocol.h"

unsigned long int MAX_DATA = 200000;

enum mode{
    TRANSMITTER,
    RECEIVER
};

enum control{
    DATA,
    START,
    END
};

enum type{
    FILE_SIZE,
    FILE_NAME
};

enum mode mode; // TRANSMITTER|RECEIVER
char filename[100], port[100];


void printUsage(){
    printf("Usage:  app receiver|transmitter serialPort filename\n");
    printf("\tex: app receiver /dev/ttyS0\n");
    printf("\tex: app t /dev/ttyS0 myfile.jpg\n");
}

int readArgs(int argc, char**argv){
    if(argc != 4 && argc != 3)
        return 1;
    if(!strcmp(argv[1],"receiver") || !strcmp(argv[1],"r"))
        mode = RECEIVER;
    else if (!strcmp(argv[1],"transmitter") || !strcmp(argv[1], "t"))
        mode = TRANSMITTER;
    else
        return 1;
        
    strcpy(port, argv[2]);
    if(mode==TRANSMITTER)
        strcpy(filename,argv[3]);
    
    return 0;
}

int CPACKET_SIZE = 300;

int transmitter(){ 
    int file = open(filename, O_RDONLY); 

    int fd = llopen(port, mode);

    unsigned char cpacket[CPACKET_SIZE];
    cpacket[0] = START; //C
    cpacket[1] = FILE_SIZE; //T
    cpacket[2] = sizeof(MAX_DATA); //L
    int i = 0;
    for(;i< sizeof(MAX_DATA);i++){
        cpacket[i+3] = MAX_DATA >> 8*(sizeof(MAX_DATA) - i - 1);
    }
    i += 3;
    cpacket[i] = FILE_NAME; //T2
    cpacket[i+1] = strlen(filename); //L2
    int j = i + 2;


    for(int x = 0; x < strlen(filename); x++){
        cpacket[j+x] = filename[x];
    }
    
    llwrite(fd,&cpacket,j+strlen(filename));    

    int DPACKET_SIZE = MAX_DATA+4;
    unsigned char dpacket[DPACKET_SIZE];
    unsigned char * ptr = &dpacket;
    int r = 0;
    while((r = read(file, ptr+4, MAX_DATA)) > 0){
        dpacket[0] = 1;
        dpacket[1] = MAX_DATA >> 8;
        dpacket[2] = MAX_DATA & 0x00ff;
        dpacket[3] = 1;
        DPACKET_SIZE = r + 4;
        llwrite(fd,&dpacket,DPACKET_SIZE);
        //usleep(1000000);
        memset(&dpacket, '\0', MAX_DATA+4);
    }

    cpacket[0] = END;
    llwrite(fd,&cpacket,j+strlen(filename));

    llclose(fd);

    close(file);
}

int receiver(){ //Give permission to read and write to owner

    int fd = llopen(port, mode);

    unsigned char cpacket[CPACKET_SIZE];
    int r = 0;
    while(r<=0){
        r = llread(fd,&cpacket,CPACKET_SIZE);
    }

    if(cpacket[0]!=START) //START EXPECTED
        return 1;
    unsigned long int filesize = 0;
    int i = 0;
    if(cpacket[1] == FILE_SIZE){//cpacket[1]->T
        for(; i < cpacket[2]; i++){ //cpacket[2]->L
            filesize = filesize | (cpacket[i+3] << (cpacket[2]-i-1)*8);
        }
    }

    char filen[200];

    if(cpacket[i+3] == FILE_NAME){//cpacket[i+3]->T2
        int j = i+4;
        for(int x = 0; x < cpacket[i+4]; x++){
            filen[x] = cpacket[x+i+5];
        }
    }

    int file = open(filen, O_WRONLY | O_CREAT, 0644);

    unsigned char dpacket[MAX_DATA+4];

    unsigned char * ptr = &dpacket;
    r=0;
    while ( (r = llread(fd, &dpacket,MAX_DATA+4)) >= 0)
    {
        /* code */
        if(dpacket[0] == END)
            continue;
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