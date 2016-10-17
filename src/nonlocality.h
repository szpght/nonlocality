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
int accept_jauntily(int fd);
int connect_from_str(char *ip, uint16_t port);
void *tunneling_thr_routine(void *param);
int create_readfds(fd_set *readfds, ConnectionVector *connections);
bool serve_pair(fd_set *readfds, ConnectionPair pair);
bool move_data(int src_fd, int dest_fd);
void print_connections(ConnectionVector *vector);
uint16_t port_from_string(char *port);
void sequence_message(int seq, char *msg);
int accept_timeout(int fd);


// conn_vector.c
void vector_init(ConnectionVector *vector);
void vector_add(ConnectionVector *vector, ConnectionPair pair);
void vector_delete(ConnectionVector *vector, int position);
