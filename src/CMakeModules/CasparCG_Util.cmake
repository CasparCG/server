# Collect all ExternalProjects that have been defined
set(CASPARCG_EXTERNAL_PROJECTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_external_project NAME)
	SET (CASPARCG_EXTERNAL_PROJECTS "${CASPARCG_EXTERNAL_PROJECTS}" "${NAME}" CACHE INTERNAL "")
ENDFUNCTION()

# Mark a project as depending on all of the ExternalProjects, to ensure build order
FUNCTION(casparcg_add_build_dependencies PROJECT)
	ADD_DEPENDENCIES (${PROJECT} ${CASPARCG_EXTERNAL_PROJECTS})
ENDFUNCTION()

# Specify a module header file to build into the application
SET (CASPARCG_MODULE_INCLUDE_STATEMENTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_include_statement HEADER_FILE_TO_INCLUDE)
	SET (CASPARCG_MODULE_INCLUDE_STATEMENTS "${CASPARCG_MODULE_INCLUDE_STATEMENTS}"
			"#include <${HEADER_FILE_TO_INCLUDE}>"
			CACHE INTERNAL ""
	)
ENDFUNCTION ()

# Specify a module init function to call during application initialisation
SET (CASPARCG_MODULE_INIT_STATEMENTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_init_statement INIT_FUNCTION_NAME NAME_TO_LOG)
	SET (CASPARCG_MODULE_INIT_STATEMENTS "${CASPARCG_MODULE_INIT_STATEMENTS}"
			"	${INIT_FUNCTION_NAME}(dependencies)\;"
			"	CASPAR_LOG(info) << L\"Initialized ${NAME_TO_LOG} module.\"\;"
			""
			CACHE INTERNAL ""
	)
ENDFUNCTION ()

# Specify a module uninit function to call during application shutdown
SET (CASPARCG_MODULE_UNINIT_STATEMENTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_uninit_statement UNINIT_FUNCTION_NAME)
	SET (CASPARCG_MODULE_UNINIT_STATEMENTS
			"	${UNINIT_FUNCTION_NAME}()\;"
			"${CASPARCG_MODULE_UNINIT_STATEMENTS}"
			CACHE INTERNAL ""
	)
ENDFUNCTION ()

# Specify a module function to call during startup to handle custom command line arguments
SET (CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_command_line_arg_interceptor INTERCEPTOR_FUNCTION_NAME)
	set(CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS "${CASPARCG_MODULE_COMMAND_LINE_ARG_INTERCEPTORS_STATEMENTS}"
			"	if (${INTERCEPTOR_FUNCTION_NAME}(argc, argv))"
			"		return true\;"
			""
			CACHE INTERNAL ""
	)
ENDFUNCTION ()

# Add a module to be linked into the application
SET (CASPARCG_MODULE_PROJECTS "" CACHE INTERNAL "")
FUNCTION (casparcg_add_module_project PROJECT)
	SET (CASPARCG_MODULE_PROJECTS "${CASPARCG_MODULE_PROJECTS}" "${PROJECT}" CACHE INTERNAL "")
ENDFUNCTION ()

# http://stackoverflow.com/questions/7172670/best-shortest-way-to-join-a-list-in-cmake
FUNCTION (join_list VALUES GLUE OUTPUT)
	STRING (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
	STRING (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
	SET (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
ENDFUNCTION ()
