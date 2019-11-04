CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-pthread

HTTP-ping: main.c utils.c
	$(CC) $(CFLAGS) -o HTTP-ping main.c utils.c $(LDFLAGS)

HTTP-ping-test:
	$(CC) $(CFLAGS) -o HTTP-ping-test utils.c Test/TestMain.c Test/CuTest.c

all:	HTTP-ping HTTP-ping-test

clean:
	$(RM) HTTP-ping 
	$(RM) HTTP-ping-test




