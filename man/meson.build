a2x = find_program('a2x', required: false)

if (a2x.found())
  custom_target(
    'radeontop.1',
    input: 'radeontop.asc',
    output: 'radeontop.1',
    command: [ a2x, '-D', '@OUTDIR@', '-f', 'manpage', '@INPUT0@' ],
    install: false
  )
endif

install_man('radeontop.1')
