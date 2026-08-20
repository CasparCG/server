set(SOURCES
    ultralight.cpp
    ultralight.h
    producer/ultralight_producer.cpp
    producer/ultralight_producer.h
    producer/ultralight_cg_proxy.cpp
    producer/ultralight_cg_proxy.h
)

casparcg_add_module_project(ultralight
    SOURCES ${SOURCES}
    INIT_FUNCTION "ultralight::init"
    UNINIT_FUNCTION "ultralight::uninit"
)

target_include_directories(ultralight PUBLIC
    "${ULTRALIGHT_SDK_PATH}/include"
)

# Use full paths to .so files so the linker finds them when linking
# the final casparcg executable (static lib link dirs don't propagate).
target_link_libraries(ultralight PUBLIC
    "${ULTRALIGHT_SDK_PATH}/bin/libUltralight.so"
    "${ULTRALIGHT_SDK_PATH}/bin/libUltralightCore.so"
    "${ULTRALIGHT_SDK_PATH}/bin/libWebCore.so"
    "${ULTRALIGHT_SDK_PATH}/bin/libAppCore.so"
)
