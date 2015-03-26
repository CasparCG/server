cmake_minimum_required (VERSION 2.6)
project ("modules")

if (MSVC)
	add_subdirectory(bluefish)
	add_subdirectory(decklink)
endif ()

add_subdirectory(ffmpeg)

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(image)
	add_subdirectory(oal)
	add_subdirectory(psd)
	add_subdirectory(reroute)
	add_subdirectory(screen)
endif ()

