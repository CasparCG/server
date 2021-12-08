linux_sources = [
    'os/linux/filesystem.cpp',
    'os/linux/prec_timer.cpp',
    'os/linux/thread.cpp',
]

common_lib = static_library(
        'common',
        'diagnostics/graph.cpp',
        'gl/gl_check.cpp',
        'base64.cpp',
        'env.cpp',
        'filesystem.cpp',
        'log.cpp',
        'tweener.cpp',
        'utf.cpp',
        linux_sources,
        dependencies: [boost_dep, opengl_dep, glew_dep],
        include_directories: include_root,

        cpp_pch : [ 'pch/stdafx.h']
)
