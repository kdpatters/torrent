/*
 * chunk.c
 *
 * Chunk parsing and hash operations
 */

#include <ctype.h>
#include <assert.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memset
#include <stdio.h>
#include "sha.h"
#include "chunk.h"
#include "debug.h"

int helper_parse_chunkf(FILE *fp, char **hashes, int **ids) {
  DPRINTF(DEBUG_CHUNKS, "helper_parse_chunkf: Started\n");
  
  int n = 0, size = 1;
  *hashes = malloc(size * CHK_HASH_BYTES);
  *ids = malloc(size * sizeof(*ids));

  // Read lines from the chunk list file
  size_t buf_size = 0;
  char *buf;
  while (getline(&buf, &buf_size, fp) > 0) {
	if (size <= n) { // Increase size of arrays if they become filled
		size *= 2;
		*hashes = realloc(*hashes, size * CHK_HASH_BYTES);
  		*ids = realloc(*ids, size * sizeof(**ids));
	}
    if (strlen(buf) < CHK_HASH_ASCII + strlen("0 ")) {
      fprintf(stderr, "Line \"%s\" in chunk file was too short and could not be parsed.\n", 
        buf);
    }
	else { // Store the chunk's hash and id
		char id[buf_size], hash[buf_size];
		sscanf(buf, "%s %s", id, hash);
		strncpy(&(*hashes)[n * CHK_HASH_BYTES], hash, CHK_HASH_BYTES);
		(*ids)[n++] = atoi(id);
	}
  }	
  free(buf);
  DPRINTF(DEBUG_CHUNKS, "helper_parse_chunkf: Finished parsing chunk file\n");
  return n;
}

FILE *file_read_or_die(char *fname) {
  FILE *fp = fopen(fname, "r");
  if (fp == NULL) {
    fprintf(stderr, "Could not read file \"%s\".\n", fname);
    exit(1);
  }
  return fp;
}

int masterchunkf_parse(char *fname, char **hashes, int **ids, char **dataf) {
	DPRINTF(DEBUG_CHUNKS, "masterchunkf_parse: Parsing master chunk file %s\n", fname);

	FILE *fp = file_read_or_die(fname);
	size_t buf_size1 = 0, buf_size2 = 0;
  	char *buf1, *buf2; // Buf2 is a throwaway variable

	// Read the first two lines of the master chunk file
	if ((getline(&buf1, &buf_size1, fp) < 0) ||
		(getline(&buf2, &buf_size2, fp) < 0)) {
		fprintf(stderr, "Could not read first 2 lines of master chunk file.\n");
		exit(1);
	}

	// Parse the name of the data file
	*dataf = malloc(buf_size1);
	sscanf(buf1, "%s %s", buf2, *dataf);

	free(buf1);
	free(buf2);
	return helper_parse_chunkf(fp, hashes, ids); // Return # of chunks
}

int chunkf_parse(char *fname, char **hashes, int **ids) {
	DPRINTF(DEBUG_CHUNKS, "chunkf_parse: Parsing chunk file %s\n", fname);

	FILE *fp = file_read_or_die(fname);
	return helper_parse_chunkf(fp, hashes, ids); // Return # of chunks
}

/*
 * Return the ID for a specific hash given as bytes.  Uses an array of hashes
 * 'hashes' and an array of ids 'ids' of length 'n_hashes' to find the id of the
 * given hash 'hash'.  If the ID is not found, the function will return -1.
 */
int hash2id(char *hash, char *hashes, int *ids, int n_hashes) {
   for (int i = 0; i < n_hashes; i++) {
       if (memcmp(hash, &hashes[i * CHK_HASH_BYTES], CHK_HASH_BYTES))
           return ids[i];
   }
return -1;
}

/* 
 * Given an array of hashes of length 'CHK_HASH_BYTES', return the hash 
 * corresponding to the id 'id' or NULL if the id is not found.
 */
char *id2hash(int id, char *hashes, int n_hashes) {
    if (id < n_hashes) {
        return &hashes[id * CHK_HASH_BYTES];
    }
    return NULL;
}

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

/* Returns True only when the hash generated from `str` matches the
 * hash given by `hash` */
int verify_hash(uint8_t *str, int len, uint8_t *hash) {
  uint8_t gen_hash[SHA1_HASH_SIZE];
  shahash(str, len, gen_hash);
  return !memcmp((char *) hash, (char *) &gen_hash, SHA1_HASH_SIZE);
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
