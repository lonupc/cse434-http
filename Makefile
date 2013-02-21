TARGETS = http_server httpget
CC = gcc
CFLAGS += -Wall -Wextra -pedantic -ggdb -std=gnu99


all: $(TARGETS)


http_server: http_server.o server_util.o header_parse.o

http_server.o: server_util.h header_parse.h
server_util.o: server_util.h
header_parse.o: server_util.h header_parse.h

httpget: httpgetMatt.o client_util.o
	$(CC) $(CFLAGS) $^ -o httpget

httpgetMatt.o: client_util.h
client_util.o: client_util.h

clean:
	-rm $(TARGETS) *.o
