project('zinc', 'cpp',
  #default_options: ['b_sanitize=address', 'b_lundef=false']
)

add_project_arguments(
  '-mbmi2',
  '-Werror', '-Wall', '-Wextra', '-Wpedantic',
  '-Wno-unused-parameter', '-Wno-unused-but-set-variable', '-Wno-unused-variable',
  #'-fsanitize=implicit-conversion', '-fsanitize=integer', '-fsanitize=undefined',
  '-std=c++17',
  language: 'cpp',
)

cxx = meson.get_compiler('cpp')
m_dep = cxx.find_library('m', required : false)

incdir = include_directories('libzinc')

zinc_test = executable('zinc-test', 'test/zinc-test.cc', include_directories: incdir, dependencies: m_dep)
test('zinc-test', zinc_test)

glfw_dep = dependency('glfw3')
cairo_dep = dependency('cairo')
cairo_test = executable('cairo-test', 'test/cairo-test.cc', include_directories: incdir, dependencies: [m_dep, cairo_dep, glfw_dep])
test('cairo-test', cairo_test)

install_subdir('libzinc', install_dir: 'include')
