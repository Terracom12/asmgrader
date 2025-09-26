# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner


# CPM.cmake README:
# > We recommend that if you use PATCHES, you also set CPM_SOURCE_CACHE. See issue 577.
# https://github.com/cpm-cmake/CPM.cmake/issues/577
if(NOT (DEFINED CPM_SOURCE_CACHE OR DEFINED ENV{CPM_SOURCE_CACHE}))
    message(WARNING "It is highly recommended to set the enviornment or cmake variable `CPM_SOURCE_CACHE` so as to not unnecessarily re-download libraries. Defaulting to ${CPM_SOURCE_DIR}/build/CPM.")
    set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/CPM)
endif()

include(cmake/CPM.cmake)

function(_resolve_alias out_var target)
    get_target_property(real ${target} ALIASED_TARGET)
    if(real)
        set(${out_var} ${real} PARENT_SCOPE)
    else()
        set(${out_var} ${target} PARENT_SCOPE)
    endif()
endfunction()

function(_force_system_includes target)
    _resolve_alias(real_target ${target})
    get_target_property(dirs ${real_target} INTERFACE_INCLUDE_DIRECTORIES)
    message(VERBOSE "Changing dirs: ${dirs} to SYSTEM for ${target} (${real_target})")
    if(dirs)
        # Clear the old include dirs
        set_target_properties(${real_target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")
        # Re-add them as SYSTEM
        target_include_directories(${real_target} SYSTEM INTERFACE ${dirs})
        message(VERBOSE "Changing dirs: ${dirs} to SYSTEM for ${target} (${real_target})")
    endif()

    # Recurse into its dependencies
    get_target_property(deps ${real_target} INTERFACE_LINK_LIBRARIES)
    if(deps)
        message(
            VERBOSE 
            "Recursively applying _force_system_includes to deps ${deps} of ${target} (${real_target})"
        )
        foreach(dep IN LISTS deps)
            if(TARGET ${dep})
                _force_system_includes(${dep})
            endif()
        endforeach()
    endif()
endfunction()

# Done as a function so that updates to variables like
# CMAKE_CXX_FLAGS don't propagate out to other
# targets
macro(asmgrader_setup_dependencies)
    # Package summary:
    #
    # fmt::fmt
    # spdlog
    # argparse
    # Boost::endian
    # Boost::type_index
    # Boost::mp11
    # Boost::pfr
    # Boost::describe
    # Boost::stacktrace
    # Boost::preprocessor
    # range-v3
    # Microsoft.GSL::GSL
    # nlohmann_json::nlohmann_json
    # elfio::elfio
    # Catch2::Catch2WithMain
    # libassert::assert

    # For each dependency, see if it's
    # already been provided to us by a parent project

    if(NOT TARGET fmtlib::fmtlib)
        CPMAddPackage("gh:fmtlib/fmt#11.1.3")
        _force_system_includes(fmt::fmt)
    endif()

    # spdlog for developer logging
    if(NOT TARGET spdlog::spdlog)
        CPMAddPackage(
            NAME spdlog
            VERSION 1.15.1
            GITHUB_REPOSITORY "gabime/spdlog"
            OPTIONS "SPDLOG_FMT_EXTERNAL ON"
            SYSTEM YES
        )
        _force_system_includes(spdlog::spdlog)
    endif()


    if(NOT TARGET argparse::argparse)
        # argparse for command line argument parsing
        CPMAddPackage(
            URI "gh:p-ranav/argparse@3.2"
            SYSTEM TRUE
        )
        _force_system_includes(argparse::argparse)
    endif()

    if(NOT (
        TARGET Boost::endian AND
        TARGET Boost::type_index AND
        TARGET Boost::mp11 AND
        TARGET Boost::pfr AND
        TARGET Boost::describe AND
        TARGET Boost::stacktrace AND
        TARGET Boost::preprocessor
    ))
        # Boost for various utilities
        CPMAddPackage(
            NAME Boost
            VERSION 1.88.0 # Versions less than 1.85.0 may need patches for installation targets.
            URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
            URL_HASH SHA256=f48b48390380cfb94a629872346e3a81370dc498896f16019ade727ab72eb1ec

            # Make the compiler think that Boost is a system lib as to not emit warnings
            PATCHES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/boost-pfr-system.patch"
            OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
                    "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES endian\\\;type_index\\\;describe\\\;mp11\\\;pfr\\\;stacktrace\\\;preprocessor" # Note the escapes!
            SYSTEM TRUE
        )
        _force_system_includes(Boost::endian)
        _force_system_includes(Boost::type_index)
        _force_system_includes(Boost::mp11)
        _force_system_includes(Boost::pfr)
        _force_system_includes(Boost::describe)
        _force_system_includes(Boost::stacktrace)
        _force_system_includes(Boost::preprocessor)
    endif()

    if(NOT TARGET range-v3)
        # Ranges v3 for cleaner loop structures
        CPMAddPackage(
            NAME range-v3
            URL "https://github.com/ericniebler/range-v3/archive/0.12.0.zip"
            VERSION 0.12.0
            # the range-v3 CMakeLists.txt screws with configuration options
            DOWNLOAD_ONLY TRUE
        )

        if(range-v3_ADDED)
            add_library(range-v3 INTERFACE IMPORTED)
            target_include_directories(range-v3 SYSTEM INTERFACE "${range-v3_SOURCE_DIR}/include")
        endif()
    endif()

    if(NOT TARGET Microsoft.GSL::GSL)
        # GSL for more conformant code
        CPMAddPackage(
            NAME GSL
            GITHUB_REPOSITORY "microsoft/GSL"
            VERSION 4.2.0
            GIT_SHALLOW TRUE
            OPTIONS "GSL_MSVC_STATIC_ANALYZER=OFF"
            SYSTEM TRUE
        )
        _force_system_includes(Microsoft.GSL::GSL)
    endif()



    if(NOT TARGET nlohmann_json::nlohmann_json)
        # JSON library for (eventual) output serialization
        CPMAddPackage("gh:nlohmann/json@3.10.5")
        _force_system_includes(nlohmann_json::nlohmann_json)
    endif()


    if(NOT TARGET elfio::elfio)
        # ELFIO for reading symbols from an ELF
        CPMAddPackage(
            NAME elfio
            GITHUB_REPOSITORY "serge1/ELFIO"
            GIT_TAG Release_3.12
        )
        _force_system_includes(elfio::elfio)
    endif()

    if(NOT TARGET libassert::assert)
        # libassert for fancy, overengineered assertions
        CPMAddPackage("gh:jeremy-rifkin/libassert@2.2.1")
        _force_system_includes(libassert::assert)
    endif()


    if(NOT TARGET Catch2::Catch2WithMain)
        CPMAddPackage("gh:catchorg/Catch2@3.8.1")
        _force_system_includes(Catch2::Catch2WithMain)
    endif()
endmacro()
