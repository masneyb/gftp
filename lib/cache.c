/*****************************************************************************/
/*  cache.c - contains the cache routines                                    */
/*  Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>                  */
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
static const char cvsid[] = "$Id$";

char *
gftp_cache_get_url_prefix (gftp_request * request)
{
  if (strcmp (request->protocol_name, "HTTP") == 0 &&
      strcmp (request->proxy_config, "ftp") == 0)
    return ("ftp");

  return (request->url_prefix);
}


int
gftp_new_cache_entry (gftp_request * request)
{
  char *cachedir, *tempstr, *temp1str;
  int cache_fd, fd;
  ssize_t ret;

  cachedir = expand_path (BASE_CONF_DIR "/cache");
  if (access (cachedir, F_OK) == -1)
    {
      if (mkdir (cachedir, S_IRUSR | S_IWUSR | S_IXUSR) < 0)
        {
          if (request != NULL)
            request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: Could not make directory %s: %s\n"),
                                 cachedir, g_strerror (errno));

          return (-1);
        }
    }

  tempstr = g_strdup_printf ("%s/index.db", cachedir);
  if ((fd = open (tempstr, O_WRONLY | O_APPEND | O_CREAT, 
                  S_IRUSR | S_IWUSR)) == -1)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error: Cannot open local file %s: %s\n"),
                                   tempstr, g_strerror (errno));

      g_free (tempstr);
      g_free (cachedir);
      return (-1);
    }
  g_free (tempstr);

  tempstr = g_strdup_printf ("%s/cache.XXXXXX", cachedir);
  if ((cache_fd = mkstemp (tempstr)) < 0)
    {
      g_free (tempstr);
      if (request != NULL)
        request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: Cannot create temporary file: %s\n"),
                                 g_strerror (errno));
      return (-1);
    }
  g_free (cachedir);

  lseek (fd, 0, SEEK_END);
  temp1str = g_strdup_printf ("%s://%s@%s:%d%s\t%s\n", 
                           gftp_cache_get_url_prefix (request),
                           request->username == NULL ? "" : request->username,
                           request->hostname == NULL ? "" : request->hostname,
                           request->port, 
                           request->directory == NULL ? "" : request->directory,
                           tempstr);
  g_free (tempstr);
  ret = gftp_write (NULL, temp1str, strlen (temp1str), fd);
  g_free (temp1str);

  if (close (fd) != 0 || ret < 0)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));

      close (cache_fd);
      return (-1);
    }

  return (cache_fd);
}


int
gftp_find_cache_entry (gftp_request * request)
{
  char *indexfile, *pos, buf[BUFSIZ], description[BUFSIZ];
  gftp_getline_buffer * rbuf;
  int indexfd, cachefd;
  size_t len;

  g_snprintf (description, sizeof (description), "%s://%s@%s:%d%s",
              gftp_cache_get_url_prefix (request),
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              request->directory == NULL ? "" : request->directory);

  indexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = open (indexfile, O_RDONLY)) == -1)
    {
      g_free (indexfile);
      return (-1);
    }
  g_free (indexfile);

  rbuf = NULL;
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf), indexfd) > 0)
    {
      len = strlen (buf);

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
	continue;

      len = strlen (description);
      if (pos - buf != len)
        continue;

      if (strncmp (buf, description, len) == 0)
	{
	  pos++;
	  if (close (indexfd) != 0)
            {
              if (request != NULL)
                request->logging_function (gftp_logging_error, 
                                       request->user_data,
                                       _("Error closing file descriptor: %s\n"),
                                       g_strerror (errno));
              return (-1);
            }

	  if ((cachefd = open (pos, O_RDONLY)) == -1)
            {
              if (request != NULL)
                request->logging_function (gftp_logging_error, 
                                   request->user_data,
                                   _("Error: Cannot open local file %s: %s\n"),
                                   pos, g_strerror (errno));
              return (-1);
            }

          if (lseek (cachefd, 0, SEEK_END) == 0)
            { 
              close (cachefd); 
              return (-1);
            } 

          if (lseek (cachefd, 0, SEEK_SET) == -1)
            {
              if (request != NULL)
                request->logging_function (gftp_logging_error, 
                                       request->user_data,
                                       _("Error: Cannot seek on file %s: %s\n"),
                                       pos, g_strerror (errno));

            }

	  return (cachefd);
	}
    }
  close (indexfd);
  return (-1);
}


void
gftp_clear_cache_files (void)
{
  char *indexfile, buf[BUFSIZ], *pos;
  gftp_getline_buffer * rbuf;
  int indexfd;
  size_t len;

  indexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = open (indexfile, O_RDONLY)) == -1)
    {
      g_free (indexfile);
      return;
    }

  rbuf = NULL;
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf), indexfd) > 0)
    {
      len = strlen (buf);

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
	continue;
      unlink (pos + 1);
    }

  close (indexfd);
  unlink (indexfile);
  g_free (indexfile);
}


void
gftp_delete_cache_entry (gftp_request * request, int ignore_directory)
{
  char *oldindexfile, *newindexfile, *pos, buf[BUFSIZ], description[BUFSIZ];
  gftp_getline_buffer * rbuf;
  int indexfd, newfd;
  size_t len, buflen;
  int remove;

  g_snprintf (description, sizeof (description), "%s://%s@%s:%d%s",
              gftp_cache_get_url_prefix (request),
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              ignore_directory || request->directory == NULL ? "" : request->directory);

  oldindexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = open (oldindexfile, O_RDONLY)) == -1)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error: Cannot open local file %s: %s\n"),
                                   oldindexfile, g_strerror (errno));

      g_free (oldindexfile);
      return;
    }

  newindexfile = expand_path (BASE_CONF_DIR "/cache/index.db.new");
  if ((newfd = open (newindexfile, O_WRONLY | O_CREAT, 
                     S_IRUSR | S_IWUSR)) == -1)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error: Cannot open local file %s: %s\n"),
                                   newindexfile, g_strerror (errno));

      g_free (oldindexfile);
      g_free (newindexfile);
      return;
    }

  rbuf = NULL;
  buflen = strlen (description);
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf), indexfd) > 0)
    {
      len = strlen (buf);

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
        {
          if (request != NULL)
            request->logging_function (gftp_logging_error, request->user_data,
                            _("Error: Invalid line %s in cache index file\n"), 
                            buf);

          continue;
        }

      remove = 0;
      if (ignore_directory)
        {
          if (strncmp (buf, description, strlen (description)) == 0)
            remove = 1;
        }
      else
        {
          if (buflen == pos - buf && strncmp (buf, description, pos - buf) == 0)
            remove = 1;
        }

 
      if (remove)
        unlink (pos + 1);
      else
        {
          buf[strlen (buf)] = '\n';
          if (gftp_write (NULL, buf, strlen (buf), newfd) < 0)
            break;
        }
    }

  close (indexfd);
  close (newfd);

  unlink (oldindexfile);
  rename (newindexfile, oldindexfile);

  g_free (oldindexfile);
  g_free (newindexfile);
}

