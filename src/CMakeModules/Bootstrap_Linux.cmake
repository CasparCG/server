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
set(USE_SYSTEM_CEF ON CACHE BOOL "Use the version of cef from your OS (only tested with Ubuntu)")
set(CASPARCG_BINARY_NAME "casparcg" CACHE STRING "Custom name of the binary to build (this disables some install files)")
set(ENABLE_AVX2 OFF CACHE BOOL "Enable the AVX2 instruction set (requires a CPU that supports it)")
set(ENABLE_VULKAN OFF CACHE BOOL "Enable Vulkan support")

# Determine build (target) platform
SET (PLATFORM_FOLDER_NAME "linux")

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
find_package(FFmpeg REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX EGL)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenAL REQUIRED)
find_package(SFML 3 COMPONENTS Graphics System Window QUIET)
if(NOT SFML_FOUND)
    find_package(SFML 2 COMPONENTS graphics system window REQUIRED)
endif()

IF (ENABLE_VULKAN)
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
endif()


find_package(X11 REQUIRED)

if (ENABLE_HTML)
    if (USE_SYSTEM_CEF)
        set(CEF_LIB_PATH "/usr/lib/casparcg-cef-142")

        add_library(CEF::CEF INTERFACE IMPORTED)
        target_include_directories(CEF::CEF INTERFACE
            "/usr/include/casparcg-cef-142"
        )
        target_link_libraries(CEF::CEF INTERFACE
            "-Wl,-rpath,${CEF_LIB_PATH} ${CEF_LIB_PATH}/libcef.so"
            "${CEF_LIB_PATH}/libcef_dll_wrapper.a"
        )
    else()
        casparcg_add_external_project(cef)
        ExternalProject_Add(cef
            URL ${CASPARCG_DOWNLOAD_MIRROR}/cef/cef_binary_142.0.17+g60aac24+chromium-142.0.7444.176_linux64_minimal.tar.bz2
            URL_HASH SHA256=1d89e19b2f446105f9a1fe6fdc96bced86249b5884241dcc4013b7c94dabf424
            DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
            CMAKE_ARGS -DUSE_SANDBOX=Off
            INSTALL_COMMAND ""
            BUILD_BYPRODUCTS
                "<SOURCE_DIR>/Release/libcef.so"
                "<BINARY_DIR>/libcef_dll_wrapper/libcef_dll_wrapper.a"
        )
        # CEF needs the resources next to libcef.so
        ExternalProject_Add_Step(cef copy_resources
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "<SOURCE_DIR>/Resources"
                "<SOURCE_DIR>/Release"
                DEPENDEES build
                DEPENDERS install
        )
        ExternalProject_Get_Property(cef SOURCE_DIR)
        ExternalProject_Get_Property(cef BINARY_DIR)

        add_library(CEF::CEF INTERFACE IMPORTED)
        target_include_directories(CEF::CEF INTERFACE
            "${SOURCE_DIR}"
        )
        target_link_libraries(CEF::CEF INTERFACE
            # Note: All of these must be referenced in the BUILD_BYPRODUCTS above, to satisfy ninja
            "${SOURCE_DIR}/Release/libcef.so"
            "${BINARY_DIR}/libcef_dll_wrapper/libcef_dll_wrapper.a"
        )

        install(DIRECTORY ${SOURCE_DIR}/Resources/locales TYPE LIB)
        install(FILES ${SOURCE_DIR}/Resources/chrome_100_percent.pak TYPE LIB)
        install(FILES ${SOURCE_DIR}/Resources/chrome_200_percent.pak TYPE LIB)
        install(FILES ${SOURCE_DIR}/Resources/icudtl.dat TYPE LIB)
        install(FILES ${SOURCE_DIR}/Resources/resources.pak TYPE LIB)

        install(FILES ${SOURCE_DIR}/Release/chrome-sandbox TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/libcef.so TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/libEGL.so TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/libGLESv2.so TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/libvk_swiftshader.so TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/libvulkan.so.1 TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/v8_context_snapshot.bin TYPE LIB)
        install(FILES ${SOURCE_DIR}/Release/vk_swiftshader_icd.json TYPE LIB)
    endif()
