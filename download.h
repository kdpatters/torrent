/* 
 * download.h
 *
 */

#include <netinet/in.h>
#include <time.h>

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
#define TIME_WAIT_IHAVE 5 // Timeout in seconds

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

// Download of a single chunk
typedef struct chunkd_s {
  // Chunk info
  int chunk_id;
  char hash[CHK_HASH_BYTES];

  // Peer list to store IHAVE responses
  int *peer_list;
  int pl_filled;
  int pl_size;
    
  // Chunk download status
  int state;
  int peer;
  int n_tries_get;
  time_t last_get_sent;
  time_t last_data_recv;
  
  // Data
  char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
  data_node_t pieces;
} chunkd_t;

// Download of multiple requested chunks
typedef struct download_s {
  time_t time_started;
  int waiting_ihave;

  // Chunk info
  int n_chunks;
  int n_in_progress;
  chunkd_t *chunks;

  char *output_file[MAX_FILENAME];
} download_t;

int dload_rarest_chunk(download_t *download);
char dload_peer_add(download_t *download, int peer_id, int chunk_id);
void dload_start(download_t *, char *, int *, int);
