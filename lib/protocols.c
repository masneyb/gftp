/*****************************************************************************/
/*  protocols.c - Skeleton functions for the protocols gftp supports         */
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
    {
      memset (request->password, 0, strlen (request->password));
      g_free (request->password);
    }
  if (request->account)
    {
      memset (request->account, 0, strlen (request->account));
      g_free (request->account);
    }
  if (request->directory)
    g_free (request->directory);
  if (request->last_ftp_response)
    g_free (request->last_ftp_response);
  if (request->protocol_data)
    g_free (request->protocol_data);

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
gftp_file_destroy (gftp_file * file)
{
  g_return_if_fail (file != NULL);

  if (file->file)
    g_free (file->file);
  if (file->utf8_file)
    g_free (file->utf8_file);
  if (file->user)
    g_free (file->user);
  if (file->group)
    g_free (file->group);
  if (file->attribs)
    g_free (file->attribs);
  if (file->destfile)
    g_free (file->destfile);
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

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  if (request->free_hostp && request->hostp != NULL)
    freeaddrinfo (request->hostp);
#endif
  request->hostp = NULL;

#ifdef USE_SSL
  if (request->ssl != NULL)
    {
      SSL_free (request->ssl);
      request->ssl = NULL;
    }
#endif

#if GLIB_MAJOR_VERSION > 1
  if (request->iconv_initialized)
    {
      g_iconv_close (request->iconv);
      request->iconv_initialized = 0;
    }
#endif

  request->cached = 0;
  if (request->disconnect == NULL)
    return;
  request->disconnect (request);
}


off_t
gftp_get_file (gftp_request * request, const char *filename, int fd,
               off_t startsize)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->cached = 0;
  if (request->get_file == NULL)
    return (GFTP_EFATAL);

  return (request->get_file (request, filename, fd, startsize));
}


int
gftp_put_file (gftp_request * request, const char *filename, int fd,
               off_t startsize, off_t totalsize)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->cached = 0;
  if (request->put_file == NULL)
    return (GFTP_EFATAL);
  return (request->put_file (request, filename, fd, startsize, totalsize));
}


off_t
gftp_transfer_file (gftp_request * fromreq, const char *fromfile, 
                    int fromfd, off_t fromsize, 
                    gftp_request * toreq, const char *tofile,
                    int tofd, off_t tosize)
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
  size = gftp_get_file (fromreq, fromfile, fromfd, tosize);
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
  ret = gftp_put_file (toreq, tofile, tofd, tosize, size);
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

  if (size == 0)
    return (0);

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

  return (request->abort_transfer (request));
}


int
gftp_list_files (gftp_request * request)
{
  char *remote_lc_time, *locret;
  int fd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  gftp_lookup_request_option (request, "remote_lc_time", &remote_lc_time);
  if (remote_lc_time != NULL && *remote_lc_time != '\0')
    locret = setlocale (LC_TIME, remote_lc_time);
  else
    locret = setlocale (LC_TIME, NULL);

  if (locret == NULL)
    {
      locret = setlocale (LC_TIME, NULL);
      request->logging_function (gftp_logging_misc, request,
                                 _("Error setting LC_TIME to '%s'. Falling back to '%s'\n"),
                                 remote_lc_time, locret);
    }

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


#if GLIB_MAJOR_VERSION > 1

static char *
_gftp_get_next_charset (char **curpos)
{
  char *ret, *endpos;

  if (**curpos == '\0')
    return (NULL);

  ret = *curpos;
  if ((endpos = strchr (*curpos, ',')) == NULL)
    *curpos += strlen (*curpos);
  else
    {
      *endpos = '\0';
      *curpos = endpos + 1;
    }

  return (ret);
}


char *
gftp_string_to_utf8 (gftp_request * request, const char *str)
{
  char *ret, *remote_charsets, *stpos, *cur_charset, *tempstr;
  gsize bread, bwrite;
  GError * error;

  if (request == NULL)
    return (NULL);

  if (request->iconv_initialized)
    return (g_convert_with_iconv (str, -1, request->iconv, &bread, &bwrite, 
                                  &error));
  else if (g_utf8_validate (str, -1, NULL))
    return (NULL);

  gftp_lookup_request_option (request, "remote_charsets", &tempstr);
  if (*tempstr == '\0')
    {
      error = NULL;
      if ((ret = g_locale_to_utf8 (str, -1, &bread, &bwrite, &error)) != NULL)
        return (ret);

      return (NULL);
    }

  remote_charsets = g_strdup (tempstr);
  ret = NULL;
  stpos = remote_charsets;
  while ((cur_charset = _gftp_get_next_charset (&stpos)) != NULL)
    {
      if ((request->iconv = g_iconv_open ("UTF-8", cur_charset)) == (GIConv) -1)
        continue;

      error = NULL;
      if ((ret = g_convert_with_iconv (str, -1, request->iconv, &bread, &bwrite,
                                       &error)) == NULL)
        {
          g_iconv_close (request->iconv);
          request->iconv = NULL;
          continue;
        }
      else
        {
          request->iconv_initialized = 1;
          break;
        }
    }

  g_free (remote_charsets);

  return (ret);
}


char *
gftp_string_from_utf8 (gftp_request * request, const char *str)
{
  char *ret, *remote_charsets, *stpos, *cur_charset, *tempstr;
  gsize bread, bwrite;
  GError * error;

  if (request == NULL)
    return (NULL);

  if (request->iconv_initialized)
    return (g_convert_with_iconv (str, -1, request->iconv, &bread, &bwrite, 
                                  &error));
  else if (g_utf8_validate (str, -1, NULL))
    return (NULL);

  gftp_lookup_request_option (request, "remote_charsets", &tempstr);
  if (*tempstr == '\0')
    {
      error = NULL;
      if ((ret = g_locale_from_utf8 (str, -1, &bread, &bwrite, &error)) != NULL)
        return (ret);

      return (NULL);
    }

  remote_charsets = g_strdup (tempstr);
  ret = NULL;
  stpos = remote_charsets;
  while ((cur_charset = _gftp_get_next_charset (&stpos)) != NULL)
    {
      if ((request->iconv = g_iconv_open (cur_charset, "UTF-8")) == (GIConv) -1)
        continue;

      error = NULL;
      if ((ret = g_convert_with_iconv (str, -1, request->iconv, &bread, &bwrite,
                                       &error)) == NULL)
        {
          g_iconv_close (request->iconv);
          request->iconv = NULL;
          continue;
        }
      else
        {
          request->iconv_initialized = 1;
          break;
        }
    }

  g_free (remote_charsets);

  return (ret);
}

#else

char *
gftp_string_to_utf8 (gftp_request * request, const char *str)
{
  return (NULL);
}


char *
gftp_string_from_utf8 (gftp_request * request, const char *str)
{
  return (NULL);
}

#endif


int
gftp_get_next_file (gftp_request * request, const char *filespec,
                    gftp_file * fle)
{
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
      gftp_file_destroy (fle);
      ret = request->get_next_file (request, fle, fd);

      if (ret >= 0 && fle->file != NULL)
        fle->utf8_file = gftp_string_to_utf8 (request, fle->file);

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
    } while (ret > 0 && !gftp_match_filespec (fle->file, filespec));

  return (ret);
}


