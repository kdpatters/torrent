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
 
<<<<<<< HEAD

typedef struct chunkd_s {
  // Chunk info
    int chunk_id;
    char hash[CHK_HASH_BYTES];

  // Chunk upload status
    int state;
    int peer_id;
    int n_tries_get;
    clock_t last_get_sent;
    clock_t last_data_recv;
  
  // Data
    char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
    data_packet_t *packetlist; //list of packets from each chunk
} chunkd_t;

 typedef struct upload {
    chunkd_t chunk;// chunk to upload 
    int recv[]; // to keep count of packets sent for upload and recv 
    int ack_ind; //ack no. as int to index into recv
    clock_t last_ack_rec;
} upload_t;


    typedef struct upload{
        upload_t uploads[MAX_UPLOADS];
    }upload_t;

    // Function to get ID for a specific chunk HASH
// Function to read a chunk based on its specific ID from a file; should
//   also verify that chunk is not corrupt using hash, then return a pointer to that chunk
// Function to divide a specific chunk into packets and store those packets in data structure
// Function to send packets
// Function to check timeouts for GET and DATA
// Function to process a GET request from another client -> do something, then update the struct
// Function to process an ACK from another client -> do something, then update the struct
// Function to process a DATA packet from another client -> do something, then update the struct
=======
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
>>>>>>> c55b58a794499f5f8cef2e85d675a1c7c0894f36
