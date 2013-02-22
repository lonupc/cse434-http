#ifndef HEADER_PARSE
#define HEADER_PARSE

#define HEADER_LINE_SIZE 1024

#include <time.h>

/* For now we only support GET */
enum req_method { INVALID, GET };

struct req_info {
    enum req_method method;
    char *resource;
    char *http_ver;

    char *user_agent;
    int found_host;
    struct tm *if_modified_since;
};

void setup_req_info(struct req_info *info);
void clear_req_info(struct req_info *info);
int parse_headers(int sock, struct req_info *info);
/* N.b. this function might stomp all over key and val */
void parse_single_header(char *key, char *val, struct req_info *info);
char *parse_time(char *timestr, struct tm *out);

#endif /* HEADER_PARSE */
