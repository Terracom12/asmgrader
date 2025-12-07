# Modified from mesonbuild/WrapDB to disclude tests, cause why would I want those

project(
  'range-v3',
  'cpp',
  version: '0.12.0',
  license: 'Boost, libc++, Stepanov and McJones "Elements of Programming", SGI STL',
)

range_inc = include_directories('include')

range_dep = declare_dependency(
  include_directories: range_inc,
)
