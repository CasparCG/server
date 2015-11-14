cmake_minimum_required (VERSION 2.6)
project ("CasparCG Server")

find_package(Git)

set(GIT_REV "0")
set(GIT_HASH "N/A")

if (GIT_FOUND)
	exec_program("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}"
			ARGS rev-list --all --count
			OUTPUT_VARIABLE GIT_REV)
	exec_program("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}"
			ARGS rev-parse --verify --short HEAD
			OUTPUT_VARIABLE GIT_HASH)
endif ()

include(CMakeDependentOption)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

CMAKE_DEPENDENT_OPTION(BUILD_MODULE_BLUEFISH "Build Bluefish module" ON "MSVC" OFF)
option(BUILD_MODULE_DECKLINK "Build Decklink module" ON)
option(BUILD_MODULE_HTML "Build HTML module" ON)
option(BUILD_MODULE_PSD "Build PSD module" ON)
CMAKE_DEPENDENT_OPTION(BUILD_MODULE_FLASH "Build Flash module" ON "MSVC" OFF)
CMAKE_DEPENDENT_OPTION(BUILD_MODULE_NEWTEK "Build Newtek module" ON "MSVC" OFF)

option(USE_SYSTEM_BOOST     "Compile against system boost instead of bundled one" OFF)
option(USE_SYSTEM_FFMPEG    "Compile against system FFmpeg instead of bundled one" OFF)
option(USE_SYSTEM_TBB       "Compile against system TBB instead of bundled one" OFF)
option(USE_SYSTEM_GLEW      "Compile against system GLEW instead of bundled one" OFF)
option(USE_SYSTEM_FREETYPE  "Compile against system FreeType instead of bundled one" OFF)
option(USE_SYSTEM_FREEIMAGE "Compile against system FreeImage instead of bundled one" OFF)
option(USE_SYSTEM_OPENAL    "Compile against system OpenAL instead of bundled one" OFF)
option(USE_SYSTEM_SFML      "Compile against system SFML instead of bundled one" OFF)

configure_file("${PROJECT_SOURCE_DIR}/version.tmpl" "${PROJECT_SOURCE_DIR}/version.h")

set(DEPENDENCIES_FOLDER			"${PROJECT_SOURCE_DIR}/dependencies64")

if (MSVC)
	set(PLATFORM_FOLDER_NAME	"win32")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(PLATFORM_FOLDER_NAME	"linux")
endif ()

## boost ##
if(USE_SYSTEM_BOOST)
	find_package(Boost 1.56 REQUIRED COMPONENTS chrono date_time filesystem locale log regex system thread)
	add_definitions(-DBOOST_ALL_DYN_LINK)
else()
	set(Boost_INCLUDE_DIRS		"${DEPENDENCIES_FOLDER}/boost")
	set(Boost_LIBRARIES boost_system boost_thread boost_chrono boost_filesystem boost_log boost_locale boost_regex boost_date_time)
	link_directories("${DEPENDENCIES_FOLDER}/boost/stage/lib/${PLATFORM_FOLDER_NAME}")
endif()
add_definitions( -DBOOST_THREAD_VERSION=4 )
add_definitions( "-DBOOST_ASIO_ERROR_CATEGORY_NOEXCEPT=noexcept(true)" ) # Workaround macro redefinition in boost

## TBB ##
if(USE_SYSTEM_TBB)
	find_package(TBB REQUIRED)
else()
	set(TBB_INCLUDE_DIRS		"${DEPENDENCIES_FOLDER}/tbb/include")
	if(MSVC)
		set(TBB_LIBRARIES optimized tbb debug tbb_debug)
	else()
		set(TBB_LIBRARIES tbb tbbmalloc tbbmalloc_proxy)
	endif()
	set(TBB_BIN_DIR				"${DEPENDENCIES_FOLDER}/tbb/bin/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/tbb/lib/${PLATFORM_FOLDER_NAME}")
