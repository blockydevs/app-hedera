#pragma once

#include <stdbool.h>
#include "crypto_update.pb.h"

typedef enum {
    GENERIC_UPDATE = 0,
    STAKE_UPDATE = 1,
    UNSTAKE_UPDATE = 2
} update_type_t;

/**
 * Check if a crypto update transaction only modifies staking fields
 * (decline_reward, staked_account_id, or staked_node_id)
 * 
 * @param update_body Pointer to the crypto update transaction body
 * @return update_type_t enum: GENERIC_UPDATE, STAKE_UPDATE, or UNSTAKE_UPDATE
 */
update_type_t identify_special_update(const struct _Hedera_CryptoUpdateTransactionBody *update_body);
