#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include "nonlocality.h"
#include "server.h"


ServerState state = { .seq = 109 };
ConnectionVector connections;


int main(int argc, char **argv) {
    puts("hello, wonderful world");
    load_config();
    server();
    return 0;
}


ServerConfig load_config() {
    // TODO load from command line
    state.control_port = atoi(getenv("CONTROL_PORT"));
    state.tunneled_port = atoi(getenv("TUNNELED_PORT"));
    state.data_port = atoi(getenv("DATA_PORT"));
}


void server() {
    // TODO errors unprobable but it would be a good idea to check for them
    int control_socket = get_tcp_socket();
    listen_on_port(control_socket, state.control_port, 5);

    struct sockaddr_in client_sockaddr;
    socklen_t client_sockaddr_length = sizeof(client_sockaddr);
    printf("Waiting for connection.... ");
    int client_socket = accept(control_socket, (struct sockaddr*)&client_sockaddr, &client_sockaddr_length);
    if (client_socket < 0)
        die("client connection accept error");
    puts("connected");
    for (;;) {
        handle_client_request(client_socket);
    }
}


void handle_client_request(int client_fd) {
    int packet_type = NEW_CLIENT;
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
    printf("received NewClientPacket, port: %d\n", packet.port);
    start_tunneling(client_fd, packet);

}


void start_tunneling(int client_socket, NewClientPacket packet) {
    printf("Tunneling port %d\n", packet.port);
    if (pthread_create(&state.client_thr, NULL, client_thr_routine, client_socket))
        die("cannot create client thread");
    if (pthread_create(&state.tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");

    // TODO don't block wnen client connects
    void *junk;
    pthread_join(state.client_thr, &junk);
}


void *client_thr_routine(void *param) {
    int client_socket = (int) param;
    puts("created client thread, hurray.");

    vector_init(&connections);

    int tunneled_socket = get_tcp_socket();
    listen_on_port(tunneled_socket, state.tunneled_port, 5);
    int data_socket = get_tcp_socket();
    listen_on_port(data_socket, state.data_port, 5);
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
        receive_amount(conn.client, &(NewConnectionData){}, sizeof(NewConnectionData));

        pthread_mutex_lock(&connections.mutex);
        vector_add(&connections, conn);
        pthread_mutex_unlock(&connections.mutex);
    }
}
