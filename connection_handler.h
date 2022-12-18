//
// Created by argem on 03.12.2022.
//

#ifndef PTHREAD___CONNECTION_HANDLER_H
#define PTHREAD___CONNECTION_HANDLER_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http_header_parser.h"
#include "hash_map.h"


enum HandlingStep{
    PARSING_REQ_TYPE = 1337,
    PARSING_REQ_HEADERS,

    CONNECT_STEP,
    SENDING_REQ,

    PARSING_RESP_CODE,
    PARSING_RESP_HEADERS,
    PARSING_RESP_BODY,

    SENDING_RESP,

    HANDLED,
    HANDLED_EXCEPTIONALLY
};

enum ConnectionState{
    NOT_CONNECTED = 4242,
    AWAIT_CONNECTION,
    CONNECTED
};


struct HttpConnectionHandlerContext{
    int client_fd;
    int server_fd;
    int connection_state;
    int client_events;
    int server_events;

    request_t request;
    response_t response;

    vchar cbuff;
    size_t cppos; //client processing position

    size_t sended;
    size_t read_;

    vchar sbuff;
    int is_from_cache;
    size_t sppos; //server processing position
    long chunk_size;
    size_t chunk_read;

    hash_map_t *hm;

    int handling_step;
};
typedef struct HttpConnectionHandlerContext handler_context_t;


//#define ELEMENT_TYPE handler_context_t;
//#include "cvector_def.h"


void
init_context(handler_context_t* context, int client_fd, hash_map_t* hm);

void
destroy_context(handler_context_t* context);

void
handle(handler_context_t* context, int fd, int events);


#endif //PTHREAD___CONNECTION_HANDLER_H
