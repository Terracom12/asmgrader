# asmgrader top-level project setup
# ==============================================================================

project(
  'asmgrader',
  ['cpp'],
  license : 'MIT',
  version : '0.4.4',
  meson_version : '>=0.55.0',
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
  boost_dep += [boost_subproj.dependency('boost_' + mod)]
endforeach

# =========
# argparse
#
# for command line argument parsing
# =========
argparse_dep = dependency('argparse')

# =========
# spdlog
#
# for fast runtime logging
# =========
spdlog_dep = dependency('spdlog')

# =========
# fmt
#
# for friendly string interpolation
# =========
fmt_dep = dependency('fmt')

# =========
# gsl (guidelines support library)
#
# for better code conformance
# =========
gsl_dep = dependency('microsoft_gsl')

# =========
# range-v3
#
# for "modern" range-based patterns
# =========
rangev3_dep = dependency('range-v3')

# =========
# elfio
#
# for parsing of ELF files
# =========
elfio_dep = dependency('elfio')

# =========
# cpptrace
#
# for human-readable backtraces
# =========
cpptrace_dep = dependency('cpptrace')

# =========
# libassert
#
# for human-readable assertions with tons of info
# =========
libasssert_dep = dependency('libassert')


# ==============================================================================
# Sub-Directories
# ==============================================================================

subdir('include')
subdir('src')

# project_target = executable(
#   meson.project_name(),
#   sources : project_sources,
#   dependencies: [rangev3_dep, fmt_dep],
# )
