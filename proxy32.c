#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>

#include "socket_utils.h"
#include "common.h"
#include "connection_handler.h"


static int
str_pol_hash(const char *str) {
    const int p = 31;
    const int m = 100043;
    int hash = 0;
    long p_pow = 1;
    for (const char *it = str; *it; ++it) {
        hash = (int) ((hash + (*it - 'a' + 1) * p_pow) % m);
        p_pow = (p_pow * p) % m;
    }
    return hash;
}


static int
request_hash(void *req_) {
    request_t *req = (request_t *) req_;
    return str_pol_hash(req->type) + str_pol_hash(req->version) + str_pol_hash(req->uri);
}


static int
request_equals(void *req1_, void *req2_) {
    request_t *req1 = req1_;
    request_t *req2 = req2_;
    static const char *headers[] = {"Host", "Range", NULL};

    int eq = strcmp(req1->type, req2->type) == 0
             && strcmp(req1->version, req2->version) == 0
             && strcmp(req1->uri, req2->uri) == 0;

    for (const char **it = headers; *it != NULL && eq; ++it) {
        header_t *host1 = find_header(&req1->headers, *it);
        header_t *host2 = find_header(&req2->headers, *it);
        if (host1 != NULL && host2 != NULL) {
            eq = strcmp(host1->value, host2->value) == 0;
        }
    }

    return eq;
}


enum Config {
    MAX_CONNECTIONS = 100,
    PORT = 4242,
    POLL_TIMEOUT = 1000
};


struct HandlerThreadContext {
    pthread_spinlock_t *spinlock;
    int *cnt_of_threads;
    handler_context_t *context;
};
typedef struct HandlerThreadContext thread_context_t;


static void *
handler_subroutine(void *arg_) {
    thread_context_t *arg = (thread_context_t *) arg_;
    handler_context_t *context = arg->context;
    struct pollfd fds[2];
    fds[0].fd = context->client_fd;
    fds[0].events = (short) context->client_events;
    size_t fds_count = 1;

    while (1) {
        int cnt_fds = poll(fds, fds_count, POLL_TIMEOUT);
        ASSERT(cnt_fds != -1);

        for (size_t i = 0; i < fds_count; ++i) {
            if (context->client_fd == fds[i].fd && (context->client_events & fds[i].revents)
                || context->server_fd == fds[i].fd && (context->server_events & fds[i].revents)) {

                handle(context, fds[i].fd, fds[i].revents);
                if (context->handling_step == HANDLED || context->handling_step == HANDLED_EXCEPTIONALLY) {
                    fprintf(stderr, "handled\n");
                    ASSERT(pthread_spin_lock(arg->spinlock) == 0);
                    --arg->cnt_of_threads;
                    ASSERT(pthread_spin_unlock(arg->spinlock) == 0);
                    free(context);
                    free(arg);
                    return NULL;
                }
            }
        }
        fds_count = 0;
        if (context->client_events != 0) {
            fds[fds_count].fd = context->client_fd;
            fds[fds_count].events = (short) context->client_events;
            ++fds_count;
        }
        if (context->server_events != 0 && context->server_fd != -1) {
            fds[fds_count].fd = context->server_fd;
            fds[fds_count].events = (short) context->server_events;
            ++fds_count;
        }
    }
}


int
main() {
    ASSERT(sigaction(SIGPIPE, &(struct sigaction) {SIG_IGN}, NULL) == 0);

    servsock_t servsock;
    ASSERT(create_servsock(PORT, MAX_CONNECTIONS, &servsock) == SUCCESS);

    int count_of_threads = 0;
    pthread_spinlock_t spinlock;
    ASSERT(pthread_spin_init(&spinlock, 0) == 0);

    hash_map_t hm;
    hash_map_init(&hm, sizeof(request_t), sizeof(vchar), request_hash, request_equals);

    while (1) {
        int new_fd;
        ASSERT((new_fd = accept_servsock(&servsock, 1)) != ERROR);
        ASSERT(pthread_spin_lock(&spinlock) == 0);
        int cnt = count_of_threads;
        ASSERT(pthread_spin_unlock(&spinlock) == 0);
        if(cnt >= MAX_CONNECTIONS){
            close(new_fd);
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;
        ASSERT(setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);

        handler_context_t *context = malloc(sizeof(handler_context_t));
        thread_context_t *arg = malloc(sizeof(thread_context_t));
        pthread_t *tid = malloc(sizeof(pthread_t));
        ASSERT(arg != NULL && context != NULL);
        arg->spinlock = &spinlock;
        arg->context = context;
        arg->cnt_of_threads = &count_of_threads;
        init_context(context, new_fd, &hm);

        pthread_attr_t tattr;
        ASSERT(pthread_attr_init(&tattr)==0);
        ASSERT(pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED)==0);
        ASSERT(pthread_create(tid, &tattr, handler_subroutine, arg) == 0);
        fprintf(stderr, "connect\n");
    }
}
