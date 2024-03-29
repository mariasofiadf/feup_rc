
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>
#include <string.h>
#include <sys/stat.h>

#include "protocol.h"

unsigned short int MAX_DATA = 1000;
unsigned long int filesize;

#define VERBOSE 1

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

int CPACKET_SIZE = 3000;

int send_ctrl_packet(int fd, enum control ctrl){
    struct stat sb;

    if (stat(filename, &sb) == -1) {
        perror("stat");
        exit(1);
    }

    unsigned long int filesize = sb.st_size;

    unsigned char cpacket[CPACKET_SIZE];
    if(ctrl == START)
        cpacket[0] = START; //C
    else
        cpacket[0] = END;
    cpacket[1] = FILE_SIZE; //T
    cpacket[2] = sizeof(filesize); //L
    int i = 0;
    for(;i< sizeof(filesize);i++){
        cpacket[i+3] = filesize >> 8*(sizeof(filesize) - i - 1);
    }
    i += 3;
    cpacket[i] = FILE_NAME; //T2
    cpacket[i+1] = strlen(filename); //L2
    int j = i + 2;


    for(int x = 0; x < strlen(filename); x++){
        cpacket[j+x] = filename[x];
    }
    
    llwrite(fd,&cpacket,j+strlen(filename));  


    if(VERBOSE) printf("[App] Sent control packet: filename: %s\tfilesize:%d\n", filename, filesize);
}

send_data_packets(int fd, int file){
    int DPACKET_SIZE = MAX_DATA+4;
    unsigned char dpacket[DPACKET_SIZE];
    unsigned char * ptr = &dpacket;
    int r = 0, counter = 0;
    while((r = read(file, ptr+4, MAX_DATA)) > 0){
        dpacket[0] = 1;
        dpacket[1] = counter % 255; 
        dpacket[2] = r >> 8;
        dpacket[3] = r & 0x00ff;
        DPACKET_SIZE = r + 4;
        llwrite(fd,&dpacket,DPACKET_SIZE);
        if(VERBOSE) printf("[App] Sent %d data bytes\n",r);
        memset(&dpacket, '\0', MAX_DATA+4);
        counter++;
    }
}


int wait_ctrl_packet(int fd){

    unsigned char cpacket[CPACKET_SIZE];
    int r = 0;
    while(r<=0){
        r = llread(fd,&cpacket,CPACKET_SIZE);
    }

    if(cpacket[0]!=START) //START EXPECTED
        return 1;
    filesize = 0;
    int i = 0;
    if(cpacket[1] == FILE_SIZE){//cpacket[1]->T
        for(; i < cpacket[2]; i++){ //cpacket[2]->L
            filesize = filesize | (cpacket[i+3] << (cpacket[2]-i-1)*8);
        }
    }

    if(cpacket[i+3] == FILE_NAME){//cpacket[i+3]->T2
        int j = i+4;
        for(int x = 0; x < cpacket[i+4]; x++){
            filename[x] = cpacket[x+i+5];
        }
    }

    if(VERBOSE) printf("[App] Received control packet: filename: %s\tfilesize:%d\n", filename, filesize);
    return filesize;
}

int wait_data_packets(int fd, int file){
    unsigned char dpacket[MAX_DATA+4];

    unsigned char * ptr = &dpacket;
    int r=0, counter = 0, data_rcv = 0, byte_counter = 0;
    while ( (r = llread(fd, &dpacket,MAX_DATA+4)) >= 0)
    {
        if(dpacket[0] == END)
            continue;
        data_rcv = (dpacket[2]<<8) | (dpacket[3]);
        byte_counter += data_rcv;
        if(VERBOSE) printf("[App] Received %d data bytes\n",data_rcv);
        counter = dpacket[1];
        if(r > 0){
            write(file, ptr+4, r-4);
            memset(&dpacket, '\0', MAX_DATA+4);
        }
    }
    if(byte_counter == filesize)
        printf("[App] Received %d bytes as expected\n", byte_counter);
    else printf("[App] Only %d bytes\n", byte_counter);
    return byte_counter;
}

int transmitter(){ 
    int file = open(filename, O_RDONLY); 

    int fd = llopen(port, mode);

    send_ctrl_packet(fd, START);

    send_data_packets(fd, file);

    send_ctrl_packet(fd, END);

    llclose(fd);

    close(file);
}


int receiver(){ //Give permission to read and write to owner

    int fd = llopen(port, mode);

    wait_ctrl_packet(fd);

    int file = open(filename, O_WRONLY | O_CREAT, 0644);

    wait_data_packets(fd, file);

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
