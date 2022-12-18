//
// Created by argem on 01.12.2022.
//

#include "http_header_parser.h"

#include <string.h>

#define ELEMENT_TYPE header_t
#include "cvector_impl.h"

#define ELEMENT_TYPE char
#include "cvector_impl.h"

#include "common.h"


void
request_init(request_t* request){
    request->type = NULL;
    request->uri = NULL;
    request->version = NULL;
    request->body = NULL;
    vheader_t_init(&request->headers);
}

__attribute__((unused)) void
request_destroy(request_t* request){
    if(request == NULL){
        return;
    }
    free(request->type);
    free(request->uri);
    free(request->version);
    free(request->body);
    for(size_t i=0; i<request->headers.cnt; ++i){
        header_t* header = vheader_t_get(&request->headers, i);
        free(header->type);
        free(header->value);
    }
    vheader_t_free(&request->headers);
}

int
parse_req_type(const char** buf, request_t* request){
    const char* cur = *buf;
    const char* crlf = strstr(cur, "\r\n");
    ASSERT_RETURN2(crlf != NULL, NO_END_OF_LINE);

    const char* ws1 = strstr(cur, " ");
    ASSERT_RETURN2(ws1 != NULL && ws1 < crlf, PARSING_ERROR);
    const char* ws2 = strstr(ws1+1, " ");
    ASSERT_RETURN2(ws2 != NULL && ws2 < crlf, PARSING_ERROR);

    ASSERT((request->type = malloc(ws1 - cur + 1)) != NULL);
    memcpy(request->type, cur, ws1 - cur);
    request->type[ws1 - cur] = '\0';

    ASSERT((request->uri = malloc(ws2 - ws1)) != NULL);
    memcpy(request->uri, ws1 + 1, ws2 - ws1 - 1);
    request->uri[ws2-ws1-1] = '\0';

    ASSERT((request->version = malloc(crlf - ws2)) != NULL);
    memcpy(request->version, ws2 + 1, crlf - ws2 - 1);
    request->version[crlf-ws2-1] = '\0';

    *buf = crlf+2;
    return OK;
}

int
parse_next_header(const char **buf, header_t* header){
    const char* cur = *buf;
    const char* crlf = strstr(cur, "\r\n");
    ASSERT_RETURN2(crlf != NULL, NO_END_OF_LINE);

    if(crlf == cur){
        *buf += 2;
        return END_OF_HEADER;
    }

    const char* sep = strstr(cur, ": ");
    ASSERT_RETURN2(sep != NULL && sep < crlf, PARSING_ERROR);

    header->type = malloc(sep - cur + 1);
    memcpy(header->type, cur, sep - cur);
    header->type[sep-cur] = '\0';

    header->value = malloc(crlf - sep - 1);
    memcpy(header->value, sep+2, crlf - sep - 2);
    header->value[crlf - sep - 2] = '\0';

    *buf = crlf+2;
    return OK;
}

header_t*
find_header(vheader_t *headers, const char* type){
    for(size_t i=0; i<headers->cnt; ++i){
        header_t* hi = vheader_t_get(headers, i);
        if(strcasecmp(hi->type, type) == 0){
            return hi;
        }
    }

    return NULL;
}

void
response_init(response_t* response){
    if(response == NULL){
        return;
    }
    response->body = NULL;
    response->version = NULL;
    response->code = NULL;
    vheader_t_init(&response->headers);
}

void
response_destroy(response_t* response){
    free(response->code);
    free(response->version);
    free(response->body);
    for(size_t i=0; i<response->headers.cnt; ++i){
        header_t *header = vheader_t_get(&response->headers, i);
        free(header->type);
        free(header->value);
    }
}

int
parse_response_code(const char** buf, response_t* response){
    const char* cur = *buf;
    const char* crlf = strstr(cur, "\r\n");
    ASSERT_RETURN2(crlf != NULL, NO_END_OF_LINE);

    const char* ws = strstr(cur, " ");
    ASSERT_RETURN2(ws != NULL && ws < crlf, PARSING_ERROR);

    response->version = malloc(ws - cur + 1);
    memcpy(response->version, cur, ws - cur);
    response->version[ws-cur] = '\0';

    response->code = malloc(crlf - ws);
    memcpy(response->code, ws+1, crlf - ws - 1);
    response->code[crlf - ws - 1] = '\0';

    *buf = crlf+2;
    return OK;
}

static void
add_string2vchar(const char* str, vchar* buff){
    size_t len = strlen(str);
    vchar_alloc2(buff, len);
    memcpy(buff->ptr+buff->cnt, str, len);
    buff->cnt += len;
}

static void
add_strings2vchar(const char** strs, vchar* buff){
    for(const char** it = strs; *it != NULL; ++it){
        add_string2vchar(*it, buff);
    }
}

void
request2vchar(request_t* req, vchar* buff){
    buff->cnt = 0;
    const char* parts[] = {req->type, " ", req->uri, " ", req->version, "\r\n", NULL};
    add_strings2vchar(parts, buff);

    for(size_t i=0; i<req->headers.cnt; ++i){
        header_t* header = vheader_t_get(&req->headers, i);
        const char* hparts[] = {header->type, ": ", header->value, "\r\n", NULL};
        add_strings2vchar(hparts, buff);
    }
    add_string2vchar("\r\n", buff);
}