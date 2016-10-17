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
    int client_socket;
} ServerState;

extern ServerState state;

void server();
void signal_handler(int signal);
void load_config(char **argv);
void start_tunneling();
void *client_thr_routine(void *param);
int sendToClient(char *buffer, size_t size);