int
gftp_parse_bookmark (gftp_request * request, gftp_request * local_request, 
                     const char * bookmark, int *refresh_local)
{
  gftp_logging_func logging_function;
  gftp_bookmarks_var * tempentry;
  char *default_protocol;
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
    gftp_set_password (request, tempentry->pass);

  if (tempentry->acct != NULL)
    gftp_set_account (request, tempentry->acct);

  gftp_set_hostname (request, tempentry->hostname);
  gftp_set_directory (request, tempentry->remote_dir);
  gftp_set_port (request, tempentry->port);

  if (local_request != NULL &&
      tempentry->local_dir != NULL &&
      *tempentry->local_dir != '\0')
    {
      gftp_set_directory (local_request, tempentry->local_dir);
      *refresh_local = 1;
    }
  else
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
  char *pos, *endpos, *endhostpos, tempchar, *default_protocol, *stpos;
  gftp_logging_func logging_function;
  int len, i, init_ret;
  const char *cpos;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (url != NULL, GFTP_EFATAL);

  logging_function = request->logging_function;
  gftp_request_destroy (request, 0);
  request->logging_function = logging_function;

  for (cpos = url; *cpos == ' '; cpos++);
  stpos = g_strdup (cpos);
  for (pos = stpos + strlen (stpos) - 1; *pos == ' '; pos--)
    *pos = '\0';

  i = GFTP_FTP_NUM;

  if ((pos = strstr (stpos, "://")) != NULL)
    {
      *pos = '\0';

      for (i = 0; gftp_protocols[i].url_prefix; i++)
        {
          if (strcmp (gftp_protocols[i].url_prefix, stpos) == 0)
            break;
        }

      if (gftp_protocols[i].url_prefix == NULL)
        {
          request->logging_function (gftp_logging_misc, NULL, 
                                     _("The protocol '%s' is currently not supported.\n"),
                                     stpos);
          g_free (stpos);
          return (GFTP_EFATAL);
        }

      *pos = ':';
    }
  else
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
    }

  if (gftp_protocols[i].url_prefix == NULL)
    i = GFTP_FTP_NUM;

  if ((init_ret = gftp_protocols[i].init (request)) < 0)
    {
      gftp_request_destroy (request, 0);
      return (init_ret);
    }

  if (request->parse_url != NULL)
    {
      g_free (stpos);
      return (request->parse_url (request, url));
    }

  pos = stpos;
  len = strlen (request->url_prefix);
  if (strncmp (pos, request->url_prefix, len) == 0 && 
      strncmp (pos + len, "://", 3) == 0)
    pos += len + 3;

  if (i == GFTP_LOCAL_NUM)
    {
      for (; *pos == ' ' || *pos == '\t'; pos++);
      request->directory = g_strdup (pos);
      g_free (stpos);
      return (0);
    }

  if ((endhostpos = strrchr (pos, '@')) != NULL)
    {
      /* A user/password was entered */
      if ((endpos = strchr (pos, ':')) == NULL || endhostpos < endpos)
        {
          /* No password was entered */
          gftp_set_password (request, "");
          endpos = endhostpos;
        }

      *endpos = '\0';
      gftp_set_username (request, pos);

      pos = endpos + 1;
      if ((endpos = strchr (pos, '@')) != NULL)
        {
          if (strchr (endpos + 1, '@') != NULL)
            endpos = strchr (endpos + 1, '@');
          *endpos = '\0';
          gftp_set_password (request, pos);

          pos = endpos + 1;
        }
    }

  /* Now get the hostname and an optional port and optional directory */
  endhostpos = pos + strlen (pos);
  if ((endpos = strchr (pos, ':')) != NULL)
    endhostpos = endpos;
  else if ((endpos = strchr (pos, '/')) != NULL)
    endhostpos = endpos;
  tempchar = *endhostpos;
  *endhostpos = '\0';
  gftp_set_hostname (request, pos);
  *endhostpos = tempchar;

  if ((endpos = strchr (pos, ':')) != NULL)
    {
      /* A port was entered */
      pos = endpos + 1;
      gftp_set_port (request, strtol (pos, NULL, 10));
    }
  if ((endpos = strchr (pos, '/')) != NULL)
    gftp_set_directory (request, endpos);
  g_free (stpos);
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
  g_return_if_fail (username != NULL);

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
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->mkdir == NULL)
    return (GFTP_EFATAL);

  utf8 = gftp_string_from_utf8 (request, directory);
  if (utf8 != NULL)
    {
      ret = request->mkdir (request, utf8);
      g_free (utf8);
    }
  else
    ret = request->mkdir (request, directory);

  return (ret);
}


int
gftp_rename_file (gftp_request * request, const char *oldname,
                  const char *newname)
{
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->rename == NULL)
    return (GFTP_EFATAL);

  utf8 = gftp_string_from_utf8 (request, newname);
  if (utf8 != NULL)
    {
      ret = request->rename (request, oldname, utf8);
      g_free (utf8);
    }
  else
    ret = request->rename (request, oldname, newname);

  return (ret);
}


