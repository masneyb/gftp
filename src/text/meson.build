sources = [
    'gftp-text.c',
    'gftp-text.h',
    'textui.c',
]

all_sources = sources
foreach s : libgftp_sources
  all_sources += '../../lib/' + s
endforeach
foreach s : libgftpui_sources
  all_sources += '../../src/uicommon/' + s
endforeach

executable(
    'gftp-text',
    all_sources,
    c_args: libgftp_c_args,
    dependencies: libdeps + libgftp_deps + libgftpui_deps,
    install: true,
    install_dir: install_dir_base / get_option('bindir')
)