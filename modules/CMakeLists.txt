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
add_subdirectory(html)

if (MSVC)
	add_subdirectory(flash)
	add_subdirectory(psd)
endif ()

add_subdirectory(reroute)

