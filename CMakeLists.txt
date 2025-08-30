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

##### Project configuration source file
# For now, just substitutes version numbers as defined in root CMake project() decl

configure_file(src/version.hpp.in asmgrader/version.hpp @ONLY)

# Set up CMake project options (ASMGRADER_* args)
include(cmake/ProjectOptions.cmake)
asmgrader_setup_options()

# Set up docs subproject, if option is specified
if(${ASMGRADER_BUILD_DOCS})
    include(cmake/Doxygen.cmake)

    asmgrader_enable_doxygen("")

    # Exit early if we ONLY want to build docs
    if(${ASMGRADER_BUILD_DOCS_ONLY})
        message(WARNING "Exiting early because ASMGRADER_BUILD_DOCS_ONLY was specified")
        return()
    endif()
endif()


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

if(EXISTS "${CMAKE_SOURCE_DIR}/cs3b-grader/CMakeLists.txt")
    add_subdirectory(cs3b-grader)
endif()

