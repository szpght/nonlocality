#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "nonlocality.h"
#include "server.h"


void handle_client_request(int client_fd);

void start_tunneling(NewClientPacket packet);

int main(int argc, char **argv) {
    puts("hello, wonderful world");
    ServerConfig config = load_config();
    server(config);
    return 0;
}


ServerConfig load_config() {
    ServerConfig config;
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
    ssize_t received = receive_amount(client_fd, &packet, sizeof(packet));
    if (received < sizeof(packet))
        die("error!!!1!11");
    start_tunneling(packet);

}


void start_tunneling(NewClientPacket packet) {

}