int
gftp_chmod (gftp_request * request, const char *file, int mode)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if (request->chmod == NULL)
    return (GFTP_EFATAL);
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


char
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


static int
gftp_need_proxy (gftp_request * request, char *service, char *proxy_hostname, 
                 int proxy_port)
{
  gftp_config_list_vars * proxy_hosts;
  gftp_proxy_hosts * hostname;
  size_t hostlen, domlen;
  unsigned char addy[4];
  struct sockaddr *addr;
  GList * templist;
  gint32 netaddr;
  char *pos;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo hints;
  int port, errnum;
  char serv[8];
#endif

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);

  if (proxy_hostname == NULL || *proxy_hostname == '\0')
    return (0);
  else if (proxy_hosts->list == NULL)
    return (proxy_hostname != NULL && 
            *proxy_hostname != '\0');

  request->hostp = NULL;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  request->free_hostp = 1;
  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  port = request->use_proxy ? proxy_port : request->port;
  if (port == 0)
    strcpy (serv, service);
  else
    snprintf (serv, sizeof (serv), "%d", port);

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), request->hostname);

  if ((errnum = getaddrinfo (request->hostname, serv, &hints, 
                             &request->hostp)) != 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, gai_strerror (errnum));
      return (GFTP_ERETRYABLE);
    }

  addr = request->hostp->ai_addr;

#else /* !HAVE_GETADDRINFO */
  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), request->hostname);

  if (!(request->hostp = r_gethostbyname (request->hostname, &request->host,
                                          NULL)))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  addr = (struct sockaddr *) request->host.h_addr_list[0];

#endif /* HAVE_GETADDRINFO */

  templist = proxy_hosts->list;
  while (templist != NULL)
    {
      hostname = templist->data;
      if (hostname->domain != NULL)
        {
           hostlen = strlen (request->hostname);
           domlen = strlen (hostname->domain);
           if (hostlen > domlen)
             {
                pos = request->hostname + hostlen - domlen;
                if (strcmp (hostname->domain, pos) == 0)
                  return (0);
             }
        }

      if (hostname->ipv4_network_address != 0)
        {
          memcpy (addy, addr, sizeof (addy));
          netaddr =
            (((addy[0] & 0xff) << 24) | ((addy[1] & 0xff) << 16) |
             ((addy[2] & 0xff) << 8) | (addy[3] & 0xff)) & 
             hostname->ipv4_netmask;
          if (netaddr == hostname->ipv4_network_address)
            return (0);
        }
      templist = templist->next;
    }

  return (proxy_hostname != NULL && *proxy_hostname != '\0');
}


static char *
copy_token (char **dest, char *source)
{
  /* This function is used internally by gftp_parse_ls () */
  char *endpos, savepos;

  endpos = source;
  while (*endpos != ' ' && *endpos != '\t' && *endpos != '\0')
    endpos++;
  if (*endpos == '\0')
    return (NULL);

  savepos = *endpos;
  *endpos = '\0';
  *dest = g_malloc (endpos - source + 1);
  strcpy (*dest, source);
  *endpos = savepos;

  /* Skip the blanks till we get to the next entry */
  source = endpos + 1;
  while ((*source == ' ' || *source == '\t') && *source != '\0')
    source++;
  return (source);
}


static char *
goto_next_token (char *pos)
{
  while (*pos != ' ' && *pos != '\t' && *pos != '\0')
    pos++;

  if (pos == '\0')
    return (pos);

  while ((*pos == ' ' || *pos == '\t') && *pos != '\0')
    pos++;

  return (pos);
}


time_t
parse_time (char *str, char **endpos)
{
  struct tm curtime, *loctime;
  time_t t, ret;
  char *tmppos;
  size_t slen;
  int i, num;

  slen = strlen (str);
  memset (&curtime, 0, sizeof (curtime));
  curtime.tm_isdst = -1;

  if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && 
      isdigit ((int) str[3]))
    {
      /* This is how DOS will return the date/time */
      /* 07-06-99  12:57PM */

      tmppos = strptime (str, "%m-%d-%y %I:%M%p", &curtime);
    }
  else if (slen > 4 && isdigit ((int) str[0]) && str[2] == '-' && 
           isalpha (str[3]))
    {
      /* 10-Jan-2003 09:14 */
      tmppos = strptime (str, "%d-%h-%Y %H:%M", &curtime);
    }
  else if (slen > 4 && isdigit ((int) str[0]) && str[4] == '/')
    {
      /* 2003/12/25 */
      tmppos = strptime (str, "%Y/%m/%d", &curtime);
    }
  else
    {
      /* This is how most UNIX, Novell, and MacOS ftp servers send their time */
      /* Jul 06 12:57 or Jul  6  1999 */

      if (strchr (str, ':') != NULL)
        {
          tmppos = strptime (str, "%h %d %H:%M", &curtime);
          t = time (NULL);
          loctime = localtime (&t);

          if (curtime.tm_mon > loctime->tm_mon)
            curtime.tm_year = loctime->tm_year - 1;
          else
            curtime.tm_year = loctime->tm_year;
        }
      else
        tmppos = strptime (str, "%h %d %Y", &curtime);
    }

  if (tmppos != NULL)
    ret = mktime (&curtime);
  else
    ret = 0;

  if (endpos != NULL)
    {
      if (tmppos == NULL)
        {
          /* We cannot parse this date format. So, just skip this date field
             and continue to the next token. This is mainly for the HTTP 
             support */

          *endpos = str;
          for (num = 0; num < 2 && **endpos != '\0'; num++)
            {
              for (i=0; 
                   (*endpos)[i] != ' ' && (*endpos)[i] != '\t' && 
                    (*endpos)[i] != '\0'; 
                   i++);
              *endpos += i;

              for (i=0; (*endpos)[i] == ' ' || (*endpos)[i] == '\t'; i++);
              *endpos += i;
            }
        }
      else
        *endpos = tmppos;
    }

  return (ret);
}


static void 
gftp_parse_vms_attribs (char *dest, char **src)
{
  char *endpos;

  if (*src == NULL)
    return;

  if ((endpos = strchr (*src, ',')) != NULL)
    *endpos = '\0';

  if (strchr (*src, 'R') != NULL)
    *dest = 'r';
  if (strchr (*src, 'W') != NULL)
    *(dest + 1) = 'w';
  if (strchr (*src, 'E') != NULL)
    *(dest + 2) = 'x';

  *src = endpos + 1;
}


