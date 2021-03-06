typedef struct server_state_s {
    download_t download;
    upload_t *uploads;
    bt_config_t *config;
    int num_uploads;
    int num_downloads;

    // Data file
    char *dataf;

    // Master chunk file
    int *mcf_ids;
    char *mcf_hashes;
    int mcf_len;

    // Has chunks file
    int *hcf_ids;
    char *hcf_hashes;
    int hcf_len;

    // Peer info
    int n_peers; // Number of peers in the network
    char *peer_free; // Sparse array containing bools for whether each peer is free
    int peer_free_size; // Size of 'peer_free' array

    int sock;
} server_state_t;
