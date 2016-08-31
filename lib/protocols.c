/*****************************************************************************/
/*  protocols.c - Skeleton functions for the protocols gftp supports         */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
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

gftp_request *
gftp_request_new (void)
{
  gftp_request *request;

  request = g_malloc0 (sizeof (*request));
  request->datafd = -1;
  request->cachefd = -1;
  request->server_type = GFTP_DIRTYPE_OTHER;
  return (request);
}


void
gftp_request_destroy (gftp_request * request, int free_request)
{
  g_return_if_fail (request != NULL);

  gftp_disconnect (request);

  if (request->destroy != NULL)
    request->destroy (request);

  if (request->hostname)
    g_free (request->hostname);
  if (request->username)
    g_free (request->username);
  if (request->password)
    g_free (request->password);
  if (request->account)
    g_free (request->account);
  if (request->directory)
    g_free (request->directory);
  if (request->last_ftp_response)
    g_free (request->last_ftp_response);
  if (request->protocol_data)
    g_free (request->protocol_data);

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  if (request->remote_addr != NULL)
    g_free (request->remote_addr);
#endif

  if (request->local_options_vars != NULL)
    {
      gftp_config_free_options (request->local_options_vars,
                                request->local_options_hash,
                                request->num_local_options_vars);
    }

  memset (request, 0, sizeof (*request));

  if (free_request)
    g_free (request);
  else
    {
      request->datafd = -1;
      request->cachefd = -1;
      request->server_type = GFTP_DIRTYPE_OTHER;
    }
}


/* This function is called to copy protocol specific data from one request 
   structure to another. This is typically called when a file transfer is
   completed, state information can be copied back to the main window */
void
gftp_copy_param_options (gftp_request * dest_request,
                         gftp_request * src_request)
{
  g_return_if_fail (dest_request != NULL);
  g_return_if_fail (src_request != NULL);
  g_return_if_fail (dest_request->protonum == src_request->protonum);

  if (dest_request->copy_param_options)
    dest_request->copy_param_options (dest_request, src_request);
}


void
gftp_file_destroy (gftp_file * file, int free_it)
{
  g_return_if_fail (file != NULL);

  if (file->file)
    g_free (file->file);
  if (file->user)
    g_free (file->user);
  if (file->group)
    g_free (file->group);
  if (file->destfile)
    g_free (file->destfile);

  if (free_it)
    g_free (file);
  else
    memset (file, 0, sizeof (*file));
}


int
gftp_connect (gftp_request * request)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->connect == NULL)
    return (GFTP_EFATAL);

  if ((ret = gftp_set_config_options (request)) < 0)
    return (ret);

  return (request->connect (request));
}


void
gftp_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);

#ifdef USE_SSL
  if (request->ssl != NULL)
    {
      SSL_free (request->ssl);
      request->ssl = NULL;
    }
#endif

#if GLIB_MAJOR_VERSION > 1
  if (request->iconv_from_initialized)
    {
      g_iconv_close (request->iconv_from);
      request->iconv_from_initialized = 0;
    }

  if (request->iconv_to_initialized)
    {
      g_iconv_close (request->iconv_to);
      request->iconv_to_initialized = 0;
    }

  if (request->iconv_charset)
  {
    g_free (request->iconv_charset);
    request->iconv_charset = NULL;
  }
#endif

  request->cached = 0;
  if (request->disconnect == NULL)
    return;
  request->disconnect (request);
}


off_t
gftp_get_file (gftp_request * request, const char *filename,
               off_t startsize)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->cached = 0;
  if (request->get_file == NULL)
    return (GFTP_EFATAL);

  return (request->get_file (request, filename, startsize));
}


int
gftp_put_file (gftp_request * request, const char *filename,
               off_t startsize, off_t totalsize)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->cached = 0;
  if (request->put_file == NULL)
    return (GFTP_EFATAL);

  return (request->put_file (request, filename, startsize, totalsize));
}


off_t
gftp_transfer_file (gftp_request * fromreq, const char *fromfile, 
                    off_t fromsize, gftp_request * toreq, const char *tofile,
                    off_t tosize)
{
  /* Needed for systems that size(float) < size(void *) */
  union { intptr_t i; float f; } maxkbs;
  off_t size;
  int ret;

  g_return_val_if_fail (fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromfile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (toreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (tofile != NULL, GFTP_EFATAL);

  gftp_lookup_request_option (toreq, "maxkbs", &maxkbs.f);

  if (maxkbs.f > 0)
    {
      toreq->logging_function (gftp_logging_misc, toreq,
                    _("File transfer will be throttled to %.2f KB/s\n"), 
                    maxkbs.f);
    }

  if (fromreq->protonum == toreq->protonum &&
      fromreq->transfer_file != NULL)
    return (fromreq->transfer_file (fromreq, fromfile, fromsize, toreq, 
                                    tofile, tosize));

  fromreq->cached = 0;
  toreq->cached = 0;

get_file:
  size = gftp_get_file (fromreq, fromfile, tosize);
  if (size < 0)
    {
      if (size == GFTP_ETIMEDOUT)
        {
          ret = gftp_connect (fromreq);
          if (ret < 0)
            return (ret);

          goto get_file;
        }

      return (size);
    }

put_file:
  ret = gftp_put_file (toreq, tofile, tosize, size);
  if (ret != 0)
    {
      if (size == GFTP_ETIMEDOUT)
        {
          ret = gftp_connect (fromreq);
          if (ret < 0)
            return (ret);

          goto put_file;
        }

      if (gftp_abort_transfer (fromreq) != 0)
        gftp_end_transfer (fromreq);

      return (ret);
    }

  return (size);
}


ssize_t 
gftp_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);

  if (request->get_next_file_chunk != NULL)
    return (request->get_next_file_chunk (request, buf, size));

  return (request->read_function (request, buf, size, request->datafd));
}


