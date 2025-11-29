tests_srcs = [
  'test_static_expected.cpp',
  'test_static_formatters.cpp',
  'test_static_tuple_matcher.cpp',
  'test_expression_inspection.cpp',
  'test_syntax_highlighter.cpp',
  'test_cconstexpr.cpp',
  'test_static_string.cpp',
  'test_static_byte.cpp',
  'test_bit_casts.cpp',
  'test_metadata.cpp',
  'test_subprocess.cpp',
  'test_expected.cpp',
  'test_symbol_reader.cpp',
  'test_memory_io.cpp',
  'test_program.cpp',
  'test_database_reader.cpp',
  'test_registers_state.cpp',
  'test_file_searcher.cpp',
  'test_byte_ranges.cpp',
  'test_tempfile.cpp',
]

static_tests_srcs = []

fs = import('fs')
foreach t : tests_srcs
  file_content = fs.read(t, encoding: 'utf-8')
  if file_content.contains('STATIC_REQUIRE')
    static_tests_srcs += [t]
  endif
endforeach


inc = include_directories('../src')

#   ##### Simple assembly executable
#   # add_asm_arch_specific_impl(['asm_tests', 'resources/simple_asm'])
#
#   asm_tests_weird_name_exec = '(asm_tests () weird (name) ~`'
#   # add_custom_command(['TARGET', 'asm_tests', 'POST_BUILD', 'COMMAND', 'cp', '$<TARGET_FILE:asm_tests>', '${asm_tests_weird_name_exec}', 'VERBATIM'])
#   ##### Test runner executable

asmgrader_tests_target = executable(
  'asmgrader_tests',
  sources: ['catch2_custom.hpp', tests_srcs, 'catch_main.cpp'],
  dependencies: [catch2_dep, asmgrader_dep],
  include_directories: inc,
  cpp_args: ['-DCATCH_CONFIG_RUNTIME_STATIC_REQUIRE'],
)

#   # add_dependencies(['asmgrader_tests', 'asm_tests'])
#   # target_compile_definitions(['asmgrader_tests', 'PUBLIC', 'CATCH_CONFIG_RUNTIME_STATIC_REQUIRE', 'PRIVATE', 'ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\"', 'RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"', 'ASM_TESTS_WEIRD_NAME_EXEC=R\"abc(${asm_tests_weird_name_exec})abc\"'])
#   # target_link_libraries(['asmgrader_tests', 'PUBLIC', 'asmgrader_core', 'PRIVATE', 'Catch2::Catch2', 'asmgrader_core_interface', 'asmgrader_warnings'])
#   # TODO: Migh want to look into changing this later.
#   # Just attempt to compile all tests in the constexpr version for simplicity,
#   # since there really aren't that many.
#   asmgrader_constexpr_tests_exe = executable('asmgrader_constexpr_tests', 'EXCLUDE_FROM_ALL', tests_srcs)
#   # target_link_libraries(['asmgrader_constexpr_tests', 'PUBLIC', 'asmgrader_core', 'PRIVATE', 'Catch2::Catch2WithMain', 'asmgrader_core_interface', 'asmgrader_warnings'])
#   asm_tests_weird_name_exec = '${CMAKE_CURRENT_BINARY_DIR}/(asm_tests () weird \\(name) ~`'
#   # add_custom_command(['TARGET', 'asmgrader_tests', 'POST_BUILD', 'COMMAND', 'cp', '$<TARGET_FILE:asm_tests>', '${asm_tests_weird_name_exec}', 'VERBATIM'])
#   # target_compile_definitions(['asmgrader_constexpr_tests', 'PRIVATE', 'ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\"', 'RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"', 'ASM_TESTS_WEIRD_NAME_EXEC=R\"abc(${asm_tests_weird_name_exec})abc\"'])
#   ##### Built executables of the CLI library with no user-defined student tests
#   # Used to check whether basic flags (--help and --version) work properly
#   asmgrader_dumb_cli_exe = executable('asmgrader_dumb_cli', 'dumb_assignment.cpp')
#   # target_compile_definitions(['asmgrader_dumb_cli', 'PRIVATE', 'ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\"'])
#   # target_link_libraries(['asmgrader_dumb_cli', 'PRIVATE', 'asmgrader_core_with_main', 'asmgrader_warnings'])
#   asmgrader_dumb_profcli_exe = executable('asmgrader_dumb_profcli', 'dumb_assignment.cpp')
#   # target_compile_definitions(['asmgrader_dumb_profcli', 'PRIVATE', 'ASM_TESTS_EXEC=\"$<TARGET_FILE:asm_tests>\"'])
#   # target_link_libraries(['asmgrader_dumb_profcli', 'PRIVATE', 'asmgrader_prof_core_with_main', 'asmgrader_warnings'])
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
