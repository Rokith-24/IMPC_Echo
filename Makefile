
CC = gcc
CFLAGS = -Wall -Wextra

SERVER = icmp_reply
CLIENT = icmp_request

all: $(SERVER) $(CLIENT)

$(SERVER): icmp_reply.c
	$(CC) $(CFLAGS) icmp_reply.c -o $(SERVER)

$(CLIENT): icmp_request.c
	$(CC) $(CFLAGS) icmp_request.c -o $(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT)

server: $(SERVER)
	sudo ./$(SERVER)

client: $(CLIENT)
	@echo "Usage: make client IP=192.168.1.10"
	sudo ./$(CLIENT) $(IP)

.PHONY: all clean server client
