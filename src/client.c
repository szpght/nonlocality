#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include "nonlocality.h"
#include "client.h"


ConnectionVector connections;


int main(int argc, char **argv) {
    if (argc < 6) {
        die("Usage: <server ip> <dest ip> <tunneled port> <control port> <data port>");
    }
    char *server_ip = argv[1];
    char *destination_ip = argv[2];
    uint16_t control_port = atoi(argv[4]);
    uint16_t tunneled_port = atoi(argv[3]);
    uint16_t data_port = atoi(argv[5]);
    pthread_t tunneling_thr;

    vector_init(&connections);
    signal(SIGUSR1, signal_handler);

    if (pthread_create(&tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");

    int control_fd = -1;
    while (control_fd == -1) {
        control_fd = connect_from_str(server_ip, control_port);
        sleep(1);
    }
    puts("connected to server");

    // listen for new connection requests
    for (;;) {
        NewConnectionData ncpacket;
        ssize_t received = receive_amount(control_fd, &ncpacket, sizeof(ncpacket));
        if (received < sizeof(ncpacket))
            die ("full NewConnectionData packet not received");
        printf("received NewConnectionData, seq: %d\n", ncpacket.seq);

        // create new connection
        ConnectionPair conn;
        conn.seq = ncpacket.seq;
        conn.server = connect_from_str(server_ip, data_port);
        puts("created new connection to server");

        // send sequence number of created connection
        ssize_t retval = send_amount(conn.server, &ncpacket, sizeof(ncpacket));
        if (retval < sizeof(ncpacket))
            die("cannot send sequence number of created connection");
        puts("sequence number sent");

        // create connection to machine behind firewall
        conn.client = connect_from_str(destination_ip, tunneled_port);

        // add connection to list
        pthread_mutex_lock(&connections.mutex);
        vector_add(&connections, conn);
        pthread_mutex_unlock(&connections.mutex);
        puts("connection added to list");
    }
}


// TODO check if works in wild
void signal_handler(int signal) {
    if (signal == SIGUSR1)
        print_connections(&connections);
}