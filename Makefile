TARGETS = http_server
CC = gcc
CFLAGS += -Wall -Wextra -pedantic -ggdb


all: $(TARGETS)


http_server: http_server.o server_util.o header_parse.o

http_server.o: server_util.h header_parse.h
server_util.o: server_util.h
header_parse.o: server_util.h header_parse.h

clean:
	-rm $(TARGETS) *.o
