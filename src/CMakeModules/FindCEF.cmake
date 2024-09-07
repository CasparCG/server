# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:

FindCEF
-------

Find the CEF library.

Imported Targets
^^^^^^^^^^^^^^^^

``CEF::CEF``

Result Variables
^^^^^^^^^^^^^^^^

``CEF_FOUND``
``CEF_VERSION``
``CEF_INCLUDE_DIRS``
``CEF_LIBRARIES``

#]=======================================================================]

if(NOT CEF_FIND_VERSION_MAJOR)
  message(FATAL_ERROR "Missing CEF major version")
endif()

find_path(CEF_INCLUDE_DIR
  NAMES cef_app.h include/cef_app.h
  PATHS ${CEF_CUSTOM_INCLUDE_DIRS}
  PATH_SUFFIXES cef/${CEF_FIND_VERSION_MAJOR}
)

find_library(CEF_LIBRARY
  NAMES cef
  PATHS ${CEF_CUSTOM_LIBRARY_DIRS}
  PATH_SUFFIXES cef/${CEF_FIND_VERSION_MAJOR}
)

find_library(CEF_WRAPPER_LIBRARY
  NAMES cef_dll_wrapper
  PATHS ${CEF_CUSTOM_LIBRARY_DIRS}
  PATH_SUFFIXES cef/${CEF_FIND_VERSION_MAJOR}
)

set(CEF_VERSION ${CEF_FIND_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CEF
  FOUND_VAR CEF_FOUND
  REQUIRED_VARS
    CEF_INCLUDE_DIR
    CEF_LIBRARY
    CEF_WRAPPER_LIBRARY
  VERSION_VAR CEF_VERSION
)

if(CEF_FOUND)
  set(CEF_LIBRARIES ${CEF_LIBRARY} ${CEF_WRAPPER_LIBRARY})
  set(CEF_INCLUDE_DIRS ${CEF_INCLUDE_DIR})
endif()

if(CEF_FOUND AND NOT TARGET CEF::CEF)
  add_library(CEF::Lib UNKNOWN IMPORTED)
  set_target_properties(CEF::Lib PROPERTIES
    IMPORTED_LOCATION "${CEF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CEF_INCLUDE_DIR}"
  )

  add_library(CEF::Wrapper UNKNOWN IMPORTED)
  set_target_properties(CEF::Wrapper PROPERTIES
    IMPORTED_LOCATION "${CEF_WRAPPER_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CEF_INCLUDE_DIR}"
  )

  add_library(CEF::CEF INTERFACE IMPORTED)
  set_property(TARGET CEF::CEF PROPERTY
    INTERFACE_LINK_LIBRARIES CEF::Lib CEF::Wrapper
  )
endif()

mark_as_advanced(CEF_INCLUDE_DIR CEF_LIBRARY CEF_WRAPPER_LIBRARY)
