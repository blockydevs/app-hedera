#ifndef HEDERA_H
#define HEDERA_H

#include <stdbool.h>
#include <stdint.h>
#include "ui/app_globals.h"

bool hedera_get_pubkey(uint32_t path[static MAX_DERIV_PATH_LEN], uint8_t raw_pubkey[static RAW_PUBKEY_SIZE]);

bool hedera_sign(uint32_t path[static MAX_DERIV_PATH_LEN], const uint8_t* tx, uint8_t tx_len,
                 /* out */ uint8_t* result);

#endif // HEDERA_H