ssize_t 
gftp_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);

  if (request->put_next_file_chunk != NULL)
    return (request->put_next_file_chunk (request, buf, size));

  return (request->write_function (request, buf, size, request->datafd));
}


int
gftp_end_transfer (gftp_request * request)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (!request->cached && 
      request->end_transfer != NULL)
    ret = request->end_transfer (request);
  else
    ret = 0;

  if (request->cachefd > 0)
    {
      close (request->cachefd);
      request->cachefd = -1;
    }

  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
      request->last_dir_entry_len = 0;
    }

  return (ret);
}


int
gftp_abort_transfer (gftp_request * request)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->abort_transfer == NULL)
    return (GFTP_EFATAL);

  /* FIXME - end the transfer if it is not successful */
  return (request->abort_transfer (request));
}


int
gftp_stat_filename (gftp_request * request, const char *filename, mode_t * mode,
                    off_t * filesize)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  if (request->stat_filename != NULL)
    return (request->stat_filename (request, filename, mode, filesize));
  else
    return (0);
}


int
gftp_list_files (gftp_request * request)
{
  char *remote_lc_time, *locret;
  int fd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

#if ENABLE_NLS
  gftp_lookup_request_option (request, "remote_lc_time", &remote_lc_time);
  if (remote_lc_time != NULL && *remote_lc_time != '\0')
    locret = setlocale (LC_TIME, remote_lc_time);
  else
    locret = setlocale (LC_TIME, NULL);

  if (locret == NULL)
    {
      locret = setlocale (LC_TIME, NULL);
      request->logging_function (gftp_logging_error, request,
                                 _("Error setting LC_TIME to '%s'. Falling back to '%s'\n"),
                                 remote_lc_time, locret);
    }
#else
  locret = _("<unknown>");
#endif

  request->cached = 0;
  if (request->use_cache && (fd = gftp_find_cache_entry (request)) > 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Loading directory listing %s from cache (LC_TIME=%s)\n"),
                                 request->directory, locret);

      request->cachefd = fd;
      request->cached = 1;
      return (0);
    }
  else if (request->use_cache)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Loading directory listing %s from server (LC_TIME=%s)\n"),
                                 request->directory, locret);

      request->cachefd = gftp_new_cache_entry (request);
      request->cached = 0; 
    }

  if (request->list_files == NULL)
    return (GFTP_EFATAL);

  return (request->list_files (request));
}


int
gftp_get_next_file (gftp_request * request, const char *filespec,
                    gftp_file * fle)
{
  char *slashpos, *tmpfile, *utf8;
  size_t destlen;
  int fd, ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->get_next_file == NULL)
    return (GFTP_EFATAL);

  if (request->cached && request->cachefd > 0)
    fd = request->cachefd;
  else
    fd = request->datafd;

  memset (fle, 0, sizeof (*fle));
  do
    {
      gftp_file_destroy (fle, 0);
      ret = request->get_next_file (request, fle, fd);
      if (fle->file != NULL && (slashpos = strrchr (fle->file, '/')) != NULL)
        {
          if (*(slashpos + 1) == '\0')
            {
              gftp_file_destroy (fle, 0);
              continue;
            }

          *slashpos = '\0';
          tmpfile = g_strdup (slashpos + 1);

          if (strcmp (fle->file, request->directory) != 0)
            request->logging_function (gftp_logging_error, request,
                                       _("Warning: Stripping path off of file '%s'. The stripped path (%s) doesn't match the current directory (%s)\n"),
                                       tmpfile, fle->file, request->directory,
                                       g_strerror (errno));
          
          g_free (fle->file);
          fle->file = tmpfile;
        }

      if (ret >= 0 && fle->file != NULL)
        {
          if (g_utf8_validate (fle->file, -1, NULL))
            fle->filename_utf8_encoded = 1;
          else
            {
              utf8 = gftp_filename_to_utf8 (request, fle->file, &destlen);
              if (utf8 != NULL)
                {
                  g_free (fle->file);
                  fle->file = utf8;
                  fle->filename_utf8_encoded = 1;
                }
            }
        }

      if (ret >= 0 && !request->cached && request->cachefd > 0 && 
          request->last_dir_entry != NULL)
        {
          if (gftp_fd_write (request, request->last_dir_entry,
                          request->last_dir_entry_len, request->cachefd) < 0)
            {
              request->logging_function (gftp_logging_error, request,
                                        _("Error: Cannot write to cache: %s\n"),
                                        g_strerror (errno));
              close (request->cachefd);
              request->cachefd = -1;
            }
        }
    } while (ret > 0 && !gftp_match_filespec (request, fle->file, filespec));

  return (ret);
}


