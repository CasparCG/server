cmake_minimum_required (VERSION 3.16)

# Project settings
PROJECT ("CasparCG Server")
SET (CONFIG_VERSION_MAJOR 2)
SET (CONFIG_VERSION_MINOR 4)
SET (CONFIG_VERSION_BUG 0)
SET (CONFIG_VERSION_TAG "Beta1")

set(CASPARCG_DOWNLOAD_MIRROR https://github.com/CasparCG/dependencies/releases/download/ CACHE STRING "Source/mirror to use for external dependencies")
set(CASPARCG_DOWNLOAD_CACHE ${CMAKE_CURRENT_BINARY_DIR}/external CACHE STRING "Download cache directory for cmake ExternalProjects")

option(ENABLE_HTML "Enable HTML module, require CEF" ON)

# Add custom cmake modules path
SET (CASPARCG_PATCH_DIR ${CMAKE_SOURCE_DIR}/CMakeModules/patches)
LIST (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

# Determine the git hash
FIND_PACKAGE (Git)
SET (CONFIG_VERSION_GIT_HASH "N/A")
IF (DEFINED ENV{GIT_HASH} AND NOT $ENV{GIT_HASH} STREQUAL "")
	SET (CONFIG_VERSION_GIT_HASH "$ENV{GIT_HASH}")
ELSEIF (GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/../.git")
	EXEC_PROGRAM ("${GIT_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/../" ARGS rev-parse --verify --short HEAD OUTPUT_VARIABLE CONFIG_VERSION_GIT_HASH)
ENDIF ()

cmake_host_system_information(RESULT CONFIG_CPU_COUNT QUERY NUMBER_OF_PHYSICAL_CORES)

INCLUDE (CasparCG_Util)
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/version.tmpl" "${CMAKE_BINARY_DIR}/generated/version.h")

if (MSVC)
	INCLUDE (Bootstrap_Windows)
else ()
	INCLUDE (Bootstrap_Linux)

	# Output build settings
	MESSAGE (STATUS "Target architecture: ${CMAKE_SYSTEM_PROCESSOR}")
	MESSAGE (STATUS "CPU Count: ${CONFIG_CPU_COUNT}")
	MESSAGE (STATUS "Target build type: ${CMAKE_BUILD_TYPE}")
	MESSAGE (STATUS "Build Version: ${CONFIG_VERSION_MAJOR}.${CONFIG_VERSION_MINOR}.${CONFIG_VERSION_BUG} ${CONFIG_VERSION_TAG} (Revision: ${CONFIG_VERSION_GIT_HASH})")
endif ()

INCLUDE_DIRECTORIES ("${CMAKE_BINARY_DIR}/generated")

ADD_SUBDIRECTORY (tools)
ADD_SUBDIRECTORY (accelerator)
ADD_SUBDIRECTORY (common)
ADD_SUBDIRECTORY (core)
ADD_SUBDIRECTORY (modules)
ADD_SUBDIRECTORY (protocol)
ADD_SUBDIRECTORY (shell)
