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
	if test -w $(srcdir); then \
		(cd $(srcdir); \
		db2html -u gftp-faq.sgml); \
	fi

pdf:
	if test -w $(srcdir); then \
		(cd $(srcdir); db2pdf gftp-faq.sgml); \
	fi

clean:
	rm -rf gftp-faq gftp-faq.junk gftp-faq.pdf

dist-hook: html
	cp $(srcdir)/gftp-faq/gftp-faq.html $(distdir)/../README.html
	html2text -nobs -o $(distdir)/../README -style pretty $(srcdir)/gftp-faq/gftp-faq.html 
