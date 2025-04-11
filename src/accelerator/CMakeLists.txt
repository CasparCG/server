cmake_minimum_required (VERSION 3.16)
project (accelerator)

set(SOURCES
	ogl/image/image_kernel.cpp
	ogl/image/image_mixer.cpp
	ogl/image/image_shader.cpp
	ogl/image/frame_converter.cpp

	ogl/util/buffer.cpp
	ogl/util/compute_shader.cpp
	ogl/util/device.cpp
	ogl/util/shader.cpp
	ogl/util/shaders_source.cpp
	ogl/util/texture.cpp
	ogl/util/matrix.cpp
	ogl/util/transforms.cpp

	accelerator.cpp
)
set(HEADERS
	ogl/image/image_kernel.h
	ogl/image/image_mixer.h
	ogl/image/image_shader.h
	ogl/image/frame_converter.h

	ogl/util/buffer.h
	ogl/util/compute_shader.h
	ogl/util/device.h
	ogl/util/shader.h
	ogl/util/shaders_source.h
	ogl/util/texture.h
	ogl/util/matrix.h
	ogl/util/transforms.h

	accelerator.h
	StdAfx.h
)

compile_gl_shaders(accelerator-shaders
    NAMESPACE "caspar::accelerator::ogl"
    SOURCES
        "ogl/image/shader.vert"
        "ogl/image/shader.frag"
        "ogl/image/shader_compute_to_rgba.comp"
        "ogl/image/shader_compute_from_rgba.comp"
    )

casparcg_add_library(accelerator SOURCES ${SOURCES} ${HEADERS})
add_dependencies(accelerator accelerator-shaders)
target_include_directories(accelerator PRIVATE
    ..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLEW_INCLUDE_PATH}
    ${SFML_INCLUDE_PATH}
    )
target_precompile_headers(accelerator PRIVATE "StdAfx.h")

source_group(sources ./.*)
source_group(sources\\cpu\\image cpu/image/.*)
source_group(sources\\cpu\\util cpu/util/.*)
source_group(sources\\ogl\\image ogl/image/.*)
source_group(sources\\ogl\\util ogl/util/.*)

target_link_libraries(accelerator common core)
