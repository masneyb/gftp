## Process this file with automake to produce Makefile.in

bin_PROGRAMS = @GFTP_TEXT@
EXTRA_PROGRAMS = gftp-text
gftp_text_SOURCES=gftp-text.c textui.c
INCLUDES=@GLIB_CFLAGS@ -I../../intl
LDADD = ../../lib/libgftp.a ../../lib/fsplib/libfsp.a ../uicommon/libgftpui.a @GLIB_LIBS@ @EXTRA_LIBS@ @READLINE_LIBS@ @SSL_LIBS@ @LIBINTL@
noinst_HEADERS=gftp-text.h
localedir=$(datadir)/locale
