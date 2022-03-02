## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gftp-text

gftp_text_SOURCES=gftp-text.c textui.c

AM_CPPFLAGS = $(GLIB_CFLAGS)

LDADD = ../../lib/libgftp.a ../uicommon/libgftpui.a $(GLIB_LIBS) $(EXTRA_LIBS) $(READLINE_LIBS) $(SSL_LIBS) $(LIBINTL)

noinst_HEADERS = gftp-text.h
