cmake_minimum_required (VERSION 3.16)
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
		platform_specific.h
		server.h
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


add_executable(casparcg ${SOURCES} ${HEADERS})
target_compile_features(casparcg PRIVATE cxx_std_17)
target_include_directories(casparcg PRIVATE
    ..
    ${BOOST_INCLUDE_PATH}
    )
casparcg_add_build_dependencies(casparcg)

source_group(sources ./*)

target_link_libraries(casparcg
		accelerator
		common
		core
		protocol

		TBB::tbb
		TBB::tbbmalloc
		"${CASPARCG_MODULE_PROJECTS}"
)

if (MSVC)
	target_link_libraries(casparcg
		Winmm.lib
		Ws2_32.lib
		OpenGL32.lib
		glew32.lib
		debug zlibstaticd.lib
		optimized zlibstatic.lib

		OpenAL::OpenAL
		optimized TBB::tbbmalloc_proxy

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
endif ()

add_custom_target(casparcg_copy_dependencies ALL)
casparcg_add_build_dependencies(casparcg_copy_dependencies)

set(OUTPUT_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
add_custom_command(TARGET casparcg_copy_dependencies POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory \"${OUTPUT_FOLDER}\")

if (MSVC)
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND if $(ConfigurationName) == Debug (
				    echo \"Copy Debug ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
			    )
		    )
	    endforeach(FILE_TO_COPY)
	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND if $(ConfigurationName) NEQ Debug (
				    echo \"Copy Release ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
			    )
		    )
	    endforeach(FILE_TO_COPY)

	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG_DIRS})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND if $(ConfigurationName) == Debug (
				    echo \"Copy Debug ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
			    )
		    )
	    endforeach(FILE_TO_COPY)
	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE_DIRS})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND if $(ConfigurationName) NEQ Debug (
				    echo \"Copy Release ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
			    )
		    )
	    endforeach(FILE_TO_COPY)
    elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND echo \"Copy Debug ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
		    )
	    endforeach(FILE_TO_COPY)

	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_DEBUG_DIRS})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND echo \"Copy Debug ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
		    )
	    endforeach(FILE_TO_COPY)
    else ()
	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND echo \"Copy Release ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/\" &&
				    ${CMAKE_COMMAND} -E copy \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/\"
		    )
	    endforeach(FILE_TO_COPY)

	    foreach(FILE_TO_COPY ${CASPARCG_RUNTIME_DEPENDENCIES_RELEASE_DIRS})
		    get_filename_component(FOLDER_NAME "${FILE_TO_COPY}" NAME)
		    add_custom_command(
			    TARGET casparcg_copy_dependencies
			    POST_BUILD
			    COMMAND echo \"Copy Release ${FILE_TO_COPY}\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${OUTPUT_FOLDER}/${FOLDER_NAME}/\" &&
				    ${CMAKE_COMMAND} -E copy_directory \"${FILE_TO_COPY}\" \"${CMAKE_CURRENT_BINARY_DIR}/${FOLDER_NAME}/\"
		    )
	    endforeach(FILE_TO_COPY)
    endif ()
else ()
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/shell/casparcg ${CMAKE_BINARY_DIR}/staging/bin/casparcg)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/casparcg.config ${CMAKE_BINARY_DIR}/staging/casparcg.config)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/liberation-fonts/LiberationMono-Regular.ttf ${CMAKE_BINARY_DIR}/staging/LiberationMono-Regular.ttf)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/shell/run.sh ${CMAKE_BINARY_DIR}/staging/run.sh)

# Disabled as HTML module is disabled
  if (ENABLE_HTML AND NOT USE_SYSTEM_CEF)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CEF_RESOURCE_PATH}/locales ${CMAKE_BINARY_DIR}/staging/lib/locales)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_100_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_100_percent.pak)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/chrome_200_percent.pak ${CMAKE_BINARY_DIR}/staging/lib/chrome_200_percent.pak)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/icudtl.dat ${CMAKE_BINARY_DIR}/staging/lib/icudtl.dat)
	ADD_CUSTOM_COMMAND (TARGET casparcg POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CEF_RESOURCE_PATH}/resources.pak ${CMAKE_BINARY_DIR}/staging/lib/resources.pak)

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
endif ()
