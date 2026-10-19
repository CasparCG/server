include(CheckCXXSourceCompiles)

message(STATUS "Dependency check compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Dependency check linker: ${CMAKE_LINKER}")
execute_process(COMMAND "${CMAKE_LINKER}" --version COMMAND_ERROR_IS_FATAL ANY)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS} ${FFMPEG_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES
    ${Boost_LIBRARIES}
    ${FFMPEG_LIBRARIES}
    OpenGL::GL OpenGL::OpenGL OpenGL::EGL GLEW::glew
    TBB::tbb TBB::tbbmalloc OpenAL::OpenAL
    sfml-graphics ${X11_LIBRARIES}
    dl icui18n icuuc z pthread
)
# Retain every shared library in the probe and check its transitive symbol resolution.
set(CMAKE_REQUIRED_LINK_OPTIONS -Wl,--no-as-needed -Wl,--no-allow-shlib-undefined)
if(ENABLE_HTML)
    if(USE_SYSTEM_CEF)
        list(APPEND CMAKE_REQUIRED_LIBRARIES CEF::CEF)
    else()
        message(STATUS "Bundled CEF linkage is checked only by a full build, not by this dependency probe")
    endif()
endif()

# Recheck on every invocation so package upgrades cannot reuse a previous success.
unset(CASPARCG_SYSTEM_DEPENDENCIES_LINK CACHE)
check_cxx_source_compiles([=[
    #include <boost/version.hpp>
    #include <span>
    extern "C" {
    #include <libavcodec/avcodec.h>
    }
    static_assert(BOOST_VERSION >= 108300);
    int main() {
        int values[] = {1, 2};
        return std::span(values).size() == 2 && avcodec_version() != 0 ? 0 : 1;
    }
]=] CASPARCG_SYSTEM_DEPENDENCIES_LINK)
if(NOT CASPARCG_SYSTEM_DEPENDENCIES_LINK)
    message(FATAL_ERROR "System dependency compile/link probe failed; see CMakeFiles/CMakeConfigureLog.yaml")
endif()
message(STATUS "System dependency compile/link probe passed; CasparCG itself has not been compiled")
