cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

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

project(CS3BUnittests VERSION 0.0.1 LANGUAGES CXX)

# CMake package manager for easy dependency management
#  use local packages if possible as to not download unnecessarily
set(CPM_USE_LOCAL_PACKAGES ON)
include(cmake/CPM.cmake)

# Debug / Release preprocessor definitions
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-DDEBUG>")
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release>>:-DRELEASE>")

# Add main project library
add_subdirectory(src)

option(ENABLE_TESTING "Whether to enable building of unit tests or not" FALSE)
if (ENABLE_TESTING)
    # Add project tests
    add_subdirectory(test)
endif()
