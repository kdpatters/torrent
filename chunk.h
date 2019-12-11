/*
 * chunk.h
 *
 * Skeleton for CMPU-375 programming project #2: chunk parsing
 */

#include <stdio.h>
#include <inttypes.h>

#define BT_CHUNK_SIZE (512 * 1024)
#define CHK_HASH_BYTES 20
#define CHK_HASH_ASCII (2 * CHK_HASH_BYTES)
#define ascii2hex(ascii,len,buf) hex2binary((ascii),(len),(buf))
#define hex2ascii(buf,len,ascii) binary2hex((buf),(len),(ascii))

/* Returns the number of chunks created, return -1 on error */
int make_chunks(FILE *fp, uint8_t **chunk_hashes);  

/* returns the sha hash of the string */
void shahash(uint8_t *chr, int len, uint8_t *target);

/* converts a hex string to ascii */
void binary2hex(uint8_t *buf, int len, char *ascii);

/* converts an ascii to hex */
void hex2binary(char *hex, int len, uint8_t*buf);

char *id2hash(int, char *, int);
int hash2id(char *, char *, int *, int);
int masterchunkf_parse(char *, char **, int **, char **);
int chunkf_parse(char *, char **, int **);
int verify_hash(uint8_t *str, int len, uint8_t *hash);
