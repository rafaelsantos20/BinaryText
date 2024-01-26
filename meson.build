project('binarytext', 'cpp', version: '1.0', default_options: ['cpp_std=c++20', 'buildtype=release', 'warning_level=3', 'werror=false'])

if meson.get_compiler('cpp').get_argument_syntax() == 'gcc'
    add_project_arguments(['-Wshadow', '-Wconversion', '-Warith-conversion', '-Wdouble-promotion'], language: 'cpp')
endif

sources = files('main.cpp', 'Utility.cpp')

executable('binarytext', sources)
