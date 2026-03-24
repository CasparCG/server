# CEF_Linux.cmake — CEF configuration for Linux builds.
# Included by Bootstrap_Linux.cmake inside the if(ENABLE_HTML) guard.

# --- Distro detection & default cache variable values ---

if(EXISTS "/etc/os-release")
    file(READ "/etc/os-release" _os_release)
    if(_os_release MATCHES "(^|\n)ID=fedora(\n|$)")
        set(_CEF_ON_FEDORA TRUE)
    endif()
    unset(_os_release)
endif()

if(_CEF_ON_FEDORA)
    set(_CEF_SYSTEM_PACKAGE_DEFAULT "")
    set(_CEF_USE_FIND_PACKAGE_DEFAULT ON)
else()
    set(_CEF_SYSTEM_PACKAGE_DEFAULT "casparcg-cef-142")
    set(_CEF_USE_FIND_PACKAGE_DEFAULT OFF)
endif()

option(CEF_USE_FIND_PACKAGE
    "Use find_package(CEF) to locate system CEF. Requires a distro-provided FindCEF.cmake (e.g. Fedora cef-devel). Takes precedence over CEF_SYSTEM_PACKAGE."
    ${_CEF_USE_FIND_PACKAGE_DEFAULT})
set(CEF_SYSTEM_PACKAGE "${_CEF_SYSTEM_PACKAGE_DEFAULT}" CACHE STRING
    "Name of the CasparCG-packaged system CEF package (drives include/lib paths under /usr/include/ and /usr/lib/). Set to empty string to download CEF automatically. Unused when CEF_USE_FIND_PACKAGE is ON.")

# --- CEF target setup ---

if(CEF_USE_FIND_PACKAGE)
    # Use distro-provided FindCEF.cmake (e.g. Fedora cef-devel).
    # That module builds libcef_dll_wrapper via add_subdirectory and exposes:
    #   CEF::Library  — imported libcef.so
    #   CEF::Wrapper  — built libcef_dll_wrapper (CEF_API_VERSION applied publicly)
    # Set CEF_API_VERSION before find_package so FindCEF picks it up.
    if(CEF_STABLE_API_VERSION)
        set(CEF_API_VERSION "${CEF_STABLE_API_VERSION}" CACHE STRING "CEF API version" FORCE)
    endif()
    find_package(CEF REQUIRED)

    # Derive rpath from the actual installed library location
    get_target_property(_cef_lib_location CEF::Library IMPORTED_LOCATION)
    get_filename_component(_cef_lib_dir "${_cef_lib_location}" DIRECTORY)

    add_library(CEF::CEF INTERFACE IMPORTED)
    target_link_libraries(CEF::CEF INTERFACE
        CEF::Wrapper
        CEF::Library
        "-Wl,-rpath,${_cef_lib_dir}"
    )
    # Fedora's CEF headers (145+) use C++20 features (concepts, <=>). Propagate
    # the requirement so any target linking CEF::CEF is compiled with C++20.
    target_compile_features(CEF::CEF INTERFACE cxx_std_20)
    unset(_cef_lib_location)
    unset(_cef_lib_dir)

elseif(CEF_SYSTEM_PACKAGE)
    # CasparCG-packaged system CEF (Ubuntu/Debian, e.g. casparcg-cef-142)
    set(_cef_pkg_lib_path "/usr/lib/${CEF_SYSTEM_PACKAGE}")
    add_library(CEF::CEF INTERFACE IMPORTED)
    target_include_directories(CEF::CEF INTERFACE "/usr/include/${CEF_SYSTEM_PACKAGE}")
    target_link_libraries(CEF::CEF INTERFACE
        "-Wl,-rpath,${_cef_pkg_lib_path} ${_cef_pkg_lib_path}/libcef.so"
        "${_cef_pkg_lib_path}/libcef_dll_wrapper.a"
    )

else()
    # Auto-download CEF
    casparcg_add_external_project(cef)
    ExternalProject_Add(cef
        URL ${CASPARCG_DOWNLOAD_MIRROR}/cef/cef_binary_142.0.17+g60aac24+chromium-142.0.7444.176_linux64_minimal.tar.bz2
        URL_HASH SHA256=1d89e19b2f446105f9a1fe6fdc96bced86249b5884241dcc4013b7c94dabf424
        DOWNLOAD_DIR ${CASPARCG_DOWNLOAD_CACHE}
        CMAKE_ARGS -DUSE_SANDBOX=Off $<$<BOOL:${CEF_STABLE_API_VERSION}>:-Dapi_version=${CEF_STABLE_API_VERSION}>
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS
            "<SOURCE_DIR>/Release/libcef.so"
            "<BINARY_DIR>/libcef_dll_wrapper/libcef_dll_wrapper.a"
    )
    ExternalProject_Get_Property(cef SOURCE_DIR)
    ExternalProject_Get_Property(cef BINARY_DIR)

    add_library(CEF::CEF INTERFACE IMPORTED)
    target_include_directories(CEF::CEF INTERFACE "${SOURCE_DIR}")
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

# --- CEF API version (compile definition) ---
#
# For the find_package path, FindCEF.cmake already applies CEF_API_VERSION as a
# PUBLIC compile option on CEF::Wrapper (which CEF::CEF links to), so nothing
# more is needed here.
#
# For the download path, CEF_STABLE_API_VERSION is forwarded as -Dapi_version to
# the dll_wrapper build, so the compiled wrapper and the compile definition below
# always agree.
#
# For the CEF_SYSTEM_PACKAGE (Ubuntu/Debian) path, the packager controls both the
# pre-built dll_wrapper and the default value of CEF_STABLE_API_VERSION, so they
# also always agree.

if(CEF_STABLE_API_VERSION AND NOT CEF_USE_FIND_PACKAGE)
    target_compile_definitions(CEF::CEF INTERFACE CEF_API_VERSION=${CEF_STABLE_API_VERSION})
endif()
