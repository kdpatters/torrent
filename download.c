/* 
 * download.c
 * 
 */

/* 
 * not_at_max_downloads
 * 
 * Check that client has not already begun the maximum number of downloads. 
 */
char not_at_max_downloads(downloads_s downloads) {
  return downloads->n < MAX_DOWNLOADS;
}

/*
 * init_downloads_struct
 * 
 */
void init_downloads_struct(downloads_s downloads) {
    memset(&downloads, 0, sizeof(downloads));
}

/* 
 * find_first_empty
 * 
 * Find and return the index of the first empty download slot.
 */
int find_first_empty(downloads_s downloads) {
    for (int i = 0; downloads->downloads[i].state; i++) {
        if (i >= MAX_DOWNLOADS - 1) {
            fprintf(stderr, "Attempted to add a new download when max downloads exceeded.\n");
            exit(-1);
        }
    }
    return i;
}

/* 
 * begin_download
 * 
 * Initialize the struct to keep track of the chunks and connected peers for the
 * specific download.
 */
void begin_download(char hash[][MAX_HASH_BYTES], downloads_s downloads) {
    // Initialize struct to keep track of current downloads
    download_s *curr = &downloads[find_first_empty(downloads)];
    curr->

    // Send the first get
    send_get();
}

void send_get() {
    // Create the GET packet
    create_get_packet();

    // Send the GET packet
    send_pack();
}

void create_get_packet() {
    // Initialize packet
    // Set type
    // Call helper
    create_pack_helper()
}

void check_retry_get() {
    // Check when the last data packet was received
    // Either attempt to verify chunk, stop download entirely (if max retries exceeded), or send another GET
}

void process_data() {
    // Store data in chunk download struct
    /
}

void verify_chunk() {
    // Generate hash for chunk data
    // Compare generated hash to current hash
}

void create_ack_packet() {
    // Initialize packet
    // Set type
    // Call helper
}

void send_ack() {
    create_ack_packet
    send_pack()
}
