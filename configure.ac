dnl Process this file with autoconf to produce a configure script.

AC_INIT([gftp], [2.0.19], [https://www.gftp.org/])
AM_INIT_AUTOMAKE([1.11 -Wall foreign no-dist-gzip dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_GNU_GETTEXT([external])
AM_PROG_AR
AC_PROG_LIBTOOL
LT_INIT

# Init intltool
IT_PROG_INTLTOOL

AC_ARG_ENABLE(gtk2, 
              [  --disable-gtk2	  Do not look for GTK+ 2.0 libraries], 
              enable_gtk2=$enableval, 
              enable_gtk2="yes")

AC_ARG_ENABLE(gtk3,
              [  --enable-gtk3          Enable GTK+ 3.0 (Experimental)],
              enable_gtk3=$enableval,
              enable_gtk3="no")

AC_ARG_ENABLE(gtkport, 
              [  --disable-gtkport	  Disable compiling the GTK+ port], 
              enable_gtkport=$enableval, 
              enable_gtkport="yes")

AC_ARG_ENABLE(textport, 
              [  --disable-textport	  Disable compiling the text port], 
              enable_textport=$enableval, 
              enable_textport="yes")
AC_ARG_ENABLE(ssl, 
              [  --disable-ssl		  Disable SSL support], 
              enable_ssl=$enableval, 
              enable_ssl="yes")

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(PREFIX)

AC_CANONICAL_HOST
AC_PROG_CC
AC_SYS_LARGEFILE
AC_C_CONST
AC_C_INLINE

AM_MAINTAINER_MODE

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h libutil.h limits.h malloc.h pty.h strings.h sys/ioctl.h sys/time.h unistd.h stdint.h sys/mkdev.h inttypes.h)

AC_TYPE_MODE_T
AC_TYPE_INTPTR_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_SIZEOF(off_t)

AC_FUNC_ALLOCA
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS(gai_strerror getaddrinfo getcwd gettimeofday getwd mkdir mktime putenv rmdir select socket strdup strstr strtod strtol uname grantpt openpty getdtablesize)

EXTRA_LIBS=""

# FreeBSD needs this
AC_CHECK_LIB(util, openpty, [EXTRA_LIBS="-lutil"
                             AC_DEFINE(HAVE_OPENPTY, [1], [Define if you have the openpty function.]) ])

AC_CHECK_LIB(socket, socket, EXTRA_LIBS="-lsocket")
  
AC_CHECK_LIB(nsl, gethostbyname, EXTRA_LIBS="$EXTRA_LIBS -lnsl")

GFTP_TEXT=""
USE_READLINE="yes"
READLINE_LIBS=""
if test "x$enable_textport" = "xyes" ; then
  GFTP_TEXT="gftp-text"

  AC_MSG_CHECKING([whether to use readline])
  AC_ARG_WITH(readline,
    [  --with-readline         turn on readline [default=yes]],
    [case "${withval}" in
      yes) enable_readline="yes" ; AC_MSG_RESULT([yes]) ;;
      no)  enable_readline="no"  ; AC_MSG_RESULT([no])  ;;
      *)   enable_readline="yes" ; AC_MSG_RESULT([yes]) ;;
    esac],[AC_MSG_RESULT([${USE_READLINE}])])

  AS_IF([ test x"$enable_readline" != xno ],[
    unset ac_cv_lib_readline_readline
    AC_CHECK_HEADERS([readline/readline.h],[
      sLIBS=$LIBS
      LIBS=""
      AC_SEARCH_LIBS(tgetent, termcap curses ncurses ncursesw, [], [])
      AC_SEARCH_LIBS(readline,[readline edit], [
        AC_DEFINE([HAVE_LIBREADLINE],1,Define if the text port uses readline)
        READLINE_LIBS=$LIBS
      ])
      LIBS=$sLIBS
    ])
  ])
fi

AC_CHECK_LIB(m, log10, EXTRA_LIBS="$EXTRA_LIBS -lm")

