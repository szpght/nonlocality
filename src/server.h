#pragma once
#include <pthread.h>
#include <stdbool.h>


typedef struct {
} ServerConfig;

typedef struct {
    pthread_t client_thr;
    pthread_t tunneling_thr;
    uint16_t control_port;
    uint16_t data_port;
    uint16_t tunneled_port;
    uint32_t seq;
} ServerState;

extern ServerState state;

void server();
ServerConfig load_config();
void new_client(int client_fd);
void handle_client_request(int client_fd);
void start_tunneling(int client_socket, NewClientPacket packet);
void *client_thr_routine(void *param);
