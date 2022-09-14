dnl Process this file with autoconf to produce a configure script.

AC_INIT([gftp],[2.9.1b],[https://www.gftp.org/])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_MACRO_DIR([autoconf-m4])
AM_INIT_AUTOMAKE([-Wall foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

m4_ifdef([AC_CONFIG_HEADERS], [AC_CONFIG_HEADERS([config.h])], [AC_CONFIG_HEADER([config.h])])
AM_GNU_GETTEXT([external])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_MAINTAINER_MODE], [AM_MAINTAINER_MODE])

AC_PROG_RANLIB
#L#T_INIT

AC_ARG_ENABLE(gtk2, 
              [  --disable-gtk2          Do not look for GTK+ 2.0 libraries], 
              enable_gtk2=$enableval, 
              enable_gtk2="yes")

AC_ARG_ENABLE(gtk3,
              [  --enable-gtk3           Enable GTK+ 3.0 (Experimental)],
              enable_gtk3=$enableval,
              enable_gtk3="no")

AC_ARG_ENABLE(gtkport, 
              [  --disable-gtkport       Disable compiling the GTK+ port], 
              enable_gtkport=$enableval, 
              enable_gtkport="yes")

AC_ARG_ENABLE(gtk_warnings,
              [  --enable-gtk-warnings   So you can see what is deprecated in GTK2...],
              gtk_warnings=$enableval,
              gtk_warnings='np')

AC_ARG_ENABLE(textport, 
              [  --disable-textport	  Disable compiling the text port], 
              enable_textport=$enableval, 
              enable_textport="yes")

AC_ARG_ENABLE(ssl, 
              [  --disable-ssl		  Disable SSL support], 
              enable_ssl=$enableval, 
              enable_ssl="yes")

AC_ARG_ENABLE(debug,
              [  --enable-debug      Print debug info to std output],
              enable_debug=$enableval,
              enable_debug="no")

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(PREFIX)

AC_CANONICAL_HOST
AC_PROG_CC

AC_CHECK_HEADERS(libutil.h malloc.h pty.h sys/ioctl.h sys/mkdev.h sys/sysmacros.h)
AC_CHECK_FUNCS(gettimeofday select socket grantpt openpty getdtablesize)

EXTRA_LIBS="-lm"

# FreeBSD needs this
AC_CHECK_LIB(util, openpty, EXTRA_LIBS="$EXTRA_LIBS -lutil")
AC_CHECK_LIB(socket, socket, EXTRA_LIBS="$EXTRA_LIBS -lsocket")
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
    PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.14.0], GFTP_GTK=gftp-gtk, AC_MSG_ERROR(You have GLIB 2.0 installed but I cannot find GTK+ 2.0. Run configure with --disable-gtk2 or install GTK+ 2.0))
    PKG_CHECK_MODULES([GTHREAD], gthread-2.0 >=  2.18.0, HAVE_GTHREADS="yes")
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

if test "x$enable_ssl" = "xyes" ; then
	PKG_CHECK_MODULES([SSL], [openssl >= 1.0], HAVE_SSL=yes, HAVE_SSL=no)
	if test "x$SSL_LIBS" != "x" ; then
		AC_DEFINE(USE_SSL, 1, [define if you want to enable SSL support])
	fi
fi
AC_SUBST(SSL_CFLAGS)
AC_SUBST(SSL_LIBS)

if test "x$enable_debug" = "xyes" ; then
	AC_DEFINE(GFTP_DEBUG, 1, [print debug info to std output])
fi

GETTEXT_PACKAGE=gftp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

case "$CC" in
  gcc*) CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers" ;;
  *) ;;
esac

if test x"$gtk_warnings" = x"yes"; then
   CPPFLAGS="$CPPFLAGS -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED -DGTK_DISABLE_SINGLE_INCLUDES"
fi

AC_CONFIG_FILES(
	Makefile
	docs/Makefile
	docs/sample.gftp/Makefile
	icons/Makefile
	lib/Makefile
	src/gftp
	src/Makefile
	src/uicommon/Makefile
	src/gtk/Makefile
	src/text/Makefile
	po/Makefile
)
AC_OUTPUT
