all: app

app: app.c protocol.c receiver.c issuer.c
	gcc -o app app.c protocol.c receiver.c issuer.c

clean:
	rm app