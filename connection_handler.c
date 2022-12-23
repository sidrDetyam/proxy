//
// Created by argem on 03.12.2022.
//

#include "connection_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include "common.h"
#include "socket_utils.h"


void init_context(handler_context_t *context, int client_fd, hash_map_t *hm) {
    request_init(&context->request);
    response_init(&context->response);

    vchar_init(&context->cbuff);
    vchar_forced_alloc(&context->cbuff);
    context->cppos = 0;
//    vchar_init(&context->sbuff);
//    vchar_forced_alloc(&context->sbuff);
    context->sppos = 0;
    context->sended = 0;

//    context->is_from_cache = 0;
    context->connection_state = NOT_CONNECTED;
    context->client_fd = client_fd;
    context->server_fd = -1;
    context->client_events = POLLIN;
    context->server_events = 0;
    context->hm = hm;
    context->handling_step = PARSING_REQ_TYPE;
}


static void
wakeup(handler_context_t* context){
    context->client_events = POLLOUT;
    if(context->is_master){
        for(size_t i=0; i<context->entry->cnt_events; ++i){
            int* waiter = context->entry->waiter_client_events[i];
            if(waiter != NULL) {
                *waiter = POLLOUT;
            }
        }
        context->entry->cnt_events = 0;
    }
}


void
destroy_context(handler_context_t *context) {
    fprintf(stderr, "%d\n", context->client_fd);
    request_destroy(&context->request);
    response_destroy(&context->response);
    vchar_free(&context->cbuff);

    close(context->client_fd);
    if (context->server_fd != -1) {
        close(context->server_fd);
    }

    if(context->entry != NULL) {
        ASSERT(pthread_mutex_lock(&context->entry->lock) == 0);
        if (context->is_master) {
            wakeup(context);
            if (context->handling_step != HANDLED) {
                context->entry->status = INVALID;
                vchar_free(&context->entry->buff);
            }
        } else {
            context->entry->waiter_client_events[context->my_waiter_id] = NULL;
        }
        ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);
    }

    if(context->handling_step != HANDLED) {
        context->handling_step = HANDLED_EXCEPTIONALLY;
    }
}


static char *
str_copy(const char *src) {
    if (src == NULL) {
        return NULL;
    }
    size_t len = strlen(src);
    char *copy = malloc(len + 1);
    ASSERT(copy != NULL);
    memcpy(copy, src, len + 1);
    return copy;
}


static void
init_cache_entry(cache_entry_t* entry){
    vchar_init(&entry->buff);
    //TODO
    entry->waiter_client_events = malloc(sizeof(int) * 1000);
    entry->cnt_events = 0;
    entry->status = DOWNLOADING;
    ASSERT(pthread_mutex_init(&entry->lock, NULL) == 0);
}


static void
add_to_cache(handler_context_t *context) {
    request_t *req_copy = malloc(sizeof(request_t));
    //vchar* resp_buff_copy = malloc(sizeof(vchar));
    ASSERT(req_copy != NULL);// && resp_buff_copy != NULL);
    request_init(req_copy);
    req_copy->type = str_copy(context->request.type);
    req_copy->uri = str_copy(context->request.uri);
    req_copy->version = str_copy(context->request.version);
    req_copy->body = str_copy(context->request.body);

    for (size_t i = 0; i < context->request.headers.cnt; ++i) {
        header_t copy;
        header_t *orig = vheader_t_get(&context->request.headers, i);
        copy.value = str_copy(orig->value);
        copy.type = str_copy(orig->type);
        vheader_t_push_back(&req_copy->headers, &copy);
    }

    context->entry = malloc(sizeof(cache_entry_t));
    ASSERT(context->entry != NULL);
    init_cache_entry(context->entry);
    context->is_master = 1;
    hash_map_put(context->hm, req_copy, &context->entry);
}


enum Config {
    MIN_READ_BUFF_SIZE = 1000000,
    DEFAULT_PORT = 80,
    //RECV_TIMEOUT_US = 250000
};

#define CACHED
//#define KEEP_ALIVE

