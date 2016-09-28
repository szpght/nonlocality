#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/select.h>
#include "nonlocality.h"
#include "server.h"


ServerState state = { .seq = 666 };
ServerConfig config;
ConnectionVector connections;


int main(int argc, char **argv) {
    puts("hello, wonderful world");
    ServerConfig config = load_config();
    server(config);
    return 0;
}


ServerConfig load_config() {
    // TODO error checking
    config.control_port = atoi(getenv("CONTROL_PORT"));
    config.tunneled_port = atoi(getenv("TUNNELED_PORT"));
    config.data_port = atoi(getenv("DATA_PORT"));
    return config;
}


void server(ServerConfig config) {
    // TODO errors unprobable but it would be a good idea to check for them
    int control_socket = get_tcp_socket();
    listen_on_port(control_socket, config.control_port, 5);

    struct sockaddr_in client_sockaddr;
    int client_sockaddr_length = sizeof(client_sockaddr);
    printf("Waiting for connection.... ");
    int client_socket = accept(control_socket, (struct sockaddr*)&client_sockaddr, (socklen_t*)&client_sockaddr_length);
    if (client_socket < 0)
        die("client connection accept error");
    puts("connected");
    for (;;) {
        handle_client_request(client_socket);
    }
}


void handle_client_request(int client_fd) {
    int packet_type;
    ssize_t received = receive_amount(client_fd, &packet_type, 4);
    if (received < 4)
        die("error when receiving packet length");
    switch (packet_type) {
        case NEW_CLIENT:
            new_client(client_fd);
            break;
        default:
            printf("Unknown packet type received: %d\n", packet_type);
            abort();
    }
}


void new_client(int client_fd) {
    NewClientPacket packet;
    ssize_t received = receive_amount(client_fd, (char*) &packet, sizeof(packet));
    if (received < sizeof(packet))
        die("error!!!1!11");
    start_tunneling(client_fd, packet);

}


void start_tunneling(int client_socket, NewClientPacket packet) {
    state.tunneled_port = packet.port;
    printf("Tunneling port %d\n", packet.port);
    if (pthread_create(&state.client_thr, NULL, client_thr_routine, client_socket))
        die("you have no resources to create thread, poor (wo)man");
    // TODO BLOCKING WHEN CLIENT CONNECTS
    void *junk;
    pthread_join(state.client_thr, &junk);
}


void *client_thr_routine(void *param) {
    int client_socket = (int) param;
    puts("created client thread, hurray.");

    // initialize connections array
    vector_init(&connections);

    int tunneled_socket = get_tcp_socket();
    listen_on_port(tunneled_socket, state.tunneled_port, 5);
    int data_socket = get_tcp_socket();
    listen_on_port(data_socket, config.data_port, 5);
    for (;;) {
        ConnectionPair conn;
        // accept connection to be tunneled
        conn.server = accept_jauntily(tunneled_socket);

        // tell client to make new connection
        NewConnectionData packet = { .seq = state.seq++ };
        ssize_t sent = send_amount(client_socket, (char*) &packet, sizeof(packet));
        if (sent < sizeof(packet))
            die("error when sending NewConnectionData");

        // wait for client making connection
        conn.client = accept_jauntily(data_socket);

        // receive sequence number
        receive_amount(data_socket, &(NewConnectionData){}, sizeof(NewConnectionData));

        vector_add(&connections, conn);
    }
}


void *tunneling_thr_routine(void *param) {
    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = 0;

        for (int i = 0; i < connections.size; ++i) {
            ConnectionPair pair = connections.conns[i];
            FD_SET(pair.client, &readfds);
            if (pair.client > max_fd)
                max_fd = pair.client;
            FD_SET(pair.server, &readfds);
            if (pair.server > max_fd)
                max_fd = pair.server;
        }
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100 * 1000;
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1)
            die("select returned -1");
        if (ret == 0)
            continue;

        for (int i = 0; i < connections.size; ++i) {
            ConnectionPair pair = connections.conns[i];
            if (FD_ISSET(pair.client, &readfds))
                move_data(pair.client, pair.server);
            if (FD_ISSET(pair.server, &readfds))
                move_data(pair.server, pair.client);
        }
    }
}


void move_data(int src, int dest) {
    char buffer[RECV_BUFFER_SIZE];
    ssize_t received = recv(src, buffer, RECV_BUFFER_SIZE, 0);
    send_amount(dest, buffer, received);
}


void announce_new_connection(ConnectionPair pair) {

}