int
gftp_parse_bookmark (gftp_request * request, gftp_request * local_request, 
                     const char * bookmark, int *refresh_local)
{
  gftp_logging_func logging_function;
  gftp_bookmarks_var * tempentry;
  char *default_protocol;
  const char *email;
  int i, init_ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (bookmark != NULL, GFTP_EFATAL);
  
  logging_function = request->logging_function;
  gftp_request_destroy (request, 0);
  request->logging_function = logging_function;

  if ((tempentry = g_hash_table_lookup (gftp_bookmarks_htable, 
                                        bookmark)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not find bookmark %s\n"), 
                                 bookmark);
      return (GFTP_EFATAL);
    }
  else if (tempentry->hostname == NULL || *tempentry->hostname == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Bookmarks Error: The bookmark entry %s does not have a hostname\n"), bookmark);
      return (GFTP_EFATAL);
    }

  if (tempentry->user != NULL)
    gftp_set_username (request, tempentry->user);

  if (tempentry->pass != NULL)
    {
      if (strcmp (tempentry->pass, "@EMAIL@") == 0)
        {
          gftp_lookup_request_option (request, "email", &email);
          gftp_set_password (request, email);
        }
      else
        gftp_set_password (request, tempentry->pass);
    }

  if (tempentry->acct != NULL)
    gftp_set_account (request, tempentry->acct);

  gftp_set_hostname (request, tempentry->hostname);
  gftp_set_directory (request, tempentry->remote_dir);
  gftp_set_port (request, tempentry->port);

  if (local_request != NULL && tempentry->local_dir != NULL &&
      *tempentry->local_dir != '\0')
    {
      gftp_set_directory (local_request, tempentry->local_dir);
      if (refresh_local != NULL)
        *refresh_local = 1;
    }
  else if (refresh_local != NULL)
    *refresh_local = 0;

  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (strcmp (gftp_protocols[i].name, tempentry->protocol) == 0)
        {
          if ((init_ret = gftp_protocols[i].init (request)) < 0)
            {
              gftp_request_destroy (request, 0);
              return (init_ret);
            }
          break;
        }
    }

  if (gftp_protocols[i].name == NULL)
    {
      gftp_lookup_request_option (request, "default_protocol", 
                                  &default_protocol);

      if (default_protocol != NULL && *default_protocol != '\0')
        {
          for (i = 0; gftp_protocols[i].url_prefix; i++)
            {
              if (strcmp (gftp_protocols[i].name, default_protocol) == 0)
                break;
            }
        }

      if (gftp_protocols[i].url_prefix == NULL)
        i = GFTP_FTP_NUM;
    }

  gftp_copy_local_options (&request->local_options_vars,
                           &request->local_options_hash,
                           &request->num_local_options_vars,
                           tempentry->local_options_vars,
                           tempentry->num_local_options_vars);

  if ((init_ret = gftp_protocols[i].init (request)) < 0)
    {
      gftp_request_destroy (request, 0);
      return (init_ret);
    }

  return (0);
}


int
gftp_parse_url (gftp_request * request, const char *url)
{
  char *pos, *endpos, *default_protocol, *new_url;
  gftp_logging_func logging_function;
  const char *clear_pos;
  int i, ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (url != NULL, GFTP_EFATAL);

  logging_function = request->logging_function;
  gftp_request_destroy (request, 0);
  request->logging_function = logging_function;

  for (clear_pos = url;
       *clear_pos == ' ' || *clear_pos == '\t';
       clear_pos++);

  new_url = g_strdup (clear_pos);

  for (pos = new_url + strlen (new_url) - 1;
       *pos == ' ' || *pos == '\t';
       pos--)
    *pos = '\0';

  /* See if the URL has a protocol... */
  if ((pos = strstr (new_url, "://")) != NULL)
    {
      *pos = '\0';

      for (i = 0; gftp_protocols[i].url_prefix; i++)
        {
          if (strcmp (gftp_protocols[i].url_prefix, new_url) == 0)
            break;
        }

      if (gftp_protocols[i].url_prefix == NULL)
        {
          request->logging_function (gftp_logging_error, NULL, 
                                     _("The protocol '%s' is currently not supported.\n"),
                                     new_url);
          g_free (new_url);
          return (GFTP_EFATAL);
        }

      *pos = ':';
      pos += 3;
    }
  else
    {
      gftp_lookup_request_option (request, "default_protocol", 
                                  &default_protocol);

      i = GFTP_FTP_NUM;
      if (default_protocol != NULL && *default_protocol != '\0')
        {
          for (i = 0; gftp_protocols[i].url_prefix; i++)
            {
              if (strcmp (gftp_protocols[i].name, default_protocol) == 0)
                break;
            }
        }

      if (gftp_protocols[i].url_prefix == NULL)
        {
          request->logging_function (gftp_logging_error, NULL, 
                                     _("The protocol '%s' is currently not supported.\n"),
                                     default_protocol);
          g_free (new_url);
          return (GFTP_EFATAL);
        }

      pos = new_url;
    }

  if ((ret = gftp_protocols[i].init (request)) < 0)
    {
      gftp_request_destroy (request, 0);
      return (ret);
    }

  if ((endpos = strchr (pos, '/')) != NULL)
    {
      gftp_set_directory (request, endpos);
      *endpos = '\0';
    }

  if (request->parse_url != NULL)
    {
      ret = request->parse_url (request, new_url);
      g_free (new_url);
      return (ret);
    }

  if (*pos != '\0')
    {
      if (endpos == NULL)
        endpos = pos + strlen (pos) - 1;
      else
        endpos--;

      for (; isdigit (*endpos); endpos--);

      if (*endpos == ':' && isdigit (*(endpos + 1)))
        {
          gftp_set_port (request, strtol (endpos + 1, NULL, 10));
          *endpos = '\0';
        }

      if ((endpos = strrchr (pos, '@')) != NULL)
        {
          gftp_set_hostname (request, endpos + 1);
          *endpos = '\0';

          if ((endpos = strchr (pos, ':')) != NULL)
            {
              *endpos = '\0';
              gftp_set_username (request, pos);
              gftp_set_password (request, endpos + 1);
            }
          else
            {
              gftp_set_username (request, pos);
              gftp_set_password (request, "");
            }
        }
      else
        gftp_set_hostname (request, pos);
    }

  g_free (new_url);
  return (0);
}


