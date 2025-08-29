cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)

# Creates compile_commands.json to be used with an LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  message( FATAL_ERROR "Compiling with MSVC is unsupported!" )
endif()

# Disable in-source builds
include(cmake/PreventInSourceBuilds.cmake)

# Get the project version based on the most recent git tag
include(cmake/GitVersionInfo.cmake)
git_version_info(
    ASMGRADER_VERSION_MAJOR 
    ASMGRADER_VERSION_MINOR 
    ASMGRADER_VERSION_PATCH 
    ASMGRADER_VERSION_GIT_HASH
)

project(AsmGrader
    VERSION ${ASMGRADER_VERSION_MAJOR}.${ASMGRADER_VERSION_MINOR}.${ASMGRADER_VERSION_PATCH}
    LANGUAGES CXX
)

enable_language(ASM)



# Setup CMake project options (ASMGRADER_* args)
include(cmake/ProjectOptions.cmake)
asmgrader_setup_options()

asmgrader_global_options()

# Set up project dependencies
# CPM (CMake package manager) is used for easy dependency management
include(cmake/SetupDependencies.cmake)
asmgrader_setup_dependencies()

asmgrader_local_options()

add_subdirectory(src)

include(CTest)

if (ASMGRADER_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(EXISTS "cs3b-grader/CMakeLists.txt")
    add_subdirectory(cs3b-grader)
endif()

