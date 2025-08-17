##### Dependencies

# Boost for various utilities
CPMAddPackage(
  NAME Boost
  VERSION 1.88.0 # Versions less than 1.85.0 may need patches for installation targets.
  URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.tar.xz
  PATCHES "boost-pfr-system.patch"   # Make the compiler think that Boost is a system lib as to not emit warnings
  #URL_HASH SHA256=3621533e820dcab1e8012afd583c0c73cf0f77694952b81352bf38c1488f9cb4
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
          "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES endian\\\;type_index\\\;describe\\\;mp11\\\;pfr\\\;stacktrace" # Note the escapes!
)

# spdlog for logging
CPMAddPackage("gh:fmtlib/fmt#11.1.3")
set(SPDLOG_FMT_EXTERNAL ON)
CPMAddPackage(
  GITHUB_REPOSITORY gabime/spdlog
  VERSION 1.15.1
  OPTIONS "SPDLOG_FMT_EXTERNAL 1"
)

# argparse for command line argument parsing
CPMAddPackage("gh:p-ranav/argparse@3.2")

# nlohmann's JSON library
CPMAddPackage("gh:nlohmann/json@3.10.5")

# Ranges v3 for cleaner loop structures
CPMAddPackage(
  NAME range-v3
  URL https://github.com/ericniebler/range-v3/archive/0.12.0.zip
  VERSION 0.12.0
  # the range-v3 CMakeLists screws with configuration options
  DOWNLOAD_ONLY True
)

if(range-v3_ADDED)
  add_library(range-v3 INTERFACE IMPORTED)
  target_include_directories(range-v3 INTERFACE "${range-v3_SOURCE_DIR}/include")
endif()

# GSL for more conformant code
CPMAddPackage(
  NAME GSL
  GITHUB_REPOSITORY microsoft/GSL
  VERSION 4.2.0
  GIT_SHALLOW TRUE
  OPTIONS "GSL_MSVC_STATIC_ANALYZER=OFF"
)

# TODO: Probably want to statically link...
find_package(PkgConfig REQUIRED)
# libelf for reading symbols from an ELF
pkg_check_modules(LIBELF REQUIRED libelf)

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
    subprocess/memory/concepts.hpp
    subprocess/memory/memory_io.hpp
    subprocess/memory/memory_io_base.hpp
    subprocess/memory/ptrace_memory_io.hpp
    subprocess/memory/memory_io_serde.hpp
    subprocess/run_result.hpp

    util/class_traits.hpp
    util/macros.hpp
    util/linux.hpp
    util/expected.hpp
    util/unreachable.hpp
    util/error_types.hpp
    util/extra_formatters.hpp
    util/byte_vector.hpp
    util/byte_array.hpp
    util/timespec_operator_eq.hpp
    util/terminal_checks.hpp

    meta/always_false.hpp
    meta/functional_traits.hpp
    meta/count_if.hpp
    meta/remove_all_pointers.hpp

    logging.hpp

    registrars/global_registrar.hpp
    registrars/auto_registrars.hpp

    output/plaintext_serializer.hpp
    output/serializer.hpp
    output/sink.hpp
    output/stdout_sink.hpp

    symbols/elf_reader.hpp
    symbols/symbol.hpp
    symbols/symbol_table.hpp

    program/program.hpp

    user/cl_args.hpp

    api/asm_function.hpp
    api/asm_symbol.hpp
    api/asm_data.hpp
    api/asm_buffer.hpp
    api/test_base.hpp
    api/assignment.hpp
    api/metadata.hpp
    api/test_context.hpp
    api/macros.hpp
    api.hpp

    test_runner.hpp

    grading_session.hpp

    exceptions.hpp
)

set(
    INTERF_COMPILE_OPTS

    "$<$<COMPILE_LANGUAGE:C,CXX>:-Werror;-Wall;-Wextra;-Wpedantic;-Wshadow;-Wconversion>"

    # Permit usage of statment expression from macros, which makes error
    # propegation a lot nicer
    "$<$<COMPILE_LANGUAGE:C,CXX>:-Wno-gnu-statement-expression-from-macro-expansion>"

    # Enable debug symbols; disable optimizations
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-g3;-O0;-fno-omit-frame-pointer>"
    # Enable sanitizers in debug mode
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-fsanitize=address,undefined,signed-integer-overflow>"
    # disable PIE in debug mode, as ASLR messes with ASan pre-llvm17
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-no-pie>"

    # Enable optimizations and warnings are errors in release mode
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Release>>:-O3;-Werror>"
)

set(
    INTERF_LINK_OPTS

    # Disable optimizations in debug mode
    "$<$<CONFIG:Debug>:-O0;-fno-omit-frame-pointer>"

    # Enable sanitizers in debug mode
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-fsanitize=address,undefined,signed-integer-overflow>"
    # disable PIE in debug mode, as ASLR messes with ASan pre-llvm17
    "$<$<AND:$<COMPILE_LANGUAGE:C,CXX>,$<CONFIG:Debug>>:-no-pie>"
)

# TODO: Yeah, `.` is bad practice...
set(
    INTERF_INCLUDE_DIRS

    .
)

set(
    INTERF_LINK_LIBS

    fmt::fmt
    argparse
    spdlog
    nlohmann_json::nlohmann_json
    Boost::endian
    Boost::type_index
    Boost::mp11
    Boost::pfr
    Boost::describe
    Boost::stacktrace
    range-v3
    Microsoft.GSL::GSL
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

    util/terminal_checks.cpp

    output/plaintext_serializer.cpp
    output/stdout_sink.cpp

    registrars/global_registrar.cpp

    test_runner.cpp

    symbols/elf_reader.cpp
    symbols/symbol_table.cpp

    program/program.cpp

    user/cl_args.cpp

    api/assignment.cpp
    api/test_context.cpp
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

    # libelf options
    target_include_directories(${basename}_interface INTERFACE ${LIBELF_INCLUDE_DIRS})
    target_link_libraries(${basename}_interface INTERFACE ${LIBELF_LIBRARIES})
    target_compile_options(${basename}_interface INTERFACE ${LIBELF_CFLAGS_OTHER})

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

##### Professor version
create_targets(asmgrader_prof)
target_compile_definitions(
    asmgrader_prof_interface

    INTERFACE
    "PROFESSOR_VERSION"
)
