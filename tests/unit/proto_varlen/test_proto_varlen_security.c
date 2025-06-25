#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

// Include the module under test
#include "proto_varlen_parser.h"

// Test helper functions to create malicious protobuf data
static void write_varint(uint8_t **buffer, uint64_t value) {
    while (value >= 0x80) {
        **buffer = (uint8_t)(value | 0x80);
        (*buffer)++;
        value >>= 7;
    }
    **buffer = (uint8_t)value;
    (*buffer)++;
}

// SECURITY TESTS - Buffer Overflows and Infinite Loops

// Test 1: Varint overflow protection (shift >= 64)
static void test_decode_varint_infinite_loop_protection(void **state) {
    (void) state;
    
    // Create a malicious varint with 10+ continuation bytes (should cause shift >= 64)
    uint8_t malicious_data[16];
    for (int i = 0; i < 15; i++) {
        malicious_data[i] = 0xFF; // All bits set, continuation bit on
    }
    malicious_data[15] = 0x7F; // Final byte
    
    const uint8_t *ptr = malicious_data;
    const uint8_t *end = malicious_data + sizeof(malicious_data);
    protobuf_field_t field;
    
    // This should fail gracefully, not infinite loop or crash
    bool result = parse_field_tag(&ptr, end, &field);
    assert_false(result);
}

// Test 2: Integer overflow in pointer arithmetic
static void test_pointer_arithmetic_overflow(void **state) {
    (void) state;
    
    uint8_t buffer[64];
    uint8_t *ptr = buffer;
    
    // Create a string field with UINT64_MAX length
    uint64_t tag = (15 << 3) | 2; // field 15, wire type string
    write_varint(&ptr, tag);
    
    // Write maximum possible length (this would cause overflow)
    uint64_t malicious_length = UINT64_MAX;
    write_varint(&ptr, malicious_length);
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should fail safely, not crash
    assert_false(result);
}

// Test 3: Pointer arithmetic overflow in nested structures
static void test_nested_pointer_overflow(void **state) {
    (void) state;
    
    uint8_t buffer[128];
    uint8_t *ptr = buffer;
    
    // Create TransactionBody field 15 (cryptoUpdateAccount)
    uint64_t outer_tag = (15 << 3) | 2;
    write_varint(&ptr, outer_tag);
    
    // Claim the inner message has SIZE_MAX length
    write_varint(&ptr, SIZE_MAX);
    
    // Add some minimal inner content
    uint64_t inner_tag = (14 << 3) | 2;
    write_varint(&ptr, inner_tag);
    write_varint(&ptr, 4);
    memcpy(ptr, "test", 4);
    ptr += 4;
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should handle overflow gracefully
    assert_false(result);
}

// Test 4: Large string length causing buffer overflow
static void test_large_string_length_overflow(void **state) {
    (void) state;
    
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    
    // Create a StringValue submessage with huge claimed length
    uint8_t string_value[64];
    uint8_t *sv_ptr = string_value;
    
    // StringValue.value field (field 1)
    uint64_t sv_tag = (1 << 3) | 2;
    write_varint(&sv_ptr, sv_tag);
    
    // Claim string is 0x7FFFFFFF bytes long (but we only have a few bytes)
    write_varint(&sv_ptr, 0x7FFFFFFF);
    
    // Add minimal actual data
    memcpy(sv_ptr, "hi", 2);
    sv_ptr += 2;
    
    size_t sv_len = sv_ptr - string_value;
    
    // Create CryptoUpdateTransactionBody
    uint8_t crypto_update[96];
    uint8_t *cu_ptr = crypto_update;
    uint64_t cu_tag = (14 << 3) | 2;
    write_varint(&cu_ptr, cu_tag);
    write_varint(&cu_ptr, sv_len);
    memcpy(cu_ptr, string_value, sv_len);
    cu_ptr += sv_len;
    
    size_t cu_len = cu_ptr - crypto_update;
    
    // Create TransactionBody
    uint64_t tb_tag = (15 << 3) | 2;
    write_varint(&ptr, tb_tag);
    write_varint(&ptr, cu_len);
    memcpy(ptr, crypto_update, cu_len);
    ptr += cu_len;
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should detect the inconsistency and fail safely
    assert_false(result);
}

// Test 5: Infinite loop in skip_field with malicious wire type
static void test_skip_field_unknown_wire_type(void **state) {
    (void) state;
    
    uint8_t buffer[32];
    uint8_t *ptr = buffer;
    
    // Create a field with unknown wire type (7, which doesn't exist)
    uint64_t malicious_tag = (10 << 3) | 7; // field 10, wire type 7 (invalid)
    write_varint(&ptr, malicious_tag);
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should fail on unknown wire type
    assert_false(result);
}

