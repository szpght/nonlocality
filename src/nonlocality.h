#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>

#define RECV_BUFFER_SIZE 1500
#define CONNECT_TIMEOUT_SEC 5
#define PING_SEND_INTERVAL_SEC 10
#define PING_SEND_TIMEOUT_SEC 5
#define CLIENT_SEND_TIMEOUT_SEC 30
#define CLIENT_CONNECTION_RESET_TIME 25
#define CLIENT_CONNECT_RETRY_TIME_SEC 1
#define SEND_TO_CLIENT_RETRY_INTERVAL_SEC 1


enum PacketType {
    KEEP_ALIVE = 0,
    NEW_CLIENT = 1,
    NEW_CONNECTION = 2,
};

typedef struct {
    uint16_t port; // port on which machine behind firewall listens
} NewClientPacket;

typedef struct {
    uint32_t seq;
} NewConnectionData;

typedef struct {
    int seq;
    int server;
    int client;
    uint64_t from_server;
    uint64_t from_client;
    time_t last_activity;
} ConnectionPair;

typedef struct {
    int size;
    int capacity;
    ConnectionPair *conns;
    pthread_mutex_t mutex;
} ConnectionVector;


// nonlocality.c
int get_tcp_socket();
struct sockaddr_in listen_on_port(int socket_fd, uint16_t port, int queue_length);
void die(char *msg);
ssize_t receive_amount(int fd, char *buffer, size_t len);
ssize_t send_amount(int fd, char *buffer, size_t len);
ssize_t send_amount_timeout(int fd, char *buffer, size_t len, int timeout_sec);
ssize_t receive_amount_timeout(int fd, char *buffer, size_t len, int timeout_sec);
int accept_jauntily(int fd);
int connect_from_str(char *ip, uint16_t port);
void *tunneling_thr_routine(void *param);
void print_pair_statistics(ConnectionPair *pair);
int create_readfds(fd_set *readfds, ConnectionVector *connections);
bool serve_pair(fd_set *readfds, ConnectionPair *pair);
ssize_t move_data(int src_fd, int dest_fd);
uint16_t port_from_string(char *port);
void sequence_message(int seq, char *msg);
int accept_timeout(int fd);
ssize_t recv_timeout(int fd, void *buffer, size_t len, int timeout_sec);
ssize_t send_timeout(int fd, void *buffer, size_t len, int timeout_sec);


// conn_vector.c
void vector_init(ConnectionVector *vector);
void vector_add(ConnectionVector *vector, ConnectionPair pair);
void vector_delete(ConnectionVector *vector, int position);
