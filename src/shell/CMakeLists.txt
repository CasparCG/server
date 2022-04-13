cmake_minimum_required (VERSION 2.6)
project (shell)

join_list("${CASPARCG_MODULE_INCLUDE_STATEMENTS}" "\n" CASPARCG_MODULE_INCLUDE_STATEMENTS)
join_list("${CASPARCG_MODULE_INIT_STATEMENTS}" "\n" CASPARCG_MODULE_INIT_STATEMENTS)
join_list("${CASPARCG_MODULE_UNINIT_STATEMENTS}" "\n" CASPARCG_MODULE_UNINIT_STATEMENTS)
join_list("${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}" "\n" CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS)
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/included_modules.tmpl" "${CMAKE_BINARY_DIR}/generated/included_modules.h")

set(SOURCES
		casparcg.config
		main.cpp
		server.cpp
)
set(HEADERS
		server.h
)

add_executable(casparcg ${SOURCES} ${HEADERS})

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

source_group(sources ./*)

target_link_libraries(casparcg
		accelerator
		common
		core
		protocol

		"${CASPARCG_MODULE_PROJECTS}"
)

target_link_libraries(casparcg
	${Boost_LIBRARIES}
	${TBB_LIBRARIES}
	${TBB_MALLOC_LIBRARIES}
	${TBB_MALLOC_PROXY_LIBRARIES}
	${SFML_LIBRARIES}
	${GLEW_LIBRARIES}
	${X11_LIBRARIES}
	${FFMPEG_LIBRARIES}
	dl
	icui18n
	icuuc
	z
	pthread
	OpenGL::OpenGL OpenGL::EGL
)

set_target_properties(casparcg PROPERTIES INSTALL_RPATH "$ORIGIN/../lib" BUILD_WITH_INSTALL_RPATH ON)

add_custom_target(casparcg_copy_dependencies ALL)

set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory \"${OUTPUT_FOLDER}\")

ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/shell/casparcg ${CMAKE_BINARY_DIR}/staging/bin/casparcg)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/casparcg.config ${CMAKE_BINARY_DIR}/staging/casparcg.config)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/liberation-fonts/LiberationMono-Regular.ttf ${CMAKE_BINARY_DIR}/staging/LiberationMono-Regular.ttf)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/run.sh ${CMAKE_BINARY_DIR}/staging/run.sh)

if (ENABLE_HTML)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_RESOURCE_PATH}/locales ${CMAKE_BINARY_DIR}/staging/lib/locales)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_100_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_100_percent.pak)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_200_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_200_percent.pak)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/icudtl.dat ${CMAKE_BINARY_DIR}/staging/lib/icudtl.dat)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/resources.pak ${CMAKE_BINARY_DIR}/staging/lib/resources.pak)

ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_BIN_PATH}/swiftshader ${CMAKE_BINARY_DIR}/staging/lib/swiftshader)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/chrome-sandbox ${CMAKE_BINARY_DIR}/staging/lib/chrome-sandbox)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libcef.so ${CMAKE_BINARY_DIR}/staging/lib/libcef.so)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libEGL.so ${CMAKE_BINARY_DIR}/staging/lib/libEGL.so)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libGLESv2.so ${CMAKE_BINARY_DIR}/staging/lib/libGLESv2.so)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libvk_swiftshader.so ${CMAKE_BINARY_DIR}/staging/lib/libvk_swiftshader.so)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libvulkan.so.1 ${CMAKE_BINARY_DIR}/staging/lib/libvulkan.so.1)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/snapshot_blob.bin ${CMAKE_BINARY_DIR}/staging/lib/snapshot_blob.bin)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/v8_context_snapshot.bin ${CMAKE_BINARY_DIR}/staging/lib/v8_context_snapshot.bin)
ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/vk_swiftshader_icd.json ${CMAKE_BINARY_DIR}/staging/lib/vk_swiftshader_icd.json)
endif ()
