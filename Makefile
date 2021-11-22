CC = gcc
CFLAGS  = -g -Wall
all: app

app: app.c protocol.c receiver.c transmitter.c
	$(CC) $(CFLAGS) -o app app.c protocol.c receiver.c transmitter.c

clean:
	rm app