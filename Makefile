CC = gcc
CFLAGS = -Wall -std=c99

all: parentProcess primaryLikesServer

parentProcess: parentProcess.c
	$(CC) $(CFLAGS) -o parentProcess parentProcess.c

primaryLikesServer: primaryLikesServer.c
	$(CC) $(CFLAGS) -o primaryLikesServer primaryLikesServer.c

run: all
	./parentProcess & ./primaryLikesServer

clean:
	rm -f parentProcess primaryLikesServer
