#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdint.h>

// Include the module under test
#include "proto_varlen_parser.h"

// Test helper functions to create protobuf data
static void write_varint(uint8_t **buffer, uint64_t value) {
    while (value >= 0x80) {
        **buffer = (uint8_t)(value | 0x80);
        (*buffer)++;
        value >>= 7;
    }
    **buffer = (uint8_t)value;
    (*buffer)++;
}

static void write_string_field(uint8_t **buffer, uint32_t field_number, const char *str) {
    // Write tag (field_number << 3 | WIRE_TYPE_STRING)
    uint64_t tag = (field_number << 3) | 2;
    write_varint(buffer, tag);
    
    // Write string length
    size_t len = strlen(str);
    write_varint(buffer, len);
    
    // Write string data
    memcpy(*buffer, str, len);
    *buffer += len;
}

static void write_submessage_field(uint8_t **buffer, uint32_t field_number, 
                                   const uint8_t *submessage, size_t submessage_len) {
    // Write tag (field_number << 3 | WIRE_TYPE_STRING)
    uint64_t tag = (field_number << 3) | 2;
    write_varint(buffer, tag);
    
    // Write submessage length
    write_varint(buffer, submessage_len);
    
    // Write submessage data
    memcpy(*buffer, submessage, submessage_len);
    *buffer += submessage_len;
}

// Test cases for parse_field_tag

static void test_parse_field_tag_valid(void **state) {
    (void) state; // unused
    
    uint8_t data[] = {0x0A}; // field 1, wire type 2 (string)
    const uint8_t *ptr = data;
    const uint8_t *end = data + sizeof(data);
    protobuf_field_t field;
    
    bool result = parse_field_tag(&ptr, end, &field);
    
    assert_true(result);
    assert_int_equal(1, field.field_number);
    assert_int_equal(WIRE_TYPE_STRING, field.wire_type);
    assert_ptr_equal(ptr, field.data);
}

static void test_parse_field_tag_invalid_data(void **state) {
    (void) state; // unused
    
    const uint8_t *ptr = NULL;
    const uint8_t *end = (const uint8_t *)1;
    protobuf_field_t field;
    
    bool result = parse_field_tag(&ptr, end, &field);
    
    assert_false(result);
}

static void test_parse_field_tag_insufficient_data(void **state) {
    (void) state; // unused
    
    uint8_t data[] = {0x80}; // incomplete varint
    const uint8_t *ptr = data;
    const uint8_t *end = data + sizeof(data);
    protobuf_field_t field;
    
    bool result = parse_field_tag(&ptr, end, &field);
    
    assert_false(result);
}

static void test_parse_field_tag_large_field_number(void **state) {
    (void) state; // unused
    
    uint8_t data[] = {0xF8, 0x0F}; // field 255, wire type 0 (varint)
    const uint8_t *ptr = data;
    const uint8_t *end = data + sizeof(data);
    protobuf_field_t field;
    
    bool result = parse_field_tag(&ptr, end, &field);
    
    assert_true(result);
    assert_int_equal(255, field.field_number);
    assert_int_equal(WIRE_TYPE_VARINT, field.wire_type);
}

// Test cases for extract_nested_string_field

static void test_extract_nested_string_field_valid(void **state) {
    (void) state; // unused
    
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    
    // Create a StringValue submessage
    uint8_t string_value[32];
    uint8_t *sv_ptr = string_value;
    write_string_field(&sv_ptr, 1, "test_memo"); // StringValue.value field
    size_t sv_len = sv_ptr - string_value;
    
    // Create CryptoUpdateTransactionBody submessage
    uint8_t crypto_update[64];
    uint8_t *cu_ptr = crypto_update;
    write_submessage_field(&cu_ptr, 14, string_value, sv_len); // memo field
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody with cryptoUpdateAccount field
    write_submessage_field(&ptr, 15, crypto_update, cu_len); // cryptoUpdateAccount field
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_true(result);
    assert_string_equal("test_memo", output);
}

static void test_extract_nested_string_field_field_not_found(void **state) {
    (void) state; // unused
    
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    
    // Create a StringValue submessage
    uint8_t string_value[32];
    uint8_t *sv_ptr = string_value;
    write_string_field(&sv_ptr, 1, "test_memo");
    size_t sv_len = sv_ptr - string_value;
    
    // Create CryptoUpdateTransactionBody with field 13 instead of 14
    uint8_t crypto_update[64];
    uint8_t *cu_ptr = crypto_update;
    write_submessage_field(&cu_ptr, 13, string_value, sv_len);
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody
    write_submessage_field(&ptr, 15, crypto_update, cu_len);
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_false(result);
}

