#define _XOPEN_SOURCE /* for strptime */
#define _XOPEN_SOURCE_EXTENDED /* for strdup */
#include "server_util.h"
#include "header_parse.h"
#include <time.h>
#include <string.h>
#include <strings.h> /* strcasecmp */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


int parse_headers(int sock, struct req_info *info) {
    char buf[HEADER_LINE_SIZE];
    char *p1,
         *p2,
         *p3;
    int st;


    if ((st = recv_getline(sock, buf, HEADER_LINE_SIZE)) < 0) {
        die_error(sock, 400, "Bad request");
    } else if (st == 0) {
        close(sock);
        exit(0);
    }

    p1 = strstr(buf, " ");
    if (p1 == NULL || strncmp(buf, "GET", p1-buf) != 0) {
        die_error(sock, 405, "Method not allowed");
    }
    info->method = GET;

    ++p1;
    p2 = strstr(p1, " ");
    if (p2 == NULL) {
        die_error(sock, 400, "Bad request");
    }
    *p2 = '\0';

    info->resource = strdup(p1);
    info->http_ver = strdup(p2+1);

    /* HTTP/1.1 defaults to persistent connections */
    if (strcmp(info->http_ver, "HTTP/1.1") == 0) {
        info->want_persistent = 1;
    }

    while (1) {
        if ((st = recv_getline(sock, buf, HEADER_LINE_SIZE)) < 0) {
            die_error(sock, 400, "Bad request");
        } else if (st == 0) {
            close(sock);
            exit(0);
        }
        /* Empty line, we've reached the end of the headers. */
        if (buf[0] == 0) {
            break;
        }

        p1 = buf;
        while (isspace(*p1)) ++p1; /* Find start of header */
        p2 = p3 = strstr(p1, ":");
        --p2;
        while (isspace(*p2)) --p2; /* Find end of header key */
        *(p2+1) = '\0';          /* Mark it */
        ++p3;
        while (isspace(*p3)) ++p3; /* Find start of header value */

        if (!parse_single_header(p1, p3, info)) {
            die_error(sock, 501, "Not implemented");
        }

    }

    /* HTTP/1.1 explicitly requires the Host: header */
    if (strcasecmp(info->http_ver, "HTTP/1.1") == 0 && !info->found_host) {
        die_error(sock, 400, "Bad request");
    }

    return 1;
}

int parse_single_header(char *key, char *val, struct req_info *info) {
    if (strcasecmp(key, "If-Modified-Since") == 0) {
        info->if_modified_since = malloc(sizeof(*info->if_modified_since));
        if (!info->if_modified_since) {
            fprintf(stderr, "Out of memory.\n");
        } else {
            parse_time(val, info->if_modified_since);
        }
    }
    if (strcasecmp(key, "User-Agent") == 0) {
        char *ua_end = val + strlen(val) - 1;
        while (isspace(*ua_end)) --ua_end;
        *(ua_end+1) = '\0';

        info->user_agent = malloc(ua_end-val + 1);
        if (info->user_agent) {
            strcpy(info->user_agent, val);
        } else {
            fputs("Out of memory\n", stderr);
        }
    }
    if (strcasecmp(key, "Host") == 0) {
        info->found_host = 1;
    }
    if (strcasecmp(key, "Connection") == 0) {
        if (strcmp(val, "close") == 0) {
            info->want_persistent = 0;
        }
        else if (strcmp(val, "Keep-Alive") == 0) {
            info->want_persistent = 1;
        }
    }
    /* Cookies are explicitly not supported */
    if (strcasecmp(key, "Cookie") == 0) {
        return 0;
    }
    /* Ignore all other headers */
    return 1;
}


char *parse_time(char *timestr, struct tm *out) {
    char *ret;

    /* RFC 822 / RFC 1123 */
    ret = strptime(timestr, "%a, %d %b %Y %T GMT", out);
    if (ret) return ret;

    /* RFC 850 / RFC 1036 */
    ret = strptime(timestr, "%a, %d-%b-%y %T GMT", out);
    if (ret) return ret;

    /* asctime() format */
    ret = strptime(timestr, "%a %b %d %T %Y", out);
    return ret;
}

void setup_req_info(struct req_info *info) {
    /* I set all the pointers to NULL in case they're not set when I free them
     * in clear_req_info(). free(NULL) is a nop, by the standard. */
    info->method = INVALID;
    info->resource = NULL;
    info->http_ver = NULL;
    info->user_agent = NULL;
    info->want_persistent = 0;
    info->found_host = 0;
    info->if_modified_since = NULL;
}
void clear_req_info(struct req_info *info) {
    free(info->resource);
    free(info->http_ver);
    free(info->user_agent);
    free(info->if_modified_since);
    setup_req_info(info);
}
