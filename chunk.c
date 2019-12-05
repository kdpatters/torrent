/*
 * chunk.c
 *
 * Skeleton for CMPU-375 programming project #2: chunk parsing
 */

#include "sha.h"
#include "chunk.h"
#include <ctype.h>
#include <assert.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memset

<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
void hash_nodes_to_list(chunk_hash_t *chunklist, int num_chunks, 
  char hashes[][CHK_HASHLEN], int ids[]) {

  // Convert linked list intro string array
  chunk_hash_t *curr = chunklist;
  for (int i = 0; i < num_chunks; i++) {
    strncpy(hashes[i], curr->hash, CHK_HASHLEN);
    chunk_hash_t *prev = curr;
    curr = curr->next;
    free(prev);
  }
}

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

  // Store data for individual chunk
  int id;
  char hash[CHK_HASHLEN];
  chunk_hash_t *curr = NULL;

  int buf_size = sizeof(int) + sizeof(' ') + CHK_HASHLEN;
  char *buf = malloc(buf_size + 1);
  // Read a line from the chunklist file
  while (getline(&buf, (size_t *) &buf_size, f) > 0) {
    if (strlen(buf) < CHK_HASHLEN + strlen("0 ")) {
      fprintf(stderr, "Line \"%s\" in chunkfile \"%s\" was too short and could not be parsed.\n", 
        buf, chunkfile);
	  exit(-1);
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

int parse_hashes_ids(char *chunkfile, char hashes[][CHK_HASHLEN], int ids[]) {
  chunk_hash_t *chunklist = NULL; // Parse the chunkfile	
  int num_chunks = parse_chunkfile(chunkfile, &chunklist);
  hash_nodes_to_list(chunklist, num_chunks, hashes, ids);

  // Create array of strings
  hashes = malloc(num_chunks * (CHK_HASHLEN));
  ids = malloc(num_chunks * sizeof(int));
  memset(hashes, 0, sizeof(*hashes));
  memset(ids, 0, sizeof(*ids));

  return num_chunks;
}

>>>>>>> 6ee487ea7c13d87368f3ed3335c69a6e1972c4f3
/*
 * Return the ID for a specific hash given as bytes.  If the ID is not
 * found, the function will return -1.
 */
int hash2id(char *hash, bt_config_t *config) {
    chunk_hash_t *chunklist = NULL;
    int n_chunks = 0; // stupid stupid stupid
    
    chunk_hash_t *curr_ch = chunklist;
    for (int j = 0; j < n_chunks; j++) {
        if (strncmp(hash, curr_ch->hash, CHK_HASHLEN))
            return curr_ch->id;
        curr_ch = curr_ch->next; // Move on to next chunk hash if not found
    }
	return -1;
}
>>>>>>> df9a181f4f5640dc6c126adb24ea3a4bbd3471a0

/**
 * fp -- the file pointer you want to chunkify.
 * chunk_hashes -- The chunks are stored at this locations.
 */
/* Returns the number of chunks created */
int make_chunks(FILE *fp, uint8_t *chunk_hashes[]) {
	//allocate a big buffer for the chunks from the file.
	uint8_t *buffer = (uint8_t *) malloc(BT_CHUNK_SIZE);
	int numchunks = 0;
	int numbytes = 0;

	// read the bytes from the file and fill in the chunk_hashes
	while((numbytes = fread(buffer, sizeof(uint8_t), BT_CHUNK_SIZE, fp)) > 0 ) {
		shahash(buffer, numbytes, chunk_hashes[numchunks++]);
	}

	return numchunks;
}

/**
 * str -- is the data you want to hash
 * len -- the length of the data you want to hash.
 * hash -- the target where the function stores the hash. The length of the
 *         array should be SHA1_HASH_SIZE.
 */
void shahash(uint8_t *str, int len, uint8_t *hash) {
	SHA1Context sha;

	// init the context.
	SHA1Init(&sha);

	// update the hashes.
	// this can be used multiple times to add more
	// data to hash.
	SHA1Update(&sha, str, len);

	// finalize the hash and store in the target.
	SHA1Final(&sha, hash);

	// A little bit of paranoia.
	memset(&sha, 0, sizeof(sha));
}

/**
 * converts the binary char string str to ascii format. the length of 
 * ascii should be 2 times that of str
 */
void binary2hex(uint8_t *buf, int len, char *hex) {
	int i=0;
	for(i=0;i<len;i++) {
		sprintf(hex+(i*2), "%.2x", buf[i]);
	}
	hex[len*2] = 0;
}
  
/**
 *Ascii to hex conversion routine
 */
static uint8_t _hex2binary(char hex)
{
     hex = toupper(hex);
     uint8_t c = ((hex <= '9') ? (hex - '0') : (hex - ('A' - 0x0A)));
     return c;
}

/**
 * converts the ascii character string in "ascii" to binary string in "buf"
 * the length of buf should be atleast len / 2
 */
void hex2binary(char *hex, int len, uint8_t*buf) {
	int i = 0;
	for(i=0;i<len;i+=2) {
		buf[i/2] = 	_hex2binary(hex[i]) << 4 
				| _hex2binary(hex[i+1]);
	}
}

#ifdef _TEST_CHUNK_C_
int main(int argc, char *argv[]) {
	uint8_t *test = "dash";
	uint8_t hash[SHA1_HASH_SIZE], hash1[SHA1_HASH_SIZE];
	char ascii[SHA1_HASH_SIZE*2+1];

	shahash(test,4,hash);
	
	binary2hex(hash,SHA1_HASH_SIZE,ascii);

	printf("%s\n",ascii);

	assert(strlen(ascii) == 40);

	hex2binary(ascii, strlen(ascii), hash1);

	binary2hex(hash1,SHA1_HASH_SIZE,ascii);

	printf("%s\n",ascii);
}
#endif //_TEST_CHUNK_C_
