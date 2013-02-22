/* Much of the structure of the networking code in main() and bind_to_self() is
 * from Beej's Guide to Network Programming, at http://beej.us/guide/bgnet/ */
#include "header_parse.h"
#include "server_util.h"
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
int bind_to_self();
/* Reap children left over from fork() */
void reap_children(int sig);
/* Utility function for IPv4/IPv6 independence */
void *get_in_addr(struct sockaddr *sa);
/* Main service function */
void serve(int sock, struct sockaddr_storage addr, char *serv_root);


int main() {
    int serv_sock;
    int client_sock;
    struct sockaddr_storage client_addr;
    socklen_t client_len;
    /* It would be easy enough to add support for an "htdocs" directory */
    char server_root[1024];
    
    struct sigaction sa;

    getcwd(server_root, sizeof(server_root));

    serv_sock = bind_to_self();

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
            serve(client_sock, client_addr, server_root);
            exit(0);
        }

        /* I don't need their socket here. */
        close(client_sock);
    }

    return 0;
}


void serve(int sock, struct sockaddr_storage addr, char *serv_root) {
    char s[INET6_ADDRSTRLEN];
    struct req_info info;
    FILE *fp;
    char buf[1024];
    int persistent_request;

    inet_ntop(addr.ss_family, get_in_addr((struct sockaddr *)&addr),
            s, sizeof s);
    printf("Got connection from %s\n", s);
    setup_req_info(&info);

    /* Persistent request loop */
    do {
        struct stat stat_buf;

        parse_headers(sock, &info);

        if (info.user_agent) {
            printf("User agent \"%s\" requested \"%s\" with %s\n", info.user_agent, info.resource, info.http_ver);
        } else {
            printf("Got request \"%s\" with %s\n", info.resource, info.http_ver);
        }

        strncpy(buf, serv_root, sizeof buf);
        strncat(buf, info.resource, sizeof buf - strlen(buf) - 1);


considered_harmful:
        /* I could clean up this path, but I don't even care.
         * Security? Who needs it?
         */
        printf("Offering file \"%s\"\n", buf);
        if (stat(buf, &stat_buf) == -1) {
            if (errno == ENOENT) {
                die_error(sock, 404, "Not found");
            }
            if (errno == EACCES) {
                die_error(sock, 403, "Forbidden");
            }
            printf("died on line %d\nerrno = %s\n", __LINE__, strerror(errno));
            die_error(sock, 500, "Internal server error");
        }

        /* Is it world-readable? */
        if ((stat_buf.st_mode & 0004) == 0) {
            die_error(sock, 403, "Forbidden");
        }

        /* We only support regular files */
        if (!S_ISREG(stat_buf.st_mode)) {
            /* If it's a directory, we'll try again */
            if (S_ISDIR(stat_buf.st_mode)) {
                strncat(buf, "index.html", sizeof buf - strlen(buf) - 1);
                goto considered_harmful;
            }
            die_error(sock, 500, "Internal server error");
        }

        if (info.if_modified_since) {
            time_t tmp = mktime(info.if_modified_since);
            if (tmp != -1 && tmp < stat_buf.st_mtime) {
                die_error(sock, 304, "Not modified");
            }
        }

        /* OK, from this point forth, we know the file's ok. */
        if (!(fp = fopen(buf, "rb"))) {
            die_error(sock, 500, "Internal server error");
        }

        snprintf(buf, sizeof buf, "HTTP/1.1 200 OK\r\n"
                SERVER_HEADER
                "Content-Length: %lu\r\n"
                "\r\n",
                (unsigned long)stat_buf.st_size);

        sendall(sock, buf, strlen(buf));
        printf("Sent headers:\n%s", buf);

        while (!feof(fp) && !ferror(fp)) {
            size_t nread = fread(buf, 1, sizeof buf, fp);
            sendall(sock, buf, nread);
        }

        persistent_request = info.want_persistent;
        clear_req_info(&info);
    } while (persistent_request);

    close(sock);
}



int bind_to_self() {
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
    (void)sig; /* Get rid of unused parameter warning */
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
