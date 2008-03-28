## Process this file with automake to produce Makefile.in 

SUBDIRS=fsplib
noinst_LIBRARIES = libgftp.a
libgftp_a_SOURCES=bookmark.c cache.c charset-conv.c config_file.c fsp.c ftps.c \
                  https.c local.c misc.c mkstemps.c parse-dir-listing.c \
                  protocols.c pty.c rfc959.c rfc2068.c sshv2.c sslcommon.c \
                  socket-connect.c socket-connect-getaddrinfo.c \
                  socket-connect-gethostbyname.c sockutils.c
INCLUDES=@GLIB_CFLAGS@ @PTHREAD_CFLAGS@ -I../intl -DSHARE_DIR=\"$(datadir)/gftp\" -DLOCALE_DIR=\"$(datadir)/locale\"
noinst_HEADERS=gftp.h ftpcommon.h httpcommon.h options.h
