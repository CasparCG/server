cmake_minimum_required (VERSION 3.28)
project (screen)

set(SOURCES
	consumer/screen_consumer.cpp
	consumer/screen_consumer.h

	consumer_screen_vertex.h
	consumer_screen_fragment.h

	util/config.cpp
	util/config.h

	screen.cpp
	screen.h
)

bin2c("consumer/screen.vert" "consumer_screen_vertex.h" "caspar::screen" "vertex_shader")
bin2c("consumer/screen.frag" "consumer_screen_fragment.h" "caspar::screen" "fragment_shader")

if (MSVC)
else ()
	list(APPEND SOURCES
		util/x11_util.cpp
		util/x11_util.h
	)
endif ()

# Vulkan screen consumer. Presents using the shared accelerator Vulkan device.
# Non-Apple only in this phase (no macOS window backend yet); macOS keeps the GL
# consumer.
if (ENABLE_VULKAN AND NOT APPLE)
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

	if (MSVC)
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
target_link_libraries(screen PRIVATE accelerator ffmpeg GLEW::glew sfml-system sfml-window)

if (ENABLE_VULKAN)
	# screen.cpp's register_vulkan_requirements needs VkBootstrap + vulkan headers
	# on every platform (the consumer itself is gated above).
	target_link_libraries(screen PRIVATE Vulkan::Headers vk-bootstrap::vk-bootstrap)
	target_compile_definitions(screen PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
	if (NOT APPLE)
		if (NOT MSVC)
			# Windows uses the SFML window backend; GLFW is only needed on Linux
			target_link_libraries(screen PRIVATE glfw)
		endif ()
		add_dependencies(screen vulkan_screen_shaders)
	endif ()
endif ()

target_compile_definitions(screen PUBLIC NOMINMAX)
set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\vulkan_consumer vulkan_consumer/*)
source_group(sources ./*)
