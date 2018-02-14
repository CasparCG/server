cmake_minimum_required (VERSION 2.6)
project (common)

set(SOURCES
		diagnostics/graph.cpp

		gl/gl_check.cpp

		base64.cpp
		env.cpp
		filesystem.cpp
		log.cpp
		stdafx.cpp
		tweener.cpp
		utf.cpp
)
if (MSVC)
	set(OS_SPECIFIC_SOURCES
			compiler/vs/disable_silly_warnings.h

			os/windows/filesystem.cpp
			os/windows/prec_timer.cpp
			os/windows/thread.cpp
			os/windows/windows.h
	)
else ()
	set(OS_SPECIFIC_SOURCES
			os/linux/filesystem.cpp
			os/linux/prec_timer.cpp
			os/linux/thread.cpp
	)
endif ()
set(HEADERS
		diagnostics/graph.h

		gl/gl_check.h

		os/filesystem.h
		os/thread.h

		array.h
		assert.h
		base64.h
		endian.h
		enum_class.h
		env.h
		executor.h
		except.h
		filesystem.h
		forward.h
		future.h
		log.h
		memory.h
		memshfl.h
		param.h
		prec_timer.h
		ptree.h
		scope_exit.h
		stdafx.h
		timer.h
		tweener.h
		utf.h
)

add_library(common ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
add_precompiled_header(common stdafx.h FORCEINCLUDE)
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")

include_directories(..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})
include_directories(${GLEW_INCLUDE_PATH})

source_group(sources ./*)
source_group(sources\\gl gl/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\compiler\\vs compiler/vs/*)
source_group(sources\\os\\windows os/windows/*)
source_group(sources\\os os/*)