// Test 6: Zero-length buffer edge case
static void test_zero_length_buffer(void **state) {
    (void) state;
    
    uint8_t *empty_buffer = NULL;
    char output[64];
    
    bool result = extract_nested_string_field(empty_buffer, 0, 14, output, sizeof(output));
    assert_false(result);
    
    // Also test with non-NULL but zero size
    uint8_t dummy = 0;
    result = extract_nested_string_field(&dummy, 0, 14, output, sizeof(output));
    assert_false(result);
}

// Test 7: Buffer overflow via size_t to uint64_t conversion
static void test_size_conversion_overflow(void **state) {
    (void) state;
    
    uint8_t buffer[64];
    char output[4]; // Very small output buffer
    
    // Create valid small data, but the vulnerability is in output buffer size handling
    uint8_t *ptr = buffer;
    
    // StringValue submessage
    uint8_t string_value[32];
    uint8_t *sv_ptr = string_value;
    uint64_t sv_tag = (1 << 3) | 2;
    write_varint(&sv_ptr, sv_tag);
    write_varint(&sv_ptr, 20); // 20 byte string
    memcpy(sv_ptr, "very_long_test_string", 20);
    sv_ptr += 20;
    size_t sv_len = sv_ptr - string_value;
    
    // CryptoUpdate
    uint8_t crypto_update[48];
    uint8_t *cu_ptr = crypto_update;
    uint64_t cu_tag = (14 << 3) | 2;
    write_varint(&cu_ptr, cu_tag);
    write_varint(&cu_ptr, sv_len);
    memcpy(cu_ptr, string_value, sv_len);
    cu_ptr += sv_len;
    size_t cu_len = cu_ptr - crypto_update;
    
    // TransactionBody
    uint64_t tb_tag = (15 << 3) | 2;
    write_varint(&ptr, tb_tag);
    write_varint(&ptr, cu_len);
    memcpy(ptr, crypto_update, cu_len);
    ptr += cu_len;
    
    size_t buffer_len = ptr - buffer;
    
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should truncate safely without buffer overflow
    assert_true(result);
    assert_int_equal(3, strlen(output)); // Should be truncated to 3 chars + null
    assert_string_equal("ver", output);
}

// Test 8: Deeply nested malicious recursion
static void test_deeply_nested_structures(void **state) {
    (void) state;
    
    uint8_t buffer[512];
    uint8_t *ptr = buffer;
    
    // Create nested structure with many layers to test stack overflow protection
    for (int depth = 0; depth < 100; depth++) {
        uint64_t tag = (15 << 3) | 2; // Always field 15, string type
        write_varint(&ptr, tag);
        write_varint(&ptr, 10); // Claim 10 bytes for inner message
        
        if (ptr >= buffer + sizeof(buffer) - 20) break; // Prevent our own buffer overflow
    }
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 14, output, sizeof(output));
    
    // Should handle deep nesting gracefully
    assert_false(result);
}

// Test 9: Invalid field number edge cases
static void test_invalid_field_numbers(void **state) {
    (void) state;
    
    uint8_t buffer[32];
    uint8_t *ptr = buffer;
    
    // Test field number 0 (invalid in protobuf)
    uint64_t zero_field = (0 << 3) | 2;
    write_varint(&ptr, zero_field);
    write_varint(&ptr, 4);
    memcpy(ptr, "test", 4);
    ptr += 4;
    
    size_t buffer_len = ptr - buffer;
    
    char output[64];
    bool result = extract_nested_string_field(buffer, buffer_len, 0, output, sizeof(output));
    
    // Should handle field 0 appropriately
    assert_false(result);
}

// Test 10: Memory boundary edge case - end pointer manipulation
static void test_end_pointer_edge_cases(void **state) {
    (void) state;
    
    // Test with end < start (invalid pointers)
    uint8_t data[] = {0x0A, 0x04, 't', 'e', 's', 't'};
    const uint8_t *start = data + 3;
    const uint8_t *end = data; // end < start
    protobuf_field_t field;
    
    bool result = parse_field_tag(&start, end, &field);
    assert_false(result);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_decode_varint_infinite_loop_protection),
        cmocka_unit_test(test_pointer_arithmetic_overflow),
        cmocka_unit_test(test_nested_pointer_overflow),
        cmocka_unit_test(test_large_string_length_overflow),
        cmocka_unit_test(test_skip_field_unknown_wire_type),
        cmocka_unit_test(test_zero_length_buffer),
        cmocka_unit_test(test_size_conversion_overflow),
        cmocka_unit_test(test_deeply_nested_structures),
        cmocka_unit_test(test_invalid_field_numbers),
        cmocka_unit_test(test_end_pointer_edge_cases),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
} 