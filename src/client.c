#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include "nonlocality.h"
#include "client.h"

int main(int argc, char **argv) {
    char server_ip[] = "127.0.0.1";
    uint16_t control_port = atoi(getenv("CONTROL_PORT"));
    uint16_t tunneled_port = atoi(getenv("TUNNELED_PORT"));
    uint16_t data_port = atoi(getenv("DATA_PORT"));


    int control_fd = connect_from_str(server_ip, control_port);

    // send tunneled port
    NewClientPacket packet = { .port = tunneled_port};
    ssize_t sent = send_amount(control_fd, &packet, sizeof(packet));
    if (sent < sizeof(packet))
        die("could send NewClientPacket");

    // listen for new connection requests
    for (;;) {
        NewConnectionData ncpacket;
        ssize_t received = receive_amount(control_fd, &ncpacket, sizeof(ncpacket));
        if (received < sizeof(ncpacket))
            die ("full NewConnectionData packet not received");

        // create new connection
        int new_conn = connect_from_str(server_ip, data_port);

        // send sequence number of created connection
        send_amount(new_conn, &ncpacket, sizeof(ncpacket));

        // TODO create connection to machine behind firewall

        // TODO add connection to list

    }
}
