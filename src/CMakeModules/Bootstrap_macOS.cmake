cmake_minimum_required (VERSION 3.28)

# macOS (Apple Silicon) bootstrap.
#
# Dependencies are expected from Homebrew:
#   brew install cmake ninja pkg-config boost ffmpeg tbb sfml glew openal-soft \
#                molten-vk vulkan-headers vulkan-loader shaderc simde
#
# macOS only provides OpenGL 4.1, while the CasparCG OpenGL backend needs 4.5 (and EGL),
# so on macOS the Vulkan backend (running on Metal through MoltenVK) is the only mixer.

include(ExternalProject)
include(FetchContent)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(ENABLE_HTML OFF CACHE BOOL "Enable CEF and HTML producer (not yet supported on macOS)" FORCE)
set(ENABLE_VULKAN ON CACHE BOOL "Enable Vulkan support" FORCE)
set(ENABLE_OPENGL OFF CACHE BOOL "Enable the OpenGL mixer backend (requires OpenGL 4.5, unavailable on macOS)" FORCE)
set(USE_STATIC_BOOST OFF CACHE BOOL "Use static Boost libraries")
set(CASPARCG_BINARY_NAME "casparcg" CACHE STRING "Custom name of the binary to build")

SET (PLATFORM_FOLDER_NAME "macos")

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    MESSAGE (STATUS "Setting build type to 'Release' as none was specified.")
    SET (CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
ENDIF ()

# Homebrew prefix (/opt/homebrew on Apple Silicon)
if (NOT HOMEBREW_PREFIX)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if (NOT HOMEBREW_PREFIX)
        set(HOMEBREW_PREFIX "/opt/homebrew")
    endif()
endif()
list(APPEND CMAKE_PREFIX_PATH
    "${HOMEBREW_PREFIX}"
    "${HOMEBREW_PREFIX}/opt/openal-soft"
    "${HOMEBREW_PREFIX}/opt/icu4c"
)
set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/lib/pkgconfig:${HOMEBREW_PREFIX}/opt/openal-soft/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

if (USE_STATIC_BOOST)
    SET (Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost 1.83.0 COMPONENTS thread filesystem log_setup log locale regex date_time coroutine REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenAL CONFIG REQUIRED)
find_package(SFML 3 COMPONENTS Graphics System Window REQUIRED)
find_package(ICU COMPONENTS i18n uc REQUIRED)

find_package(Vulkan REQUIRED)
FetchContent_Declare(vk_bootstrap
        URL ${CASPARCG_DOWNLOAD_MIRROR}/vk-bootstrap/vk-bootstrap-1.4.328.tar.gz
        URL_HASH SHA256=3be0220de218dc3e692aeac552b2953860a0e0a48257f4a61c3f1c1472674744
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        )
FetchContent_MakeAvailable(vk_bootstrap)

FetchContent_Declare(vma
        URL ${CASPARCG_DOWNLOAD_MIRROR}/VulkanMemoryAllocator/VulkanMemoryAllocator-3.3.0.tar.gz
        URL_HASH SHA256=c4f6bbe6b5a45c2eb610ca9d231158e313086d5b1a40c9922cb42b597419b14e
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
)
FetchContent_MakeAvailable(vma)

SET (BOOST_INCLUDE_PATH "${Boost_INCLUDE_DIRS}")
SET (FFMPEG_INCLUDE_PATH "${FFMPEG_INCLUDE_DIRS}")

LINK_DIRECTORIES("${FFMPEG_LIBRARY_DIRS}" "${HOMEBREW_PREFIX}/lib")
include_directories(SYSTEM "${HOMEBREW_PREFIX}/include")

SET_PROPERTY (GLOBAL PROPERTY USE_FOLDERS ON)

ADD_DEFINITIONS (-DUNICODE)
ADD_DEFINITIONS (-D_UNICODE)
ADD_DEFINITIONS (-DGLEW_NO_GLU)
ADD_DEFINITIONS (-DBOOST_NO_SWPRINTF)
ADD_DEFINITIONS (-DTBB_USE_CAPTURED_EXCEPTION=1)
ADD_DEFINITIONS (-DNDEBUG)
ADD_DEFINITIONS (-DBOOST_LOCALE_HIDE_AUTO_PTR)
ADD_DEFINITIONS (-DGL_SILENCE_DEPRECATION)
ADD_DEFINITIONS (-DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)

if (NOT USE_STATIC_BOOST)
    ADD_DEFINITIONS (-DBOOST_ALL_DYN_LINK)
endif()

IF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    ADD_COMPILE_OPTIONS (-O3)
endif()

# Apple Silicon: no SSE, use SIMDe to translate the SSE intrinsics to NEON
ADD_COMPILE_DEFINITIONS (USE_SIMDE)
ADD_COMPILE_DEFINITIONS (SIMDE_ENABLE_OPENMP)
ADD_COMPILE_OPTIONS (-fopenmp-simd)

ADD_COMPILE_OPTIONS (-Wno-deprecated-declarations -Wno-write-strings -Wno-multichar -Wno-cpp)
ADD_COMPILE_OPTIONS (-Wno-nonnull -Wno-nullability-completeness)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

# Find the Homebrew Vulkan loader + MoltenVK ICD at runtime
set(CMAKE_BUILD_RPATH "${HOMEBREW_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH "@executable_path/lib;${HOMEBREW_PREFIX}/lib")
