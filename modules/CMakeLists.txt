cmake_minimum_required (VERSION 2.6)
project ("modules")

add_subdirectory(reroute)
add_subdirectory(ffmpeg)
add_subdirectory(oal)

if (MSVC)
	add_subdirectory(bluefish)
endif ()	

add_subdirectory(decklink)
add_subdirectory(screen)
add_subdirectory(html)
add_subdirectory(psd)

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(newtek)
endif ()

add_subdirectory(image)

