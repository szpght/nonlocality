#pragma once
#include <pthread.h>
#include <stdbool.h>


typedef struct {
    uint16_t control_port;
    uint16_t tunneled_port;
    uint16_t data_port;
} ServerConfig;

typedef struct {
    pthread_t client_thr;
    uint16_t tunneled_port;
    int connections_max;
    int *connections;
    int connections_num;
} ServerState;

typedef struct {
    int socket;
    bool available;
} Connection;

extern ServerState state;

void server(ServerConfig config);
ServerConfig load_config();
void new_client(int client_fd);
void handle_client_request(int client_fd);
void start_tunneling(int client_socket, NewClientPacket packet);
void *client_thr_routine(void *param);
void announce_new_connection();
