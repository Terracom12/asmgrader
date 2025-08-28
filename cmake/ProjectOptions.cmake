# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

include(CheckCXXSourceCompiles)


macro(asmgrader_supports_sanitizers)
    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
        message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
        set(TEST_PROGRAM "int main() { return 0; }")

        # Check if UndefinedBehaviorSanitizer works at link time
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

        if(HAS_UBSAN_LINK_SUPPORT)
            message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
            set(SUPPORTS_UBSAN ON)
        else()
            message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
            set(SUPPORTS_UBSAN OFF)
        endif()
    else()
        set(SUPPORTS_UBSAN OFF)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*")
        message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
        set(TEST_PROGRAM "int main() { return 0; }")

        # Check if AddressSanitizer works at link time
        set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
        check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

        if(HAS_ASAN_LINK_SUPPORT)
            message(STATUS "AddressSanitizer is supported at both compile and link time.")
            set(SUPPORTS_ASAN ON)
        else()
            message(WARNING "AddressSanitizer is NOT supported at link time.")
            set(SUPPORTS_ASAN OFF)
        endif()
    else()
        set(SUPPORTS_ASAN OFF)
    endif()
endmacro()

macro(asmgrader_setup_options)
    # option(ASMGRADER_ENABLE_HARDENING "Enable hardening" ON)
    # option(ASMGRADER_ENABLE_COVERAGE "Enable coverage reporting" OFF)
    # cmake_dependent_option(
    #     ASMGRADER_ENABLE_GLOBAL_HARDENING
    #     "Attempt to push hardening options to built dependencies"
    #     ON
    #     ASMGRADER_ENABLE_HARDENING
    #     OFF
    # )

    asmgrader_supports_sanitizers()

    if(NOT PROJECT_IS_TOP_LEVEL)
        option(ASMGRADER_BUILD_TESTS "Enable building unit tests" OFF)
        option(ASMGRADER_BUILD_DOCS "Enable building docs (Doxygen)" OFF)
        # option(ASMGRADER_ENABLE_IPO "Enable IPO/LTO" OFF)
        option(ASMGRADER_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        # option(ASMGRADER_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
        # option(ASMGRADER_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
        option(ASMGRADER_ENABLE_PCH "Enable precompiled headers" OFF)
        option(ASMGRADER_ENABLE_CACHE "Enable ccache" OFF)
    else()
        option(ASMGRADER_BUILD_TESTS "Enable building unit tests" ON)
        option(ASMGRADER_BUILD_DOCS "Enable building docs (Doxygen)" OFF)
        # option(ASMGRADER_ENABLE_IPO "Enable IPO/LTO" ON)
        option(ASMGRADER_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
        option(ASMGRADER_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
        option(ASMGRADER_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
        option(ASMGRADER_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
        option(ASMGRADER_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
        # option(ASMGRADER_ENABLE_CLANG_TIDY "Enable clang-tidy" ON)
        # option(ASMGRADER_ENABLE_CPPCHECK "Enable cpp-check analysis" ON)
        option(ASMGRADER_ENABLE_PCH "Enable precompiled headers" OFF)
        option(ASMGRADER_ENABLE_CACHE "Enable ccache" ON)
    endif()

    if(NOT PROJECT_IS_TOP_LEVEL)
        mark_as_advanced(
            ASMGRADER_BUILD_TESTS
            # ASMGRADER_ENABLE_IPO
            ASMGRADER_WARNINGS_AS_ERRORS
            ASMGRADER_ENABLE_SANITIZER_ADDRESS
            ASMGRADER_ENABLE_SANITIZER_LEAK
            ASMGRADER_ENABLE_SANITIZER_UNDEFINED
            ASMGRADER_ENABLE_SANITIZER_THREAD
            ASMGRADER_ENABLE_SANITIZER_MEMORY
            # ASMGRADER_ENABLE_CLANG_TIDY
            # ASMGRADER_ENABLE_CPPCHECK
            # ASMGRADER_ENABLE_COVERAGE
            ASMGRADER_ENABLE_PCH
            ASMGRADER_ENABLE_CACHE
        )
    endif()
endmacro()


# Options that should be set *before* adding dependencies
macro(asmgrader_global_options)
    if(${ASMGRADER_BUILD_DOCS})
        include(FetchContent)
        include(cmake/Doxygen.cmake)
        asmgrader_enable_doxygen("")

        if(${ASMGRADER_BUILD_DOCS_ONLY})
            unset(ASMGRADER_BUILD_DOCS_ONLY CACHE)
            message(WARNING "Exiting early because ASMGRADER_BUILD_DOCS_ONLY was specified")
            return()
        endif()
    endif()
endmacro()

# Options that are project-specific, and may be set after adding dependencies
macro(asmgrader_local_options)
    if(PROJECT_IS_TOP_LEVEL)
        include(cmake/StandardProjectSettings.cmake)
    endif()

    add_library(asmgrader_warnings INTERFACE)
    add_library(asmgrader_options INTERFACE)

    target_compile_definitions(
        asmgrader_options
        INTERFACE
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    )


    include(cmake/CompilerWarnings.cmake)
    asmgrader_set_target_warnings(
        asmgrader_warnings
        ${ASMGRADER_WARNINGS_AS_ERRORS}
    )

    include(cmake/Sanitizers.cmake)
    asmgrader_enable_sanitizers(
        asmgrader_options
        ${ASMGRADER_ENABLE_SANITIZER_ADDRESS}
        ${ASMGRADER_ENABLE_SANITIZER_LEAK}
        ${ASMGRADER_ENABLE_SANITIZER_UNDEFINED}
        ${ASMGRADER_ENABLE_SANITIZER_THREAD}
        ${ASMGRADER_ENABLE_SANITIZER_MEMORY}
    )

    if(ASMGRADER_ENABLE_PCH)
        target_precompile_headers(
            asmgrader_options
            INTERFACE
            <limits>
            <compare>

            <concepts>

            <memory>

            <type_traits>

            <functional>
            <optional>
            <tuple>
            <utility>
            <variant>

            <array>
            <vector>

            <algorithm>
            <numeric>

            <string>
            <string_view>

            <regex>

            <filesystem>
            <iostream>
        )
    endif()

    if(ASMGRADER_ENABLE_CACHE)
        include(cmake/Cache.cmake)
        asmgrader_enable_cache()
    endif()

    # include(cmake/StaticAnalyzers.cmake)
    # if(ASMGRADER_ENABLE_CLANG_TIDY)
    #     asmgrader_enable_clang_tidy(asmgrader_options ${ASMGRADER_WARNINGS_AS_ERRORS})
    # endif()

    # if(ASMGRADER_ENABLE_CPPCHECK)
    #     asmgrader_enable_cppcheck(
    #         ${ASMGRADER_WARNINGS_AS_ERRORS}
    #         "" # override cppcheck options
    #     )
    # endif()

    # if(ASMGRADER_ENABLE_COVERAGE)
    #     include(cmake/Tests.cmake)
    #     asmgrader_enable_coverage(asmgrader_options)
    # endif()
endmacro()
