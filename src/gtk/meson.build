sources = [
    'bookmarks.c',
	'chmod_dialog.c',
	'dnd.c',
	'gftp-gtk.c',
	'../gtkcompat.h',
	'gtkui.c',
	'gtkui_transfer.c',
	'listbox.c',
	'menu-items.c',
	'misc-gtk.c',
	'options_dialog.c',
	'platform_specific.c',
	'transfer.c',
	'view_dialog.c'
]

all_sources = sources
foreach s : libgftp_sources
  all_sources += '../../lib/' + s
endforeach
foreach s : libgftpui_sources
  all_sources += '../../src/uicommon/' + s
endforeach

executable(
    'gftp-gtk',
    all_sources,
    c_args: libgftp_c_args,
    dependencies: libdeps + gtk_dep + libgftp_deps + libgftpui_deps,
    install: true,
    install_dir: install_dir_base / get_option('bindir')
)