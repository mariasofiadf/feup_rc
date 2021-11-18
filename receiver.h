#ifndef RECEIVER_H
#define RECEIVER_H

//RECEIVER

int llopen_receiver(int fd);

int wait_set(int fd);

int send_ua(int fd);

int llread(int fd, char *buffer);

#endif