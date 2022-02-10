#!/bin/sh

case $1 in po|pot)
	# Before creating a release you might want to update the po files
	#git clean -dfx
	if [ -f ../configure.ac ] ; then
		cd ..
	fi
	test -f ./configure   || ./autogen.sh
	test -f ./po/Makefile || ./configure
	test -f ./po/Makefile || exit 1
	find po -name '*.pot' -delete # updates don't happen if pot files already exist...
	make -C po update-${1}
	# cleanup
	rm -f po/*.po~
	#sed -i '/#~ /d' po/*.po
	#git clean -dfx
	if [ "$1" = "pot" ] ; then
		potfile="$(ls po/*.pot | head -n 1)"
		echo
		echo "** $potfile has been updated"
	fi
	exit
	;;
esac


if test "$1" =  "linguas" ; then
	cd $(dirname "$0")

	LINGUAS="$(ls *.po | sed 's/\.po$//')"
	filez=$(find .. -type f -name '*.h' -or -name '*.c' -or -name '*.cc' -or -name '*.cpp' -or -name '*.hh')
	POTFILES="$(grep '_(' $filez | sed -e 's%^\./%%' -e 's%:.*%%' | sort -u)"

	sed -i \
		-e "s%LINGUAS = .*%LINGUAS = $(echo $LINGUAS)%" \
		-e "s%POTFILES = .*%POTFILES = $(echo $POTFILES)%" \
		Makefile.in

	grep 'LINGUAS =' Makefile.in
	grep 'POTFILES =' Makefile.in
	echo
	echo "** Makefile.in has been updated"
fi