void
gftp_set_hostname (gftp_request * request, const char *hostname)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (hostname != NULL);

  if (request->hostname)
    g_free (request->hostname);
  request->hostname = g_strdup (hostname);
}


void
gftp_set_username (gftp_request * request, const char *username)
{
  g_return_if_fail (request != NULL);

  if (request->username)
    g_free (request->username);

  if (username != NULL)
    request->username = g_strdup (username);
  else
    request->username = NULL;
}


void
gftp_set_password (gftp_request * request, const char *password)
{
  g_return_if_fail (request != NULL);

  if (request->password)
    g_free (request->password);

  if (password != NULL)
    request->password = g_strdup (password);
  else
    request->password = NULL;
}


void
gftp_set_account (gftp_request * request, const char *account)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (account != NULL);

  if (request->account)
    g_free (request->account);
  request->account = g_strdup (account);
}


int
gftp_set_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);


  if (request->datafd <= 0 && !request->always_connected)
    {
      if (directory != request->directory)
        {
          if (request->directory)
            g_free (request->directory);
          request->directory = g_strdup (directory);
        }
      return (0);
    }
  else if (request->chdir == NULL)
    return (GFTP_EFATAL);
  return (request->chdir (request, directory));
}


void
gftp_set_port (gftp_request * request, unsigned int port)
{
  g_return_if_fail (request != NULL);

  request->port = port;
}


int
gftp_remove_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->rmdir == NULL)
    return (GFTP_EFATAL);
  return (request->rmdir (request, directory));
}


int
gftp_remove_file (gftp_request * request, const char *file)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->rmfile == NULL)
    return (GFTP_EFATAL);
  return (request->rmfile (request, file));
}


int
gftp_make_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->mkdir == NULL)
    return (GFTP_EFATAL);

  return (request->mkdir (request, directory));
}


int
gftp_rename_file (gftp_request * request, const char *oldname,
                  const char *newname)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->rename == NULL)
    return (GFTP_EFATAL);

  return (request->rename (request, oldname, newname));
}


int
gftp_chmod (gftp_request * request, const char *file, mode_t mode)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->chmod == NULL)
    return (GFTP_EFATAL);

  mode &= S_IRWXU | S_IRWXG | S_IRWXO | S_ISUID | S_ISGID | S_ISVTX;
  return (request->chmod (request, file, mode));
}


int
gftp_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->set_file_time == NULL)
    return (GFTP_EFATAL);
  return (request->set_file_time (request, file, datetime));
}


int
gftp_site_cmd (gftp_request * request, int specify_site, const char *command)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->site == NULL)
    return (GFTP_EFATAL);
  return (request->site (request, specify_site, command));
}


off_t
gftp_get_file_size (gftp_request * request, const char *filename)
{
  g_return_val_if_fail (request != NULL, 0);

  if (request->get_file_size == NULL)
    return (0);
  return (request->get_file_size (request, filename));
}


static GHashTable *
gftp_gen_dir_hash (gftp_request * request, int *ret)
{
  GHashTable * dirhash;
  gftp_file * fle;
  off_t *newsize;

  dirhash = g_hash_table_new (string_hash_function, string_hash_compare);
  *ret = gftp_list_files (request);
  if (*ret == 0)
    {
      fle = g_malloc0 (sizeof (*fle));
      while (gftp_get_next_file (request, NULL, fle) > 0)
        {
          newsize = g_malloc0 (sizeof (*newsize));
          *newsize = fle->size;
          g_hash_table_insert (dirhash, fle->file, newsize);
          fle->file = NULL;
          gftp_file_destroy (fle, 0);
        }
      gftp_end_transfer (request);
      g_free (fle);
    }
  else
    {
      g_hash_table_destroy (dirhash);
      dirhash = NULL;
    }

  return (dirhash);
}


static void
destroy_hash_ent (gpointer key, gpointer value, gpointer user_data)
{

  g_free (key);
  g_free (value);
}


static void
gftp_destroy_dir_hash (GHashTable * dirhash)
{
  if (dirhash == NULL)
    return;

  g_hash_table_foreach (dirhash, destroy_hash_ent, NULL);
  g_hash_table_destroy (dirhash);
}


