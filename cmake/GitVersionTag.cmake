# Get the version (major + minor + patch) from the latest tag on the local git repo
macro(git_version_tag version_major_var version_minor_var version_patch_var)
    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(${version_major_var} 0)
    set(${version_minor_var} 0)
    set(${version_patch_var} 0)

    if(GIT_TAG STREQUAL "")
        message(WARNING "Git repo does not have a tag to pull version from. Defaulting to v0.0.0")
        return()
    else()
        message(VERBOSE "Attempting to obtain version from Latest git tag (${GIT_TAG})")
    endif()

    if(${GIT_TAG} MATCHES "v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(${version_major_var} ${CMAKE_MATCH_1})
        set(${version_minor_var} ${CMAKE_MATCH_2})
        set(${version_patch_var} ${CMAKE_MATCH_3})
        message(STATUS "Building as version ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} (obtained from latest git tag)")
    else()
        message(WARNING "Could not resolve version from git tag. Defaulting to v0.0.0")
    endif()

endmacro()
