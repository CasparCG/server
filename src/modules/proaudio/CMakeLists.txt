cmake_minimum_required (VERSION 3.28)
project (proaudio)

set(SOURCES
		consumer/proaudio_consumer.cpp
		consumer/proaudio_consumer.h

		producer/proaudio_producer.cpp
		producer/proaudio_producer.h

		util/device_util.cpp
		util/device_util.h
		util/ring_buffer.h
		proaudio.cpp
		proaudio.h
)

casparcg_add_module_project(proaudio
	SOURCES ${SOURCES}
	INIT_FUNCTION "proaudio::init"
)
target_link_libraries(proaudio PRIVATE PortAudio::PortAudio)

set_target_properties(proaudio PROPERTIES FOLDER modules)
source_group(sources\\consumer consumer/*)
source_group(sources\\producer producer/*)
source_group(sources\\util util/*)
source_group(sources ./*)

if(BUILD_TESTING)
	add_executable(proaudio_tests tests/proaudio_tests.cpp util/device_util.cpp)
	target_compile_features(proaudio_tests PRIVATE cxx_std_20)
	target_include_directories(proaudio_tests PRIVATE ../..)
	target_include_directories(proaudio_tests SYSTEM PRIVATE ${BOOST_INCLUDE_PATH})
	casparcg_add_build_dependencies(proaudio_tests)
	target_link_libraries(proaudio_tests PRIVATE core common TBB::tbbmalloc GLEW::glew PortAudio::PortAudio)
	if(MSVC)
		target_link_libraries(proaudio_tests PRIVATE Winmm.lib Ws2_32.lib OpenGL32.lib)
	else()
		target_link_libraries(proaudio_tests PRIVATE ${Boost_LIBRARIES} OpenGL::GL)
	endif()
	add_test(NAME proaudio_mock_samples COMMAND proaudio_tests)
	set_tests_properties(proaudio_mock_samples PROPERTIES LABELS "module" TIMEOUT 60)
endif()
