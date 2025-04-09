#ifdef HAVE_SWAP

#include "handle_swap_sign_transaction.h"
#include "os.h"
#include "string.h"
#include "swap.h"
#include "swap_utils.h"

#include <sign_transaction.h>
#include <app_globals.h>
#include <inttypes.h>

typedef struct swap_validated_s {
    bool initialized;
    uint64_t amount;
    char recipient[(18 * 2) + 1];
    uint64_t fee;
} swap_validated_t;

static swap_validated_t G_swap_validated;

// sign_tx_context_t st_ctx;

static uint8_t *G_swap_sign_return_value_address;

// Save the data validated during the Exchange app flow
bool copy_transaction_parameters(create_transaction_parameters_t *params) {
    // Ensure no subcoin configuration
    if (params->coin_configuration != NULL || params->coin_configuration_length != 0) {
        PRINTF("No coin_configuration expected\n");
        return false;
    }

    // Ensure no extraid
    if (params->destination_address_extra_id == NULL) {
        PRINTF("destination_address_extra_id expected\n");
        return false;
    } else if (params->destination_address_extra_id[0] != '\0') {
        PRINTF("destination_address_extra_id expected empty, not '%s'\n",
               params->destination_address_extra_id);
        return false;
    }

    // first copy parameters to stack, and then to global data.
    // We need this "trick" as the input data position can overlap with app globals
    swap_validated_t swap_validated;
    memset(&swap_validated, 0, sizeof(swap_validated));

    // Save recipient
    strlcpy(swap_validated.recipient,
            params->destination_address,
            sizeof(swap_validated.recipient));
    if (swap_validated.recipient[sizeof(swap_validated.recipient) - 1] != '\0') {
        PRINTF("Address copy error\n");
        return false;
    }

    // Save amount
    if (!swap_str_to_u64(params->amount, params->amount_length, &swap_validated.amount)) {
        return false;
    }
    // Save the fee
    if (!swap_str_to_u64(params->fee_amount, params->fee_amount_length, &swap_validated.fee)) {
        return false;
    }
    swap_validated.initialized = true;

    // Full reset the global variables
    os_explicit_zero_BSS_segment();

    // Keep the address at which we'll reply the signing status
    G_swap_sign_return_value_address = &params->result;

    // Commit the values read from exchange to the clean global space
    memcpy(&G_swap_validated, &swap_validated, sizeof(swap_validated));

    return true;
}

bool validate_swap_amount(uint64_t amount) {

    if (amount != G_swap_validated.amount) {
        PRINTF("Amount not equal\n");
        return false;
    }
    // NOTE: in other Nano Apps the validation is done in string type. We're keeping it as well.
    char validated_amount_str[MAX_PRINTABLE_AMOUNT_SIZE];
    if (print_token_amount(G_swap_validated.amount,HEDERA_SIGN,HEDERA_DECIMALS, validated_amount_str, sizeof(validated_amount_str)) != 0) {
        PRINTF("Conversion failed\n");
        return false;
    }
    char amount_str[MAX_PRINTABLE_AMOUNT_SIZE];
    if (print_token_amount(amount, HEDERA_SIGN, HEDERA_DECIMALS, amount_str, sizeof(amount_str)) != 0) {
        PRINTF("Conversion failed\n");
        return false;
    }

    if (strcmp(amount_str, validated_amount_str) != 0) {
        PRINTF("Amount requested in this transaction = %s\n", amount_str);
        PRINTF("Amount validated in swap = %s\n", validated_amount_str);
        return false;
    }
    return true;
}

bool swap_check_validity() {
    PRINTF("swap_validated.recipient: %s\n", G_swap_validated.recipient);
    PRINTF("st_ctx.recipients: %s\n", st_ctx.recipients);
    PRINTF("Inside swap_check_validity\n");
    if (!G_swap_validated.initialized) {
        PRINTF("Swap Validated data not initialized.\n");
        return false;
    }

    // Validate amount
    if (!validate_swap_amount(st_ctx.transaction.data.cryptoTransfer.transfers
                        .accountAmounts[0]
                        .amount)) {
        PRINTF("Amount on Transaction is different from validated package.\n");
        return false;
    }

    // Validate fee
    if (st_ctx.transaction.transactionFee != G_swap_validated.fee) {
        PRINTF("Gas fee on Transaction is different from validated package.\n");
        return false;
    }

    // Validate recipient
    if (memcmp(st_ctx.recipients, G_swap_validated.recipient, (18 * 2) + 1) != 0) {
        PRINTF("Recipient on Transaction is different from validated package.\n");
        PRINTF("Recipient requested in the transaction: %.*H\n", FULL_ADDRESS_LENGTH, st_ctx.recipients);
        PRINTF("Recipient validated in the swap: %.*H\n", FULL_ADDRESS_LENGTH, G_swap_validated.recipient);
        return false;
    }

    // Validate recipient
    return true;
}


void __attribute__((noreturn)) finalize_exchange_sign_transaction(bool is_success) {
    PRINTF("Returning to Exchange with status %d\n", is_success);
    *G_swap_sign_return_value_address = is_success;
    os_lib_end();
}

#endif