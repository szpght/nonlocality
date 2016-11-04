#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <execinfo.h>
#include "nonlocality.h"


int get_tcp_socket() {
    errno = 0;
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int socket_errno = errno;
    int err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (s != -1 && !err)
        return s;
    puts("socket() or setsockopt() failed");
    printf("socket() errno: %d\nsetsockopt() errno: %d", socket_errno, errno);
    exit(1);
}


struct sockaddr_in listen_on_port(int socket_fd, uint16_t port, int queue_length) {
    struct sockaddr_in server_sockaddr = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(port)
    };
    bind(socket_fd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr));
    if (listen(socket_fd, queue_length)) {
        printf("Cannot listen on port %d, errno %d\n", port, errno);
        exit(1);
    }
}


void die(char *msg) {
    puts(msg);
    puts("stacktrace:");
    void *addresses[1024];
    int n = backtrace(addresses, 1024);
    char **symbols = backtrace_symbols(addresses, n);
    for (int i = 0; i < n; ++i)
        puts(symbols[i]);
    exit(1);
}


ssize_t receive_amount(int fd, void *buffer, size_t len) {
    ssize_t received = 0;
    while (received < len) {
        ssize_t last_received = recv(fd, buffer + received, len - received, 0);
        if (last_received == 0 || last_received == -1)
            break;
        received += last_received;
    }
    return received;
}


ssize_t send_amount(int fd, void *buffer, size_t len) {
    ssize_t sent = 0;
    while (sent < len) {
        ssize_t last_sent = send(fd, buffer + sent, len - sent, 0);
        if (last_sent == -1) {
            printf("send returned -1, errno: %d\n", errno);
            return sent;
        }
        sent += last_sent;
    }
    return sent;
}


ssize_t send_amount_timeout(int fd, void *buffer, size_t len, int timeout_sec) {
    // TODO think of something more abstract
    ssize_t sent = 0;
    while (sent < len) {
        ssize_t last_sent = send_timeout(fd, buffer + sent, len - sent, timeout_sec);
        if (last_sent == -1) {
            printf("send_timeout returned -1, errno: %d\n", errno);
            return sent;
        }
        sent += last_sent;
    }
    return sent;
}


ssize_t receive_amount_timeout(int fd, void *buffer, size_t len, int timeout_sec) {
    ssize_t received = 0;
    while (received < len) {
        ssize_t last_received = recv_timeout(fd, buffer + received, len - received, timeout_sec);
        if (last_received <= 0)
            break;
        received += last_received;
    }
    return received;
}


int accept_jauntily(int fd) {
    int retval = accept(fd, &(struct sockaddr){}, &(socklen_t){ sizeof(struct sockaddr_in)});
    if (retval == -1) {
        printf("error when accepting connection, errno: %d\n", errno);
        return -1;
    }
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

    int so_error = 0;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    connect(fd, (struct sockaddr*) &serv_ip, sizeof(serv_ip));
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    struct timeval tv = { .tv_sec = CONNECT_TIMEOUT_SEC, .tv_usec = 0 };

    if (select(fd + 1, NULL, &fdset, NULL, &tv) == 1) {
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &(socklen_t){sizeof so_error});
        if (!so_error) {
            int opts = fcntl(fd, F_GETFL, 0);
            opts &= (~O_NONBLOCK);
            fcntl(fd, F_SETFL, opts);
            return fd;
        }
    }
    close(fd);
    return -1;
}


void *tunneling_thr_routine(void *param) {
    ConnectionVector *connections = param;
    puts("starting tunneling loop");
    for (;;) {
        fd_set readfds;
        pthread_mutex_lock(&connections->mutex);
        int max_fd = create_readfds(&readfds, connections);
        pthread_mutex_unlock(&connections->mutex);

        struct timeval timeout = { .tv_sec = 0, .tv_usec = 50 * 1000};
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1)
            die("select returned -1");
        if (ret == 0)
            continue;

        pthread_mutex_lock(&connections->mutex);
        for (int i = 0; i < connections->size; ++i) {
            ConnectionPair *pair = &connections->conns[i];
            bool success = serve_pair(&readfds, pair);
            if (!success) {
                close(pair->client);
                close(pair->server);
                sequence_message(pair->seq, "connection closed with stats:");
                print_pair_statistics(pair);
                vector_delete(connections, i);
                --i;
            }
        }
        pthread_mutex_unlock(&connections->mutex);
    }
}


