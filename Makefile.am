## Process this file with automake to produce Makefile.in

SUBDIRS=docs lib po src
EXTRA_DIST=ChangeLog-old README THANKS TODO \
	gftp.spec.in debian/changelog debian/control debian/copyright \
	debian/dirs debian/docs debian/menu debian/rules

dist-hook:
	cp gftp.spec $(distdir)     

ACLOCAL_AMFLAGS = -I m4
