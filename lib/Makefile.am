## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libgftp.a

libgftp_a_SOURCES= \
	cache.c \
	charset-conv.c \
	config_file.c \
	ftp-dir-listing.c \
	misc.c \
	protocols.c \
	protocol_bookmark.c \
	protocol_ftp.c \
	protocol_ftps.c \
	protocol_localfs.c \
	protocol_http.c \
	protocol_https.c \
	pty.c \
	sshv2.c \
	sslcommon.c \
	socket-connect.c \
	sockutils.c

AM_CPPFLAGS= \
	$(GLIB_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	-DSHARE_DIR=\"$(datadir)/gftp\" \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	-DDOC_DIR=\"$(docdir)\"

noinst_HEADERS= \
	gftp.h \
	protocol_ftp.h \
	protocol_http.h \
	options.h