void print_pair_statistics(ConnectionPair *pair) {
    char time_human[100];
    struct tm *ts;
    ts = localtime(&pair->last_activity);
    strftime(time_human, sizeof time_human, "%a %Y-%m-%d %H:%M:%S", ts);
    printf("client -> server: %llub\nserver -> client: %llub\nlast activity: %s\n",
           (long long unsigned) pair->from_client, (long long unsigned) pair->from_server, time_human);
}


int create_readfds(fd_set *readfds, ConnectionVector *connections) {
    int max_fd = 0;
    FD_ZERO(readfds);

    for (int i = 0; i < connections->size; ++i) {
        ConnectionPair pair = connections->conns[i];
        FD_SET(pair.client, readfds);
        if (pair.client > max_fd)
            max_fd = pair.client;
        FD_SET(pair.server, readfds);
        if (pair.server > max_fd)
            max_fd = pair.server;
    }
    return max_fd;
}


bool serve_pair(fd_set *readfds, ConnectionPair *pair) {
    int count = 2;
    if (FD_ISSET(pair->client, readfds)) {
        ssize_t moved = move_data(pair->client, pair->server);
        count -= !moved;
        pair->from_client += moved;
    }
    if (FD_ISSET(pair->server, readfds)) {
        ssize_t moved = move_data(pair->server, pair->client);
        count -= !moved;
        pair->from_server += moved;
    }

    // if some move_data returned false
    if (count < 2)
        return false;
    time(&pair->last_activity);
    return true;
}


ssize_t move_data(int src_fd, int dest_fd) {
    // TODO think about nonblocking operation
    char buffer[RECV_BUFFER_SIZE];
    ssize_t received = recv(src_fd, buffer, RECV_BUFFER_SIZE, 0);
    if (!received) {
        return 0;
    }
    ssize_t sent = send_amount(dest_fd, buffer, received);
    if (sent < received) {
        return 0;
    }
    return sent;
}


uint16_t port_from_string(char *port_argument) {
    long port = strtol(port_argument, NULL, 10);
    if (port > 0 && port < 65536)
        return (uint16_t ) port;
    printf("Bad port value: %s\n", port_argument);
    exit(1);
}


void sequence_message(int seq, char *msg) {
    printf("seq=%d: %s\n", seq, msg);
}


int accept_timeout(int fd) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval timeout = { .tv_sec = CONNECT_TIMEOUT_SEC, .tv_usec = 0 };

    int retval = select(fd + 1, &readfds, NULL, NULL, &timeout);

    // timeout
    if (retval == 0) {
        puts("accept timeout - select() returned 0");
        return -1;
    }
    if (retval == -1) {
        printf("error on select when accepting connection, errno %d\n", errno);
        return -1;
    }
    retval = accept(fd, &(struct sockaddr){}, &(socklen_t){ sizeof(struct sockaddr_in) });
    if (retval == -1) {
        printf("error on accept when accepting connection, errno: %d\n", errno);
        return -1;
    }
    return retval;
}


ssize_t recv_timeout(int fd, void *buffer, size_t len, int timeout_sec) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    struct timeval timeout = { timeout_sec, 0 };
    int retval = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (retval != 1)
        return -1;
    return recv(fd, buffer, len, 0);
}


ssize_t send_timeout(int fd, void *buffer, size_t len, int timeout_sec) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    struct timeval timeout = { timeout_sec, 0 };
    int retval = select(fd + 1, NULL, &writefds, NULL, &timeout);
    if (retval != 1)
        return -1;
    return send(fd, buffer, len, 0);
}
