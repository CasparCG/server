parsed_include = []
foreach include : modules_include
    parsed_include += '#include "@0@"'.format(include)
endforeach

parsed_init = []
foreach init : modules_init
    strings = init.split()
    parsed_init += '    @0@(dependencies);'.format(strings[0])
    parsed_init += '    CASPAR_LOG(info) << L"Initialized @0@ module.";'.format(strings[1])
    parsed_init += ''
endforeach

parsed_uninit = []
foreach uninit : modules_uninit
    parsed_uninit += '    @0@();'.format(uninit)
    parsed_uninit += ''
endforeach

parsed_cli_intercept = []
foreach intercept : modules_cli_intercept
    parsed_cli_intercept += '    if(@0@(argc, argv))'.format(intercept)
    parsed_cli_intercept += '        return true;'
    parsed_cli_intercept += ''
endforeach

sep = '\n'

mod_includes = sep.join(parsed_include)
cli_intercept = sep.join(parsed_cli_intercept)
mods_init = sep.join(parsed_init)
mods_uninit = sep.join(parsed_uninit)

included_modules_conf = configuration_data()
included_modules_conf.set('modules_include', mod_includes)
included_modules_conf.set('modules_cli_intercept', cli_intercept)
included_modules_conf.set('modules_init', mods_init)
included_modules_conf.set('modules_uninit', mods_uninit)

included_modules_h = configure_file(
    input: 'included_modules_h.tmpl',
    output: 'included_modules.h',
    configuration: included_modules_conf
)

server = executable(
    'casparcg',
    'linux_specific.cpp',
    'main.cpp',
    'server.cpp',
    included_modules_h,

    include_directories: include_root,
    dependencies: [tbb_dep, boost_dep, x11_dep],
    link_with: [accelerator_lib, common_lib, core_lib, protocol_lib, modules],
    install_rpath: '$ORIGIN/../lib'
)

staging_args = ['./copy_staging.sh', '@BUILD_ROOT@', '@SOURCE_ROOT@', '@INPUT@']
if have_cef
    staging_args += cef_path
endif

staging = custom_target(
    'staging',
    input: server,
    output: 'fake',
    command: staging_args,
    build_always: true,
)