endif ()

# OMT (Open Media Transport) - source build for the omt module's libomt.so/libvmx.so.
# Upstream publishes no prebuilt Linux binary, so this builds both from source instead.
#
# Opt-in and OFF by default: requires the .NET 8 SDK and clang, beyond what CasparCG's
# own build otherwise needs. The omt module still works without this if libomt.so is
# installed some other way.
option(OMT_BUILD_FROM_SOURCE "Build the Open Media Transport runtime (libomt, libvmx) from source and bundle it. Requires the .NET 8 SDK ('dotnet') and clang, in addition to CasparCG's normal build tools." OFF)

if (OMT_BUILD_FROM_SOURCE)
    find_program(OMT_DOTNET_EXECUTABLE dotnet)
    if (NOT OMT_DOTNET_EXECUTABLE)
        message(FATAL_ERROR "OMT_BUILD_FROM_SOURCE requires the .NET 8 SDK ('dotnet' not found in PATH) - "
                             "install it from https://dotnet.microsoft.com/download/dotnet/8.0, or configure "
                             "with -DOMT_BUILD_FROM_SOURCE=OFF.")
    endif()

    # libvmx: the video codec libomt depends on. See omt_build_libvmx_linux.sh for how it
    # picks the right build script for the host architecture (x86_64 or aarch64).
    #
    # PATCH_COMMAND works around a Clang narrowing warning that upstream's code triggers
    # as a hard error (GCC/MSVC only warn). Patches both build scripts, since the wrapper
    # decides at build time which one actually runs.
    casparcg_add_external_project(libvmx-src)
    ExternalProject_Add(libvmx-src
        GIT_REPOSITORY https://github.com/openmediatransport/libvmx.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        PATCH_COMMAND sh -c "sed -i 's/ -shared/ -Wno-c++11-narrowing -shared/' <SOURCE_DIR>/build/buildlinuxx64.sh <SOURCE_DIR>/build/buildlinuxarm64.sh"
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/omt_build_libvmx_linux.sh
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "<SOURCE_DIR>/build/libvmx.so"
    )
    ExternalProject_Get_Property(libvmx-src SOURCE_DIR)
    set(OMT_LIBVMX_SO "${SOURCE_DIR}/build/libvmx.so")

    # libomtnet: libomt.csproj expects a prebuilt libomtnet.dll next to it, at
    # ../libomtnet/bin/Release/netstandard2.0/libomtnet.dll. Both are checked out under
    # the same parent directory so that path resolves correctly.
    set(OMT_NET_SRC_ROOT "${CMAKE_CURRENT_BINARY_DIR}/omt-src")

    casparcg_add_external_project(libomtnet)
    ExternalProject_Add(libomtnet
        GIT_REPOSITORY https://github.com/openmediatransport/libomtnet.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        SOURCE_DIR "${OMT_NET_SRC_ROOT}/libomtnet"
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND sh -c "cd build && dotnet build ../libomtnet.sln -c Release"
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${OMT_NET_SRC_ROOT}/libomtnet/bin/Release/netstandard2.0/libomtnet.dll"
    )

    # libomt: see omt_build_libomt_linux.sh for why this needs a wrapper rather than a direct
    # BUILD_COMMAND - dotnet publish's exact output path isn't fixed the way libvmx's is.
    casparcg_add_external_project(libomt-src)
    ExternalProject_Add(libomt-src
        GIT_REPOSITORY https://github.com/openmediatransport/libomt.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        SOURCE_DIR "${OMT_NET_SRC_ROOT}/libomt"
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE TRUE
        BUILD_COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/omt_build_libomt_linux.sh
        INSTALL_COMMAND ""
        DEPENDS libvmx-src libomtnet
        BUILD_BYPRODUCTS "${OMT_NET_SRC_ROOT}/libomt/libomt.so"
    )
    set(OMT_LIBOMT_SO "${OMT_NET_SRC_ROOT}/libomt/libomt.so")

    # Install to the right lib directory: lib64 on most 64-bit Linux, plain lib on
    # Debian/Ubuntu. Needed for dlopen() to find these at runtime, not just for
    # packaging. -DOMT_INSTALL_LIBDIR overrides this.
    set(OMT_INSTALL_LIBDIR "" CACHE STRING "Install directory for the OMT runtime libraries (libomt.so, libvmx.so), relative to the install prefix. Empty = auto-detect.")
    if (NOT OMT_INSTALL_LIBDIR)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT EXISTS "/etc/debian_version")
            set(OMT_INSTALL_LIBDIR "lib64")
        elseif (CMAKE_INSTALL_LIBDIR)
            set(OMT_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
        else()
            set(OMT_INSTALL_LIBDIR "lib")
        endif()
    endif()
    install(FILES "${OMT_LIBVMX_SO}" DESTINATION "${OMT_INSTALL_LIBDIR}")
    install(FILES "${OMT_LIBOMT_SO}" DESTINATION "${OMT_INSTALL_LIBDIR}")
endif()

SET (BOOST_INCLUDE_PATH "${Boost_INCLUDE_DIRS}")
SET (FFMPEG_INCLUDE_PATH "${FFMPEG_INCLUDE_DIRS}")

LINK_DIRECTORIES("${FFMPEG_LIBRARY_DIRS}")

SET_PROPERTY (GLOBAL PROPERTY USE_FOLDERS ON)

ADD_DEFINITIONS (-DSFML_STATIC)
ADD_DEFINITIONS (-DUNICODE)
ADD_DEFINITIONS (-D_UNICODE)
ADD_DEFINITIONS (-DGLEW_NO_GLU)
ADD_DEFINITIONS (-DGLEW_EGL)
ADD_DEFINITIONS (-D__NO_INLINE__) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_NO_SWPRINTF) # swprintf on Linux seems to always use , as decimal point regardless of C-locale or C++-locale
ADD_DEFINITIONS (-DTBB_USE_CAPTURED_EXCEPTION=1)
ADD_DEFINITIONS (-DNDEBUG) # Needed for precompiled headers to work
ADD_DEFINITIONS (-DBOOST_LOCALE_HIDE_AUTO_PTR) # Needed for C++17 in boost 1.67+


