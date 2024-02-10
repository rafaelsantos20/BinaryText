project('binarytext', 'cpp', version: '1.0', default_options: ['cpp_std=c++20', 'buildtype=release', 'warning_level=3', 'werror=false'])

compiler = meson.get_compiler('cpp')

if compiler.get_argument_syntax() == 'gcc'
    if compiler.has_argument('-Wshadow', required: false)
        add_project_arguments('-Wshadow', language: 'cpp')
    endif

    if compiler.has_argument('-Wconversion', required: false)
        add_project_arguments('-Wconversion', language: 'cpp')
    endif

    if compiler.has_argument('-Warith-conversion', required: false)
        add_project_arguments('-Warith-conversion', language: 'cpp')
    endif

    if compiler.has_argument('-Wdouble-promotion', required: false)
        add_project_arguments('-Wdouble-promotion', language: 'cpp')
    endif
endif

sources = files('main.cpp', 'Utility.cpp')

executable('binarytext', sources)
