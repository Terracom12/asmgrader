doxygen = find_program('doxygen', required: false)
if not doxygen.found()
  warning('doxygen executable not found - documentation building disabled')
  subdir_done()
endif
dot = find_program('dot', required: false)
if not dot.found()
  warning('dot executable not found - documentation building disabled')
  subdir_done()
endif

docs_root = meson.current_source_dir()
doxyfile_conf = configuration_data({
  'DOXYGEN_PROJECT_NAME': meson.project_name(),
  'DOXYGEN_PROJECT_VERSION': meson.project_version(),
  'DOXYGEN_USE_MDFILE_AS_MAINPAGE': meson.project_source_root() / 'README.md',
  'DOXYGEN_EXCLUDE_PATTERNS': '@0@ tests'.format(meson.project_build_root()),
  'DOXYGEN_HTML_EXTRA_STYLESHEET': ' '.join([
    docs_root / 'doxygen-awesome-css' / 'doxygen-awesome.css',
    docs_root / 'doxygen-awesome-css' / 'doxygen-awesome-sidebar-only.css',
    docs_root / 'doxygen-awesome-css' / 'doxygen-awesome-sidebar-only-darkmode-toggle.css',
  ]),
  'DOXYGEN_HTML_HEADER': docs_root / 'header.html',
  'DOXYGEN_HTML_EXTRA_FILES': ' '.join([
    docs_root / 'doxygen-awesome-css' / 'doxygen-awesome-darkmode-toggle.js',
    docs_root / 'doxygen-awesome-css' / 'doxygen-awesome-paragraph-link.js',
    docs_root / 'doxygen-awesome-fragment-copy-button-modified.js',
  ]),
  'DOXYGEN_OUTPUT_DIRECTORY': meson.current_build_dir(),
  'DOXYGEN_IMAGE_PATH': docs_root / 'images',
  'DOXYGEN_INPUT': ' '.join([
    meson.project_source_root() / 'src',
    meson.project_source_root() / 'include',
    meson.project_source_root() / 'README.md',
    docs_root,
  ]),
 
})
doxyfile = configure_file(
  input: 'Doxyfile.in',
  output: 'Doxyfile',
  configuration: doxyfile_conf,
)

custom_target(
  'docs',
  build_by_default: false,
  build_always_stale: true,
  command: [doxygen, doxyfile],
  output: ['.']
)