static int
gftp_parse_ls_vms (char *str, gftp_file * fle)
{
  char *curpos, *endpos;

  /* .PINE-DEBUG1;1              9  21-AUG-2002 20:06 [MYERSRG] (RWED,RWED,,) */
  /* WWW.DIR;1                   1  23-NOV-1999 05:47 [MYERSRG] (RWE,RWE,RE,E) */

  if ((curpos = strchr (str, ';')) == NULL)
    return (GFTP_EFATAL);

  *curpos = '\0';
  if (strlen (str) > 4 && strcmp (curpos - 4, ".DIR") == 0)
    {
      fle->isdir = 1;
      fle->attribs = g_strdup ("drwxr-xr-x");
      *(curpos - 4) = '\0';
    }
  else
   fle->attribs = g_strdup ("-rw-r--r--");

  fle->file = g_strdup (str);

  curpos = goto_next_token (curpos + 1);
  fle->size = gftp_parse_file_size (curpos) * 512; /* Is this correct? */
  curpos = goto_next_token (curpos);

  fle->datetime = parse_time (curpos, &curpos);

  curpos = goto_next_token (curpos);

  if (*curpos != '[')
    return (GFTP_EFATAL);
  curpos++;
  if ((endpos = strchr (curpos, ']')) == NULL)
    return (GFTP_EFATAL);
  *endpos = '\0';
  fle->user = g_strdup (curpos);
  fle->group = g_strdup ("");

  curpos = goto_next_token (endpos + 1);
  if ((curpos = strchr (curpos, ',')) == NULL)
    return (0);
  curpos++;

  gftp_parse_vms_attribs (fle->attribs + 1, &curpos);
  gftp_parse_vms_attribs (fle->attribs + 4, &curpos);
  gftp_parse_vms_attribs (fle->attribs + 7, &curpos);

  return (0);
}


static int
gftp_parse_ls_mvs (char *str, gftp_file * fle)
{
  char *curpos;

  /* Volume Unit    Referred Ext Used Recfm Lrecl BlkSz Dsorg Dsname */
  /* SVI52A 3390   2003/12/10  8  216  FB      80 27920  PS  CARDS.DELETES */
  /* SVI528 3390   2003/12/12  1    5  FB      80 24000  PO  CLIST */

  curpos = goto_next_token (str + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  fle->datetime = parse_time (curpos, &curpos);

  curpos = goto_next_token (curpos);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  fle->size = gftp_parse_file_size (curpos) * 55996; 
  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);
  if (curpos == NULL)
    return (GFTP_EFATAL);

  if (strncmp (curpos, "PS", 2) == 0)
    fle->attribs = g_strdup ("-rw-r--r--");
  else if (strncmp (curpos, "PO", 2) == 0)
    fle->attribs = g_strdup ("drwxr-xr-x");
  else
    return (GFTP_EFATAL);

  curpos = goto_next_token (curpos + 1);

  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));
  fle->file = g_strdup (curpos);

  return (0);
}
  

static int
gftp_parse_ls_eplf (char *str, gftp_file * fle)
{
  char *startpos;
  int isdir = 0;

  startpos = str;
  while (startpos)
    {
      startpos++;
      switch (*startpos)
        {
        case '/':
          isdir = 1;
          break;
        case 's':
          fle->size = gftp_parse_file_size (startpos + 1);
          break;
        case 'm':
          fle->datetime = strtol (startpos + 1, NULL, 10);
          break;
        }
      startpos = strchr (startpos, ',');
    }

  if ((startpos = strchr (str, 9)) == NULL)
    return (GFTP_EFATAL);

  if (isdir)
    fle->attribs = g_strdup ("drwxr-xr-x");
  else
    fle->attribs = g_strdup ("-rw-r--r--");

  fle->file = g_strdup (startpos + 1);
  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));
  return (0);
}


static int
gftp_parse_ls_unix (gftp_request * request, char *str, size_t slen,
                    gftp_file * fle)
{
  char *endpos, *startpos, *pos;
  int cols;

  /* If there is no space between the attribs and links field, just make one */
  if (slen > 10)
    str[10] = ' ';

  /* Determine the number of columns */
  cols = 0;
  pos = str;
  while (*pos != '\0')
    {
      while (*pos != '\0' && *pos != ' ' && *pos != '\t')
        {
          if (*pos == ':')
            break;
          pos++;
        }

      cols++;

      if (*pos == ':')
        {
          cols++;
          break;
        }

      while (*pos == ' ' || *pos == '\t')
        pos++;
    }

  startpos = str;
  /* Copy file attributes */
  if ((startpos = copy_token (&fle->attribs, startpos)) == NULL)
    return (GFTP_EFATAL);

  if (cols >= 9)
    {
      /* Skip the number of links */
      startpos = goto_next_token (startpos);

      /* Copy the user that owns this file */
      if ((startpos = copy_token (&fle->user, startpos)) == NULL)
        return (GFTP_EFATAL);

      /* Copy the group that owns this file */
      if ((startpos = copy_token (&fle->group, startpos)) == NULL)
        return (GFTP_EFATAL);
    }
  else
    {
      fle->group = g_strdup (_("unknown"));
      if (cols == 8)
        {
          if ((startpos = copy_token (&fle->user, startpos)) == NULL)
            return (GFTP_EFATAL);
        }
      else
        fle->user = g_strdup (_("unknown"));
      startpos = goto_next_token (startpos);
    }

  if (request->server_type == GFTP_DIRTYPE_CRAY)
    {
      /* See if this is a Cray directory listing. It has the following format:
      drwx------     2 feiliu    g913     DK  common      4096 Sep 24  2001 wv */
      if (cols == 11 && strstr (str, "->") == NULL)
        {
          startpos = goto_next_token (startpos);
          startpos = goto_next_token (startpos);
        }
    }

  /* See if this is a block or character device. We will store the major number
     in the high word and the minor number in the low word.  */
  if ((fle->attribs[0] == 'b' || fle->attribs[0] == 'u' || 
       fle->attribs[0] == 'c') &&
      ((endpos = strchr (startpos, ',')) != NULL))
    {
      fle->size = strtol (startpos, NULL, 10) << 16;

      startpos = endpos + 1;
      while (*startpos == ' ')
        startpos++;

      /* Get the minor number */
      if ((endpos = strchr (startpos, ' ')) == NULL)
        return (GFTP_EFATAL);
      fle->size |= strtol (startpos, NULL, 10) & 0xFF;
    }
  else
    {
      /* This is a regular file  */
      if ((endpos = strchr (startpos, ' ')) == NULL)
        return (GFTP_EFATAL);
      fle->size = gftp_parse_file_size (startpos);
    }

  /* Skip the blanks till we get to the next entry */
  startpos = endpos + 1;
  while (*startpos == ' ')
    startpos++;

  fle->datetime = parse_time (startpos, &startpos);

  /* Skip the blanks till we get to the next entry */
  startpos = goto_next_token (startpos);

  /* Parse the filename. If this file is a symbolic link, remove the -> part */
  if (fle->attribs[0] == 'l' && ((endpos = strstr (startpos, "->")) != NULL))
    *(endpos - 1) = '\0';

  fle->file = g_strdup (startpos);

  /* Uncomment this if you want to strip the spaces off of the end of the file.
     I don't want to do this by default since there are valid filenames with
     spaces at the end of them. Some broken FTP servers like the Paradyne IPC
     DSLAMS append a bunch of spaces at the end of the file.
  for (endpos = fle->file + strlen (fle->file) - 1; 
       *endpos == ' '; 
       *endpos-- = '\0');
  */

  return (0);
}


