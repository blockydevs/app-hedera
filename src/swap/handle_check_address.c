#ifdef HAVE_SWAP

#include "handle_check_address.h"
#include "../get_public_key.h"
#include <ctype.h>
#include <string.h>


static void derive_public_key(uint32_t path[static MAX_DERIV_PATH_LEN], uint8_t public_key[RAW_PUBKEY_SIZE],
                              uint8_t public_key_str[RAW_PUBKEY_SIZE]) {
    hedera_get_pubkey(path, public_key);
 

    public_key_to_bytes(G_io_apdu_buffer, public_key);
    bin2hex(public_key_str, G_io_apdu_buffer, KEY_SIZE);
    public_key_str[KEY_SIZE] = '\0';
}

int handle_check_address(const check_address_parameters_t *params) {
    if (params->coin_configuration != NULL || params->coin_configuration_length != 0) {
        PRINTF("No coin_configuration expected\n");
        return 0;
    }

    if (params->address_parameters == NULL || params->address_parameters_length < 4) {
        PRINTF("Derivation path expected\n");
        return 0;
    }

    if (params->address_to_check == NULL) {
        PRINTF("Address to check expected\n");
        return 0;
    }

    if (params->extra_id_to_check == NULL) {
        PRINTF("extra_id_to_check expected\n");
        return 0;
    }
    if (params->extra_id_to_check[0] != '\0') {
        PRINTF("extra_id_to_check expected empty, not '%s'\n",
               params->extra_id_to_check);
        return 0;
    }

    uint8_t public_key[RAW_PUBKEY_SIZE];
    uint8_t public_key_str[RAW_PUBKEY_SIZE];
    
    // Derive public key
    uint32_t path[MAX_DERIV_PATH_LEN];
    if (get_key_index_from_buffer(params->address_parameters, params->address_parameters_length, path, NULL) != 0) {
        return 0;
    }
    derive_public_key(path, public_key, public_key_str);

    UNUSED(public_key);

    uint8_t offset_0x = 0;
    if (memcmp(params->address_to_check, "0x", 2) == 0) {
        offset_0x = 2;
    }

    if (strcmp(params->address_to_check + offset_0x, (char *)public_key_str) !=
        0) {
        PRINTF("Address %s != %s\n", params->address_to_check + offset_0x,
               public_key_str);
        return 0;
    }

    PRINTF("Addresses match\n");
    return 1;
}

#endif
