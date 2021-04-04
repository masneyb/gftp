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
	test -f ./configure   || ./autogen.sh
	test -f ./po/Makefile || ./configure
	make -C po update-po
	# cleanup
	rm -f po/*.po~
	sed -i '/#~ /d' po/*.po
	#git clean -dfx
	exit
fi

if test "$1" =  "linguas" ; then
	test -d "po/" || exit
	pot_in=$(grep '_(' $(find . -type f -name '*.h' -or -name '*.c') | sed -e 's%^\./%%' -e 's%:.*%%' | sort -u)
	echo "${pot_in}" > po/POTFILES.in
	linguas=$(find po -name '*.po' | sed -e 's%.*/%%' -e 's%\.po%%' | sort)
	echo ${linguas} > po/LINGUAS
	exit
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

# Get all required m4 macros required for configure
$LIBTOOLIZE ${verbose} --copy --force || exit 1
$ACLOCAL ${verbose} || exit 1

# Generate config.h.in
$AUTOHEADER ${verbose} --force || exit 1

# Generate Makefile.in's
auxdir='.'
if grep "AC_CONFIG_AUX_DIR" configure.ac ; then
	auxdir="$(grep AC_CONFIG_AUX_DIR configure.ac | cut -f 2 -d '[' | cut -f 1 -d ']')"
fi
touch ${auxdir}/config.rpath
$AUTOMAKE ${verbose} --add-missing --copy --force || exit 1

if grep -q "IT_PROG_INTLTOOL" configure.ac ; then
	intltoolize ${verbose2} -c --automake --force || exit 1
	# po/Makefile.in.in has these lines:
	#    mostlyclean:
	#       rm -f *.pox $(GETTEXT_PACKAGE).pot *.old.po cat-id-tbl.tmp
	# prevent $(GETTEXT_PACKAGE).pot from being deleted by `make clean`
	sed 's/pox \$(GETTEXT_PACKAGE).pot/pox/' po/Makefile.in.in > po/Makefile.in.inx
	mv -f po/Makefile.in.inx po/Makefile.in.in
fi

# generate configure
$AUTOCONF ${verbose} --force || exit 1

rm -rf autom4te.cache
