cmake_minimum_required (VERSION 3.16)
project (accelerator)

set(SOURCES
	ogl/image/image_kernel.cpp
	ogl/image/image_mixer.cpp
	ogl/image/image_shader.cpp

	ogl/util/buffer.cpp
	ogl/util/device.cpp
	ogl/util/shader.cpp
	ogl/util/texture.cpp
	ogl/util/matrix.cpp
	ogl/util/transforms.cpp
	
	vulkan/image/image_kernel.cpp
	vulkan/image/image_mixer.cpp

	vulkan/util/buffer.cpp
	vulkan/util/device.cpp
	vulkan/util/pipeline.cpp
	vulkan/util/renderpass.cpp
	vulkan/util/texture.cpp
	vulkan/util/matrix.cpp
	vulkan/util/transforms.cpp

	accelerator.cpp
)
set(HEADERS
	ogl/image/image_kernel.h
	ogl/image/image_mixer.h
	ogl/image/image_shader.h

	ogl/util/buffer.h
	ogl/util/context.h
	ogl/util/device.h
	ogl/util/shader.h
	ogl/util/texture.h
	ogl/util/matrix.h
	ogl/util/transforms.h

	ogl_image_vertex.h
	ogl_image_fragment.h
	
	vulkan/image/image_kernel.h
	vulkan/image/image_mixer.h

	vulkan/util/buffer.h
	vulkan/util/device.h
	vulkan/util/draw_params.h
	vulkan/util/pipeline.h
	vulkan/util/texture.h
	vulkan/util/matrix.h
	vulkan/util/transforms.h
	vulkan/util/uniform_block.h

	vulkan_image_vertex.h
	vulkan_image_fragment.h

	accelerator.h
	StdAfx.h
)

if (MSVC)
	list(APPEND SOURCES
		ogl/util/context_sfml.cpp
	)
else()
	list(APPEND SOURCES
		ogl/util/context_egl.cpp
	)
endif()


find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/image)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/image)

file(GLOB SHADERS
	${SHADER_SOURCE_DIR}/vertex_shader.vert
	${SHADER_SOURCE_DIR}/fragment_shader.frag)

foreach(source IN LISTS SHADERS)
	get_filename_component(FILENAME ${source} NAME_WLE)
	add_custom_command(
		COMMAND
			${glslc_executable}
			#      -MD -MF ${SHADER_BINARY_DIR}/${FILENAME}.d
			-o ${SHADER_BINARY_DIR}/${FILENAME}.spv
			${source}
		OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
		DEPENDS ${source} ${SHADER_BINARY_DIR}
		COMMENT "Compiling ${FILENAME}"
	)
	list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

bin2c("vulkan/image/vertex_shader.spv" "vulkan_image_vertex.h" "caspar::accelerator::vulkan" "vertex_shader")
bin2c("vulkan/image/fragment_shader.spv" "vulkan_image_fragment.h" "caspar::accelerator::vulkan" "fragment_shader")


bin2c("ogl/image/shader.vert" "ogl_image_vertex.h" "caspar::accelerator::ogl" "vertex_shader")
bin2c("ogl/image/shader.frag" "ogl_image_fragment.h" "caspar::accelerator::ogl" "fragment_shader")

casparcg_add_library(accelerator SOURCES ${SOURCES} ${HEADERS})
target_include_directories(accelerator PRIVATE .. ${CMAKE_CURRENT_BINARY_DIR})
target_precompile_headers(accelerator PRIVATE StdAfx.h)
target_link_libraries(accelerator PRIVATE common core GLEW::glew vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator Vulkan::Vulkan)

if (MSVC)
target_link_libraries(accelerator PRIVATE sfml-window)
endif()

source_group(sources ./.*)
source_group(sources\\cpu\\image cpu/image/.*)
source_group(sources\\cpu\\util cpu/util/.*)
source_group(sources\\ogl\\image ogl/image/.*)
source_group(sources\\ogl\\util ogl/util/.*)
source_group(sources\\vulkan\\image vulkan/image/.*)
source_group(sources\\vulkan\\util vulkan/util/.*)
