project('radeontop', 'c', version: '1.2')

enable_nls = get_option('nls')
enable_xcb = get_option('xcb')
enable_amdgpu = get_option('amdgpu')
enable_executable = get_option('executable')

cc = meson.get_compiler('c')

radeontop_deps = [
  dependency('ncurses'),
  dependency('pciaccess'),
  dependency('threads'),
  cc.find_library('m', required: false),
]

if (enable_xcb)
  radeontop_deps += dependency('xcb')
  radeontop_deps += dependency('xcb-dri2')
  radeontop_deps += cc.find_library('dl', required: true)
endif

if (enable_amdgpu)
  radeontop_deps += dependency('libdrm', version: '>=2.4.63')
  radeontop_deps += dependency('libdrm_amdgpu', version: '>=2')
else
  radeontop_deps += dependency('libdrm')
endif

i18n = import('i18n')

subdir('include')
subdir('translations')
subdir('src')
subdir('man')
