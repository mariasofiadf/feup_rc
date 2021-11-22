#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include<signal.h>

#include "header.h"


u_int8_t RR_COUNT = RR_ZERO;

int BCC2_NOK = -65;

//RECEIVER

int llopen_receiver(int fd)
{
    wait_set(fd);

    send_ua(fd);

    return 0;
}

int wait_set(int fd)
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
            if (rcv == SET)
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
        printf("Received SET message\n");
    return 0;
}


int send_rr(int fd)
{
    unsigned char rr_message[5];
    rr_message[0] = FLAG;
    rr_message[1] = A_ISSUER;
    rr_message[2] = RR_COUNT;
    rr_message[3] = A_ISSUER ^ RR_COUNT;
    rr_message[4] = FLAG;

    send_trama(fd, rr_message, 6);

    if(DEBUG)
        printf("Sent RR message\n");

    return 0;
}

int bcc2_ok(unsigned char*buffer, int length){
    unsigned char bcc2 = buffer[1]; //buffer[0] is BCC1
    for(int i = 2; i < length-1; i++){
        bcc2 = bcc2 ^ buffer[i];
    }
    if(bcc2 == buffer[length-1])
        return 1;
    return 0;
}



wait_info(int fd, unsigned char *buffer, int size){
    
    unsigned char rcv = "";
    int finished = 0, count = 0;
    unsigned char a ="", c ="",bcc="", bcc2="";
    enum state state = START;
    unsigned char * destuffed;
    unsigned char stuffed[size*2];
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
            if (rcv == C_ZERO || rcv == C_ONE)
            {
                c = rcv;
                state = RECEIVING;
            }
            else if (rcv == FLAG)
                state = FLAG_RCV;
            else if (rcv == DISC)
                state = DISC_ST;
            else
                state = START;
            break;
        case RECEIVING: //RECEIVING
            if (rcv == FLAG)
            {
                state = DATA_RCV; //2nd Flag received
            }
            else
            {
                stuffed[count] = rcv;
                count++;
            }
            break;
        case DATA_RCV://TODO FLAG_RCV
            //printf("DATA_RCV\n");
            destuffed = malloc(count);
            int destuff_size = destuffing(stuffed, destuffed, count);
            if(destuffed[0] != a^c)//BCC1 check
                state = START;
            if((c == C_ONE && RR_COUNT==RR_ZERO)||(c == C_ZERO && RR_COUNT==RR_ONE)){
                printf("Received %d repeated information bytes correctly\n", destuff_size-2);
                return 0;
            }
            if (bcc2_ok(destuffed, destuff_size))
            {
                
                for(int i = 0; i < destuff_size-2; i++){ //Copies data to buffer (removing bcc's)
                    buffer[i] = destuffed[i+1];
                }
                printf("Received %d information bytes correctly\n", destuff_size-2);
                if(RR_COUNT==RR_ONE)
                    RR_COUNT = RR_ZERO;
                else
                    RR_COUNT = RR_ONE;
                free(destuffed);
                return destuff_size-2;
            }
            else{
                free(destuffed);
                printf("Received %d information bytes INCORRECTLY\n", destuff_size-2);
                return BCC2_NOK;
            }
            break;
        case DISC_ST:
            printf("Received DISC message\n");
            send_disc(fd);
            wait_ua(fd);
            return -1;
            break;
        default:
            break;
        }
    }

    return -1;

}

int send_rej(int fd)
{
    unsigned char rej_message[5];
    rej_message[0] = FLAG;
    rej_message[1] = A_ISSUER;
    rej_message[2] = RR_COUNT & REJ_MASK; // if RR_COUNT==RR_ZERO then = REJ_ZERO
    rej_message[3] = A_ISSUER ^ rej_message[2];
    rej_message[4] = FLAG;

    send_trama(fd, rej_message, 6);

    if(DEBUG)
        printf("Sent REJ message\n");

    return 0;
}

int llread(int fd, unsigned char *buffer, int size)
{

    int r = wait_info(fd, buffer, size);

    if(r>=0)
        send_rr(fd);
    else if(r == BCC2_NOK){
        send_rej(fd);
        return 0;
    }
    return r;
}