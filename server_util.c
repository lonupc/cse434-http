#include "server_util.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SERVER_HEADERS \
    "Server: http_server/1.0\r\n" \
    "\r\n"

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

void die_error(int sock, int code, char *desc) {
    char buf[32];
    printf("dieing with error %d.\n", code);
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", code);

    sendall(sock, buf, strlen(buf));
    sendall(sock, desc, strlen(desc));
    sendall(sock, "\r\n", 2);

    sendall(sock, SERVER_HEADERS, sizeof(SERVER_HEADERS) - 1);

    close(sock);
    exit(2);
}
