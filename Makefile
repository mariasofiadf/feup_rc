all: send receiver

send: issuer.c
	gcc -o issuer issuer.c

receiver: receiver.c
	gcc -o receiver receiver.c

clean:
	rm issuer receiver