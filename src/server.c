#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
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
    server();
    return 0;
}


void load_config(char **argv) {
    state.control_port = port_from_string(argv[2]);
    state.tunneled_port = port_from_string(argv[1]);
    state.data_port = port_from_string(argv[3]);
}


void server() {
    vector_init(&connections);

    int control_socket = get_tcp_socket();
    listen_on_port(control_socket, state.control_port, 5);
    start_tunneling();

    for (;;) {
        state.client_socket = accept_jauntily(control_socket);
        if (state.client_socket < 0)
            die("client connection accept error");
        puts("client connected");

        // TODO ping
    }
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
        ConnectionPair conn = { .seq = state.seq++, -1, -1 };

        // accept connection to be tunneled
        conn.server = accept_jauntily(tunneled_socket);
        sequence_message(conn.seq, "accepted connection");

        // tell client to make new connection
        // if contact unsuccesfull, disconnect tunneled connection
        NewConnectionData packet = { .seq = conn.seq };
        int sent = send_to_client((char *) &packet, sizeof(packet));
        if (sent == -1) {
            sequence_message(conn.seq, "could not send connection request to client - disconnecting");
            goto error;
        }

        // wait for client making connection
        conn.client = accept_timeout(data_socket);
        if (conn.client == -1) {
            sequence_message(conn.seq, "accept timed out - disconnecting");
            goto error;
        }
        sequence_message(conn.seq, "accepted connection from client");

        // receive sequence number
        ssize_t received = receive_amount(conn.client, &(NewConnectionData){}, sizeof(NewConnectionData));
        if (received < sizeof(NewConnectionData)) {
            sequence_message(conn.seq, "could not receive sequence number - disconnecting");
            goto error;
        }
        sequence_message(conn.seq, "received sequence number - connection established");

        pthread_mutex_lock(&connections.mutex);
        vector_add(&connections, conn);
        pthread_mutex_unlock(&connections.mutex);

        continue;
        error:
        close(conn.server);
        close(conn.client);
    }
}

int send_to_client(char *buffer, size_t size) {
    // TODO implement function correctly
    return send_amount(state.client_socket, buffer, size);
}
