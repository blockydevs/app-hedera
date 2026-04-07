#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "swap/swap_token_utils.h"

#define MAX_ASSET_LEN 16
#define MAX_OUT_LEN 64
#define MAX_STR_LEN 16

static size_t min_size(size_t a, size_t b) {
    return (a < b) ? a : b;
}

static uint64_t load_u64_be(const uint8_t *data, size_t len) {
    uint64_t v = 0;
    for (size_t i = 0; i < len; i++) {
        v = (v << 8) | data[i];
    }
    return v;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size == 0) return 0;

    size_t offset = 0;
    uint8_t flags = data[offset++];

    uint8_t decimals = (offset < size) ? data[offset++] : 0;

    size_t amount_len = min_size(8, size - offset);
    uint64_t amount = load_u64_be(data + offset, amount_len);
    offset += amount_len;

    uint8_t out_len_hint = (offset < size) ? data[offset++] : 0;
    size_t out_len = out_len_hint % (MAX_OUT_LEN + 1);

    const char *asset = NULL;
    char asset_buf[MAX_ASSET_LEN + 1];
    if ((flags & 0x1) && offset < size) {
        size_t asset_len = data[offset++] % (MAX_ASSET_LEN + 1);
        size_t avail = size - offset;
        if (asset_len > avail) asset_len = avail;
        memcpy(asset_buf, data + offset, asset_len);
        asset_buf[asset_len] = '\0';
        asset = asset_buf;
        offset += asset_len;
    }

    char out_buf[MAX_OUT_LEN + 1];
    memset(out_buf, 0, sizeof(out_buf));
    (void)print_token_amount(amount, asset, decimals, out_buf, out_len);

    uint8_t str_buf[MAX_STR_LEN];
    size_t str_len = min_size(MAX_STR_LEN, size - offset);
    memset(str_buf, 0, sizeof(str_buf));
    if (str_len > 0) {
        memcpy(str_buf, data + offset, str_len);
    }

    uint64_t parsed = 0;
    size_t len_hint = data[0] % (MAX_STR_LEN + 1);
    (void)swap_str_to_u64(str_buf, len_hint, &parsed);
    (void)swap_str_to_u64(str_buf, MAX_STR_LEN, &parsed);

    return 0;
}