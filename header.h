#ifndef HEADER_FILE
#define HEADER_FILE

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0b01111110
#define A_ISSUER 0x03
#define A_RECEIVER 0x01

#define SET 0b00000011
#define DISC 0b00001011
#define UA 0b00000111
#define RR 0b00000101
#define REJ 0b00000001

#define C_ONE  0b01000000
#define C_ZERO 0b00000000


#define TRANSMITTER 0
#define RECEIVER 1

#include <stdlib.h>

struct trama_s
{
    char F;
    char A;
    char C;
    char BCC;  
};

struct trama_i
{
    char F;
    char A;
    char C;
    unsigned char * D;
    char BCC;  
};


enum state {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    DATA_RCV,
    DISC_ST,
    STOP_ST
};

#endif
