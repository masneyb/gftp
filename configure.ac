dnl Process this file with autoconf to produce a configure script.

AC_INIT(lib/gftp.h)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(gftp,2.0.19)

AC_GNU_SOURCE
AC_USE_SYSTEM_EXTENSIONS

AC_ARG_ENABLE(gtk20, 
              [  --disable-gtk20	  Don't look for GTK+ 2.0 libraries], 
              enable_gtk20=$enableval, 
              enable_gtk20="yes")

AC_ARG_ENABLE(gtk30,
              [  --enable-gtk30          Enable GTK+ 3.0 (Experimental)],
              enable_gtk30=$enableval,
              enable_gtk30="no")

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
AC_EXEEXT
AC_PROG_RANLIB
AC_SYS_LARGEFILE
dnl AC_ISC_POSIX
AC_C_CONST
AC_C_INLINE
AC_INTL_PRINTF

AM_MAINTAINER_MODE

AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h libutil.h limits.h malloc.h pty.h strings.h sys/ioctl.h sys/time.h unistd.h stdint.h sys/mkdev.h inttypes.h)

dnl AM_TYPE_PTRDIFF_T
AC_TYPE_SOCKLEN_T
AC_TYPE_MODE_T
AC_TYPE_INTPTR_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_SIZEOF(off_t)

AC_FUNC_ALLOCA
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_FUNC_UTIME_NULL
AC_CHECK_FUNCS(gai_strerror getaddrinfo getcwd gettimeofday getwd mkdir mktime putenv rmdir select socket strdup strstr strtod strtol uname grantpt openpty getdtablesize)

# This is needed by fsplib. This check is from configure.ac in that distribution.
AC_CHECK_TYPE(union semun, ,AC_DEFINE(_SEM_SEMUN_UNDEFINED,1,[Define if you do not have semun in sys/sem.h]),
[#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
])

if test "x$GFTP_TEXT" = "x" ; then
  PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.0.0, found_glib20=1, found_glib20=0)
  if test "x$found_glib20" = "x0" ; then
    AC_CHECK_LIB(glib, g_main_iterate, HAVE_GLIB=1)
    echo "Found a version of glib 1.x gFTP needs GLIB 1.2.3 or higher"
  fi
fi

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
  [  --with-readline         turn on readline [default=yes, default tries -lncurses, -lcurses, -ltermcap]],
    [case "${withval}" in
      yes) USE_READLINE="yes" ; AC_MSG_RESULT([yes]) ;;
      no) USE_READLINE="no" ; AC_MSG_RESULT([no]) ;;
      only) USE_READLINE="yes"
            EXTRA_LIBREADLINE_DEPS=" "
            AC_MSG_RESULT([yes (using only readline)]) ;;
      *) USE_READLINE="yes"
         EXTRA_LIBREADLINE_DEPS="${withval}"
         AC_MSG_RESULT([yes (using extra libraries ${withval})]) ;;
    esac],[AC_MSG_RESULT([${USE_READLINE}])])

  dnl Checks for libraries.
  dnl When checking readline, check using extra libraries first.
  dnl We want to protect against the link somehow succeeding, but only
  dnl failing at runtime, as seems to happen on some BSD systems.
  if test "$USE_READLINE" = yes ; then
    if test "$EXTRA_LIBREADLINE_DEPS" = "" ; then
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                   HAVE_LIBREADLINE=false, -lncurses)
      if test x${HAVE_LIBREADLINE} = xtrue ; then
        echo "  Using -lreadline -lncurses"
        EXTRA_LIBREADLINE_DEPS=-lncurses
        AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
      else
        unset ac_cv_lib_readline_readline
        AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                     HAVE_LIBREADLINE=false, -lcurses)
        if test x${HAVE_LIBREADLINE} = xtrue ; then
          echo "  Using -lreadline -lcurses"
          EXTRA_LIBREADLINE_DEPS=-lcurses
          AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
        else
          unset ac_cv_lib_readline_readline
          AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                       HAVE_LIBREADLINE=false, -ltermcap)
          if test x${HAVE_LIBREADLINE} = xtrue ; then
            echo "  Using -lreadline -ltermcap"
            EXTRA_LIBREADLINE_DEPS=-ltermcap
            AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
          else
            unset ac_cv_lib_readline_readline
            AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                         HAVE_LIBREADLINE=false)
            if test x${HAVE_LIBREADLINE} = xtrue ; then
              AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
            else
              AC_DEFINE(HAVE_LIBREADLINE, [0], [Define if the text port uses readline])
            fi
          fi
        fi
      fi
    else
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
                   HAVE_LIBREADLINE=false, ${EXTRA_LIBREADLINE_DEPS})
      if test x${HAVE_LIBREADLINE} = xtrue ; then
        echo "  Using -lreadline ${EXTRA_LIBREADLINE_DEPS}"
        AC_DEFINE(HAVE_LIBREADLINE, [1], [Define if the text port uses readline])
      else
        AC_DEFINE(HAVE_LIBREADLINE, [0], [Define if the text port uses readline])
      fi
    fi
  else
    HAVE_LIBREADLINE=false
    AC_DEFINE(HAVE_LIBREADLINE, [0], [Define if the text port uses readline])
  fi
  if test x${HAVE_LIBREADLINE} = xtrue; then
    READLINE_LIBS="-lreadline ${EXTRA_LIBREADLINE_DEPS}"
  fi
