# - Try to find FreeImage
#
# Once done this will define
#  FREEIMAGE_FOUND         - System has FreeImage
#  FREEIMAGE_INCLUDE_DIRS  - The FreeImage include directories
#  FREEIMAGE_LIBRARIES     - The libraries needed to use FreeImage
#  FREEIMAGE_LIBRARY_DIRS  - Library directories
#  FREEIMAGE_DEFINITIONS   - Compiler switches required for using FreeImage
#
# Copyright (c) 2015, Dimitry Ishenko, <dimitry.ishenko@gmail.com>
#
# Distributed under BSD license.

if(NOT WIN32)
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PC_FREEIMAGE freeimage)
    endif()
endif()

find_path(FREEIMAGE_INCLUDE_DIRS FreeImage.h
    HINTS ${PC_FREEIMAGE_INCLUDEDIR}
          ${PC_FREEIMAGE_INCLUDE_DIRS})

find_library(FREEIMAGE_LIBRARIES NAMES freeimage
    HINTS ${PC_FREEIMAGE_LIBDIR}
          ${PC_FREEIMAGE_LIBRARY_DIRS})

set(FREEIMAGE_DEFINITIONS ${PC_FREEEMAGE_CFLAGS_OTHER})

mark_as_advanced(FREEIMAGE_INCLUDE_DIRS FREEIMAGE_LIBRARIES FREEIMAGE_DEFINITIONS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FREEIMAGE DEFAULT_MSG FREEIMAGE_INCLUDE_DIRS FREEIMAGE_LIBRARIES)
