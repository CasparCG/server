cmake_minimum_required (VERSION 2.6)
project (common)

set(SOURCES
		compiler/vs/stack_walker.cpp
		diagnostics/graph.cpp
		gl/gl_check.cpp

		base64.cpp
		env.cpp
		except.cpp
		log.cpp
		page_locked_allocator.cpp
		polling_filesystem_monitor.cpp
		prec_timer.cpp
		stdafx.cpp
		tweener.cpp
		utf.cpp
)
set(HEADERS
		compiler/vs/disable_silly_warnings.h
		compiler/vs/stack_walker.h
		diagnostics/graph.h
		gl/gl_check.h
		os/windows/windows.h
		os/windows/system_info.h
		os/windows/windows.h

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
		filesystem_monitor.h
		forward.h
		future.h
		future_fwd.h
		linq.h
		lock.h
		log.h
		memory.h
		memshfl.h
		page_locked_allocator.h
		param.h
		polling_filesystem_monitor.h
		prec_timer.h
		reactive.h
		semaphore.h
		stdafx.h
		tweener.h
		utf.h
)

add_library(common ${SOURCES} ${HEADERS})
add_precompiled_header(common stdafx.h FORCEINCLUDE)

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${RXCPP_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${GLEW_INCLUDE_PATH})

source_group(sources ./*)
source_group(sources\\gl gl/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\compiler\\vs compiler/vs/*)

