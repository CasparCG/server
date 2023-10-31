cmake_minimum_required (VERSION 3.16)

include(ExternalProject)

INCLUDE (PlatformIntrospection)
_DETERMINE_CPU_COUNT (CONFIG_CPU_COUNT)

find_package(Git)

set(CONFIG_VERSION_GIT_HASH "N/A")

if (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
	exec_program("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}"
			ARGS rev-parse --verify --short HEAD
			OUTPUT_VARIABLE CONFIG_VERSION_GIT_HASH)
endif ()

CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/version.tmpl" "${CMAKE_BINARY_DIR}/generated/version.h")
INCLUDE_DIRECTORIES ("${CMAKE_BINARY_DIR}/generated")

set(CASPARCG_DOWNLOAD_MIRROR https://github.com/CasparCG/dependencies/releases/download/ CACHE STRING "Source/mirror to use for external dependencies")
set(CASPARCG_DOWNLOAD_CACHE ${CMAKE_CURRENT_BINARY_DIR}/external CACHE STRING "Download cache directory for cmake ExternalProjects")
set(BOOST_USE_PRECOMPILED ON CACHE BOOL "Use precompiled boost")

set(CASPARCG_MODULE_INCLUDE_STATEMENTS "" CACHE INTERNAL "")
set(CASPARCG_MODULE_INIT_STATEMENTS "" CACHE INTERNAL "")
set(CASPARCG_MODULE_UNINIT_STATEMENTS "" CACHE INTERNAL "")
set(CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS "" CACHE INTERNAL "")
set(CASPARCG_MODULE_PROJECTS "" CACHE INTERNAL "")
set(CASPARCG_RUNTIME_DEPENDENCIES "" CACHE INTERNAL "")
set(CASPARCG_RUNTIME_DEPENDENCIES_DIRS "" CACHE INTERNAL "")

set(CASPARCG_EXTERNAL_PROJECTS "" CACHE INTERNAL "")
function (casparcg_add_external_project NAME)
	set(CASPARCG_EXTERNAL_PROJECTS "${CASPARCG_EXTERNAL_PROJECTS}" "${NAME}" CACHE INTERNAL "")
endfunction()

function(casparcg_add_build_dependencies PROJECT)
	add_dependencies(${PROJECT} ${CASPARCG_EXTERNAL_PROJECTS})
endfunction()

function(casparcg_add_include_statement HEADER_FILE_TO_INCLUDE)
	set(CASPARCG_MODULE_INCLUDE_STATEMENTS "${CASPARCG_MODULE_INCLUDE_STATEMENTS}"
			"#include <${HEADER_FILE_TO_INCLUDE}>"
			CACHE INTERNAL "")
endfunction()

function(casparcg_add_init_statement INIT_FUNCTION_NAME NAME_TO_LOG)
	set(CASPARCG_MODULE_INIT_STATEMENTS "${CASPARCG_MODULE_INIT_STATEMENTS}"
			"	${INIT_FUNCTION_NAME}(dependencies)\;"
			"	CASPAR_LOG(info) << L\"Initialized ${NAME_TO_LOG} module.\"\;"
			""
			CACHE INTERNAL "")
endfunction()

function(casparcg_add_uninit_statement UNINIT_FUNCTION_NAME)
	set(CASPARCG_MODULE_UNINIT_STATEMENTS
			"	${UNINIT_FUNCTION_NAME}()\;"
			"${CASPARCG_MODULE_UNINIT_STATEMENTS}"
			CACHE INTERNAL "")
endfunction()

function(casparcg_add_command_line_arg_interceptor INTERCEPTOR_FUNCTION_NAME)
	set(CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS "${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}"
			"	if (${INTERCEPTOR_FUNCTION_NAME}(argc, argv))"
			"		return true\;"
			""
			CACHE INTERNAL "")
endfunction()

function(casparcg_add_module_project PROJECT)
	set(CASPARCG_MODULE_PROJECTS "${CASPARCG_MODULE_PROJECTS}" "${PROJECT}" CACHE INTERNAL "")
endfunction()

# http://stackoverflow.com/questions/7172670/best-shortest-way-to-join-a-list-in-cmake
function(join_list VALUES GLUE OUTPUT)
	string (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
	string (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
	set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

function(casparcg_add_runtime_dependency FILE_TO_COPY)
	set(CASPARCG_RUNTIME_DEPENDENCIES "${CASPARCG_RUNTIME_DEPENDENCIES}" "${FILE_TO_COPY}" CACHE INTERNAL "")
endfunction()
function(casparcg_add_runtime_dependency_dir FILE_TO_COPY)
	set(CASPARCG_RUNTIME_DEPENDENCIES_DIRS "${CASPARCG_RUNTIME_DEPENDENCIES_DIRS}" "${FILE_TO_COPY}" CACHE INTERNAL "")
endfunction()

casparcg_add_runtime_dependency("${PROJECT_SOURCE_DIR}/shell/casparcg.config")

# BOOST
casparcg_add_external_project(boost)
if (BOOST_USE_PRECOMPILED)
	ExternalProject_Add(boost
	URL ${CASPARCG_DOWNLOAD_MIRROR}/boost/boost_1_67_0-precompiled.zip
	URL_HASH MD5=8fd5450206d48acc51dff83ce5a34a20
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	)
	ExternalProject_Get_Property(boost SOURCE_DIR)
	set(BOOST_INCLUDE_PATH "${SOURCE_DIR}/include/boost-1_67")
	link_directories("${SOURCE_DIR}/lib")
else ()
	set(BOOST_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/boost-install)
	ExternalProject_Add(boost
	URL ${CASPARCG_DOWNLOAD_MIRROR}/boost/boost_1_67_0.zip
	URL_HASH MD5=6da1ba65f8d33b1d306616e5acd87f67
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	BUILD_IN_SOURCE 1
	CONFIGURE_COMMAND ./bootstrap.bat
		--with-libraries=filesystem
		--with-libraries=locale
		--with-libraries=log
		--with-libraries=log_setup
		--with-libraries=regex
		--with-libraries=system
		--with-libraries=thread
	BUILD_COMMAND ./b2 install --prefix=${BOOST_INSTALL_DIR} link=static variant=release threading=multi runtime-link=shared -j ${CONFIG_CPU_COUNT} 
	INSTALL_COMMAND ""
	)
	set(BOOST_INCLUDE_PATH "${BOOST_INSTALL_DIR}/include/boost-1_67")
	link_directories("${BOOST_INSTALL_DIR}/lib")
endif ()
add_definitions( -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE )
add_definitions( -DBOOST_COROUTINES_NO_DEPRECATION_WARNING )
add_definitions( -DBOOST_LOCALE_HIDE_AUTO_PTR )

# FFMPEG
casparcg_add_external_project(ffmpeg-lib)
ExternalProject_Add(ffmpeg-lib
	URL ${CASPARCG_DOWNLOAD_MIRROR}/ffmpeg/ffmpeg-5.1.2-full_build-shared.zip
	URL_HASH MD5=bcb1efb68701a4b71e8a7efd9b817965
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(ffmpeg-lib SOURCE_DIR)
set(FFMPEG_INCLUDE_PATH "${SOURCE_DIR}/include")
set(FFMPEG_BIN_PATH "${SOURCE_DIR}/bin")
link_directories("${SOURCE_DIR}/lib")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avcodec-59.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avdevice-59.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avfilter-8.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avformat-59.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/avutil-57.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/postproc-56.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swresample-4.dll")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/swscale-6.dll")
# for scanner:
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/ffmpeg.exe")
casparcg_add_runtime_dependency("${FFMPEG_BIN_PATH}/ffprobe.exe")

# TBB
casparcg_add_external_project(tbb)
ExternalProject_Add(tbb
	URL ${CASPARCG_DOWNLOAD_MIRROR}/tbb/oneapi-tbb-2021.1.1-win.zip
	URL_HASH MD5=51bf49044d477dea67670abd92f8814c
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(tbb SOURCE_DIR)
set(TBB_INCLUDE_PATH "${SOURCE_DIR}/include")
set(TBB_BIN_PATH "${SOURCE_DIR}/redist/intel64/vc14")
link_directories("${SOURCE_DIR}/lib/intel64/vc14")
casparcg_add_runtime_dependency("${TBB_BIN_PATH}/tbb12.dll")
casparcg_add_runtime_dependency("${TBB_BIN_PATH}/tbb12_debug.dll")

# GLEW
casparcg_add_external_project(glew)
ExternalProject_Add(glew
	URL ${CASPARCG_DOWNLOAD_MIRROR}/glew/glew-2.2.0-win32.zip
	URL_HASH MD5=1feddfe8696c192fa46a0df8eac7d4bf
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(glew SOURCE_DIR)
set(GLEW_INCLUDE_PATH ${SOURCE_DIR}/include)
set(GLEW_BIN_PATH ${SOURCE_DIR}/bin/Release/x64)
link_directories(${SOURCE_DIR}/lib/Release/x64)
add_definitions( -DGLEW_NO_GLU )
casparcg_add_runtime_dependency("${GLEW_BIN_PATH}/glew32.dll")

# SFML
casparcg_add_external_project(sfml)
ExternalProject_Add(sfml
	URL ${CASPARCG_DOWNLOAD_MIRROR}/sfml/SFML-2.4.2-windows-vc14-64-bit.zip
	URL_HASH MD5=8a2f747335fa21a7a232976daa9031ac
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(sfml SOURCE_DIR)
set(SFML_INCLUDE_PATH ${SOURCE_DIR}/include)
set(SFML_BIN_PATH "${SOURCE_DIR}/bin")
link_directories(${SOURCE_DIR}/lib)
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-graphics-d-2.dll")
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-graphics-2.dll")
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-window-d-2.dll")
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-window-2.dll")
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-system-d-2.dll")
casparcg_add_runtime_dependency("${SFML_BIN_PATH}/sfml-system-2.dll")

# FREEIMAGE
casparcg_add_external_project(freeimage)
ExternalProject_Add(freeimage
	URL ${CASPARCG_DOWNLOAD_MIRROR}/freeimage/FreeImage3180Win32Win64.zip
	URL_HASH MD5=393d3df75b14cbcb4887da1c395596e2
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(freeimage SOURCE_DIR)
set(FREEIMAGE_INCLUDE_PATH "${SOURCE_DIR}/Dist/x64")
set(FREEIMAGE_BIN_PATH "${FREEIMAGE_INCLUDE_PATH}")
link_directories("${FREEIMAGE_INCLUDE_PATH}")
casparcg_add_runtime_dependency("${FREEIMAGE_INCLUDE_PATH}/FreeImage.dll")

#ZLIB
casparcg_add_external_project(zlib)
ExternalProject_Add(zlib
	URL ${CASPARCG_DOWNLOAD_MIRROR}/zlib/zlib-1.3.tar.gz
	URL_HASH MD5=60373b133d630f74f4a1f94c1185a53f
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(zlib SOURCE_DIR)
ExternalProject_Get_Property(zlib BINARY_DIR)
set(ZLIB_INCLUDE_PATH "${SOURCE_DIR};${BINARY_DIR}")
link_directories(${BINARY_DIR}/Release)

# OPENAL
casparcg_add_external_project(openal)
ExternalProject_Add(openal
	URL ${CASPARCG_DOWNLOAD_MIRROR}/openal/openal-soft-1.19.1-bin.zip
	URL_HASH MD5=b78ef1ba26f7108e763f92df6bbc3fa5
	DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
	BUILD_IN_SOURCE 1
	CONFIGURE_COMMAND ""
	BUILD_COMMAND cp bin/Win64/soft_oal.dll bin/Win64/OpenAL32.dll
	INSTALL_COMMAND ""
)
ExternalProject_Get_Property(openal SOURCE_DIR)
set(OPENAL_INCLUDE_PATH "${SOURCE_DIR}/include")
link_directories("${SOURCE_DIR}/libs/Win64")
casparcg_add_runtime_dependency("${SOURCE_DIR}/bin/Win64/OpenAL32.dll")

# LIBERATION_FONTS
set(LIBERATION_FONTS_BIN_PATH "${PROJECT_SOURCE_DIR}/shell/liberation-fonts")
casparcg_add_runtime_dependency("${LIBERATION_FONTS_BIN_PATH}/LiberationMono-Regular.ttf")

# CEF
if (ENABLE_HTML)
	casparcg_add_external_project(cef)
	ExternalProject_Add(cef
		URL ${CASPARCG_DOWNLOAD_MIRROR}/cef/cef_binary_4638_windows_x64.zip
		URL_HASH MD5=14ad547122903eba3f145322fb02bc6d
		DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
		CMAKE_ARGS -DUSE_SANDBOX=Off -DCEF_RUNTIME_LIBRARY_FLAG=/MD
		INSTALL_COMMAND ""
	)
	ExternalProject_Get_Property(cef SOURCE_DIR)
	ExternalProject_Get_Property(cef BINARY_DIR)

	set(CEF_INCLUDE_PATH ${SOURCE_DIR})
	set(CEF_BIN_PATH ${SOURCE_DIR}/Release)
	set(CEF_RESOURCE_PATH ${SOURCE_DIR}/Resources)
	link_directories(${SOURCE_DIR}/Release)
	link_directories(${BINARY_DIR}/libcef_dll_wrapper/Release)

	casparcg_add_runtime_dependency_dir("${CEF_RESOURCE_PATH}/locales")
	casparcg_add_runtime_dependency("${CEF_RESOURCE_PATH}/chrome_100_percent.pak")
	casparcg_add_runtime_dependency("${CEF_RESOURCE_PATH}/chrome_200_percent.pak")
	casparcg_add_runtime_dependency("${CEF_RESOURCE_PATH}/resources.pak")
	casparcg_add_runtime_dependency("${CEF_RESOURCE_PATH}/icudtl.dat")

	casparcg_add_runtime_dependency_dir("${CEF_BIN_PATH}/swiftshader")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/snapshot_blob.bin")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/v8_context_snapshot.bin")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libcef.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/chrome_elf.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/d3dcompiler_47.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libEGL.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/libGLESv2.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/vk_swiftshader.dll")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/vk_swiftshader_icd.json")
	casparcg_add_runtime_dependency("${CEF_BIN_PATH}/vulkan-1.dll")
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT casparcg)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-DCASPAR_SOURCE_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}")
add_definitions(-D_WIN32_WINNT=0x601)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa /Zi /W4 /WX /MP /fp:fast /Zm192 /FIcommon/compiler/vs/disable_silly_warnings.h")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}	/D TBB_USE_ASSERT=1 /D TBB_USE_DEBUG /bigobj")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}	/Oi /Ot /Gy /bigobj")

if (POLICY CMP0045)
	cmake_policy(SET CMP0045 OLD)
endif ()

add_subdirectory(tools)
add_subdirectory(accelerator)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(modules)
add_subdirectory(protocol)
add_subdirectory(shell)
