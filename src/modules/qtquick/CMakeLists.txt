cmake_minimum_required (VERSION 2.6)
project (qtquick)

set(SOURCES
		producer/qtquick_cg_proxy.cpp
		producer/qtquick_producer.cpp

		qtquick.cpp
)
set(HEADERS
		producer/qtquick_cg_proxy.h
		producer/qtquick_producer.h

		qtquick.h
)

add_library(qtquick ${SOURCES} ${HEADERS})

set_property(TARGET qtquick PROPERTY CXX_STANDARD 17)

include_directories(..)
include_directories(../..)
include_directories(${BOOST_INCLUDE_PATH})
include_directories(${TBB_INCLUDE_PATH})

find_package(Qt5Quick 5.10 REQUIRED)
target_link_libraries(qtquick Qt5::Core Qt5::Quick)

set_target_properties(qtquick PROPERTIES FOLDER modules)
source_group(sources\\producer producer/*)
source_group(sources ./*)

target_link_libraries(qtquick
  common
  core
)

casparcg_add_include_statement("modules/qtquick/qtquick.h")
casparcg_add_init_statement("qtquick::init" "qtquick")
casparcg_add_uninit_statement("qtquick::uninit")
casparcg_add_command_line_arg_interceptor("qtquick::intercept_command_line")
casparcg_add_module_project("qtquick")
