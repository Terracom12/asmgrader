# Set up version info source and header files
# These include project version, git hash, and library versions (as available)
# Also includes parent "implementation" project's version, if defined

function(asmgrader_get_dependency_versions  outvar)
    set(dep_versions "")
    foreach(pkg IN LISTS CPM_PACKAGES)
        if(DEFINED CPM_PACKAGE_${pkg}_VERSION)
            list(APPEND dep_versions "${pkg} ${CPM_PACKAGE_${pkg}_VERSION}")
        else()
            list(APPEND dep_versions "${pkg} (version unknown)")
        endif()
    endforeach()

    # list(TRANSFORM dep_versions PREPEND [["]])
    # list(TRANSFORM dep_versions APPEND [["]])

    list(JOIN dep_versions " " dep_versions_str)

    set(${outvar} ${dep_versions_str} PARENT_SCOPE)
endfunction()

asmgrader_get_dependency_versions(ASMGRADER_DEPENDENCY_VERSIONS_STR)

configure_file(cmake/in/version_macros.hpp.in asmgrader/version_macros.hpp)
