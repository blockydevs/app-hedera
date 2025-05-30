#include "hedera.h"

#include <cx.h>
#include <os.h>
#include <string.h>

#include "crypto_helpers.h"
#include "get_public_key.h"
#include "utils.h"

bool hedera_get_pubkey(uint32_t path[static MAX_DERIV_PATH_LEN], uint8_t raw_pubkey[static RAW_PUBKEY_SIZE]) {

    if (CX_OK != bip32_derive_with_seed_get_pubkey_256(
                     HDW_ED25519_SLIP10, CX_CURVE_Ed25519, path, MAX_DERIV_PATH_LEN, raw_pubkey,
                     NULL, CX_SHA512, NULL, 0)) {
        return false;
    }

    return true;
}

bool hedera_sign(uint32_t path[static MAX_DERIV_PATH_LEN], const uint8_t* tx, uint8_t tx_len,
                 /* out */ uint8_t* result) {
    size_t sig_len = 64;

    if (CX_OK != bip32_derive_with_seed_eddsa_sign_hash_256(
                     HDW_ED25519_SLIP10, CX_CURVE_Ed25519, path, MAX_DERIV_PATH_LEN, CX_SHA512,
                     tx,     // hash (really message)
                     tx_len, // hash length (really message length)
                     result, // signature
                     &sig_len, NULL, 0)) {
        return false;
    }

    return true;
}
