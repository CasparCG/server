cmake_minimum_required(VERSION 2.6)
project("modules")

add_subdirectory(ffmpeg)
add_subdirectory(oal)
add_subdirectory(decklink)
add_subdirectory(screen)
add_subdirectory(newtek)
if (ENABLE_HTML)
	add_subdirectory(html)
endif ()

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(bluefish)
endif()

add_subdirectory(image)
