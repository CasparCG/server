#accelerator_includes = include_directories(
    #    meson.source_root())

accelerator_shaders = [
    ['ogl_image_vertex.h', 'ogl/image/shader.vert', 'vertex_shader'],
    ['ogl_image_fragment.h', 'ogl/image/shader.frag', 'fragment_shader']
]
accelerator_shader_targets = []

foreach shader : accelerator_shaders
    accelerator_shader_targets += custom_target(shader[2], input: shader[1], output: shader[0], command: [bin2c, 'caspar::accelerator::ogl', shader[2], '@INPUT@'], capture: true)
endforeach

accelerator_lib = static_library(
                    'accelerator',
                    'ogl/image/image_kernel.cpp',
                    'ogl/image/image_mixer.cpp',
                    'ogl/image/image_shader.cpp',

                    'ogl/util/buffer.cpp',
                    'ogl/util/device.cpp',
                    'ogl/util/shader.cpp',
                    'ogl/util/texture.cpp',

                    'accelerator.cpp',
                    #'vulkan.cpp',
                    #'vulkan/validation.cpp',
                    accelerator_shader_targets,

                    link_with: [common_lib, core_lib],

                    dependencies: [tbb_dep, boost_dep, sfml_dep, opengl_dep, glew_dep],
                    cpp_pch : ['pch/StdAfx.h', 'pch/StdAfx.cpp'],
                    include_directories: include_root
)

