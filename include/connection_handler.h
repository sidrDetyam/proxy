//
// Created by argem on 03.12.2022.
//

#ifndef PTHREAD___CONNECTION_HANDLER_H
#define PTHREAD___CONNECTION_HANDLER_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "http_utils.h"
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


enum CacheEntryStatus{
    DOWNLOADING = 3333,
    VALID,
    NEED_NEW_MASTER,
    INVALID
};

struct HttpConnectionHandlerContext;
typedef struct HttpConnectionHandlerContext handler_context_t;



struct CacheEntry{
    pthread_mutex_t lock;
    int cnt_of_clients;
    vchar buff;
    int status;
    handler_context_t ** waiter_client_events;
    size_t cnt_waiters;
    handler_context_t *state;
};
typedef struct CacheEntry cache_entry_t;


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
    cache_entry_t *entry;
    int is_master;
    ssize_t my_waiter_id;
    size_t sppos; //server processing position
    long chunk_size;
    size_t chunk_read;
    hash_map_t *hm;
    int handling_step;
};


void
store_master_state_on_client_error(handler_context_t* masters_context);

void
load_master_state(handler_context_t* context);

void
init_context(handler_context_t* context, int client_fd, hash_map_t* hm);

void
destroy_context(handler_context_t* context);

void
handle(handler_context_t* context, int fd, int events);


#endif //PTHREAD___CONNECTION_HANDLER_H
