## Process this file with automake to produce Makefile.in

bin_PROGRAMS = @GFTP_GTK@
EXTRA_PROGRAMS = gftp-gtk
gftp_gtk_SOURCES = bookmarks.c chmod_dialog.c delete_dialog.c dnd.c \
                     gftp-gtk.c menu-items.c misc-gtk.c mkdir_dialog.c \
                     options_dialog.c rename_dialog.c transfer.c view_dialog.c
INCLUDES = @GTK_CFLAGS@ @PTHREAD_CFLAGS@ -DSHARE_DIR=\"$(datadir)/gftp\" -DLOCALE_DIR=\"$(localedir)\" -I../../intl
LDADD = ../../lib/libgftp.a @INTLLIBS@ @GTK_LIBS@ @PTHREAD_LIBS@ @EXTRA_LIBS@
noinst_HEADERS = gftp-gtk.h
localedir = $(datadir)/locale
