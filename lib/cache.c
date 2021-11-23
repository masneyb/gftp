/***********************************************************************************/
/*  cache.c - contains the cache routines                                          */
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

struct gftp_cache_entry_tag
{
  char *url,
       *file;
  time_t expiration_date;

  char *pos1,
       *pos2,
       *pos3;
};
  
typedef struct gftp_cache_entry_tag gftp_cache_entry;


static int
gftp_parse_cache_line (gftp_request * request, /*@out@*/ gftp_cache_entry * centry, 
                       char *line)
{
  char *pos;

  memset (centry, 0, sizeof (*centry));

  if ((pos = strchr (line, '\t')) == NULL || *(pos + 1) == '\0')
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                            _("Error: Invalid line %s in cache index file\n"), 
                            line);
      return (-1);
    }

  centry->pos1 = pos;
  *pos++ = '\0';
  centry->url = line;
  centry->file = pos;
  
  if ((pos = strchr (pos, '\t')) == NULL || *(pos + 1) == '\0')
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                            _("Error: Invalid line %s in cache index file\n"), 
                            line);
      return (-1);
    }

  centry->pos2 = pos;
  *pos++ = '\0';
  //centry->server_type = strtol (pos, NULL, 10);

  if ((pos = strchr (pos, '\t')) == NULL || *(pos + 1) == '\0')
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                            _("Error: Invalid line %s in cache index file\n"), 
                            line);
      return (-1);
    }

  centry->pos3 = pos;
  *pos++ = '\0';
  centry->expiration_date = strtol (pos, NULL, 10);

  return (0);
}


static void
gftp_restore_cache_line (gftp_cache_entry * centry)
{
  if (centry->pos1 != NULL)
    *centry->pos1 = '\t';

  if (centry->pos2 != NULL)
    *centry->pos2 = '\t';

  if (centry->pos3 != NULL)
    *centry->pos3 = '\t';
}


void
gftp_generate_cache_description (gftp_request * request,
                                 char *description,
                                 size_t len, int ignore_directory)
{
  g_snprintf (description, len, "%s://%s@%s:%d%s",
              request->url_prefix,
              request->username == NULL ? "" : request->username,
              request->hostname == NULL ? "" : request->hostname,
              request->port, 
              ignore_directory || request->directory == NULL ? "" : request->directory);
}


int
gftp_new_cache_entry (gftp_request * request)
{
  char *cachedir, *tempstr, *temp1str;
  int cache_fd, fd;
  intptr_t cache_ttl;
  ssize_t ret;
  time_t t;

  gftp_lookup_request_option (request, "cache_ttl", &cache_ttl);
  time (&t);
  t += cache_ttl;

  cachedir = gftp_expand_path (NULL, BASE_CONF_DIR "/cache");
  if (access (cachedir, F_OK) == -1)
    {
      if (mkdir (cachedir, S_IRUSR | S_IWUSR | S_IXUSR) < 0)
        {
          if (request != NULL)
            request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not make directory %s: %s\n"),
                                 cachedir, g_strerror (errno));

          g_free (cachedir);
          return (-1);
        }
    }

  tempstr = g_strdup_printf ("%s/index.db", cachedir);
  if ((fd = gftp_fd_open (request, tempstr, O_WRONLY | O_APPEND | O_CREAT, 
                          S_IRUSR | S_IWUSR)) == -1)
    {
      g_free (tempstr);
      g_free (cachedir);
      return (-1);
    }

  g_free (tempstr);

  tempstr = g_strdup_printf ("%s/cache.XXXXXX", cachedir);
  if ((cache_fd = mkstemp (tempstr)) < 0)
    {
      g_free (tempstr);
      g_free (cachedir);
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot create temporary file: %s\n"),
                                 g_strerror (errno));
      return (-1);
    }
  g_free (cachedir);

  lseek (fd, 0, SEEK_END);
  temp1str = g_strdup_printf ("%s://%s@%s:%d%s\t%s\t%d\t%ld\n", 
                           request->url_prefix,
                           request->username == NULL ? "" : request->username,
                           request->hostname == NULL ? "" : request->hostname,
                           request->port, 
                           request->directory == NULL ? "" : request->directory,
                           tempstr, 0, t); // 0 = server_type
  g_free (tempstr);
  ret = gftp_fd_write (NULL, temp1str, strlen (temp1str), fd);
  g_free (temp1str);

  if (close (fd) != 0 || ret < 0)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
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
  char *indexfile, buf[BUFSIZ], description[BUFSIZ];
  gftp_getline_buffer * rbuf;
  gftp_cache_entry centry;
  int indexfd, cachefd;
  time_t now;

  time (&now);

  *description = '\0';
  gftp_generate_cache_description (request, description, sizeof (description),
                                   0);

  indexfile = gftp_expand_path (NULL, BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = gftp_fd_open (NULL, indexfile, O_RDONLY, 0)) == -1)
    {
      g_free (indexfile);
      return (-1);
    }
  g_free (indexfile);

  rbuf = NULL;
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf), indexfd) > 0)
    {
      if (gftp_parse_cache_line (request, &centry, buf) < 0)
        continue;

      /* See if this entry is still valid... */
      if (centry.expiration_date < now)
        continue;

      if (strcmp (description, centry.url) == 0)
	{
	  if (close (indexfd) != 0)
            {
              if (request != NULL)
                request->logging_function (gftp_logging_error, request,
                                       _("Error closing file descriptor: %s\n"),
                                       g_strerror (errno));
              return (-1);
            }

	  if ((cachefd = gftp_fd_open (request, centry.file, O_RDONLY, 0)) == -1)
            return (-1);

          if (lseek (cachefd, 0, SEEK_END) == 0)
            { 
              close (cachefd); 
              return (-1);
            } 

          if (lseek (cachefd, 0, SEEK_SET) == -1)
            {
              if (request != NULL)
                request->logging_function (gftp_logging_error, request,
                                       _("Error: Cannot seek on file %s: %s\n"),
                                       centry.file, g_strerror (errno));

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
  char *indexfile, buf[BUFSIZ];
  gftp_getline_buffer * rbuf;
  gftp_cache_entry centry;
  int indexfd;

  indexfile = gftp_expand_path (NULL, BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = gftp_fd_open (NULL, indexfile, O_RDONLY, 0)) == -1)
    {
      g_free (indexfile);
      return;
    }

  *buf = '\0';
  rbuf = NULL;
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf), indexfd) > 0)
    {
      if (gftp_parse_cache_line (NULL, &centry, buf) < 0)
        continue;

      unlink (centry.file);
    }

  close (indexfd);
  unlink (indexfile);
  g_free (indexfile);
}


