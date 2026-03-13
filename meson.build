project('gftp', 'c',
  version: '2.9.1b',
  default_options: ['warning_level=2', 'c_std=c11'])

if get_option('buildtype').startswith('debug')
    add_project_arguments('-DGFTP_DEBUG', language:'c')
endif

if (not get_option('textport')) and (not get_option('gtkport'))
    error('textport and gtkport were both disabled')
endif

if get_option('gtk3') and get_option('gtk2')
    error('options gtk3 and gtk2 cannot be enabled at the same time')
endif

project_version = meson.project_version()
project_name = meson.project_name()
install_dir_base = get_option('prefix')

add_project_arguments(f'-DPACKAGE_NAME="@project_name@"', language:'c')

cc = meson.get_compiler('c')
pthread = cc.find_library('pthread', required: true)

add_project_arguments('-D_REENTRANT', language:'c')
add_project_arguments(f'-DVERSION="@project_version@"', language:'c')

# Suppress unused parameter warnings (common in callback functions)
add_project_arguments('-Wno-unused-parameter', language:'c')

glib = dependency('glib-2.0', version: '>=2.32', required: true)
gtk2 = dependency('gtk+-2.0', version: '>=2.14', required: get_option('gtkport') and get_option('gtk2'))
gtk3 = dependency('gtk+-3.0', version: '>=3.0', required: get_option('gtkport') and get_option('gtk3'))
ssl = dependency('openssl', required: get_option('ssl'))

# Check for gtk-mac-integration on macOS
if host_machine.system() == 'darwin'
    gtkmacintegration = dependency('gtk-mac-integration-gtk3', required: true)
    if gtkmacintegration.found()
        add_project_arguments('-DMAC_INTEGRATION', language: 'c')
    endif
endif

libdeps = [glib, pthread]
if get_option('ssl') and ssl.found()
    add_project_arguments('-DUSE_SSL="1"', language:'c')
    libdeps += ssl
    add_project_arguments('-I' + meson.current_source_dir() + '/third_party/openssl/include', language: 'c')
endif

# On macOS, add CoreFoundation for app bundle support
if host_machine.system() == 'darwin'
    corefoundation = dependency('appleframeworks', modules: ['CoreFoundation'], required: false)
    if corefoundation.found()
        libdeps += corefoundation
        add_project_arguments('-DHAVE_COREFOUNDATION', language:'c')
    endif
endif

gtk_dep = []
if get_option('gtkport')
    if get_option('gtk3')
        gtk_dep += gtk3
        # Add gtk-mac-integration for GTK3 on macOS
        if host_machine.system() == 'darwin'
            if gtkmacintegration.found()
                gtk_dep += gtkmacintegration
            endif
        endif
    elif get_option('gtk2')
        gtk_dep += gtk2
    else
        error('gtkport cannot be built with both gtk2 and gtk3 options disabled')
    endif
endif

subdir('po')
subdir('lib')
subdir('src')
subdir('src/uicommon')
subdir('docs')
subdir('icons')
#subdir('third_party/glib-2.0-schemas

if get_option('textport')
    subdir('src/text')
endif

if get_option('gtkport')
    subdir('src/gtk')
endif
