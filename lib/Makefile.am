## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libgftp.a

libgftp_a_SOURCES= \
	cache.c \
	charset-conv.c \
	config_file.c \
	fsp.c fsplib.c \
	ftp-dir-listing.c \
	local.c \
	misc.c \
	protocols.c \
	protocol_bookmark.c \
	protocol_ftp.c \
	protocol_ftps.c \
	pty.c \
	sshv2.c \
	sslcommon.c \
	socket-connect.c \
	sockutils.c

AM_CPPFLAGS= \
	@GLIB_CFLAGS@ \
	@PTHREAD_CFLAGS@ \
	-DSHARE_DIR=\"$(datadir)/gftp\" \
	-DLOCALE_DIR=\"$(datadir)/locale\"

noinst_HEADERS= \
	gftp.h \
	ftpcommon.h \
	options.h \
	fsplib.h