static int
gftp_parse_ls_nt (char *str, gftp_file * fle)
{
  char *startpos;

  startpos = str;
  fle->datetime = parse_time (startpos, &startpos);

  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));

  startpos = goto_next_token (startpos);
  if (startpos[0] == '<')
    fle->attribs = g_strdup ("drwxrwxrwx");
  else
    {
      fle->attribs = g_strdup ("-rw-rw-rw-");
      fle->size = gftp_parse_file_size (startpos);
    }

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}


static int
gftp_parse_ls_novell (char *str, gftp_file * fle)
{
  char *startpos;

  if (str[12] != ' ')
    return (GFTP_EFATAL);
  str[12] = '\0';
  fle->attribs = g_strdup (str);
  startpos = str + 13;

  while ((*startpos == ' ' || *startpos == '\t') && *startpos != '\0')
    startpos++;

  if ((startpos = copy_token (&fle->user, startpos)) == NULL)
    return (GFTP_EFATAL);

  fle->group = g_strdup (_("unknown"));

  fle->size = gftp_parse_file_size (startpos);

  startpos = goto_next_token (startpos);
  fle->datetime = parse_time (startpos, &startpos);

  startpos = goto_next_token (startpos);
  fle->file = g_strdup (startpos);
  return (0);
}


int
gftp_parse_ls (gftp_request * request, const char *lsoutput, gftp_file * fle)
{
  char *str, *endpos, tmpchar;
  int result, is_vms;
  size_t len;

  g_return_val_if_fail (lsoutput != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  str = g_strdup (lsoutput);
  memset (fle, 0, sizeof (*fle));

  len = strlen (str);
  if (len > 0 && str[len - 1] == '\n')
    str[--len] = '\0';
  if (len > 0 && str[len - 1] == '\r')
    str[--len] = '\0';

  switch (request->server_type)
    {
      case GFTP_DIRTYPE_CRAY:
      case GFTP_DIRTYPE_UNIX:
        result = gftp_parse_ls_unix (request, str, len, fle);
        break;
      case GFTP_DIRTYPE_EPLF:
        result = gftp_parse_ls_eplf (str, fle);
        break;
      case GFTP_DIRTYPE_NOVELL:
        result = gftp_parse_ls_novell (str, fle);
        break;
      case GFTP_DIRTYPE_DOS:
        result = gftp_parse_ls_nt (str, fle);
        break;
      case GFTP_DIRTYPE_VMS:
        result = gftp_parse_ls_vms (str, fle);
        break;
      case GFTP_DIRTYPE_MVS:
        result = gftp_parse_ls_mvs (str, fle);
        break;
      default: /* autodetect */
        if (*lsoutput == '+')
          result = gftp_parse_ls_eplf (str, fle);
        else if (isdigit ((int) str[0]) && str[2] == '-')
          result = gftp_parse_ls_nt (str, fle);
        else if (str[1] == ' ' && str[2] == '[')
          result = gftp_parse_ls_novell (str, fle);
        else
          {
            if ((endpos = strchr (str, ' ')) != NULL)
              {
                /* If the first token in the string has a ; in it, then */
                /* we'll assume that this is a VMS directory listing    */
                tmpchar = *endpos;
                *endpos = '\0';
                is_vms = strchr (str, ';') != NULL;
                *endpos = tmpchar;
              }
            else
              is_vms = 0;

            if (is_vms)
              result = gftp_parse_ls_vms (str, fle);
            else
              result = gftp_parse_ls_unix (request, str, len, fle);
          }
        break;
    }
  g_free (str);

  if (fle->attribs == NULL)
    return (result);

  if (*fle->attribs == 'd')
    fle->isdir = 1;
  if (*fle->attribs == 'l')
    fle->islink = 1;
  if (strchr (fle->attribs, 'x') != NULL && !fle->isdir && !fle->islink)
    fle->isexe = 1;

  return (result);
}


static GHashTable *
gftp_gen_dir_hash (gftp_request * request, int *ret)
{
  unsigned long *newsize;
  GHashTable * dirhash;
  gftp_file * fle;
  char * newname;

  dirhash = g_hash_table_new (string_hash_function, string_hash_compare);
  *ret = gftp_list_files (request);
  if (*ret == 0)
    {
      fle = g_malloc0 (sizeof (*fle));
      while (gftp_get_next_file (request, NULL, fle) > 0)
        {
          newname = fle->file;
          newsize = g_malloc (sizeof (unsigned long));
          *newsize = fle->size;
          g_hash_table_insert (dirhash, newname, newsize);
          fle->file = NULL;
          gftp_file_destroy (fle);
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
  g_hash_table_foreach (dirhash, destroy_hash_ent, NULL);
  g_hash_table_destroy (dirhash);
}


static GList *
gftp_get_dir_listing (gftp_transfer * transfer, int getothdir, int *ret)
{
  unsigned long *newsize;
  GHashTable * dirhash;
  GList * templist;
  gftp_file * fle;
  char *newname;

  if (getothdir && transfer->toreq)
    {
      dirhash = gftp_gen_dir_hash (transfer->toreq, ret);
      if (*ret < 0)
        return (NULL);
    }
  else 
    dirhash = NULL; 

  if (gftp_list_files (transfer->fromreq) != 0)
    return (NULL);

  fle = g_malloc (sizeof (*fle));
  templist = NULL;
  while (gftp_get_next_file (transfer->fromreq, NULL, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0 || strcmp (fle->file, "..") == 0)
        {
          gftp_file_destroy (fle);
          continue;
        }

      if (dirhash && 
          (newsize = g_hash_table_lookup (dirhash, fle->file)) != NULL)
        fle->startsize = *newsize;

      if (transfer->toreq && fle->destfile == NULL)
        fle->destfile = gftp_build_path (transfer->toreq->directory, 
                                         fle->file, NULL);

      if (transfer->fromreq->directory != NULL &&
          *transfer->fromreq->directory != '\0' &&
          *fle->file != '/')
        {
          newname = gftp_build_path (transfer->fromreq->directory,
                                     fle->file, NULL);

          g_free (fle->file);
          fle->file = newname;
        }

      templist = g_list_append (templist, fle);

      fle = g_malloc (sizeof (*fle));
    }
  gftp_end_transfer (transfer->fromreq);

  gftp_file_destroy (fle);
  g_free (fle);

  if (dirhash)
    gftp_destroy_dir_hash (dirhash);


  return (templist);
}


int
gftp_get_all_subdirs (gftp_transfer * transfer,
                      void (*update_func) (gftp_transfer * transfer))
{
  char *oldfromdir, *oldtodir, *newname, *pos;
  int forcecd, remotechanged, ret;
  GList * templist, * lastlist;
  unsigned long *newsize;
  GHashTable * dirhash;
  gftp_file * curfle;

  g_return_val_if_fail (transfer != NULL, GFTP_EFATAL);
  g_return_val_if_fail (transfer->fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (transfer->files != NULL, GFTP_EFATAL);

  if (transfer->toreq != NULL)
    {
      ret = 0;
      dirhash = gftp_gen_dir_hash (transfer->toreq, &ret);
      if (ret < 0)
        return (ret);
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
        curfle->startsize = *newsize;

      if (curfle->size < 0 && GFTP_IS_CONNECTED (transfer->fromreq))
        curfle->size = gftp_get_file_size (transfer->fromreq, curfle->file);

      if (transfer->toreq && curfle->destfile == NULL)
        curfle->destfile = gftp_build_path (transfer->toreq->directory, 
                                            curfle->file, NULL);

      if (transfer->fromreq->directory != NULL &&
          *transfer->fromreq->directory != '\0' &&
          *curfle->file != '/')
        {
          newname = gftp_build_path (transfer->fromreq->directory,
                                     curfle->file, NULL);
          g_free (curfle->file);
          curfle->file = newname;
        }

      if (lastlist->next == NULL)
        break;
    }

  if (dirhash)
    gftp_destroy_dir_hash (dirhash);

  oldfromdir = oldtodir = NULL;
  remotechanged = 0;
  forcecd = 0;
  for (templist = transfer->files; templist != NULL; templist = templist->next)
    {
      curfle = templist->data;

      if (curfle->isdir)
        {
          oldfromdir = transfer->fromreq->directory;
          transfer->fromreq->directory = curfle->file;

          if (transfer->toreq)
            {
              oldtodir = transfer->toreq->directory;
              transfer->toreq->directory = curfle->destfile;
            } 
          forcecd = 1;
          if (gftp_set_directory (transfer->fromreq, 
                                  transfer->fromreq->directory) == 0)
            {
              if (curfle->startsize > 0 && transfer->toreq)
                {
                  remotechanged = 1;
                  if (gftp_set_directory (transfer->toreq, 
                                          transfer->toreq->directory) != 0)
                    curfle->startsize = 0;
                } 

              ret = 0;
              lastlist->next = gftp_get_dir_listing (transfer, 
                                                     curfle->startsize > 0,
                                                     &ret);
              if (ret < 0)
                {
                  if (!GFTP_IS_CONNECTED (transfer->fromreq) ||
                      !GFTP_IS_CONNECTED (transfer->toreq))
                    return (ret);
                }

              if (lastlist->next != NULL)
                {
                  lastlist->next->prev = lastlist;
                  for (; lastlist->next != NULL; lastlist = lastlist->next);
                }
              transfer->numdirs++;
              if (update_func)
                update_func (transfer);
            }
          else
            curfle->transfer_action = GFTP_TRANS_ACTION_SKIP;

          transfer->fromreq->directory = oldfromdir;
          if (transfer->toreq)
            transfer->toreq->directory = oldtodir;
        }
      else
        transfer->numfiles++;
    }

  if (forcecd)
    gftp_set_directory (transfer->fromreq, transfer->fromreq->directory);
  if (remotechanged && transfer->toreq)
    gftp_set_directory (transfer->toreq, transfer->toreq->directory);

  if (update_func)
    {
      transfer->numfiles = transfer->numdirs = -1;
      update_func (transfer);
    }
  return (0);
}


#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
static int
get_port (struct addrinfo *addr)
{
  struct sockaddr_in * saddr;
  int port;

  if (addr->ai_family == AF_INET)
    {
      saddr = (struct sockaddr_in *) addr->ai_addr;
      port = ntohs (saddr->sin_port);
    }
  else
    port = 0;

  return (port);
}
#endif


int
gftp_connect_server (gftp_request * request, char *service,
                     char *proxy_hostname, int proxy_port)
{
  char *connect_host, *disphost;
  int port, sock;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo hints, *res;
  intptr_t enable_ipv6;
  char serv[8];
  int errnum;

  if ((request->use_proxy = gftp_need_proxy (request, service,
                                             proxy_hostname, proxy_port)) < 0)
    return (request->use_proxy);
  else if (request->use_proxy == 1)
    request->hostp = NULL;

  gftp_lookup_request_option (request, "enable_ipv6", &enable_ipv6);

  request->free_hostp = 1;
  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;

  if (enable_ipv6)
    hints.ai_family = PF_UNSPEC;
  else
    hints.ai_family = AF_INET;

  hints.ai_socktype = SOCK_STREAM;

  if (request->use_proxy)
    {
      connect_host = proxy_hostname;
      port = proxy_port;
    }
  else
    {
      connect_host = request->hostname;
      port = request->port;
    }

  if (request->hostp == NULL)
    {
      if (port == 0)
        strcpy (serv, service); 
      else
        snprintf (serv, sizeof (serv), "%d", port);

      request->logging_function (gftp_logging_misc, request,
                                 _("Looking up %s\n"), connect_host);
      if ((errnum = getaddrinfo (connect_host, serv, &hints, 
                                 &request->hostp)) != 0)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot look up hostname %s: %s\n"),
                                     connect_host, gai_strerror (errnum));
          return (GFTP_ERETRYABLE);
        }
    }

  disphost = connect_host;
  for (res = request->hostp; res != NULL; res = res->ai_next)
    {
      disphost = res->ai_canonname ? res->ai_canonname : connect_host;
      port = get_port (res);
      if (!request->use_proxy)
        request->port = port;

      if ((sock = socket (res->ai_family, res->ai_socktype, 
                          res->ai_protocol)) < 0)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Failed to create a socket: %s\n"),
                                     g_strerror (errno));
          continue; 
        } 

      request->logging_function (gftp_logging_misc, request,
                                 _("Trying %s:%d\n"), disphost, port);

      if (connect (sock, res->ai_addr, res->ai_addrlen) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     disphost, g_strerror (errno));
          close (sock);
          continue;
        }
      break;
    }

  if (res == NULL)
    {
      if (request->hostp != NULL)
        {
          freeaddrinfo (request->hostp);
          request->hostp = NULL;
        }
      return (GFTP_ERETRYABLE);
    }

