/* 
 * upload.c
 * 
 * Implementations of file upload feature for torrent client.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h>
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
    char *f_name = parse_Master_chunkfile(config->chunkfile, chunklist);
    read_chunk(upload, f_name, addr, config);

}


// Function to read a chunk based on its specific ID from a file;
void read_chunk(upload_t upload, char *filename, struct sock_in_addr *address, bt_config_t *confg) {
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
        make_packets(upload, buf, buf_size, address, confg);
    )
}

//making packets after reading chunk
void make_packets(upload_t upload, char* buf, int buf_size, struct sock_in_addr *add, bt_config_t *conf {
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
        send_data(upload, sequence, listsize, add, conf);  //send packetlist
    
}

char check_upload_peer(upload_t upload, bt_config_t cfg) {
    bt_peer_t *p;
    for (p = cfg->peers; p != NULL; p = p->next) {  // Iterate through peers

        if (p->id != upload->chunk->peer_id) { 
            continue;
        }
        else {
            return 1;
        }
    }
    return -1;
}


send_data (upload_t upload, u_int sequence, int l_size, struct sockaddr_in_addr *addr_, bt_config_t *cf) {
    
    data_node_t *curr = upload->chunk->packetlist;
    
    for(int i = 0; i < l_size; i++) {
        if(curr->seq_num = sequence) {
            send_pack(curr->data, addr_, cf);
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
void check_retry_ack(upload_t *upload, int seq, struct sockaddr_in *dest, bt_config_t *co) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_rec = upload->last_ack_rec;
    
    if((curr_time - last_rec) > TIMEOUT_ACK) {
        send_data(upload, seq, dest, co); //if not ack received
    }

}
data_packet_t create_ack_wrap(data_packet_t ack_pack, char* payld, int payld_len) {
    ack_pack->ack_num = ACK_NUM;
    int ack_n = ACK_NUM;
    init_packet(ack_pack, ACK_TYPE, 0, ack_n, payld, payld_len);
    ACK_NUM++;
}

void send_ack(data_packet_t ack, struct sockaddr_in *desti, bt_config_t *cg) {
    data_packet_t created_ack =  create_ack_packet(ack);
    send_pack(created_ack, sizeof(created_ack), desti, cg);
}
// /* 
//  * download.c
//  * 
//  * Implementations of file download feature for torrent client.
//  */

// #include <string.h>
// #include <stdlib.h>
// #include <time.h>
// #include "peer.h"
// #include "upload.h"
// #include "download.h"
// #include "bt_parse.h"
// #include "input_buffer.h"




// void uploading(data_packet_t pack, config_t config, struct sock_in_addr *addr) {
//     upload_t upload;
//     memset(&uploads, 0, sizeof(uploads));

//     int id = hash_to_id(upload->chunk->hash, config);
//     upload->chunk->chunk_id = id;
//     //read first line from config->chunkfile to get filename 

//     read_chunk(upload, filename, addr, config);
//     chunk_hash_t **chunklist;
//     parse_Master_chunkfile(config->chunkfile, chunklist);

// }


// // Function to read a chunk based on its specific ID from a file;
// void read_chunk(upload_t upload, char *filename, struct sock_in_addr *addr, bt_config_t *config) {
//     // Open chunkfile
//     FILE *f;
//     f = fopen(filename, "r");
//     if (f == NULL) {
//         fprintf(stderr, "Could not read file \"%s\".\n", filename);
//         exit(1);
//     }

//     int id = upload->chunk->chunk_id;

//     int buf_size = OFFSET * id; //chunk-length
//     char *buf = malloc(buf_size + 1); 
//     int listsize = buf_size / DATALEN;
//     double rem = buf_size % DATALEN;
//     data_node_t nodeslist; //linked list of nodes to be created

//     data_packet_t *pack;
//     data_packet_t *head;
//     data_node_t *curr_node = NULL;

//     char *mem_curr = &buf;

//     while (getline(&buf, (size_t *) &buf_size, f) > 0) {

//         //divide into packets
//         for(int i = 0; i < listsize - 1; i++) {
//             while(mem_curr <= buf_size) {
//                 data_packet_t p = create_data_packet(pack, mem_curr, DATALEN);
//                 data_node_t *node;
//                 memset(node, 0, sizeof(node));
//                 strcpy(node->data, p->data);
//                 node->prev = curr_node;
//                 curr_node = node; //make curr pointer to point 
//                 node = node->next; //move to next to create further new nodes
                
//                 mem_curr += DATALEN; //increase by how much read and stored in packet
//             }
//         }
//         mem_curr = mem_curr - DATALEN; //for last remaining bytes of chunk in masterfile, reset pointer
//         data_packet_t p = create_data_packet(pack, mem_curr, rem); //create the last data packet
//         data_node_t *node;
//         memset(node, 0, sizeof(node));
//         strcpy(node->data, p->data); //fill in data in last node
//         node->prev = curr_node;
//         upload->chunk->piece = head; 
//         send_data(upload, addr, config);  //send packetlist
//     }
// }

// boolean check_upload_peer(upload_t upload, bt_config_t config) {
//     bt_peer_t *p;
//     boolean check = false;
//     for (p = config->peers; p != NULL; p = p->next) {  // Iterate through peers

//         if (p->id != upload->chunk->peer_id) { 
//             continue;
//         }
//         else {
//             check = true;
//         }
//     }
//     return check;
// }


