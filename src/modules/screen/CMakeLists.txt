cmake_minimum_required (VERSION 3.28)
project (screen)

set(SOURCES
	consumer/screen_config.cpp
	consumer/screen_config.h
	consumer/screen_consumer.h

	screen.cpp
	screen.h
)

if (APPLE)
	# Native NSWindow + CAMetalLayer implementation
	list(APPEND SOURCES
		consumer/screen_consumer_metal.mm
		consumer_screen_metal.h
	)
	bin2c("consumer/screen.metal" "consumer_screen_metal.h" "caspar::screen" "metal_shader")
	set_source_files_properties(consumer/screen_consumer_metal.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
else ()
	list(APPEND SOURCES
		consumer/screen_consumer.cpp

		consumer_screen_vertex.h
		consumer_screen_fragment.h
	)
	bin2c("consumer/screen.vert" "consumer_screen_vertex.h" "caspar::screen" "vertex_shader")
	bin2c("consumer/screen.frag" "consumer_screen_fragment.h" "caspar::screen" "fragment_shader")

	if (NOT MSVC)
		list(APPEND SOURCES
			util/x11_util.cpp
			util/x11_util.h
		)
	endif ()
endif ()

casparcg_add_module_project(screen
	SOURCES ${SOURCES}
	INIT_FUNCTION "screen::init"
)
target_include_directories(screen PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${FFMPEG_INCLUDE_PATH}
)

if (APPLE)
	target_link_libraries(screen PRIVATE "-framework Cocoa" "-framework Metal" "-framework QuartzCore")
else ()
	target_link_libraries(screen PRIVATE accelerator ffmpeg GLEW::glew sfml-system sfml-window)
endif ()

set_target_properties(screen PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources ./*)