void
gftp_delete_cache_entry (gftp_request * request, char *descr, 
                         int ignore_directory)
{
  char *oldindexfile, *newindexfile, buf[BUFSIZ], description[BUFSIZ];
  int indexfd, newfd, del_entry;
  gftp_getline_buffer * rbuf;
  gftp_cache_entry centry;
  size_t len;
  time_t now;
 
  g_return_if_fail (request != NULL || descr != NULL);

  time (&now);
  if (request != NULL)
    {
      *description = '\0';
      gftp_generate_cache_description (request, description, sizeof (description),
                                       ignore_directory);
    }
  else if (descr != NULL)
    { 
      strncpy (description, descr, sizeof (description));
      description[sizeof (description) - 1] = '\0';
    }
  else
    return;

  oldindexfile = gftp_expand_path (NULL, BASE_CONF_DIR "/cache/index.db");
  if ((indexfd = gftp_fd_open (NULL, oldindexfile, O_RDONLY, 0)) == -1)
    {
      g_free (oldindexfile);
      return;
    }

  newindexfile = gftp_expand_path (NULL, BASE_CONF_DIR "/cache/index.db.new");
  if ((newfd = gftp_fd_open (request, newindexfile, O_WRONLY | O_CREAT, 
                             S_IRUSR | S_IWUSR)) == -1)
    {
      g_free (oldindexfile);
      g_free (newindexfile);
      return;
    }

  *buf = '\0';
  rbuf = NULL;
  while (gftp_get_line (NULL, &rbuf, buf, sizeof (buf) - 1, indexfd) > 0)
    {
      if (gftp_parse_cache_line (request, &centry, buf) < 0)
        continue;

      del_entry = 0;
      if (centry.expiration_date < now)
        del_entry = 1;
      else if (ignore_directory)
        {
          if (strncmp (centry.url, description, strlen (description)) == 0)
            del_entry = 1;
        }
      else
        {
          if (strcmp (centry.url, description) == 0)
            del_entry = 1;
        }

 
      if (del_entry)
        unlink (centry.file);
      else
        {
          /* Make sure we put the tabs back in the line. I do it this way 
             so that I don't have to allocate memory again for each line 
             as we read it */
          gftp_restore_cache_line (&centry);

          /* Make sure when we call gftp_get_line() that we pass the read size
             as sizeof(buf) - 1 so that we'll have room to put the newline */
          len = strlen (buf);
          buf[len--] = '\n';

          if (gftp_fd_write (NULL, buf, len, newfd) < 0)
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

