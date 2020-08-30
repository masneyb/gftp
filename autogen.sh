#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
cd $srcdir

PROJECT=gFTP
if ! test -f lib/gftp.h  ; then
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
fi

test -z "$AUTOMAKE"   && AUTOMAKE=automake
test -z "$ACLOCAL"    && ACLOCAL=aclocal
test -z "$AUTOCONF"   && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader

if test "$1" == "verbose" || test "$1" == "--verbose" ; then
	set -x
	verbose='--verbose'
	verbose2='--debug'
fi

#Get all required m4 macros required for configure
libtoolize ${verbose} --copy --force || exit 1
$ACLOCAL ${verbose} -I m4 || exit 1

#Generate config.h.in
$AUTOHEADER ${verbose} --force || exit 1

#Generate Makefile.in's
touch config.rpath
$AUTOMAKE ${verbose} --add-missing --copy --force || exit 1

if grep "IT_PROG_INTLTOOL" configure.ac >/dev/null ; then
	intltoolize ${verbose2} -c --automake --force || exit 1
	# po/Makefile.in.in has these lines:
	#    mostlyclean:
	#       rm -f *.pox $(GETTEXT_PACKAGE).pot *.old.po cat-id-tbl.tmp
	# prevent $(GETTEXT_PACKAGE).pot from being deleted by `make clean`
	sed 's/pox \$(GETTEXT_PACKAGE).pot/pox/' po/Makefile.in.in > po/Makefile.in.inx
	mv -f po/Makefile.in.inx po/Makefile.in.in
fi

#generate configure
$AUTOCONF ${verbose} --force || exit 1

rm -rf autom4te.cache
