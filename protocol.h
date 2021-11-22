#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "transmitter.h"
#include "receiver.h"

int send_trama(int fd, char * buf, int length);

int port_connect(char** argv);

int llopen(char *port, int mode);

//returns size of stuffed data
int stuffing(char * data, char * stuf_data, unsigned int data_size);

int destuffing(char * stuf_data, char * destuf_data, unsigned int data_size);

int send_ua(int fd);

int send_disc(int fd);

int wait_disc(int fd);

#endif