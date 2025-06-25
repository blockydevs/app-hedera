cmake_minimum_required(VERSION 3.10)
project(ledger-app-hedera-unit-tests C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CMOCKA REQUIRED cmocka)

enable_testing()
include(CTest)

include_directories(../../src)

# Disabled due to BOLOS SDK dependencies
# add_library(hedera_format SHARED ../../src/hedera_format.c)
# add_executable(test_hedera_format test_hedera_format.c)
# target_link_libraries(test_hedera_format ${CMOCKA_LIBRARIES} hedera_format)
# target_include_directories(test_hedera_format PUBLIC ${CMOCKA_INCLUDE_DIRS})
# target_compile_options(test_hedera_format PUBLIC ${CMOCKA_CFLAGS_OTHER})
# target_link_directories(test_hedera_format PUBLIC ${CMOCKA_LIBRARY_DIRS})
# add_test(test_hedera_format ${CMAKE_CURRENT_BINARY_DIR}/test_hedera_format)

# Proto varlen parser tests (standalone, minimal dependencies)
add_library(proto_varlen_parser SHARED ../../src/proto_varlen_parser.c)
target_compile_definitions(proto_varlen_parser PRIVATE NO_BOLOS_SDK=1)

# Parser tests for proto_varlen_parser (basic functionality, no security concerns)
add_executable(test_proto_varlen_parser proto_varlen/test_proto_varlen_parser.c)
target_link_libraries(test_proto_varlen_parser ${CMOCKA_LIBRARIES} proto_varlen_parser)
target_include_directories(test_proto_varlen_parser PUBLIC ${CMOCKA_INCLUDE_DIRS})
target_compile_options(test_proto_varlen_parser PUBLIC ${CMOCKA_CFLAGS_OTHER})
target_link_directories(test_proto_varlen_parser PUBLIC ${CMOCKA_LIBRARY_DIRS})
add_test(test_proto_varlen_parser ${CMAKE_CURRENT_BINARY_DIR}/test_proto_varlen_parser)

# Security tests for proto_varlen_parser (buffer overflows, infinite loops)
add_executable(test_proto_varlen_security proto_varlen/test_proto_varlen_security.c)
target_link_libraries(test_proto_varlen_security ${CMOCKA_LIBRARIES} proto_varlen_parser)
target_include_directories(test_proto_varlen_security PUBLIC ${CMOCKA_INCLUDE_DIRS})
target_compile_options(test_proto_varlen_security PUBLIC ${CMOCKA_CFLAGS_OTHER})
target_link_directories(test_proto_varlen_security PUBLIC ${CMOCKA_LIBRARY_DIRS})
add_test(test_proto_varlen_security ${CMAKE_CURRENT_BINARY_DIR}/test_proto_varlen_security)

# Edge case tests for proto_varlen_parser (empty data, single bytes, pointer manipulation)
add_executable(test_proto_varlen_edge_cases proto_varlen/test_proto_varlen_edge_cases.c)
target_link_libraries(test_proto_varlen_edge_cases ${CMOCKA_LIBRARIES} proto_varlen_parser)
target_include_directories(test_proto_varlen_edge_cases PUBLIC ${CMOCKA_INCLUDE_DIRS})
target_compile_options(test_proto_varlen_edge_cases PUBLIC ${CMOCKA_CFLAGS_OTHER})
target_link_directories(test_proto_varlen_edge_cases PUBLIC ${CMOCKA_LIBRARY_DIRS})
add_test(test_proto_varlen_edge_cases ${CMAKE_CURRENT_BINARY_DIR}/test_proto_varlen_edge_cases)