static GList *
gftp_get_dir_listing (gftp_transfer * transfer, int getothdir, int *ret)
{
  GHashTable * dirhash;
  GList * templist;
  gftp_file * fle;
  off_t *newsize;
  char *newname;

  if (getothdir && transfer->toreq != NULL)
    {
      dirhash = gftp_gen_dir_hash (transfer->toreq, ret);
      if (*ret == GFTP_EFATAL)
        return (NULL);
    }
  else 
    dirhash = NULL; 

  *ret = gftp_list_files (transfer->fromreq);
  if (*ret < 0)
    {
      gftp_destroy_dir_hash (dirhash);
      return (NULL);
    }

  fle = g_malloc0 (sizeof (*fle));
  templist = NULL;
  while (gftp_get_next_file (transfer->fromreq, NULL, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0 || strcmp (fle->file, "..") == 0)
        {
          gftp_file_destroy (fle, 0);
          continue;
        }

      if (dirhash && 
          (newsize = g_hash_table_lookup (dirhash, fle->file)) != NULL)
        {
          fle->exists_other_side = 1;
          fle->startsize = *newsize;
        }
      else
        fle->exists_other_side = 0;

      if (transfer->toreq && fle->destfile == NULL)
        fle->destfile = gftp_build_path (transfer->toreq,
                                         transfer->toreq->directory, 
                                         fle->file, NULL);

      if (transfer->fromreq->directory != NULL &&
          *transfer->fromreq->directory != '\0' &&
          *fle->file != '/')
        {
          newname = gftp_build_path (transfer->fromreq,
                                     transfer->fromreq->directory,
                                     fle->file, NULL);

          g_free (fle->file);
          fle->file = newname;
        }

      templist = g_list_append (templist, fle);

      fle = g_malloc0 (sizeof (*fle));
    }
  gftp_end_transfer (transfer->fromreq);

  gftp_file_destroy (fle, 1);
  gftp_destroy_dir_hash (dirhash);

  return (templist);
}


static void
_cleanup_get_all_subdirs (gftp_transfer * transfer, char *oldfromdir,
                          char *oldtodir,
                          void (*update_func) (gftp_transfer * transfer))
{
  if (update_func != NULL)
    {
      transfer->numfiles = transfer->numdirs = -1;
      update_func (transfer);
    }

  if (oldfromdir != NULL)
    g_free (oldfromdir);

  if (oldtodir != NULL)
    g_free (oldtodir);
}


static GList *
_setup_current_directory_transfer (gftp_transfer * transfer, int *ret)
{
  GHashTable * dirhash;
  char *pos, *newname;
  gftp_file * curfle;
  GList * lastlist;
  off_t *newsize;

  *ret = 0;
  if (transfer->toreq != NULL)
    {
      dirhash = gftp_gen_dir_hash (transfer->toreq, ret);
      if (*ret == GFTP_EFATAL)
        return (NULL);
    }
  else
    dirhash = NULL;

  for (lastlist = transfer->files; ; lastlist = lastlist->next)
    {
      curfle = lastlist->data;

      if ((pos = strrchr (curfle->file, '/')) != NULL)
        pos++;
      else
        pos = curfle->file;

      if (dirhash != NULL && 
          (newsize = g_hash_table_lookup (dirhash, pos)) != NULL)
        {
          curfle->exists_other_side = 1;
          curfle->startsize = *newsize;
        }
      else
        curfle->exists_other_side = 0;

      if (curfle->size < 0 && GFTP_IS_CONNECTED (transfer->fromreq))
        {
          curfle->size = gftp_get_file_size (transfer->fromreq, curfle->file);
          if (curfle->size == GFTP_EFATAL)
            {
              gftp_destroy_dir_hash (dirhash);
              *ret = curfle->size;
              return (NULL);
            }
        }

      if (transfer->toreq && curfle->destfile == NULL)
        curfle->destfile = gftp_build_path (transfer->toreq,
                                            transfer->toreq->directory, 
                                            curfle->file, NULL);

      if (transfer->fromreq->directory != NULL &&
          *transfer->fromreq->directory != '\0' && *curfle->file != '/')
        {
          newname = gftp_build_path (transfer->fromreq,
                                     transfer->fromreq->directory,
                                     curfle->file, NULL);
          g_free (curfle->file);
          curfle->file = newname;
        }

      if (lastlist->next == NULL)
        break;
    }

  gftp_destroy_dir_hash (dirhash);

  return (lastlist);
}


static int
_lookup_curfle_in_device_hash (gftp_request * request, gftp_file * curfle,
                               GHashTable * device_hash)
{
  GHashTable * inode_hash;

  if (curfle->st_dev == 0 || curfle->st_ino == 0)
    return (0);

  if ((inode_hash = g_hash_table_lookup (device_hash,
                                         GUINT_TO_POINTER ((guint) curfle->st_dev))) != NULL)
    {
      if (g_hash_table_lookup (inode_hash,
                               GUINT_TO_POINTER ((guint) curfle->st_ino)))
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Found recursive symbolic link %s\n"),
                                     curfle->file);
          return (1);
        }

      g_hash_table_insert (inode_hash, GUINT_TO_POINTER ((guint) curfle->st_ino),
                           GUINT_TO_POINTER (1));
      return (0);
    }
  else
    {
      inode_hash = g_hash_table_new (uint_hash_function, uint_hash_compare);
      g_hash_table_insert (inode_hash, GUINT_TO_POINTER ((guint) curfle->st_ino),
                           GUINT_TO_POINTER (1));
      g_hash_table_insert (device_hash, GUINT_TO_POINTER ((guint) curfle->st_dev),
                           inode_hash);
      return (0);
    }

}


static void
_free_inode_hash (gpointer key, gpointer value, gpointer user_data)
{
  g_hash_table_destroy (value);
}


static void
_free_device_hash (GHashTable * device_hash)
{
  g_hash_table_foreach (device_hash, _free_inode_hash, NULL);
  g_hash_table_destroy (device_hash);
}


