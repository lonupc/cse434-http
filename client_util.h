#ifndef CLIENT_UTIL
#define CLIENT_UTIL

#define ARGUMENT_MAX 50

void parse_addr(int argc, char * argv[], char ** address, char ** fileName);
int parse_time(int argc, char * argv[], char * time); 
int recv_getline(int sock, char *buf, int buflen);
void sendall(int sock, char *buf, int buflen);

#endif /* CLIENT_UTIL */
