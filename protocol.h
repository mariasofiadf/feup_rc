#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "transmitter.h"
#include "receiver.h"

int send_trama(int fd, char * buf, int length);

int port_connect(char** argv);

int llopen(char *port, int mode);


#endif