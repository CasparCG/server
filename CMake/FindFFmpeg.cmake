# vim: ts=2 sw=2
# - Try to find the required FFmpeg componenta (default: avformat, avutil, avcodec)
#
# Once done this will define
#  FFmpeg_FOUND         - System has all the required FFmpeg components
#  FFmpeg_INCLUDE_DIRS  - The FFmpeg components include directories
#  FFmpeg_LIBRARIES     - Libraries needed to use the FFmpeg components
#  FFmpeg_LIBRARY_DIRS  - Library directories
#  FFmpeg_DEFINITIONS   - Compiler switches required for using the FFmpeg components
#
# For each of the components
#   - avcodec
#   - avdevice
#   - avfilter
#   - avformat
#   - avresample
#   - avutil
#   - postproc
#   - swresample
#   - swscale
# the following variables will be defined
#  <component>_FOUND        - System has the <component>
#  <component>_INCLUDE_DIRS - The <component> include directories
#  <component>_LIBRARIES    - Libraries needed to use the <component>
#  <component>_DEFINITIONS  - Compiler switches required for using the <component>
#  <component>_VERSION      - The <component> version
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2015, Dimitry Ishenko, <dimitry.ishenko@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.

include(FindPackageHandleStandardArgs)

# The default components were taken from a survey over other FindFFmpeg.cmake files.
if(NOT FFmpeg_FIND_COMPONENTS)
  set(FFmpeg_FIND_COMPONENTS avcodec avformat avutil)
endif()

#
### Macro: set_component_found
#
# Marks the given component as found, 
# if both *_LIBRARIES AND *_INCLUDE_DIRS are present.
#
macro(set_component_found _component )
  if(${_component}_LIBRARIES AND ${_component}_INCLUDE_DIRS)
    set(${_component}_FOUND TRUE)
  endif()
endmacro()

#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig
# and then looking up the libraries and include directories.
#
macro(find_component _component _pkgconfig _library _header)

  if(NOT WIN32)
     find_package(PkgConfig)
     if(PKG_CONFIG_FOUND)
       pkg_check_modules(PC_${_component} ${_pkgconfig})
     endif()
  endif()

  find_path(${_component}_INCLUDE_DIRS ${_header}
    HINTS
      ${PC_${_component}_INCLUDEDIR}
      ${PC_${_component}_INCLUDE_DIRS}
    PATH_SUFFIXES
      ffmpeg
  )

  find_library(${_component}_LIBRARIES NAMES ${_library}
    HINTS
      ${PC_${_component}_LIBDIR}
      ${PC_${_component}_LIBRARY_DIRS}
  )

  set(${_component}_DEFINITIONS  ${PC_${_component}_CFLAGS_OTHER} CACHE STRING "The ${_component} CFLAGS.")
  set(${_component}_VERSION      ${PC_${_component}_VERSION}      CACHE STRING "The ${_component} version.")

  set_component_found(${_component})

  mark_as_advanced(
    ${_component}_INCLUDE_DIRS
    ${_component}_LIBRARIES
    ${_component}_DEFINITIONS
    ${_component}_VERSION
  )

endmacro()

# Check if the results have already been cached
if(NOT FFmpeg_LIBRARIES)

  find_component(avcodec    libavcodec    avcodec    libavcodec/avcodec.h)
  find_component(avfilter   libavfilter   avfilter   libavfilter/avfilter.h)
  find_component(avformat   libavformat   avformat   libavformat/avformat.h)
  find_component(avdevice   libavdevice   avdevice   libavdevice/avdevice.h)
  find_component(avresample libavresample avresample libavresample/avresample.h)
  find_component(avutil     libavutil     avutil     libavutil/avutil.h)
  find_component(swresample libswresample swresample libswresample/swresample.h)
  find_component(swscale    libswscale    swscale    libswscale/swscale.h)
  find_component(postproc   libpostproc   postproc   libpostproc/postprocess.h)

  foreach(_component ${FFmpeg_FIND_COMPONENTS})
    if(${_component}_FOUND)
      list(APPEND FFmpeg_LIBRARIES    ${${_component}_LIBRARIES})
      list(APPEND FFmpeg_INCLUDE_DIRS ${${_component}_INCLUDE_DIRS})

      get_filename_component(_dir     ${${_component}_LIBRARIES} PATH)
      list(APPEND FFmpeg_LIBRARY_DIRS ${_dir})

      list(APPEND FFmpeg_DEFINITIONS  ${${_component}_DEFINITIONS})
    endif()
  endforeach()

  if(FFmpeg_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES FFmpeg_INCLUDE_DIRS)
  endif()

  if(FFmpeg_LIBRARY_DIRS)
      list(REMOVE_DUPLICATES FFmpeg_LIBRARY_DIRS)
  endif()

  # Cache the results
  set(FFmpeg_INCLUDE_DIRS ${FFmpeg_INCLUDE_DIRS} CACHE STRING "The FFmpeg include directories." FORCE)
  set(FFmpeg_LIBRARIES    ${FFmpeg_LIBRARIES}    CACHE STRING "The FFmpeg libraries." FORCE)
  set(FFmpeg_LIBRARY_DIRS ${FFmpeg_LIBRARY_DIRS} CACHE STRING "The FFmpeg library directories." FORCE)
  set(FFmpeg_DEFINITIONS  ${FFmpeg_DEFINITIONS}  CACHE STRING "The FFmpeg CFLAGS." FORCE)

  mark_as_advanced(FFmpeg_INCLUDE_DIRS FFmpeg_LIBRARIES FFmpeg_LIBRARY_DIRS FFmpeg_DEFINITIONS)

endif()

# Set non-cached _FOUND vars for the components
foreach(_component avcodec avdevice avfilter avformat avresample avutil postproc swresample swscale)
  set_component_found(${_component})
endforeach()

# Make a list of required vars
set(_FFmpeg_REQUIRED_VARS FFmpeg_LIBRARIES FFmpeg_INCLUDE_DIRS)
foreach(_component ${FFmpeg_FIND_COMPONENTS})
  list(APPEND _FFmpeg_REQUIRED_VARS ${_component}_LIBRARIES ${_component}_INCLUDE_DIRS)
endforeach()

find_package_handle_standard_args(FFmpeg DEFAULT_MSG ${_FFmpeg_REQUIRED_VARS})
