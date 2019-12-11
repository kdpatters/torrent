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

#define PCT_TIMEOUT 500000

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 3; // your group number here
  strcpy(config.chunk_file, "example/C.masterchunks");
  strcpy(config.has_chunk_file, "example/A.chunks");
  peer_test();
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
  DPRINTF(DEBUG_ALL, "peer_addr_to_id: Started\n");
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

struct sockaddr_in *peer_id_to_addr(int peer_id, server_state_t *state) {
  DPRINTF(DEBUG_ALL, "peer_id_to_add: Started\n");
  for (bt_peer_t *p = state->config->peers; p != NULL; p = p->next) {
    if (p->id == peer_id) {
      return &p->addr;
    }
  }
  fprintf(stderr, "Could not find peer ID\n");
  exit(1);
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
  dload_start(&state->download, hashes, ids, n_hashes, outputf);

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
    fprintf(stderr, "get_n_hashes: Received packet claims to contain more than \
maximum number of hashes with %d total.\n", n);
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

    // Check if the chunk is in the "has chunk file"
    int id = hash2id(hash, state->hcf_hashes, state->hcf_ids, state->hcf_len);
    if (id >= 0) {
      strncpy(&matched[m++ * CHK_HASH_BYTES], hash, CHK_HASH_BYTES);
    }
  } 
  DPRINTF(DEBUG_CHUNKS, "process_whohas: Matched %d out of %d requested chunks\n", m, n);

  // Send the WHOHAS response
  data_packet_t *packet_list = NULL;
  // The WHOHAS response should technically fit in only 1 packet...
  int n_packets = hashes2packets(&packet_list, matched, m, &pct_ihave);
  DPRINTF(DEBUG_PACKETS, "process_whohas: Created %d IHAVE packets for %d chunks\n",
    n_packets, m);
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
    DPRINTF(DEBUG_PACKETS, "process_ihave: Read chunk id %d from packet\n", id);
    dload_peer_add(&state->download, peer_addr_to_id(from, state), id);
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
      upl->peer_id = peer_addr_to_id(from, state);
      upl->chunk.chunk_id = id;
      read_chunk(upl, state->dataf, buf); // Reads chunk from Data file in Master chunkfile
      make_packets(upl, buf, buf_size); // Split chunk into packets
      upl->busy = BUSY; // Change upload status

      for(int i = 0; i < ACK_WINDOW_SZ; i++) {
        pct_send(&upl->chunk.packetlist[i], &from, state->sock);  // Send first 8 packets
        upl->next_pack_ind = ACK_WINDOW_SZ; // Set the next pct ind to be the window sz after first sending & increment from there 

      } 
      upl->busy = BUSY; // Change upload status 
      pct_send(&upl->chunk.packetlist[upl->seq_num++], &from, state->sock);  // Send first packet
    }

    else { // Otherwise send DENIED back
      data_packet_t pac;
      pct_denied(&pac);
      pct_send(&pac, &from, state->sock);
    }
}

