/* 
 * download.h
 *
 */

#include <time.h>
#include "bt_parse.h"
#include "packet.h"

#define MAX_DOWNLOADS 1

// Chunk download statuses
#define NOT_STARTED 0
#define WAIT_IHAVE 1 // Waiting for IHAVE responses
#define WAIT_DATA 2 // Waiting for data packets
#define STALLED 3 // Download is unable to complete
#define STOPPED 4 // Download has finished or user has stopped download

// Download windows
#define MS_TO_S 1000
#define GET_WINDOW (MS_TO_S * 5)
#define DATA_WINDOW (MS_TO_S * 5)
#define MAX_RETRIES_GET 3

// Filename lengths
#define MAX_FILENAME 100

// Each data node includes its sequence number and a pointer to a byte array
typedef struct data_node_s {
    int seq_num;
    int data_len;
    void *data;
    struct data_node_s *prev;
    struct data_node_s *next; 
} data_node_t;

typedef struct chunkd_s {
  // Chunk info
  int chunk_id;
  char hash[CHK_HASH_BYTES];

  // Chunk download status
  int state;
  int peer_id;
  int n_tries_get;
  clock_t last_get_sent;
  clock_t last_data_recv;
  
  // Data
  char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
  data_node_t pieces;
} chunkd_t;

typedef struct download_s {
  clock_t time_started;

  // Chunk info
  int n_chunks;
  chunkd_t *chunks;

  char *output_file[MAX_FILENAME];
} download_t;

void process_ihave(data_packet_t *packet, bt_config_t *config, 
  struct sockaddr_in *from);
