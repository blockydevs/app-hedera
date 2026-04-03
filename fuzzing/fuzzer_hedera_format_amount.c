#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "hedera_format_amount.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!data || size == 0) return 0;

    uint64_t amount = 0;
    uint8_t decimals = 0;

    if (size >= 8) memcpy(&amount, data, 8);
    if (size >= 9) decimals = data[8];

    (void)hedera_format_amount(amount, decimals);
    (void)hedera_format_amount(amount, (uint8_t)(decimals % 32));

    return 0;
}
