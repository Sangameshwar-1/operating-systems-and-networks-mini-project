# Networking Makefile
# Compiles S.H.A.M. protocol implementation

CC = gcc
CFLAGS = -std=c99 \
         -D_POSIX_C_SOURCE=200809L \
         -D_XOPEN_SOURCE=700 \
         -Wall -Wextra \
         -Wno-unused-parameter \
         -Wno-deprecated-declarations \
         -fno-asm

# Link with crypto library for MD5
LDFLAGS = -lcrypto

TARGETS = server client

.PHONY: all clean

all: $(TARGETS)

server: server.c sham.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

client: client.c sham.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) server_log.txt client_log.txt *.o

test: all
	@echo "Run server: ./server <port> [--chat] [loss_rate]"
	@echo "Run client: ./client <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]"
	@echo "Run client chat: ./client <server_ip> <server_port> --chat [loss_rate]"
