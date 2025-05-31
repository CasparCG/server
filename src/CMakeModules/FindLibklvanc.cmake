# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# Based off cmake-3.25/Modules/FindLibinput.cmake

#[=======================================================================[.rst:
FindLibklvanc
------------

Find libklvanc headers and library.

Imported Targets
^^^^^^^^^^^^^^^^

``Libklvanc::Libklvanc``
  The libklvanc library, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project:

``Libklvanc_FOUND``
  true if (the requested version of) libklvanc is available.
``Libklvanc_VERSION``
  the version of libklvanc.
``Libklvanc_LIBRARIES``
  the libraries to link against to use libklvanc.
``Libklvanc_INCLUDE_DIRS``
  where to find the libklvanc headers.
``Libklvanc_COMPILE_OPTIONS``
  this should be passed to target_compile_options(), if the
  target is not used for linking

#]=======================================================================]


# Use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig QUIET)
pkg_check_modules(PKG_Libklvanc QUIET libklvanc)

set(Libklvanc_COMPILE_OPTIONS ${PKG_Libklvanc_CFLAGS_OTHER})
set(Libklvanc_VERSION ${PKG_Libklvanc_VERSION})

find_path(Libklvanc_INCLUDE_DIR
  NAMES
    vanc-eia_608.h
  HINTS
    ${PKG_Libklvanc_INCLUDE_DIRS}
)
find_library(Libklvanc_LIBRARY
  NAMES
    klvanc
  HINTS
    ${PKG_Libklvanc_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libklvanc
  FOUND_VAR
    Libklvanc_FOUND
  REQUIRED_VARS
    Libklvanc_LIBRARY
    Libklvanc_INCLUDE_DIR
  VERSION_VAR
    Libklvanc_VERSION
)

if(Libklvanc_FOUND AND NOT TARGET Libklvanc::Libklvanc)
  add_library(Libklvanc::Libklvanc UNKNOWN IMPORTED)
  set_target_properties(Libklvanc::Libklvanc PROPERTIES
    IMPORTED_LOCATION "${Libklvanc_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${Libklvanc_COMPILE_OPTIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${Libklvanc_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Libklvanc_LIBRARY Libklvanc_INCLUDE_DIR)

if(Libklvanc_FOUND)
  set(Libklvanc_LIBRARIES ${Libklvanc_LIBRARY})
  set(Libklvanc_INCLUDE_DIRS ${Libklvanc_INCLUDE_DIR})
endif()
