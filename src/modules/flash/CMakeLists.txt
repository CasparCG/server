cmake_minimum_required (VERSION 3.16)
project (flash CXX)

set(SOURCES
		producer/FlashAxContainer.cpp
		producer/flash_producer.cpp

		util/swf.cpp

		flash.cpp

		interop/Flash9e_i.c
)
set(HEADERS
		producer/FlashAxContainer.h
		producer/flash_producer.h

		util/swf.h
		interop/axflash.h
		interop/TimerHelper.h

		flash.h
)

add_library(flash ${SOURCES} ${HEADERS})
target_compile_features(flash PRIVATE cxx_std_14)
target_include_directories(flash PRIVATE
    ..
    ../../
    ${BOOST_INCLUDE_PATH}
    ${TBB_INCLUDE_PATH}
    ${ZLIB_INCLUDE_PATH}
    )
target_precompile_headers(flash PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:../common/compiler/vs/disable_silly_warnings.h>"
    $<$<COMPILE_LANGUAGE:CXX>:<algorithm$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<array$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<cassert$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<deque$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<functional$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<cmath$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<memory$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<queue$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<string$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<vector$<ANGLE-R>>

    $<$<COMPILE_LANGUAGE:CXX>:<tbb/concurrent_queue.h$<ANGLE-R>>

    $<$<COMPILE_LANGUAGE:CXX>:<boost/algorithm/string.hpp$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<boost/filesystem.hpp$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<boost/format.hpp$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<boost/property_tree/ptree.hpp$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<boost/range/algorithm.hpp$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:CXX>:<boost/regex.hpp$<ANGLE-R>>

    "$<$<COMPILE_LANGUAGE:CXX>:../common/memory.h>"
    "$<$<COMPILE_LANGUAGE:CXX>:../common/utf.h>"

    "$<$<COMPILE_LANGUAGE:CXX>:../common/except.h>"
    "$<$<COMPILE_LANGUAGE:CXX>:../common/log.h>"

    $<$<COMPILE_LANGUAGE:C>:<rpc.h$<ANGLE-R>>
    $<$<COMPILE_LANGUAGE:C>:<rpcndr.h$<ANGLE-R>>
    )

configure_file("${PROJECT_SOURCE_DIR}/packages.config" "${CMAKE_CURRENT_BINARY_DIR}/packages.config")


set_target_properties(flash PROPERTIES FOLDER modules)
source_group(sources\\interop interop/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

target_link_libraries(flash common core)

casparcg_add_include_statement("modules/flash/flash.h")
casparcg_add_init_statement("flash::init" "flash")
casparcg_add_module_project("flash")
