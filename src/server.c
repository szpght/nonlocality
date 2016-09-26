#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "nonlocality.h"
#include "server.h"


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
    int client_socket = accept(control_socket, (struct sockaddr*)&client_sockaddr, (socklen_t*)&client_sockaddr_length);
}

void a()
{
    int client_connected = 0;
    int server_socket, some_junk;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    // TODO error checking

    struct sockaddr_in server = {
            .sin_family = AF_INET,
            .sin_addr.s_addr = INADDR_ANY,
            .sin_port = htons(6666)
    }, client;
    bind(server_socket, (struct sockaddr*)&server, sizeof(server ));
    listen(server_socket, 2);
    puts("listening");

    int new_socket = accept(server_socket, (struct sockaddr*)&client, (socklen_t*)&some_junk);
    char buffer[1000] = "55555555555";
    unsigned char *tmp;
    int received;
    received = recv(new_socket, buffer, 1000, 0);
    buffer[received] = 0;
    tmp = buffer;
    for (; *tmp != 0; tmp++)
        printf("%.2x ", *tmp);
    printf("\n%d: %s", received, buffer);
}
