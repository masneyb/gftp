#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# special params:
# - po       update .pot & po files
# - linguas  update LINGUAS and POTFILES.in
# - release  create release tarball

#===========================================================================

if test "$1" =  "po" ; then
	# Before creating a release you might want to update the po files
	test -d "po/"         || exit
	#git clean -dfx
	test -f ./configure   || ./autogen.sh
	test -f ./po/Makefile || ./configure
	find po -name '*.pot' -delete # updates don't happen if pot files already exist...
	make -C po update-po
	# cleanup
	rm -f po/*.po~
	#sed -i '/#~ /d' po/*.po
	#git clean -dfx
	exit
fi

if test "$1" =  "linguas" ; then
	./po/Makefile.in.gen
	exit $?
fi

#===========================================================================

if test "$1" == "release" || test "$1" == "--release" ; then
	pkg="$(grep -m 1 AC_INIT configure.ac | cut -f 2 -d '[' | cut -f 1 -d ']')"
	ver="$(grep -m 1 AC_INIT configure.ac | cut -f 3 -d '[' | cut -f 1 -d ']')"
	ver=$(echo $ver)
	dir=${pkg}-${ver}
	rm -rf ../$dir
	mkdir -p ../$dir
	cp -rf $PWD/* ../$dir
	( cd ../$dir ; ./autogen.sh )
	cd ..
	tar -Jcf ${dir}.tar.xz $dir
	exit
fi


#===========================================================================
#                    autogen.sh [--verbose]
#===========================================================================

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
cd $srcdir

test -z "$AUTOMAKE"   && AUTOMAKE=automake
test -z "$ACLOCAL"    && ACLOCAL=aclocal
test -z "$AUTOCONF"   && AUTOCONF=autoconf
test -z "$AUTOHEADER" && AUTOHEADER=autoheader
test -z "$LIBTOOLIZE" && LIBTOOLIZE=$(which libtoolize glibtoolize 2>/dev/null | head -1)
test -z "$LIBTOOLIZE" && LIBTOOLIZE=libtoolize #paranoid precaution

if test "$1" == "verbose" || test "$1" == "--verbose" ; then
	set -x
	verbose='--verbose'
	verbose2='--debug'
fi

# pre-create some dirs / files
auxdir='.'
if grep -q "AC_CONFIG_AUX_DIR" configure.ac ; then
	auxdir="$(grep AC_CONFIG_AUX_DIR configure.ac | cut -f 2 -d '[' | cut -f 1 -d ']')"
fi
mkdir -p ${auxdir}
touch ${auxdir}/config.rpath
m4dir="$(grep AC_CONFIG_MACRO_DIR configure.ac | cut -f 2 -d '[' | cut -f 1 -d ']')"
if test -n "$m4dir" ; then
	mkdir -p ${m4dir}
fi

# Get all required m4 macros required for configure
if grep -q LT_INIT configure.ac ; then
	$LIBTOOLIZE ${verbose} --copy --force || exit 1
fi
$ACLOCAL ${verbose} || exit 1

# Generate config.h.in
$AUTOHEADER ${verbose} --force || exit 1

# Generate Makefile.in's
$AUTOMAKE ${verbose} --add-missing --copy --force || exit 1

# generate configure
$AUTOCONF ${verbose} --force || exit 1

rm -rf autom4te.cache
