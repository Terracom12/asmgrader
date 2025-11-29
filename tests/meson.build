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

asm_tests_target = executable(
  'asm_tests',
  sources: 'resources/simple_asm_@0@.s'.format(arch_suffix),
  cpp_args: [
    '-nostartfiles'
  ],
  link_args: [
    '-nostartfiles'
  ],
)
asm_tests_weird_name_target = custom_target(
  'asm_tests_weird_name',
  build_by_default: true,
  command: ['cp', '@INPUT@', '@OUTPUT@'],
  input: asm_tests_target,
  output: '(asm_tests () weird (name) ~`',
)

tests_defines = [
  '-DASM_TESTS_EXEC="@0@"'.format(asm_tests_target.full_path()),
  '-DASM_TESTS_WEIRD_NAME_EXEC="@0@"'.format(asm_tests_weird_name_target.full_path()),
  '-DRESOURCES_DIR="@0@"'.format(meson.current_source_dir() / 'resources'),
]

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

test('unittests', asmgrader_tests_target)
test(
  'unittests.static',
  find_program('meson'),
  args: ['compile', asmgrader_static_tests_target.name()],
  workdir: meson.project_build_root(),
)
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
