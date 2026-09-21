cmake_minimum_required(VERSION 3.28)
project(omt)

set(SOURCES
    omt.cpp
    omt.h

    consumer/omt_consumer.cpp
    consumer/omt_consumer.h

    producer/omt_producer.cpp
    producer/omt_producer.h

    util/omt_util.cpp
    util/omt_util.h

    interop/libomt.h
)

casparcg_add_module_project(omt
    SOURCES ${SOURCES}
    INIT_FUNCTION "omt::init"
)

target_include_directories(omt PRIVATE
    ..
    ${FFMPEG_INCLUDE_PATH}
)

target_link_libraries(omt PRIVATE
    core
    common
    ffmpeg
)

set_target_properties(omt PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources\\interop interop/*)
source_group(sources ./*)

# libomt/libvmx are loaded dynamically at runtime (see util/omt_util.cpp), so this just
# copies the prebuilt DLLs next to the server executable if OMT_SDK_DIR is set.
if (MSVC AND DEFINED OMT_SDK_DIR)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
        set(OMT_LIB_DIR "${OMT_SDK_DIR}/Libraries/Winarm64")
    else()
        set(OMT_LIB_DIR "${OMT_SDK_DIR}/Libraries/Winx64")
    endif()

    # Skip the EXISTS check if we fetched the SDK ourselves, since its files only
    # appear once the build step runs, after this configure-time check would run.
    if (OMT_SDK_FETCHED OR EXISTS "${OMT_LIB_DIR}/libomt.dll")
        casparcg_add_runtime_dependency("${OMT_LIB_DIR}/libomt.dll")
    endif()
    if (OMT_SDK_FETCHED OR EXISTS "${OMT_LIB_DIR}/libvmx.dll")
        casparcg_add_runtime_dependency("${OMT_LIB_DIR}/libvmx.dll")
    endif()
endif()
