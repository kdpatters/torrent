/*
 * peer.c
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
#include "bt_parse.h"
#include "chunk.h"
#include "debug.h"
#include "spiffy.h"
#include "input_buffer.h"
#include "packet.h"
#include "upload.h"
#include "download.h"
#include "server_state.h"
#include "test_peer.h"

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 3; // your group number here
  strcpy(config.chunk_file, "example/C.masterchunks");
  strcpy(config.has_chunk_file, "example/A.chunks");
  test_peer();
  return 0;
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

/*
 * id_in_ids
 *
 * Returns a boolean for whether a given integer `id` is in an array
 * of integers `ids` of length `ids_len`.
 */
int id_in_ids(int id, int *ids, int ids_len) {
  for (int i = 0; i < ids_len; i++) {
    if (id == ids[i])
      return 1;
  }
  return 0;  
}

/* Returns the id for a peer based on its IP address and port. */
int peer_addr_to_id(struct sockaddr_in peer, server_state_t *state) {
  for (bt_peer_t *p = state->config->peers; p != NULL; p = p->next) {
    if ((p->addr.sin_port = peer.sin_port) && 
        (p->addr.sin_addr.s_addr = peer.sin_addr.s_addr)) {
          return p->id;
    }
  }
fprintf(stderr, "Could not find peer ID for IP and port\n");
exit(1);
return -1; // Alternative behavior could be to simply ignore the packet
}

// Sends array of packets to every peer except sender
void flood_peers(data_packet_t *packet_list, int n_packets,
  server_state_t *state) {
  DPRINTF(DEBUG_SOCKETS, "flood_peers: Preparing to flood network with packets\n");

  // Iterate through known peers
  for (bt_peer_t *p = state->config->peers; p != NULL; p = p->next) {
    if (p->id != state->config->identity) {
        DPRINTF(DEBUG_SOCKETS, "flood_peers: Sending list of %d packet(s) to peer %d\n",
          n_packets, p->id);
        // Iterate through packets, sending each to given peer
        for (int i = 0; i < n_packets; i++)
          pct_send(&packet_list[i], &p->addr, state->sock); 
    }
  }
}

/* Creates a list of packets from given hashes, which it stores in
 * `packet_list`.  Returns the number of created packets. */
int hashes2packets(data_packet_t **packet_list, char *hashes, 
    int n_hashes, void (*packet_func)(data_packet_t *, char, char *)) {
  DPRINTF(DEBUG_PACKETS, "hashes2packets: Dividing hash list into packets\n");
    
  // Create an array of packets to store the chunk hashes
  int list_size = (n_hashes / MAX_CHK_HASHES) + 
    ((n_hashes % MAX_CHK_HASHES) != 0);
  *packet_list = malloc(sizeof(data_packet_t) * list_size);
  DPRINTF(DEBUG_PACKETS, "hashes2packets: Allocated %d packets for %d hashes\n",
    list_size, n_hashes);

  // Package the chunks into packets
  DPRINTF(DEBUG_PACKETS, "hashes2packets: Calling helper function to initialize packets\n");
  for (int i = 0; i < list_size; i++) {
    packet_func(packet_list[i], 
      MIN(MAX_CHK_HASHES, n_hashes), 
      &hashes[i * MAX_CHK_HASHES]);
    n_hashes -= MIN(MAX_CHK_HASHES, n_hashes);
  }
  DPRINTF(DEBUG_PACKETS, "hashes2packets: Finished creating packets\n");
  return list_size;
}

// Executes a user specified "GET <chunk list> <output file>" command
void cmd_get(char *chunkf, char *outputf, server_state_t *state) {
  DPRINTF(DEBUG_COMMANDS, "cmd_get: Executing get command\n");

  // Parse the chunk file
  int *ids = NULL;
  char *hashes = NULL;
  int n_hashes = chunkf_parse(chunkf, &hashes, &ids);

  // Create the packets
  data_packet_t *packet_list = NULL;
  int n_packets = hashes2packets(&packet_list, hashes, n_hashes, &pct_whohas);

  // Flood the network
  flood_peers(packet_list, n_packets, state);

  // Start the download process
  dload_start(&state->download, hashes, ids, n_hashes);

  free(hashes);
  free(ids);
  free(packet_list);
}

/* 
 * get_n_hashes
 *
 * Parses the integer giving the number of hashes in the payload for
 * WHOHAS and IHAVE packets.
 */
int get_n_hashes(data_packet_t pct) {
  char n = pct.data[0]; // Get the number of hashes in the packet

  if (n > MAX_CHK_HASHES) {
    fprintf(stderr, "Received packet claims to contain more than \
maximum number of hashes.\n");
    exit(1);
  }

  return n;
}

void process_whohas(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
  char matched[DATALEN];
  char m = 0; // Number of hash matches
  char n = get_n_hashes(pct);

  // Parse the hashes from the packet
  for (int i = 0; i < n; i++) {
    char *hash = pct.data + PAYLOAD_TOPPER + i * CHK_HASH_BYTES;

    int id = hash2id(hash, state->mcf_hashes, state->mcf_ids, state->mcf_len);

    // Check if the chunk is in the "has chunk file"
    if (id_in_ids(id, state->hcf_ids, state->hcf_len)) {
      strncpy(&matched[m++ * CHK_HASH_BYTES], hash, CHK_HASH_BYTES);
    }
  } 

  // Send the WHOHAS response
  data_packet_t *packet_list = NULL;
  // The WHOHAS response should technically fit in only 1 packet...
  int n_packets = hashes2packets(&packet_list, matched, m, &pct_ihave);
  for (int i = 0; i < n_packets; i++)
    pct_send(&packet_list[i], &from, state->sock);
  free(packet_list);
}

