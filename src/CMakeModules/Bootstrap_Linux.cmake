cmake_minimum_required (VERSION 3.16)

include(ExternalProject)
include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
# Prefer the new boost helper
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(ENABLE_HTML ON CACHE BOOL "Enable CEF and HTML producer")
set(USE_STATIC_BOOST OFF CACHE BOOL "Use shared library version of Boost")
set(USE_SYSTEM_CEF ON CACHE BOOL "Use the version of cef from your OS (only tested with Ubuntu)")
set(SKIP_SHADER_CHECK OFF CACHE BOOL "Skip the GLSL shader check (useful if glslangValidator is not available for your platform)")

# Determine build (target) platform
SET (PLATFORM_FOLDER_NAME "linux")

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE (STATUS "Setting build type to 'Release' as none was specified.")
	SET (CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
	SET_PROPERTY (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF ()
MARK_AS_ADVANCED (CMAKE_INSTALL_PREFIX)

if (USE_STATIC_BOOST)
	SET (Boost_USE_STATIC_LIBS ON)
endif()
FIND_PACKAGE (Boost 1.74.0 COMPONENTS system thread chrono filesystem log_setup log locale regex date_time coroutine REQUIRED)
FIND_PACKAGE (FFmpeg REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED COMPONENTS OpenGL GLX)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (TBB REQUIRED)
FIND_PACKAGE (OpenAL REQUIRED)
FIND_PACKAGE (SFML 2 COMPONENTS graphics window system REQUIRED)
FIND_PACKAGE (X11 REQUIRED)

if (ENABLE_HTML)
    if (USE_SYSTEM_CEF)
        set(CEF_LIB_PATH "/usr/lib/casparcg-cef-131")
        set(CEF_INCLUDE_PATH "/usr/include/casparcg-cef-131")

        set(CEF_LIB
            "-Wl,-rpath,${CEF_LIB_PATH} ${CEF_LIB_PATH}/libcef.so"
            "${CEF_LIB_PATH}/libcef_dll_wrapper.a"
        )
    else()
        casparcg_add_external_project(cef)
        ExternalProject_Add(cef
            URL ${CASPARCG_DOWNLOAD_MIRROR}/cef/cef_binary_131.4.1%2Bg437feba%2Bchromium-131.0.6778.265_linux64_minimal.tar.bz2
            URL_HASH SHA1=cbe52ac3c39ef93fdc5021588e12c466e801d9af
            DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
            CMAKE_ARGS -DUSE_SANDBOX=Off
            INSTALL_COMMAND ""
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
    endif()
endif ()

SET (BOOST_INCLUDE_PATH "${Boost_INCLUDE_DIRS}")
SET (TBB_INCLUDE_PATH "${TBB_INCLUDE_DIRS}")
SET (GLEW_INCLUDE_PATH "${GLEW_INCLUDE_DIRS}")
SET (SFML_INCLUDE_PATH "${SFML_INCLUDE_DIR}")
SET (FFMPEG_INCLUDE_PATH "${FFMPEG_INCLUDE_DIRS}")

LINK_DIRECTORIES("${FFMPEG_LIBRARY_DIRS}")

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


if (NOT USE_STATIC_BOOST)
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
