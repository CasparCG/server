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
		timer.h
		tweener.h
		utf.h
)

add_library(common ${SOURCES} ${HEADERS} ${OS_SPECIFIC_SOURCES})
target_compile_features(common PRIVATE cxx_std_14)
target_include_directories(common PRIVATE
    ..
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${GLEW_INCLUDE_PATH}
    )
target_precompile_headers(common PRIVATE
    "compiler/vs/disable_silly_warnings.h"
    <algorithm>
    <atomic>
    <boost/core/null_deleter.hpp>
    <boost/date_time/posix_time/posix_time.hpp>
    <boost/exception/all.hpp>
    <boost/filesystem.hpp>
    <boost/filesystem/convenience.hpp>
    <boost/filesystem/fstream.hpp>
    <boost/filesystem/operations.hpp>
    <boost/filesystem/path.hpp>
    <GL/glew.h>
    <boost/make_shared.hpp>
    <boost/optional.hpp>
    <boost/property_tree/ptree.hpp>
    <boost/property_tree/ptree_fwd.hpp>
    <boost/property_tree/xml_parser.hpp>
    <boost/range/adaptor/sliced.hpp>
    <boost/range/adaptor/transformed.hpp>
    <boost/range/join.hpp>
    <boost/regex.hpp>
    <boost/shared_ptr.hpp>
    <boost/thread.hpp>
    <boost/throw_exception.hpp>
    <cmath>
    <cstddef>
    <cstdint>
    <cstring>
    <ctime>
    <exception>
    <fstream>
    <functional>
    <future>
    <initializer_list>
    <iomanip>
    <ios>
    <iostream>
    <list>
    <locale>
    <map>
    <memory>
    <mutex>
    <ostream>
    <stdexcept>
    <string>
    <tbb/concurrent_queue.h>
    <tuple>
    <type_traits>
    <unordered_map>
    <vector>

   )
configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")


source_group(sources ./*)
source_group(sources\\gl gl/*)
source_group(sources\\diagnostics diagnostics/*)
source_group(sources\\compiler\\vs compiler/vs/*)
source_group(sources\\os\\windows os/windows/*)
source_group(sources\\os os/*)
