#pragma once
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define RECV_BUFFER_SIZE 1000


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


int get_tcp_socket();
struct sockaddr_in listen_on_port(int socket_fd, uint16_t port, int queue_length);
void die(char *msg);
ssize_t receive_amount(int fd, char *buffer, size_t len);
ssize_t send_amount(int fd, char *buffer, size_t len);