int
gftp_get_all_subdirs (gftp_transfer * transfer,
                      void (*update_func) (gftp_transfer * transfer))
{
  GList * templist, * lastlist;
  char *oldfromdir, *oldtodir;
  GHashTable * device_hash;
  gftp_file * curfle;
  off_t linksize;
  mode_t st_mode;
  int ret;

  g_return_val_if_fail (transfer != NULL, GFTP_EFATAL);
  g_return_val_if_fail (transfer->fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (transfer->files != NULL, GFTP_EFATAL);

  if (transfer->files == NULL)
    return (0);

  ret = 0;
  lastlist = _setup_current_directory_transfer (transfer, &ret);
  if (lastlist == NULL)
    return (ret);

  oldfromdir = oldtodir = NULL;
  device_hash = g_hash_table_new (uint_hash_function, uint_hash_compare);

  for (templist = transfer->files; templist != NULL; templist = templist->next)
    {
      curfle = templist->data;

      if (_lookup_curfle_in_device_hash (transfer->fromreq, curfle,
                                         device_hash))
        continue;

      if (S_ISLNK (curfle->st_mode) && !S_ISDIR (curfle->st_mode))
        {
          st_mode = 0;
          linksize = 0;
          ret = gftp_stat_filename (transfer->fromreq, curfle->file, &st_mode,
                                    &linksize);
          if (ret == GFTP_EFATAL)
            {
              _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                        update_func);
              return (ret);
            }
          else if (ret == 0)
            {
              if (S_ISDIR (st_mode))
                curfle->st_mode = st_mode;
              else
                curfle->size = linksize;
            }
        }

      if (!S_ISDIR (curfle->st_mode))
        {
          transfer->numfiles++;
          continue;
        }

      /* Got a directory... */
      transfer->numdirs++;

      if (oldfromdir == NULL)
        oldfromdir = g_strdup (transfer->fromreq->directory);

      ret = gftp_set_directory (transfer->fromreq, curfle->file);
      if (ret < 0)
        {
          _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                    update_func);
          _free_device_hash (device_hash);
          return (ret);
        }

      if (transfer->toreq != NULL)
        {
          if (oldtodir == NULL)
            oldtodir = g_strdup (transfer->toreq->directory);

          if (curfle->exists_other_side)
            {
              ret = gftp_set_directory (transfer->toreq, curfle->destfile);
              if (ret == GFTP_EFATAL)
                {
                  _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                            update_func);
                  _free_device_hash (device_hash);
                  return (ret);
                }
            }
          else
            {
              if (transfer->toreq->directory != NULL)
                g_free (transfer->toreq->directory);

              transfer->toreq->directory = g_strdup (curfle->destfile);
            }
        } 

      ret = 0;
      lastlist->next = gftp_get_dir_listing (transfer,
                                             curfle->exists_other_side, &ret);
      if (ret < 0)
        {
          _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                    update_func);
          _free_device_hash (device_hash);
          return (ret);
        }

      if (lastlist->next != NULL)
        {
          lastlist->next->prev = lastlist;
          for (; lastlist->next != NULL; lastlist = lastlist->next);
        }

      if (update_func != NULL)
        update_func (transfer);
    }

  _free_device_hash (device_hash);

  if (oldfromdir != NULL)
    {
      ret = gftp_set_directory (transfer->fromreq, oldfromdir);
      if (ret == GFTP_EFATAL)
        {
          _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                    update_func);
          return (ret);
        }
    }

  if (oldtodir != NULL)
    {
      ret = gftp_set_directory (transfer->toreq, oldtodir);
      if (ret == GFTP_EFATAL)
        {
          _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir,
                                    update_func);
          return (ret);
        }
    }

  _cleanup_get_all_subdirs (transfer, oldfromdir, oldtodir, update_func);

  return (0);
}


int
gftp_set_config_options (gftp_request * request)
{
  if (request->set_config_options != NULL)
    return (request->set_config_options (request));
  else
    return (0);
}


