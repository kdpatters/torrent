/*
 * packet.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "packet.h"
#include "bt_parse.h"
#include "chunk.h"
#include "debug.h"

/*
 * pct_init
 */
void pct_init(data_packet_t *pack, char pack_type, int seq_num, int ack_num,
    char *payload, int payload_len) {

    // Calculate the packet length
    short packet_len = sizeof(header_t) + payload_len;
    if (packet_len > PACKETLEN) {
        fprintf(stderr, "Something went wrong: constructed packet is longer than the \
maximum possible length.\n");
        exit(1);
    }
    else if (packet_len <= 0) {
        fprintf(stderr, "Something went wrong: constructed packet has a \
non-positive length %d.\n", packet_len);
        exit(1); 
    }

    // Initialize packet
    memset(pack, 0, sizeof(*pack));
    DPRINTF(DEBUG_PACKETS, "pct_init: Initialized packet of size %ld\n", sizeof(*pack)); 

    // Create the header
    DPRINTF(DEBUG_PACKETS, "pct_init: Creating packet header\n"); 
    header_t *header = &pack->header;
    header->magicnum = MAGICNUM;
    header->version = VERSIONNUM;
    header->packet_type = pack_type;
    header->header_len = sizeof(*header);
    header->packet_len = packet_len;
    header->seq_num = seq_num;
    header->ack_num = ack_num;

    // Copy the data into the packet
    DPRINTF(DEBUG_PACKETS, "pct_init: Copying payload into packet\n"); 
    memcpy(pack->data, payload, payload_len);

    DPRINTF(DEBUG_PACKETS, "pct_init: Created packet of type %d\n", pack->header.packet_type); 
}

/* Create a payload formatted with a chunk count, some amount of 
 * padding then a series of chunk hashes. Used for WHOHAS and IHAVE
 * packets. Returns the length of the payload. */
int pct_hash_payload(char *payload, char n_hashes, char *hashes) {
  int hash_bytes = n_hashes * CHK_HASH_BYTES;                              
  int payload_len = PAYLOAD_TOPPER + hash_bytes;                           
  sprintf(payload, "%c%*c%*s", n_hashes, PADDING,                          
    ' ', hash_bytes, hashes);                  
  return payload_len;                            
}

void pct_whohas(data_packet_t *packet, char n_hashes, char *hashes) { 
  DPRINTF(DEBUG_PACKETS, "Creating WHOHAS packet\n");   
  char payload[DATALEN];                                                   
  int payload_len = pct_hash_payload(payload, n_hashes, hashes);
  pct_init(packet, WHOHAS_TYPE, 0, 0, payload, payload_len);               
}                                                                          
                                                                           
void pct_ihave(data_packet_t *packet, char n_hashes, char *hashes) {
  DPRINTF(DEBUG_PACKETS, "Creating IHAVE packet\n");       
  char payload[DATALEN];                                                   
  int payload_len = pct_hash_payload(payload, n_hashes, hashes);
  pct_init(packet, IHAVE_TYPE, 0, 0, payload, payload_len);
}                                                                          
                                                                           
void pct_get(data_packet_t *packet, char *hash) {
  DPRINTF(DEBUG_PACKETS, "Creating GET packet\n");                           
  pct_init(packet, GET_TYPE, 0, 0, hash, CHK_HASH_BYTES);                  
}                                                                          
                                                                           
void pct_data(data_packet_t *packet, int seq_num, char *data, int len) {
  DPRINTF(DEBUG_PACKETS, "Creating DATA packet\n"); 
  pct_init(packet, DATA_TYPE, seq_num, 0, data, len);                      
}                                                                          
                                                                           
void pct_ack(data_packet_t *packet, int ack_num) {
  DPRINTF(DEBUG_PACKETS, "Creating ACK packet\n");                          
  pct_init(packet, ACK_TYPE, 0, ack_num, "", 0);                           
}                                                                          
                                                                           
void pct_denied(data_packet_t *packet) {      
  DPRINTF(DEBUG_PACKETS, "Creating DENIED packet\n");                              
  pct_init(packet, DENIED_TYPE, 0, 0, "", 0);                              
}   

void pct_send(data_packet_t *pack, struct sockaddr_in *dest, int sockfd) {
    DPRINTF(DEBUG_SOCKETS, "pct_send: Sending packet of type %d\n", pack->header.packet_type);
    sendto(sockfd, pack, pack->header.packet_len, 0, 
        (const struct sockaddr *) dest, sizeof(*dest));
    DPRINTF(DEBUG_SOCKETS, "pct_send: Packet sent\n");
}
