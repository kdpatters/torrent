/*
 * packet.h
 * 
 * Header file for things about packets.
 */

#include <netinet/in.h>

// Header constants
#define MAGICNUM 3752
#define VERSIONNUM 1

// Packet types
#define WHOHAS_TYPE 0
#define IHAVE_TYPE 1
#define GET_TYPE 2
#define DATA_TYPE 3
#define ACK_TYPE 4
#define DENIED_TYPE 5

#define PACKETLEN 1500
#define PADDING 3
#define CHK_COUNT sizeof(char)
#define PAYLOAD_TOPPER (char) (CHK_COUNT + PADDING)
#define HEADER_SIZE sizeof(header_t)
#define DATALEN (PACKETLEN - HEADER_SIZE)
#define SPACELEFT (DATALEN - PADDING - CHK_COUNT)
#define MAX_CHK_HASHES (SPACELEFT / CHK_HASH_BYTES)

typedef struct header_s {
  short magicnum;
  char version;
  char packet_type;
  short header_len;
  short packet_len; 
  u_int seq_num;
  u_int ack_num;
} header_t;  

typedef struct data_packet {
  header_t header;
  char data[DATALEN];
} data_packet_t;

void pct_init(data_packet_t *, char, int, int, char *, int);
void pct_whohas(data_packet_t *packet, char n_hashes, char *hashes);
void pct_ihave(data_packet_t *packet, char n_hashes, char *hashes);
void pct_get(data_packet_t *packet, char *hash);
void pct_data(data_packet_t *packet, int seq_num, char *data, char len);
void pct_ack(data_packet_t *packet, int ack_num);
void pct_denied(data_packet_t *packet);
void pct_send(data_packet_t *, struct sockaddr_in *, int);
