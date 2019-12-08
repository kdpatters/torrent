/*
 * packet.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "packet.h"
#include "bt_parse.h"
#include "chunk.h"

/*
 * pct_init
 */
void pct_init(data_packet_t *pack, char pack_type, int seq_num, int ack_num,
    char *payload, int payload_len) {

    // Calculate the packet length
    short packet_len = sizeof(header_t) + payload_len;
    if (packet_len > PACKETLEN) {
        fprintf(stderr, "Something went wrong: constructed packet is longer than the \
maximum possible length.");
        exit(1);
    }
    else if (packet_len <= 0) {
        fprintf(stderr, "Something went wrong: constructed packet has a \
non-positive length.");
        exit(1); 
    }

    // Initialize packet
    memset(pack, 0, sizeof(*pack));

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

void pct_whohas(data_packet_t *packet, char n_hashes, char *hashes) {   
  char payload[DATALEN];                                                   
  int hash_bytes = n_hashes * CHK_HASH_BYTES;                              
  int payload_len = PAYLOAD_TOPPER + hash_bytes;                           
  sprintf(payload, "%c%*c%*s", n_hashes, PADDING,                          
    ' ', hash_bytes, hashes);                                              
  pct_init(packet, WHOHAS_TYPE, 0, 0, payload, payload_len);               
}                                                                          
                                                                           
void pct_ihave(data_packet_t *packet, char n_hashes, char *hashes) {       
  pct_init(packet, IHAVE_TYPE, 0, 0, hashes, n_hashes * CHK_HASH_BYTES);
}                                                                          
                                                                           
void pct_get(data_packet_t *packet, char *hash) {                          
  pct_init(packet, GET_TYPE, 0, 0, hash, CHK_HASH_BYTES);                  
}                                                                          
                                                                           
void pct_data(data_packet_t *packet, int seq_num, char *data, char len) {
  pct_init(packet, DATA_TYPE, seq_num, 0, data, len);                      
}                                                                          
                                                                           
void pct_ack(data_packet_t *packet, int ack_num) {                         
  pct_init(packet, ACK_TYPE, 0, ack_num, "", 0);                           
}                                                                          
                                                                           
void pct_denied(data_packet_t *packet) {                                   
  pct_init(packet, DENIED_TYPE, 0, 0, "", 0);                              
}   

void pct_send(data_packet_t *pack, struct sockaddr_in *dest, int sockfd) {
    sendto(sockfd, pack, pack->header.packet_len, 0, 
        (const struct sockaddr *) dest, sizeof(*dest));
}
