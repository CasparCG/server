include(ProcessorCount)
# Returns a simple string describing the current architecture. Possible
# return values currently include: amd64, x86_64, x86.
MACRO (_DETERMINE_ARCH var)
  IF (DEFINED CB_DOWNLOAD_DEPS_ARCH)
    SET (_arch ${CB_DOWNLOAD_DEPS_ARCH})
  ELSEIF (DEFINED ENV{target_arch})
    # target_arch is used by environment.bat to represent the desired
    # target architecture, so use that value first if set.
    STRING (TOLOWER "$ENV{target_arch}" _arch)
  ELSE (DEFINED CB_DOWNLOAD_DEPS_ARCH)
    # We tweak MacOS, which for some reason claims to be i386
    IF (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      # QQQ MacOS 10.7 could be 32-bit; we should catch and abort
      SET (_arch x86_64)
    ELSEIF (CMAKE_SYSTEM_NAME STREQUAL "SunOS")
      EXECUTE_PROCESS (COMMAND isainfo -k
        COMMAND tr -d '\n'
        OUTPUT_VARIABLE _arch)
    ELSEIF (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
      # If user didn't specify the arch via target_arch or
      # CB_DOWNLOAD_DEPS_ARCH, assume that the target is the same as
      # the current host architecture and derive that from
      # Windows-provided environment variables.
      IF (DEFINED ENV{PROCESSOR_ARCHITEW6432})
        STRING (TOLOWER "$ENV{PROCESSOR_ARCHITEW6432}" _arch)
      ELSE ()
        STRING (TOLOWER "$ENV{PROCESSOR_ARCHITECTURE}" _arch)
      ENDIF ()
    ELSE (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      STRING (TOLOWER ${CMAKE_SYSTEM_PROCESSOR} _arch)
    ENDIF (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    SET (CB_DOWNLOAD_DEPS_ARCH ${_arch} CACHE STRING
      "Architecture for downloaded dependencies")
    MARK_AS_ADVANCED (CB_DOWNLOAD_DEPS_ARCH)
  ENDIF (DEFINED CB_DOWNLOAD_DEPS_ARCH)
  SET (${var} ${_arch})
ENDMACRO (_DETERMINE_ARCH)


# Returns a lowercased version of a given lsb_release field.
MACRO (_LSB_RELEASE field retval)
  EXECUTE_PROCESS (COMMAND lsb_release "--${field}"
  OUTPUT_VARIABLE _output ERROR_VARIABLE _output RESULT_VARIABLE _result)
  IF (_result)
    MESSAGE (FATAL_ERROR "Cannot determine Linux revision! Output from "
    "lsb_release --${field}: ${_output}")
  ENDIF (_result)
  STRING (REGEX REPLACE "^[^:]*:" "" _output "${_output}")
  STRING (TOLOWER "${_output}" _output)
  STRING (STRIP "${_output}" ${retval})
ENDMACRO (_LSB_RELEASE)


# Returns a simple string describing the current platform. Possible
# return values currently include: windows_msvc2015; windows_msvc (implies
# MSVC 2013); macosx; or any value from _DETERMINE_LINUX_DISTRO.
MACRO (_DETERMINE_PLATFORM var)
  IF (DEFINED CB_DOWNLOAD_DEPS_PLATFORM)
    SET (_plat ${CB_DOWNLOAD_DEPS_PLATFORM})
  ELSE (DEFINED CB_DOWNLOAD_DEPS_PLATFORM)
    SET (_plat ${CMAKE_SYSTEM_NAME})
    IF (_plat STREQUAL "Windows")
	  if (${MSVC_VERSION} LESS 1900)
	    SET (_plat "windows_msvc")
	  elseif (${MSVC_VERSION} LESS 1910)
	    SET (_plat "windows_msvc2015")
	  elseif (${MSVC_VERSION} EQUAL 1910)
	    SET (_plat "windows_msvc2017")
	  ELSE()
        MESSAGE(FATAL_ERROR "Unsupported MSVC version: ${MSVC_VERSION}")
      ENDIF ()
    ELSEIF (_plat STREQUAL "Darwin")
      SET (_plat "macosx")
    ELSEIF (_plat STREQUAL "Linux")
      FIND_PROGRAM(LSB_RELEASE lsb_release)
      MARK_AS_ADVANCED(LSB_RELEASE)
      IF (LSB_RELEASE)
        _DETERMINE_LINUX_DISTRO (_plat)
      ELSE (LSB_RELEASE)
        MESSAGE (WARNING "Can't determine Linux platform without lsb_release")
        SET (_plat "unknown")
      ENDIF (LSB_RELEASE)
    ELSEIF (_plat STREQUAL "SunOS")
      SET (_plat "sunos")
    ELSEIF (_plat STREQUAL "FreeBSD")
      SET (_plat "freebsd")
    ELSE (_plat STREQUAL "Windows")
      MESSAGE (WARNING "Sorry, don't recognize your system ${_plat}. ")
      SET (_plat "unknown")
    ENDIF (_plat STREQUAL "Windows")
    SET (CB_DOWNLOAD_DEPS_PLATFORM ${_plat} CACHE STRING
      "Platform for downloaded dependencies")
    MARK_AS_ADVANCED (CB_DOWNLOAD_DEPS_PLATFORM)
  ENDIF (DEFINED CB_DOWNLOAD_DEPS_PLATFORM)
  SET (${var} ${_plat})
ENDMACRO (_DETERMINE_PLATFORM)


# Returns a simple string describing the current Linux distribution
# compatibility. Possible return values currently include:
# ubuntu14.04, ubuntu12.04, ubuntu10.04, centos5, centos6, debian7, debian8.
MACRO (_DETERMINE_LINUX_DISTRO _distro)
  _LSB_RELEASE (id _id)
  _LSB_RELEASE (release _rel)
  IF (_id STREQUAL "linuxmint")
    # Linux Mint is an Ubuntu derivative; estimate nearest Ubuntu equivalent
    SET (_id "ubuntu")
    IF (_rel VERSION_LESS 13)
      SET (_rel 10.04)
    ELSEIF (_rel VERSION_LESS 17)
      SET (_rel 12.04)
    ELSEIF (_rel VERSION_LESS 18)
      SET (_rel 14.04)
    ELSE ()
      SET (_rel 16.04)
    ENDIF ()
  ELSEIF (_id STREQUAL "debian" OR _id STREQUAL "centos" )
    # Just use the major version from the CentOS/Debian identifier - we don't
    # need different builds for different minor versions.
    STRING (REGEX MATCH "[0-9]+" _rel "${_rel}")
  ELSEIF (_id STREQUAL "fedora" AND _rel VERSION_LESS 26)
    SET (_id "centos")
    SET (_rel "7")
  ELSEIF (_id STREQUAL "opensuse project" OR _id STREQUAL "suse linux")
    SET(_id "suse")
  ENDIF (_id STREQUAL "linuxmint")
  SET (${_distro} "${_id}${_rel}")
ENDMACRO (_DETERMINE_LINUX_DISTRO)

# Returns number of CPUs the system has. The value can be overwritten by the
# CB_CPU_COUNT environment variable. If neither of these work, return some
# (hopefully) reasonable default.
MACRO (_DETERMINE_CPU_COUNT _var)
  SET(_count 0)
  IF (DEFINED $ENV{CB_CPU_COUNT})
    SET(_count $ENV{CB_CPU_COUNT})
  ENDIF (DEFINED $ENV{CB_CPU_COUNT})

  IF (_count EQUAL 0)
    ProcessorCount(_count)
  ENDIF (_count EQUAL 0)

  IF (_count EQUAL 0)
    MESSAGE(WARNING "Couldn't determine number of CPUs to use. Using default.")
    SET(_count 4)
  ENDIF (_count EQUAL 0)

  SET(${_var} ${_count})
ENDMACRO (_DETERMINE_CPU_COUNT)

# Sets _platform to the name of the current platform if it is a supported
# platform, or a False value otherwise.
# "Supported" means that we produce and distribute builds to
# customers on that platform.
# QQQ This list should come from manifest/product-config.json ultimately.
# _platform is in the same format as _DETERMINE_PLATFORM().
MACRO (TEST_FOR_SUPPORTED_PLATFORM _supported_platform)
  SET (${_supported_platform} 0)

  # First get the current platform
  _DETERMINE_PLATFORM(_platform)

  # .. and check it against the list, returning it if found.
  SET (_supported_platforms
       "ubuntu14.04" "ubuntu16.04"
       "windows_msvc2015")
  LIST (FIND _supported_platforms ${_platform} _index)
  IF (_index GREATER "-1")
    SET(${_supported_platform} ${_platform})
  ENDIF (_index GREATER "-1")
ENDMACRO (TEST_FOR_SUPPORTED_PLATFORM)
