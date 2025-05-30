#ifndef GET_PUBLIC_KEY_H
#define GET_PUBLIC_KEY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_globals.h"
#include "handlers.h"
#include "hedera.h"
#include "app_io.h"
#include "ui_common.h"
#include "utils.h"

typedef struct get_public_key_context_s {
    uint32_t deriv_path[MAX_DERIV_PATH_LEN];
    uint32_t key_index;

    // Lines on the UI Screen
    char ui_approve_l2[RAW_PUBKEY_SIZE*2 +1];

    uint8_t raw_pubkey[RAW_PUBKEY_SIZE];

    // Public Key Compare
    uint8_t display_index;
    uint8_t full_key[RAW_PUBKEY_SIZE*2 +1];
    uint8_t partial_key[DISPLAY_SIZE + 1];
} get_public_key_context_t;

extern get_public_key_context_t gpk_ctx;

// Implementation of this function is very tolerant to malformed derivation paths
// It could parse single index or full derivation path (two to five elements)
// If buffer is empty it will use default derivation path
// It returns 0 if it successfully parsed the derivation path, -1 otherwise
int get_key_index_from_buffer(const uint8_t *buffer, uint16_t len, uint32_t out_deriv_path[MAX_DERIV_PATH_LEN], uint32_t* out_key_index);

#endif // GET_PUBLIC_KEY_H