#include "server_util.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Not the most efficient code, reads one byte at a time. But it works. */
int recv_getline(int sock, char *buf, int buflen) {
    int n;
    int i = 0;

    while (i++ < buflen-1) {
        n = recv(sock, buf, 1, 0);
        if (n < 1) return n;
        if (*buf++ == '\n')
            break;
    }

    if (*(buf-1) == '\n') {
        /* Gracefully handle just \n instead of \r\n */
        if (*(buf-2) == '\r') {
            *(buf-2) = '\0';
        } else {
            *(buf-1) = '\0';
        }
        return 1;
    } else {
        *buf = '\0';
        return -1;
    }
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
    char htmlbuf[1024];
    printf("Returning status %d.\n\n", code);
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", code);
    snprintf(htmlbuf, sizeof htmlbuf, ERROR_HTML, code, code, desc);

    sendall(sock, buf, strlen(buf));
    sendall(sock, desc, strlen(desc));
    sendall(sock, "\r\n", 2);

    /* n.b. -1 for the trailing \0 */
    sendall(sock, SERVER_HEADER, sizeof(SERVER_HEADER) - 1);

    snprintf(buf, sizeof buf, "Content-Length: %lu\r\n", (unsigned long)strlen(htmlbuf));
    sendall(sock, buf, strlen(buf));
    sendall(sock, "\r\n", 2);

    /* Send a helpful friendly HTML file */
    sendall(sock, htmlbuf, strlen(htmlbuf));

    close(sock);
    exit(2);
}
