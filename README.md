RadeonTop – meson wrapper
=========

Original repo: https://github.com/clbr/radeontop

This fork contains a meson wrapper for radeontop, building a separate library
which can be used as a subproject

Supported build options: `nls`, `xcb`, `amdgpu` (see the original README.md), 
`executable` (to build the executable).

Use `libradeontop_dep` when embedding this repo as a meson subproject.
