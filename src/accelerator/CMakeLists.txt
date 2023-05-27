cmake_minimum_required (VERSION 3.16)
project (accelerator)

set(SOURCES
	ogl/image/image_kernel.cpp
	ogl/image/image_mixer.cpp
	ogl/image/image_shader.cpp

	ogl/util/buffer.cpp
	ogl/util/device.cpp
	ogl/util/shader.cpp
	ogl/util/texture.cpp

	accelerator.cpp
)
if (MSVC)
	set(WIN32_SPECIFIC_SOURCES
        d3d/d3d_device.cpp
		d3d/d3d_device_context.cpp
		d3d/d3d_texture2d.cpp
	)
endif ()
set(HEADERS
	ogl/image/image_kernel.h
	ogl/image/image_mixer.h
	ogl/image/image_shader.h

	ogl/util/buffer.h
	ogl/util/device.h
	ogl/util/shader.h
	ogl/util/texture.h

	ogl_image_vertex.h
	ogl_image_fragment.h

	accelerator.h
)
if (MSVC)
	set(WIN32_SPECIFIC_HEADERS
        d3d/d3d_device.h
		d3d/d3d_device_context.h
		d3d/d3d_texture2d.h
	)
endif ()

bin2c("ogl/image/shader.vert" "ogl_image_vertex.h" "caspar::accelerator::ogl" "vertex_shader")
bin2c("ogl/image/shader.frag" "ogl_image_fragment.h" "caspar::accelerator::ogl" "fragment_shader")

add_library(accelerator ${SOURCES} ${HEADERS} ${WIN32_SPECIFIC_SOURCES} ${WIN32_SPECIFIC_HEADERS})
target_compile_features(accelerator PRIVATE cxx_std_14)
target_include_directories(accelerator PRIVATE
    ..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${GLEW_INCLUDE_PATH}
    ${FFMPEG_INCLUDE_PATH}
    ${SFML_INCLUDE_PATH}
    )
target_precompile_headers(accelerator PRIVATE
    "../common/compiler/vs/disable_silly_warnings.h"
    <GL/glew.h>
    <SFML/Window/Context.hpp>
    <algorithm>
    <array>
    <boost/algorithm/cxx11/all_of.hpp>
    <boost/asio/deadline_timer.hpp>
    <boost/asio/dispatch.hpp>
    <boost/asio/io_context.hpp>
    <boost/asio/io_service.hpp>
    <boost/asio/post.hpp>
    <boost/asio/spawn.hpp>
    <boost/property_tree/ptree_fwd.hpp>
    <boost/range/adaptor/transformed.hpp>
    <boost/range/algorithm_ext/erase.hpp>
    <cmath>
    "../common/array.h"
    "../common/assert.h"
    "../common/env.h"
    "../common/except.h"
    "../common/forward.h"
    "../common/future.h"
    "../common/gl/gl_check.h"
    "../common/memory.h"
    "../common/scope_exit.h"
    "../core/frame/frame.h"
    "../core/frame/frame_transform.h"
    "../core/frame/frame_visitor.h"
    "../core/frame/geometry.h"
    "../core/frame/pixel_format.h"
    "../core/fwd.h"
    "../core/mixer/image/blend_modes.h"
    "../core/mixer/image/image_mixer.h"
    "../core/video_format.h"
    <cstddef>
    <cstdint>
    <future>
    <memory>
    <mutex>
    <set>
    <string>
    <tbb/concurrent_queue.h>
    <tbb/concurrent_unordered_map.h>
    <thread>
    <type_traits>
    <unordered_map>
    <vector>

    )


source_group(sources ./.*)
source_group(sources\\cpu\\image cpu/image/.*)
source_group(sources\\cpu\\util cpu/util/.*)
source_group(sources\\ogl\\image ogl/image/.*)
source_group(sources\\ogl\\util ogl/util/.*)
source_group(sources\\d3d d3d/.*)

target_link_libraries(accelerator common core)
