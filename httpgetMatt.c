#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "client_util.h"
#include <arpa/inet.h>

#define PORT "http" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define HEADER_LINE_SIZE 1024

void handle_response(int sock, char* fileToWrite);


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int main(int argc, char *argv[]) {
	int sockfd;  
	char buf[MAXDATASIZE];
	char *fileName,
         *address,
         *time;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	FILE *fp;
	char s[INET_ADDRSTRLEN];

    /* address and fileName should be freed. */
	parse_addr(argc, argv, &address, &fileName);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(address, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
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
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure
	
	// Create HTTP request message using inputted args
	snprintf(buf, sizeof buf,
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Accept: text/plain, text/html\r\n"
		"Accept-Charset: *\r\n"
		"Accept-Encoding: *\r\n"
		"AcceptLanguage: en\r\n"
		"From: twoBerksAndAWilson\r\n"
		"User-Agent: customHTTPClient/0.1\r\n",
        fileName, address);
	
	if (parse_time(argc, argv, &time)) {
		sprintf(buf+strlen(buf), "If-modified-since: ");
		sprintf(buf+strlen(buf), time);
        free(time);
	}
	
	sprintf(buf+strlen(buf), "\r\n\r\n");
	
	// Send HTTP request messages
	sendall(sockfd, buf, strlen(buf));
	
	// Check for incoming messages
	//rv = recv(sockfd, buf, MAXDATASIZE, 0);
	//if ( rv != 0) {
		handle_response(sockfd, buf);
		printf(buf);
		fp = fopen(fileName, "wb");
		fprintf(fp, buf);
		fclose(fp);
	//}
	
	close(sockfd);

	return 0;
}
/* Handles all the logic associated with the header response file. 
If there is an error message, it prints it. If not, it reads the file,
and returns the string that needs to be written to fileToWrite
*/
void handle_response(int sock, char* fileToWrite){
	char buf [HEADER_LINE_SIZE];
	char *p1;
	char *p2;
	int length;
	int n;


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
	}
	//If the response is good and we are receiving the file
	else{
		printf("Reading File");
		
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
	/* Read the file  from the buffer*/
	n = recv(sock, fileToWrite, length, 0);
	if (n < 1){
	fprintf(stderr, "Client Error receiving file");
	
	}
}

