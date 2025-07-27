# Partially based on Catch2 docs at:
# https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top

include(${CMAKE_SOURCE_DIR}/cmake/AddAsmArchImpl.cmake)

# Catch2 for unit testing
CPMAddPackage("gh:catchorg/Catch2@3.4.0")

##### Test runner executable

add_executable(
    tests

    catch2_custom.hpp
    test_subprocess.cpp
    test_expected.cpp
    test_symbol_reader.cpp
    test_memory_io.cpp
    test_program.cpp
    catch_main.cpp
)

target_link_libraries(
    tests
    PUBLIC autograder_core
    PRIVATE Catch2::Catch2
)

target_include_directories(
    tests
    PRIVATE ../core
)

##### Simple assembly executable


add_asm_arch_specific_impl(asm_tests simple_asm)
# set_source_files_properties(${TEST_ASM_SRC} PROPERTIES LANGUAGE ASM)
# add_executable(asm_tests ${TEST_ASM_SRC})

target_compile_definitions(
    tests

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
)

# list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
# include(Catch)
# include(CTest)
# catch_discover_tests(tests)
