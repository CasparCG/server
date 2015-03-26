cmake_minimum_required (VERSION 2.6)
project (common)

set(SOURCES
		diagnostics/graph.cpp

		gl/gl_check.cpp

		base64.cpp
		env.cpp
		log.cpp
		polling_filesystem_monitor.cpp
		stdafx.cpp
		tweener.cpp
		utf.cpp
)
if (MSVC)
	set(OS_SPECIFIC_SOURCES
			compiler/vs/disable_silly_warnings.h
			compiler/vs/stack_walker.cpp
			compiler/vs/stack_walker.h

			os/windows/current_version.h
			os/windows/page_locked_allocator.cpp
			os/windows/prec_timer.cpp
			os/windows/scheduling.cpp
			os/windows/stack_trace.cpp
			os/windows/system_info.cpp
			os/windows/win32_exception.cpp
			os/windows/win32_exception.h
			os/windows/windows.h
	)
elseif (CMAKE_COMPILER_IS_GNUCXX)
	set(OS_SPECIFIC_SOURCES
			os/linux/signal_handlers.cpp
			os/linux/scheduling.cpp
			os/linux/stack_trace.cpp
			os/linux/system_info.cpp
	)
endif ()
set(HEADERS
		diagnostics/graph.h

		gl/gl_check.h

		os/general_protection_fault.h
		os/page_locked_allocator.h
		os/scheduling.h
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
		filesystem_monitor.h
		forward.h
		future.h
		future_fwd.h
		linq.h
		lock.h
		log.h
		memory.h
		memshfl.h
		param.h
		polling_filesystem_monitor.h
		prec_timer.h
		reactive.h
		semaphore.h
		stdafx.h
		tweener.h
		utf.h
)

add_library(common ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
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
source_group(sources\\os\\windows os/windows/*)
source_group(sources\\os os/*)

