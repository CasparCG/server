
html_lib = static_library(
    'html',
	'producer/html_cg_proxy.cpp',
	'producer/html_producer.cpp',

	'html.cpp',

    include_directories : [include_root, cef_include],
    link_with : [common_lib, core_lib],
    dependencies: [boost_dep, cef_dep]
)

modules += html_lib
modules_include += 'modules/html/html.h'
modules_init += 'html::init html'
modules_cli_intercept += 'html::intercept_command_line'
