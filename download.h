/* 
 * download.h
 *
 */

#include <time.h>
#define MAX_DOWNLOADS 1

typedef struct dll_node_s {
    int val;
    struct dll_node_s prev;
    struct dll_node_s next; 
} dll_node_t;

typedef struct ihave_list_s {
  char hash[CHK_HASH_BYTES];
  bt_peer_t peers;
} ihave_list_t;

typedef struct chunk_download {
  char hash[CHK_HASH_BYTES];
  void * data; // 512KB for chunk data
  dll_node_t pieces;
  clock_t last_received;
}

typedef struct download_s {
  ihave_list_t ihave_recv[]; // Array of hashes with linked list for peers with each
  clock_t time_started;
  int state; // 0: invalid, 1: waiting for ihave, 2: waiting for data, 3: stalled, 4: stopped
} download_t;

typedef struct downloads_s {
    download_t downloads[MAX_DOWNLOADS];
    int n; // Number of concurrent downloads
} downloads_t;

char not_at_max_downloads();
void begin_download(char hash[][MAX_HASH_BYTES]);
void send_get();
void check_retry_get();
void process_data();
void verify_chunk();
    
// Function to get ID for a specific chunk HASH
// Function to read a chunk based on its specific ID from a file; should
//   also verify that chunk is not corrupt using hash, then return a pointer to that chunk
// Function to divide a specific chunk into packets and store those packets in data structure
// Function to send packets
// Function to check timeouts for GET and DATA
// Function to process a GET request from another client -> do something, then update the struct
// Function to process an ACK from another client -> do something, then update the struct
// Function to process a DATA packet from another client -> do something, then update the struct