// int parse_Master_chunkfile (char *chunkfile, chunk_hash_t **m_chunklist) {

//     int n_chunks = 0;
//     // Open chunkfile
//     FILE *f;
//     f = fopen(chunkfile, "r");
//     if (f == NULL) {
//         fprintf(stderr, "Could not read chunkfile \"%s\".\n", chunkfile);
//         exit(1);
//     }

//     // Store data for individual chunk hash
//     int id;
//     char hash[CHK_HASHLEN];

//     // Zero out the chunklist
//     chunk_hash_t *curr = NULL;

//     int buf_size = MAX_LINE;
//     char *buf = malloc(buf_size + 1);
//     int cur_line = 0;

//     // Read a line from the chunklist file
//   while (getline(&buf, (size_t *) &buf_size, f) > 0) {
//     while(cur_line >= 2) { //read 3rd line
//         if (strlen(buf) < CHK_HASHLEN + strlen("0 ")) {
//         fprintf(stderr, "Line \"%s\" in chunkfile \"%s\" was too short and could not be parsed.\n", 
//             buf, chunkfile);
//         }
//         // Attempt to parse ID and hash from current line in file
//         else {
//             buf[strcspn(buf, "\n")] = '\0'; 
//             int offset = strcspn(buf, " ");

//             // Parse ID
//             char num[offset];
//             strncpy(num, buf + offset - 1, sizeof(num));
//             num[offset++] = '\0';
//             id = atoi(num);

//             // Parse the hash
//             offset += strspn(buf + offset, " ");

//             if (offset >= buf_size) {
//                 fprintf(stderr, "Could not parse both id and hash from line \"%s\" in chunkfile \"%s\".\n", 
//                 buf, chunkfile);
//             }
//             else {
//                 // Read the hash from the buffer
//                 strcpy(hash, buf + offset);
//             }

//             if (strlen(hash) != CHK_HASHLEN) {
//                 fprintf(stderr, "Expected hash \"%s\" to be of length %d.\n", hash, CHK_HASHLEN);
//             }
//             // Found correct number of arguments and of correct lengths, so allocate
//             // a new node 
//             else {
//                 chunk_hash_t *new_node;
//                 new_node = malloc(sizeof(chunk_hash_t));
//                 assert(new_node != NULL);
//                 memset(new_node, 0, sizeof(*new_node)); // Zero out the memory
//                 strncpy(new_node->hash, hash, CHK_HASHLEN);
//                 new_node->id = id;

//                 if (curr == NULL) {
//                 curr = new_node;
//                 *m_chunklist = curr;
//                 }
//                 else {
//                 curr->next = new_node;
//                 curr = curr->next;
//                 }
//                 n_chunks++;
//             }
//         }
//         cur_line++;
//     }
//   }
//   fclose(f); //close the file
//   free(buf); // Since buf may have been reallocated by `getline`, free it
//   return n_chunks;
// }


// send_data (upload_t upload, int sequence, struct sockaddr_in_addr *addr, bt_config_t *config) {
    
//     data_node_t sending_node = upload->chunk->piece;
//     data_node_t *curr;

//     for(curr = sending_node); curr != NULL, curr = curr->next;) {
//         if(curr->seq_num = sequence) {
//             send_pack(curr->data, addr, config);
//         }
//     }
// }

// data_packet_t create_data_packet (data_packet_t data_packet, char* curr, size_t size) {
//     header_t *data_header = &data_packet->header;
//     data_header->magicnum = MAGICNUM;
//     data_header->version = VERSIONNUM;

//     data_header->packet_type = DATA_TYPE;
//     data_header->header_len = sizeof(data_header);
//     data_header->seq_num = SEQ_NUM;
//     short packet_len = sizeof(header) + DATALEN;

//     if (packet_len > PACKETLEN) {
//         perror("Something went wrong: constructed packet is longer than the
//         maximum possible length.");
//         exit(1);
//     }
//     data_header->packet_len = packet_len;

//     memcpy(data_packet->data, curr, size);
//     SEQ_NUM++;

//     return data_packet;
// }

// // Check when the last data packet was received
// void check_retry_get(upload_t *upload, int seq, struct sockaddr_in *dest, bt_config_t *config) {

//     clock_t curr_time;
//     curr_time = clock();
//     clock_t last_rec = upload->last_ack_rec;
    
//     if((curr_time - last_rec) > TIMEOUT_ACK) {
//         send_data(upload, seq, dest, config); //if not ack received
//     }

// }


// data_packet_t create_ack_packet(data_packet_t ack_pack) {
//     header_t *ack_header = &ack_pack->header;
//     ack_header->magicnum = MAGICNUM;
//     ack_header->version = VERSIONNUM;

//     ack_header->packet_type = ACK_TYPE;
//     ack_header->header_len = sizeof(ack_header);
//     ack_header->ack_num = ACK_NUM;
//     short packet_len = sizeof(header) + DATALEN;

//     if (packet_len > PACKETLEN) {
//         perror("Something went wrong: constructed packet is longer than the
//         maximum possible length.");
//         exit(1);
//     }
//     ack_header->packet_len = packet_len;

//     ACK_NUM++;

//     return ack_pack;
// }

// void send_ack(data_packet_t ack, struct sockaddr_in *dest, bt_config_t *config) {
//     data_packet_t created_ack =  create_ack_packet(ack);
//     send_pack(created_ack, sizeof(created_ack), dest, config);
// }
