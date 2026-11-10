cmake_minimum_required (VERSION 3.28)
project("modules")

add_subdirectory(image)
add_subdirectory(ffmpeg)
add_subdirectory(oal)
if (NOT APPLE)
	# TODO(macos): DeckLink SDK for macOS uses CFStringRef/different interop headers
	add_subdirectory(decklink)
endif ()
if (ENABLE_OPENGL OR APPLE)
	# OpenGL 4.5 consumer, or the native Metal consumer on macOS
	add_subdirectory(screen)
endif ()
add_subdirectory(newtek)
add_subdirectory(artnet)

if (ENABLE_HTML)
	add_subdirectory(html)
endif ()

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(bluefish)
endif()

