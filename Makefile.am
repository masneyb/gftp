## Process this file with automake to produce Makefile.in

SUBDIRS= docs lib po src
EXTRA_DIST= config.rpath ChangeLog-old README THANKS TODO \
 gftp.spec.in debian/changelog debian/compat debian/control debian/copyright \
 debian/gftp-common.dirs debian/gftp-common.docs debian/gftp-common.install \
 debian/gftp-gtk.install debian/gftp-gtk.links debian/gftp-gtk.menu \
 debian/gftp-text.install debian/gftp-text.links debian/gftp-text.postinst \
 debian/gftp-text.prerm debian/rules

dist-hook:
	cp gftp.spec $(distdir)     

ACLOCAL_AMFLAGS = -I m4

