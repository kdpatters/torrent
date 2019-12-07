/*
 * bt_parse.c
 *
 * Skeleton for CMPU-375 programming project #2:
 *   command line and config file parsing stubs.
 */

#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "bt_parse.h"
#include "debug.h"

static const char* const _bt_optstring = "p:c:f:m:i:d:h";

void bt_init(bt_config_t *config, int argc, char **argv) {
  bzero(config, sizeof(bt_config_t));

  strcpy(config->output_file, "output.dat");
  strcpy(config->peer_list_file, "nodes.map");
  config->argc = argc;
  config->argv = argv;
}

void bt_usage() {
  fprintf(stderr, 
	  "usage:  peer [-h] [-d <debug>] -p <peerfile> -c <chunkfile> -m <maxconn>\n"
	  "            -f <master-chunk-file> -i <identity>\n");
}

void bt_help() {
  bt_usage();
  fprintf(stderr,
	  "         -h                help (this message)\n"
	  "         -p <peerfile>     The list of all peers\n"
	  "         -c <chunkfile>    The list of chunks\n"
	  "         -m <maxconn>      Max # of downloads\n"
	  "	    -f <master-chunk> The master chunk file\n"
	  "         -i <identity>     Which peer # am I?\n"
	  );
}

bt_peer_t *bt_peer_info(const bt_config_t *config, int peer_id)
{
  assert(config != NULL);

  bt_peer_t *p;
  for (p = config->peers; p != NULL; p = p->next) {
    if (p->id == peer_id) {
      return p;
    }
  }
  return NULL;
}
 
void bt_parse_command_line(bt_config_t *config) {
  int c, old_optind;
  bt_peer_t *p;

  int argc = config->argc;
  char **argv = config->argv;

  DPRINTF(DEBUG_INIT, "bt_parse_command_line starting\n");
  old_optind = optind;
  while ((c = getopt(argc, argv, _bt_optstring)) != -1) {
    switch(c) {
    case 'h':
      bt_help();
      exit(0);
    case 'd':
      if (set_debug(optarg) == -1) {
	fprintf(stderr, "%s:  Invalid debug argument.  Use -d list  to see debugging options.\n",
		argv[0]);
	exit(-1);
      }
      break;
    case 'p': 
      strcpy(config->peer_list_file, optarg);
      break;
    case 'c':
      strcpy(config->has_chunk_file, optarg);
      break;
    case 'f':
      strcpy(config->chunk_file, optarg);
      break;
    case 'm':
      config->max_conn = atoi(optarg);
      break;
    case 'i':
      config->identity = atoi(optarg);
      break;
    default:
      bt_usage();
      exit(-1);
    }
  }

  bt_parse_peer_list(config);

  if (config->identity == 0) {
    fprintf(stderr, "bt_parse error:  Node identity must not be zero!\n");
    exit(-1);
  }
  if ((p = bt_peer_info(config, config->identity)) == NULL) {
    fprintf(stderr, "bt_parse error:  No peer information for myself (id %d)!\n",
	    config->identity);
    exit(-1);
  }
  config->myport = ntohs(p->addr.sin_port);
  assert(config->identity != 0);
  assert(strlen(config->chunk_file) != 0);
  assert(strlen(config->has_chunk_file) != 0);

  optind = old_optind;
}

void bt_parse_peer_list(bt_config_t *config) {
  FILE *f;
  bt_peer_t *node;
  char line[BT_FILENAME_LEN], hostname[BT_FILENAME_LEN];
  int nodeid, port;
  struct hostent *host;

  assert(config != NULL);
  
  f = fopen(config->peer_list_file, "r");
  assert(f != NULL);
  
  while (fgets(line, BT_FILENAME_LEN, f) != NULL) {
    if (line[0] == '#') continue;
    assert(sscanf(line, "%d %s %d", &nodeid, hostname, &port) != 0);

    node = (bt_peer_t *) malloc(sizeof(bt_peer_t));
    assert(node != NULL);

    node->id = nodeid;

    host = gethostbyname(hostname);
    assert(host != NULL);
    node->addr.sin_addr.s_addr = *(in_addr_t *)host->h_addr;
    node->addr.sin_family = AF_INET;
    node->addr.sin_port = htons(port);

    node->next = config->peers;
    config->peers = node;
  }
}

void bt_dump_config(bt_config_t *config) {
  /* Print out the results of the parsing. */
  bt_peer_t *p;
  assert(config != NULL);

  printf("CMPU 375 PROJECT 2 PEER\n\n");
  printf("chunk-file:     %s\n", config->chunk_file);
  printf("has-chunk-file: %s\n", config->has_chunk_file);
  printf("max-conn:       %d\n", config->max_conn);
  printf("peer-identity:  %d\n", config->identity);
  printf("peer-list-file: %s\n", config->peer_list_file);
  
  for (p = config->peers; p; p = p->next) 
    printf("  peer %d: %s:%d\n", p->id, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
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

    char fname[MAX_FILENAME];
    memset(fname, 0, sizeof(fname));

    // Read a line from the chunklist file
  while (getline(&buf, (size_t *) &buf_size, f) > 0) {
    if(cur_line == 0) {
      int char_indx = strcspn(buf, " ");
      while (char_indx < strlen(buf)) {
        if (buf[char_indx] == ' ')
          char_indx++;
        else {
          strncpy(fname, buf, MAX_FILENAME);
          if(fname[MAX_FILENAME - 1] != '\0') {
            fprintf(stderr, "Filename for Datafile in masterchunkfile is too long.\n");
            exit(1);
          }
          break;
          
        }

      }
    }


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
