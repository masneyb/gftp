## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libgftp.a
libgftp_a_SOURCES=bookmark.c cache.c config_file.c ftps.c https.c local.c misc.c \
                  protocols.c pty.c rfc959.c rfc2068.c sshv2.c sslcommon.c
INCLUDES=@GLIB_CFLAGS@ @PTHREAD_CFLAGS@ -I../intl -DSHARE_DIR=\"$(datadir)/gftp\" -DLOCALE_DIR=\"$(localedir)\"
noinst_HEADERS=gftp.h ftpcommon.h httpcommon.h options.h
