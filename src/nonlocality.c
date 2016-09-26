#include <stdio.h>
#include <stdlib.h>
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
        ssize_t last_received = recv(fd, buffer + received, len - received, 0);
        received += last_received;
        if (!last_received)
            return received;
    }
    return received;
}
