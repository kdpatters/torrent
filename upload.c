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

void uploading(char *f_name, data_packet_t *pack, int sock, struct sockaddr_in *addr) {
    upload_t upl;

    memset(&upl, 0, sizeof(upl));

    int buf_size = BT_CHUNK_SIZE; // Chunk-length
    char buf[buf_size]; 

    int id = hash_to_id(upl->chunk->hash, conf);
    upl->chunk->chunk_id = id;
    read_chunk(upl, f_name, buf); // Reads chunk from Data file in Master chunkfile
    make_packets(upl, buf, buf_size); // Create packets from the chunk
    pct_send(upl.chunk->packetlist[0], addr, sock);  // Send first packet
}

// Wrapper func for sending packets
void create_data_wrap(data_packet_t *pack, char *payload, int payload_len) {
        pack->header.seq_num = SEQ_NUMB;
        int sequence = pack->header.seq_num;
        pct_init(pack, DATA_TYPE, sequence, 0, payload, payload_len);
        SEQ_NUMB++;  
}

// Making packets after reading chunk
void make_packets(upload_t *upl, char* buf, int buf_size, struct sockaddr_in *add, bt_config_t *conf) {

    int rem = buf_size % DATALEN;
    int listsize = buf_size / DATALEN + !!rem;
    data_packet_t *packs = malloc(sizeof(*packs) * listsize); // Array of packets to be created

    data_packet_t pack; // Packet to be created
    int offs = 0;

        // Divide into packets
        for(int i = 0; i < listsize - 1; i++) {
            while(offs < buf_size + DATALEN) {
                char* mem_curr = buf + offs; 
                create_data_wrap(packs[i], mem_curr, DATALEN);                
                offs += DATALEN;
            }
        }
        // For last remaining bytes of chunk in masterfile
        char *mem_curr = buf_size + offs; // Position memory pointer
        create_data_wrap(packs[listsize - 1], mem_curr, rem); //create the last data packet
        upl->chunk->packetlist = packs;
        upl->chunk->l_size = listsize;
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

    int id = upl->chunk->chunk_id;
    int position = id * BT_CHUNK_SIZE;
    char *mem_curr = buf; // Pointer to buffer
    int whence = SEEK_SET; // Offset bytes set to start of file

    if (fseek(f, position, whence)) {
        fprintf(stderr, "Could not read chunk %d\n", id);
        exit(1);
    }
    // Position pointer to where chunk located indicated by buf_size
        fread(buf, BT_CHUNK_SIZE, 1, f); // Read a chunk
}


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

// Check when the last data packet was received
void check_retry_ack(upload_t *upl, int seq, struct sockaddr_in *dest, bt_config_t *co) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_rec = upl->last_ack_rec;
    
    if((curr_time - last_rec) > T_OUT_ACK) {
        send_data(upl, seq, dest, co); // If not ack received
    }

}
