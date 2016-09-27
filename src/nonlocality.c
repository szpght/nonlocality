#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "nonlocality.h"


int get_tcp_socket() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (s != -1 && !err)
        return s;
    close(s);
    return -1;
}


struct sockaddr_in listen_on_port(int socket_fd, uint16_t port, int queue_length) {
    struct sockaddr_in server_sockaddr = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port)
    };
    bind(socket_fd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr));
    listen(socket_fd, queue_length);
}


void die(char *msg) {
    puts(msg);
    abort();
}


ssize_t receive_amount(int fd, char *buffer, size_t len) {
    ssize_t received = 0;
    while (received < len) {
        ssize_t last_received = recv(fd, buffer + received, len - received, NULL);
        received += last_received;
        if (!last_received)
            break;
    }
    return received;
}


ssize_t send_amount(int fd, char *buffer, size_t len) {
    ssize_t sent = 0;
    while (sent < len) {
        ssize_t last_sent = send(fd, buffer + sent, len - sent, NULL);
        if (last_sent == -1)
            printf("sent returned -1, errno: %d\n", errno);
        sent += last_sent;
        if (!last_sent)
            break;
    }
    return sent;
}


int accept_jauntily(int fd) {
    struct sockaddr_in data_sockaddr;
    socklen_t data_sockaddr_length = sizeof(data_sockaddr);
    return accept(fd, &data_sockaddr, &data_sockaddr_length);
}