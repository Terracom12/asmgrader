# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

# Enable doxygen doc builds of source
function(asmgrader_enable_doxygen DOXYGEN_THEME)
    # If not specified, use the top readme file as the first page
    if((NOT DOXYGEN_USE_MDFILE_AS_MAINPAGE) AND EXISTS "${PROJECT_SOURCE_DIR}/README.md")
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")
    endif()


    # set better defaults for doxygen
    # is_verbose(_is_verbose)
    # if(NOT ${_is_verbose})
    # endif()
    set(DOXYGEN_CALLER_GRAPH NO)
    set(DOXYGEN_CALL_GRAPH NO)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_COLLABORATION_GRAPH NO)
    set(DOXYGEN_PAGE_OUTLINE_PANEL NO)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_FULL_SIDEBAR NO)
    # svg files are much smaller than jpeg and png, and yet they have higher quality
    set(DOXYGEN_DOT_IMAGE_FORMAT svg)
    set(DOXYGEN_DOT_TRANSPARENT YES)

    set(DOXYGEN_HTML_COLORSTYLE LIGHT)

    set(DOXYGEN_JAVADOC_AUTOBRIEF NO)
    set(DOXYGEN_MULTILINE_CPP_IS_BRIEF YES)

    # Older Doxygen version will not handle C++20 without this
    # set(DOXYGEN_CLANG_ASSISTED_PARSING YES)
    # set(DOXYGEN_CLANG_OPTIONS -std=c++20)

    # Exclude the vcpkg files and the files CMake downloads under _deps (like project_options)
    if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
        set(cpm_deps_dir ${CMAKE_BINARY_DIR}/CPM)
    else()
        set(cpm_deps_dir ENV{CPM_SOURCE_CACHE})
    endif()

    set(DOXYGEN_EXCLUDE_PATTERNS 
        "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/*" 
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/*" 
        "${cpm_deps_dir}"
        "cs3b-grader"
        "tests"
    )

    # TODO: Setup a Doxyfile.in; this is getting to be unmanageable

    # use a modern doxygen theme
    # https://github.com/jothepro/doxygen-awesome-css v1.6.1
    FetchContent_Declare(_doxygen_theme
                        URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.3.4.zip)
    FetchContent_MakeAvailable(_doxygen_theme)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET 
        "${_doxygen_theme_SOURCE_DIR}/doxygen-awesome.css"
        "${_doxygen_theme_SOURCE_DIR}/doxygen-awesome-sidebar-only.css"
        "${_doxygen_theme_SOURCE_DIR}/doxygen-awesome-sidebar-only-darkmode-toggle.css"
    )


    ##### Doxygen header extensions:
    # https://jothepro.github.io/doxygen-awesome-css/md_docs_2extensions.html
    set(DOXYGEN_HTML_HEADER "${PROJECT_SOURCE_DIR}/doc/header.html")

    set(DOXYGEN_HTML_EXTRA_FILES ${DOXYGEN_HTML_EXTRA_FILES} "${_doxygen_theme_SOURCE_DIR}/doxygen-awesome-darkmode-toggle.js")
    ##### END Doxygen header extensions

    # find doxygen and dot if available
    find_package(Doxygen 1.10 REQUIRED OPTIONAL_COMPONENTS dot)

    if(NOT DOXYGEN_VERSION MATCHES "1.12.0")
        message(WARNING "Doxygen version should be be exactly 1.12.0; your version is ${DOXYGEN_VERSION}. It's a happy medium between supporting C++20 features and not rendering weirdly")
    endif()


    # add doxygen-docs target
    message(STATUS "Adding `doxygen-docs` target that builds the documentation.")
    doxygen_add_docs(
        doxygen-docs 
        ALL 
        ${PROJECT_SOURCE_DIR}
        COMMENT "Generating documentation - entry file: ${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
    )
endfunction()
