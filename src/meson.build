project('CasparCG', 'cpp', version: '2.3.3', default_options: ['cpp_std=c++17'])

caspar_version = meson.project_version().split('.')
caspar_gen = caspar_version[0]
caspar_major = caspar_version[1]
caspar_minor = caspar_version[2]
caspar_tag = 'Dev'

version_conf = configuration_data()
version_conf.set('caspar_gen', caspar_gen)
version_conf.set('caspar_major', caspar_major)
version_conf.set('caspar_minor', caspar_minor)
version_conf.set('caspar_tag', caspar_tag)

vcs_version_h = vcs_tag(
    input: 'vcs_version_h.in',
    output: 'vcs_version.h'
)

version_h = configure_file(
    input: 'version_h.tmpl',
    output: 'version.h',
    configuration: version_conf)

cpp = meson.get_compiler('cpp')

boost_dep = dependency('boost', modules : ['system', 'thread', 'chrono', 'filesystem', 'log', 'locale', 'regex', 'date_time', 'coroutine'])
opengl_dep = dependency('gl')
freeimage_dep = dependency('FreeImage', required: false)
if not freeimage_dep.found()
    freeimage_dep = cpp.find_library('freeimage', has_headers: ['FreeImage.h'])
endif
glew_dep  = dependency('glew')
tbb_dep   = [dependency('tbb')]

#On ubuntu tbbmalloc_proxy is not found via pkgconfig/cmake, so fallback to finding via classic way
tbb_malloc_proxy = dependency('tbbmalloc_proxy', required:false)
if not tbb_malloc_proxy.found()
    tbb_malloc_proxy = cpp.find_library('tbbmalloc_proxy', has_headers: ['tbb/tbbmalloc_proxy.h'])
endif
tbb_dep += tbb_malloc_proxy

tbb_malloc = dependency('tbbmalloc', required:false)
if not tbb_malloc.found()
    tbb_malloc_proxy = cpp.find_library('tbbmalloc', has_headers: ['tbb/scalable_allocator.h'])
endif
tbb_dep += tbb_malloc

openal_dep = dependency('openal')
sfml_dep  = [dependency('sfml-window'), dependency('sfml-graphics'), dependency('sfml-system')]
x11_dep   = dependency('x11')
cef_dep   = dependency('cef', required: false)


cef_path = '/opt/cef'
have_cef = cef_dep.found()
cef_include = []
if not cef_dep.found()
    cef_dep  = [
        cpp.find_library('cef_dll_wrapper', dirs: join_paths(cef_path, 'Release'), required: false),
        cpp.find_library('cef', dirs: join_paths(cef_path, 'Release'), has_headers: ['include/cef_app.h', 'include/cef_version.h'], header_include_directories: include_directories(cef_path), required: false)
    ]
    if cef_dep[0].found() and cef_dep[1].found()
        cef_include += include_directories(cef_path)
        have_cef = true
    endif
endif
ffmpeg_deps = [
    dependency('libavcodec'),
    dependency('libavdevice'),
    dependency('libavfilter'),
    dependency('libavutil'),
    dependency('libavformat'),
    dependency('libswresample'),
    dependency('libswscale'),
]

add_project_arguments(
    [
        '-DSFML_STATIC',
        '-DUNICODE',
        '-D_UNICODE',
        '-DGLEW_NO_GLU',
        '-DBOOST_NO_SWPRINTF',
        '-DTBB_USE_CAPTURED_EXCEPTION=1',
    ],
    language: 'cpp'
)

#add_project_arguments([
#    '-DSFML_STATIC',
#    '-DUNICODE',
#    '-D_UNICODE',
#    '-DGLEW_NO_GLU',
#    '-D__NO_INLINE__',
#    '-DBOOST_NO_SWPRINTF',
#    '-DTBB_USE_CAPTURED_EXCEPTION=1',
#    '-DNDEBUG'],
#    language: 'cpp'
#)
add_project_arguments(cpp.get_supported_arguments(
    [
        '-fnon-call-exceptions',
        '-msse3',
        '-msse4',
        '-msse4.1',
        '-Wno-deprecated-declarations',
        '-Wno-write-strings',
        '-Wno-multichar',
        '-Wno-cpp',
        '-Wno-unknown-pragmas',
        '-Wno-terminate',
        '-Wno-unused'
    ]),
    language: 'cpp'
)

include_root = include_directories('.')

modules_include = []
modules_cli_intercept = []
modules_init = []
modules_uninit = []

subdir('tools')
subdir('common')
subdir('core')
subdir('accelerator')
subdir('protocol')
subdir('modules')
subdir('shell')
