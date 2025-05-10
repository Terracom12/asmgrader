# Based on Catch2 docs at https://github.com/catchorg/Catch2/blob/devel/docs/cmake-integration.md#top

# Catch2 for unit testing
# FetchContent_Declare(
#   Catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG v3.6.0
# )
# FetchContent_MakeAvailable(Catch2)
CPMAddPackage("gh:catchorg/Catch2@3.4.0")

add_executable(
    tests
    test_subprocess.cpp
)

target_link_libraries(
    tests
    PUBLIC autograder_lib
    PRIVATE Catch2::Catch2WithMain
)

target_include_directories(
    tests
    PRIVATE ../src
)

# list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
# include(Catch)
# include(CTest)
# catch_discover_tests(tests)
