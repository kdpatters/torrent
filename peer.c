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

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
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


void process_inbound_udp(int sock) {
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[PACKETLEN];

  fromlen = sizeof(from);
  spiffy_recvfrom(sock, buf, PACKETLEN, 0, (struct sockaddr *) &from, &fromlen);

  printf("PROCESS_INBOUND_UDP SKELETON -- replace!\n"
	 "Incoming message from %s:%d\n%s\n\n", 
	 inet_ntoa(from.sin_addr),
	 ntohs(from.sin_port),
	 buf);
}

void create_whohas_packet(data_packet_t *packet, int num_chunks, 
  chunk_hash_t *chunklist) {

  memset(packet, 0, sizeof(*packet));

  // Create the WHOHAS header
  header_t *header = &packet->header;
  header->magicnum = MAGICNUM;
  header->version = VERSIONNUM;
  header->packet_type = WHOHAS_TYPE;
  header->header_len = sizeof(header);

  // Calculate the packet length
  // Use num_chunks + 1 to include padding and chunk count at start
  int packet_len = sizeof(header) + CHK_COUNT + PADDING \
    + num_chunks * CHK_HASHLEN;
  if (packet_len > PACKETLEN) {
    perror("Something went wrong: constructed WHOHAS packet is longer than the \
maximum possible length.");
    exit(1);
  }
  header->packet_len = packet_len;

  // Create the payload
  packet->data[0] = num_chunks;
  // Get start of chunks in payload
  chunk_hash_t *curr_chunk = chunklist;
  int inc = CHK_COUNT + PADDING;
  for (int i = 0; i < num_chunks; i++) {
    strncpy(packet->data + inc, curr_chunk->hash, CHK_HASHLEN);
    curr_chunk = curr_chunk->next; // Move on to next chunk hash
    inc += CHK_HASHLEN;
    if (inc >= DATALEN) {
      fprintf(stderr, "There are too many chunks to fit in the payload for WHOHAS packet.");
      exit(1);
    }
  }
}

/* Creates a singly linked list, storing hash and ID */
int parse_chunkfile(char *chunkfile, chunk_hash_t *chunklist) {
  int n_chunks = 0;

  // Open chunkfile
  FILE *f;
  f = fopen(chunkfile, "r");
  if (f == NULL) {
    fprintf(stderr, "Could not read chunkfile \"%s\".", chunkfile);
    exit(1);
  }

  // Store data for individual chunk has
  int *id = NULL;
  char hash[CHK_HASHLEN];

  // Zero out the chunklist
  memset(chunklist, 0, sizeof(*chunklist));
  chunk_hash_t *curr = chunklist;

  char *buf = malloc(MAX_LINE);
  // Read a line from the chunklist file
  while (getline(&buf, (size_t *) sizeof(buf), f) > 0) {
    if (strlen(buf) < CHK_HASHLEN + strlen("0 ")) {
      fprintf(stderr, "Line \"%s\" in chunkfile \"%s\" was too short and could not be parsed.", 
        buf, chunkfile);
    }
    // Attempt to parse ID and hash from current line in file
    else {
      if (sscanf(buf, "%d %s\n", id, hash) < 2) {
        fprintf(stderr, "Could not parse both id and hash from line \"%s\" in chunkfile \"%s\".", 
          buf, chunkfile);
      }
      else if (strlen(hash) != CHK_HASHLEN) {
        fprintf(stderr, "Expect hash \"%s\" to be of length %d.", hash, CHK_HASHLEN);
      }
      // Found correct number of arguments and of correct lengths, so allocate
      // a new node 
      else {
        curr = (chunk_hash_t *) malloc(sizeof(chunk_hash_t));
        assert(curr != NULL);
        memset(curr, 0, sizeof(*curr)); // Zero out the memory
        strcpy(curr->hash, hash);
        curr->id = *id;
        curr = curr->next;
        n_chunks++;
      }
    }
  }
  free(buf); // Since buf may have been reallocated by `getline`, free it
  return n_chunks;
}

void process_get(char *chunkfile, char *outputfile, bt_config_t *config) {
  // Parse the chunkfile
  chunk_hash_t *chunklist = NULL;
  int num_chunks = parse_chunkfile(chunkfile, chunklist);

  // Create an array of packets to store the chunk hashes
  data_packet_t *packetlist[(int) ceill((double) num_chunks / 
    (double) MAX_CHK_HASHES)];
  memset(packetlist, 0, sizeof(*packetlist)); // Zero out packetlist memory

  // Package the chunks into packets
  for (int i = 0; i < sizeof(packetlist); i++) {
    create_whohas_packet(packetlist[i], 
      MIN(MAX_CHK_HASHES, num_chunks), 
      &chunklist[i * MAX_CHK_HASHES]);
  }

  // Create the socket
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("Could not create socket.");
    exit(1);
  }
  
  // Iterate through known peers
  bt_peer_t *p;
  for (p = config->peers; p != NULL; p = p->next) {
    // Iterate through packets, sending each to given peer
    for (int i = 0; i < sizeof(packetlist); i++) {
      sendto(sockfd, packetlist[i], packetlist[i]->header.packet_len, 0, 
        (const struct sockaddr *) &p->addr, sizeof(p->addr));
    }
  }
  //
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
  
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
	process_inbound_udp(sock);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
	process_user_input(STDIN_FILENO, userbuf, handle_user_input, config);
      }
    }
  }
}
