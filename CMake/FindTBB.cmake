# - Try to find TBB components (adapted from FindFFmpeg.cmake)
#
# Once done this will define
#  TBB_FOUND         - System has all TBB components
#  TBB_INCLUDE_DIRS  - The TBB include directories
#  TBB_LIBRARIES     - The libraries needed to use TBB components
#  TBB_LIBRARY_DIRS  - Library directories
#  TBB_DEFINITIONS   - Compiler switches required for using TBB components
#
# For each of the components
#   - tbb
#   - tbbmalloc
#   - tbbmalloc_proxy
# the following variables will be defined
#  <component>_FOUND        - System has <component>
#  <component>_INCLUDE_DIRS - The <component> include directories
#  <component>_LIBRARIES    - The libraries needed to use <component>
#  <component>_DEFINITIONS  - Compiler switches required for using <component>
#
# Original FindFFmpeg.cmake copyrights:
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
#
# Copyright (c) 2015, Dimitry Ishenko, <dimitry.ishenko@gmail.com>
#
# Distributed under BSD license.

include(FindPackageHandleStandardArgs)

if(NOT TBB_FIND_COMPONENTS)
    set(TBB_FIND_COMPONENTS tbb tbbmalloc tbbmalloc_proxy)
endif()

#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up
# the libraries and include directories
#
macro(find_component _component _pkgconfig _library _header)
    if(NOT WIN32)
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(PC_${_component} ${_pkgconfig})
        endif()
    endif()

    find_path(${_component}_INCLUDE_DIRS ${_header}
        HINTS ${PC_${_component}_INCLUDEDIR}
              ${PC_${_component}_INCLUDE_DIRS})

    find_library(${_component}_LIBRARIES NAMES ${_library}
        HINTS ${PC_${_component}_LIBDIR}
              ${PC_${_component}_LIBRARY_DIRS})

    set(${_component}_DEFINITIONS  ${PC_${_component}_CFLAGS_OTHER})

    if(${_component}_LIBRARIES AND ${_component}_INCLUDE_DIRS)
        set(${_component}_FOUND TRUE)
    endif ()

    mark_as_advanced(${_component}_INCLUDE_DIRS ${_component}_LIBRARIES ${_component}_DEFINITIONS)
endmacro()

find_component(tbb             tbb             tbb             tbb/tbb.h)
find_component(tbbmalloc       tbbmalloc       tbbmalloc       tbb/tbb_allocator.h)
find_component(tbbmalloc_proxy tbbmalloc_proxy tbbmalloc_proxy tbb/tbbmalloc_proxy.h)

foreach(_component ${TBB_FIND_COMPONENTS})
    if(${_component}_FOUND)
        list(APPEND TBB_INCLUDE_DIRS ${${_component}_INCLUDE_DIRS})
        list(APPEND TBB_LIBRARIES    ${${_component}_LIBRARIES})

        get_filename_component(_dir  ${${_component}_LIBRARIES} PATH)
        list(APPEND TBB_LIBRARY_DIRS ${_dir})

        list(APPEND TBB_DEFINITIONS  ${${_component}_DEFINITIONS})
    endif()
endforeach()

if(TBB_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES TBB_INCLUDE_DIRS)
endif()

if(TBB_LIBRARY_DIRS)
    list(REMOVE_DUPLICATES TBB_LIBRARY_DIRS)
endif ()

mark_as_advanced(TBB_INCLUDE_DIRS TBB_LIBRARY_DIRS TBB_LIBRARIES TBB_DEFINITIONS)

set(_TBB_REQUIRED_VARS TBB_LIBRARIES TBB_INCLUDE_DIRS)
foreach(_component ${TBB_FIND_COMPONENTS})
    list(APPEND _TBB_REQUIRED_VARS ${_component}_LIBRARIES ${_component}_INCLUDE_DIRS)
endforeach ()

find_package_handle_standard_args(TBB DEFAULT_MSG ${_TBB_REQUIRED_VARS})