fi

AC_CHECK_LIB(m, log10, EXTRA_LIBS="$EXTRA_LIBS -lm")

AM_WITH_DMALLOC

AC_SUBST(EXTRA_LIBS)
AC_SUBST(READLINE_LIBS)
AC_SUBST(GFTP_TEXT)

GFTP_GTK=""
PTHREAD_CFLAGS=""
PTHREAD_LIBS=""

# I don't have PKG_CHECK_MODULES or AM_PATH_GLIB check for gthread because
# the text port doesn't need to be compiled against the thread libraries
GTHREAD_LIBS=""

if test "x$enable_gtkport" = "xyes" ; then

  if test "x$GFTP_GTK" = "x" ; then
    PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.0.0], found_glib20=1, found_glib20=0)

    PKG_CHECK_MODULES(GTHREADS, gthread-2.0 >=  2.2.0, HAVE_GTHREADS="yes")
    AC_SUBST(GTHREADS_CFLAGS)
    AC_SUBST(GTHREADS_LIBS)
    if test "x$HAVE_GTHREADS" = xyes ; then
      GTHREAD_LIBS="-lgthread-2.0"
    fi

    if test "x$enable_gtk20" = "xyes" ; then
      if test "x$enable_gtk30" = "xyes" ; then
        AC_MSG_ERROR(GTK 2.0 configuration cannot be built while experimental GTK 3.0 support is enabled)
      fi 
      dnl Test for gtk+-2.0
      PKG_CHECK_MODULES([GTK], [gtk+-2.0 >= 2.0.0], GFTP_GTK=gftp-gtk, AC_MSG_ERROR(You have GLIB 2.0 installed but I cannot find GTK+ 2.0. Run configure with --disable-gtk20 or install GTK+ 2.0))
    fi
    if test "x$enable_gtk30" = "xyes" ; then
      if test "x$enable_gtk20" = "xyes" ; then 
        AC_MSG_ERROR(GTK 3.0 experimental configuration cannot be built while GTK 2.0 support is enabled)
      fi
      dnl Test for gtk+-3.0
      PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0], GFTP_GTK=gftp-gtk, AC_MSG_ERROR(You have GLIB 2.0 installed but I cannot find GTK+ 3.0. Run configure without --enable-gtk30 or install GTK+ 3.0))
    fi
  else
    PKG_CHECK_MODULES([GLIB], [glib-1.0 >= 1.0.0])
    if test "x$HAVE_GTHREADS" != xyes ; then
      AC_CHECK_LIB(gthread, g_thread_init, GTHREAD_LIBS="-lgthread")
    fi

  fi

  if test "x$GFTP_GTK" = xgftp-gtk; then
    case $host in
      *-hpux*)
        AC_CHECK_LIB(rt, sem_init)

        # From H.Merijn Brand <h.m.brand@hccnet.nl>
        echo '#define pthread_attr_init(x) __pthread_attr_init_system(x)' >>confdefs.h
        echo '#define pthread_create(x) __pthread_create_system(x)' >>confdefs.h
        ;;
    esac

    AC_CHECK_LIB(pthread, pthread_create, PTHREAD_LIBS="-lpthread")

    if test "x$PTHREAD_LIBS" = x ; then
      AC_CHECK_LIB(pthreads, pthread_create, PTHREAD_LIBS="-lpthreads")
    fi

    if test "x$PTHREAD_LIBS" = x ; then
      AC_CHECK_LIB(c_r, pthread_create, PTHREAD_LIBS="-lc_r")
    fi

    if test "x$PTHREAD_LIBS" = x ; then
      echo ;
      echo "Error: Cannot find the pthread libraries. If you don't have them installed," ; 
      echo "your X libraries probably aren't thread safe either. You can download the" ; 
      echo "pthread library and the thread safe X libraries from my webpage at" ;
      echo "http://www.gftp.org/. Note the pthread library and the thread safe" ;
      echo "X libraries on my webpage are for Linux libc5 machines ONLY" ;
      echo ;
      exit
    fi

    PTHREAD_CFLAGS="-D_REENTRANT"

    # I took some of these from the glib 1.3.3 configure.in
    case $host in
      *-aix*)
        PTHREAD_CFLAGS="$PTHREAD_CFLAGS -D_THREAD_SAFE"
        if test x"$GCC" = xyes; then
          PTHREAD_CFLAGS="$PTHREAD_CFLAGS -mthreads"
        fi
        ;;
      *-freebsd*)
        PTHREAD_CFLAGS="$PTHREAD_CFLAGS -D_THREAD_SAFE"

        # FreeBSD 2.2.x shiped with gcc 2.7.2.x, which doesn't support
        # -mthreads flag.
        ;;
      *-openbsd*)
        PTHREAD_CFLAGS="$PTHREAD_CFLAGS -D_THREAD_SAFE"
          if test "x$PTHREAD_LIBS" = "x-lc_r"; then
            PTHREAD_LIBS="-pthread"
          fi
        ;;
      *-sysv5uw7*)  # UnixWare 7
        if test "$GCC" != "yes"; then
          PTHREAD_CFLAGS="$PTHREAD_CFLAGS -Kthread"
        else
          PTHREAD_CFLAGS="$PTHREAD_CFLAGS -pthread"
        fi
        ;;
      *-dg-dgux*)  # DG/UX
        PTHREAD_CFLAGS="$PTHREAD_CFLAGS -D_POSIX4A_DRAFT10_SOURCE"
    esac
  fi
fi
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(GTHREAD_LIBS)
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

AM_GNU_GETTEXT

AC_CHECK_PROG(DB2HTML, db2html, true, false)
AM_CONDITIONAL(HAVE_DOCBOOK, $DB2HTML)

<<<<<<< HEAD:configure.ac
AC_CONFIG_FILES(po/Makefile.in 
	intl/Makefile
=======
AC_CONFIG_FILES(
>>>>>>> 06f54cef98bf54bc4917566fe6b3c403e8330402:configure.ac
	Makefile
	docs/Makefile
	docs/sample.gftp/Makefile
	lib/Makefile
	lib/fsplib/Makefile
	src/gftp
	src/Makefile
	src/uicommon/Makefile
	src/gtk/Makefile
	src/text/Makefile
	gftp.spec
)
AC_OUTPUT
