/*
 * peer.c
 *
 * Skeleton for CMPU-375 programming project #2.
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "peer.h"
<<<<<<< HEAD
#include "upload.h"
=======
#include "download.h"
>>>>>>> df9a181f4f5640dc6c126adb24ea3a4bbd3471a0

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 3; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  bt_parse_command_line(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif
  
  peer_run(&config);
  return 0;
}
//to convert chunk hashes from string format to actual hexadecimal bytes 
void hashstr_to_bytes(char instr[CHK_HASHLEN], char outstr[CHK_HASH_BYTES]) {
  for (int i = 0; i < CHK_HASH_BYTES; i++)
    sscanf(&instr[i * 2], "%2hhx", &outstr[i]);
}
//hash bytes back to string 
void bytes_to_hashstr(char instr[CHK_HASH_BYTES], char outstr[CHK_HASHLEN]) {
  for (int i = 0; i < CHK_HASH_BYTES; i++)
    sprintf(&outstr[i * 2], "%02hhx", instr[i]);
}

<<<<<<< HEAD
/* Creates a singly linked list, storing hash and ID */
int parse_chunkfile(char *chunkfile, chunk_hash_t **chunklist) {
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
  // Read a line from the chunklist file
  while (getline(&buf, (size_t *) &buf_size, f) > 0) {
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
          *chunklist = curr;
        }
        else {
          curr->next = new_node;
          curr = curr->next;
        }
        n_chunks++;
      }
    }
  }
  free(buf); // Since buf may have been reallocated by `getline`, free it
  return n_chunks;
}

=======
>>>>>>> 6ee487ea7c13d87368f3ed3335c69a6e1972c4f3
void create_pack_helper(data_packet_t *packet, header_t *header, int num_chunks, 
  char chunks[][CHK_HASHLEN + 1]) {

  // Calculate the packet length
  // Use num_chunks + 1 to include padding and chunk count at start
  int packet_len = sizeof(header) + CHK_COUNT + PADDING \
    + num_chunks * CHK_HASH_BYTES;
  if (packet_len > PACKETLEN) {
    perror("Something went wrong: constructed packet is longer than the \
    maximum possible length.");
    exit(1);
  }
  header->packet_len = packet_len;

  // Create the payload
  packet->data[0] = num_chunks;
  // Zero out the padding
  memset(&packet->data[1], 0, PADDING);

  // Get start of chunks in payload
  
  int inc = CHK_COUNT + PADDING;
  for (int i = 0; i < num_chunks; i++) {
    char chunk_bytes[CHK_HASH_BYTES];
    hashstr_to_bytes(chunks[i], chunk_bytes);
    memcpy(packet->data + inc, chunk_bytes, CHK_HASH_BYTES);
    inc += CHK_HASH_BYTES;
    if (inc >= DATALEN - CHK_HASH_BYTES) {
      fprintf(stderr, "There are too many chunks to fit in the payload for packet.");
      exit(1);
    }
  }
}

void create_iHave_packet(data_packet_t *hav_packet, int hav_num_chunks, char chunks[][CHK_HASHLEN + 1]) {
   memset(hav_packet, 0, sizeof(*hav_packet));

   //Create IHAVE header
   header_t *hav_header = &hav_packet->header;
   hav_header->magicnum = MAGICNUM;
   hav_header->version = VERSIONNUM;
   hav_header->header_len = sizeof(hav_header);
   hav_header->packet_type = IHAVE_TYPE;

   create_pack_helper(hav_packet, hav_header, hav_num_chunks, chunks);
} 

void iHave_check(data_packet_t *packet, bt_config_t *config, struct sockaddr_in *from) {

    chunk_hash_t *chunklist = NULL;
    int n_chunks = parse_chunkfile(config->has_chunk_file, &chunklist); //num of chunks in has_chunk_file

    char *curr_pack_ch = packet->data; //pointer to start of packet chunk
    int num_chunks = *curr_pack_ch; //dereference to get stored number of chunks in packet

    curr_pack_ch += PADDING + CHK_COUNT; //get to the start of chunk of hashes
    char matched[MAX_CHK_HASHES][CHK_HASHLEN + 1];
    int m_point = 0;

    data_packet_t newpack; 
    
    for(int i = 0; i < num_chunks; i++) { //for each chunk in packet
      chunk_hash_t *curr_ch = chunklist;
      char compare[CHK_HASHLEN + 1]; 
      memset(compare, 0, sizeof(compare));
      bytes_to_hashstr(curr_pack_ch + i * CHK_HASH_BYTES, compare);
      for(int j = 0; j < n_chunks; j++) { //each chunk in has_chunk_file
        if (strncmp(compare, curr_ch->hash, CHK_HASHLEN) == 0) {
          strncpy(matched[m_point++], compare, sizeof(compare));
          if (m_point >= sizeof(matched)) {
            create_iHave_packet(&newpack, m_point, matched); //create iHave packet
            send_pack(&newpack, from, config); //send the packet to peer requesting
            memset(matched, 0, sizeof(matched)); //clearing array of strings
            m_point = 0; //set mno. of matches to 0 for the next packet
          }
          break;
        }
        curr_ch = curr_ch->next; // Move on to next chunk hash        
      }
  }
  // Send packet if there is still data left to send
  if (m_point > 0) {
    create_iHave_packet(&newpack, m_point, matched); //create iHave packet
    send_pack(&newpack, from, config); //send the packet to peer requesting
  }
}

