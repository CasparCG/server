# Find FreeImage includes and library
#
# This module defines
#  FreeImage_INCLUDE_DIRS
#  FreeImage_LIBRARIES, the libraries to link against to use FreeImage.
#  FreeImage_LIBRARY_DIRS, the location of the libraries
#  FreeImage_FOUND, If false, do not try to use FreeImage
#
# Copyright © 2007, Matt Williams
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)
	SET(FreeImage_FIND_QUIETLY TRUE) # Already in cache, be silent
ELSE(FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)
	MESSAGE(STATUS "Looking for FreeImage")
ENDIF (FreeImage_LIBRARIES AND FreeImage_INCLUDE_DIRS)

SET(FreeImage_INCLUDE_SEARCH_DIRS
	${FreeImage_LIBRARY_SEARCH_DIRS}
	${CMAKE_LIBRARY_PATH}
	/opt/freeimage/lib
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/lib
	/usr/local/lib64
	/usr/lib
	/usr/lib64
	/sw/lib
	/opt/local/lib
	/opt/csw/lib
	/opt/lib
	/usr/freeware/lib64
)

SET(FreeImage_LIBRARY_SEARCH_DIRS
	${FreeImage_LIBRARY_SEARCH_DIRS}
	${CMAKE_LIBRARY_PATH}
	/opt/freeimage/lib
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/lib
	/usr/local/lib64
	/usr/lib
	/usr/lib64
	/sw/lib
	/opt/local/lib
	/opt/csw/lib
	/opt/lib
	/usr/freeware/lib64
)

FIND_PATH(FreeImage_INCLUDE_DIRS FreeImage.h ${FreeImage_INCLUDE_SEARCH_DIRS})
FIND_LIBRARY(FreeImage_LIBRARIES freeimage PATHS ${FreeImage_LIBRARY_SEARCH_DIRS})

#Do some preparation
SEPARATE_ARGUMENTS(FreeImage_INCLUDE_DIRS)
SEPARATE_ARGUMENTS(FreeImage_LIBRARIES)

MARK_AS_ADVANCED(FreeImage_INCLUDE_DIRS FreeImage_LIBRARIES FreeImage_LIBRARY_DIRS)

IF (FreeImage_INCLUDE_DIRS AND FreeImage_LIBRARIES)
	SET(FreeImage_FOUND TRUE)
ENDIF (FreeImage_INCLUDE_DIRS AND FreeImage_LIBRARIES)

IF (FreeImage_FOUND)
	IF (NOT FreeImage_FIND_QUIETLY)
		MESSAGE(STATUS "  libraries : ${FreeImage_LIBRARIES} from ${FreeImage_LIBRARY_DIRS}")
		MESSAGE(STATUS "  includes  : ${FreeImage_INCLUDE_DIRS}")
	ENDIF (NOT FreeImage_FIND_QUIETLY)
ELSE (FreeImage_FOUND)
	IF (FreeImage_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find FreeImage")
	ENDIF (FreeImage_FIND_REQUIRED)
ENDIF (FreeImage_FOUND)
