all: send receiver

send: writenoncanonical.c
	gcc -o send writenoncanonical.c

receiver: noncanonical.c
	gcc -o receiver noncanonical.c

clean:
	rm send receiver