void
print_file_list (GList * list)
{
  gftp_file * tempfle;
  GList * templist;
  char *attribs;

  printf ("--START OF FILE LISTING - TOP TO BOTTOM--\n");
  for (templist = list; ; templist = templist->next)
    {
      tempfle = templist->data;
      attribs = gftp_convert_attributes_from_mode_t (tempfle->st_mode);

      printf ("%s:%s:" GFTP_OFF_T_PRINTF_MOD ":" GFTP_OFF_T_PRINTF_MOD ":%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, attribs);

      g_free (attribs);
      if (templist->next == NULL)
        break;
    }

  printf ("--START OF FILE LISTING - BOTTOM TO TOP--\n");
  for (; ; templist = templist->prev)
    {
      tempfle = templist->data;
      attribs = gftp_convert_attributes_from_mode_t (tempfle->st_mode);

      printf ("%s:%s:" GFTP_OFF_T_PRINTF_MOD ":" GFTP_OFF_T_PRINTF_MOD ":%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, attribs);

      g_free (attribs);
      if (templist == list)
        break;
    }
  printf ("--END OF FILE LISTING--\n");
}


void
gftp_swap_socks (gftp_request * dest, gftp_request * source)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (source != NULL);
  g_return_if_fail (dest->protonum == source->protonum);

  dest->datafd = source->datafd;
  dest->cached = 0;
#ifdef USE_SSL
  dest->ssl = source->ssl;
#endif

  if (!source->always_connected)
    {
      source->datafd = -1;
      source->cached = 1;
#ifdef USE_SSL
      source->ssl = NULL;
#endif
    }

  if (dest->swap_socks != NULL)
    dest->swap_socks (dest, source);
}


void
gftp_calc_kbs (gftp_transfer * tdata, ssize_t num_read)
{
  /* Needed for systems that size(float) < size(void *) */
  union { intptr_t i; float f; } maxkbs;
  unsigned long waitusecs;
  double start_difftime;
  struct timeval tv;
  int waited;

  gftp_lookup_request_option (tdata->fromreq, "maxkbs", &maxkbs.f);

  if (g_thread_supported ())
    g_static_mutex_lock (&tdata->statmutex);

  gettimeofday (&tv, NULL);

  tdata->trans_bytes += num_read;
  tdata->curtrans += num_read;
  tdata->stalled = 0;

  start_difftime = (tv.tv_sec - tdata->starttime.tv_sec) + ((double) (tv.tv_usec - tdata->starttime.tv_usec) / 1000000.0);

  if (start_difftime <= 0)
    tdata->kbs = tdata->trans_bytes / 1024.0;
  else
    tdata->kbs = tdata->trans_bytes / 1024.0 / start_difftime;

  waited = 0;
  if (maxkbs.f > 0 && tdata->kbs > maxkbs.f)
    {
      waitusecs = num_read / 1024.0 / maxkbs.f * 1000000.0 - start_difftime;

      if (waitusecs > 0)
        {
          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->statmutex);

          waited = 1;
          usleep (waitusecs);

          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->statmutex);
        }

    }

  if (waited)
    gettimeofday (&tdata->lasttime, NULL);
  else
    memcpy (&tdata->lasttime, &tv, sizeof (tdata->lasttime));

  if (g_thread_supported ())
    g_static_mutex_unlock (&tdata->statmutex);
}


static int
_do_sleep (int sleep_time)
{
  struct timeval tv;

  tv.tv_sec = sleep_time;
  tv.tv_usec = 0;

  return (select (0, NULL, NULL, NULL, &tv));
}


int
gftp_get_transfer_status (gftp_transfer * tdata, ssize_t num_read)
{
  intptr_t retries, sleep_time;
  gftp_file * tempfle;
  int ret1, ret2;

  gftp_lookup_request_option (tdata->fromreq, "retries", &retries);
  gftp_lookup_request_option (tdata->fromreq, "sleep_time", &sleep_time);

  if (g_thread_supported ())
    g_static_mutex_lock (&tdata->structmutex);

  if (tdata->curfle == NULL)
    {
      if (g_thread_supported ())
        g_static_mutex_unlock (&tdata->structmutex);

      return (GFTP_EFATAL);
    }

  tempfle = tdata->curfle->data;

  if (g_thread_supported ())
    g_static_mutex_unlock (&tdata->structmutex);

  gftp_disconnect (tdata->fromreq);
  gftp_disconnect (tdata->toreq);

  if (tdata->cancel || num_read == GFTP_EFATAL)
    return (GFTP_EFATAL);
  else if (num_read >= 0 && !tdata->skip_file)
    return (0);

  if (num_read != GFTP_ETIMEDOUT && !tdata->conn_error_no_timeout)
    {
      if (retries != 0 && 
          tdata->current_file_retries >= retries)
        {
          tdata->fromreq->logging_function (gftp_logging_error, tdata->fromreq,
                   _("Error: Remote site %s disconnected. Max retries reached...giving up\n"),
                   tdata->fromreq->hostname != NULL ? 
                         tdata->fromreq->hostname : tdata->toreq->hostname);
          return (GFTP_EFATAL);
        }
      else
        {
          tdata->fromreq->logging_function (gftp_logging_error, tdata->fromreq,
                     _("Error: Remote site %s disconnected. Will reconnect in %d seconds\n"),
                     tdata->fromreq->hostname != NULL ? 
                         tdata->fromreq->hostname : tdata->toreq->hostname, 
                     sleep_time);
        }
    }

  while (retries == 0 || 
         tdata->current_file_retries <= retries)
    {
      /* Look up the options in case the user changes them... */
      gftp_lookup_request_option (tdata->fromreq, "retries", &retries);
      gftp_lookup_request_option (tdata->fromreq, "sleep_time", &sleep_time);

      if (num_read != GFTP_ETIMEDOUT && !tdata->conn_error_no_timeout &&
          !tdata->skip_file)
        _do_sleep (sleep_time);

      tdata->current_file_retries++;

      ret1 = ret2 = 0;
      if ((ret1 = gftp_connect (tdata->fromreq)) == 0 &&
          (ret2 = gftp_connect (tdata->toreq)) == 0)
        {
          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          tdata->resumed_bytes = tdata->resumed_bytes + tdata->trans_bytes - tdata->curresumed - tdata->curtrans;
          tdata->trans_bytes = 0;
          if (tdata->skip_file)
            {
              tdata->total_bytes -= tempfle->size;
              tdata->curtrans = 0;

              tdata->curfle = tdata->curfle->next;
              tdata->next_file = 1;
              tdata->skip_file = 0;
              tdata->cancel = 0;
              tdata->fromreq->cancel = 0;
              tdata->toreq->cancel = 0;
            }
          else
            {
              tempfle->transfer_action = GFTP_TRANS_ACTION_RESUME;
              tempfle->startsize = tdata->curtrans + tdata->curresumed;
              /* We decrement this here because it will be incremented in 
                 the loop again */
              tdata->curresumed = 0;
              tdata->current_file_number--; /* Decrement this because it 
                                               will be incremented when we 
                                               continue in the loop */
            }

          gettimeofday (&tdata->starttime, NULL);

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);

          return (GFTP_ERETRYABLE);
        }
      else if (ret1 == GFTP_EFATAL || ret2 == GFTP_EFATAL)
        {
          gftp_disconnect (tdata->fromreq);
          gftp_disconnect (tdata->toreq);
          return (GFTP_EFATAL);
        }
    }

  return (0);
}


