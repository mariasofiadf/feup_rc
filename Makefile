all: app

app: app.c protocol.c receiver.c transmitter.c
	gcc -o app app.c protocol.c receiver.c transmitter.c

clean:
	rm app