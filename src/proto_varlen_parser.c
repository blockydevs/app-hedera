#include "proto_varlen_parser.h"
#include <string.h>

// Forward declaration
static bool skip_field(const uint8_t **data, const uint8_t *end, uint32_t wire_type);

// Decode a varint from protobuf data (second-stage decoding)
static bool decode_varint(const uint8_t **data, const uint8_t *end, uint64_t *result) {
    
    uint64_t value = 0;
    uint8_t shift = 0;
    uint8_t byte;
    
    do {
        if (*data >= end) {
            return false;
        }
        if (shift >= 64) {
            return false; // Varint overflow
        }
        
        byte = **data;
        (*data)++;
        
        value |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    *result = value;
    return true;
}

bool parse_field_tag(const uint8_t **data, const uint8_t *end, protobuf_field_t *field) {
    if (!data || !*data || !end || !field) {
        return false;
    }
    
    uint64_t tag;
    if (!decode_varint(data, end, &tag)) {
        return false;
    }
    
    field->field_number = (uint32_t)(tag >> 3);
    field->wire_type = (wire_type_t)(tag & 0x07);
    field->data = *data;
    field->length = 0;
    
    return true;
}

// Second-stage protobuf decoder for nested StringValue fields
bool extract_nested_string_field(const uint8_t *buffer, size_t buffer_size, 
                                  uint32_t field_number, char *output, size_t output_size) {
    if (!buffer || !output || output_size == 0) {
        return false;
    }
    
    const uint8_t *data = buffer;
    const uint8_t *end = buffer + buffer_size;
    
    // Clear output buffer
    memset(output, 0, output_size);
    
    // First stage: find field 15 (cryptoUpdateAccount) in the TransactionBody
    while (data < end) {
        // Decode field tag (contains field number and wire type)
        uint64_t tag;
        if (!decode_varint(&data, end, &tag)) {
            return false;
        }
        
        uint32_t field_num = (uint32_t)(tag >> 3);
        uint32_t wire_type = (uint32_t)(tag & 7);
        
        if (field_num == 15) { // cryptoUpdateAccount field
            if (wire_type != WIRE_TYPE_STRING) {
                return false;
            }
            
            // Decode cryptoUpdateAccount submessage length
            uint64_t crypto_update_length;
            if (!decode_varint(&data, end, &crypto_update_length)) {
                return false;
            }
            
            // Check bounds
            if (data + crypto_update_length > end) {
                return false;
            }
            
            // Second stage: parse the CryptoUpdateTransactionBody submessage to find target field
            const uint8_t *crypto_data = data;
            const uint8_t *crypto_end = data + crypto_update_length;
            
            while (crypto_data < crypto_end) {
                // Decode field tag within CryptoUpdateTransactionBody
                uint64_t crypto_tag;
                if (!decode_varint(&crypto_data, crypto_end, &crypto_tag)) {
                    return false;
            }
            
                uint32_t crypto_field_num = (uint32_t)(crypto_tag >> 3);
                uint32_t crypto_wire_type = (uint32_t)(crypto_tag & 7);
                
                if (crypto_field_num == field_number) { // Found the target field
                    if (crypto_wire_type != WIRE_TYPE_STRING) {
                        return false;
                    }
                    
                    // Decode StringValue submessage length
                    uint64_t string_value_length;
                    if (!decode_varint(&crypto_data, crypto_end, &string_value_length)) {
                        return false;
                    }
                    
                    // Check bounds
                    if (crypto_data + string_value_length > crypto_end) {
                        return false;
                    }
                    
                    // Third stage: parse the StringValue submessage to find field 1 (the actual string)
                    const uint8_t *sv_data = crypto_data;
                    const uint8_t *sv_end = crypto_data + string_value_length;
                    
                    while (sv_data < sv_end) {
                        // Decode field tag within StringValue
                        uint64_t sv_tag;
                        if (!decode_varint(&sv_data, sv_end, &sv_tag)) {
                            return false;
                        }
                        
                        uint32_t sv_field_num = (uint32_t)(sv_tag >> 3);
                        uint32_t sv_wire_type = (uint32_t)(sv_tag & 7);
                        
                        if (sv_field_num == 1) { // StringValue.value field
                            if (sv_wire_type != WIRE_TYPE_STRING) {
                                return false;
                            }
                            
                            // Decode string length
                            uint64_t string_length;
                            if (!decode_varint(&sv_data, sv_end, &string_length)) {
                                return false;
                            }
                            
                            // Check bounds
                            if (sv_data + string_length > sv_end) {
                                return false;
                            }
                            
                            // Extract string data, stopping at null char or max length
                            size_t copy_len = (size_t)string_length;
                            if (copy_len >= output_size) {
                                copy_len = output_size - 1;
                            }
                            
                            // Find null terminator within the string data
                            for (size_t i = 0; i < copy_len; i++) {
                                if (sv_data[i] == '\0') {
                                    copy_len = i;
                    break;
                }
                            }
                            
                            memcpy(output, sv_data, copy_len);
                            output[copy_len] = '\0';
                            return true;
                        } else {
                            // Skip other fields in StringValue
                            if (!skip_field(&sv_data, sv_end, sv_wire_type)) {
                                return false;
                            }
                        }
                    }
                    
                    // Target field found but no string value inside
                    return false;
                } else {
                    // Skip other fields in CryptoUpdateTransactionBody
                    if (!skip_field(&crypto_data, crypto_end, crypto_wire_type)) {
                        return false;
                    }
                }
            }
            
            // cryptoUpdateAccount found but target field not inside
            return false;
        } else {
            // Skip field based on wire type
            if (!skip_field(&data, end, wire_type)) {
                    return false;
            }
        }
    }
    
    return false; // Field not found
}

// Helper function to skip a field based on wire type during second-stage decoding
static bool skip_field(const uint8_t **data, const uint8_t *end, uint32_t wire_type) {
    switch (wire_type) {
        case WIRE_TYPE_VARINT: {
            uint64_t dummy;
            if (!decode_varint(data, end, &dummy)) {
                return false;
            }
            break;
        }
        case WIRE_TYPE_64BIT:
            if (*data + 8 > end) {
                return false;
            }
            *data += 8;
            break;
        case WIRE_TYPE_STRING: {
            uint64_t length;
            if (!decode_varint(data, end, &length)) {
                return false;
            }
            if (*data + length > end) {
                return false;
            }
            *data += length;
            break;
        }
        case WIRE_TYPE_32BIT:
            if (*data + 4 > end) {
    return false;
            }
            *data += 4;
            break;
        default:
            return false; // Unknown wire type
    }
    return true;
}
