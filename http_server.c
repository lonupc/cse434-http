/* Much of the structure of the networking code in main() and do_bind() is from
 * Beej's Guide to Network Programming, at http://beej.us/guide/bgnet/ */
#include "header_parse.h"
#include "server_util.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
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
/* Reap children left over from fork() */
void reap_children(int sig);
/* Utility function for IPv4/IPv6 independence */
void *get_in_addr(struct sockaddr *sa);
/* Main service function */
void serve(int sock, struct sockaddr_storage addr, socklen_t len);


int main() {
    int serv_sock;
    int client_sock;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    
    struct sigaction sa;

    serv_sock = do_bind();

    if (listen(serv_sock, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    /* Reap child processes when I fork */
    sa.sa_handler = reap_children;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(2);
    }

    while (1) {
        client_len = sizeof client_addr;
        client_sock = accept(serv_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        if (!fork()) {
            /* Client process */
            close(serv_sock);
            serve(client_sock, client_addr, client_len);
            exit(0);
        }

        /* I don't need their socket here. */
        close(client_sock);
    }

    return 0;
}


void serve(int sock, struct sockaddr_storage addr, socklen_t len) {
    char s[INET6_ADDRSTRLEN];
    struct req_info info;

    inet_ntop(addr.ss_family, get_in_addr((struct sockaddr *)&addr),
            s, sizeof s);
    printf("Got connection from %s\n", s);
    setup_req_info(&info);

    parse_headers(sock, &info);

    if (info.user_agent) {
        printf("User agent \"%s\" requested \"%s\"\n", info.user_agent, info.resource);
    } else {
        printf("Got request \"%s\"\n", info.resource);
    }

    clear_req_info(&info);
    close(sock);
}



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
        if (bind(ret, p->ai_addr, p->ai_addrlen) == -1) {
            close(ret);
            perror("bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Error: failed to bind to a port.\n");
        exit(1);
    }

    freeaddrinfo(servinfo);
    return ret;
}

/* A few utility functions */
void reap_children(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
