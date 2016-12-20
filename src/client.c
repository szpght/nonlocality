#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include "nonlocality.h"
#include "client.h"
#include "version.h"


ConnectionVector connections;


int main(int argc, char **argv) {
    puts("nonlocality client");
    print_version();
    if (argc < 6) {
        die("Usage: <server ip> <dest ip> <tunneled port> <control port> <data port>");
    }
    signal(SIGPIPE, SIG_IGN); // ignore broken pipe signal
    char *server_ip = argv[1];
    char *destination_ip = argv[2];
    uint16_t control_port = port_from_string(argv[4]);
    uint16_t tunneled_port = port_from_string(argv[3]);
    uint16_t data_port = port_from_string(argv[5]);
    pthread_t tunneling_thr;

    vector_init(&connections);

    if (pthread_create(&tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");

    int control_fd = connect_to_server(server_ip, control_port);

    puts("connected to server");

    // listen for new connection requests
    for (;;) {
        NewConnectionData ncpacket;
        for (;;) {
            ssize_t received = receive_amount_timeout(control_fd, &ncpacket, sizeof ncpacket, CLIENT_CONNECTION_RESET_TIME);
            if (received == sizeof ncpacket)
                break;
            puts("didn't receive ping/new connection on time - reconnecting");
            close(control_fd);
            control_fd = connect_to_server(server_ip, control_port);
        }

        if (ncpacket.seq == 0) {
            struct tm *tv = localtime(&(time_t){time(NULL)});
            char buffer[100];
            strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S", tv);
            printf("ping at %s\r", buffer);
            fflush(stdout);
            continue;
        }

        printf("new connection request, seq=%d\n", ncpacket.seq);

        // create new connection
        ConnectionPair conn = { .seq = ncpacket.seq };
        conn.server = connect_from_str(server_ip, data_port);
        puts("established connection to server");

        // send sequence number of created connection
        ssize_t retval = send_amount(conn.server, &ncpacket, sizeof(ncpacket));
        if (retval < sizeof(ncpacket))
            die("cannot send sequence number of created connection");
        puts("sequence number sent");

        // create connection to machine behind firewall
        conn.client = connect_from_str(destination_ip, tunneled_port);

        if (conn.client == -1) {
            sequence_message(conn.seq, "could not connect to client");
            close(conn.server);
            continue;
        }

        // add connection to list
        pthread_mutex_lock(&connections.mutex);
        vector_add(&connections, conn);
        pthread_mutex_unlock(&connections.mutex);
        printf("connection %d added to list\n", conn.seq);
    }
}


int connect_to_server(char *ip, uint16_t port) {
    int fd = -1;
    for (;;) {
        fd = connect_from_str(ip, port);
        if (fd != -1)
            return fd;
        sleep(CLIENT_CONNECT_RETRY_TIME_SEC);
    }
}
