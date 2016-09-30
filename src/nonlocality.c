#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <sys/select.h>
#include <pthread.h>
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
    printf("received %d bytes in thread %d from socket %d: ", len, pthread_self(), fd);
    for (int i = 0; i < len; ++i)
        printf("%02x", buffer[i]);
    puts("");
    return received;
}


ssize_t send_amount(int fd, char *buffer, size_t len) {
    printf("sending %d bytes in thread %d to socket %d: ",len, pthread_self(), fd);
    for (int i = 0; i < len; ++i)
        printf("%02x", buffer[i]);
    puts("");

    ssize_t sent = 0;
    while (sent < len) {
        ssize_t last_sent = send(fd, buffer + sent, len - sent, NULL);
        if (last_sent == -1)
            printf("send returned -1, errno: %d\n", errno);
        sent += last_sent;
        if (!last_sent)
            break;
    }
    return sent;
}


int accept_jauntily(int fd) {
    struct sockaddr_in data_sockaddr;
    socklen_t data_sockaddr_length = sizeof(data_sockaddr);
    int retval = accept(fd, &data_sockaddr, &data_sockaddr_length);;
    printf("accepted connection on socket %d in thread %d\n", retval, pthread_self());
    return retval;
}


int connect_from_str(char *ip, uint16_t port) {
    int fd = get_tcp_socket();
    struct sockaddr_in serv_ip;
    memset(&serv_ip, 0, sizeof(struct sockaddr_in));
    serv_ip.sin_family = AF_INET;
    serv_ip.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_ip.sin_addr) != 1)
        die("bad ip address");
    int retval = connect(fd, &serv_ip, sizeof(serv_ip));
    if (retval) {
        printf("Connect error: %d\n", errno);
        die("");
    }
    return fd;
}


void *tunneling_thr_routine(void *param) {
    ConnectionVector *connections = param;
    puts("starting tunneling loop");
    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = 0;

        for (int i = 0; i < connections->size; ++i) {
            ConnectionPair pair = connections->conns[i];
            FD_SET(pair.client, &readfds);
            if (pair.client > max_fd)
                max_fd = pair.client;
            FD_SET(pair.server, &readfds);
            if (pair.server > max_fd)
                max_fd = pair.server;
        }
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1)
            die("select returned -1");
        if (ret == 0)
            continue;

        for (int i = 0; i < connections->size; ++i) {
            ConnectionPair pair = connections->conns[i];
            if (FD_ISSET(pair.client, &readfds))
                move_data(pair.client, pair.server);
            if (FD_ISSET(pair.server, &readfds))
                move_data(pair.server, pair.client);
        }
    }
}


void move_data(int src_fd, int dest_fd) {
    char buffer[RECV_BUFFER_SIZE];
    ssize_t received = recv(src_fd, buffer, RECV_BUFFER_SIZE, 0);
    printf("moving data in thread %d, sockets %d -> %d, size: %d\n", pthread_self(), src_fd, dest_fd, received);
    send_amount(dest_fd, buffer, received);
}
