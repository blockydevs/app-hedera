#include "staking.h"

#include <stddef.h>

#include "crypto_update.pb.h"

bool is_ledger_account(const Hedera_AccountID *account_id) {
    if (account_id == NULL) {
        return false;
    }

    return (account_id->shardNum == LEDGER_ACCOUNT_SHARD &&
            account_id->realmNum == LEDGER_ACCOUNT_REALM &&
            account_id->account.accountNum == LEDGER_ACCOUNT_NUM);
}

update_type_t identify_special_update(
    const Hedera_CryptoUpdateTransactionBody *update_body) {
    PRINTF("Staking: entry \n");

    if (update_body == NULL) {
        PRINTF("Staking: NULL check failed\n");
        return GENERIC_UPDATE;
    }

    // Check if any non-staking fields are set
    // If any of these are true, then non-staking fields are being modified

    // Key update (not allowed for staking-only)
    if (update_body->has_key) {
        PRINTF("Staking: has key\n");
        return GENERIC_UPDATE;
    }

    // Proxy account (deprecated but still checked)
    if (update_body->has_proxyAccountID) {
        PRINTF("Staking: has proxy account\n");
        return GENERIC_UPDATE;
    }

    // Proxy fraction (deprecated but still checked)
    if (update_body->proxyFraction != 0) {
        PRINTF("Staking: proxy fraction not zero\n");
        return GENERIC_UPDATE;
    }

    // Send record threshold fields (deprecated)
    if (update_body->which_sendRecordThresholdField != 0) {
        PRINTF("Staking: has send record threshold\n");
        return GENERIC_UPDATE;
    }

    // Receive record threshold fields (deprecated)
    if (update_body->which_receiveRecordThresholdField != 0) {
        PRINTF("Staking: has receive record threshold\n");
        return GENERIC_UPDATE;
    }

    // Auto renew period
    if (update_body->has_autoRenewPeriod) {
        PRINTF("Staking: has auto renew period\n");
        return GENERIC_UPDATE;
    }

    // Expiration time
    if (update_body->has_expirationTime) {
        PRINTF("Staking: has expiration time\n");
        return GENERIC_UPDATE;
    }

    // Receiver signature required
    PRINTF("Staking: receiverSigRequiredField = %d\n",
           update_body->which_receiverSigRequiredField);
    if (update_body->which_receiverSigRequiredField != 0) {
        PRINTF("Staking: has receiver sig required\n");
        return GENERIC_UPDATE;
    }

    PRINTF("Staking: pre memo \n");

    // Account memo
    if (update_body->has_memo) {
        return GENERIC_UPDATE;
    }

    PRINTF("Staking: post memo \n");

    // Max automatic token associations
    if (update_body->has_max_automatic_token_associations) {
        return GENERIC_UPDATE;
    }

    // At this point, only staking fields can be set
    // Check what type of staking operation this is
    bool has_staking_target = false;
    bool is_unstake = false;

    PRINTF("Staking: post if block \n");

    // Check staking target (account or node)
    if (update_body->which_staked_id != 0) {
        PRINTF("Staking: has staked ID\n");
        has_staking_target = true;

        if (update_body->which_staked_id ==
            Hedera_CryptoUpdateTransactionBody_staked_account_id_tag) {
            PRINTF("Staking: has staked account ID\n");
            if (update_body->staked_id.staked_account_id.shardNum == 0 &&
                update_body->staked_id.staked_account_id.realmNum == 0 &&
                update_body->staked_id.staked_account_id.account.accountNum ==
                    0) {
                PRINTF("Staking: unstaking account\n");
                is_unstake = true;
            }
        } else if (update_body->which_staked_id ==
                   Hedera_CryptoUpdateTransactionBody_staked_node_id_tag) {
            PRINTF("Staking: has staked node ID\n");
            // Staking to a node - check if it's -1 (unstaking)
            if (update_body->staked_id.staked_node_id == -1) {
                PRINTF("Staking: unstaking node\n");
                is_unstake = true;
            }
        }
    }

    // Check decline rewards setting
    bool has_decline_reward = update_body->has_decline_reward;

    // Determine the update type based on what fields are set
    if (!has_staking_target && !has_decline_reward) {
        // No staking fields are being modified
        return GENERIC_UPDATE;
    }

    if (is_unstake) {
        return UNSTAKE_UPDATE;
    }

    return STAKE_UPDATE;
}
