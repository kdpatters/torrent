/* 
 * upload.c
 * 
 * Implementations of file upload feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "peer.h"
#include "upload.h"
#include "download.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "chunk.h"



void uploading(data_packet_t pack, config_t config, struct sock_in_addr *addr) {
    upload_t upload;
    memset(&uploads, 0, sizeof(uploads));

    int id = hash_to_id(upload->chunk->hash, config);
    upload->chunk->chunk_id = id;
    //read first line from config->chunkfile to get filename 

    read_chunk(upload, filename, addr, config);
    chunk_hash_t **chunklist;
    parse_Master_chunkfile(config->chunkfile, chunklist);

}


// Function to read a chunk based on its specific ID from a file;
void read_chunk(upload_t upload, char *filename, struct sock_in_addr *addr, bt_config_t *config) {
    // Open chunkfile
    FILE *f;
    f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Could not read file \"%s\".\n", filename);
        exit(1);
    }

    int id = upload->chunk->chunk_id;

    int buf_size = BT_CHUNK_SIZE * id; //chunk-length
    char *buf = malloc(buf_size + 1); 

    
    char *mem_curr = &buf;
    int whence = SEEK_SET;
    while (fseek(f, buf_size, whence) == 0)(
        fread(buf, BT_CHUNK_SIZE, f);
        make_packets(upload, buf, buf_size, addr, config);
    )
}

//making packets after reading chunk
void make_packets(upload_t upload, char* buf, int buf_size, struct sock_in_addr *addr, bt_config_t *config {
    int listsize = buf_size / DATALEN;
    double rem = buf_size % DATALEN;
    data_packet_t *packs[listsize][PACKETLEN]; //array of packets to be created

    data_packet_t pack; //packet to be created
    char* mem_curr = buf; //pointer to start of buffer

        //divide into packets
        for(int i = 0; i < listsize - 1; i++) {
            while(mem_curr <= buf_size) {
                create_data_packet(&packs[i], mem_curr, DATALEN);                
                mem_curr += DATALEN; //increase by how much read and stored in packet
            }
        }
        mem_curr = mem_curr - DATALEN; //for last remaining bytes of chunk in masterfile, reset pointer
        create_data_wrap(&packs[listsize - 1], mem_curr, rem); //create the last data packet
        upload->chunk->packetlist = packs;
        u_int sequence = packs[0]->seq_num;
        send_data(upload, sequence, listsize, addr, config);  //send packetlist
    
}

char check_upload_peer(upload_t upload, bt_config_t config) {
    bt_peer_t *p;
    boolean check = false;
    for (p = config->peers; p != NULL; p = p->next) {  // Iterate through peers

        if (p->id != upload->chunk->peer_id) { 
            continue;
        }
        else {
            check = true;
        }
    }
    return check;
}


send_data (upload_t upload, u_int sequence, int l_size, struct sockaddr_in_addr *addr, bt_config_t *config) {
    
    data_node_t *curr = upload->chunk->packetlist;
    
    for(int i = 0; i < l_size; i++) {
        if(curr->seq_num = sequence) {
            send_pack(curr->data, addr, config);
        }
    }
}

//make wrapper func later
void create_data_wrap(data_packet_t *pack, char *payload, int payload_len) {
        pack->seq_num = SEQ_NUM;
        int sequence = pack->seq_num;
        init_packet(pack, DATA_TYPE, sequence, 0, payload, payload_len);
        SEQ_NUM++;  
}

// Check when the last data packet was received
void check_retry_ack(upload_t *upload, int seq, struct sockaddr_in *dest, bt_config_t *config) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_rec = upload->last_ack_rec;
    
    if((curr_time - last_rec) > TIMEOUT_ACK) {
        send_data(upload, seq, dest, config); //if not ack received
    }

}
data_packet_t create_ack_wrap(data_packet_t ack_pack, char* payload, int payload_len) {
    ack_pack->ack_num = ACK_NUM;
    int ack_n = ACK_NUM;
    init_packet(ack_pack, ACK_TYPE, 0, ack_n, payload, payload_len);
    ACK_NUM++;
}

void send_ack(data_packet_t ack, struct sockaddr_in *dest, bt_config_t *config) {
    data_packet_t created_ack =  create_ack_packet(ack);
    send_pack(created_ack, sizeof(created_ack), dest, config);
}