AM_WITH_DMALLOC

AC_SUBST(EXTRA_LIBS)
AC_SUBST(READLINE_LIBS)
AC_SUBST(GFTP_TEXT)

GFTP_GTK=""
PTHREAD_CFLAGS=""
PTHREAD_LIBS=""
GTHREAD_CFLAGS=""
GTHREAD_LIBS=""

PKG_CHECK_MODULES(
  [GLIB], [glib-2.0 >= 2.32.0], found_glib=yes,
  AC_MSG_ERROR(Glib >= 2.32.0+ is required)
  )

if test "x$enable_gtkport" = "xyes" ; then

  if test "x$enable_gtk2" = "xno" ; then
    if test "x$enable_gtk3" = "xno" ; then
      echo "Error: gtk2 and gtk3 are disabled. Not GTK port." ; 
      exit 1
    fi
  fi
  if test "x$enable_gtk2" = "xyes" ; then
    if test "x$enable_gtk3" = "xyes" ; then
      enable_gtk2=no
    fi
  fi
  if test "x$enable_gtk2" = "xyes" ; then
    dnl Test for gtk+-2.0
    PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.24.0], GFTP_GTK=gftp-gtk, AC_MSG_ERROR(You have GLIB 2.0 installed but I cannot find GTK+ 2.0. Run configure with --disable-gtk2 or install GTK+ 2.0))
    PKG_CHECK_MODULES([GTHREAD], gthread-2.0 >=  2.32.0, HAVE_GTHREADS="yes")
  fi
  if test "x$enable_gtk3" = "xyes" ; then
    dnl Test for gtk+-3.0
    PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0], GFTP_GTK=gftp-gtk, AC_MSG_ERROR(You have GLIB 2.0 installed but I cannot find GTK+ 3.0. Run configure without --enable-gtk3 or install GTK+ 3.0))
  fi

  if test "x$GFTP_GTK" = xgftp-gtk; then
    # see https://chromium.googlesource.com/chromiumos/third_party/cairo/+/master/build/configure.ac.pthread
    AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIBS="-lpthread")
    if test "x$PTHREAD_LIBS" = x ; then
      AC_CHECK_LIB(pthreads, pthread_create, PTHREAD_LIBS="-lpthreads")
    fi
    if test "x$PTHREAD_LIBS" = x ; then
      AC_CHECK_LIB(c_r, pthread_create, PTHREAD_LIBS="-lc_r")
    fi
    if test "x$PTHREAD_LIBS" = x ; then
      echo "Error: Cannot find the pthread libraries." ; 
      exit 1
    fi
    PTHREAD_CFLAGS="-D_REENTRANT"
  fi
fi
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GFTP_GTK)

SSL_LIBS=""
if test "x$enable_ssl" = "xyes" ; then
	AC_CHECK_HEADERS(openssl/ssl.h ssl.h)
	if test $ac_cv_header_openssl_ssl_h = yes -o $ac_cv_header_ssl_h = yes ; then
		AC_CHECK_LIB(ssl, SSL_library_init, SSL_LIBS="-lcrypto -lssl")

		if test "x$SSL_LIBS" != "x" ; then
			AC_DEFINE(USE_SSL, 1, 
                                  [define if you want to enable SSL support])
		fi
	fi
fi
AC_SUBST(SSL_LIBS)

GETTEXT_PACKAGE=gftp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AM_GLIB_GNU_GETTEXT

case "$CC" in
  gcc*) CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers" ;;
  *) ;;
esac

AC_CHECK_PROG(DB2HTML, db2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK, $DB2HTML)

AC_CONFIG_FILES(
	Makefile
	docs/Makefile
	docs/sample.gftp/Makefile
	lib/Makefile
	src/gftp
	src/Makefile
	src/uicommon/Makefile
	src/gtk/Makefile
	src/text/Makefile
	po/Makefile.in
	gftp.spec
)
AC_OUTPUT
