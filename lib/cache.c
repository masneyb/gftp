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


FILE *
gftp_new_cache_entry (gftp_request * request)
{
  char *cachedir, tempstr[BUFSIZ];
  int cache_fd;
  FILE *fd;

  if ((fd = gftp_find_cache_entry (request)) != NULL)
    return (fd);

  cachedir = expand_path (BASE_CONF_DIR "/cache");
  if (access (cachedir, F_OK) == -1)
    {
      if (mkdir (cachedir, 0x1C0) < 0)
        return (NULL);
    }

  g_snprintf (tempstr, sizeof (tempstr), "%s/index.db", cachedir);
  if ((fd = fopen (tempstr, "ab+")) == NULL)
    {
      g_free (cachedir);
      return (NULL);
    }

  g_snprintf (tempstr, sizeof (tempstr), "%s/cache.XXXXXX", cachedir);
  if ((cache_fd = mkstemp (tempstr)) < 0)
    return (NULL);
  g_free (cachedir);

  fseek (fd, 0, SEEK_END);
  fprintf (fd, "%s://%s@%s:%d%s\t%s\n", 
              gftp_cache_get_url_prefix (request),
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              request->directory == NULL ? "" : request->directory,
              tempstr);

  if (fclose (fd) != 0)
    {
      close (cache_fd);
      return (NULL);
    }

  if ((fd = fdopen (cache_fd, "wb+")) == NULL)
    {
      close (cache_fd);
      return (NULL);
    }

  return (fd);
}


FILE *
gftp_find_cache_entry (gftp_request * request)
{
  char *indexfile, *pos, buf[BUFSIZ], description[BUFSIZ];
  FILE *indexfd, *cachefd;
  size_t len;

  g_snprintf (description, sizeof (description), "%s://%s@%s:%d%s",
              gftp_cache_get_url_prefix (request),
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              request->directory == NULL ? "" : request->directory);

  indexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = fopen (indexfile, "rb")) == NULL)
    {
      g_free (indexfile);
      return (NULL);
    }
  g_free (indexfile);

  while (fgets (buf, sizeof (buf), indexfd))
    {
      len = strlen (buf);
      if (buf[len - 1] == '\n')
        buf[--len] = '\0';
      if (buf[len - 1] == '\r')
        buf[--len] = '\0';

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
	continue;

      len = strlen (description);
      if (pos - buf != len)
        continue;

      if (strncmp (buf, description, len) == 0)
	{
	  pos++;
	  if (fclose (indexfd) != 0)
            return (NULL);

	  if ((cachefd = fopen (pos, "rb+")) == NULL)
            return (NULL);

          fseek (cachefd, 0, SEEK_END);
          if (ftell (cachefd) == 0)
            { 
              fclose (cachefd); 
              return (NULL);
            } 
          fseek (cachefd, 0, SEEK_SET);
	  return (cachefd);
	}
    }
  fclose (indexfd);
  return (NULL);
}


void
gftp_clear_cache_files (void)
{
  char *indexfile, buf[BUFSIZ], *pos;
  FILE *indexfd;
  size_t len;

  indexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = fopen (indexfile, "rb")) == NULL)
    {
      g_free (indexfile);
      return;
    }

  while (fgets (buf, sizeof (buf), indexfd))
    {
      len = strlen (buf);
      if (buf[len - 1] == '\n')
        buf[--len] = '\0';
      if (buf[len - 1] == '\r')
        buf[--len] = '\0';

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
        {
          printf (_("Error: Invalid line %s in cache index file\n"), buf);
	  continue;
        }
      unlink (pos + 1);
    }

  fclose (indexfd);
  unlink (indexfile);
  g_free (indexfile);
}


void
gftp_delete_cache_entry (gftp_request * request, int ignore_directory)
{
  char *oldindexfile, *newindexfile, *pos, buf[BUFSIZ], description[BUFSIZ];
  FILE *indexfd, *newfd;
  size_t len, buflen;
  int remove;

  g_snprintf (description, sizeof (description), "%s://%s@%s:%d%s",
              gftp_cache_get_url_prefix (request),
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              ignore_directory || request->directory == NULL ? "" : request->directory);

  oldindexfile = expand_path (BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = fopen (oldindexfile, "rb")) == NULL)
    {
      g_free (oldindexfile);
      return;
    }

  newindexfile = expand_path (BASE_CONF_DIR "/cache/index.db.new");
  if ((newfd = fopen (newindexfile, "wb")) == NULL)
    {
      g_free (oldindexfile);
      g_free (newindexfile);
      return;
    }

  buflen = strlen (description);
  while (fgets (buf, sizeof (buf) - 1, indexfd))
    {
      len = strlen (buf);
      if (buf[len - 1] == '\n')
        buf[--len] = '\0';
      if (buf[len - 1] == '\r')
        buf[--len] = '\0';

      if (!((pos = strrchr (buf, '\t')) != NULL && *(pos + 1) != '\0'))
        {
          printf (_("Error: Invalid line %s in cache index file\n"), buf);
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
          fwrite (buf, 1, strlen (buf), newfd);
        }
    }

  fclose (indexfd);
  fclose (newfd);

  unlink (oldindexfile);
  rename (newindexfile, oldindexfile);

  g_free (oldindexfile);
  g_free (newindexfile);
}

