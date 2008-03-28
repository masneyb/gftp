/*****************************************************************************/
/*  sockutils.c - various utilities for dealing with sockets                 */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                  */
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

#include "gftp.h"
static const char cvsid[] = "$Id: protocols.c 952 2008-01-24 23:31:26Z masneyb $";

ssize_t
gftp_get_line (gftp_request * request, gftp_getline_buffer ** rbuf, 
               char * str, size_t len, int fd)
{
  ssize_t (*read_function) (gftp_request * request, void *ptr, size_t size,
                            int fd);
  char *pos, *nextpos;
  size_t rlen, nslen;
  int end_of_buffer;
  ssize_t ret;

  if (request == NULL || request->read_function == NULL)
    read_function = gftp_fd_read;
  else
    read_function = request->read_function;

  if (*rbuf == NULL)
    {
      *rbuf = g_malloc0 (sizeof (**rbuf));
      (*rbuf)->max_bufsize = len;
      (*rbuf)->buffer = g_malloc0 ((gulong) ((*rbuf)->max_bufsize + 1));

      if ((ret = read_function (request, (*rbuf)->buffer, 
                                (*rbuf)->max_bufsize, fd)) <= 0)
        {
          gftp_free_getline_buffer (rbuf);
          return (ret);
        }
      (*rbuf)->buffer[ret] = '\0';
      (*rbuf)->cur_bufsize = ret;
      (*rbuf)->curpos = (*rbuf)->buffer;
    }

  ret = 0;
  while (1)
    {
      pos = strchr ((*rbuf)->curpos, '\n');
      end_of_buffer = (*rbuf)->curpos == (*rbuf)->buffer && 
            ((*rbuf)->max_bufsize == (*rbuf)->cur_bufsize || (*rbuf)->eof);

      if ((*rbuf)->cur_bufsize > 0 && (pos != NULL || end_of_buffer))
        {
          if (pos != NULL)
            {
              nslen = pos - (*rbuf)->curpos + 1;
              nextpos = pos + 1;
              if (pos > (*rbuf)->curpos && *(pos - 1) == '\r')
                pos--;
              *pos = '\0';
            }
          else
            {
              nslen = (*rbuf)->cur_bufsize;
              nextpos = NULL;

              /* This is not an overflow since we allocated one extra byte to
                 buffer above */
              ((*rbuf)->buffer)[nslen] = '\0';
            }

          strncpy (str, (*rbuf)->curpos, len);
          str[len - 1] = '\0';
          (*rbuf)->cur_bufsize -= nslen;

          if (nextpos != NULL)
            (*rbuf)->curpos = nextpos;
          else
            (*rbuf)->cur_bufsize = 0;

          ret = nslen;
          break;
        }
      else
        {
          if ((*rbuf)->cur_bufsize == 0 || *(*rbuf)->curpos == '\0')
            {
              rlen = (*rbuf)->max_bufsize;
              pos = (*rbuf)->buffer;
            }
          else
            {
              memmove ((*rbuf)->buffer, (*rbuf)->curpos, (*rbuf)->cur_bufsize);
              pos = (*rbuf)->buffer + (*rbuf)->cur_bufsize;
              rlen = (*rbuf)->max_bufsize - (*rbuf)->cur_bufsize;
            }

          (*rbuf)->curpos = (*rbuf)->buffer;

          if ((*rbuf)->eof)
            ret = 0;
          else
            {
              ret = read_function (request, pos, rlen, fd);
              if (ret < 0)
                {
                  gftp_free_getline_buffer (rbuf);
                  return (ret);
                }
            }

          if (ret == 0)
            {
              if ((*rbuf)->cur_bufsize == 0)
                {
                  gftp_free_getline_buffer (rbuf);
                  return (ret);
                }

              (*rbuf)->eof = 1;
            }

          (*rbuf)->cur_bufsize += ret;
          (*rbuf)->buffer[(*rbuf)->cur_bufsize] = '\0';
        }
    }

  return (ret);
}


void
gftp_free_getline_buffer (gftp_getline_buffer ** rbuf)
{
  g_free ((*rbuf)->buffer);
  g_free (*rbuf);
  *rbuf = NULL;
}


