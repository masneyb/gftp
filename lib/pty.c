/*****************************************************************************/
/*  pty.c - general purpose routines                                         */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                  */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program; if not, write to the Free Software              */
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

static const char cvsid[] = "$Id$";

#include "gftp.h"


#ifdef __sgi

int
open_ptys (gftp_request * request, int *fdm, int *fds)
{
  char *pts_name;

  if ((pts_name = _getpty (fdm, O_RDWR, 0600, 0)) == NULL)
    return (GFTP_ERETRYABLE);

  if ((*fds = open (pts_name, O_RDWR)) < 0)
    {
      close (*fdm);
      return (GFTP_ERETRYABLE);
    }

  return (0);
}

#elif HAVE_GRANTPT

int
open_ptys (gftp_request * request, int *fdm, int *fds)
{
  char *pts_name;

  if ((*fdm = open ("/dev/ptmx", O_RDWR)) < 0)
    return (GFTP_ERETRYABLE);

  if (grantpt (*fdm) < 0)
    {
      close (*fdm);
      return (GFTP_ERETRYABLE);
    }

  if (unlockpt (*fdm) < 0)
    {
      close (*fdm);
      return (GFTP_ERETRYABLE);
    }

  if ((pts_name = ptsname (*fdm)) == NULL)
    {
      close (*fdm);
      return (GFTP_ERETRYABLE);
    }

  if ((*fds = open (pts_name, O_RDWR)) < 0)
    {
      close (*fdm);
      return (GFTP_ERETRYABLE);
    }

  /* I intentionally ignore these errors */
  ioctl (*fds, I_PUSH, "ptem");
  ioctl (*fds, I_PUSH, "ldterm");
  ioctl (*fds, I_PUSH, "ttcompat");

  return (0);
}

#elif HAVE_OPENPTY

int
open_ptys (gftp_request * request, int *fdm, int *fds)
{
  char *pts_name;

  if (openpty (fdm, fds, &pts_name, NULL, NULL ) < 0)
    return (GFTP_ERETRYABLE);

  ioctl (*fds, TIOCSCTTY, NULL);

  return (0);
}

#else /* !HAVE_OPENPTY */

/* Fall back to *BSD... */

int
open_ptys (gftp_request * request, int *fdm, int *fds)
{
  char pts_name[20], *pos1, *pos2;

  strncpy (pts_name, "/dev/ptyXY", sizeof (pts_name));
  for (pos1 = "pqrstuvwxyzPQRST"; *pos1 != '\0'; pos1++) 
    {
      pts_name[8] = *pos1;
      for (pos2 = "0123456789abcdef"; *pos2 != '\0'; pos2++)
        {
          pts_name[9] = *pos2;
          if ((*fdm = open (pts_name, O_RDWR)) < 0)
            continue;

          pts_name[5] = 't';
          chmod (pts_name, S_IRUSR | S_IWUSR);
          chown (pts_name, getuid (), -1);

          if ((*fds = open (pts_name, O_RDWR)) < 0)
            {
              pts_name[5] = 'p';
              continue;
            }

#if defined(TIOCSCTTY) && !defined(CIBAUD)
          ioctl (*fds, TIOCSCTTY, NULL);
#endif

          return (0);
        }
    }

  return (GFTP_ERETRYABLE);
}

#endif /* __sgi */


int
tty_raw (int fd)
{
  struct termios buf;

  if (tcgetattr (fd, &buf) < 0)
    return (-1);

  buf.c_iflag |= IGNPAR;
  buf.c_iflag &= ~(ICRNL | ISTRIP | IXON | IGNCR | IXANY | IXOFF | INLCR);
  buf.c_lflag &= ~(ECHO | ICANON | ISIG | ECHOE | ECHOK | ECHONL);
#ifdef IEXTEN
  buf.c_lflag &= ~(IEXTEN);
#endif

  buf.c_oflag &= ~(OPOST);
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;

  if (tcsetattr (fd, TCSADRAIN, &buf) < 0)
    return (-1);
  return (0);
}


