/* A lot of the code in main for connecting to the host is structured using
code from Beej's Guide to Network Programming at http://beej.us/guide/bgnet/ */

#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client_util.h"

#define PORT "http" // the port client will be connecting to 
#define MAXDATASIZE 16384 // max number of bytes we can get at once 
#define HEADER_LINE_SIZE 1024 // max header size to read in

void handle_response(int sock, char* fileToWrite);
void *get_in_addr(struct sockaddr *sa);


int main(int argc, char *argv[]) {
	int client_sock;  
	char buf[MAXDATASIZE];
	char *fileName,
         *address,
         *time;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET_ADDRSTRLEN];

    /* address and fileName should be freed. */
	parse_addr(argc, argv, &address, &fileName);

	memset(&hints, 0, sizeof hints);	// Make sure hints is empty
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Get Address info
	if ((rv = getaddrinfo(address, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// Connect to first result possible
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((client_sock = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(client_sock, p->ai_addr, p->ai_addrlen) == -1) {
			close(client_sock);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("Client: connecting to %s\n.", s);

	freeaddrinfo(servinfo); // all done with this structure
	
	// Create HTTP request message using inputted args
	snprintf(buf, sizeof buf,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Accept: text/plain, text/html\r\n"
		"Accept-Charset: *\r\n"					// Set to '*' for simplicity
		"Accept-Encoding: *\r\n"				// Set to '*' for simplicity
		"AcceptLanguage: en\r\n"
		"From: twoBerksAndAWilson\r\n"
		"User-Agent: customHTTPClient/0.1\r\n",
        fileName, address);
	
	// If time is resent in the command ling arguments, add If-modified-since header
	if (parse_time(argc, argv, &time)) {
		sprintf(buf+strlen(buf), "If-modified-since: ");
		sprintf(buf+strlen(buf), time);
        free(time);
	}
	
	// Add crlf to end HTTP request message
	sprintf(buf+strlen(buf), "\r\n\r\n");
	
	// Send HTTP request messages
	sendall(client_sock, buf, strlen(buf));
	printf("HTTP request message sent");
	
	// Read HTTP response message
	handle_response(client_sock, fileName);
	
	// Close socket
	close(client_sock);

	return 0;
}

// Get address for the socket
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Handles all the logic associated with the header response file. 
If there is an error message, it prints it. If not, it reads the file,
and returns the string that needs to be written to fileToWrite
*/
void handle_response(int sock, char* fileName){
	char buf [HEADER_LINE_SIZE];
	char *p1;
	char *p2;
	int length;
	int n;
	FILE *fp;

	if (!recv_getline(sock, buf, HEADER_LINE_SIZE)){
		fprintf(stderr, "Client error receiving file");
		exit(1);
	}
    printf("Got initial header line \"%s\"\n", buf);

	p1 = strstr(buf, " ");
	if (p1 == NULL){
		fprintf(stderr, "Client error receiving file");
		exit(1);
	}

	/* If there isn't a normal status code, print it out and exit*/
	else if (strncmp (p1+1, "200 OK", 6) != 0){
		printf("Received error: \"%s\"", p1);
		exit(0);

	//If the response is good and we are receiving the file
	} else{
		printf("Reading File\n");
		
		while(1){
			if (!recv_getline(sock, buf, HEADER_LINE_SIZE)){
				fprintf(stderr, "Client error receiving file");
				exit(1);	
			}
			
			/* We've reached the end of the headers */
			if (*buf == '\0'){
				break;
			}
			/* The only header we care about is the content length */
			if (strncmp(buf, "Content-Length", 14) == 0){
				p2 = buf + 16;
				length = atoi(p2);
			}
		}	
	}
	/* Open file for reading */
	fp = fopen(basename(fileName), "wb");
	
	/* Get the file conents from the file in parts until entire file
	   is downloaded */
	while (length != 0) {
		n = recv(sock, buf, sizeof(buf)-1, 0);
        buf[n] = '\0';
		fputs(buf, fp);
		length -= n;
	}
	
	if (n < 1){
		fprintf(stderr, "Client Error receiving file");
	}
	printf("File written.\n");
}

