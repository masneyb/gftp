## Process this file with automake to produce Makefile.in 

noinst_LIBRARIES = libgftp.a
libgftp_a_SOURCES=bookmark.c cache.c config_file.c local.c misc.c protocols.c \
                  rfc959.c rfc2068.c ssh.c sshv2.c
INCLUDES=@GLIB_CFLAGS@ @PTHREAD_CFLAGS@ -I../intl -DSHARE_DIR=\"$(datadir)/gftp\" 
noinst_HEADERS=gftp.h options.h
