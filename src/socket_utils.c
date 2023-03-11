//
// Created by argem on 30.11.2022.
//
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>

#include "../include/socket_utils.h"
#include "../utils/common.h"


int
create_servsock(uint16_t port, int limit, servsock_t* sock){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_RETURN(server_fd >= 0);

    int opt = 1;
    ASSERT_RETURN_C(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == 0,
                    close(server_fd));

    sock->addr.sin_family = AF_INET;
    sock->addr.sin_addr.s_addr = INADDR_ANY;
    sock->addr.sin_port = htons(port);
    sock->fd = server_fd;
    sock->socklen = (socklen_t) sizeof(sock->addr);

    ASSERT_RETURN_C(bind(server_fd, (struct sockaddr*) &sock->addr, sock->socklen) == 0,
                    close(server_fd));
    ASSERT_RETURN_C(listen(server_fd, limit) == 0,
                    close(server_fd));

    return SUCCESS;
}

__attribute__((unused)) int
close_servsock(servsock_t* sock){
    ASSERT_RETURN(shutdown(sock->fd, SHUT_RDWR) == 0);
    return SUCCESS;
}

int
accept_servsock(servsock_t* servsock, int is_non_blocking){
    int fd = accept(servsock->fd, (struct sockaddr*)& servsock->addr, &servsock->socklen);
    int rc = is_non_blocking? fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) : 0;
    return (fd >= 0 && rc == 0? fd : ERROR);
}


int
name2addr(const char *host_name, uint16_t host_port, struct sockaddr_in *sock_addr) {
    struct hostent *server = gethostbyname(host_name);
    ASSERT_RETURN(server != NULL);

    memset(&sock_addr->sin_zero, 0, sizeof(sock_addr->sin_zero));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(host_port);
    memcpy(&sock_addr->sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    return SUCCESS;
}


int
connect_to_host(int fd, const char *host_name, uint16_t host_port){
    struct sockaddr_in host_addr;
    ASSERT_RETURN2(name2addr(host_name, host_port, &host_addr) == SUCCESS, -1);
    return connect(fd, (struct sockaddr *) &host_addr, sizeof(host_addr));
}