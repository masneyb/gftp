#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# This was derived from Glib's autogen.sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=gFTP
TEST_TYPE=-f
FILE=lib/gftp.h
GETTEXTIZE=gettextize

DIE=0

AUTOMAKE=automake
ACLOCAL=aclocal
AUTOCONF=autoconf
AUTOHEADER=autoheader

autoconf_version=`$AUTOCONF --version 2>/dev/null`
if [ "x$autoconf_version" = "x" ] ; then 
       echo
       echo "GNU autoconf must be installed to build gFTP"
       echo "GNU autoconf is available from http://www.gnu.org/software/autoconf/"
       DIE=1
fi

automake_version=`$AUTOMAKE --version 2>/dev/null`
if [ "x$automake_version" = "x" ] ; then 
       echo
       echo "GNU automake must be installed to build gFTP"
       echo "GNU automake is available from http://www.gnu.org/software/automake/"
       DIE=1
fi

gettext_version=`$GETTEXTIZE --version 2>/dev/null | grep 'GNU'`
if [ "x$gettext_version" = "x" ] ; then 
       echo
       echo "GNU gettext must be installed to build gFTP"
       echo "GNU gettext is available from http://www.gnu.org/software/gettext/"
       DIE=1
fi

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        if test -z "$*"; then
                echo "I am going to run ./configure with no arguments - if you wish "
                echo "to pass any to it, please specify them on the $0 command line."
        fi
fi

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ *) am_opt=--include-deps;;
esac

intl=`$GETTEXTIZE --help 2>/dev/null | grep -- '--intl'`
if test -z "$intl"; then
	GETTEXTIZE_FLAGS="-c"
else
	GETTEXTIZE_FLAGS="-c --intl"
fi

echo "$GETTEXTIZE $GETTEXTIZE_FLAGS"
$GETTEXTIZE $GETTEXTIZE_FLAGS

echo "$ACLOCAL $ACLOCAL_FLAGS"
$ACLOCAL $ACLOCAL_FLAGS

# optionally feature autoheader
($AUTOHEADER --version)  < /dev/null > /dev/null 2>&1 && $AUTOHEADER

AUTOMAKE_FLAGS="-a -c $am_opt"
echo "$AUTOMAKE $AUTOMAKE_FLAGS"
$AUTOMAKE $AUTOMAKE_FLAGS

echo $AUTOCONF
$AUTOCONF

cd $ORIGDIR

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        CFLAGS="-Wall -ansi -D_GNU_SOURCE -O -g" $srcdir/configure "$@"

        echo 
        echo "Now type 'make' to compile $PROJECT."
fi
