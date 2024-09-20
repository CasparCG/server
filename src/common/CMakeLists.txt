cmake_minimum_required (VERSION 3.16)
project (common)

set(SOURCES
		diagnostics/graph.cpp

		gl/gl_check.cpp

		base64.cpp
		env.cpp
		filesystem.cpp
		log.cpp
		tweener.cpp
		utf.cpp
)
if (MSVC)
	list(APPEND SOURCES
			compiler/vs/disable_silly_warnings.h

			os/windows/filesystem.cpp
			os/windows/prec_timer.cpp
			os/windows/thread.cpp
			os/windows/windows.h
	)
else ()
	list(APPEND SOURCES
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

casparcg_add_library(common SOURCES ${SOURCES} ${HEADERS})
target_include_directories(common PRIVATE
    ..
    ${GLEW_INCLUDE_PATH}
    )
target_precompile_headers(common PRIVATE "stdafx.h")

source_group(sources ./*)
source_group(sources\\gl gl/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\compiler\\vs compiler/vs/*)
source_group(sources\\os\\windows os/windows/*)
source_group(sources\\os os/*)
