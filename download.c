/* 
 * download.c
 * 
 * Implementations of file download feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "bt_parse.h"
#include "chunk.h"
#include "packet.h"
#include "download.h"
#include "debug.h"

#define INVALID_FIELD 0

void write_chunk(int id, char *fname, char *buf) {
  FILE *f = fopen(fname, "r+");
  if (f == NULL) {
    fprintf(stderr, "Problem opening file %s\n", fname);
    exit(1);
  }
  int position = id * BT_CHUNK_SIZE;
  int whence = SEEK_SET; // Offset bytes set to start of file
  if (fseek(f, position, whence)) {
      fprintf(stderr, "Could not read chunk %d\n", id);
      exit(1);
  }
  fwrite(buf, BT_CHUNK_SIZE, 1, f); // Read a chunk
  fclose(f);
}

/* 
 * dload_cumul_ack
 *
 * Returns the number to use to create a cumulative ACK for a given
 * chunk download.
 */
int dload_cumul_ack(chunkd_t *chk) {
    for (int i = 0; i < chk->pieces_size; i++) {
        /* 'pieces_filled' set to 1 only if we have received a
         * data packet with a sequence number of that index. */
        if (chk->pieces_filled[i] == 0) // Missing packet in sequence
            return i;
        else if (i == (chk->pieces_size - 1)) // Every element is a 1
            return i + 1;
    }
    fprintf(stderr, "Cannot create ACK.  No pieces have been stored yet for this chunk\n");
    exit(1);
}

// Returns true if chunk is verified and written to disk correctly.
char dload_verify_and_write_chunk(chunkd_t *chk, char *fname) {
  if (verify_hash((uint8_t *) chk->data, chk->total_bytes, (uint8_t *) chk->hash)) {
    DPRINTF(DEBUG_DOWNLOAD, 
      "dload_verify_and_write_chunk: Chunk %d successfully downloaded!\n", 
      chk->chunk_id);
    write_chunk(chk->chunk_id, fname, chk->data);
    return 0;
  }
  return 1;
}

void dload_assemble_chunk(chunkd_t *chk) {
  // Assemble pieces
  void *data_ptr = chk->data;
  for (int j = 0; j < chk->pieces_size; j++) {
    if (chk->pieces_filled[j]) {
      data_packet_t *dat = &chk->pieces[j];
      int n_bytes = dat->header.packet_len - dat->header.header_len;
      memcpy(data_ptr, dat->data, n_bytes);
      data_ptr += n_bytes;
    }
  }
}

void dload_store_data(chunkd_t *chk, data_packet_t pct) {
  // Check if it is necessary to resize pieces array
  int seq_num = pct.header.seq_num - 1;
  if (chk->pieces_size <= seq_num) {
    DPRINTF(DEBUG_DOWNLOAD, "dload_store_data: Chunk packet array is too small, allocating more memory\n");
    int old_size = chk->pieces_size;
    int new_size = (1 + seq_num) * 2;
    chk->pieces_size = new_size;
    chk->pieces = realloc(chk->pieces, sizeof(*chk->pieces) * chk->pieces_size);
    chk->pieces_filled = realloc(chk->pieces_filled, 
      sizeof(*chk->pieces_filled) * chk->pieces_size);
    memset(&chk->pieces_filled[old_size], 0, sizeof(*chk->pieces_filled) * 
      (new_size - old_size));
  }
  // Insert the data into the pieces array if slot is not already filled
  DPRINTF(DEBUG_DOWNLOAD, "dload_store_data: Copying packet data into chunk info in download struct\n");
  if (!chk->pieces_filled[seq_num]) {
    memcpy(&chk->pieces[seq_num], &pct, sizeof(pct));
    chk->pieces_filled[seq_num] = 1;
    DPRINTF(DEBUG_DOWNLOAD, "dload_store_data: Packet contains payload of %d bytes\n", pct.header.packet_len - pct.header.header_len);
    chk->total_bytes += pct.header.packet_len - pct.header.header_len;
  }
  else {
    DPRINTF(DEBUG_DOWNLOAD, "dload_store_data: Received duplicate of packet \
sequence no. %d\n", seq_num);
    exit(0);
  }

  // Store information about last_data_recv
  chk->last_data_recv = time(0);
}

char dload_complete(chunkd_t *chk) {
  return (chk->total_bytes >= BT_CHUNK_SIZE);
}

/* 
 * dload_rarest_chunk
 *
 * Stores list of indexes for chunks in download struct from rarest to
 * least rare in list 'chunks'.  Stores -1 for an index if the chunk
 * is busy or finished downloading already.
 */
