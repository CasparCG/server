cmake_minimum_required (VERSION 3.28)

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
set(CASPARCG_BINARY_NAME "casparcg" CACHE STRING "Custom name of the binary to build (this disables some install files)")
set(ENABLE_AVX2 OFF CACHE BOOL "Enable the AVX2 instruction set (requires a CPU that supports it)")
set(ENABLE_VULKAN ON BOOL "Enable Vulkan support (required for macOS build, requires Vulkan SDK to be installed)")

# Determine build (target) platform
SET (PLATFORM_FOLDER_NAME "Mac")

IF (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	MESSAGE (STATUS "Setting build type to 'Release' as none was specified.")
	SET (CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
	SET_PROPERTY (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
ENDIF ()
MARK_AS_ADVANCED (CMAKE_INSTALL_PREFIX)

if (USE_STATIC_BOOST)
	SET (Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost 1.83.0 COMPONENTS thread filesystem log_setup log locale regex date_time coroutine REQUIRED)
find_package(TBB REQUIRED)
find_package(zstd REQUIRED)
find_package(OpenAL REQUIRED PATHS /opt/homebrew/opt/openal-soft)
find_package(SFML 2 COMPONENTS graphics window REQUIRED PATHS /opt/homebrew/opt/sfml@2)
SET (CMAKE_PREFIX_PATH /opt/homebrew/opt/ffmpeg@7 ${CMAKE_PREFIX_PATH})
find_package(FFmpeg REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED) # window/swapchain for the Vulkan screen consumer

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

if (NOT TARGET OpenAL::OpenAL)
    add_library(OpenAL::OpenAL INTERFACE IMPORTED)
    target_include_directories(OpenAL::OpenAL INTERFACE ${OPENAL_INCLUDE_DIR})
    target_link_libraries(OpenAL::OpenAL INTERFACE ${OPENAL_LIBRARY})
endif()

if (ENABLE_HTML)
    casparcg_add_external_project(cef)
    ExternalProject_Add(cef
        URL https://casparcgvulkan.com/cef_binary_131.4.1+g437feba+chromium-131.0.6778.265_macosarm64_minimal.tar.bz2
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        CMAKE_ARGS -DUSE_SANDBOX=Off ${EXTERNAL_CMAKE_ARGS}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "<SOURCE_DIR>/Release/Chromium Embedded Framework.framework/Chromium Embedded Framework"
            "<BINARY_DIR>/libcef_dll_wrapper/libcef_dll_wrapper.a"
    )
    
    ExternalProject_Get_Property(cef SOURCE_DIR)
    ExternalProject_Get_Property(cef BINARY_DIR)

    set(CEF_ROOT "${SOURCE_DIR}" CACHE INTERNAL "CEF root directory")

    # Create the CEF::CEF interface target directly (no find_package needed,
    # since CEF is downloaded at build time via ExternalProject)
    add_library(CEF::CEF INTERFACE IMPORTED)
    add_dependencies(CEF::CEF cef)
    target_include_directories(CEF::CEF INTERFACE
        "${SOURCE_DIR}"
    )

    target_link_libraries(CEF::CEF INTERFACE
        # Note: All of these must be referenced in the BUILD_BYPRODUCTS above, to satisfy ninja
        "${SOURCE_DIR}/Release/Chromium Embedded Framework.framework/Chromium Embedded Framework"
        "${BINARY_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.a"
        -lpthread
        "-framework AppKit"
        "-framework Cocoa"
        "-framework IOSurface"
    )

    target_compile_definitions(CEF::CEF INTERFACE
        __STDC_CONSTANT_MACROS
        __STDC_FORMAT_MACROS
    )
    
endif ()

SET (BOOST_INCLUDE_PATH "${Boost_INCLUDE_DIRS}")
SET (FFMPEG_INCLUDE_PATH "${FFMPEG_INCLUDE_DIRS}")
SET (SFML_INCLUDE_PATH "${SFML_INCLUDE_DIRS}")

LINK_DIRECTORIES("${FFMPEG_LIBRARY_DIRS}")
LINK_DIRECTORIES("${Boost_LIBRARY_DIRS}")

SET_PROPERTY (GLOBAL PROPERTY USE_FOLDERS ON)

ADD_DEFINITIONS (-DSFML_STATIC)
ADD_DEFINITIONS (-DUNICODE)
ADD_DEFINITIONS (-D_UNICODE)
ADD_DEFINITIONS (-D__NO_INLINE__) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_NO_SWPRINTF) # swprintf on Linux seems to always use , as decimal point regardless of C-locale or C++-locale
ADD_DEFINITIONS (-DTBB_USE_CAPTURED_EXCEPTION=1)
ADD_DEFINITIONS (-DNDEBUG) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_LOCALE_HIDE_AUTO_PTR) # Needed for C++17 in boost 1.67+
ADD_DEFINITIONS (-DNO_OGL) 


if (NOT USE_STATIC_BOOST)
	ADD_DEFINITIONS (-DBOOST_ALL_DYN_LINK)
endif()

IF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	ADD_COMPILE_OPTIONS (-O3) # Needed for precompiled headers to work
endif()

ADD_COMPILE_DEFINITIONS (_GNU_SOURCE)
ADD_COMPILE_DEFINITIONS (USE_SIMDE) 
# ADD_COMPILE_DEFINITIONS (SIMDE_ENABLE_OPENMP) # Enable OpenMP support in simde
# ADD_COMPILE_OPTIONS (-fopenmp-simd) # Enable OpenMP SIMD support
ADD_COMPILE_OPTIONS (-fnon-call-exceptions) # Allow signal handler to throw exception

ADD_COMPILE_OPTIONS (-Wno-deprecated-declarations -Wno-write-strings -Wno-multichar -Wno-cpp -Werror -Wno-nonnull -Wno-nullability-completeness)
IF (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    ADD_COMPILE_OPTIONS (-Wno-terminate)
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Help TBB figure out what compiler support for c++11 features
    # https://github.com/01org/tbb/issues/22
    string(REPLACE "." "0" TBB_USE_GLIBCXX_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    message(STATUS "ADDING: -DTBB_USE_GLIBCXX_VERSION=${TBB_USE_GLIBCXX_VERSION}")
    add_definitions(-DTBB_USE_GLIBCXX_VERSION=${TBB_USE_GLIBCXX_VERSION})
ENDIF ()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
