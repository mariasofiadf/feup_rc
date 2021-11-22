#ifndef TRANSMITTER_H
#define TRANSMITTER_H

void sig_handler(int signum);

int llopen_transmitter (int fd);

int send_set(int fd);

int llwrite(int fd, unsigned char * buffer, int length);

int llclose(int fd);

#endif