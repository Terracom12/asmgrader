# Enable by default
enable_testing()

# Partially based on Catch2 docs at
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top
include(${CMAKE_SOURCE_DIR}/cmake/AddAsmArchImpl.cmake)

##### Test runner executable

add_executable(
    asmgrader_tests

    catch2_custom.hpp

    test_metadata.cpp
    test_subprocess.cpp
    test_expected.cpp
    test_symbol_reader.cpp
    test_memory_io.cpp
    test_program.cpp
    test_database_reader.cpp
    test_registers_state.cpp
    test_formatters.cpp

    catch_main.cpp
)

target_link_libraries(
    asmgrader_tests
    PUBLIC asmgrader_core

    PRIVATE 
    Catch2::Catch2
    asmgrader_core_interface
)

##### Simple assembly executable


add_asm_arch_specific_impl(asm_tests simple_asm)

target_compile_definitions(
    asmgrader_tests

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
)

# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

# automatically discover tests that are defined in catch based test files you can modify the unittests. Set TEST_PREFIX
# to whatever you want, or use different for different binaries
catch_discover_tests(
    asmgrader_tests
    TEST_PREFIX
    "unittests."
    REPORTER
    XML
    OUTPUT_DIR
    .
    OUTPUT_PREFIX
    "unittests."
    OUTPUT_SUFFIX
    .xml
)
