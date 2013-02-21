#include "client_util.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/*  If there is a command line argument that is not a URL,
	we assume that it is a time. This manipulates a "time" string
	and returns 1 if there is a time and 0 if there isn't*/ 
int parse_time(int argc, char * argv[], char * time){
	int i;

	for (i = 1; i < argc; i++){
		if(strncmp("h", argv[i], 1))
			break;
	}
	
	//If there is no time requirement
	if (i == argc)
	 	return 0;

	strcpy(time, argv[i]);
	return 1;
}

/* This searchese the arguements and determines if there is a well-formed
	url. If there is, it sets the address and fileName strings, if not, 
	it exits the program*/ 
void parse_addr(int argc, char * argv[], char ** address, char ** fileName)
{
	int i;
    char *addrStart, *addrEnd;
	
	for(i = 1; i < argc; i++){
		if (!strncmp ("http://", argv[i], 7))
            break;
	}

	if (i == argc){
		fprintf(stderr, "No valid web address");
        exit(1);
	}

    addrStart = argv[i] + sizeof "http://" - 1;
    addrEnd = strstr(addrStart, "/");
	if(addrEnd == NULL) {
		fprintf(stderr, "Invalid web address and file format\n");
		exit(1);
	}

    *fileName = strdup(addrEnd);
    *addrEnd = '\0';
    *address = strdup(addrStart);
}

/* Not the most efficient code, reads one byte at a time. But it works. */
int recv_getline(int sock, char *buf, int buflen) {
    int n;
    int i = 0;

    while (i++ < buflen-1) {
        n = recv(sock, buf, 1, 0);
        if (n < 1) return 0;
        if (*buf++ == '\n')
            break;
    }

    *buf = '\0';
    return (*(buf-1) == '\n');
}

void sendall(int sock, char *buf, int buflen) {
    int nch;
    while (buflen) {
        if ((nch = send(sock, buf, buflen, 0)) >= 0) {
            buflen -= nch;
            buf += nch;
        } else {
            perror("send");
            close(sock);
            exit(2);
        }
    }
}
