screen_shaders = [
    ['consumer_screen_vertex.h', 'consumer/screen.vert', 'vertex_shader'],
    ['consumer_screen_fragment.h', 'consumer/screen.frag', 'fragment_shader']
]
screen_shader_targets = []

foreach shader : screen_shaders
    screen_shader_targets += custom_target(shader[2], input: shader[1], output: shader[0], command: [bin2c, 'caspar::screen', shader[2], '@INPUT@'], capture: true)
endforeach

linux_sources = [
    'util/x11_util.cpp'
]

screen_lib = static_library(
    'screen',
	'consumer/screen_consumer.cpp',

	'screen.cpp',
    linux_sources,
    screen_shader_targets,

    link_with: [common_lib, core_lib, accelerator_lib],
    dependencies: [boost_dep, sfml_dep, x11_dep],
    include_directories: [include_root],
)

modules += screen_lib
modules_include += 'modules/screen/screen.h'
modules_init += 'screen::init screen'
