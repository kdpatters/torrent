/*
 * packet.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "packet.h"
#include "bt_parse.h"

/*
 * init_packet
 */
void init_packet(data_packet_t *pack, char pack_type, int seq_num, int ack_num,
    char *payload, int payload_len) {

    // Initialize packet
    memset(pack, 0, sizeof(*pack));

    // Calculate the packet length
    short packet_len = sizeof(header_t) + payload_len;
    if (packet_len > PACKETLEN) {
        fprintf(stderr, "Something went wrong: constructed packet is longer than the \
maximum possible length.");
        exit(-1);
    }
    else if (packet_len <= 0) {
        fprintf(stderr, "Something went wrong: constructed packet has a \
non-positive length.");
        exit(-1); 
    }

    // Create the header
    header_t *header = &pack->header;
    header->magicnum = MAGICNUM;
    header->version = VERSIONNUM;
    header->packet_type = pack_type;
    header->header_len = sizeof(header);
    header->packet_len = packet_len;
    header->seq_num = seq_num;
    header->ack_num = ack_num;

    // Copy the data into the packet
    memcpy(pack->data, payload, payload_len);
}

void send_pack(data_packet_t *pack, struct sockaddr_in *dest, bt_config_t *config) {
    // Create the socket
    int sockfd = config->sock;

    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(myaddr));
    memcpy(&myaddr.sin_addr, &dest->sin_addr, sizeof(dest->sin_addr));
    memcpy(&myaddr.sin_port, &dest->sin_port, sizeof(dest->sin_port));
    myaddr.sin_family = AF_INET;

    sendto(sockfd, pack, pack->header.packet_len, 0, 
    (const struct sockaddr *) &myaddr, sizeof(myaddr));
}