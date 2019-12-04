/* upload file for uploading
* upload.c
*/

#include "upload.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "peer.h"
#include "upload.h"




void upload(data_packet_t pack, config_t config, struct sock_in_addr *addr) {
    upload_t upload;
    memset(&uploads, 0, sizeof(uploads));

    int id = hash_to_id(upload->hash, config);
    //read first line from config->chunkfile to get filename 

    read_chunk(upload, id, filename, addr, config);
    chunk_hash_t **chunklist;
    parse_Master_chunkfile(config->chunkfile, chunklist);

}


// Function to read a chunk based on its specific ID from a file;
void read_chunk(upload_t upload, int id, char *filename, sockaddr_in_addr *from, bt_config_t *config) {
    // Open chunkfile
    FILE *f;
    f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Could not read file \"%s\".\n", filename);
        exit(1);
    }

    int buf_size = OFFSET * id; //chunk-length
    char *buf = malloc(buf_size + 1); 
    int listsize = buf_size / DATALEN;
    double rem = buf_size % DATALEN;
    data_packet_t packetlist[listsize]; //array of data packets to send

    data_packet_t *pack;

    char *mem_curr = &buf;

    while (getline(&buf, (size_t *) &buf_size, f) > 0) {

        //divide into packets
        for(int i = 0; i < listsize - 1; i++) {
            while(mem_curr <= buf_size) {
                data_packet_t p = create_data_packet(pack, mem_curr, DATALEN);
                strcpy(packetlist[i]->data, pack->data);
                
            }
            mem_curr += DATALEN; //increase by how much read and stored in packet
            
        }
        mem_curr = mem_curr - DATALEN; //for last remaining bytes of chunk in masterfile, reset pointer
        data_packet_t p = create_data_packet(pack, mem_curr, rem); 
        strcpy(packetlist[listsize - 1]->data, pack->data);//create packet
        upload->packets = &packetlist;
        send_data(upload, listsize, &from, config);  //send packetlist
    }
}



int parse_Master_chunkfile (char *chunkfile, chunk_hash_t **m_chunklist) {

    int n_chunks = 0;
    // Open chunkfile
    FILE *f;
    f = fopen(chunkfile, "r");
    if (f == NULL) {
        fprintf(stderr, "Could not read chunkfile \"%s\".\n", chunkfile);
        exit(1);
    }

    // Store data for individual chunk hash
    int id;
    char hash[CHK_HASHLEN];

    // Zero out the chunklist
    chunk_hash_t *curr = NULL;

    int buf_size = MAX_LINE;
    char *buf = malloc(buf_size + 1);
    int cur_line = 0;

    // Read a line from the chunklist file
  while (getline(&buf, (size_t *) &buf_size, f) > 0) {
    while(cur_line >= 2) { //read 3rd line
        if (strlen(buf) < CHK_HASHLEN + strlen("0 ")) {
        fprintf(stderr, "Line \"%s\" in chunkfile \"%s\" was too short and could not be parsed.\n", 
            buf, chunkfile);
        }
        // Attempt to parse ID and hash from current line in file
        else {
            buf[strcspn(buf, "\n")] = '\0'; 
            int offset = strcspn(buf, " ");

            // Parse ID
            char num[offset];
            strncpy(num, buf + offset - 1, sizeof(num));
            num[offset++] = '\0';
            id = atoi(num);

            // Parse the hash
            offset += strspn(buf + offset, " ");

            if (offset >= buf_size) {
                fprintf(stderr, "Could not parse both id and hash from line \"%s\" in chunkfile \"%s\".\n", 
                buf, chunkfile);
            }
            else {
                // Read the hash from the buffer
                strcpy(hash, buf + offset);
            }

            if (strlen(hash) != CHK_HASHLEN) {
                fprintf(stderr, "Expected hash \"%s\" to be of length %d.\n", hash, CHK_HASHLEN);
            }
            // Found correct number of arguments and of correct lengths, so allocate
            // a new node 
            else {
                chunk_hash_t *new_node;
                new_node = malloc(sizeof(chunk_hash_t));
                assert(new_node != NULL);
                memset(new_node, 0, sizeof(*new_node)); // Zero out the memory
                strncpy(new_node->hash, hash, CHK_HASHLEN);
                new_node->id = id;

                if (curr == NULL) {
                curr = new_node;
                *m_chunklist = curr;
                }
                else {
                curr->next = new_node;
                curr = curr->next;
                }
                n_chunks++;
            }
        }
        cur_line++;
    }
  }
  fclose(f); //close the file
  free(buf); // Since buf may have been reallocated by `getline`, free it
  return n_chunks;
}

