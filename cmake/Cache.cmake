# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

# Enable cache if available
function(asmgrader_enable_cache)
    set(CACHE_OPTION
        "ccache"
        CACHE STRING "Compiler cache to be used"
    )
    set(CACHE_OPTION_VALUES "ccache" "sccache")
    set_property(CACHE CACHE_OPTION PROPERTY STRINGS ${CACHE_OPTION_VALUES})
    list(
        FIND
        CACHE_OPTION_VALUES
        ${CACHE_OPTION}
        CACHE_OPTION_INDEX
    )

    if(${CACHE_OPTION_INDEX} EQUAL -1)
        message(
            STATUS
            "Using custom compiler cache system: '${CACHE_OPTION}', explicitly supported entries are ${CACHE_OPTION_VALUES}"
        )
    endif()

    find_program(CACHE_BINARY NAMES ${CACHE_OPTION_VALUES})
    if(CACHE_BINARY)
        message(STATUS "${CACHE_BINARY} found and enabled")
        set(CMAKE_CXX_COMPILER_LAUNCHER
            ${CACHE_BINARY}
            CACHE FILEPATH "CXX compiler cache used"
        )
        set(CMAKE_C_COMPILER_LAUNCHER
            ${CACHE_BINARY}
            CACHE FILEPATH "C compiler cache used"
        )
    else()
        message(WARNING "${CACHE_OPTION} is enabled but was not found. Not using it")
    endif()
endfunction()
