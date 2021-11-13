all: send receiver

send: issuer.c
	gcc -o issuer issuer.c common.c

receiver: receiver.c
	gcc -o receiver receiver.c common.c

clean:
	rm issuer receiver