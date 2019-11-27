/*
 * peer.c
 *
 * Skeleton for CMPU-375 programming project #2.
 *
 */

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  char *chunks[]) {

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
  int inc = CHK_COUNT + PADDING;
  for (int i = 0; i < num_chunks; i++) {
    strncpy(packet->data + inc, chunks[i], CHK_HASHLEN);
    inc += CHK_HASHLEN;
    if (inc >= DATALEN) {
      perror("There are too many chunks to fit in the payload for WHOHAS packet.");
      exit(1);
    }
  }
}



void process_get(char *chunkfile, char *outputfile) {
  printf("PROCESS GET SKELETON CODE CALLED.  Fill me in!  (%s, %s)\n", 
	chunkfile, outputfile);
  // Send request to each client on the network to request the file chunks
  // Iterate through nodes and request chunk from each
  //servaddr.sin_family = AF_INET; 
  //servaddr.sin_port = htons(PORT); 
  //servaddr.sin_addr.s_addr = INADDR_ANY; 

  // Function to divide chunk list into correct number of packets
  // Two loops to iterate through clients and chunk list

  //create_whohas_packet(data_packet_t *packet, int num_chunks, 
  //  char *chunks[]);
  //sentto(sockfd, MSG, MSG_LEN, MSG_CONFIRM, (const struct sockaddr *) &);
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      process_get(chunkf, outf);
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
	process_user_input(STDIN_FILENO, userbuf, handle_user_input,
			   "Currently unused");
      }
    }
  }
}
