#define _XOPEN_SOURCE /* for strptime */
#include "server_util.h"
#include "header_parse.h"
#include <time.h>
#include <string.h>
#include <strings.h> /* strcasecmp */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define HEADER_LINE_SIZE 1024


int parse_headers(int sock, struct req_info *info) {
    char buf[HEADER_LINE_SIZE];
    char *p1,
         *p2,
         *p3;


    if (!recv_getline(sock, buf, HEADER_LINE_SIZE)) {
        die_error(sock, 400, "Bad request");
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

    info->resource = malloc((p2-p1) + 1);
    if (info->resource == NULL) {
        die_error(sock, 500, "Internal server error");
    }
    strncpy(info->resource, p1, p2-p1);
    info->resource[p2-p1] = '\0';



    while (1) {
        if (!recv_getline(sock, buf, HEADER_LINE_SIZE)) {
            die_error(sock, 400, "Bad request");
        }
        if (strcmp(buf, "\r\n") == 0) {
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

        parse_single_header(p1, p3, info);

    }

    return 1;
}

void parse_single_header(char *key, char *val, struct req_info *info) {
    if (strcasecmp(key, "If-Modified-Since") == 0) {
        parse_time(val, &info->if_modified_since);
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
    info->resource = info->user_agent = NULL;
}
void clear_req_info(struct req_info *info) {
    free(info->resource);
    free(info->user_agent);
    setup_req_info(info);
}
