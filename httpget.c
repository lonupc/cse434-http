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

int main(int argc, char *argv[]) {
	int serv_sock;
	int client_sock;
	struct sockaddr_storage client_addr;

	client_sock = do_bind();

	//argc is the number of arguements passed into the command line
	//argv[1] is the first arguement

	//Connect to the server


	

}

/* Returns the name of the file by traversing the arguements
and determining which one is the file we're requesting */

char *getFile(int argc, char * argv[]){
	int i;

	for(i = 0; i < argc; i++){
		
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

