cmake_minimum_required (VERSION 3.16)
project (shell)

add_definitions(-DNAPI_VERSION=6)

include_directories(${CMAKE_JS_INC})


join_list("${CASPARCG_MODULE_INCLUDE_STATEMENTS}" "\n" CASPARCG_MODULE_INCLUDE_STATEMENTS)
join_list("${CASPARCG_MODULE_INIT_STATEMENTS}" "\n" CASPARCG_MODULE_INIT_STATEMENTS)
join_list("${CASPARCG_MODULE_UNINIT_STATEMENTS}" "\n" CASPARCG_MODULE_UNINIT_STATEMENTS)
join_list("${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}" "\n" CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS)
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/included_modules.tmpl" "${CMAKE_BINARY_DIR}/generated/included_modules.h")

set(SOURCES
		casparcg.config
		main.cpp
		server.cpp

		operations/channel.cpp
		operations/consumer.cpp
		operations/producer.cpp
		operations/stage.cpp
		operations/util.cpp
		conv.cpp
)
set(HEADERS
		platform_specific.h
		server.h

		operations/base.h
		operations/channel.h
		operations/consumer.h
		operations/producer.h
		operations/stage.h
		operations/promiseWrapper.h
		operations/util.h

		conv.h
)

if (MSVC)
	list(APPEND SOURCES
			resource.h
			shell.rc
			windows_specific.cpp
	)
else ()
	list(APPEND SOURCES
			linux_specific.cpp
	)
endif ()


add_library(casparcg SHARED ${SOURCES} ${HEADERS} ${CMAKE_JS_SRC})
target_compile_features(casparcg PRIVATE cxx_std_17)
target_include_directories(casparcg PRIVATE
    ..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
	${CMAKE_JS_INC}
	# hack: these aren't being propogated correctly
	/home/julus/Projects/casparcg-server-node/node/node_modules/node-api-headers/include
	/home/julus/Projects/casparcg-server-node/node/node_modules/node-addon-api
    )
casparcg_add_build_dependencies(casparcg)


source_group(sources ./*)

target_link_libraries(casparcg
		accelerator
		common
		core
		protocol

		"${CASPARCG_MODULE_PROJECTS}"
)


# add_library(casparcg SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(casparcg PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(casparcg ${CMAKE_JS_LIB})

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
  # Generate node.lib
  execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET} ${CMAKE_STATIC_LINKER_FLAGS})
endif()

if (MSVC)
	target_link_libraries(casparcg
		Winmm.lib
		Ws2_32.lib
		optimized tbb.lib
		debug tbb_debug.lib
		OpenGL32.lib
		glew32.lib
		openal32.lib
		zlibstatic.lib
		debug sfml-graphics-d.lib
		debug sfml-window-d.lib
		debug sfml-system-d.lib
		optimized sfml-graphics.lib
		optimized sfml-window.lib
		optimized sfml-system.lib
		
		avformat.lib
		avcodec.lib
		avutil.lib
		avfilter.lib
		avdevice.lib
		swscale.lib
		swresample.lib
	)
else ()
	target_link_libraries(casparcg
		${Boost_LIBRARIES}
		${TBB_LIBRARIES}
		${SFML_LIBRARIES}
		${GLEW_LIBRARIES}
		OpenGL::GL
		${X11_LIBRARIES}
		${FFMPEG_LIBRARIES}
		dl
		icui18n
		icuuc
		z
		pthread
	)

    set_target_properties(casparcg PROPERTIES INSTALL_RPATH "$ORIGIN/../lib" BUILD_WITH_INSTALL_RPATH ON)
endif ()

# add_custom_target(casparcg_copy_dependencies ALL)
# casparcg_add_build_dependencies(casparcg_copy_dependencies)

# set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
# add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory \"${OUTPUT_FOLDER}\")

# if (MSVC)
# 	foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG})
# 		get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
# 		add_custom_command(
# 			TARGET casparcg_copy_dependencies
# 			POST_BUILD 
# 			COMMAND if $(ConfigurationName) == Debug (
# 				echo \"Copy Debug ${FILE_TO_COPY}\" &&
# 				${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
# 				${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
# 			)
# 		)
# 	endforeach(FILE_TO_COPY)
# 	foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE})
# 		get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
# 		add_custom_command(
# 			TARGET casparcg_copy_dependencies
# 			POST_BUILD 
# 			COMMAND if $(ConfigurationName) NEQ Debug (
# 				echo \"Copy Release ${FILE_TO_COPY}\" &&
# 				${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
# 				${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
# 			)
# 		)
# 	endforeach(FILE_TO_COPY)

# 	foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG_DIRS})
# 		get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
# 		add_custom_command(
# 			TARGET casparcg_copy_dependencies
# 			POST_BUILD 
# 			COMMAND if $(ConfigurationName) == Debug (
# 				echo \"Copy Debug ${FILE_TO_COPY}\" &&
# 				${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
# 				${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
# 			)
# 		)
# 	endforeach(FILE_TO_COPY)
# 	foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE_DIRS})
# 		get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
# 		add_custom_command(
# 			TARGET casparcg_copy_dependencies
# 			POST_BUILD 
# 			COMMAND if $(ConfigurationName) NEQ Debug (
# 				echo \"Copy Release ${FILE_TO_COPY}\" &&
# 				${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
# 				${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
# 			)
# 		)
# 	endforeach(FILE_TO_COPY)
# else ()
# 	# ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/shell/casparcg ${CMAKE_BINARY_DIR}/staging/bin/casparcg)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/casparcg.config ${CMAKE_BINARY_DIR}/staging/casparcg.config)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/liberation-fonts/LiberationMono-Regular.ttf ${CMAKE_BINARY_DIR}/staging/LiberationMono-Regular.ttf)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/run.sh ${CMAKE_BINARY_DIR}/staging/run.sh)

# # Disabled as HTML module is disabled
#   if (ENABLE_HTML AND NOT USE_SYSTEM_CEF)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_RESOURCE_PATH}/locales ${CMAKE_BINARY_DIR}/staging/lib/locales)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_100_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_100_percent.pak)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_200_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_200_percent.pak)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/icudtl.dat ${CMAKE_BINARY_DIR}/staging/lib/icudtl.dat)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/resources.pak ${CMAKE_BINARY_DIR}/staging/lib/resources.pak)

# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/chrome-sandbox ${CMAKE_BINARY_DIR}/staging/lib/chrome-sandbox)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libcef.so ${CMAKE_BINARY_DIR}/staging/lib/libcef.so)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libEGL.so ${CMAKE_BINARY_DIR}/staging/lib/libEGL.so)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libGLESv2.so ${CMAKE_BINARY_DIR}/staging/lib/libGLESv2.so)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libvk_swiftshader.so ${CMAKE_BINARY_DIR}/staging/lib/libvk_swiftshader.so)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/libvulkan.so.1 ${CMAKE_BINARY_DIR}/staging/lib/libvulkan.so.1)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/snapshot_blob.bin ${CMAKE_BINARY_DIR}/staging/lib/snapshot_blob.bin)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/v8_context_snapshot.bin ${CMAKE_BINARY_DIR}/staging/lib/v8_context_snapshot.bin)
# 	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_BIN_PATH}/vk_swiftshader_icd.json ${CMAKE_BINARY_DIR}/staging/lib/vk_swiftshader_icd.json)
#   endif ()
# endif ()
