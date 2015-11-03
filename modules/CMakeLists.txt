cmake_minimum_required (VERSION 2.6)
project ("modules")

add_subdirectory(reroute)
add_subdirectory(ffmpeg)
add_subdirectory(oal)

if(BUILD_MODULE_BLUEFISH)
	add_subdirectory(bluefish)
endif()

if(BUILD_MODULE_DECKLINK)
	add_subdirectory(decklink)
endif()

add_subdirectory(screen)

if(BUILD_MODULE_HTML)
	add_subdirectory(html)
endif()

if(BUILD_MODULE_PSD)
	add_subdirectory(psd)
endif()

if(BUILD_MODULE_FLASH)
	add_subdirectory(flash)
endif()

if(BUILD_MODULE_NEWTEK)
	add_subdirectory(newtek)
endif()

add_subdirectory(image)

