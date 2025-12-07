# asmgrader top-level project setup
# ==============================================================================

project(
  'asmgrader',
  ['cpp'],
  license : 'MIT',
  version : '0.4.4',
  meson_version : '>=1.8.5',
  default_options : [
    'warning_level=3',
    'default_library=static',
    'cpp_std=c++20',
  ],
)

# ==============================================================================
# Dependencies
# ==============================================================================

# =========
# Boost
#
# ...is *special*
# =========
boost_module_names = [
  'endian',
  'type_index',
  'describe',
  'mp11',
  'pfr',
  # 'stacktrace',
  'preprocessor',
]
cmake = import('cmake')
boost_opts = cmake.subproject_options()
boost_opts.add_cmake_defines({
  'BOOST_SHARED_LIBS': 'OFF',
  'BOOST_SKIP_INSTALL_RULES': 'ON',
  'BOOST_INCLUDE_LIBRARIES': ';'.join(boost_module_names),
})
boost_subproj = cmake.subproject('boost',
  options: boost_opts
)
boost_dep = []
foreach mod : boost_module_names
  boost_dep += [boost_subproj.dependency('boost_' + mod, include_type: 'system')]
endforeach

# =========
# argparse
#
# for command line argument parsing
# =========
argparse_dep = dependency('argparse', include_type: 'system')

# =========
# fmt
#
# for friendly string interpolation
# =========
fmt_dep = dependency('fmt', include_type: 'system')

# =========
# spdlog
#
# for fast runtime logging
# =========
spdlog_dep = dependency(
  'spdlog',
  default_options: ['std_format=disabled']
)


# =========
# gsl (guidelines support library)
#
# for better code conformance
# =========
gsl_dep = dependency('microsoft_gsl', include_type: 'system')

# =========
# range-v3
#
# for "modern" range-based patterns
# =========
rangev3_dep = dependency('range-v3', include_type: 'system')

# =========
# elfio
#
# for parsing of ELF files
# =========
elfio_dep = dependency('elfio', include_type: 'system')

# =========
# cpptrace
#
# for human-readable backtraces
# =========
cpptrace_dep = dependency('cpptrace', include_type: 'system')

# =========
# libassert
#
# for human-readable assertions with tons of info
# =========
libasssert_dep = dependency('libassert', include_type: 'system')

# =========
# Catch2
#
# for testing
# =========
catch2_dep = dependency(
  'catch2',
  include_type: 'system',
  default_options: ['tests=false'],
)


# ==============================================================================
# Configuration data
# ==============================================================================

split_version = meson.project_version().split('.')
git_hash_cmd = run_command('git', 'log', '-n1', '--format=%H', capture: true, check: false)

if git_hash_cmd.returncode() == 0
  git_hash_str = git_hash_cmd.stdout().strip()
else
  git_hash_str = ''
endif

# Used to define macros in the file version_config.hpp.in under include/

asmgrader_conf_data = configuration_data({
  'ASMGRADER_VERSION_MAJOR': split_version[0],
  'ASMGRADER_VERSION_MINOR': split_version[1],
  'ASMGRADER_VERSION_PATCH': split_version[2],
  'ASMGRADER_VERSION_GIT_HASH': git_hash_str,
  'ASMGRADER_BUILD_TYPE': get_option('buildtype'),
  'ASMGRADER_IMPL_VERSION_INFO': get_option('impl_version_info'),
  'ASMGRADER_DEPENDENCY_VERSIONS_STR': '' # TODO
})

# ==============================================================================
# Sub-Directories
# ==============================================================================

subdir('include')
subdir('src')

if not meson.is_subproject()
  subdir('tests')
  subdir('docs')
endif
