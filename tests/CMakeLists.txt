# Enable by default
enable_testing()

# Partially based on Catch2 docs at
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top
include(${CMAKE_SOURCE_DIR}/cmake/AddAsmArchImpl.cmake)

set(
    TESTS_SRCS

    test_static_expected.cpp
    test_static_formatters.cpp
    test_static_tuple_matcher.cpp
    test_expression_inspection.cpp
    test_syntax_highlighter.cpp
    test_cconstexpr.cpp
    test_static_string.cpp
    test_static_byte.cpp
    test_bit_casts.cpp
    test_metadata.cpp
    test_subprocess.cpp
    test_expected.cpp
    test_symbol_reader.cpp
    test_memory_io.cpp
    test_program.cpp
    test_database_reader.cpp
    test_registers_state.cpp
    test_file_searcher.cpp
    test_byte_ranges.cpp
)

##### Simple assembly executable

add_asm_arch_specific_impl(asm_tests resources/simple_asm)

set(asm_tests_weird_name_exec "${CMAKE_CURRENT_BINARY_DIR}/(asm_tests () weird \\(name) ~`")

add_custom_command(
    TARGET asm_tests POST_BUILD
    COMMAND cp $<TARGET_FILE:asm_tests> "${asm_tests_weird_name_exec}"
    VERBATIM
)

##### Test runner executable

add_executable(
    asmgrader_tests

    catch2_custom.hpp

    ${TESTS_SRCS}

    catch_main.cpp
)

add_dependencies(asmgrader_tests asm_tests)

target_compile_definitions(
    asmgrader_tests

    PUBLIC
    CATCH_CONFIG_RUNTIME_STATIC_REQUIRE # Make STATIC_REQUIRE behave as REQUIRE

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
    "RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
    "ASM_TESTS_WEIRD_NAME_EXEC=R\"abc(${asm_tests_weird_name_exec})abc\""
)

target_link_libraries(
    asmgrader_tests
    PUBLIC asmgrader_core

    PRIVATE 
    Catch2::Catch2
    asmgrader_core_interface
)

# TODO: Migh want to look into changing this later.
# Just attempt to compile all tests in the constexpr version for simplicity,
# since there really aren't that many.
add_executable(
    asmgrader_constexpr_tests
    EXCLUDE_FROM_ALL

    ${TESTS_SRCS}
)

target_link_libraries(
    asmgrader_constexpr_tests
    PUBLIC asmgrader_core

    PRIVATE 
    Catch2::Catch2WithMain
    asmgrader_core_interface
)

set(asm_tests_weird_name_exec "${CMAKE_CURRENT_BINARY_DIR}/(asm_tests () weird \\(name) ~`")

add_custom_command(
    TARGET asmgrader_tests POST_BUILD
    COMMAND cp $<TARGET_FILE:asm_tests> "${asm_tests_weird_name_exec}"
    VERBATIM
)

target_compile_definitions(
    asmgrader_constexpr_tests

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
    "RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\""
    "ASM_TESTS_WEIRD_NAME_EXEC=R\"abc(${asm_tests_weird_name_exec})abc\""
)

# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

catch_discover_tests(
    asmgrader_tests
    TEST_PREFIX
    "unittests."
    REPORTER
    "JUnit"
    OUTPUT_DIR
    ${CMAKE_SOURCE_DIR}/reports
    OUTPUT_PREFIX
    "junit-unittests."
    OUTPUT_SUFFIX
    ".xml"
    EXTRA_ARGS
    "--reporter console::out=-"
    "--invisibles"
)

add_test(
    NAME asmgrader_constexpr_tests
    COMMAND ${CMAKE_MAKE_PROGRAM} asmgrader_constexpr_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
