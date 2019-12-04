






// Function to get ID for a specific chunk HASH
int hash_to_id (char *hash, config_t config) {
    chunk_hash_t *chunklist = NULL;
    int n_chunks = parse_Master_chunkfile(config->chunk_file, &chunklist); //num of chunks in (master)chunk_file
    
    chunk_hash_t *curr_ch = chunklist;
    for(int j = 0; j < n_chunks; j++) {
        if(strncmp(hash, curr_ch->hash, CHK_HASHLEN)
            return curr_ch->id;
        curr_ch = curr_ch->next; // Move on to next chunk hash if not found
    }

}