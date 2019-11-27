/* Header file for peer */

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
#define CHK_COUNT 1
#define CHK_HASHLEN 20
#define DATALEN PACKETLEN - sizeof(header_t)

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