#else /* !HAVE_GETADDRINFO */
  struct sockaddr_in remote_address;
  struct servent serv_struct;
  int curhost;

  if ((request->use_proxy = gftp_need_proxy (request, service,
                                             proxy_hostname, proxy_port)) < 0)
    return (request->use_proxy);
  else if (request->use_proxy == 1)
    request->hostp = NULL;

  if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Failed to create a IPv4 socket: %s\n"),
                                 g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  memset (&remote_address, 0, sizeof (remote_address));
  remote_address.sin_family = AF_INET;

  if (request->use_proxy)
    {
      connect_host = proxy_hostname;
      port = proxy_port;
    }
  else
    {
      connect_host = request->hostname;
      port = request->port;
    }

  if (port == 0)
    {
      if (!r_getservbyname (service, "tcp", &serv_struct, NULL))
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot look up service name %s/tcp. Please check your services file\n"),
                                     service);
          close (sock);
          return (GFTP_EFATAL);
        }

      port = ntohs (serv_struct.s_port);

      if (!request->use_proxy)
        request->port = port;
    }

  remote_address.sin_port = htons (port);

  if (request->hostp == NULL)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Looking up %s\n"), connect_host);
      if (!(request->hostp = r_gethostbyname (connect_host, &request->host,
                                              NULL)))
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot look up hostname %s: %s\n"),
                                     connect_host, g_strerror (errno));
          close (sock);
          return (GFTP_ERETRYABLE);
        }
    }

  disphost = NULL;
  for (curhost = 0; request->host.h_addr_list[curhost] != NULL; curhost++)
    {
      disphost = request->host.h_name;
      memcpy (&remote_address.sin_addr, request->host.h_addr_list[curhost],
              request->host.h_length);
      request->logging_function (gftp_logging_misc, request,
                                 _("Trying %s:%d\n"),
                                 request->host.h_name, port);

      if (connect (sock, (struct sockaddr *) &remote_address,
                   sizeof (remote_address)) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     connect_host, g_strerror (errno));
        }
      break;
    }

  if (request->host.h_addr_list[curhost] == NULL)
    {
      close (sock);
      return (GFTP_ERETRYABLE);
    }
