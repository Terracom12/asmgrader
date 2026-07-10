# core_link_libs_public = ['fmt::fmt', 'spdlog', 'Boost::endian', 'Boost::type_index', 'Boost::mp11', 'Boost::pfr', 'Boost::describe', 'Boost::stacktrace', 'Boost::preprocessor', 'range-v3', 'Microsoft.GSL::GSL', 'libassert::assert']
# core_link_libs_private = ['asmgrader_options', 'asmgrader_warnings', 'argparse', 'nlohmann_json::nlohmann_json', 'elfio::elfio']

asmgrader_private_inc = include_directories('.')

asmgrader_deps = [
  boost_dep,
  argparse_dep,
  spdlog_dep,
  fmt_dep,
  gsl_dep,
  rangev3_dep,
  elfio_dep,
  cpptrace_dep,
  libasssert_dep,
]

asmgrader_sources = [
  'subprocess/subprocess.cpp',
  'subprocess/traced_subprocess.cpp',
  'subprocess/tracer.cpp',
  'subprocess/memory/memory_io_base.cpp',
  'subprocess/memory/ptrace_memory_io.cpp',
  'common/terminal_checks.cpp',
  'output/plaintext_serializer.cpp',
  'output/stdout_sink.cpp',
  'registrars/global_registrar.cpp',
  'test_runner.cpp',
  'multi_student_runner.cpp',
  'symbols/elf_reader.cpp',
  'symbols/symbol_table.cpp',
  'program/program.cpp',
  'user/cl_args.cpp',
  'api/tempfile.cpp',
  'api/process_statistics.cpp',
  'api/assignment.cpp',
  'api/test_context.cpp',
  'api/syntax_highlighter.cpp',
  'api/stringize.cpp',
  'database_reader.cpp',
  'user/file_searcher.cpp',
  'user/assignment_file_searcher.cpp',
  'app/professor_app.cpp',
  'app/student_app.cpp',
  'version.cpp',
  'logging.cpp',
]

asmgrader_with_main_sources = ['main.cpp']

asmgrader_defines = []
if get_option('professor_mode')
  asmgrader_defines += ['-DPROFESSOR_VERSION']
endif

# ==============================================================================
# Project and dependency setup
# ==============================================================================

asmgrader_target = library(
  'asmgrader',
  sources: asmgrader_sources,
  include_directories: [asmgrader_public_inc, asmgrader_private_inc],
  dependencies: asmgrader_deps,
  cpp_args: asmgrader_defines,
)
asmgrader_dep = declare_dependency(
  compile_args: asmgrader_defines,
  include_directories: asmgrader_public_inc,
  link_with: asmgrader_target,
  dependencies: asmgrader_deps,
)
meson.override_dependency('asmgrader', asmgrader_dep)

asmgrader_with_main_target = library(
  'asmgrader_with_main',
  sources: asmgrader_with_main_sources,
  include_directories: [asmgrader_public_inc, asmgrader_private_inc],
  dependencies: [asmgrader_deps, asmgrader_dep],
)
asmgrader_with_main_dep = declare_dependency(
  include_directories: asmgrader_public_inc,
  link_with: asmgrader_with_main_target,
  dependencies: asmgrader_deps,
)
meson.override_dependency('asmgrader_with_main', asmgrader_with_main_dep)
