#include <os.h>
#include <os_io.h>
#include <os_io_seproxyhal.h>
#include "get_public_key.h"
#include "ui/app_globals.h"
#include <swap_utils.h>

get_public_key_context_t gpk_ctx;

static void set_default_deriv_path_with_index(uint32_t out_deriv_path[MAX_DERIV_PATH_LEN], uint32_t key_index) {
    out_deriv_path[0] = PATH_ZERO;
    out_deriv_path[1] = PATH_ONE;
    out_deriv_path[2] = PATH_TWO;
    out_deriv_path[3] = PATH_THREE;
    out_deriv_path[4] = PATH_FOUR | key_index;
}

int parse_derivation_path(const uint8_t *buffer, uint16_t len, uint32_t out_deriv_path[MAX_DERIV_PATH_LEN], uint32_t* out_key_index) {
    uint32_t deriv_path_len = len / 4;
    // If the buffer is empty, use the default derivation path
    if (buffer == NULL || deriv_path_len == 0) {
        PRINTF("Empty derivation path buffer, falling back to default derivation path\n");
        set_default_deriv_path_with_index(out_deriv_path, 0);
        if (out_key_index) *out_key_index = 0;
        return 0;
    }

    // If the buffer is too long, return an error
    if (deriv_path_len > MAX_DERIV_PATH_LEN) {
        PRINTF("Invalid derivation path length\n");
        return -1;
    }

    // Parse the derivation path from the buffer
    uint32_t bip32_path[MAX_DERIV_PATH_LEN];
    for (size_t i = 0; i < deriv_path_len; i++) {
        bip32_path[i] = ((uint32_t)buffer[4*i+0]) |
                        ((uint32_t)buffer[4*i+1] << 8) |
                        ((uint32_t)buffer[4*i+2] << 16) |
                        ((uint32_t)buffer[4*i+3] << 24);
    }

    // If there is only one element, treat it as the key index
    if (deriv_path_len == 1)
    {
        PRINTF("Deriving from a single index\n");
        set_default_deriv_path_with_index(out_deriv_path, bip32_path[0]);
        if (out_key_index) *out_key_index = bip32_path[0] & 0x7FFFFFFF;
        return 0;
    }

    // From here, parse full derivation paths
    // Check if the first two elements are correct
    if (bip32_path[0] != (PATH_ZERO) || bip32_path[1] != (PATH_ONE)) {
        PRINTF("Invalid derivation path prefix\n");
        PRINTF("bip32_path[0]: %d\n", bip32_path[0]);
        PRINTF("bip32_path[1]: %d\n", bip32_path[1]);
        return -1;
    }
    // If there are only two elements, it is m/44'/3030', so index=0
    // This is the old LedgerLive Hedera derivation path format, without index
    if (deriv_path_len == 2) {
        set_default_deriv_path_with_index(out_deriv_path, 0);
        if (out_key_index) *out_key_index = 0;
        return 0;
    }
    // If there are 3, 4 or 5 elements, last element is the index
    // We don't support assigning other elements of the derivation path because of safety reasons -
    // Users only confirm the index
    out_deriv_path[0] = PATH_ZERO;
    out_deriv_path[1] = PATH_ONE;
    out_deriv_path[2] = PATH_TWO;
    out_deriv_path[3] = PATH_THREE;
    out_deriv_path[4] = PATH_FOUR | bip32_path[deriv_path_len-1];
    if (out_key_index) *out_key_index = bip32_path[deriv_path_len-1] & 0x7FFFFFFF;
    return 0;
}


static bool get_pk() {
    // Derive Key
    if (!hedera_get_pubkey(gpk_ctx.deriv_path, gpk_ctx.raw_pubkey)) {
        return false;
    }

    if (sizeof(G_io_apdu_buffer) < 32) {
        THROW(EXCEPTION_INTERNAL);
    }

    // Put Key bytes in APDU buffer
    uint8_t key_buffer[33];
    public_key_to_bytes(key_buffer, gpk_ctx.raw_pubkey);
    memcpy(G_io_apdu_buffer, key_buffer, 32);
    memcpy(gpk_ctx.raw_pubkey, key_buffer, 32);

    // Populate Key Hex String
    bin2hex(gpk_ctx.full_key, G_io_apdu_buffer, KEY_SIZE);
    gpk_ctx.full_key[KEY_SIZE] = '\0';

    return true;
}

void handle_get_public_key(uint8_t p1, uint8_t p2, uint8_t* buffer,
                           uint16_t len,
                           /* out */ volatile unsigned int* flags,
                           /* out */ volatile unsigned int* tx) {
    UNUSED(p2);
    UNUSED(len);
    UNUSED(tx);

    if (buffer == NULL || len < sizeof(uint32_t)) {
        THROW(EXCEPTION_INTERNAL);
    }
    
    // Read Key Index
    if (parse_derivation_path(buffer, len, gpk_ctx.deriv_path, &gpk_ctx.key_index) != 0) {
        THROW(EXCEPTION_MALFORMED_APDU);
    }

    // If p1 != 0, silent mode, for use by apps that request the user's public
    // key frequently Only do UI actions for p1 == 0
    if (p1 == 0) {
        // Complete "Export Public | Key #x?"
        hedera_snprintf(gpk_ctx.ui_approve_l2, DISPLAY_SIZE,
#if defined(TARGET_NANOX) || defined(TARGET_NANOS2) || defined(TARGET_NANOS)
                        "Key #%u?",
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
                        "#%u",
#endif
                        gpk_ctx.key_index);
    }

    // Populate context with PK
    if (!get_pk()) {
        io_exchange_with_code(EXCEPTION_INTERNAL, 0);
    }

    if (p1 == 0) {
        ui_get_public_key();
    }
    // Skip UI interaction when called from swap context
    // Return public key and yield control to exchange handler
    else {
        io_exchange_with_code(EXCEPTION_OK, 32);
        if (!G_called_from_swap) {
            ui_idle();
        }
    }
    

    *flags |= IO_ASYNCH_REPLY;
}