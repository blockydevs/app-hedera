#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdint.h>

// Include the module under test
#include "proto_varlen_parser.h"

// EDGE CASE TESTS - Empty data, single bytes, pointer manipulation

// Test 1: **data is NULL
static void test_data_pointer_null(void **state) {
    (void) state;
    
    const uint8_t **data = NULL;
    uint8_t dummy_end = 0;
    const uint8_t *end = &dummy_end;
    protobuf_field_t field;
    
    bool result = parse_field_tag(data, end, &field);
    assert_false(result);
}

// Test 2: *data is NULL
static void test_data_content_null(void **state) {
    (void) state;
    
    const uint8_t *data_ptr = NULL;
    const uint8_t **data = &data_ptr;
    uint8_t dummy_end = 0;
    const uint8_t *end = &dummy_end;
    protobuf_field_t field;
    
    bool result = parse_field_tag(data, end, &field);
    assert_false(result);
}

// Test 3: Empty buffer (data == end)
static void test_empty_buffer_exact(void **state) {
    (void) state;
    
    uint8_t buffer[1] = {0x00};
    const uint8_t *data = buffer;
    const uint8_t *end = buffer; // data == end (empty)
    protobuf_field_t field;
    
    bool result = parse_field_tag(&data, end, &field);
    assert_false(result);
}

// Test 4: Single byte - valid varint
static void test_single_byte_valid_varint(void **state) {
    (void) state;
    
    uint8_t buffer[1] = {0x08}; // field 1, wire type 0 (varint)
    const uint8_t *data = buffer;
    const uint8_t *end = buffer + 1;
    protobuf_field_t field;
    
    bool result = parse_field_tag(&data, end, &field);
    assert_true(result);
    assert_int_equal(1, field.field_number);
    assert_int_equal(WIRE_TYPE_VARINT, field.wire_type);
}

// Test 5: Single byte - incomplete varint (continuation bit set)
static void test_single_byte_incomplete_varint(void **state) {
    (void) state;
    
    uint8_t buffer[1] = {0x88}; // field 1, wire type 0, but continuation bit set
    const uint8_t *data = buffer;
    const uint8_t *end = buffer + 1;
    protobuf_field_t field;
    
    bool result = parse_field_tag(&data, end, &field);
    assert_false(result); // Should fail due to incomplete varint
}

// Test 6: Two bytes - minimal complete field
static void test_two_bytes_minimal_field(void **state) {
    (void) state;
    
    uint8_t buffer[2] = {0x0A, 0x00}; // field 1, wire type 2 (string), length 0
    const uint8_t *data = buffer;
    const uint8_t *end = buffer + 2;
    protobuf_field_t field;
    
    bool result = parse_field_tag(&data, end, &field);
    assert_true(result);
    assert_int_equal(1, field.field_number);
    assert_int_equal(WIRE_TYPE_STRING, field.wire_type);
    assert_ptr_equal(buffer + 1, field.data); // Should point to length byte
}

// Test 7: Extract from completely empty input buffer
static void test_extract_from_empty_input(void **state) {
    (void) state;
    
    char output[64];
    bool result = extract_nested_string_field(NULL, 0, 14, output, sizeof(output));
    assert_false(result);
    
    // Also test with valid pointer but zero size
    uint8_t dummy = 0;
    result = extract_nested_string_field(&dummy, 0, 14, output, sizeof(output));
    assert_false(result);
}

// Test 8: Extract with single byte input (incomplete)
static void test_extract_single_byte_input(void **state) {
    (void) state;
    
    uint8_t buffer[1] = {0x78}; // field 15, wire type 0 - but incomplete
    char output[64];
    
    bool result = extract_nested_string_field(buffer, 1, 14, output, sizeof(output));
    assert_false(result);
}

// Test 9: Extract with two bytes (minimal but still incomplete)
static void test_extract_two_byte_input(void **state) {
    (void) state;
    
    uint8_t buffer[2] = {0x7A, 0x01}; // field 15, wire type 2, length 1 - but no data
    char output[64];
    
    bool result = extract_nested_string_field(buffer, 2, 14, output, sizeof(output));
    assert_false(result); // Should fail - not enough data for claimed length
}

// Test 10: Extract with output buffer size 1 (edge case for string handling)
static void test_extract_output_size_one(void **state) {
    (void) state;
    
    uint8_t buffer[32];
    uint8_t *ptr = buffer;
    
    // Create minimal valid structure
    // TransactionBody field 15
    *ptr++ = 0x7A; // field 15, wire type 2
    *ptr++ = 0x06; // length 6
    
    // CryptoUpdateTransactionBody field 14
    *ptr++ = 0x72; // field 14, wire type 2
    *ptr++ = 0x04; // length 4
    
    // StringValue field 1
    *ptr++ = 0x0A; // field 1, wire type 2
    *ptr++ = 0x02; // length 2
    *ptr++ = 'h';
    *ptr++ = 'i';
    
    char output[1]; // Only space for null terminator
    bool result = extract_nested_string_field(buffer, ptr - buffer, 14, output, sizeof(output));
    
    assert_true(result);
    assert_string_equal("", output); // Should be empty due to truncation
}