int
gftp_fd_open (gftp_request * request, const char *pathname, int flags, mode_t mode)
{
  int fd;

  if (mode == 0)
    fd = open (pathname, flags);
  else
    fd = open (pathname, flags, mode);

  if (fd < 0)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                                   _("Error: Cannot open local file %s: %s\n"),
                                   pathname, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  if (fcntl (fd, F_SETFD, 1) == -1)
    {
      if (request != NULL)
        request->logging_function (gftp_logging_error, request,
                                   _("Error: Cannot set close on exec flag: %s\n"),
                                   g_strerror (errno));

      return (-1);
    }

  return (fd);
}


void
gftp_setup_startup_directory (gftp_request * request, const char *option_name)
{
  char *startup_directory, *tempstr;

  gftp_lookup_request_option (request, option_name, &startup_directory);

  if (*startup_directory != '\0' &&
      (tempstr = gftp_expand_path (request, startup_directory)) != NULL)
    {
      gftp_set_directory (request, tempstr);
      g_free (tempstr);
    }
}


char *
gftp_convert_attributes_from_mode_t (mode_t mode)
{
  char *str;

  str = g_malloc0 (11UL);
  
  str[0] = '?';
  if (S_ISREG (mode))
    str[0] = '-';

  if (S_ISLNK (mode))
    str[0] = 'l';

  if (S_ISBLK (mode))
    str[0] = 'b';

  if (S_ISCHR (mode))
    str[0] = 'c';

  if (S_ISFIFO (mode))
    str[0] = 'p';

  if (S_ISSOCK (mode))
    str[0] = 's';

  if (S_ISDIR (mode))
    str[0] = 'd';

  str[1] = mode & S_IRUSR ? 'r' : '-';
  str[2] = mode & S_IWUSR ? 'w' : '-';

  if ((mode & S_ISUID) && (mode & S_IXUSR))
    str[3] = 's';
  else if (mode & S_ISUID)
    str[3] = 'S';
  else if (mode & S_IXUSR)
    str[3] = 'x';
  else
    str[3] = '-';
    
  str[4] = mode & S_IRGRP ? 'r' : '-';
  str[5] = mode & S_IWGRP ? 'w' : '-';

  if ((mode & S_ISGID) && (mode & S_IXGRP))
    str[6] = 's';
  else if (mode & S_ISGID)
    str[6] = 'S';
  else if (mode & S_IXGRP)
    str[6] = 'x';
  else
    str[6] = '-';

  str[7] = mode & S_IROTH ? 'r' : '-';
  str[8] = mode & S_IWOTH ? 'w' : '-';

  if ((mode & S_ISVTX) && (mode & S_IXOTH))
    str[9] = 't';
  else if (mode & S_ISVTX)
    str[9] = 'T';
  else if (mode & S_IXOTH)
    str[9] = 'x';
  else
    str[9] = '-';

  return (str);
}


mode_t
gftp_convert_attributes_to_mode_t (char *attribs)
{
  mode_t mode;

  if (attribs[0] == 'd')
    mode = S_IFDIR;
  else if (attribs[0] == 'l')
    mode = S_IFLNK;
  else if (attribs[0] == 's')
    mode = S_IFSOCK;
  else if (attribs[0] == 'b')
    mode = S_IFBLK;
  else if (attribs[0] == 'c')
    mode = S_IFCHR;
  else
    mode = S_IFREG;

  if (attribs[1] == 'r')
    mode |= S_IRUSR;
  if (attribs[2] == 'w')
    mode |= S_IWUSR;
  if (attribs[3] == 'x' || attribs[3] == 's')
    mode |= S_IXUSR;
  if (attribs[3] == 's' || attribs[3] == 'S')
    mode |= S_ISUID;

  if (attribs[4] == 'r')
    mode |= S_IRGRP;
  if (attribs[5] == 'w')
    mode |= S_IWGRP;
  if (attribs[6] == 'x' ||
      attribs[6] == 's')
    mode |= S_IXGRP;
  if (attribs[6] == 's' || attribs[6] == 'S')
    mode |= S_ISGID;

  if (attribs[7] == 'r')
    mode |= S_IROTH;
  if (attribs[8] == 'w')
    mode |= S_IWOTH;
  if (attribs[9] == 'x' ||
      attribs[9] == 's')
    mode |= S_IXOTH;
  if (attribs[9] == 't' || attribs[9] == 'T')
    mode |= S_ISVTX;

  return (mode);
}


unsigned int
gftp_protocol_default_port (gftp_request * request)
{
  struct servent serv_struct;

  if (r_getservbyname (gftp_protocols[request->protonum].url_prefix, "tcp",
                       &serv_struct, NULL) == NULL)
    return (gftp_protocols[request->protonum].default_port);
  else
    return (ntohs (serv_struct.s_port));
}

