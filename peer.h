/* Header file for peer */

#include "download.h"
#include "bt_parse.h"
#include "chunk.h"

typedef struct server_state_s {
    bt_config_t config;
    download_t download;
} server_state_t;