// Test 11: Pointer advancing beyond buffer bounds
static void test_pointer_advancement_beyond_bounds(void **state) {
    (void) state;
    
    uint8_t buffer[4] = {0x7A, 0xFF, 0xFF, 0xFF}; // field 15, huge length
    char output[64];
    
    bool result = extract_nested_string_field(buffer, 4, 14, output, sizeof(output));
    assert_false(result); // Should detect buffer overflow
}

// Test 12: Field number extraction from single byte edge cases
static void test_field_number_edge_cases(void **state) {
    (void) state;
    
    // Test field 0 (invalid)
    uint8_t buffer1[1] = {0x00}; // field 0, wire type 0
    const uint8_t *data1 = buffer1;
    const uint8_t *end1 = buffer1 + 1;
    protobuf_field_t field1;
    
    bool result1 = parse_field_tag(&data1, end1, &field1);
    assert_true(result1); // Should parse but field number will be 0
    assert_int_equal(0, field1.field_number);
    
    // Test maximum single-byte field number (field 15, not 31)
    // 0xF8 = 11111000 binary = field 31, wire type 0
    // But let's test a more reasonable field number that fits in protobuf encoding
    uint8_t buffer2[1] = {0x78}; // field 15, wire type 0 (01111000 binary)
    const uint8_t *data2 = buffer2;
    const uint8_t *end2 = buffer2 + 1;
    protobuf_field_t field2;
    
    bool result2 = parse_field_tag(&data2, end2, &field2);
    assert_true(result2);
    assert_int_equal(15, field2.field_number);
    assert_int_equal(WIRE_TYPE_VARINT, field2.wire_type);
}

// Test 13: Wire type extraction edge cases
static void test_wire_type_edge_cases(void **state) {
    (void) state;
    
    // Test all valid wire types in single byte
    uint8_t wire_types[] = {0x00, 0x01, 0x02, 0x05}; // VARINT, 64BIT, STRING, 32BIT
    wire_type_t expected[] = {WIRE_TYPE_VARINT, WIRE_TYPE_64BIT, WIRE_TYPE_STRING, WIRE_TYPE_32BIT};
    
    for (int i = 0; i < 4; i++) {
        uint8_t buffer[1] = {wire_types[i]}; // field 0, different wire types
        const uint8_t *data = buffer;
        const uint8_t *end = buffer + 1;
        protobuf_field_t field;
        
        bool result = parse_field_tag(&data, end, &field);
        assert_true(result);
        assert_int_equal(expected[i], field.wire_type);
    }
}

// Test 14: Memory alignment edge cases
static void test_memory_alignment_edge_cases(void **state) {
    (void) state;
    
    // Create unaligned buffer
    uint8_t large_buffer[1024];
    uint8_t *unaligned_ptr = large_buffer + 1; // Force unalignment
    
    unaligned_ptr[0] = 0x0A; // field 1, wire type 2
    
    const uint8_t *data = unaligned_ptr;
    const uint8_t *end = unaligned_ptr + 1;
    protobuf_field_t field;
    
    bool result = parse_field_tag(&data, end, &field);
    assert_true(result);
    assert_int_equal(1, field.field_number);
}

// Test 15: Stress test with minimal valid input
static void test_minimal_valid_nested_extraction(void **state) {
    (void) state;
    
    uint8_t buffer[16];
    uint8_t *ptr = buffer;
    
    // Absolute minimal valid nested structure
    *ptr++ = 0x7A; // field 15, wire type 2
    *ptr++ = 0x08; // length 8
    
    *ptr++ = 0x72; // field 14, wire type 2
    *ptr++ = 0x06; // length 6
    
    *ptr++ = 0x0A; // field 1, wire type 2
    *ptr++ = 0x04; // length 4
    *ptr++ = 't';
    *ptr++ = 'e';
    *ptr++ = 's';
    *ptr++ = 't';
    
    char output[64];
    bool result = extract_nested_string_field(buffer, ptr - buffer, 14, output, sizeof(output));
    
    assert_true(result);
    assert_string_equal("test", output);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_data_pointer_null),
        cmocka_unit_test(test_data_content_null),
        cmocka_unit_test(test_empty_buffer_exact),
        cmocka_unit_test(test_single_byte_valid_varint),
        cmocka_unit_test(test_single_byte_incomplete_varint),
        cmocka_unit_test(test_two_bytes_minimal_field),
        cmocka_unit_test(test_extract_from_empty_input),
        cmocka_unit_test(test_extract_single_byte_input),
        cmocka_unit_test(test_extract_two_byte_input),
        cmocka_unit_test(test_extract_output_size_one),
        cmocka_unit_test(test_pointer_advancement_beyond_bounds),
        cmocka_unit_test(test_field_number_edge_cases),
        cmocka_unit_test(test_wire_type_edge_cases),
        cmocka_unit_test(test_memory_alignment_edge_cases),
        cmocka_unit_test(test_minimal_valid_nested_extraction),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
} 