void process_data(server_state_t *state, data_packet_t pct, struct sockaddr_in from) {
 
  int peer_id = peer_addr_to_id(from, state); // Get sender's peer id

  chunkd_t *chunks = state->download.chunks;
  int n = state->download.n_chunks;
  for (int i = 0; i < n; i++) {
    // Look for the relevant chunk
    chunkd_t *chk = &chunks[i];
    if (chk->peer == peer_id) {
    DPRINTF(DEBUG_DOWNLOAD, "process_data: Received DATA from peer %d for chunk %d\n" , 
      peer_id, i);
    DPRINTF(DEBUG_DOWNLOAD, "process_data: Have received total of %d / %d bytes so far\n",
      chk->total_bytes, BT_CHUNK_SIZE);
 
      // Check if it is necessary to resize pieces array
      int seq_num = pct.header.seq_num;
      if (chk->pieces_size <= seq_num) {
        DPRINTF(DEBUG_DOWNLOAD, "process_data: Chunk packet array is too small, allocating more memory\n");
        int old_size = chk->pieces_size;
        int new_size = (1 + seq_num) * 2;
        chk->pieces_size = new_size;
        chk->pieces = realloc(chk->pieces, sizeof(*chk->pieces) * chk->pieces_size);
        chk->pieces_filled = realloc(chk->pieces_filled, 
          sizeof(*chk->pieces_filled) * chk->pieces_size);
        memset(&chk->pieces_filled[old_size], 0, sizeof(*chk->pieces_filled) * 
          (new_size - old_size));
        fprintf(stderr, "filled: %d\n", chk->pieces_filled[old_size]);
      }
      // Insert the data into the pieces array if slot is not already filled
      DPRINTF(DEBUG_DOWNLOAD, "process_data: Copying packet data into chunk info in download struct\n");
      if (!chk->pieces_filled[seq_num]) {
        memcpy(&chk->pieces[seq_num], &pct, sizeof(pct));
        chk->pieces_filled[seq_num] = 1;
        DPRINTF(DEBUG_DOWNLOAD, "process_data: Packet contains payload of %d bytes\n", pct.header.packet_len - pct.header.header_len);
        chk->total_bytes += pct.header.packet_len - pct.header.header_len;
      }
      else {
        DPRINTF(DEBUG_DOWNLOAD, "process_data: Received duplicate of packet \
sequence no. %d\n", seq_num);
        exit(0);
      }

      // Store information about last_data_recv
      chk->last_data_recv = time(0);

      // Send ACK response
      data_packet_t new_packet;
      pct_ack(&new_packet, seq_num);
      pct_send(&new_packet, &from, state->sock);

      // Check if download is complete, if so verify chunk then write to disk
      if (chk->total_bytes >= BT_CHUNK_SIZE) {
        DPRINTF(DEBUG_DOWNLOAD, "process_data: Combining pieces of chunk\n");

        // Assemble pieces
        void *data_ptr = chk->data;
        for (int j = 0; j < chk->pieces_size; j++) {
          if (chk->pieces_filled[j]) {
            data_packet_t *dat = &chk->pieces[j];
            int n_bytes = dat->header.packet_len - dat->header.header_len;
            memcpy(data_ptr, dat->data, n_bytes);
            data_ptr += n_bytes;
          }
        }

        DPRINTF(DEBUG_DOWNLOAD, "process_data: Verifying the chunk\n");
        // Verify the chunk
        if (verify_hash((uint8_t *) chk->data, chk->total_bytes, (uint8_t *) chk->hash)) {
          DPRINTF(DEBUG_DOWNLOAD, 
            "process_data: Chunk %d successfully downloaded!\n", chk->chunk_id);
          // Write the chunk to disk
          char *fname = state->download.output_file;
          FILE *f = fopen(fname, "w+");
          if (f == NULL) {
            fprintf(stderr, "Could not open output file %s\n", fname);
            exit(1);
          }
          fwrite(chk->data, chk->total_bytes, 1, f);
          fclose(f);
        } else {
          DPRINTF(DEBUG_DOWNLOAD, "Chunk could not be verified, redownloading\n");
          // Redownload chunk
          // chk->pieces_filled = 0;
          // data_packet_t pack;                                                                    
          // pct_get(&pack, chk->hash);                          
          // pct_send(&pack, peer_id_to_addr(chunk->peer, 
          //     state), state->sock);
          // break;
        }

        // Empty the pieces
        free(chk->pieces);
        free(chk->pieces_filled);
      }
     }
  }
}
   
/*
 * Returns index of the relevant upload if found, otherwise returns -1.
 */
