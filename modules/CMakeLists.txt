cmake_minimum_required (VERSION 2.6)
project ("modules")

add_subdirectory(ffmpeg)

if (MSVC)
	add_subdirectory(bluefish)
	add_subdirectory(decklink)
	add_subdirectory(oal)
	add_subdirectory(screen)
endif ()

add_subdirectory(image)

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(psd)
endif ()

add_subdirectory(reroute)

