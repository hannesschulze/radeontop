radeontop_files = files(
  'auth.c',
  'detect.c',
  'dump.c',
  'family_str.c',
  'radeon.c',
  'radeontop.c',
  'ticks.c',
  'ui.c',
)

if (enable_xcb)
  add_project_arguments('-DENABLE_XCB=1', language: 'c')
endif

if (enable_nls)
  add_project_arguments('-DENABLE_NLS=1', language: 'c')
endif

if (enable_amdgpu)
  add_project_arguments('-DENABLE_AMDGPU=1', language: 'c')
  add_project_arguments('-DHAS_DRMGETDEVICE=1', language: 'c')
  if (dependency('libdrm_amdgpu').version().version_compare('>=2.4.79'))
    add_project_arguments('-DHAS_AMDGPU_QUERY_SENSOR_INFO=1', language: 'c')
  endif

  radeontop_files += 'amdgpu.c'
endif

executable(
  'radeontop',
  radeontop_files,
  dependencies: radeontop_deps,
  include_directories: radeontop_inc,
  install: true,
)