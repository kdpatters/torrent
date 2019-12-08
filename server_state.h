typedef struct server_state_s {
    download_t download;
    bt_config_t *config;

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

    int sock;
} server_state_t;
