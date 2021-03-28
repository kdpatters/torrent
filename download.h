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
#define DOWNLOADING 2 // Waiting for data packets
#define STALLED 3 // Download is unable to complete
#define STOPPED 4 // Download has finished or user has stopped download
#define COMPLETE 5

// Download windows
#define MS_TO_S 1000
#define GET_WINDOW (MS_TO_S * 5)
#define DATA_WINDOW (MS_TO_S * 5)
#define MAX_RETRIES_GET 3
#define TIME_WAIT_IHAVE 5 // Timeout in seconds

// Filename lengths
#define MAX_FILENAME 100

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
  int recv_first_data;
  time_t last_get_sent;
  time_t last_data_recv;
  
  // Data
  char data[BT_CHUNK_SIZE]; // Space reserved for entire chunk
  data_packet_t *pieces;
  int pieces_size;
  int total_bytes;  // Size of pieces array
  int *pieces_filled;
} chunkd_t;

// Download of multiple requested chunks
typedef struct download_s {
  int initd; // Boolen for whether download has been initialized
  time_t time_started;
  int waiting_ihave;
  int n_ihave_waiting; // Number of IHAVE responses waiting for
  int n_ihave_recv; // Number of IHAVE responses received
  time_t last_update;

  // Chunk info
  int n_chunks;
  int n_in_progress;
  chunkd_t *chunks;

  char output_file[MAX_FILENAME];
} download_t;

void dload_chunk(download_t *download, int indx, struct sockaddr_in *addr, int sock, char *);
char dload_pick_chunk(download_t *download, char *peer_free);
char dload_ihave_done(download_t *download);
char dload_peer_add(download_t *download, int peer_id, int chunk_id);
void dload_start(download_t *, char *, int *, int, char *, int);
char dload_complete(chunkd_t *chk);
void dload_store_data(chunkd_t *chk, data_packet_t pct);
void dload_assemble_chunk(chunkd_t *chk);
char dload_verify_and_write_chunk(chunkd_t *chk, char *fname);
int dload_cumul_ack(chunkd_t *chk);
void dload_clear(download_t *);
