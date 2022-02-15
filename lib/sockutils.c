/***********************************************************************************/
/*  sockutils.c - various utilities for dealing with sockets                       */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                        */
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

ssize_t
gftp_get_line (gftp_request * request, gftp_getline_buffer ** rbuf, 
               char * str, size_t len, int fd)
{
  //DEBUG_TRACE("%s [gftp_get_line] rbuf: %p str:%s\n" , __FILE__, *rbuf, str)
  ssize_t (*read_function) (gftp_request * request, void *ptr, size_t size,
                            int fd);
  char *pos, *nextpos;
  size_t rlen, nslen;
  int end_of_buffer;
  ssize_t ret;
  gftp_getline_buffer * rrbuf; // = *rbuf (avoid too many (*rbuf)->)

  if (request == NULL || request->read_function == NULL) {
      read_function = gftp_fd_read;
  } else {
      read_function = request->read_function;
  }

  if (*rbuf == NULL)
  {
      // initialize buffer and default values for the struct
      *rbuf = g_malloc0 (sizeof (**rbuf));
      rrbuf = *rbuf;
      rrbuf->max_bufsize = len;
      rrbuf->buffer  = g_malloc0 ((gsize) (rrbuf->max_bufsize + 1));

      ret = read_function (request, rrbuf->buffer, rrbuf->max_bufsize, fd);
      if (ret <= 0) {
          // nothing was retrieved from the socket
          gftp_free_getline_buffer (rbuf);
          return (ret);
      }
      rrbuf->buffer[ret] = '\0';
      rrbuf->cur_bufsize = ret;
      rrbuf->curpos      = rrbuf->buffer;
  }
  else
  {
     // buffer already created, use a variable that is more readable
     rrbuf = *rbuf;
  }

  //g_return_val_if_fail (rrbuf->curpos != NULL, GFTP_EFATAL);

  ret = 0;
  while (1)
  {
      pos = strchr (rrbuf->curpos, '\n');
      end_of_buffer = 0;
      if (rrbuf->curpos == rrbuf->buffer && 
          (rrbuf->max_bufsize == rrbuf->cur_bufsize || rrbuf->eof))
      {
         end_of_buffer = 1;
      }

      if (rrbuf->cur_bufsize > 0 && (pos != NULL || end_of_buffer))
      {
          if (pos != NULL)
          {
              nslen = pos - rrbuf->curpos + 1;
              nextpos = pos + 1;
              if (pos > rrbuf->curpos && *(pos - 1) == '\r')
                pos--;
              *pos = '\0';
          }
          else
          {
              nslen = rrbuf->cur_bufsize;
              nextpos = NULL;
              /* This is not an overflow since we allocated one extra byte to
                 buffer above */
              (rrbuf->buffer)[nslen] = '\0';
          }

          strncpy (str, rrbuf->curpos, len);
          str[len - 1] = '\0';
          rrbuf->cur_bufsize -= nslen;

          if (nextpos != NULL)
            rrbuf->curpos = nextpos;
          else
            rrbuf->cur_bufsize = 0;

          ret = nslen;
          break;
      }
      else
      {
          if (rrbuf->cur_bufsize == 0 || *rrbuf->curpos == '\0')
          {
              rlen = rrbuf->max_bufsize;
              pos = rrbuf->buffer;
          }
          else
          {
              memmove (rrbuf->buffer, rrbuf->curpos, rrbuf->cur_bufsize);
              pos = rrbuf->buffer + rrbuf->cur_bufsize;
              rlen = rrbuf->max_bufsize - rrbuf->cur_bufsize;
          }

          rrbuf->curpos = rrbuf->buffer;

          if (rrbuf->eof)
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
              if (rrbuf->cur_bufsize == 0)
              {
                  gftp_free_getline_buffer (rbuf);
                  return (ret);
              }

              rrbuf->eof = 1;
          }

          rrbuf->cur_bufsize += ret;
          rrbuf->buffer[rrbuf->cur_bufsize] = '\0';
        }
    }

    return (ret);
}


void gftp_free_getline_buffer (gftp_getline_buffer ** rbuf)
{
  DEBUG_PRINT_FUNC
  if ((*rbuf)->buffer) {
     g_free ((*rbuf)->buffer);
  }
  if (*rbuf) {
     g_free (*rbuf);
  }
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


int gftp_fd_get_sockblocking (gftp_request * request, int fd)
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

  return (flags & O_NONBLOCK) != 0;
}


int gftp_fd_set_sockblocking (gftp_request * request, int fd, int non_blocking)
{
#ifndef __APPLE__
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
#endif
  return (0);
}


struct servent *
r_getservbyname (const char *name, const char *proto,
                 struct servent *result_buf, int *h_errnop)
{
  static WGMutex servfunclock;
  struct servent *sent;

  Wg_mutex_lock (&servfunclock);

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

  Wg_mutex_unlock (&servfunclock);
  return (sent);
}

