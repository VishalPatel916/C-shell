CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: server client

common: sham_common.c
	$(CC) $(CFLAGS) -c sham_common.c -o sham_common.o

server: server.c common
	$(CC) $(CFLAGS) server.c sham_common.o -o server

client: client.c common
	$(CC) $(CFLAGS) client.c sham_common.o -o client

clean:
	rm -f server client sham_common.o server_log.txt client_log.txt

.PHONY: all clean