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
add_subdirectory(psd)

if (MSVC)
	add_subdirectory(flash)
endif ()

add_subdirectory(reroute)