if (NOT USE_STATIC_BOOST)
	ADD_DEFINITIONS (-DBOOST_ALL_DYN_LINK)
endif()

IF (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	ADD_COMPILE_OPTIONS (-O3) # Needed for precompiled headers to work
endif()
IF (CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x64|x86_64|amd64|e2k)")
    ADD_COMPILE_OPTIONS (-msse3)
    ADD_COMPILE_OPTIONS (-mssse3)
    ADD_COMPILE_OPTIONS (-msse4.1)
    IF (ENABLE_AVX2)
        ADD_COMPILE_OPTIONS (-mfma)
        ADD_COMPILE_OPTIONS (-mavx)
        ADD_COMPILE_OPTIONS (-mavx2)
    ENDIF ()
ELSE ()
    ADD_COMPILE_DEFINITIONS (USE_SIMDE) # Enable OpenMP support in simde
    ADD_COMPILE_DEFINITIONS (SIMDE_ENABLE_OPENMP) # Enable OpenMP support in simde
    ADD_COMPILE_OPTIONS (-fopenmp-simd) # Enable OpenMP SIMD support
ENDIF ()

ADD_COMPILE_OPTIONS (-fnon-call-exceptions) # Allow signal handler to throw exception

ADD_COMPILE_OPTIONS (-Wno-deprecated-declarations -Wno-write-strings -Wno-multichar -Wno-cpp -Werror)

IF (ENABLE_VULKAN)
    ADD_COMPILE_OPTIONS (-Wno-nonnull -Wno-nullability-completeness)
ENDIF()

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
