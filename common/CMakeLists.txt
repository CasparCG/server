cmake_minimum_required (VERSION 2.6)
project (common)

set(SOURCES
		diagnostics/graph.cpp

		gl/gl_check.cpp

		base64.cpp
		env.cpp
		except.cpp
		filesystem.cpp
		log.cpp
		polling_filesystem_monitor.cpp
		stdafx.cpp
		thread_info.cpp
		tweener.cpp
		utf.cpp
)
if (MSVC)
	set(OS_SPECIFIC_SOURCES
			compiler/vs/disable_silly_warnings.h
			compiler/vs/StackWalker.cpp
			compiler/vs/StackWalker.h

			os/windows/filesystem.cpp
			os/windows/page_locked_allocator.cpp
			os/windows/prec_timer.cpp
			os/windows/threading.cpp
			os/windows/stack_trace.cpp
			os/windows/system_info.cpp
			os/windows/win32_exception.cpp
			os/windows/win32_exception.h
			os/windows/windows.h
	)
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(OS_SPECIFIC_SOURCES
			os/linux/filesystem.cpp
			os/linux/prec_timer.cpp
			os/linux/signal_handlers.cpp
			os/linux/threading.cpp
			os/linux/stack_trace.cpp
			os/linux/system_info.cpp
	)
endif ()
set(HEADERS
		diagnostics/graph.h

		gl/gl_check.h

		os/filesystem.h
		os/general_protection_fault.h
		os/page_locked_allocator.h
		os/threading.h
		os/stack_trace.h
		os/system_info.h

		array.h
		assert.h
		base64.h
		blocking_bounded_queue_adapter.h
		blocking_priority_queue.h
		cache_aligned_vector.h
		endian.h
		enum_class.h
		env.h
		except.h
		executor.h
		filesystem.h
		filesystem_monitor.h
		forward.h
		future.h
		future_fwd.h
		linq.h
		lock.h
		log.h
		memory.h
		memcpy.h
		memset.h
		memshfl.h
		no_init_proxy.h
		param.h
		polling_filesystem_monitor.h
		prec_timer.h
		ptree.h
		scope_exit.h
		semaphore.h
		software_version.h
		stdafx.h
		thread_info.h
		timer.h
		tweener.h
		utf.h
)

add_library(common ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
add_precompiled_header(common stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(${ASMLIB_INCLUDE_PATH})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

source_group(sources ./*)
source_group(sources\\gl gl/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\compiler\\vs compiler/vs/*)
source_group(sources\\os\\windows os/windows/*)
source_group(sources\\os os/*)

target_link_libraries(common
	"${TBB_LIBRARIES}"
	"${GLEW_LIBRARIES}"
	"${OPENGL_gl_LIBRARY}"
	"${FREETYPE_LIBRARIES}"
	"${OPENAL_LIBRARY}"
	"${SFML_LIBRARIES}"
)

if (MSVC)
	target_link_libraries(common
		alibcof64.lib
		jpeg.lib
		sndfile.lib
		Winmm.lib
		Ws2_32.lib
		zlibstat.lib
	)
else ()
	target_link_libraries(common
		"${DEPENDENCIES_FOLDER}/asmlib/alibelf64.a"
		"${Boost_LIBRARIES}"
		X11
		jpeg
		sndfile
		udev
		Xrandr
		pthread
	)
endif ()

if(NOT USE_SYSTEM_TBB)
	if(MSVC)
		casparcg_add_runtime_dependency("${GLEW_BIN_DIR}/glew32.dll")
		casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbb.dll")
		casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbb_debug.dll")

		# Reenable when tbb gets official support for vc14
		# casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbbmalloc.dll")
		# casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbbmalloc_debug.dll")
		# casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbbmalloc_proxy.dll")
		# casparcg_add_runtime_dependency("${TBB_BIN_DIR}/tbbmalloc_proxy_debug.dll")
	else()
		casparcg_add_runtime_dependency("${GLEW_BIN_DIR}/libGLEW.so.1.12")
		casparcg_add_runtime_dependency("${TBB_BIN_DIR}/libtbb.so.2")
		casparcg_add_runtime_dependency("${TBB_BIN_DIR}/libtbbmalloc_proxy.so.2")
		casparcg_add_runtime_dependency("${TBB_BIN_DIR}/libtbbmalloc.so.2")
	endif()
endif()