#endif /* HAVE_GETADDRINFO */

  if (fcntl (sock, F_SETFD, 1) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot set close on exec flag: %s\n"),
                                 g_strerror (errno));

      return (GFTP_ERETRYABLE);
    }

  request->logging_function (gftp_logging_misc, request,
                             _("Connected to %s:%d\n"), connect_host, port);

  if (gftp_fd_set_sockblocking (request, sock, 1) < 0)
    {
      close (sock);
      return (GFTP_ERETRYABLE);
    }

  request->datafd = sock;

  if (request->post_connect != NULL)
    return (request->post_connect (request));

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

  printf ("--START OF FILE LISTING - TOP TO BOTTOM--\n");
  for (templist = list; ; templist = templist->next)
    {
      tempfle = templist->data;
#if defined (_LARGEFILE_SOURCE)
      printf ("%s:%s:%lld:%lld:%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              (long long) tempfle->size, (long long) tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
#else
      printf ("%s:%s:%ld:%ld:%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
#endif
      if (templist->next == NULL)
        break;
    }

  printf ("--START OF FILE LISTING - BOTTOM TO TOP--\n");
  for (; ; templist = templist->prev)
    {
      tempfle = templist->data;
#if defined (_LARGEFILE_SOURCE)
      printf ("%s:%s:%lld:%lld:%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              (long long) tempfle->size, (long long) tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
#else
      printf ("%s:%s:%ld:%ld:%s:%s:%s\n", 
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
#endif
      if (templist == list)
        break;
    }
  printf ("--END OF FILE LISTING--\n");
}


void
gftp_free_getline_buffer (gftp_getline_buffer ** rbuf)
{
  g_free ((*rbuf)->buffer);
  g_free (*rbuf);
  *rbuf = NULL;
}


ssize_t
gftp_get_line (gftp_request * request, gftp_getline_buffer ** rbuf, 
               char * str, size_t len, int fd)
{
  ssize_t ret, retval, rlen;
  char *pos, *nextpos;
  ssize_t (*read_function) (gftp_request * request, void *ptr, size_t size,
                            int fd);
  int end_of_buffer;

  if (request == NULL || request->read_function == NULL)
    read_function = gftp_fd_read;
  else
    read_function = request->read_function;

  if (*rbuf == NULL)
    {
      *rbuf = g_malloc0 (sizeof (**rbuf));
      (*rbuf)->max_bufsize = len;
      (*rbuf)->buffer = g_malloc0 ((*rbuf)->max_bufsize + 1);

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

  retval = GFTP_ERETRYABLE;
  do
    {
      pos = strchr ((*rbuf)->curpos, '\n');
      end_of_buffer = (*rbuf)->curpos == (*rbuf)->buffer && 
            ((*rbuf)->max_bufsize == (*rbuf)->cur_bufsize || (*rbuf)->eof);

      if ((*rbuf)->cur_bufsize > 0 && (pos != NULL || end_of_buffer))
        {
          if (pos != NULL)
            {
              retval = pos - (*rbuf)->curpos + 1;
              nextpos = pos + 1;
              if (pos > (*rbuf)->curpos && *(pos - 1) == '\r')
                pos--;
              *pos = '\0';
            }
          else
            {
              retval = (*rbuf)->cur_bufsize;
              nextpos = NULL;

              /* This is not an overflow since we allocated one extra byte to
                 buffer above */
              ((*rbuf)->curpos)[retval] = '\0';
            }

          strncpy (str, (*rbuf)->curpos, len);
          str[len - 1] = '\0';
          (*rbuf)->cur_bufsize -= retval;

          if (nextpos != NULL)
            (*rbuf)->curpos = nextpos;
          else
            (*rbuf)->cur_bufsize = 0;
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
            ret = read_function (request, pos, rlen, fd);

          if (ret < 0)
            {
              gftp_free_getline_buffer (rbuf);
              return (ret);
            }
          else if (ret == 0)
            {
              if ((*rbuf)->cur_bufsize == 0)
                {
                  gftp_free_getline_buffer (rbuf);
                  return (ret);
                }

              (*rbuf)->eof = 1;
            }

          (*rbuf)->cur_bufsize += ret;
          (*rbuf)->curpos[(*rbuf)->cur_bufsize] = '\0';
        }
    }
  while (retval == GFTP_ERETRYABLE);

  return (retval);
}


ssize_t 
gftp_fd_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  intptr_t network_timeout;
  struct timeval tv;
  fd_set fset;
  ssize_t ret;

  gftp_lookup_request_option (request, "network_timeout", &network_timeout);  

  errno = 0;
  ret = 0;
  do
    {
      FD_ZERO (&fset);
      FD_SET (fd, &fset);
      tv.tv_sec = network_timeout;
      tv.tv_usec = 0;
      ret = select (fd + 1, &fset, NULL, NULL, &tv);
      if (ret == -1 && errno == EINTR)
        {
          if (request && request->cancel)
            break;
          else
            continue;
        }
      else if (ret <= 0)
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
          if (errno == EINTR)
            {
              if (request != NULL && request->cancel)
                break;
              else
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
    }
  while (errno == EINTR && !(request != NULL && request->cancel));

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (ret);
}


ssize_t 
gftp_fd_write (gftp_request * request, const char *ptr, size_t size, int fd)
{
  intptr_t network_timeout;
  struct timeval tv;
  ssize_t w_ret;
  fd_set fset;
  size_t ret;

  gftp_lookup_request_option (request, "network_timeout", &network_timeout);  

  errno = 0;
  ret = 0;
  do
    {
      FD_ZERO (&fset);
      FD_SET (fd, &fset);
      tv.tv_sec = network_timeout;
      tv.tv_usec = 0;
      ret = select (fd + 1, NULL, &fset, NULL, &tv);
      if (ret == -1 && errno == EINTR)
        {
          if (request != NULL && request->cancel)
            break;
          else
            continue;
        }
      else if (ret <= 0)
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
          if (errno == EINTR)
            {
              if (request != NULL && request->cancel)
                break;
              else
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

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

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

  if (dest->swap_socks)
    dest->swap_socks (dest, source);
}


void
gftp_calc_kbs (gftp_transfer * tdata, ssize_t num_read)
{
  /* Needed for systems that size(float) < size(void *) */
  union { intptr_t i; float f; } maxkbs;
  unsigned long waitusecs;
  double start_difftime;
  gftp_file * tempfle;
  struct timeval tv;
  int waited;

  gftp_lookup_request_option (tdata->fromreq, "maxkbs", &maxkbs.f);

  if (g_thread_supported ())
    g_static_mutex_lock (&tdata->statmutex);

  gettimeofday (&tv, NULL);

  tempfle = tdata->curfle->data;
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


int
gftp_get_transfer_status (gftp_transfer * tdata, ssize_t num_read)
{
  int ret1, ret2;
  intptr_t retries, sleep_time;
  gftp_file * tempfle;
  struct timeval tv;

  ret1 = ret2 = 0;
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

  if (num_read < 0 || tdata->skip_file)
    {
      if (num_read == GFTP_EFATAL)
        return (GFTP_EFATAL);
      else if (!tdata->conn_error_no_timeout)
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
          if (!tdata->conn_error_no_timeout && !tdata->skip_file)
            {
              tv.tv_sec = sleep_time;
              tv.tv_usec = 0;
              select (0, NULL, NULL, NULL, &tv);
            }
          else
            tdata->conn_error_no_timeout = 0;

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
          else
            tdata->current_file_retries++;
        }
    }
  else if (tdata->cancel)
    return (GFTP_EFATAL);

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
gftp_setup_startup_directory (gftp_request * request)
{
  char *startup_directory, *tempstr;

  gftp_lookup_request_option (request, "startup_directory", &startup_directory);

  if (*startup_directory != '\0' &&
      (tempstr = expand_path (startup_directory)) != NULL)
    {
      gftp_set_directory (request, tempstr);
      g_free (tempstr);
    }
}

