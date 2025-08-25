##### Project configuration
# For now, just substitutes version numbers as defined in root CMake project() decl

configure_file(version.hpp.in version.hpp)

##### Variable declarations

set(
    INTERF_SOURCES
    subprocess/subprocess.hpp
    subprocess/traced_subprocess.hpp
    subprocess/tracer.hpp
    subprocess/tracer_types.hpp
    subprocess/syscall.hpp
    subprocess/syscall_record.hpp
    subprocess/memory/concepts.hpp
    subprocess/memory/memory_io.hpp
    subprocess/memory/memory_io_base.hpp
    subprocess/memory/ptrace_memory_io.hpp
    subprocess/memory/memory_io_serde.hpp
    subprocess/run_result.hpp

    common/class_traits.hpp
    common/macros.hpp
    common/linux.hpp
    common/expected.hpp
    common/unreachable.hpp
    common/error_types.hpp
    common/extra_formatters.hpp
    common/byte_vector.hpp
    common/byte_array.hpp
    common/timespec_operator_eq.hpp
    common/terminal_checks.hpp
    common/os.hpp
    common/aliases.hpp
    common/static_string.hpp
    common/formatters/debug.hpp
    common/formatters/generic_impl.hpp
    common/formatters/unknown.hpp
    common/formatters/enum.hpp
    common/formatters/detail.hpp
    common/formatters/std.hpp
    common/formatters/formatters.hpp
    common/formatters/aggregate.hpp

    meta/compatible_function_args.hpp
    meta/always_false.hpp
    meta/functional_traits.hpp
    meta/count_if.hpp
    meta/remove_all_pointers.hpp
    meta/integer.hpp
    meta/tuple_matcher.hpp

    logging.hpp

    registrars/global_registrar.hpp
    registrars/auto_registrars.hpp

    output/plaintext_serializer.hpp
    output/serializer.hpp
    output/sink.hpp
    output/stdout_sink.hpp
    output/verbosity.hpp

    symbols/elf_reader.hpp
    symbols/symbol.hpp
    symbols/symbol_table.hpp

    program/program.hpp

    user/cl_args.hpp
    user/file_searcher.hpp
    user/program_options.hpp

    api/asm_function.hpp
    api/asm_symbol.hpp
    api/asm_data.hpp
    api/asm_buffer.hpp
    api/test_base.hpp
    api/assignment.hpp
    api/metadata.hpp
    api/test_context.hpp
    api/macros.hpp
    api/registers_state.hpp
    api.hpp

    app/app.hpp
    app/trace_exception.hpp

    test_runner.hpp
    multi_student_runner.hpp

    grading_session.hpp

    database_reader.hpp

    exceptions.hpp
)

# -Wno-gnu-statement-expression-from-macro-expansion

# TODO: Yeah, `.` is bad practice...
set(
    INTERF_INCLUDE_DIRS

    .
)

set(
    INTERF_LINK_LIBS

    asmgrader_options
    asmgrader_warnings

    fmt::fmt
    spdlog
    argparse
    Boost::endian
    Boost::type_index
    Boost::mp11
    Boost::pfr
    Boost::describe
    Boost::stacktrace
    Boost::preprocessor
    range-v3
    Microsoft.GSL::GSL
    nlohmann_json::nlohmann_json
    elfio::elfio
)

set(
    CORE_SOURCES

    subprocess/subprocess.cpp
    subprocess/traced_subprocess.cpp
    subprocess/tracer.cpp
    subprocess/syscall_entries.inl
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

    database_reader.cpp
)

set(
    CORE_WITH_MAIN_SOURCES

    main.cpp
)

##### Helper function to create all core targets. Used to create a near-duplicate target for professor mode

function(create_targets  basename)
    ##### Library declarations

    # Interface library for including headers and compilation options
    # Just for use with `autograder_assignments_lib` target for now
    add_library(
        ${basename}_interface
        INTERFACE

        ${INTERF_SOURCES}
    )

    target_compile_options(
        ${basename}_interface
        INTERFACE

        ${INTERF_COMPILE_OPTS}
    )

    target_link_options(
        ${basename}_interface
        INTERFACE

        ${INTERF_LINK_OPTS}
    )

    # Public header files
    target_include_directories(
        ${basename}_interface
        INTERFACE 

        ${INTERF_INCLUDE_DIRS}
        "${CMAKE_CURRENT_BINARY_DIR}"   # For generated `configure.hpp`
    )

    target_link_libraries(
        ${basename}_interface
        INTERFACE

        ${INTERF_LINK_LIBS}
    )

    if (TARGET boost_stacktrace_from_exception)
        target_link_libraries(
            ${basename}_interface
            INTERFACE
            boost_stacktrace_from_exception
        )
    endif ()

    #### Core library

    # Create core library for easy use with assignment tests and unit tests in test/
    add_library(
        ${basename}_core

        ${CORE_SOURCES}
    )

    target_link_libraries(
        ${basename}_core

        PUBLIC
        ${basename}_interface
    )

    # Create library for core + main so that the user doesn't need to provide an entry point
    add_library(
        ${basename}_core_with_main

        ${CORE_WITH_MAIN_SOURCES}
    )

    target_link_libraries(
        ${basename}_core_with_main

        PUBLIC
        ${basename}_core
    )

endfunction()

##### Student version
create_targets(asmgrader)
target_sources(
    asmgrader_interface

    INTERFACE
    app/student_app.hpp
)
target_sources(
    asmgrader_core

    PRIVATE
    app/student_app.cpp
)

##### Professor version
create_targets(asmgrader_prof)
target_sources(
    asmgrader_prof_interface

    INTERFACE
    user/file_searcher.hpp
    user/assignment_file_searcher.hpp

    app/professor_app.hpp
)
target_compile_definitions(
    asmgrader_prof_interface

    INTERFACE
    "PROFESSOR_VERSION"
)
target_sources(
    asmgrader_prof_core

    PRIVATE
    user/file_searcher.cpp
    user/assignment_file_searcher.cpp

    app/professor_app.cpp
)
