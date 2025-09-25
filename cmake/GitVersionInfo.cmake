# Get the version (major + minor + patch) from the latest tag on the local git repo
# and also the full hash of the latest commit
macro(git_version_info version_major_var version_minor_var version_patch_var commit_hash)
    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND git log -n 1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(${commit_hash} "")

    if(GIT_HASH STREQUAL "")
        message(WARNING "Somehow failed to obtain hash of latest commit from git repo. Defaulting to \"\"")
    else()
        message(STATUS "Using ${GIT_HASH} as git commit hash")
        set(${commit_hash} ${GIT_HASH})
    endif()

    set(${version_major_var} 0)
    set(${version_minor_var} 0)
    set(${version_patch_var} 0)

    if(GIT_TAG STREQUAL "")
        message(WARNING "Git repo does not have a tag to pull version from. Defaulting to v0.0.0")
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
