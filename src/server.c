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
    signal(SIGPIPE, SIG_IGN); // ignore broken pipe signal
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
    pthread_mutex_init(&state.client_socket_send_mutex, NULL);
    state.client_socket = -1;

    int control_socket = get_tcp_socket();
    listen_on_port(control_socket, state.control_port, 5);
    start_threads();

    for (;;) {
        int new_client_connection = accept_jauntily(control_socket);
        close(state.client_socket);
        state.client_socket = new_client_connection;
        if (state.client_socket < 0)
            die("client connection accept error");
        puts("client connected");
    }
}


void *ping_thr_routine(void *param) {
    for (;;) {
        usleep(PING_SEND_INTERVAL_SEC * 1000000);
        send_amount_timeout(state.client_socket, &(uint32_t){0}, 4, PING_SEND_TIMEOUT_SEC);
    }
}


void start_threads() {
    printf("Tunneling port %d\n", state.tunneled_port);
    if (pthread_create(&state.client_thr, NULL, client_thr_routine, NULL))
        die("cannot create client thread");
    if (pthread_create(&state.tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");
    if (pthread_create(&state.ping_thr, NULL, ping_thr_routine, NULL))
        die("cannot create ping thread");
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
    ssize_t sent;
    for (;;) {
        sent = send_amount_timeout(state.client_socket, buffer, size, CLIENT_SEND_TIMEOUT_SEC);
        if (sent == size)
            return sent;
        sleep(SEND_TO_CLIENT_RETRY_INTERVAL_SEC);
    }
}
