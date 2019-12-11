#define PEER_FREE 1
#define PEER_BUSY 0

int get_largest_peer_id(bt_config_t *config);
int get_n_peers(bt_config_t *config);
int id_in_ids(int id, int *ids, int ids_len);                  
