# Based on Catch2 docs at https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top

# Catch2 for unit testing
# FetchContent_Declare(
#   Catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG v3.6.0
# )
# FetchContent_MakeAvailable(Catch2)
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

set(TEST_ASM_TARGET asm_tests)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(TEST_ASM_SRC "simple_asm_x86_64.s")
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(TEST_ASM_SRC "simple_asm_aarch64.s")
else ()
    message(FATAL_ERROR "Unsupported system processor") 
endif ()


set(CMAKE_ASM_FLAGS "-g -static -nolibc -nostartfiles -nostdlib")
set_source_files_properties(${TEST_ASM_SRC} PROPERTIES LANGUAGE ASM)
add_executable(asm_tests ${TEST_ASM_SRC})

target_compile_definitions(
    tests

    PRIVATE
    "ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\""
)

# list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
# include(Catch)
# include(CTest)
# catch_discover_tests(tests)
