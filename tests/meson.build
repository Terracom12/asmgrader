# unittests and cli integration testing
# ==============================================================================
#
# ==============================================================================
# Variables
# ==============================================================================

# source-file : assignment-name
assignment_sources = {
  'assignments/simple.cpp': 'simple'
}

tests_srcs = [
  'test_byte_ranges.cpp',
  'test_database_reader.cpp',
  'test_expected.cpp',
  'test_file_searcher.cpp',
  'test_memory_io.cpp',
  'test_program.cpp',
  'test_static_bit_casts.cpp',
  'test_static_byte.cpp',
  'test_static_cconstexpr.cpp',
  'test_static_expected.cpp',
  'test_static_expression_inspection.cpp',
  'test_static_formatters.cpp',
  'test_static_metadata.cpp',
  'test_static_registers_state.cpp',
  'test_static_string.cpp',
  'test_static_subprocess.cpp',
  'test_static_tuple_matcher.cpp',
  'test_symbol_reader.cpp',
  'test_syntax_highlighter.cpp',
  'test_tempfile.cpp',
]

static_tests_srcs = []

foreach t : tests_srcs
  if t.contains('static')
    static_tests_srcs += [t]
  endif
endforeach

inc = include_directories('../src')

arch_suffix = build_machine.cpu_family()
if arch_suffix not in ['x86_64', 'aarch64']
  error('Unsupported build CPU family:', build_cpu)
endif

# ==============================================================================
# Targets
# ==============================================================================

# =========
# Simple executable built from asm
# also copied to "weird" name to test path resolution
# =========
asm_tests_target = executable(
  'asm_tests',
  sources: 'resources/simple_asm_@0@.s'.format(arch_suffix),
  cpp_args: [
    '-nostartfiles'
  ],
  link_args: [
    '-nostartfiles'
  ],
  override_options: [
    'b_sanitize=none',
  ],
)
asm_tests_weird_name_target = custom_target(
  'asm_tests_weird_name',
  build_by_default: true,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  input: asm_tests_target,
  output: '(asm_tests () weird (name) ~`',
)

# =========
# Macros defined for unit tests so they know where to find
# asm executables and the resources/ dir
# =========
tests_defines = [
  '-DASM_TESTS_EXEC="@0@"'.format(asm_tests_target.full_path()),
  '-DASM_TESTS_WEIRD_NAME_EXEC="@0@"'.format(asm_tests_weird_name_target.full_path()),
  '-DRESOURCES_DIR="@0@"'.format(meson.current_source_dir() / 'resources'),
]

# =========
# Normal unit tests target built with catch2_with_main
# All compile-time tests are transformed to runtime by:
#   STATIC_REQUIRE -> REQUIRE
# =========
asmgrader_tests_target = executable(
  'asmgrader_tests',
  sources: ['catch2_custom.hpp', tests_srcs, 'catch_main.cpp'],
  dependencies: [catch2_dep, asmgrader_dep],
  include_directories: inc,
  cpp_args: [
    '-DCATCH_CONFIG_RUNTIME_STATIC_REQUIRE',
    tests_defines,
  ],
)

# =========
# Static (compile-time) tests
# Only built as a seperate test target
# =========
asmgrader_static_tests_target = library(
  'asmgrader_static_tests',
  build_by_default: false,
  sources: ['catch2_custom.hpp', static_tests_srcs],
  dependencies: [catch2_dep, asmgrader_dep],
  include_directories: inc,
  cpp_args: [
    tests_defines,
  ],
)

# =========
# Full cli app build for integration tests
# =========
asmgrader_cli = executable(
  'asmgrader_cli',
  sources: assignment_sources.keys(),
  dependencies: asmgrader_with_main_dep,
  cpp_args: [
    tests_defines,
  ],
)

# ==============================================================================
# Tests
# ==============================================================================

# =========
# Unit tests and their static (compile-time) counterpart
# =========
test('unittests', asmgrader_tests_target)
test(
  'unittests.static',
  find_program('meson'),
  args: ['compile', asmgrader_static_tests_target.name()],
  workdir: meson.project_build_root(),
)

# =========
# Very basic cli tests
#
# these will *only* check for a 0 return code
# =========