static int
read_to_vchar(int fd, vchar *buff, size_t *read_) {
    vchar_alloc2(buff, MIN_READ_BUFF_SIZE + 1);
    //printf("read!!\n");
    ssize_t cnt = read(fd, &buff->ptr[buff->cnt], MIN_READ_BUFF_SIZE);
    if (cnt == -1 || cnt == 0) {
        return ERROR;
    }
    if (read_ != NULL) {
        *read_ = cnt;
    }
    buff->cnt += cnt;
    buff->ptr[buff->cnt] = '\0';
    return SUCCESS;
}


enum STEP_RETURN {
    CONTINUE,
    WAIT
};


static void
parsing_req_type_step(handler_context_t *context, int fd, int events, int non_splitted) {
    if (!non_splitted) {
        ASSERT(context->handling_step == PARSING_REQ_TYPE &&
               fd == context->client_fd && (events & POLLIN));
        ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                         destroy_context(context),);
    }

    const char *cppos = context->cbuff.ptr + context->cppos;
    int status = parse_req_type(&cppos, &context->request);
    context->cppos = cppos - context->cbuff.ptr;

    ASSERT_RETURN2_C(status != PARSING_ERROR,
                     destroy_context(context),);

    if (status == OK) {
        context->handling_step = PARSING_REQ_HEADERS;
    }
}


static void
connect_step(handler_context_t *context, int fd, int events);


static void
parsing_req_headers_step(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_REQ_HEADERS &&
           fd == context->client_fd && (events & POLLIN));

    if (!non_splitted) {
        ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                         destroy_context(context),);
    }

    while (1) {
        header_t header;
        const char *cppos = context->cbuff.ptr + context->cppos;
        int status = parse_next_header(&cppos, &header);
        context->cppos = cppos - context->cbuff.ptr;

        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context),);

        if (status == NO_END_OF_LINE) {
            return;
        }
        if (status == OK) {
            vheader_t_push_back(&context->request.headers, &header);
            continue;
        }
        if (status == END_OF_HEADER) {
            ASSERT_RETURN2_C(strcmp(context->request.type, "GET") == 0,
                             destroy_context(context),);

#ifndef KEEP_ALIVE
            header_t *connection = find_header(&context->request.headers, "Connection");
            if (connection != NULL) {
                free(connection->value);
                connection->value = str_copy("close");
                request2vchar(&context->request, &context->cbuff);
            }
#endif

            lock(context->hm);
            cache_entry_t** entryPtr = (cache_entry_t **) hash_map_get(context->hm, &context->request);
            cache_entry_t * entry = entryPtr==NULL? NULL : *entryPtr;
            if (entry != NULL && entry->status != INVALID) {
                context->is_master = 0;
                context->entry = entry;
                fprintf(stderr, "from cache\n");

                context->client_events = POLLOUT;
                context->server_events = 0;
                context->handling_step = SENDING_RESP;
                context->sended = 0;
                return;
            }
            context->is_master = 1;
            add_to_cache(context);
            unlock(context->hm);

            context->handling_step = CONNECT_STEP;
            connect_step(context, -1, 0);
            return;
        }
    }
}


static void
connect_step(handler_context_t *context, int fd, int events) {
    if (context->connection_state == AWAIT_CONNECTION) {
        ASSERT(context->handling_step == CONNECT_STEP &&
               fd == context->server_fd && (events & POLLOUT));
    }

    if (context->connection_state == CONNECTED || context->connection_state == AWAIT_CONNECTION) {
        context->client_events = 0;
        context->server_events = POLLOUT;
        context->sended = 0;
        context->handling_step = SENDING_REQ;

        if (context->connection_state == AWAIT_CONNECTION) {
            int connection_result;
            socklen_t _ = sizeof(connection_result);
            ASSERT_RETURN2_C(getsockopt(context->server_fd, SOL_SOCKET, SO_ERROR, &connection_result, &_) >= 0
                             && connection_result == 0,
                             destroy_context(context),);
            context->connection_state = CONNECTED;
        }
        return;
    }

    ASSERT_RETURN2_C((context->server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) >= 0,
                     destroy_context(context),);

    header_t *host_name = find_header(&context->request.headers, "Host");
    ASSERT_RETURN2_C(host_name != NULL,
                     destroy_context(context),);

    int rc = connect_to_host(context->server_fd, host_name->value, DEFAULT_PORT);
    if (rc == 0) {
        context->connection_state = CONNECTED;
    } else if (errno == EINPROGRESS) {
        context->client_events = 0;
        context->server_events = POLLOUT;
        context->connection_state = AWAIT_CONNECTION;
    } else {
        destroy_context(context);
    }
}


