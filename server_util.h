#ifndef SERVER_UTIL
#define SERVER_UTIL

int recv_getline(int sock, char *buf, int buflen);
void sendall(int sock, char *buf, int buflen);
void die_error(int sock, int code, char *desc);

#endif /* SERVER_UTIL */
