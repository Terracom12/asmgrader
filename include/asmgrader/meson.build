# ==============================================================================
# Configure version info
#
# This file's sole purpose is to generate config.hpp from config.hpp.in
# ==============================================================================

split_version = meson.project_version().split('.')
git_hash_cmd = run_command('git', 'log', '-n1', '--format=%H', capture: true, check: false)

if git_hash_cmd.returncode() == 0
  git_hash_str = git_hash_cmd.stdout().strip()
else
  git_hash_str = ''
endif

# Used to define macros in the file config.hpp.in under include/

asmgrader_deps_conf_str = ''
foreach name,dep : asmgrader_named_deps
  asmgrader_deps_conf_str += '@0@ @1@ '.format(
    name,
    dep.version()
  )
endforeach
asmgrader_conf_data = configuration_data({
  'ASMGRADER_VERSION_MAJOR': split_version[0],
  'ASMGRADER_VERSION_MINOR': split_version[1],
  'ASMGRADER_VERSION_PATCH': split_version[2],
  'ASMGRADER_VERSION_GIT_HASH': git_hash_str,
  'ASMGRADER_BUILD_TYPE': get_option('buildtype'),
  'ASMGRADER_IMPL_VERSION_INFO': get_option('impl_version_info'),
  'ASMGRADER_DEPENDENCY_VERSIONS_STR': asmgrader_deps_conf_str,
})


configure_file(
  input: 'config.hpp.in',
  output: 'config.hpp',
  configuration: asmgrader_conf_data
)
