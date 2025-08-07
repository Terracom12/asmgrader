# Get the version (major + minor + patch) from the latest tag on the local git repo
function(git_version_tag version_major_var version_minor_var version_patch_var)
    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (GIT_TAG STREQUAL "")
        message(WARNING "Git repo does not have a tag to pull version from! Using 0.0.0")
        set(${version_major_var} 0 PARENT_SCOPE)
        set(${version_minor_var} 0 PARENT_SCOPE)
        set(${version_patch_var} 0 PARENT_SCOPE)
        return()
    else ()
        message(STATUS "Detected version ${GIT_TAG} from git tag")
    endif()

    string(REGEX MATCH "v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${GIT_TAG})

    set(${version_major_var} ${CMAKE_MATCH_1} PARENT_SCOPE)
    set(${version_minor_var} ${CMAKE_MATCH_2} PARENT_SCOPE)
    set(${version_patch_var} ${CMAKE_MATCH_3} PARENT_SCOPE)
endfunction()
