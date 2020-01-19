# Version file
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
version_header = configure_file(
  input: 'version.h.in',
  output: 'version.h',
  configuration: conf_data
)

radeontop_inc = include_directories('.')
radeontop_public_inc = include_directories('./radeontop')
