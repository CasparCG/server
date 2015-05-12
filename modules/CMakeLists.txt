cmake_minimum_required (VERSION 2.6)
project ("modules")

add_subdirectory(ffmpeg)

if (MSVC)
	add_subdirectory(bluefish)
	add_subdirectory(oal)
endif ()

add_subdirectory(decklink)
add_subdirectory(screen)
add_subdirectory(image)

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(psd)
	add_subdirectory(html)
endif ()

add_subdirectory(reroute)