int dload_rarest_chunk(int *rarest, download_t *download) {
    // Copy chunk values into an array for sorting
    struct elm {
        uint indx;
        uint val;
    };
    chunkd_t *chks = download->chunks;
    int n = download->n_chunks;
    int filled = 0;
    struct elm sorting[n];
    for (int i = 0; i < n; i++) {
      int state = chks[i].state;
      if (state == NOT_STARTED) {
         sorting[filled].val = chks[i].pl_filled;
         sorting[filled++].indx = i;
      }
    }

    // Insertion sort
    for (int i = 0; i < filled - 1; i++) {
        int j = i + 1;
        for (; (j > 0) && (sorting[j - 1].val > sorting[j].val); j--) {
            // Make a swap
            struct elm temp = sorting[j - 1];
            memcpy(&sorting[j - 1], &sorting[j], sizeof(sorting[j]));
            memcpy(&sorting[j], &temp, sizeof(temp));
        }
    }
   
    // Copy struct indicies into "rarest" list
    for (int i = 0; i < filled; i++) {
        rarest[i] = sorting[i].val;
    }
    return filled;
}

/* Add a peer to the download information for a specific chunk. Returns 
 * True if the peer was added successfully. */                             
char dload_peer_add(download_t *download, int peer_id, int chunk_id) {
    DPRINTF(DEBUG_DOWNLOAD, "dload_peer_add: Adding information for chunks peer has\n");
    // Iterate through requested chunks                                    
    for (int i = 0; i < download->n_chunks; i++) {                         
      chunkd_t *chk = &download->chunks[i];                                
                                                                           
      // Check if we found the IHAVE chunk in the requested chunks
      if (chk->chunk_id == chunk_id) {                                     
      DPRINTF(DEBUG_DOWNLOAD, "dload_peer_add: Matched IHAVE chunk %d in chunks requested to download, adding peer %d\n", chunk_id, peer_id);         
                                                                           
        // Resize the peer list if necessary                               
        if (chk->pl_size <= chk->pl_filled) {                              
          chk->pl_size = chk->pl_size ? chk->pl_size * 2 : 1;                         
          chk->peer_list = realloc(chk->peer_list,                         
             sizeof(*chk->peer_list) * chk->pl_size);                      
        }                                                                  
                                                                           
        // Finally add the peer                                            
        chk->peer_list[chk->pl_filled++] = peer_id;
        DPRINTF(DEBUG_DOWNLOAD, "dload_peer_add: Chunk %d now has %d peers\n", chunk_id, chk->pl_filled);
        return 1;                                                          
      }                                                                    
    }                                                                      
    return 0;                                                              
}    

/* 
 * free_chunk
 *
 * Free arrays inside of a chunk.
 */
void free_chunk(chunkd_t *chk) {
    DPRINTF(DEBUG_DOWNLOAD, "free_chunk: Freeing structs inside chunkd_t\n");
    if (chk != NULL) {
        if (chk->pl_size)
            free(chk->peer_list);
        if (chk->pieces_size) {
            free(chk->pieces);
            free(chk->pieces_filled);
        }
    }
}

/* 
 * dload_clear
 *
 * Free allocated structures within download and set download struct 
 * memory to 0 if it has previously been initialized.
 */
void dload_clear(download_t *download) {
  if (!download->initd) // Download is not initialized
    return;
  DPRINTF(DEBUG_DOWNLOAD, "dload_clear: Freeing previously allocated chunks if any\n");
  for (int i = 0; i < download->n_chunks; i++) {
    free_chunk(&download->chunks[i]);
  }
  DPRINTF(DEBUG_DOWNLOAD, "dload_clear: Freeing the chunks array itself\n");
  free(download->chunks); // Free the chunks array itselif

  // Ensure that all parts of the download struct are 0
  memset(download, 0, sizeof(*download)); 
}

/* Start the download process as a result of the GET command. */
void dload_start(download_t *download, char *hashes, int *ids, 
  int n_hashes, char *outputf) {

  DPRINTF(DEBUG_DOWNLOAD, "dload_start: Initializing download\n");
  dload_clear(download); // Clear bits if already initialized
  
  // Start the download timer
  download->time_started = time(0);
  download->waiting_ihave = 1;

  int chks_size = sizeof(*download->chunks) * n_hashes;  
  download->chunks = malloc(chks_size);
  memset(download->chunks, 0, chks_size);
  DPRINTF(DEBUG_DOWNLOAD, "dload_start: Copying hashes into chunk array\n");
  for (int i = 0; i < n_hashes; i++) {
    DPRINTF(DEBUG_DOWNLOAD, "dload_start: Added chunk %d to array\n", ids[i]);
    chunkd_t *chk = &download->chunks[i];
    chk->chunk_id = ids[i];
    strncpy(chk->hash, &hashes[i * CHK_HASH_BYTES], CHK_HASH_BYTES);
  }
  download->n_chunks = n_hashes;
  strncpy(download->output_file, outputf, MAX_FILENAME);
  download->initd = 1; // Download has now been initialized
  DPRINTF(DEBUG_DOWNLOAD, "dload_start: Done\n");
}

