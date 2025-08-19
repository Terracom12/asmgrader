cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Disallow in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not allowed.\n"
        "Please specify a seperate build directory with:\n"
        "  cmake -B <build-dir>"
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

# Creates compile_commands.json to be used with an LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_language(ASM)

if(MSVC)
  message( FATAL_ERROR "Compiling with MSVC is unsupported!" )
endif()

include(cmake/GitVersionTag.cmake)

git_version_tag(MAJOR MINOR PATCH)

project(AsmGrader VERSION ${MAJOR}.${MINOR}.${PATCH} LANGUAGES CXX)

# Show download progress
set(FETCHCONTENT_QUIET FALSE)

# CMake package manager for easy dependency management
#  use local packages if possible as to not download unnecessarily
set(CPM_USE_LOCAL_PACKAGES ON)
include(cmake/CPM.cmake)

# Debug / Release preprocessor definitions
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-DDEBUG>")
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release>>:-DRELEASE>")

add_compile_options("-fconcepts-diagnostics-depth=3")

add_subdirectory(core)

if(EXISTS "grader/" AND IS_DIRECTORY "grader/")
    add_subdirectory(grader)
endif()

option(ENABLE_TESTING "Whether to enable building of unit tests or not" FALSE)
if (ENABLE_TESTING)
    # Add project tests
    add_subdirectory(tests)
endif()
