## Process this file with automake to produce Makefile.in

bin_PROGRAMS = @GFTP_TEXT@
EXTRA_PROGRAMS = gftp-text
gftp_text_SOURCES=gftp-text.c 
INCLUDES=@GLIB_CFLAGS@ -DSHARE_DIR=\"$(datadir)/gftp\" -DLOCALE_DIR=\"$(localedir)\" -I../../intl
LDADD = ../../lib/libgftp.a @LIBINTL@ @GLIB_LIBS@ @EXTRA_LIBS@ @READLINE_LIBS@
noinst_HEADERS=gftp-text.h
localedir=$(datadir)/locale
