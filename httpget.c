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

#define PORT "http"
#define BACKLOG 10

/* Get a socket and bind to it. */
int do_bind();

/* Parses the headers and sets the address and the file Name */
void parseAddr(int argc, char * argv[], char * address, char * fileName);

int main(int argc, char *argv[]) {
	int serv_sock;
	int client_sock;
	struct sockaddr_storage server_addr;
	char * address, fileName;

	client_sock = do_bind();
	praseAddr(argc, argv[], address, fileName);
	
	
	
	
	

	//argc is the number of arguements passed into the command line
	//argv[1] is the first arguement

	//Connect to the server


	

}

/* Conn

/* Parse the command line arguements and return an array with the
IP address (in the first string), and the file name (in the second string) */

void parseAddr(int argc, char * argv[], char * address, char * fileName){
	int i;

	for(i = 0; i < argc; i++){
		if (!strncmp ("http://", argv[i], 7))
			break;//
	}
	if (i == argc){
	 perror("No valid web address");
        exit(1);
	}

	strtok(argv[i], "/");
	strtok(argv[i], "/");
	
	//At this point we should be through the "http://"
	address = strtok(argv[i], "/");
	if (!address || strcmp(address, ""){
		fprintf(stderr, "No valid address.\n");
		exit(1);
		}
	fileName = strtok(argv[i], "/");
	if (!fileName || strcmp(fileName, ""){
		fprintf(stderr, "Error: No Valid File Name.\n");
		exit(1);
		}
}

/* This creates a socket (copied from Lewis's file), but does not bind.
It is not necessary to bind in the client*/

int do_bind() {
    int ret;

    struct addrinfo hints;
    struct addrinfo *servinfo,
                    *p;
    int status;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE; /* bind to myself */
    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Get me a socket! */
        if ((ret = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        /* Lose "Address already in use" message */
        if (setsockopt(ret, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        /* Try to bind to the socket */
           break;
	}

    if (p == NULL) {
        fprintf(stderr, "Error: failed to bind to a port.\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    return ret;
}

