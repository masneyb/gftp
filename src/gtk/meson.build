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

executable(
    'gftp-gtk',
    sources,
    dependencies: libdeps + gtk_dep,
    link_with: [libgftp, libgftpui],
    install: true
)