upload(data_packet_t pack, config_t config, struct sock_in_addr *addr) {

 }

void process_inbound_udp(int sock, bt_config_t *config) {
  data_packet_t *pack;
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[PACKETLEN];

  fromlen = sizeof(from);
  spiffy_recvfrom(sock, buf, PACKETLEN, 0, (struct sockaddr *) &from, &fromlen);

/*
  printf("PROCESS_INBOUND_UDP SKELETON -- replace!\n"
	 "Incoming message from %s:%d\n%s\n\n", 
	 inet_ntoa(from.sin_addr),
	 ntohs(from.sin_port),
	 buf); */ 
<<<<<<< HEAD
   pack = (data_packet_t *)buf;
   // TODO: use a switch instead of an IF
   if (pack->header.packet_type == WHOHAS_TYPE) {
     iHave_check(pack, config, &from);
   }

   if(pack->header.packet_type == GET_TYPE) {
     upload(pack, config, &from);
   }                                             
=======
  pack = (data_packet_t *)buf;
  switch (pack->header.packet_type) {
    case WHOHAS_TYPE:
      iHave_check(pack, config, &from);
      break;
    case IHAVE_TYPE:
      printf("Received IHAVE packet.\n");
      process_ihave(pack, config, &from);
      break;
    case GET_TYPE:
      printf("Received GET packet.\n");
      break;
    case DATA_TYPE:
      printf("Received DATA packet.\n");
      break;
    case ACK_TYPE:
      printf("Received ACK packet.\n");
      break;
    case DENIED_TYPE:
      printf("Received DENIED packet.\n");
      break;
    default:
      printf("Packet type %d not understood.\n", pack->header.packet_type);
  }
>>>>>>> df9a181f4f5640dc6c126adb24ea3a4bbd3471a0
} 

void create_whohas_packet(data_packet_t *packet, int num_chunks, 
  char chunks[][CHK_HASHLEN + 1]) {

  memset(packet, 0, sizeof(*packet));

  // Create the WHOHAS header
  header_t *header = &packet->header;
  header->magicnum = MAGICNUM;
  header->version = VERSIONNUM;
  header->header_len = sizeof(header); 
  header->packet_type = WHOHAS_TYPE;
  
  create_pack_helper(packet, header, num_chunks, chunks);
}

void process_get(char *chunkfile, char *outputfile, bt_config_t *config) {
  int *ids;
  char *hashes[CHK_HASHLEN];
  int num_chunks = parse_hashes_ids(chunkfile, hashes, ids);
   
  // Create an array of packets to store the chunk hashes
  int list_size = (num_chunks / MAX_CHK_HASHES) + \
    ((num_chunks % MAX_CHK_HASHES) != 0);
  data_packet_t packetlist[list_size];
  memset(&packetlist, 0, sizeof(packetlist)); // Zero out packetlist memory

  // Package the chunks into packets
  for (int i = 0; i < list_size; i++) {
    create_whohas_packet(&packetlist[i], 
      MIN(MAX_CHK_HASHES, num_chunks), 
      &hashes[i * MAX_CHK_HASHES]);
    num_chunks -= MIN(MAX_CHK_HASHES, num_chunks);
  }

  // Iterate through known peers
  bt_peer_t *p;
  for (p = config->peers; p != NULL; p = p->next) {
    if (p->id != config->identity) {
        // Iterate through packets, sending each to given peer
        for (int i = 0; i < list_size; i++)
          send_pack(&packetlist[i], &p->addr, config); 
    }
  }
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];
  bt_config_t *config = (bt_config_t *) cbdata;

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));
  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      process_get(chunkf, outf, config);
    }
  }
}

void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;
  
  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }
    
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }
   
  config->sock = sock; // Set socket 
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
	process_inbound_udp(sock, config);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
	process_user_input(STDIN_FILENO, userbuf, handle_user_input, config);
      }
    }
  }
}
