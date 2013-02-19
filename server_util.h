#ifndef SERVER_UTIL
#define SERVER_UTIL

#define SERVER_HEADER \
    "Server: http_server/1.0\r\n"

int recv_getline(int sock, char *buf, int buflen);
void sendall(int sock, char *buf, int buflen);
void die_error(int sock, int code, char *desc);

#endif /* SERVER_UTIL */
