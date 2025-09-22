##### Variable declarations

set(
    CORE_LINK_LIBS_PUBLIC

    fmt::fmt
    spdlog
    Boost::endian
    Boost::type_index
    Boost::mp11
    Boost::pfr
    Boost::describe
    Boost::stacktrace
    Boost::preprocessor
    range-v3
    Microsoft.GSL::GSL
    libassert::assert
)

set(
    CORE_LINK_LIBS_PRIVATE

    asmgrader_options
    asmgrader_warnings

    argparse
    nlohmann_json::nlohmann_json
    elfio::elfio
)

set(
    CORE_SOURCES

    subprocess/subprocess.cpp
    subprocess/traced_subprocess.cpp
    subprocess/tracer.cpp
    subprocess/memory/memory_io_base.cpp
    subprocess/memory/ptrace_memory_io.cpp
    subprocess/run_result.cpp

    common/terminal_checks.cpp

    output/plaintext_serializer.cpp
    output/stdout_sink.cpp

    registrars/global_registrar.cpp

    test_runner.cpp
    multi_student_runner.cpp

    symbols/elf_reader.cpp
    symbols/symbol_table.cpp

    program/program.cpp

    user/cl_args.cpp

    api/assignment.cpp
    api/test_context.cpp
    api/syntax_highlighter.cpp

    database_reader.cpp

    user/file_searcher.cpp
    user/assignment_file_searcher.cpp

    app/professor_app.cpp
    app/student_app.cpp
)

set(
    CORE_WITH_MAIN_SOURCES

    main.cpp
)

##### Helper function to create all core targets. Used to create a near-duplicate target for professor mode

function(create_target basename)
    #### Interface (just for internal includes and libraries)

    add_library(${basename}_core_interface INTERFACE)

    target_include_directories(
        ${basename}_core_interface

        INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_BINARY_DIR}/asmgrader"   # For generated `version.hpp`
        # treat `include/asmgrader` as if it's within the  `src/` tree
        "${CMAKE_SOURCE_DIR}/include/asmgrader"
    )

    target_link_libraries(
        ${basename}_core_interface

        INTERFACE
        ${CORE_LINK_LIBS_PRIVATE}
    )

    #### Core library

    add_library(
        ${basename}_core

        ${CORE_SOURCES}
    )

    # Include paths
    target_include_directories(
        ${basename}_core

        PUBLIC
        "${CMAKE_BINARY_DIR}"   # For generated `version.hpp`
        "${CMAKE_SOURCE_DIR}/include"
    )


    target_link_libraries(
        ${basename}_core

        PUBLIC
        ${CORE_LINK_LIBS_PUBLIC}

        PRIVATE
        ${basename}_core_interface
    )

    if(TARGET boost_stacktrace_from_exception)
        target_link_libraries(
            ${basename}_core
            PUBLIC
            boost_stacktrace_from_exception
        )
    endif()


    # Create library for core + main so that the user doesn't need to provide an entry point
    add_library(
        ${basename}_core_with_main

        ${CORE_WITH_MAIN_SOURCES}
    )

    target_link_libraries(
        ${basename}_core_with_main

        PUBLIC
        ${basename}_core

        PRIVATE
        ${basename}_core_interface
    )
endfunction()

##### Student version
create_target(asmgrader)

##### Professor version
create_target(asmgrader_prof)
target_compile_definitions(
    asmgrader_prof_core

    PUBLIC
    "PROFESSOR_VERSION"
)
