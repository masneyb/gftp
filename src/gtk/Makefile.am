## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gftp-gtk

gftp_gtk_SOURCES = bookmarks.c \
	chmod_dialog.c \
	dnd.c \
	gftp-gtk.c \
	../gtkcompat.h \
	gtkui.c \
	gtkui_transfer.c \
	menu-items.c \
	misc-gtk.c \
	options_dialog.c \
	platform_specific.c \
	transfer.c \
	view_dialog.c

AM_CPPFLAGS = $(GTK_CFLAGS) $(PTHREAD_CFLAGS)

LDADD = ../../lib/libgftp.a ../uicommon/libgftpui.a $(GTK_LIBS) $(PTHREAD_LIBS) $(EXTRA_LIBS) $(GTHREAD_LIBS) $(SSL_LIBS) $(LIBINTL)

noinst_HEADERS = gftp-gtk.h
