//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_HTTP_HEADER_PARSER_H
#define PTHREAD_HTTP_HEADER_PARSER_H

#include<stdlib.h>

struct HEADER{
    char* type;
    char* value;
};
typedef struct HEADER header_t;

#define ELEMENT_TYPE header_t
#include "cvector_def.h"

#define ELEMENT_TYPE char
#include "cvector_def.h"

struct REQUEST{
    char* type;
    char* uri;
    char* version;
    vheader_t headers;
    char* body;
};
typedef struct REQUEST request_t;


struct RESPONSE{
    char* version;
    char* code;
    vheader_t headers;
    char* body;
    long content_length;
};
typedef struct RESPONSE response_t;

void
request_init(request_t* request);

__attribute__((unused)) void
request_destroy(request_t* request);


header_t*
find_header(vheader_t *headers, const char* type);


enum PARSE_STATUS{
    NO_END_OF_LINE,
    PARSING_ERROR,
    END_OF_HEADER,
    OK
};


int
parse_next_header(const char **buf, header_t* header);

int
parse_req_type(const char** buf, request_t* request);

void
response_init(response_t* response);

void
response_destroy(response_t* response);

int
parse_response_code(const char** buf, response_t* response);

void
request2vchar(request_t* req, vchar* buff);

#endif //PTHREAD_HTTP_HEADER_PARSER_H
