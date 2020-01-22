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
AUTOMAKE=$AUTOMAKE intltoolize -c --automake --force || exit 1

#generate configure
$AUTOCONF --force || exit 1

