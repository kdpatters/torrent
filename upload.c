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
#include "debug.h"

// Making packets after reading chunk
void make_packets(upload_t *upl, char* buf, int buf_size) {

    int rem = buf_size % DATALEN;
    int listsize = buf_size / DATALEN + !!rem;
    DPRINTF(DEBUG_UPLOAD, "make_packets: Splitting chunk across %d packets\n", listsize);
    data_packet_t *packs = malloc(sizeof(*packs) * listsize); // Array of packets to be created
    int offs = 0;

        // Divide into packets
        for (int i = 0; i < listsize; i++) {
            char* mem_curr = buf + offs; 
            pct_data(&packs[i], i, mem_curr,
                (i == (listsize - 1)) ? rem : DATALEN);         
            offs += DATALEN;
        }
        upl->chunk.packetlist = packs;
        upl->chunk.l_size = listsize;
    DPRINTF(DEBUG_UPLOAD, "make_packets: Finished creating %d packets\n", listsize);
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

// Check when the last data packet was received
/* 
void check_retry_ack(upload_t *upl, int seq, struct sockaddr_in *dest, bt_config_t *co) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_rec = upl->last_ack_rec;
    
    if((curr_time - last_rec) > T_OUT_ACK) {
        pct_send(upl, seq, dest, co); // If not ack received
    }

}
*/