ssize_t 
gftp_fd_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  intptr_t network_timeout;
  struct timeval tv;
  fd_set fset;
  ssize_t ret;
  int s_ret;

  g_return_val_if_fail (fd >= 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "network_timeout", &network_timeout);  

  errno = 0;
  ret = 0;
  FD_ZERO (&fset);

  do
    {
      FD_SET (fd, &fset);
      tv.tv_sec = network_timeout;
      tv.tv_usec = 0;
      s_ret = select (fd + 1, &fset, NULL, NULL, &tv);
      if (s_ret == -1 && (errno == EINTR || errno == EAGAIN))
        {
          if (request != NULL && request->cancel)
            {
              gftp_disconnect (request);
              return (GFTP_ERETRYABLE);
            }

          continue;
        }
      else if (s_ret <= 0)
        {
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request,
                                         _("Connection to %s timed out\n"),
                                         request->hostname);
              gftp_disconnect (request);
            }

          return (GFTP_ERETRYABLE);
        }

      if ((ret = read (fd, ptr, size)) < 0)
        {
          if (errno == EINTR || errno == EAGAIN)
            {
              if (request != NULL && request->cancel)
                {
                  gftp_disconnect (request);
                  return (GFTP_ERETRYABLE);
                }

              continue;
            }
 
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request,
                                   _("Error: Could not read from socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }

          return (GFTP_ERETRYABLE);
        }

      break;
    }
  while (1);

  return (ret);
}


ssize_t 
gftp_fd_write (gftp_request * request, const char *ptr, size_t size, int fd)
{
  intptr_t network_timeout;
  struct timeval tv;
  int ret, s_ret;
  ssize_t w_ret;
  fd_set fset;

  g_return_val_if_fail (fd >= 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "network_timeout", &network_timeout);  

  errno = 0;
  ret = 0;
  FD_ZERO (&fset);

  do
    {
      FD_SET (fd, &fset);
      tv.tv_sec = network_timeout;
      tv.tv_usec = 0;
      s_ret = select (fd + 1, NULL, &fset, NULL, &tv);
      if (s_ret == -1 && (errno == EINTR || errno == EAGAIN))
        {
          if (request != NULL && request->cancel)
            {
              gftp_disconnect (request);
              return (GFTP_ERETRYABLE);
            }

          continue;
        }
      else if (s_ret <= 0)
        {
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request,
                                         _("Connection to %s timed out\n"),
                                         request->hostname);
              gftp_disconnect (request);
            }

          return (GFTP_ERETRYABLE);
        }

      w_ret = write (fd, ptr, size);
      if (w_ret < 0)
        {
          if (errno == EINTR || errno == EAGAIN)
            {
              if (request != NULL && request->cancel)
                {
                  gftp_disconnect (request);
                  return (GFTP_ERETRYABLE);
                }

              continue;
             }
 
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request,
                                    _("Error: Could not write to socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }

          return (GFTP_ERETRYABLE);
        }

      ptr += w_ret;
      size -= w_ret;
      ret += w_ret;
    }
  while (size > 0);

  return (ret);
}


ssize_t 
gftp_writefmt (gftp_request * request, int fd, const char *fmt, ...)
{
  char *tempstr;
  va_list argp;
  ssize_t ret;

  va_start (argp, fmt);
  tempstr = g_strdup_vprintf (fmt, argp);
  va_end (argp);

  ret = request->write_function (request, tempstr, strlen (tempstr), fd);
  g_free (tempstr);
  return (ret);
}


int
gftp_fd_set_sockblocking (gftp_request * request, int fd, int non_blocking)
{
  int flags;

  g_return_val_if_fail (fd >= 0, GFTP_EFATAL);

  if ((flags = fcntl (fd, F_GETFL, 0)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot get socket flags: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (non_blocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  if (fcntl (fd, F_SETFL, flags) < 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot set socket to non-blocking: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (0);
}


struct servent *
r_getservbyname (const char *name, const char *proto,
                 struct servent *result_buf, int *h_errnop)
{
  static GStaticMutex servfunclock = G_STATIC_MUTEX_INIT;
  struct servent *sent;

  if (g_thread_supported ())
    g_static_mutex_lock (&servfunclock);

  if ((sent = getservbyname (name, proto)) == NULL)
    {
      if (h_errnop)
        *h_errnop = h_errno;
    }
  else
    {
      *result_buf = *sent;
      sent = result_buf;
    }

  if (g_thread_supported ())
    g_static_mutex_unlock (&servfunclock);
  return (sent);
}

