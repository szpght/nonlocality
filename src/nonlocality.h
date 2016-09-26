#pragma once
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>


enum PacketType {
    KEEP_ALIVE = 0,
    NEW_CONNECTION = 1,
    TAKE_CONNECTION = 2,
};


struct Packet {
    enum PacketType type;
    void *data;
};


struct NewConnectionData {
    uint32_t seq;
    uint32_t ip;
    uint32_t port;
};


struct TakeConnectionData {
    uint32_t seq;
};


int get_tcp_socket();
struct sockaddr_in listen_on_port(int socket_fd, uint16_t port, int queue_length);