## Process this file with automake to produce Makefile.in

bin_PROGRAMS = @GFTP_GTK@
EXTRA_PROGRAMS = gftp-gtk
gftp_gtk_SOURCES = bookmarks.c chmod_dialog.c delete_dialog.c dnd.c \
                     gftp-gtk.c gtkui.c gtkui_transfer.c menu-items.c \
                     misc-gtk.c options_dialog.c transfer.c view_dialog.c
INCLUDES = @GTK_CFLAGS@ @PTHREAD_CFLAGS@ -I../../intl
LDADD = ../../lib/libgftp.a ../../lib/fsplib/libfsp.a ../uicommon/libgftpui.a @GTK_LIBS@ @PTHREAD_LIBS@ @EXTRA_LIBS@ @GTHREAD_LIBS@ @SSL_LIBS@ @LIBINTL@
noinst_HEADERS = gftp-gtk.h