/* 
 * dload_ihave_done
 * 
 * Returns boolean for whether peer should stop waiting for IHAVE packets. 
 */
char dload_ihave_done(download_t *download) {                            
  double time_passed = difftime(time(0), download->time_started); 
  return (download->waiting_ihave && (time_passed > TIME_WAIT_IHAVE));
}

/* 
 * dload_pick_peer
 * 
 * Given a specific index to the chunk array in 'download' struct, returns the
 * index of a suitable peer to download the chunk from.  If no suitable peer is
 * found, returns -1;
 */
char dload_pick_peer(download_t *download, char *peer_free, int indx) {
  DPRINTF(DEBUG_DOWNLOAD, "dload_pick_peer: Looking for suitable peer from which to download chunk\n");

  // Iterate through peers for given chunk and check if each is free
  for (int i = 0; i < download->chunks[indx].pl_filled; i++) {

    int peer_id = download->chunks[indx].peer_list[i];

    if (peer_free[peer_id]) { // Found a free peer
      return peer_id;
    }

  }

  return -1;
}

/*
 * dload_pick_chunk
 *  
 * Chooses the next chunk to download, sets the peer to download the chunk from,
 * then returns the index of the chunk in the chunk array in the 'download' 
 * struct. Returns -1 if no suitable chunk found.
 * 
 */
char dload_pick_chunk(download_t *download, char *peer_free) {
  DPRINTF(DEBUG_DOWNLOAD, "download_pick_chunk: Choosing next chunk to download\n");

  // Create list of chunks from rarest to least rare
  chunkd_t *chks = download->chunks;
  int rarest[download->n_chunks];
  int n_rarest = dload_rarest_chunk(rarest, download);     

  // Iterate through the list and try to find a chunk with at least one free peer
  for (int i = 0; i < n_rarest; i++) {
    int chunk_id = rarest[i];
    int peer_id = dload_pick_peer(download, peer_free, chunk_id);

    if (peer_id > 0) { // Found a suitable peer
      chks[chunk_id].peer = peer_id;
      return chunk_id;
    }

  }

  return -1;
}

void dload_chunk(download_t *download, int indx, struct sockaddr_in *addr, 
  int sock, char *peer_free) {
  DPRINTF(DEBUG_DOWNLOAD, "dload_chunk: Sending GET to initiate download process\n");

  chunkd_t *chk = &download->chunks[indx];

  // Send GET request for specific chunk
  data_packet_t pack;                                                                   
  pct_get(&pack, chk->hash);                
  pct_send(&pack, addr, sock); 
  
  download->waiting_ihave = 0; // Stop waiting for IHAVE
  chk->state = DOWNLOADING;
  peer_free[chk->peer] = 0;
}

/* 
 * can_download
 * 
 * Check that client has not already begun to download the given chunk. 
 */
//char can_download(download_t *download) {
//   return downloads->n < MAX_DOWNLOADS;
// }

/*
 * ready_download
 * 
 * Initialize the download.
 */
// void ready_download(download_t download) {
//     memset(&download, 0, sizeof(download));
// }

// /* 
//  * download_next_chunk
//  * 
//  * Find and return the index of the next chunk to download.
//  */
// int download_next_chunk(download_t *download) {
//     int i = 0;
//     for (; download->chunk_download[i].state; i++) {
//         if (i >= MAX_DOWNLOADS - 1) {
//             fprintf(stderr, "Attempted to add a new download when max 
// downloads exceeded.\n");
//             exit(1);
//         }
//     }
//     return i;
// }

// /*
//  * check_retry_get
//  * 
//  * Check timeout for when last data packet received and last get sent have been 
//  * exceeded.  If less than the maximum number of gets have been sent, then 
//  * resend another get.  Otherwise, 
//  */
// void check_retry_get(chunkd_t chunkd, clock_t now) {
//     /* 
//      * Check if timeouts for when last get was sent and last data packet was 
//      * received have been exceeded.
//      */
//     if ((now - chunkd.last_get_sent > GET_WINDOW) &&
//         (now - chunkd.last_data_recv > DATA_WINDOW)) {
//         if (chunkd.n_tries_get < MAX_RETRIES_GET) {
//             // TODO
//             //send_get(...);
//         }
//         else {
//             /* Try to finish by downloading the chunk from a different peer. */
//             // TODO
//         }
//     }
//     // Check when the last data packet was received
//     // Either attempt to verify chunk, stop download entirely (if max retries exceeded), or send another GET
// }

// void check_chunk_downloads(download_t *download) {
//     clock_t now = clock();

//     for (int i = 0; i < download->n_chunks; i++) {
//         if (download->chunks[i].state == WAIT_DATA) {
//             check_retry_get(download->chunks[i], now);
//         }
//     }
// }

