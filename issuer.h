#ifndef TRANSMITTER_H
#define TRANSMITTER_H

void sig_handler(int signum);

int llopen_transmitter (char * port);

int send_set(int fd);

int wait_ua(int fd);

int llwrite(int fd, char * buffer, int length);

int send_disc(int fd);

int llclose(int fd);

#endif