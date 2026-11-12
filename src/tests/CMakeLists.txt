# End-to-end tests: pytest files in e2e/, run against the installed server (cmake --install). One
# CTest test per file; run single tests with pytest directly (see AGENTS.md).
find_package(Python3 COMPONENTS Interpreter)
if (Python3_FOUND)
	execute_process(COMMAND ${Python3_EXECUTABLE} -c "import pytest" RESULT_VARIABLE no_pytest OUTPUT_QUIET ERROR_QUIET)
endif ()
if (NOT Python3_FOUND OR no_pytest)
	message(STATUS "pytest not found for ${Python3_EXECUTABLE}: end-to-end tests are not registered. "
		"Install pytest, or point -DPython3_EXECUTABLE at a Python that has it.")
	return()
endif ()

# e2e_test(<name> LABELS <label>...) registers e2e/test_<name>.py as the CTest test e2e.<name>. Every
# test file must be registered here. Label "accelerator" marks tests that start channels and so
# need an accelerator (a GPU, or Mesa's drivers).
function (e2e_test name)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "LABELS")
	add_test(NAME e2e.${name}
		COMMAND ${Python3_EXECUTABLE} -m pytest -p no:cacheprovider -v ${CMAKE_CURRENT_SOURCE_DIR}/e2e/test_${name}.py
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
	set_tests_properties(e2e.${name} PROPERTIES
		LABELS "e2e;${arg_LABELS}"
		TIMEOUT 900
		ENVIRONMENT "CASPAR_BINARY=${CMAKE_INSTALL_PREFIX}/bin/${CASPARCG_BINARY_NAME}")
endfunction ()

e2e_test(lifecycle LABELS accelerator)
