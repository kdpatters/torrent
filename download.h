/* 
 * download.h
 *
 */

#include <time.h>
#include "bt_parse.h"
#include "packet.h"

#define MAX_DOWNLOADS 1
// Download statuses
#define DLOAD_INVALID 0
#define DLOAD_WAIT_IHAVE 1 // Waiting for IHAVE responses
#define DLOAD_WAIT_DATA 2 // Waiting for data packets
#define DLOAD_STALLED 3 // Download is unable to complete
#define DLOAD_STOPPED 4 // Download has finished or user has stopped download

// Windows
#define MS_TO_S 1000
#define GET_WINDOW (MS_TO_S * 5)
#define DATA_WINDOW (MS_TO_S * 5)
#define MAX_RETRIES_GET 3

typedef struct dll_node_s {
    int val;
    struct dll_node_s *prev;
    struct dll_node_s *next; 
} dll_node_t;

typedef struct ihave_list_s {
  int chunk_id;
  int n_peers;
  bt_peer_t peers;
} ihave_list_t;

typedef struct chunk_download_s {
  char hash[CHK_HASH_BYTES];
  void * data; // 512KB for chunk data
  int peer_id; // Identity of peer that to download chunk from
  dll_node_t pieces;
  int n_tries_get;
  clock_t last_get_sent;
  clock_t last_data_recv;
} chunk_download_t;

typedef struct dload_s {
  chunk_download_t chunk_download;
  clock_t time_started;
  int state;
  int n_hashes;
  ihave_list_t *ihave_recv; // Array of hashes with linked list for peers with each
} dload_t;

typedef struct downloads_s {
    dload_t downloads[MAX_DOWNLOADS];
    int n; // Number of concurrent downloads
} downloads_t;

void process_ihave(data_packet_t *packet, bt_config_t *config, 
  struct sockaddr_in *from);
    
// Function to get ID for a specific chunk HASH
// Function to read a chunk based on its specific ID from a file; should
//   also verify that chunk is not corrupt using hash, then return a pointer to that chunk
// Function to divide a specific chunk into packets and store those packets in data structure
// Function to send packets
// Function to check timeouts for GET and DATA
// Function to process a GET request from another client -> do something, then update the struct
// Function to process an ACK from another client -> do something, then update the struct
// Function to process a DATA packet from another client -> do something, then update the struct