static void
sending_req_step(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == SENDING_REQ &&
           fd == context->server_fd && (events & POLLOUT));

    ssize_t cnt = write(context->server_fd, context->cbuff.ptr + context->sended,
                        context->cbuff.cnt - context->sended);
    ASSERT_RETURN2_C(cnt > 0, destroy_context(context),);
    context->sended += cnt;

    if (context->sended == context->cbuff.cnt) {
        context->client_events = 0;
        context->server_events = POLLIN;
        context->handling_step = PARSING_RESP_CODE;
    }
}


static void
parsing_resp_code_step(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == PARSING_RESP_CODE &&
           fd == context->server_fd && (events & POLLIN) && context->is_master);

    ASSERT(pthread_mutex_lock(&context->entry->lock) == 0);
    int rs = read_to_vchar(context->server_fd, &context->entry->buff, NULL);
    int status;
    if(rs == SUCCESS){
        const char *sppos = context->entry->buff.ptr + context->sppos;
        status = parse_response_code(&sppos, &context->response);
        context->sppos = sppos - context->entry->buff.ptr;
    }
    ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);

    if(rs==ERROR || status == PARSING_ERROR){
        destroy_context(context);
        return;
    }

    if (status == OK) {
        context->handling_step = PARSING_RESP_HEADERS;
    }
}


static int
parsing_resp_headers_step(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_RESP_HEADERS &&
           fd == context->server_fd && (events & POLLIN));

    ASSERT(pthread_mutex_lock(&context->entry->lock) == 0);
    if (!non_splitted) {
        int rs = read_to_vchar(context->server_fd, &context->entry->buff, NULL);
        if(rs == ERROR){
            ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);
            destroy_context(context);
            return WAIT;
        }
    }
    char const* bptr = context->entry->buff.ptr;
    const size_t bcnt = context->entry->buff.cnt;
    ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);

    while (1) {
        header_t header;
        const char *sppos = bptr + context->sppos;
        int status = parse_next_header(&sppos, &header);
        context->sppos = sppos - bptr;

        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context), WAIT);

        if (status == NO_END_OF_LINE) {
            return WAIT;
        }
        if (status == OK) {
            vheader_t_push_back(&context->response.headers, &header);
            continue;
        }
        if (status == END_OF_HEADER) {
            header_t *cl_header = find_header(&context->response.headers, "Content-Length");
            header_t *ch_header = find_header(&context->response.headers, "Transfer-Encoding");
            ///TODO
            if (cl_header == NULL && ch_header == NULL) {
                context->client_events = POLLOUT;
                context->server_events = 0;
                context->sended = 0;
                context->handling_step = SENDING_RESP;
                return WAIT;
            }
//            ASSERT_RETURN2_C(ch_header != NULL || cl_header != NULL,
//                             destroy_context(context),);

            context->response.content_length = cl_header != NULL ?
                                               (long) strtol(cl_header->value, NULL, 10) : -1;
            context->read_ = bcnt - context->sppos;
            context->chunk_size = -1;
            context->chunk_read = 0;
            context->client_events = POLLOUT;
            context->sended = 0;
            context->handling_step = PARSING_RESP_BODY;
            return CONTINUE;
        }
    }
}


