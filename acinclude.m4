# serial 1

dnl This function is derived from
dnl http://savannah.gnu.org/cgi-bin/viewcvs/gcc/gcc/libjava/configure.in?rev=1.142.2.7&content-type=text/vnd.viewcvs-markup

AC_DEFUN([AC_TYPE_SOCKLEN_T],
  [AC_MSG_CHECKING([for socklen_t in sys/socket.h])
   AC_TRY_COMPILE([#define _POSIX_PII_SOCKET
   #include <sys/types.h>
   #include <sys/socket.h>], [socklen_t x = 5;],
     [AC_MSG_RESULT(yes)],
     [AC_DEFINE(socklen_t,int,Need to define socklen_t as an int because it should be in sys/socket.h)
      AC_MSG_RESULT(no)])
  ])

# serial 1

dnl This macro checks to see if the printf family of functions supports the
dnl %'ld format.

dnl Brian Masney <masneyb@gftp.org>

dnl 
AC_DEFUN([AC_INTL_PRINTF],
  [AC_MSG_CHECKING([whether the printf family of functions supports %'ld])
   AC_TRY_RUN([#include <stdio.h>
               #include <string.h>

               int main(void) {
                 char buf[20];
#if defined (_LARGEFILE_SOURCE)
                 sprintf (buf, "%'lld", (long) 1);
#else
                 sprintf (buf, "%'ld", (long) 1);
#endif
                 if (strchr (buf, '1') == NULL)
                   return (1);
                 return (0);
               }],
    [AC_DEFINE(HAVE_INTL_PRINTF,1,Define if printf supports %'ld)
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])
  ])

# serial 1

dnl Brian Masney <masneyb@gftp.org>

AC_DEFUN([AC_TYPE_INTPTR_T],
  [AC_MSG_CHECKING([for intptr_t in stdint.h])
   AC_TRY_COMPILE([#include <stdint.h>],
   [intptr_t i = 0;],
     [AC_MSG_RESULT(yes)],
     [AC_DEFINE(intptr_t,long,Need to define intptr_t as a long because it should be in stdint.h)
      AC_MSG_RESULT(no)])
  ])