void process_ihave(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
  char n = get_n_hashes(pct);

  // Parse the hashes from the packet
  for (int i = 0; i < n; i++) {
    char *hash = pct.data + PAYLOAD_TOPPER + i * CHK_HASH_BYTES;
    int id = hash2id(hash, state->mcf_hashes, state->mcf_ids, state->mcf_len);
    dload_peer_add(&state->download, from, id);
  }
}

// Returns the first index of an empty upload in upload array, otherwise -1
int upload_first_empty(server_state_t *state) {
  for (int i = 0; i < MAX_UPLOADS; i++) {
      if(!state->uploads[i].busy) {
        return i;
      }
    }
    return -1; 
}

void process_get(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
    upload_t *upl;
    int up_emp = upload_first_empty(state);
  

    int buf_size = BT_CHUNK_SIZE; // Chunk-length
    char buf[buf_size]; 
    int id = hash2id(pct.data, state->mcf_hashes, state->mcf_ids, state->mcf_len);   // Convert the hash to hex, then get its id

    // Check that chunk is in "has chunk file", 
    if ((up_emp != -1) && (id_in_ids(id, state->hcf_ids, state->hcf_len))) {
      upl = &state->uploads[up_emp];
      upl->chunk.chunk_id = id;
      read_chunk(upl, state->dataf, buf); // Reads chunk from Data file in Master chunkfile
      make_packets(upl, buf, buf_size); // Split chunk into packets
      upl->busy = BUSY; // Change upload status 
      pct_send(&upl->chunk.packetlist[0], &from, state->sock);  // Send first packet
    }

    else { // Otherwise send DENIED back
      data_packet_t pac;
      pct_denied(&pac);
      pct_send(&pac, &from, state->sock);
    }
}

void process_data(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
  // Send ACK response
  data_packet_t new_packet;
  pct_ack(&new_packet, pct.header.seq_num);

  // Save data inside of downloads struct
  chunkd_t *chunks = state->download.chunks;
  int n = state->download.n_chunks;
  for (int i = 0; i < n; i++) {
    // Look for the relevant chunk
    if (strncmp((char *) &chunks[i].peer, (char *) &from, sizeof(from)) == 0) {
      // Insert node into pieces list
      // Update pieces counter
      // Store information about last_data_recv
      break;
     }
  }
  // Check if download is complete, if so verify chunk then write to disc
  // Start the download of the next chunk
}

void process_ack(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
  // Store ACK in uploads struct
  // Send the next data packet
  // Update uploads struct sequence number
}

void process_denied(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
  // Stop bothering the peer who sent us the denied packet
}

void process_inbound_udp(int sock, server_state_t *state) {
  data_packet_t pct;
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[PACKETLEN];

  fromlen = sizeof(from);
  spiffy_recvfrom(sock, buf, PACKETLEN, 0, (struct sockaddr *) &from, &fromlen);
  DPRINTF(DEBUG_SOCKETS, "Received UDP packet\n");

  memcpy(&pct, buf, sizeof(pct));
  int pct_type = pct.header.packet_type;
  switch (pct_type) {
    case WHOHAS_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received WHOHAS packet.\n");
      process_whohas(state, pct, from);
      break;
    case IHAVE_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received IHAVE packet.\n");
      process_ihave(state, pct, from);
      break;
    case GET_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received GET packet.\n");
      process_get(state, pct, from);
      break;
    case DATA_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received DATA packet.\n");
      process_data(state, pct, from);
      break;
    case ACK_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received ACK packet.\n");
      process_ack(state, pct, from);
      break;
    case DENIED_TYPE:
      DPRINTF(DEBUG_SOCKETS, "Received DENIED packet.\n");
      process_denied(state, pct, from);
      break;
    default:
      DPRINTF(DEBUG_SOCKETS, "Packet type %d not understood.\n", pct_type);
  }
} 

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];
  server_state_t *state = (server_state_t *) cbdata;

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));
  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
      cmd_get(chunkf, outf, state);
    }
  }
}

void server_state_init(server_state_t *state, bt_config_t *config, 
  int sock) {

  memset(state, 0, sizeof(*state));
  state->config = config;
  state->sock = sock;

  state->mcf_len = masterchunkf_parse(config->chunk_file, 
    &state->mcf_hashes, &state->mcf_ids, &state->dataf);
  state->hcf_len = chunkf_parse(config->has_chunk_file,
    &state->hcf_hashes, &state->hcf_ids);
}

void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readfds;
  struct user_iobuf *userbuf;
  
  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(1);
  }
  
  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(1);
  }
    
  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);
  
  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(1);
  }

  // Initialize server state
  server_state_t state;
  server_state_init(&state, config, sock);
  
  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    
    if (nfds > 0) {
      if (FD_ISSET(sock, &readfds)) {
	      process_inbound_udp(sock, &state);
      }
      
      if (FD_ISSET(STDIN_FILENO, &readfds)) {
	      process_user_input(STDIN_FILENO, userbuf, handle_user_input, &state);
      }
    }
  }
}
