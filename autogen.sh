#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
cd $srcdir

PROJECT=gFTP
test -f lib/gftp.h || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

test -z "$AUTOMAKE"   && AUTOMAKE=automake
test -z "$ACLOCAL"    && ACLOCAL=aclocal
test -z "$AUTOCONF"   && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader

set -x 

#Get all required m4 macros required for configure
libtoolize --copy --force || exit 1
$ACLOCAL -I m4 || exit 1

#Generate config.h.in
$AUTOHEADER --force || exit 1

#Generate Makefile.in's
touch config.rpath
$AUTOMAKE --add-missing --copy --force || exit 1

if grep "IT_PROG_INTLTOOL" configure.ac ; then
	intltoolize -c --automake --force || exit 1
	# po/Makefile.in.in has these lines:
	#    mostlyclean:
	#       rm -f *.pox $(GETTEXT_PACKAGE).pot *.old.po cat-id-tbl.tmp
	# prevent $(GETTEXT_PACKAGE).pot from being deleted by `make clean`
	sed 's/pox \$(GETTEXT_PACKAGE).pot/pox/' po/Makefile.in.in > po/Makefile.in.inx
	mv -f po/Makefile.in.inx po/Makefile.in.in
fi

#generate configure
$AUTOCONF --force || exit 1

