#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <signal.h>
#include "nonlocality.h"
#include "server.h"


ServerState state = { .seq = 1 };
ConnectionVector connections;


int main(int argc, char **argv) {
    if (argc < 4) {
        die("Usage: <tunneled port> <control port> <data port>");
    }
    puts("hello, wonderful world");
    load_config(argv);
    signal(SIGUSR1, signal_handler);
    server();
    return 0;
}


void signal_handler(int signal) {
    if (signal == SIGUSR1)
        print_connections(&connections);
}


void load_config(char **argv) {
    state.control_port = port_from_string(argv[2]);
    state.tunneled_port = port_from_string(argv[1]);
    state.data_port = port_from_string(argv[3]);
}


void server() {
    vector_init(&connections);

    // TODO errors unprobable but it would be a good idea to check for them
    int control_socket = get_tcp_socket();
    listen_on_port(control_socket, state.control_port, 5);

    struct sockaddr_in client_sockaddr;
    socklen_t client_sockaddr_length = sizeof(client_sockaddr);
    printf("Waiting for client connection.... ");
    state.client_socket = accept(control_socket, (struct sockaddr *) &client_sockaddr, &client_sockaddr_length);
    if (state.client_socket < 0)
        die("client connection accept error");
    puts("connected");
    start_tunneling();

    // TODO don't block when client connects
    pthread_join(state.client_thr, &(void*){0});
}


void start_tunneling() {
    printf("Tunneling port %d\n", state.tunneled_port);
    if (pthread_create(&state.client_thr, NULL, client_thr_routine, NULL))
        die("cannot create client thread");
    if (pthread_create(&state.tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");
}


void *client_thr_routine(void *param) {
    puts("created client thread, hurray.");

    int tunneled_socket = get_tcp_socket();
    listen_on_port(tunneled_socket, state.tunneled_port, 5);
    int data_socket = get_tcp_socket();
    listen_on_port(data_socket, state.data_port, 5);
    for (;;) {
        ConnectionPair conn;
        conn.seq = state.seq++;

        // accept connection to be tunneled
        conn.server = accept_jauntily(tunneled_socket);

        // tell client to make new connection
        NewConnectionData packet = { .seq = conn.seq };
        ssize_t sent = send_amount(state.client_socket, (char*) &packet, sizeof(packet));
        if (sent < sizeof(packet))
            die("error when sending NewConnectionData");

        // wait for client making connection
        conn.client = accept_jauntily(data_socket);

        // receive sequence number
        receive_amount(conn.client, &(NewConnectionData){}, sizeof(NewConnectionData));

        pthread_mutex_lock(&connections.mutex);
        vector_add(&connections, conn);
        pthread_mutex_unlock(&connections.mutex);
    }
}
