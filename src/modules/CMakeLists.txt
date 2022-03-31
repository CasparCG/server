cmake_minimum_required(VERSION 2.6)
project("modules")

add_subdirectory(ffmpeg)
add_subdirectory(decklink)
add_subdirectory(newtek)
if (ENABLE_HTML)
	add_subdirectory(html)
endif ()