static void test_extract_nested_string_field_invalid_input(void **state) {
    (void) state; // unused
    
    char output[64];
    
    // Test with NULL buffer
    bool result1 = extract_nested_string_field(NULL, 10, 14, output, sizeof(output));
    assert_false(result1);
    
    // Test with NULL output
    uint8_t buffer[10] = {0};
    bool result2 = extract_nested_string_field(buffer, sizeof(buffer), 14, NULL, sizeof(output));
    assert_false(result2);
    
    // Test with zero output size
    bool result3 = extract_nested_string_field(buffer, sizeof(buffer), 14, output, 0);
    assert_false(result3);
}

static void test_extract_nested_string_field_truncated_string(void **state) {
    (void) state; // unused
    
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    
    // Create a StringValue submessage
    uint8_t string_value[64];
    uint8_t *sv_ptr = string_value;
    write_string_field(&sv_ptr, 1, "very_long_test_memo_string");
    size_t sv_len = sv_ptr - string_value;
    
    // Create CryptoUpdateTransactionBody submessage
    uint8_t crypto_update[96];
    uint8_t *cu_ptr = crypto_update;
    write_submessage_field(&cu_ptr, 14, string_value, sv_len);
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody
    write_submessage_field(&ptr, 15, crypto_update, cu_len);
    
    size_t buffer_len = ptr - buffer;
    
    char output[10]; // Small buffer to test truncation
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_true(result);
    assert_int_equal(9, strlen(output)); // Should be truncated to 9 chars + null terminator
    assert_string_equal("very_long", output);
}

static void test_extract_nested_string_field_no_crypto_update(void **state) {
    (void) state; // unused
    
    uint8_t buffer[32];
    uint8_t *ptr = buffer;
    
    // Create TransactionBody with different field (not 15)
    write_string_field(&ptr, 1, "some_other_field");
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_false(result);
}

static void test_extract_nested_string_field_empty_string(void **state) {
    (void) state; // unused
    
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    
    // Create a StringValue submessage with empty string
    uint8_t string_value[16];
    uint8_t *sv_ptr = string_value;
    write_string_field(&sv_ptr, 1, ""); // Empty string
    size_t sv_len = sv_ptr - string_value;
    
    // Create CryptoUpdateTransactionBody submessage
    uint8_t crypto_update[32];
    uint8_t *cu_ptr = crypto_update;
    write_submessage_field(&cu_ptr, 14, string_value, sv_len);
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody
    write_submessage_field(&ptr, 15, crypto_update, cu_len);
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_true(result);
    assert_string_equal("", output);
}

static void test_extract_nested_string_field_multiple_fields(void **state) {
    (void) state; // unused
    
    uint8_t buffer[512];
    uint8_t *ptr = buffer;
    
    // Create multiple StringValue submessages
    uint8_t string_value1[32];
    uint8_t *sv_ptr1 = string_value1;
    write_string_field(&sv_ptr1, 1, "first_memo");
    size_t sv_len1 = sv_ptr1 - string_value1;
    
    uint8_t string_value2[32];
    uint8_t *sv_ptr2 = string_value2;
    write_string_field(&sv_ptr2, 1, "second_memo");
    size_t sv_len2 = sv_ptr2 - string_value2;
    
    // Create CryptoUpdateTransactionBody submessage with multiple fields
    uint8_t crypto_update[128];
    uint8_t *cu_ptr = crypto_update;
    write_submessage_field(&cu_ptr, 13, string_value1, sv_len1); // account_memo field
    write_submessage_field(&cu_ptr, 14, string_value2, sv_len2); // memo field
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody
    write_submessage_field(&ptr, 15, crypto_update, cu_len);
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    assert_true(result);
    assert_string_equal("second_memo", output);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        // parse_field_tag tests
        cmocka_unit_test(test_parse_field_tag_valid),
        cmocka_unit_test(test_parse_field_tag_invalid_data),
        cmocka_unit_test(test_parse_field_tag_insufficient_data),
        cmocka_unit_test(test_parse_field_tag_large_field_number),
        
        // extract_nested_string_field tests
        cmocka_unit_test(test_extract_nested_string_field_valid),
        cmocka_unit_test(test_extract_nested_string_field_field_not_found),
        cmocka_unit_test(test_extract_nested_string_field_invalid_input),
        cmocka_unit_test(test_extract_nested_string_field_truncated_string),
        cmocka_unit_test(test_extract_nested_string_field_no_crypto_update),
        cmocka_unit_test(test_extract_nested_string_field_empty_string),
        cmocka_unit_test(test_extract_nested_string_field_multiple_fields),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
} 