#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include <pthread.h>

#include "../include/socket_utils.h"
#include "../utils/common.h"
#include "../include/connection_handler.h"
#include "proxy_config.h"


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
#ifdef DEBUG
                    fprintf(stderr, "handled\n");
#endif
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
#ifdef DEBUG
        fprintf(stderr, "connect\n");
#endif
    }
}
