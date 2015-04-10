cmake_minimum_required (VERSION 2.6)
project ("CasparCG Server")

find_package(Git)

set(GIT_REV "N/A")

if (GIT_FOUND)
	exec_program("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}"
			ARGS rev-parse --verify --short HEAD
			OUTPUT_VARIABLE GIT_REV)
endif ()

configure_file("${PROJECT_SOURCE_DIR}/version.tmpl" "${PROJECT_SOURCE_DIR}/version.h")

set(DEPENDENCIES_FOLDER			"${PROJECT_SOURCE_DIR}/dependencies64")

set(BOOST_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/boost")
set(RXCPP_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/RxCpp/include")
set(TBB_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/tbb/include")
set(GLEW_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/glew/include")
set(SFML_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/sfml/include")
set(FREETYPE_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/freetype/include")

if (MSVC)
	set(FFMPEG_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/ffmpeg/include" "${DEPENDENCIES_FOLDER}/ffmpeg/include-win32")
else ()
	set(FFMPEG_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/ffmpeg/include")
endif ()

set(ASMLIB_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/asmlib")
set(FREEIMAGE_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/freeimage/include")
set(OPENAL_INCLUDE_PATH			"${DEPENDENCIES_FOLDER}/openal/include")
set(BLUEFISH_INCLUDE_PATH		"${DEPENDENCIES_FOLDER}/bluefish/include")

if (MSVC)
	set(PLATFORM_FOLDER_NAME	"win32")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(PLATFORM_FOLDER_NAME	"linux")
endif ()

set(FFMPEG_BIN_PATH				"${DEPENDENCIES_FOLDER}/ffmpeg/bin/${PLATFORM_FOLDER_NAME}")
set(FREEIMAGE_BIN_PATH			"${DEPENDENCIES_FOLDER}/freeimage/bin/${PLATFORM_FOLDER_NAME}")
set(GLEW_BIN_PATH				"${DEPENDENCIES_FOLDER}/glew/bin")
set(OPENAL_BIN_PATH				"${DEPENDENCIES_FOLDER}/openal/bin")
set(TBB_BIN_PATH				"${DEPENDENCIES_FOLDER}/tbb/bin/${PLATFORM_FOLDER_NAME}")
set(LIBERATION_FONTS_BIN_PATH	"${DEPENDENCIES_FOLDER}/liberation-fonts")

link_directories("${DEPENDENCIES_FOLDER}/boost/stage/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/tbb/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/glew/lib")
link_directories("${DEPENDENCIES_FOLDER}/sfml/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/sfml/extlibs/lib")
link_directories("${DEPENDENCIES_FOLDER}/freetype/objs/win32/vc2010")
link_directories("${DEPENDENCIES_FOLDER}/ffmpeg/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/asmlib")
link_directories("${DEPENDENCIES_FOLDER}/freeimage/lib/${PLATFORM_FOLDER_NAME}")
link_directories("${DEPENDENCIES_FOLDER}/openal/lib")
link_directories("${DEPENDENCIES_FOLDER}/bluefish/lib")
link_directories("${DEPENDENCIES_FOLDER}/zlib/lib")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions( -DSFML_STATIC )
add_definitions( -DBOOST_THREAD_VERSION=4 )
add_definitions( -DTBB_USE_CAPTURED_EXCEPTION=0 )
add_definitions( -DUNICODE )
add_definitions( -D_UNICODE )

if (MSVC)
	set(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS}			/EHa /Zi /W4 /WX /MP /fp:fast /FIcommon/compiler/vs/disable_silly_warnings.h")
	set(CMAKE_CXX_FLAGS_DEBUG	"${CMAKE_CXX_FLAGS_DEBUG}	/D TBB_USE_ASSERT=1 /D TBB_USE_DEBUG /bigobj")
	set(CMAKE_CXX_FLAGS_RELEASE	"${CMAKE_CXX_FLAGS_RELEASE}	/Oi /Ot /Gy")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	add_compile_options( -std=c++11 )
	add_compile_options( -msse3 )
	add_compile_options( -mssse3 )
	add_compile_options( -msse4.1 )
	add_compile_options( -pthread )
endif ()

cmake_policy(SET CMP0045 OLD)
include(CMake/PrecompiledHeader.cmake)

set(CASPARCG_MODULE_INCLUDE_STATEMENTS	"" CACHE INTERNAL "")
set(CASPARCG_MODULE_INIT_STATEMENTS		"" CACHE INTERNAL "")
set(CASPARCG_MODULE_UNINIT_STATEMENTS	"" CACHE INTERNAL "")
set(CASPARCG_MODULE_PROJECTS			"" CACHE INTERNAL "")

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
	set(CASPARCG_MODULE_UNINIT_STATEMENTS "${CASPARCG_MODULE_UNINIT_STATEMENTS}"
			"	${UNINIT_FUNCTION_NAME}()\;"
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

add_subdirectory(accelerator)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(modules)

add_subdirectory(protocol)
add_subdirectory(shell)
