cmake_minimum_required(VERSION 2.6)
project("modules")

add_subdirectory(reroute) 
add_subdirectory(ffmpeg) 
add_subdirectory(oal) 
add_subdirectory(decklink) 
add_subdirectory(screen) 
add_subdirectory(html)

if (MSVC) 
	add_subdirectory(flash) 
	add_subdirectory(newtek) 
endif()

add_subdirectory(image)
