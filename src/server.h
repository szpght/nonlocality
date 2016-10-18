#pragma once
#include <pthread.h>
#include <stdbool.h>


typedef struct {
    pthread_t client_thr;
    pthread_t tunneling_thr;
    pthread_t ping_thr;
    uint16_t control_port;
    uint16_t data_port;
    uint16_t tunneled_port;
    uint32_t seq;
    int client_socket;
    pthread_mutex_t client_socket_send_mutex;
} ServerState;

extern ServerState state;

void server();
void *ping_thr_routine(void *param);
void load_config(char **argv);
void start_threads();
void *client_thr_routine(void *param);
int send_to_client(char *buffer, size_t size);
