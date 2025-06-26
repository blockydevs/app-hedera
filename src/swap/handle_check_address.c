#ifdef HAVE_SWAP

#include "handle_check_address.h"
#include "../get_public_key.h"
#include <string.h>

#define PUBLIC_KEY_BYTES_LENGTH 32

static void derive_public_key(uint32_t index, uint8_t public_key[RAW_PUBKEY_SIZE],
                              uint8_t public_key_bytes[PUBLIC_KEY_BYTES_LENGTH]) {
    hedera_get_pubkey(index, public_key);
    public_key_to_bytes(public_key_bytes, public_key);
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
    if (params->address_parameters_length < 5 || params->address_parameters_length > 21  || (params->address_parameters_length-1) % 4 != 0) {
        PRINTF("Invalid deriv path length: %d\n", params->address_parameters_length);
        return 0;
    }
    
    uint8_t public_key[RAW_PUBKEY_SIZE];
    uint8_t public_key_bytes[PUBLIC_KEY_BYTES_LENGTH];
    
    // Read Key Index (last 4 bytes of buffer)
    // The key index is the last 4 bytes of the buffer
    // It will work for both sending only index and full path
    uint32_t index = U4BE(params->address_parameters, params->address_parameters_length - 4);

    // Handle case for new app-exchange with old account on Ledger Live
    // Where for every account we have m/44/3030 path without index, so we assume index=0
    if (params->address_parameters_length == 9) {
        index = 0;
    }
    derive_public_key(index, public_key, public_key_bytes);

    UNUSED(public_key);

    if (memcmp(params->address_to_check, public_key_bytes, PUBLIC_KEY_BYTES_LENGTH) != 0) {
        // Convert to hex for debug output
        uint8_t address_hex[KEY_SIZE * 2 + 1];
        uint8_t pubkey_hex[KEY_SIZE * 2 + 1];
        
        bin2hex(address_hex, (uint8_t*)params->address_to_check, PUBLIC_KEY_BYTES_LENGTH);
        bin2hex(pubkey_hex, public_key_bytes, PUBLIC_KEY_BYTES_LENGTH);
        
        address_hex[KEY_SIZE * 2] = '\0';
        pubkey_hex[KEY_SIZE * 2] = '\0';
        
        PRINTF("Address %s != %s\n", address_hex, pubkey_hex);
        return 0;
    }

    PRINTF("Addresses match\n");
    return 1;
}

#endif