//send data packetlist
void send_pack (const void *packet, size_t listsize, struct sockaddr_in *dest, bt_config_t *config) {
    //printf("sending pack\n");
  // Create the socket
  int sockfd = config->sock;
  //if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
  //  perror("Could not create socket.");
  //  exit(1);
  //}
  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));
  memcpy(&myaddr.sin_addr, &dest->sin_addr, sizeof(dest->sin_addr));
  memcpy(&myaddr.sin_port, &dest->sin_port, sizeof(dest->sin_port));
  myaddr.sin_family = AF_INET;

  //send packet to peer requesting it
      sendto(sockfd, packet, listsize, 0, 
        (const struct sockaddr *) &myaddr, sizeof(myaddr));
    //sendto(sockfd, &packetlist[i], packetlist[i].header.packet_len, 0, 
   //     (const struct sockaddr *) &p->addr, sizeof(p->addr));

}

send_data (upload_t upload, size_t listsize, sockaddr_in_addr *addr, bt_config_t *config) {
    for(int i = 0; i < listsize; i++) {
        upload->last_ack_rec = clock();
        send_pack(upload->packets[i], listsize, addr, config); //send all packets
        
        //check if acks received for each packet
        check_rety_get(upload,) //if not retry
    }
}

data_packet_t create_data_packet (data_packet_t data_packet, char* curr, size_t size) {
    header_t *data_header = &data_packet->header;
    data_header->magicnum = MAGICNUM;
    data_header->version = VERSIONNUM;

    data_header->packet_type = DATA_TYPE;
    data_header->header_len = sizeof(data_header);
    data_header->seq_num = SEQ_NUM;
    short packet_len = sizeof(header) + DATALEN;

    if (packet_len > PACKETLEN) {
        perror("Something went wrong: constructed packet is longer than the \
        maximum possible length.");
        exit(1);
    }
    data_header->packet_len = packet_len;

    memcpy(data_packet->data, curr, size);
    SEQ_NUM++;

    return data_packet;
}



void check_retry_get(upload_t *upload, int seq, int listsize, struct sockaddr_in *dest, bt_config_t *config) {

    clock_t curr_time;
    curr_time = clock();
    clock_t last_rec = upload->last_ack_rec;
    if((curr_time - last_rec) > TIMEOUT_ACK) {
        send_pack(upload->packets[seq], listsize, dest, config); //if not ack received
    }

    // Check when the last data packet was received
}


data_packet_t create_ack_packet(data_packet_t ack_pack) {
    header_t *ack_header = &ack_pack->header;
    ack_header->magicnum = MAGICNUM;
    ack_header->version = VERSIONNUM;

    ack_header->packet_type = ACK_TYPE;
    ack_header->header_len = sizeof(ack_header);
    ack_header->ack_num = ACK_NUM;
    short packet_len = sizeof(header) + DATALEN;

    if (packet_len > PACKETLEN) {
        perror("Something went wrong: constructed packet is longer than the \
        maximum possible length.");
        exit(1);
    }
    ack_header->packet_len = packet_len;

    // Initialize packet
    // Set type
    // Call helper

    ACK_NUM++;

    return ack_pack;
}

void send_ack(data_packet_t ack, struct sockaddr_in *dest, bt_config_t *config) {
    data_packet_t created_ack =  create_ack_packet(ack);
    send_pack(created_ack, sizeof(created_ack), dest, config);
}


