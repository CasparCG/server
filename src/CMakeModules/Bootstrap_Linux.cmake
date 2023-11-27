cmake_minimum_required (VERSION 3.16)

include(ExternalProject)

# Determine build (target) platform
SET (PLATFORM_FOLDER_NAME "linux")

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE (STATUS "Setting build type to 'Release' as none was specified.")
	SET (CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
	SET_PROPERTY (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF ()
MARK_AS_ADVANCED (CMAKE_INSTALL_PREFIX)

if (NOT USE_SYSTEM_BOOST)
	SET (BOOST_ROOT_PATH "/opt/boost" CACHE STRING "Path to Boost")
	SET (ENV{BOOST_ROOT} "${BOOST_ROOT_PATH}")
	SET (Boost_USE_DEBUG_LIBS OFF)
	SET (Boost_USE_RELEASE_LIBS ON)
	SET (Boost_USE_STATIC_LIBS ON)
endif()
FIND_PACKAGE (Boost 1.67.0 COMPONENTS system thread chrono filesystem log locale regex date_time coroutine REQUIRED)

if (NOT USE_SYSTEM_FFMPEG)
	SET (FFMPEG_ROOT_PATH "/opt/ffmpeg/lib/pkgconfig" CACHE STRING "Path to FFMPEG")
	SET (ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${FFMPEG_ROOT_PATH}")
endif()
FIND_PACKAGE (FFmpeg REQUIRED)
LINK_DIRECTORIES( ${FFMPEG_LIBRARY_DIRS} )

FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (FreeImage REQUIRED)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (TBB REQUIRED)
FIND_PACKAGE (OpenAL REQUIRED)
FIND_PACKAGE (SFML 2 COMPONENTS graphics window system REQUIRED)
FIND_PACKAGE (X11 REQUIRED)

if (ENABLE_HTML)
	casparcg_add_external_project(cef)
	ExternalProject_Add(cef
		URL https://cef-builds.spotifycdn.com/cef_binary_117.2.5%2Bgda4c36a%2Bchromium-117.0.5938.152_linux64_minimal.tar.bz2
		URL_HASH SHA1=7e6c9cf591cf3b1dabe65a7611f5fc166df2ec1e
		DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
		CMAKE_ARGS -DUSE_SANDBOX=Off
		INSTALL_COMMAND ""
		PATCH_COMMAND git apply ${CASPARCG_PATCH_DIR}/cef117.patch
		BUILD_BYPRODUCTS
			"<SOURCE_DIR>/Release/libcef.so"
			"<BINARY_DIR>/libcef_dll_wrapper/libcef_dll_wrapper.a"
	)
	ExternalProject_Get_Property(cef SOURCE_DIR)
	ExternalProject_Get_Property(cef BINARY_DIR)

	# Note: All of these must be referenced in the BUILD_BYPRODUCTS above, to satisfy ninja
	set(CEF_LIB
		"${SOURCE_DIR}/Release/libcef.so" 
		"${BINARY_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.a"
	)

	set(CEF_INCLUDE_PATH "${SOURCE_DIR}")
	set(CEF_BIN_PATH "${SOURCE_DIR}/Release")
	set(CEF_RESOURCE_PATH "${SOURCE_DIR}/Resources")
endif ()

SET (BOOST_INCLUDE_PATH "${Boost_INCLUDE_DIRS}")
SET (TBB_INCLUDE_PATH "${TBB_INCLUDE_DIRS}")
SET (GLEW_INCLUDE_PATH "${GLEW_INCLUDE_DIRS}")
SET (SFML_INCLUDE_PATH "${SFML_INCLUDE_DIR}")
SET (FFMPEG_INCLUDE_PATH "${FFMPEG_INCLUDE_DIRS}")
SET (FREEIMAGE_INCLUDE_PATH "${FreeImage_INCLUDE_DIRS}")

SET_PROPERTY (GLOBAL PROPERTY USE_FOLDERS ON)

ADD_DEFINITIONS (-DSFML_STATIC)
ADD_DEFINITIONS (-DUNICODE)
ADD_DEFINITIONS (-D_UNICODE)
ADD_DEFINITIONS (-DGLEW_NO_GLU)
ADD_DEFINITIONS (-D__NO_INLINE__) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_NO_SWPRINTF) # swprintf on Linux seems to always use , as decimal point regardless of C-locale or C++-locale
ADD_DEFINITIONS (-DTBB_USE_CAPTURED_EXCEPTION=1)
ADD_DEFINITIONS (-DNDEBUG) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_LOCALE_HIDE_AUTO_PTR) # Needed for C++17 in boost 1.67+


if (USE_SYSTEM_BOOST)
	ADD_DEFINITIONS (-DBOOST_ALL_DYN_LINK)
endif()

IF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	ADD_COMPILE_OPTIONS (-O3) # Needed for precompiled headers to work
endif()
IF (CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x64|x86_64|amd64|e2k)")
    ADD_COMPILE_OPTIONS (-msse3)
    ADD_COMPILE_OPTIONS (-mssse3)
    ADD_COMPILE_OPTIONS (-msse4.1)
ELSE ()
    ADD_COMPILE_DEFINITIONS (USE_SIMDE)
ENDIF ()
ADD_COMPILE_OPTIONS (-fnon-call-exceptions) # Allow signal handler to throw exception

ADD_COMPILE_OPTIONS (-Wno-deprecated-declarations -Wno-write-strings -Wno-multichar -Wno-cpp -Werror)
IF (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    ADD_COMPILE_OPTIONS (-Wno-terminate)
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Help TBB figure out what compiler support for c++11 features
    # https://github.com/01org/tbb/issues/22
    string(REPLACE "." "0" TBB_USE_GLIBCXX_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    message(STATUS "ADDING: -DTBB_USE_GLIBCXX_VERSION=${TBB_USE_GLIBCXX_VERSION}")
    add_definitions(-DTBB_USE_GLIBCXX_VERSION=${TBB_USE_GLIBCXX_VERSION})
ENDIF ()

IF (POLICY CMP0045)
	CMAKE_POLICY (SET CMP0045 OLD)
ENDIF ()
