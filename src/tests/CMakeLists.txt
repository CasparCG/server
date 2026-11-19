add_executable(route_audio_mapping_tests route_audio_mapping_tests.cpp)
target_compile_features(route_audio_mapping_tests PRIVATE cxx_std_20)
target_include_directories(route_audio_mapping_tests PRIVATE ..)
add_test(NAME route_audio_mapping_samples COMMAND route_audio_mapping_tests)

add_executable(audio_matrix_tests audio_matrix_tests.cpp)
target_compile_features(audio_matrix_tests PRIVATE cxx_std_20)
target_include_directories(audio_matrix_tests PRIVATE ..)
target_include_directories(audio_matrix_tests SYSTEM PRIVATE ${BOOST_INCLUDE_PATH})
casparcg_add_build_dependencies(audio_matrix_tests)
target_link_libraries(audio_matrix_tests PRIVATE core common TBB::tbbmalloc GLEW::glew)
if(MSVC)
	target_link_libraries(audio_matrix_tests PRIVATE Winmm.lib Ws2_32.lib OpenGL32.lib)
else()
	target_link_libraries(audio_matrix_tests PRIVATE ${Boost_LIBRARIES} OpenGL::GL)
endif()
add_test(NAME audio_matrix_samples COMMAND audio_matrix_tests --unit)
add_test(NAME audio_matrix_channel_samples COMMAND audio_matrix_tests --module)

set_tests_properties(route_audio_mapping_samples audio_matrix_samples
	PROPERTIES LABELS "unit" TIMEOUT 60)
set_tests_properties(audio_matrix_channel_samples
	PROPERTIES LABELS "module" TIMEOUT 60)
