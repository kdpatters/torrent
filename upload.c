/* 
 * upload.c
 * 
 * Implementations of file upload feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "chunk.h"
#include "packet.h"
#include "bt_parse.h"
#include "upload.h"
#include "download.h"
#include "input_buffer.h"


// Making packets after reading chunk
void make_packets(upload_t *upl, char* buf, int buf_size) {

    int rem = buf_size % DATALEN;
    int listsize = buf_size / DATALEN + !!rem;
    data_packet_t *packs = malloc(sizeof(*packs) * listsize); // Array of packets to be created
    int offs = 0;

        // Divide into packets
        for(int i = 0; i < listsize; i++) {
            char* mem_curr = buf + offs; 
            pct_data(&packs[i], i, mem_curr, (i == listsize - 1) ? rem : DATALEN);                
            offs += DATALEN;
        }
        upl->chunk.packetlist = packs;
        upl->chunk.l_size = listsize;
        int *received_acks =  malloc(sizeof(received_acks) * listsize); // Initiaize and allocate array for future acks to be received
        upl->recv = received_acks;
}

// Function to read a chunk based on its specific ID from a file;
void read_chunk(upload_t *upl, char *filename, char *buf) {
    // Open chunkfile
    FILE *f;
    f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Could not read file \"%s\".\n", filename);
        exit(1);
    }

    int id = upl->chunk.chunk_id;
    int position = id * BT_CHUNK_SIZE;
    int whence = SEEK_SET; // Offset bytes set to start of file

    if (fseek(f, position, whence)) {
        fprintf(stderr, "Could not read chunk %d\n", id);
        exit(1);
    }
    // Position pointer to where chunk located indicated by buf_size
        fread(buf, BT_CHUNK_SIZE, 1, f); // Read a chunk
}

/* 
char check_upload_peer(upload_t *upl, bt_config_t *cfg) {
    bt_peer_t *p;
    for (p = cfg->peers; p != NULL; p = p->next) {  // Iterate through peers

        if (p->id != upl->chunk->peer_id) { 
            continue;
        }
        else {
            return 1;
        }
    }
    return -1;
}

*/


// Check if ack receieved is a duplicate that was rec before for another packet
char check_dup_ack(upload_t *upl, int sequence, data_packet *ack) {
    int n_acks = upl->rec[sequence];
    if(n_acks > 0)  // If current ack was already received before
        n_acks++; // It is a duplicate
    
    return n_acks; // Otherwise not a dup
}
 
// Check when the last data packet was received
void check_retry_upl(upload_t *upl, int seq, struct sockaddr_in *dest, bt_config_t *co) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_ack = upl->last_ack_rec;
    
    if(((curr_time - last_ack) > T_OUT_ACK) || (check_dup_ack(upl, seq) > 0) { // Time-out occurs or dup ack rec
        pct_send(upl, seq, dest, co); // If not ack received

    }

}

