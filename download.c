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

int dload_rarest_chunk(download_t *download) {
    // Find the rarest chunk
    int n_peers = download->chunks[0].pl_filled;
    int rarest = 0; // Index of rarest chunk in download array
    for (int i = 0; i < download->n_chunks; i++) {
        int count = download->chunks[i].pl_filled;
        if (count < n_peers) {
            rarest = i;
            n_peers = count;
        }
    }
    return rarest;
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

/* Start the download process as a result of the GET command. */
void dload_start(download_t *download, char *hashes, int *ids, 
  int n_hashes, char *outputf) {
  DPRINTF(DEBUG_INIT, "dload_start: Initializing download\n");
  
  // Start the download timer
  download->time_started = time(0);
  download->waiting_ihave = 1;

  download->chunks = malloc(sizeof(*download->chunks) * n_hashes);
  DPRINTF(DEBUG_INIT, "dload_start: Copying hashes into chunk array\n");
  for (int i = 0; i < n_hashes; i++) {
    DPRINTF(DEBUG_INIT, "dload_start: Added chunk %d to array\n", ids[i]);
    chunkd_t *chk = &download->chunks[i];
    chk->chunk_id = ids[i];
    strncpy(chk->hash, &hashes[i * CHK_HASH_BYTES], CHK_HASH_BYTES);
    chk->state = WAIT_IHAVE;
  }
  download->n_chunks = n_hashes;
  strncpy(download->output_file, outputf, MAX_FILENAME);
  DPRINTF(DEBUG_INIT, "dload_start: Done\n");
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
  DPRINTF(DEBUG_DOWNLOAD, "download_pick_peer: Looking for suitable peer from which to download chunk\n");

  // Iterate through peers for given chunk and check if each is free
  for (int i = 0; i < download->chunks[indx].pl_filled; i++) {

    int peer_id = download->chunks[indx].peer_list[indx];

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
  int rarest = dload_rarest_chunk(download);                     
  chunkd_t *rarest_chunks = &download->chunks[rarest];

  // Iterate through the list and try to find a chunk with at least one free peer
  for (int i = 0; i < 1; i++) {

    int peer_id = dload_pick_peer(download, peer_free, i);
    if (peer_id > 0) { // Found a suitable peer
      rarest_chunks[i].peer = peer_id;
      return i;

    }

  }

  return -1;
}

void dload_chunk(download_t *download, int indx, struct sockaddr_in *addr, 
  int sock) {
  DPRINTF(DEBUG_DOWNLOAD, "dload_chunk: Sending GET to initiate download process\n");

  data_packet_t pack;                                                                   
  pct_get(&pack, download->chunks[indx].hash);                
  pct_send(&pack, addr, sock); 
  download->waiting_ihave = 0; // Stop waiting for IHAVE   
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
//  * create_get_packet
//  */
// void create_get_packet(data_packet_t *pack, char hash[CHK_HASH_BYTES]) {
//     init_packet(pack, GET_TYPE, INVALID_FIELD, INVALID_FIELD, hash, 
//         CHK_HASH_BYTES);
// }

// /*
//  * send_get 
//  */
// void send_get(struct sockaddr_in *dest, char hash[CHK_HASH_BYTES], 
//     bt_config_t *config) {

//     data_packet_t pack;
//     create_get_packet(&pack, hash); // Create the GET packet
//     send_pack(&pack, dest, config); // Send the GET packet
// }

// void begin_download() {
//     // DO SOMETHING...
// }

/* 
 * init_download
 * 
 * Initialize the struct to keep track of the chunks and connected peers for the
 * specific download.
 */
// void init_download(char hash[][CHK_HASH_BYTES], int n_chunks, 
//     download_t *download, bt_config_t *config) {
    
//     // Get a pointer to the first empty download
//     download->ihave_recv = malloc(sizeof(ihave_list_t) * n_chunks);
//     download->n_chunks = n_chunks;
//     for (int i = 0; i < n_chunks; i++) {
//         // Get and store hash ID
//         download->ihave_recv[i].chunk_id = hash2id(hash[i], config);
//     }

//     // Officially start the download
//     download->state = DLOAD_WAIT_IHAVE;
//     download->time_started = clock();
// }

// void process_ihave(data_packet_t *packet, bt_config_t *config, 
//     struct sockaddr_in *from) {

//     int inc = CHK_HASH_BYTES; // Amount to increment offset by
//     // Store the length of the packet
//     int pack_len = packet->header.packet_len;
//     int max_offset = pack_len - inc;

//     // Parse the number of chunks in the file
//     int n_chunks;
//     memcpy(&n_chunks, packet->data, CHK_COUNT);

//     int offset = CHK_COUNT + PADDING;
//     char hash[inc]; // Temporary variable to store hash
//     for (int i = 0; i < n_chunks; i++) {
//         if (offset > max_offset) {
//             fprintf(stderr, "Parsed %d chunk hashes from IHAVE but expected %d.",
//             i + 1, n_chunks);
//             exit(1);
//         }
//         // Read and store the next hash
//         memcpy(hash, packet->data + offset, inc);

//         // THIS NEEDS A LOT OF WORK; TODO
//         // Try to match returned hash to requested one and store peer identity
//         //int id = hash2id(hash, config);
//         //for (int j = 0; j < downloads[0].n_chunks; j++) {
//         //    if (downloads[0].ihave_recv[j].chunk_id == id) {
//                 // Add peer to peer list
//                 //add_peer(downloads[0].ihave_recv[j].peers, SOMETHING);
//         //    }
//         //}

//         offset += inc;
//     }
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

// /*
//  * create_ack_packet
//  * 
//  * Wrapper for init_packet function to simplify ACK packet creation.
//  */
// void create_ack_packet(data_packet_t *pack, int ack_num) {
//     init_packet(pack, ACK_TYPE, INVALID_FIELD, ack_num, NULL, 0);
// }

// /*
//  * send_ack
//  * 
//  * Creates and sends an ACK packet with a specific ACK number.
//  */
// void send_ack(struct sockaddr_in *dest, int ack_num, bt_config_t *config) {
//     data_packet_t pack;
//     create_ack_packet(&pack, ack_num);
//     send_pack(&pack, dest, config);
// }

// /*
//  * verify_chunk
//  */
// void verify_chunk() {
//     // Generate hash for chunk data
//     // Compare generated hash to current hash
// }

// /*
//  * is_chunk_done
//  * 
//  * Returns a boolean for whether the given chunk has been downloaded entirely.
// //  */
// // char is_chunk_done() {

// // }

// /*
//  * process_data
//  */
// void process_data() {
//     // Store data in chunk download struct
// }
