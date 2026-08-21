cmake_minimum_required (VERSION 3.28)
project (screen)

set(SOURCES
	util/config.cpp
	util/config.h

	screen.cpp
	screen.h
)

# OpenGL screen consumer (Windows/Linux). macOS has no usable desktop OpenGL.
if (NOT APPLE)
	list(APPEND SOURCES
		consumer/screen_consumer.cpp
		consumer/screen_consumer.h

		consumer_screen_vertex.h
		consumer_screen_fragment.h
	)

	bin2c("consumer/screen.vert" "consumer_screen_vertex.h" "caspar::screen" "vertex_shader")
	bin2c("consumer/screen.frag" "consumer_screen_fragment.h" "caspar::screen" "fragment_shader")
endif ()

if (MSVC)
elseif(APPLE)
else ()
	list(APPEND SOURCES
		util/x11_util.cpp
		util/x11_util.h
	)
endif ()

# Vulkan screen consumer (all platforms when Vulkan is enabled). Presents using
# the shared accelerator Vulkan device.
if (ENABLE_VULKAN)
	find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
	set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_consumer)
	set(SHADER_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan_consumer)

	file(GLOB SHADERS
		${SHADER_SOURCE_DIR}/screen_vk.frag
		${SHADER_SOURCE_DIR}/screen_vk.vert)

	foreach(source IN LISTS SHADERS)
		get_filename_component(FILENAME ${source} NAME)
		add_custom_command(
			COMMAND
				${glslc_executable}
				-o ${SHADER_BINARY_DIR}/${FILENAME}.spv
				${source}
			OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
			DEPENDS ${source} ${SHADER_BINARY_DIR}
			COMMENT "Compiling ${FILENAME}"
		)
		list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	endforeach()

	add_custom_target(vulkan_screen_shaders ALL DEPENDS ${SPV_SHADERS})

	bin2c("vulkan_consumer/screen_vk.vert.spv" "screen_vk_vertex.h" "caspar::screen" "vertex_shader_spv")
	bin2c("vulkan_consumer/screen_vk.frag.spv" "screen_vk_fragment.h" "caspar::screen" "fragment_shader_spv")

	list(APPEND SOURCES
		vulkan_consumer/screen_consumer_vk.cpp
		vulkan_consumer/screen_consumer_vk.h

		vulkan_consumer/util/render_pipeline.cpp
		vulkan_consumer/util/render_pipeline.h
		vulkan_consumer/util/swapchain.cpp
		vulkan_consumer/util/swapchain.h
		vulkan_consumer/util/vk_check.h
		vulkan_consumer/util/window.h

		screen_vk_vertex.h
		screen_vk_fragment.h
	)

	if (APPLE)
		list(APPEND SOURCES vulkan_consumer/util/window_mac.mm)
	elseif (MSVC)
		list(APPEND SOURCES vulkan_consumer/util/window_sfml.cpp)
	else ()
		list(APPEND SOURCES vulkan_consumer/util/window_generic.cpp)
	endif ()
endif ()

casparcg_add_module_project(screen
	SOURCES ${SOURCES}
	INIT_FUNCTION "screen::init"
	VULKAN_REQUIREMENTS_FUNCTION "screen::register_vulkan_requirements"
)
target_include_directories(screen PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${FFMPEG_INCLUDE_PATH}
)
target_link_libraries(screen PRIVATE accelerator ffmpeg)

if (NOT APPLE)
	target_link_libraries(screen PRIVATE GLEW::glew sfml-system sfml-window)
endif ()

if (ENABLE_VULKAN)
	target_link_libraries(screen PRIVATE Vulkan::Headers vk-bootstrap::vk-bootstrap)
	if (NOT MSVC)
		# Windows uses the SFML window backend; GLFW is only needed on Linux/macOS
		target_link_libraries(screen PRIVATE glfw)
	endif ()
	target_compile_definitions(screen PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
	add_dependencies(screen vulkan_screen_shaders)
	if (APPLE)
		target_link_libraries(screen PRIVATE "-framework Cocoa" "-framework Metal" "-framework QuartzCore")
	endif ()
endif ()

target_compile_definitions(screen PUBLIC NOMINMAX)
set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\vulkan_consumer vulkan_consumer/*)
source_group(sources ./*)
