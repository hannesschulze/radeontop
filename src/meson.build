radeontop_files = files(
  'auth.c',
  'detect.c',
  'dump.c',
  'family_str.c',
  'radeon.c',
  'ticks.c',
  'ui.c',
  'context.c',
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

# Build the library
libradeontop = static_library(
  'radeontop',
  radeontop_files,
  dependencies: radeontop_deps,
  include_directories: [radeontop_inc, radeontop_public_inc],
)

libradeontop_dep = declare_dependency(
  link_with: libradeontop,
  include_directories: radeontop_public_inc,
)

if (enable_executable)
  executable(
    'radeontop',
    'radeontop.c',
    dependencies: libradeontop_dep,
    include_directories: radeontop_inc,
    install: true,
  )
endif
