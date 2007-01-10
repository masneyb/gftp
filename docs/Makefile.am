## Process this file with automake to produce Makefile.in

man_MANS=gftp.1
SUBDIRS=sample.gftp
EXTRA_DIST=USERS-GUIDE gftp.1 gftp.desktop gftp.lsm gftp.png parse-netrc.pl \
           gftp-faq.sgml

Utilitiesdir = $(datadir)/applications
Utilities_DATA = gftp.desktop

Iconsdir = $(datadir)/pixmaps
Icons_DATA = gftp.png

html:
	if HAVE_DOCBOOK
		if test -w $(srcdir); then \
			(cd $(srcdir); \
			db2html -u gftp-faq.sgml); \
		fi
	else
		echo "*** Warning: FAQ not built"

	endif

pdf:
	if HAVE_DOCBOOK
		if test -w $(srcdir); then \
			(cd $(srcdir); db2pdf gftp-faq.sgml); \
		fi
	else
		echo "*** Warning: FAQ not built"
	endif

clean:
	rm -rf gftp-faq gftp-faq.junk gftp-faq.pdf

dist-hook: html
	if HAVE_DOCBOOK
		cp $(srcdir)/gftp-faq/gftp-faq.html $(distdir)/../README.html
		html2text -nobs -o $(distdir)/../README -style pretty $(srcdir)/gftp-faq/gftp-faq.html 
	else
		echo "***"
		echo "*** Warning: FAQ not built"
		echo "*** DISTRIBUTION IS INCOMPLETE"
		echo "***"
	endif

