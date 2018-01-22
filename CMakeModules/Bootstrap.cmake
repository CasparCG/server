INCLUDE (PrecompiledHeader)

# Determine build (target) platform
INCLUDE (PlatformIntrospection)
TEST_FOR_SUPPORTED_PLATFORM (SUPPORTED_PLATFORM)
_DETERMINE_PLATFORM (CONFIG_PLATFORM)
_DETERMINE_ARCH (CONFIG_ARCH)
_DETERMINE_CPU_COUNT (CONFIG_CPU_COUNT)

# Read build type from cmake options (Default: Release)
OPTION (RELEASE_BUILD "Is this a release build?" TRUE)
IF (RELEASE_BUILD)
	SET (CMAKE_BUILD_TYPE "Release")
ELSE ()
	SET (CMAKE_BUILD_TYPE "Debug")
ENDIF ()

FIND_PACKAGE (Git)
SET (CONFIG_VERSION_GIT_REV "0")
SET (CONFIG_VERSION_GIT_HASH "N/A")
IF (GIT_FOUND)
	EXEC_PROGRAM ("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}" ARGS rev-list --all --count OUTPUT_VARIABLE CONFIG_VERSION_GIT_REV)
	EXEC_PROGRAM ("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}" ARGS rev-parse --verify --short HEAD OUTPUT_VARIABLE CONFIG_VERSION_GIT_HASH)
ENDIF ()
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/version.tmpl" "${PROJECT_SOURCE_DIR}/version.h")

FIND_PACKAGE (Boost COMPONENTS system thread chrono filesystem log locale regex date_time REQUIRED)
FIND_PACKAGE (FFmpeg REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (JPEG REQUIRED)
FIND_PACKAGE (FreeImage REQUIRED)
FIND_PACKAGE (Freetype REQUIRED)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (TBB REQUIRED)
FIND_PACKAGE (SndFile REQUIRED)
FIND_PACKAGE (OpenAL REQUIRED)
FIND_PACKAGE (GLFW REQUIRED)
FIND_PACKAGE (SFML 2 COMPONENTS graphics window system REQUIRED)

# TO CLEANUP!!

set(BOOST_INCLUDE_PATH			"${Boost_INCLUDE_DIRS}")
set(RXCPP_INCLUDE_PATH			"${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/RxCpp/include")
set(TBB_INCLUDE_PATH			"${TBB_INCLUDE_DIRS}")
set(GLEW_INCLUDE_PATH			"${GLEW_INCLUDE_DIRS}")
set(SFML_INCLUDE_PATH			"${SFML_INCLUDE_DIR}")
set(FREETYPE_INCLUDE_PATH		"${FREETYPE_INCLUDE_DIRS}")
set(FFMPEG_INCLUDE_PATH			"${FFMPEG_INCLUDE_DIRS}")
set(ASMLIB_INCLUDE_PATH			"${EXTERNAL_INCLUDE_PATH}")
set(FREEIMAGE_INCLUDE_PATH		"${FreeImage_INCLUDE_DIRS}")
set(CEF_INCLUDE_PATH			"${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/cef/include")

if (MSVC)
	set(PLATFORM_FOLDER_NAME	"win32")
else()
	set(PLATFORM_FOLDER_NAME	"linux")
endif ()

set(LIBERATION_FONTS_BIN_PATH	"${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/liberation-fonts")
set(CEF_PATH					"${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/cef")
set(CEF_BIN_PATH				"${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/cef/bin/linux")

link_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies64/cef/lib/linux")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions( -DSFML_STATIC )
add_definitions( -DTBB_USE_CAPTURED_EXCEPTION=0 )
add_definitions( -DUNICODE )
add_definitions( -D_UNICODE )
add_definitions( -DGLEW_NO_GLU )
add_definitions( -DCASPAR_SOURCE_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}" )

if (MSVC)
	set(CMAKE_CXX_FLAGS					"${CMAKE_CXX_FLAGS}					/EHa /Zi /W4 /WX /MP /fp:fast /Zm192 /FIcommon/compiler/vs/disable_silly_warnings.h")
	set(CMAKE_CXX_FLAGS_DEBUG			"${CMAKE_CXX_FLAGS_DEBUG}			/D TBB_USE_ASSERT=1 /D TBB_USE_DEBUG /bigobj")
	set(CMAKE_CXX_FLAGS_RELEASE			"${CMAKE_CXX_FLAGS_RELEASE}			/Oi /Ot /Gy /bigobj")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO	"${CMAKE_CXX_FLAGS_RELWITHDEBINFO}	/Oi /Ot /Gy /bigobj /Ob2")
else()
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_compile_options( -msse3 )
	add_compile_options( -mssse3 )
	add_compile_options( -msse4.1 )
	add_compile_options( -fnon-call-exceptions ) # Allow signal handler to throw exception
	add_compile_options( -Wno-max-unsigned-zero -Wno-inconsistent-missing-override -Wno-inconsistent-missing-override -Wno-macro-redefined -Wno-unsequenced -Wno-deprecated-declarations -Wno-logical-not-parentheses -Wno-switch -Wno-tautological-constant-out-of-range-compare -Wno-tautological-compare -Wno-writable-strings -Wno-dangling-else )
	add_definitions( -DBOOST_NO_SWPRINTF ) # swprintf on Linux seems to always use , as decimal point regardless of C-locale or C++-locale
	add_definitions( -DTBB_USE_CAPTURED_EXCEPTION=1 )
	add_definitions( -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LINK )
endif ()

if (POLICY CMP0045)
	cmake_policy(SET CMP0045 OLD)
endif ()

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