int get_relevant_upload(struct sockaddr_in from, server_state_t *state) {
    int sender_id = peer_addr_to_id(from, state);
    for (int i = 0; i < MAX_UPLOADS; i++) {
        int peer_id = state->uploads[i].peer_id;
        if (sender_id == peer_id) {
          DPRINTF(DEBUG_UPLOAD, "get_relevant_upload: Found match for peer id %d and upload %d\n", peer_id, i);  
          return i;
        }
    }
    DPRINTF(DEBUG_UPLOAD, "get_relevant_upload: Did not find any match for peer id %d in uploads\n", sender_id);
    return -1;
}

// Process ack upon receiving
void process_ack(server_state_t *state, data_packet_t ack, struct sockaddr_in from) {
  int ind = get_relevant_upload(from, state);  // Find the specific upload matching requesting peer_id
  // Ignore the ACK if we aren't processing an upload for the sender
  if (ind == -1) {
     return;
  }
  DPRINTF(DEBUG_UPLOAD, "process_ack: Reading info about upload %d\n", ind);
  upload_t *up = &state->uploads[ind];
  up->recv[ack.header.ack_num] += 1; // 1 - index into rec using ack_num to indicate ack for that packet received
  // Store ACK in uploads struct
  int next_seq = up->next_pack_ind;
  // Send the next data packet
  DPRINTF(DEBUG_UPLOAD, "process_ack: %d of %d packets sent\n",
    up->next_seq, up->chunk.l_size);
  if (up->next_seq < up->chunk.l_size) { // While index still within the packetlist
      struct sockaddr_in *peer_addr = peer_id_to_addr(up->peer_id, state); // Get peer address
      check_retry_upl(up, next_seq, state, peer_addr);
      // pct_send(&up->chunk.packetlist[up->seq_num++], peer_addr, state->sock);  // Send and update uploads struct sequence number
  } else {
    DPRINTF(DEBUG_UPLOAD, "process_ack: Upload to peer %d completed\n", up->peer_id);
  }
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

void dload_check_status(server_state_t *state) {                           
    download_t *download = &state->download;                               
    double time_passed = difftime(time(0), download->time_started);    
    //int n = download->n_chunks;                                          
    // Number of chunks waiting to download                                
    //int w = download->n_chunks - download->n_in_progress;                  
    //chunkd_t *chunks = download->chunks;                                 
    /* Check if download is waiting for IHAVE responses and the timeout 
     * has been surpassed. */         
//    DPRINTF(DEBUG_DOWNLOAD, "dload_check_status: Checking if it's time to send a GET request\n");
//    DPRINTF(DEBUG_DOWNLOAD, "dload_check_status: Download has status %d and %gs have passed since the last IHAVE\n", download->waiting_ihave, time_passed);                                    
    if (download->waiting_ihave && (time_passed > TIME_WAIT_IHAVE)) {   
      //  for (int i = 0; i < w; i++) {                                      
            int rarest = dload_rarest_chunk(download);                     
            chunkd_t *chunk = &download->chunks[rarest];                   
            //bt_peer_t *peer = bt_peer_info(state->config, chunk->peer_list[0]);
            // TODO: replace peer list with ids instead of addr            
            // TODO: check if download in progress with given peer
            if (chunk->pl_filled > 0) { 
                data_packet_t pack;                                            
                char *hash = id2hash(chunk->chunk_id,                          
                    state->mcf_hashes, state->mcf_len);                        
                pct_get(&pack, hash);                
                // Choose peer
                int peer = chunk->peer_list[0];    
                chunk->peer = peer;                      
                pct_send(&pack, peer_id_to_addr(peer, 
                    state), state->sock);
            }    
            download->waiting_ihave = 0; // Stop waiting for IHAVE             
       // }                                                               
                                                                        
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
  
  struct timeval pct_timeout;
  while (1) {
    int nfds;
    pct_timeout.tv_usec = PCT_TIMEOUT;

    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);

    dload_check_status(&state);
    //check_retry_upl();
    
    nfds = select(sock+1, &readfds, NULL, NULL, &pct_timeout);
    
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