test(
  'cli.has_help', 
  asmgrader_cli,
  args: ['--help'],
  workdir: meson.current_build_dir(),
)
test(
  'cli.has_version', 
  asmgrader_cli,
  args: ['--version'],
  workdir: meson.current_build_dir(),
)

test(
  'cli.bad_assignment_name',
  asmgrader_cli,
  args: ['534e3b148d1bf2da0b9076b8b70315c842b39589'],
  workdir: meson.current_build_dir(),
  should_fail: true,
)

# Every test assignment is expected to work against the asm program for now
foreach _,assignment_name : assignment_sources
  test(
    'cli.assignment.' + assignment_name,
    asmgrader_cli,
    args: [assignment_name, '--file', asm_tests_target],
    workdir: meson.current_build_dir(),
  )

  test(
    'cli.assignment.' + assignment_name + '.infer_exec',
    asmgrader_cli,
    args: [assignment_name],
    workdir: meson.current_build_dir(),
  )

  # student mode execs should be able to infer assignment from directory contents
  if not get_option('professor_mode')
    test(
      'cli.assignment.' + assignment_name + '.infer-all',
      asmgrader_cli,
      workdir: meson.current_build_dir(),
      suite: ['student_mode'],
    )
  endif
endforeach


#   # Provide a simple smoke test to make sure that the CLI works and can display a --help message
#   test('NAME', 'cli.has_help', 'COMMAND', 'asmgrader_dumb_cli', '--help')
#   test('NAME', 'prof_cli.has_help', 'COMMAND', 'asmgrader_dumb_profcli', '--help')
#   # Provide a test to verify that the version being reported from the application
#   # matches the version given to CMake. This will be important once you package
#   # your program. Real world shows that this is the kind of simple mistake that is easy
#   # to make, but also easy to test for.
#   test('NAME', 'cli.version_matches', 'COMMAND', 'asmgrader_dumb_cli', '--version')
#   test('NAME', 'prof_cli.version_matches', 'COMMAND', 'asmgrader_dumb_profcli', '--version')
#   # set_tests_properties(['cli.version_matches', 'prof_cli.version_matches', 'PROPERTIES', 'PASS_REGULAR_EXPRESSION', '${ASMGRADER_VERSION}'])
#   # Provide a simple smoke test to verify that the simple "dumb" tests all pass
#   # Verify when both inferring file name and when specifying
#   test('NAME', 'cli.dumb_tests_pass', 'COMMAND', 'asmgrader_dumb_cli', 'thing', '--file', '$<TARGET_FILE:asm_tests>')
#   test('NAME', 'prof_cli.dumb_tests_pass', 'COMMAND', 'asmgrader_dumb_profcli', 'thing', '--file', '$<TARGET_FILE:asm_tests>')
#   test('NAME', 'cli.infer_exec_name', 'COMMAND', 'asmgrader_dumb_cli', 'thing', 'WORKING_DIRECTORY', cmake_current_binary_dir)
#   test('NAME', 'prof_cli.infer_exec_name', 'COMMAND', 'asmgrader_dumb_profcli', 'thing', 'WORKING_DIRECTORY', cmake_current_binary_dir)
#   # Provide a simple smoke test to verify that the student mode is capable of inferring the lab name based on the exec
#   test('NAME', 'cli.infer_assignment', 'COMMAND', 'asmgrader_dumb_cli', 'WORKING_DIRECTORY', cmake_current_binary_dir)
#   # add_dependencies(['asmgrader_tests', 'asmgrader_dumb_cli', 'asmgrader_dumb_profcli'])
#   # !!DISCLAIMER!!
#   # This was adapted from the following source
#   #   cpp-best-practices/cmake_template
#   #   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   #   Courtesy of Jason Turner
#   # catch_discover_tests(['asmgrader_tests', 'TEST_PREFIX', 'unittests.', 'REPORTER', 'JUnit', 'OUTPUT_DIR', asmgrader_source_dir, '/reports', 'OUTPUT_PREFIX', 'junit-unittests.', 'OUTPUT_SUFFIX', '.xml', 'EXTRA_ARGS', '--reporter console::out=-', '--invisibles'])
#   test('NAME', 'asmgrader_constexpr_tests', 'COMMAND', cmake_make_program, 'asmgrader_constexpr_tests', 'WORKING_DIRECTORY', asmgrader_binary_dir)
