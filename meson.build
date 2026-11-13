project(
  'tty-greet',
  'cpp',
  version: '0.1.0',
  default_options: [
    'prefix=/usr',
    'cpp_std=c++17',
    'warning_level=3',
    'buildtype=minsize',
    'strip=true',
    'cpp_eh=none',
    'cpp_rtti=false',
  ],
)

# Size: drop unused sections and unreferenced libs.
add_project_arguments('-ffunction-sections', '-fdata-sections', '-fno-asynchronous-unwind-tables', language: 'cpp')
add_project_link_arguments('-Wl,--gc-sections,--as-needed', language: 'cpp')

inc = include_directories('src')

ncurses_dep = dependency('ncursesw')

core = static_library(
  'ttygreet',
  'src/session.cpp',
  'src/launch.cpp',
  'src/ui.cpp',
  include_directories: inc,
  dependencies: ncurses_dep,
)

executable(
  'tty-greet',
  'src/main.cpp',
  include_directories: inc,
  link_with: core,
  dependencies: ncurses_dep,
  install: true,
)

install_data('contrib/profile.sh', install_dir: get_option('datadir') / 'tty-greet')

core_test = executable(
  'test_core',
  'tests/test_core.cpp',
  include_directories: inc,
  link_with: core,
  dependencies: ncurses_dep,
)

test('core', core_test, args: [meson.current_source_dir() / 'tests' / 'fixtures'])
