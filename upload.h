// /* 
//  * Upload header file 
//  * upload.h
//  */

// #include <time.h>



#define MAX_UPLOADS 5 // Max no. of simultaneous connections for upload
#define MAX_LEECHERS 20 // Max # peers conn. during a single upload
#define T_OUT_DATA (MS_TO_S * 5) // 10 ms
#define T_OUT_ACK (MS_TO_S * 5) // 10 ms
#define MAX_RET_SEND_DATA 3
#define MAX_RET_REC_DATA 3
#define SEQ_NUMB 0
#define ACK_NUMB 0
#define NOT_BUSY 0
#define BUSY 1

 
typedef struct chunku_s {
  // Chunk info
    int chunk_id;
    char hash[CHK_HASH_BYTES];

  // Chunk upload status
    int state;
    int peer_id;
    int n_tries_send; // No. of tries before assuming packet lost
    //clock_t last_data_sent; 
    clock_t last_ack_recv;
  
  // Data
    char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
    data_packet_t *packetlist; // List of packets from each chunk
    int l_size; // Size of packetlist created
} chunku_t;

 typedef struct upload {
    chunku_t chunk;// Chunk to upload 
    int ack_ind; // Ack no. as int to index into recv
    clock_t t_start_send;
    int *recv; // To keep count of packets sent for upload and recv
    int busy; // Busy -- 1, Not busy - 0  
} upload_t;

void make_packets(upload_t *upl, char* buf, int buf_size);
void read_chunk(upload_t *upl, char *filename, char *buf);
