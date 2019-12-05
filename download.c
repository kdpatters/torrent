/* 
 * download.c
 * 
 * Implementations of file download feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "download.h"
#include "peer.h"

#define INVALID_FIELD 0

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
//             exit(-1);
//         }
//     }
//     return i;
// }

/*
 * create_get_packet
 */
void create_get_packet(data_packet_t *pack, char hash[CHK_HASH_BYTES]) {
    init_packet(pack, GET_TYPE, INVALID_FIELD, INVALID_FIELD, hash, 
        CHK_HASH_BYTES);
}

/*
 * send_get 
 */
void send_get(struct sockaddr_in *dest, char hash[CHK_HASH_BYTES], 
    bt_config_t *config) {

    data_packet_t pack;
    create_get_packet(&pack, hash); // Create the GET packet
    send_pack(&pack, dest, config); // Send the GET packet
}

// void begin_download() {
//     // DO SOMETHING...
// }

/* 
 * init_download
 * 
 * Initialize the struct to keep track of the chunks and connected peers for the
 * specific download.
 */
void init_download(char hash[][CHK_HASH_BYTES], int n_chunks, 
    download_t *download, bt_config_t *config) {
    
    // Get a pointer to the first empty download
    download->ihave_recv = malloc(sizeof(ihave_list_t) * n_chunks);
    download->n_chunks = n_chunks;
    for (int i = 0; i < n_chunks; i++) {
        // Get and store hash ID
        download->ihave_recv[i].chunk_id = hash2id(hash[i], config);
    }

    // Officially start the download
    download->state = DLOAD_WAIT_IHAVE;
    download->time_started = clock();
}

void process_ihave(data_packet_t *packet, bt_config_t *config, 
    struct sockaddr_in *from) {

    int inc = CHK_HASH_BYTES; // Amount to increment offset by
    // Store the length of the packet
    int pack_len = packet->header.packet_len;
    int max_offset = pack_len - inc;

    // Parse the number of chunks in the file
    int n_chunks;
    memcpy(&n_chunks, packet->data, CHK_COUNT);

    int offset = CHK_COUNT + PADDING;
    char hash[inc]; // Temporary variable to store hash
    for (int i = 0; i < n_chunks; i++) {
        // Read and store the next hash
        memcpy(hash, packet->data + offset, inc);

        // THIS NEEDS A LOT OF WORK; TODO
        // Try to match returned hash to requested one and store peer identity
        //int id = hash2id(hash, config);
        //for (int j = 0; j < downloads[0].n_chunks; j++) {
        //    if (downloads[0].ihave_recv[j].chunk_id == id) {
                // Add peer to peer list
                //add_peer(downloads[0].ihave_recv[j].peers, SOMETHING);
        //    }
        //}

        offset += inc;
        if (offset > max_offset) {
            fprintf(stderr, "Parsed %d chunk hashes from IHAVE but expected %d.",
            i + 1, n_chunks);
        }
    }
}

/*
 * check_retry_get
 * 
 * Check timeout for when last data packet received and last get sent have been 
 * exceeded.  If less than the maximum number of gets have been sent, then 
 * resend another get.  Otherwise, 
 */
void check_retry_get(chunk_download_t chunk_dload) { // Need to pass in downloads as an argument
    clock_t now = clock();
    
    /* 
     * Check if timeouts for when last get was sent and last data packet was 
     * received have been exceeded.
     */
    if ((now - chunk_dload.last_get_sent > GET_WINDOW) &&
        (now - chunk_dload.last_data_recv > DATA_WINDOW)) {
        if (chunk_dload.n_tries_get < MAX_RETRIES_GET) {
            // TODO
            //send_get(...);
        }
        else {
            /* Try to finish by downloading the chunk from a different peer. */
            // TODO
        }
    }
    // Check when the last data packet was received
    // Either attempt to verify chunk, stop download entirely (if max retries exceeded), or send another GET
}


/*
 * create_ack_packet
 * 
 * Wrapper for init_packet function to simplify ACK packet creation.
 */
void create_ack_packet(data_packet_t *pack, int ack_num) {
    init_packet(pack, ACK_TYPE, INVALID_FIELD, ack_num, NULL, 0);
}

/*
 * send_ack
 * 
 * Creates and sends an ACK packet with a specific ACK number.
 */
void send_ack(struct sockaddr_in *dest, int ack_num, bt_config_t *config) {
    data_packet_t pack;
    create_ack_packet(&pack, ack_num);
    send_pack(&pack, dest, config);
}

/*
 * verify_chunk
 */
void verify_chunk() {
    // Generate hash for chunk data
    // Compare generated hash to current hash
}

/*
 * is_chunk_done
 * 
 * Returns a boolean for whether the given chunk has been downloaded entirely.
//  */
// char is_chunk_done() {

// }

/*
 * process_data
 */
void process_data() {
    // Store data in chunk download struct
}