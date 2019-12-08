/* 
 * Upload header file 
 * upload.h
 */

#include <time.h>

// #define MAX_UPLOADS 5 // max no. of simultaneous connections for upload
// #define MAX_SEEDERS 20 // max # peers conn. during a single upload
// #define TIMEOUT_DATA 10 // ms
// #define TIMEOUT_ACK 10 // ms
// #define MAX_RETRIES_SEND_DATA 3
// #define MAX_RETRIES_REC_DATA 3
// #define SEQ_NUM 0
// #define ACK_NUM 0
 
//  // Each data node includes its sequence number and a pointer to a byte array
// typedef struct data_node_s {
//     int seq_num;
//     int data_len;
//     data_packet_t *data;
//     struct data_node_s *prev;
//     struct data_node_s *next; 
// } data_node_t;

// typedef struct chunkd_s {
//   // Chunk info
//     int chunk_id;
//     char hash[CHK_HASH_BYTES];

//   // Chunk download status
//     int state;
//     int peer_id;
//     int n_tries_get;
//     clock_t last_get_sent;
//     clock_t last_data_recv;
  
//   // Data
//     char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
//     data_node_t piece;
// } chunkd_t;

//  typedef struct upload {
//     chunkd_t chunk;// chunk to upload 
//     int recv[]; // to keep count of packets sent for upload and recv 
//     int ack_ind; //ack no. as int to index into recv
//     clock_t last_ack_rec;
// } upload_t;