endif()
add_definitions( -DTBB_USE_CAPTURED_EXCEPTION=0 )

## GLEW ##
if(USE_SYSTEM_GLEW)
	find_package(GLEW REQUIRED)
	find_package(OpenGL REQUIRED)
else()
	set(GLEW_INCLUDE_DIRS		"${DEPENDENCIES_FOLDER}/glew/include")
	if(MSVC)
		set(GLEW_LIBRARIES glew32)
		set(OPENGL_gl_LIBRARY OpenGL32)
	else()
		set(GLEW_LIBRARIES GLEW)
		set(OPENGL_gl_LIBRARY GL)
	endif()
	set(GLEW_BIN_DIR			"${DEPENDENCIES_FOLDER}/glew/bin/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/glew/lib/${PLATFORM_FOLDER_NAME}")
endif()
add_definitions( -DGLEW_NO_GLU )

set(RXCPP_INCLUDE_DIRS			"${DEPENDENCIES_FOLDER}/RxCpp/include")

## SFML ##
if(USE_SYSTEM_SFML)
	find_package(SFML REQUIRED COMPONENTS graphics window system)
else()
	set(SFML_INCLUDE_DIR		"${DEPENDENCIES_FOLDER}/sfml/include")
	if(MSVC)
		set(SFML_LIBRARIES
			optimized sfml-system-2   debug sfml-system-d-2
			optimized sfml-window-2   debug sfml-window-d-2
			optimized sfml-graphics-2 debug sfml-graphics-d-2
		)
	else()
		set(SFML_LIBRARIES
			sfml-graphics-s
			sfml-window-s
			sfml-system-s
		)
	endif()
	link_directories("${DEPENDENCIES_FOLDER}/sfml/lib/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/sfml/extlibs/lib")
	add_definitions( -DSFML_STATIC )
endif()

## FreeType ##
if(USE_SYSTEM_FREETYPE)
	find_package(Freetype REQUIRED)
else()
	set(FREETYPE_INCLUDE_DIRS	"${DEPENDENCIES_FOLDER}/freetype/include")
	set(FREETYPE_LIBRARIES freetype)
	link_directories("${DEPENDENCIES_FOLDER}/freetype/objs/win32/vc2010")
endif()

## FFmpeg ##
if(USE_SYSTEM_FFMPEG)
	find_package(FFmpeg REQUIRED COMPONENTS
		avcodec
		avdevice
		avfilter
		avformat
		avutil
		postproc
		swresample
		swscale)
else()
	set(FFmpeg_INCLUDE_DIRS		"${DEPENDENCIES_FOLDER}/ffmpeg/include")
	if (MSVC)
		list(APPEND FFmpeg_INCLUDE_DIRS "${DEPENDENCIES_FOLDER}/ffmpeg/include-win32")
	endif ()
	set(FFmpeg_BIN_DIR			"${DEPENDENCIES_FOLDER}/ffmpeg/bin/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/ffmpeg/lib/${PLATFORM_FOLDER_NAME}")
endif()

## FreeImage ##
if(USE_SYSTEM_FREEIMAGE)
	find_package(FreeImage REQUIRED)
else()
	set(FREEIMAGE_INCLUDE_DIRS	"${DEPENDENCIES_FOLDER}/freeimage/include")
	if(MSVC)
		set(FREEIMAGE_LIBRARIES optimized FreeImage debug FreeImaged)
	else()
		set(FREEIMAGE_LIBRARIES freeimage)
	endif()
	set(FREEIMAGE_BIN_DIR		"${DEPENDENCIES_FOLDER}/freeimage/bin/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/freeimage/lib/${PLATFORM_FOLDER_NAME}")
endif()

## OpenAL ##
if(USE_SYSTEM_OPENAL)
	find_package(OpenAL REQUIRED)
