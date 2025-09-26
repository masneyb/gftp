/***********************************************************************************/
/*  pty.c - general purpose routines                                               */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                        */
/*                                                                                 */
/*  Permission is hereby granted, free of charge, to any person obtaining a copy   */
/*  of this software and associated documentation files (the "Software"), to deal  */
/*  in the Software without restriction, including without limitation the rights   */
/*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/*  copies of the Software, and to permit persons to whom the Software is          */
/*  furnished to do so, subject to the following conditions:                       */
/*                                                                                 */
/*  The above copyright notice and this permission notice shall be included in all */
/*  copies or substantial portions of the Software.                                */
/*                                                                                 */
/*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/*  SOFTWARE.                                                                      */
/***********************************************************************************/

#include "gftp.h"

#if HAVE_GRANTPT

char *
gftp_get_pty_impl (void)
{
  return ("unix98");
}


static int
_gftp_ptym_open (char *pts_name, size_t len, int *fds)
{
  char *new_pts_name;
  void (*savesig)(int);
  int fdm;

  if ((fdm = open ("/dev/ptmx", O_RDWR)) < 0)
    return (GFTP_ERETRYABLE);

  savesig = signal (SIGCHLD, SIG_DFL);
  if (grantpt (fdm) < 0)
    {
      signal(SIGCHLD, savesig);
      close (fdm);
      return (GFTP_ERETRYABLE);
    }
  signal (SIGCHLD, savesig);

  if (unlockpt (fdm) < 0)
    {
      close (fdm);
      return (GFTP_ERETRYABLE);
    }

  if ((new_pts_name = ptsname (fdm)) == NULL)
    {
      close (fdm);
      return (GFTP_ERETRYABLE);
    }

  strncpy (pts_name, new_pts_name, len);
  pts_name[len - 1] = '\0';

  return (fdm);
}


static int
_gftp_ptys_open (int fdm, int fds, char *pts_name)
{
  int new_fds;

  if ((new_fds = open (pts_name, O_RDWR)) < 0)
    {
      close (fdm);
      return (-1);
    }

  return (new_fds);
}

#elif HAVE_OPENPTY

#ifdef HAVE_PTY_H
#include <pty.h>
#include <utmp.h> /* for login_tty */
#elif HAVE_LIBUTIL_H
#include <libutil.h>
#include <utmp.h> /* for login_tty */
#endif

char *
gftp_get_pty_impl (void)
{
  return ("openpty");
}


static int
_gftp_ptym_open (char *pts_name, size_t len, int *fds)
{
  int fdm;

  if (openpty (&fdm, fds, pts_name, NULL, NULL) < 0)
    return (GFTP_ERETRYABLE);

  ioctl (*fds, TIOCSCTTY, NULL);

  return (fdm);
}


static int
_gftp_ptys_open (int fdm, int fds, char *pts_name)
{
  if (login_tty (fds) < 0)
    return (GFTP_EFATAL);

  return (fds);
}

#else /* Fall back to *BSD... */

char *
gftp_get_pty_impl (void)
{
  return ("bsd");
}


static int
_gftp_ptym_open (char *pts_name, size_t len, int *fds)
{
  char *pos1, *pos2;
  int fdm;

  g_return_val_if_fail (len >= 10, GFTP_EFATAL);

  strncpy (pts_name, "/dev/ptyXY", len);
  pts_name[len - 1] = '\0';

  for (pos1 = "pqrstuvwxyzPQRST"; *pos1 != '\0'; pos1++) 
    {
      pts_name[8] = *pos1;
      for (pos2 = "0123456789abcdef"; *pos2 != '\0'; pos2++)
        {
          pts_name[9] = *pos2;
          if ((fdm = open (pts_name, O_RDWR)) < 0)
            continue;

          pts_name[5] = 't';
          chmod (pts_name, S_IRUSR | S_IWUSR);
          chown (pts_name, getuid (), -1);

          return (fdm);
        }
    }

  return (GFTP_ERETRYABLE);
}


static int
_gftp_ptys_open (int fdm, int fds, char *pts_name)
{
  int new_fds;

  if ((new_fds = open (pts_name, O_RDWR)) < 0)
    {
      close (fdm);
      return (-1);
    }

#if defined(TIOCSCTTY) && !defined(CIBAUD)
  ioctl (new_fds, TIOCSCTTY, NULL);
#endif

  return (new_fds);
}

#endif /* HAVE_GRANTPT */


static int
_gftp_tty_raw (int fd)
{
  struct termios buf;

  if (tcgetattr (fd, &buf) < 0)
    return (-1);

  buf.c_iflag |= IGNPAR;
  buf.c_iflag &= ~(ICRNL | ISTRIP | IXON | IGNCR | IXANY | IXOFF | INLCR);
  buf.c_lflag &= ~(ECHO | ICANON | ISIG | ECHOE | ECHOK | ECHONL);
  buf.c_lflag &= ~(IEXTEN);

  buf.c_oflag &= ~(OPOST);
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;

  if (tcsetattr (fd, TCSADRAIN, &buf) < 0)
    return (-1);
  return (0);
}


static void
_gftp_close_all_fds (int ptysfd)
{
  int i, maxfds;

#ifdef HAVE_GETDTABLESIZE
  maxfds = getdtablesize () - 1;
#elif defined (OPEN_MAX)
  maxfds = OPEN_MAX;
#else
  maxfds = -1;
#endif

  for (i=3; i<maxfds; i++)
    {
      if (i == ptysfd)
        continue;

      close (i);
    }
}


pid_t
gftp_exec (gftp_request * request, int *fdm, int *ptymfd, char **args)
{
  char pts_name[64];
  pid_t child;
  int ptysfd;
  int s[2];

  *pts_name = '\0';
  if ((*ptymfd = _gftp_ptym_open (pts_name, sizeof (pts_name), &ptysfd)) < 0)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                _("Cannot open master pty %s: %s\n"), pts_name,
                                g_strerror (errno));
      return (-1);
    }

  if (socketpair (AF_LOCAL, SOCK_STREAM, 0, s) < 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot create a socket pair: %s\n"),
                                 g_strerror (errno));
      return (-1);
    }

  if ((child = fork ()) == 0)
    {
      setsid ();

      if ((ptysfd = _gftp_ptys_open (*ptymfd, ptysfd, pts_name)) < 0)
        {
          printf ("Cannot open slave pts %s: %s\n", pts_name,
                  g_strerror (errno));
          return (-1);
        }

      close (s[0]);
      close (*ptymfd);

      _gftp_tty_raw (s[1]);
      _gftp_tty_raw (ptysfd);

      dup2 (s[1], 0);
      dup2 (s[1], 1);
      dup2 (ptysfd, 2);
      _gftp_close_all_fds (ptysfd);

      execvp (args[0], args);

      printf (_("Error: Cannot execute ssh: %s\n"), g_strerror (errno));
      _exit (1);
    }
  else if (child > 0)
    {
      close (s[1]);

      *fdm = s[0];
      _gftp_tty_raw (s[0]);
      _gftp_tty_raw (*ptymfd);

      return (child);
    }
  else
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fork another process: %s\n"),
                                 g_strerror (errno));
      return (-1);
    }
}

