##### Dependencies

CPMAddPackage(
  NAME Boost
  VERSION 1.86.0 # Versions less than 1.85.0 may need patches for installation targets.
  URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
  URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
          "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES range" # Note the escapes!
)

# spdlog for logging
# FetchContent_Declare(
#         spdlog
#         GIT_REPOSITORY https://github.com/gabime/spdlog.git
#         GIT_TAG v1.15.1
# )
CPMAddPackage("gh:fmtlib/fmt#11.1.3") 
set(SPDLOG_FMT_EXTERNAL ON)
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog 
  VERSION 1.15.1 
  OPTIONS "SPDLOG_FMT_EXTERNAL 1"
)
# Use our own version of fmtlib
# FetchContent_MakeAvailable(spdlog)
# # # special approach based on the issue below:
# # # https://github.com/gabime/spdlog/issues/1520
# # FetchContent_GetProperties(spdlog)
# # if (NOT spdlog_POPULATED)
# #     FetchContent_Populate(spdlog)
# #     add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
# # endif()
#
# # fmtlib for better string formatting
# FetchContent_Declare(
#   fmt
#   GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#   GIT_TAG 11.1.3
# )
# FetchContent_MakeAvailable(fmt)
#
# # argparse for command line argument parsing
# FetchContent_Declare(
#     argparse
#     GIT_REPOSITORY https://github.com/p-ranav/argparse.git
#     GIT_TAG v3.2
# )
# FetchContent_MakeAvailable(argparse)
CPMAddPackage("gh:p-ranav/argparse@3.2") 
#
# find_package(Boost REQUIRED COMPONENTS ranges)

# nlohmann's JSON library
# FetchContent_Declare(
#     json
#     URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
# )
# FetchContent_MakeAvailable(json)
CPMAddPackage("gh:nlohmann/json@3.10.5")

##### Library declaration

# Create library for easy use with assignment tests and unit tests in test/
add_library(
	autograder_lib

    subprocess/subprocess.hpp
    subprocess/subprocess.cpp
    subprocess/tracer.hpp
    subprocess/tracer.cpp
    subprocess/tracer_types.hpp
    logging.hpp
    util/class_traits.hpp
    util/linux.hpp
    # console/interface.cpp
    # console/interface.hpp
)

target_compile_options(
	autograder_lib
    PUBLIC

    "$<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion>"

    # Enable debug symbols; disable optimizations; and enable sanitizers in debug mode
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-g3;-O0;-fno-omit-frame-pointer>" 
    # "$<$<CONFIG:Debug>:-fsanitize=address,undefined>"

    # Enable optimizations and warnings are errors in release mode
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release>>:-O3;-Werror>"
)

target_link_options(
	autograder_lib

    PUBLIC
    # Disable optimizations in debug mode
    "$<$<CONFIG:Debug>:-O0;-fno-omit-frame-pointer>"
)

# Public header files
target_include_directories(
	autograder_lib
	PUBLIC ../include
    PRIVATE .
)

target_link_libraries(
	autograder_lib

    PUBLIC
    fmt::fmt
    argparse
    spdlog
    nlohmann_json::nlohmann_json
    Boost::range
)

# add_subdirectory(executables)