else()
	set(OPENAL_INCLUDE_DIR		"${DEPENDENCIES_FOLDER}/openal/include")
	if(MSVC)
		set(OPENAL_LIBRARY OpenAL32)
	else()
		set(OPENAL_LIBRARY openal)
	endif()
	set(OPENAL_BIN_DIR			"${DEPENDENCIES_FOLDER}/openal/bin/${PLATFORM_FOLDER_NAME}")
	link_directories("${DEPENDENCIES_FOLDER}/openal/lib/${PLATFORM_FOLDER_NAME}")
endif()

## AsmLib ##
set(ASMLIB_INCLUDE_DIRS			"${DEPENDENCIES_FOLDER}/asmlib")
if(MSVC)
	set(ASMLIB_LIBRARIES alibcof64)
else()
	set(ASMLIB_LIBRARIES :alibelf64.a)
endif()
link_directories("${DEPENDENCIES_FOLDER}/asmlib")

set(BLUEFISH_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/bluefish/include")
set(CEF_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/cef/include")
set(GTEST_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/gtest/include")

set(LIBERATION_FONTS_BIN_PATH	"${DEPENDENCIES_FOLDER}/liberation-fonts")
set(CEF_PATH					"${DEPENDENCIES_FOLDER}/cef")
set(CEF_BIN_PATH				"${DEPENDENCIES_FOLDER}/cef/bin/${PLATFORM_FOLDER_NAME}")

link_directories("${DEPENDENCIES_FOLDER}/bluefish/lib")
link_directories("${DEPENDENCIES_FOLDER}/zlib/lib")
link_directories("${DEPENDENCIES_FOLDER}/cef/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/gtest/lib/${PLATFORM_FOLDER_NAME}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions( -DUNICODE )
add_definitions( -D_UNICODE )
add_definitions( -D_GLIBCXX_USE_CXX11_ABI=0 ) # Allow compilation in GCC 5 while keeping old dependencies
add_definitions( -DCASPAR_SOURCE_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}" )

if (MSVC)
	set(CMAKE_CXX_FLAGS					"${CMAKE_CXX_FLAGS}					/EHa /Zi /W4 /WX /MP /fp:fast /Zm192 /FIcommon/compiler/vs/disable_silly_warnings.h")
	set(CMAKE_CXX_FLAGS_DEBUG			"${CMAKE_CXX_FLAGS_DEBUG}			/D TBB_USE_ASSERT=1 /D TBB_USE_DEBUG /bigobj")
	set(CMAKE_CXX_FLAGS_RELEASE			"${CMAKE_CXX_FLAGS_RELEASE}			/Oi /Ot /Gy /bigobj")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO	"${CMAKE_CXX_FLAGS_RELWITHDEBINFO}	/Oi /Ot /Gy /bigobj /Ob2")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
	add_compile_options( -std=c++11 )
	add_compile_options( -msse3 )
	add_compile_options( -mssse3 )
	add_compile_options( -msse4.1 )
	add_compile_options( -pthread )
	add_compile_options( -fPIC )
	add_compile_options( -fnon-call-exceptions ) # Allow signal handler to throw exception
	add_definitions( -DBOOST_NO_SWPRINTF ) # swprintf on Linux seems to always use , as decimal point regardless of C-locale or C++-locale
endif ()

if (POLICY CMP0045)
	cmake_policy(SET CMP0045 OLD)
endif ()

include(CMake/PrecompiledHeader.cmake)

set(CASPARCG_MODULE_INCLUDE_STATEMENTS							"" CACHE INTERNAL "")
set(CASPARCG_MODULE_INIT_STATEMENTS								"" CACHE INTERNAL "")
set(CASPARCG_MODULE_UNINIT_STATEMENTS							"" CACHE INTERNAL "")
set(CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS	"" CACHE INTERNAL "")
set(CASPARCG_MODULE_PROJECTS									"" CACHE INTERNAL "")
set(CASPARCG_RUNTIME_DEPENDENCIES								"" CACHE INTERNAL "")

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

add_subdirectory(accelerator)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(modules)

add_subdirectory(protocol)
add_subdirectory(shell)
add_subdirectory(unit-test)
