//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_SERVER_SOCKET_H
#define PTHREAD_SERVER_SOCKET_H

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

struct SERVER_SOCKET{
    int fd;
    struct sockaddr_in addr;
    socklen_t socklen;
};
typedef struct SERVER_SOCKET servsock_t;

int
create_servsock(uint16_t port, int limit, servsock_t* sock);

__attribute__((unused)) int
close_servsock(servsock_t* sock);

int
accept_servsock(servsock_t* servsock, int is_non_blocking);

int
name2addr(const char *host_name, uint16_t host_port, struct sockaddr_in *sock_addr);

int
connect_to_host(int fd, const char *host_name, uint16_t host_port);

#endif //PTHREAD_SERVER_SOCKET_H
