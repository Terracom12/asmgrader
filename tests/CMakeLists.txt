# Partially based on Catch2 docs at:
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top

include(${CMAKE_SOURCE_DIR}/cmake/AddAsmArchImpl.cmake)

# Catch2 for unit testing
CPMAddPackage("gh:catchorg/Catch2@3.4.0")

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

    catch_main.cpp
)

target_link_libraries(
    asmgrader_tests
    PUBLIC asmgrader_core
    PRIVATE Catch2::Catch2
)

target_include_directories(
    asmgrader_tests
    PRIVATE ../core
)

##### Simple assembly executable


add_asm_arch_specific_impl(asm_tests simple_asm)

target_compile_definitions(
    asmgrader_tests

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
)

# list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
# include(Catch)
# include(CTest)
# catch_discover_tests(tests)