static void
parsing_resp_body(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_RESP_BODY &&
           fd == context->server_fd && (events & POLLIN));

    char* bptr;
    size_t bcnt;
    if (!non_splitted) {
        size_t read_;
        ASSERT(pthread_mutex_lock(&context->entry->lock) == 0);
        int rs = read_to_vchar(context->server_fd, &context->entry->buff, &read_);
        if(rs == ERROR){
            ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);
            destroy_context(context);
            return;
        }
        if(read_ > 0){
            wakeup(context);
            context->read_ += read_;
        }
        ASSERT(pthread_mutex_unlock(&context->entry->lock) == 0);
    }
    CRITICAL_M(context->entry->lock,
               bptr = context->entry->buff.ptr; bcnt = context->entry->buff.cnt);

    ///TODO
    if (context->response.content_length != -1
        && context->response.content_length <= context->read_) {
        context->client_events = POLLOUT;
        context->server_events = 0;
        context->sppos = bcnt;
        context->handling_step = SENDING_RESP;

        CRITICAL_M(context->entry->lock, context->entry->status = VALID);
        return;
    }

    if (context->response.content_length == -1) {
        while (1) {
            if (context->chunk_size == -1) {
                char *sppos = bptr + context->sppos;
                char *crlf = strstr(sppos, "\r\n");
                if (crlf == NULL) {
                    return;
                }
                context->chunk_size = strtol(sppos, NULL, 16) + 2;
                context->chunk_read = 0;
                context->sppos += crlf - sppos + 2;
            }
            size_t ra = MIN(context->chunk_size - context->chunk_read,
                            bcnt - context->sppos);

            context->sppos += ra;
            context->chunk_read += ra;
            if (context->chunk_read == context->chunk_size) {
                if (context->chunk_size == 2) {
                    context->client_events = POLLOUT;
                    context->server_events = 0;
                    context->response.content_length = (long) context->read_;
                    context->handling_step = SENDING_RESP;
                    CRITICAL_M(context->entry->lock, context->entry->status = VALID);
                    return;
                }

                context->chunk_read = 0;
                context->chunk_size = -1;
                continue;
            }
            return;
        }
    }
}


static void
send_resp_in_receiving(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == PARSING_RESP_BODY || context->handling_step == SENDING_RESP &&
                                                          fd == context->client_fd && (events & POLLOUT));

    ASSERT(pthread_mutex_lock(&context->entry->lock)==0);
    if(context->entry->status == INVALID){
        ASSERT(pthread_mutex_unlock(&context->entry->lock)==0);
        destroy_context(context);
        return;
    }

    if (context->entry->buff.cnt - context->sended > 0) {
        ssize_t cnt = write(context->client_fd, context->entry->buff.ptr + context->sended,
                            context->entry->buff.cnt - context->sended);
        ASSERT(pthread_mutex_unlock(&context->entry->lock)==0);
        ASSERT_RETURN2_C(cnt > 0, destroy_context(context),);
        context->sended += cnt;
    } else {
        if(!context->is_master){
            cache_entry_t *entry = context->entry;
            entry->waiter_client_events[entry->cnt_events] = &context->client_events;
            ++entry->cnt_events;
        }
        context->client_events = 0;
        ASSERT(pthread_mutex_unlock(&context->entry->lock)==0);
    }
}


static void
send_resp_step(handler_context_t *context, int fd, int events) {
    send_resp_in_receiving(context, fd, events);
    if (context->sended == context->entry->buff.cnt && context->entry->status == VALID) {
        //destroy_context_on_cached(context);
        context->handling_step = HANDLED;
        destroy_context(context);
    }
}


void
handle(handler_context_t *context, int fd, int events) {
    int non_splitted = 0;

    if (context->handling_step == PARSING_REQ_TYPE) {
        //fprintf(stderr, "parsing req type\n");
        parsing_req_type_step(context, fd, events, non_splitted);
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_REQ_HEADERS) {
        //fprintf(stderr, "parsing req head\n");
        parsing_req_headers_step(context, fd, events, non_splitted);
        return;
    }
    if (context->handling_step == CONNECT_STEP) {
        //fprintf(stderr, "connect\n");
        connect_step(context, fd, events);
        return;
    }
    if (context->handling_step == SENDING_REQ) {
        //fprintf(stderr, "sending req\n");
        sending_req_step(context, fd, events);
        return;
    }
    if (context->handling_step == PARSING_RESP_CODE) {
        //fprintf(stderr, "parsing resp code\n");
        parsing_resp_code_step(context, fd, events);
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_RESP_HEADERS) {
        //fprintf(stderr, "parsing resp head\n");
        if (parsing_resp_headers_step(context, fd, events, non_splitted) == WAIT) {
            return;
        }
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_RESP_BODY) {
        //fprintf(stderr, "parsing resp body\n");
        if (fd == context->server_fd) {
            parsing_resp_body(context, fd, events, non_splitted);
        }
        if (fd == context->client_fd) {
            send_resp_in_receiving(context, fd, events);
        }
        return;
    }
    if (context->handling_step == SENDING_RESP) {
        //fprintf(stderr, "sending resp\n");
        send_resp_step(context, fd, events);
    }
}
