#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include "nonlocality.h"
#include "client.h"



int main(int argc, char **argv) {
    char server_ip[] = "127.0.0.1";
    char destination_ip[] = "127.0.0.1";
    uint16_t control_port = atoi(getenv("CONTROL_PORT"));
    uint16_t tunneled_port = atoi(getenv("TUNNELED_PORT"));
    uint16_t data_port = atoi(getenv("DATA_PORT"));
    ConnectionVector connections;
    pthread_t tunneling_thr;
    int retval;

    vector_init(&connections);

    if (pthread_create(&tunneling_thr, NULL, tunneling_thr_routine, &connections))
        die("cannot create tunneling thread");

    int control_fd = connect_from_str(server_ip, control_port);
    puts("connected to server");

    // send tunneled port
    NewClientPacket packet = { .port = tunneled_port};
    ssize_t sent = send_amount(control_fd, &packet, sizeof(packet));
    if (sent < sizeof(packet))
        die("could send NewClientPacket");
    puts("tunneled port sent");

    // listen for new connection requests
    for (;;) {
        NewConnectionData ncpacket;
        ssize_t received = receive_amount(control_fd, &ncpacket, sizeof(ncpacket));
        if (received < sizeof(ncpacket))
            die ("full NewConnectionData packet not received");
        printf("received NewConnectionData, seq: %d\n", ncpacket.seq);

        // create new connection
        ConnectionPair conn;
        conn.server = connect_from_str(server_ip, data_port);
        puts("created new connection to server");

        // send sequence number of created connection
        retval = send_amount(conn.server, &ncpacket, sizeof(ncpacket));
        if (retval < sizeof(ncpacket))
            die("cannot send sequence number of created connection");
        puts("sequence number sent");

        // create connection to machine behind firewall
        conn.client = connect_from_str(destination_ip, tunneled_port);

        // add connection to list
        vector_add(&connections, conn);
        puts("connection added to list");
    }
}
