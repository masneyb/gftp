/*****************************************************************************/
/*  protocols.c - Skeleton functions for the protocols gftp supports         */
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

gftp_request *
gftp_request_new (void)
{
  gftp_request *request;

  request = g_malloc0 (sizeof (*request));
  request->sockfd = -1;
  request->data_type = GFTP_TYPE_BINARY;
  return (request);
}


void
gftp_request_destroy (gftp_request * request)
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
  if (request->proxy_config)
    g_free (request->proxy_config);
  if (request->proxy_hostname)
    g_free (request->proxy_hostname);
  if (request->proxy_username)
    g_free (request->proxy_username);
  if (request->proxy_password)
    g_free (request->proxy_password);
  if (request->proxy_account)
    g_free (request->proxy_account);
  if (request->last_ftp_response)
    g_free (request->last_ftp_response);
  if (request->protocol_data)
    g_free (request->protocol_data);
  if (request->sftpserv_path)
    g_free (request->sftpserv_path);
  memset (request, 0, sizeof (*request));
  g_free (request);
}


void
gftp_file_destroy (gftp_file * file)
{
  g_return_if_fail (file != NULL);

  if (file->file)
    g_free (file->file);
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
  g_return_val_if_fail (request != NULL, -2);

  if (request->connect == NULL)
    return (-2);

  gftp_set_config_options (request);

  return (request->connect (request));
}


void
gftp_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  if (request->hostp)
    freeaddrinfo (request->hostp);
#endif
  request->hostp = NULL;

  if (request->sftpserv_path != NULL)
    {
      g_free (request->sftpserv_path);
      request->sftpserv_path = NULL;
    }

  request->cached = 0;
  if (request->disconnect == NULL)
    return;
  request->disconnect (request);
}


off_t
gftp_get_file (gftp_request * request, const char *filename, int fd,
               size_t startsize)
{
  g_return_val_if_fail (request != NULL, -2);

  request->cached = 0;
  if (request->get_file == NULL)
    return (-2);
  return (request->get_file (request, filename, fd, startsize));
}


int
gftp_put_file (gftp_request * request, const char *filename, int fd,
               size_t startsize, size_t totalsize)
{
  g_return_val_if_fail (request != NULL, -2);

  request->cached = 0;
  if (request->put_file == NULL)
    return (-2);
  return (request->put_file (request, filename, fd, startsize, totalsize));
}


long
gftp_transfer_file (gftp_request * fromreq, const char *fromfile, 
                    int fromfd, size_t fromsize, 
                    gftp_request * toreq, const char *tofile,
                    int tofd, size_t tosize)
{
  long size;

  g_return_val_if_fail (fromreq != NULL, -2);
  g_return_val_if_fail (fromfile != NULL, -2);
  g_return_val_if_fail (toreq != NULL, -2);
  g_return_val_if_fail (tofile != NULL, -2);

  if (strcmp (fromreq->protocol_name, toreq->protocol_name) == 0)
    {
      if (fromreq->transfer_file == NULL)
	return (-2);
      return (fromreq->transfer_file (fromreq, fromfile, fromsize, toreq, 
                                      tofile, tosize));
    }

  fromreq->cached = 0;
  toreq->cached = 0;
  if ((size = gftp_get_file (fromreq, fromfile, fromfd, tosize)) < 0)
    return (-2);

  if (gftp_put_file (toreq, tofile, tofd, tosize, size) != 0)
    {
      if (gftp_abort_transfer (fromreq) != 0)
        gftp_end_transfer (fromreq);

      return (-2);
    }

  return (size);
}


ssize_t 
gftp_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (buf != NULL, -2);

  if (request->get_next_file_chunk != NULL)
    return (request->get_next_file_chunk (request, buf, size));

  return (gftp_read (request, buf, size, request->datafd));
}


ssize_t 
gftp_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (buf != NULL, -2);

  if (request->put_next_file_chunk != NULL)
    return (request->put_next_file_chunk (request, buf, size));

  if (size == 0)
    return (0);

  return (gftp_write (request, buf, size, request->datafd));
}


int
gftp_end_transfer (gftp_request * request)
{
  int ret;

  g_return_val_if_fail (request != NULL, -2);

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
  g_return_val_if_fail (request != NULL, -2);

  if (request->abort_transfer == NULL)
    return (-2);

  return (request->abort_transfer (request));
}


int
gftp_list_files (gftp_request * request)
{
  int fd;

  g_return_val_if_fail (request != NULL, -2);

  request->cached = 0;
  if (request->use_cache && (fd = gftp_find_cache_entry (request)) > 0)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
                                 _("Loading directory listing %s from cache\n"),
                                 request->directory);

      request->cachefd = fd;
      request->cached = 1;
      return (0);
    }
  else if (request->use_cache)
    {
      request->cachefd = gftp_new_cache_entry (request);
      request->cached = 0; 
    }

  if (request->list_files == NULL)
    return (-2);
  return (request->list_files (request));
}


int
gftp_get_next_file (gftp_request * request, char *filespec, gftp_file * fle)
{
  int fd, ret;
#if GLIB_MAJOR_VERSION > 1
  gsize bread, bwrite;
  char *tempstr;
  GError * error;
#endif

  g_return_val_if_fail (request != NULL, -2);

  if (request->get_next_file == NULL)
    return (-2);

  if (request->cached && request->cachefd > 0)
    fd = request->cachefd;
  else
    fd = request->datafd;

  memset (fle, 0, sizeof (*fle));
  do
    {
      gftp_file_destroy (fle);
      ret = request->get_next_file (request, fle, fd);

#if GLIB_MAJOR_VERSION > 1
      if (fle->file != NULL && !g_utf8_validate (fle->file, -1, NULL))
        {
          error = NULL;
          if ((tempstr = g_locale_to_utf8 (fle->file, -1, &bread, 
                                           &bwrite, &error)) != NULL)
            {
              g_free (fle->file);
              fle->file = tempstr;
            }
          else
            g_warning ("Error when converting %s to UTF-8: %s\n", fle->file,
                       error->message);
        }
#endif

      if (ret >= 0 && !request->cached && request->cachefd > 0 && 
          request->last_dir_entry != NULL)
        {
          if (gftp_write (request, request->last_dir_entry,
                          request->last_dir_entry_len, request->cachefd) < 0)
            {
              request->logging_function (gftp_logging_error, request->user_data,
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
gftp_parse_url (gftp_request * request, const char *url)
{
  char *pos, *endpos, *endhostpos, *str, tempchar;
  const char *stpos;
  int len, i;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (url != NULL, -2);

  for (stpos = url; *stpos == ' '; stpos++);

  if ((pos = strstr (stpos, "://")) != NULL)
    {
      *pos = '\0';

      for (i = 0; gftp_protocols[i].url_prefix; i++)
        {
          if (strcmp (gftp_protocols[i].url_prefix, stpos) == 0)
            break;
        }

      if (gftp_protocols[i].url_prefix == NULL)
        return (-2);
      *pos = ':';
    }
  else
    {
      for (i = 0; gftp_protocols[i].url_prefix; i++)
        {
          if (strcmp (gftp_protocols[i].name, default_protocol) == 0)
            break;
        }

      if (gftp_protocols[i].url_prefix == NULL)
        i = GFTP_FTP_NUM;
    }

  gftp_protocols[i].init (request);

  if (request->parse_url != NULL)
    return (request->parse_url (request, url));

  if (i == GFTP_LOCAL_NUM)
    {
      request->directory = g_malloc (strlen (stpos + 6) + 1);
      strcpy (request->directory, stpos + 6);
      return (0);
    }

  for (; *stpos == ' '; stpos++);
  str = g_malloc (strlen (stpos) + 1);
  strcpy (str, stpos);
  for (pos = str + strlen (str) - 1; *pos == ' '; pos--)
    *pos = '\0';

  pos = str;
  len = strlen (request->url_prefix);
  if (strncmp (pos, request->url_prefix, len) == 0
      && strncmp (pos + len, "://", 3) == 0)
    pos += len + 3;

  if ((endhostpos = strchr (pos, '@')) != NULL)
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
  g_free (str);
  return (0);
}


int
gftp_set_data_type (gftp_request * request, int data_type)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->set_data_type == NULL)
    return (-2);
  return (request->set_data_type (request, data_type));
}


void
gftp_set_hostname (gftp_request * request, const char *hostname)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (hostname != NULL);

  if (request->hostname)
    g_free (request->hostname);
  request->hostname = g_malloc (strlen (hostname) + 1);
  strcpy (request->hostname, hostname);
}


void
gftp_set_username (gftp_request * request, const char *username)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (username != NULL);

  if (request->username)
    g_free (request->username);
  request->username = g_malloc (strlen (username) + 1);
  strcpy (request->username, username);
}


void
gftp_set_password (gftp_request * request, const char *password)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (password != NULL);

  if (request->password)
    g_free (request->password);
  request->password = g_malloc (strlen (password) + 1);
  strcpy (request->password, password);
}


void
gftp_set_account (gftp_request * request, const char *account)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (account != NULL);

  if (request->account)
    g_free (request->account);
  request->account = g_malloc (strlen (account) + 1);
  strcpy (request->account, account);
}


int
gftp_set_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (directory != NULL, -2);


  if (request->sockfd <= 0 && !request->always_connected)
    {
      if (directory != request->directory)
	{
	  if (request->directory)
	    g_free (request->directory);
	  request->directory = g_malloc (strlen (directory) + 1);
	  strcpy (request->directory, directory);
	}
      return (0);
    }
  else if (request->chdir == NULL)
    return (-2);
  return (request->chdir (request, directory));
}


void
gftp_set_port (gftp_request * request, unsigned int port)
{
  g_return_if_fail (request != NULL);

  request->port = port;
}


void
gftp_set_proxy_hostname (gftp_request * request, const char *hostname)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (hostname != NULL);

  if (request->proxy_hostname)
    g_free (request->proxy_hostname);
  request->proxy_hostname = g_malloc (strlen (hostname) + 1);
  strcpy (request->proxy_hostname, hostname);
}


void
gftp_set_proxy_username (gftp_request * request, const char *username)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (username != NULL);

  if (request->proxy_username)
    g_free (request->proxy_username);
  request->proxy_username = g_malloc (strlen (username) + 1);
  strcpy (request->proxy_username, username);
}


void
gftp_set_proxy_password (gftp_request * request, const char *password)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (password != NULL);

  if (request->proxy_password)
    g_free (request->proxy_password);
  request->proxy_password = g_malloc (strlen (password) + 1);
  strcpy (request->proxy_password, password);
}


void
gftp_set_proxy_account (gftp_request * request, const char *account)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (account != NULL);

  if (request->proxy_account)
    g_free (request->proxy_account);
  request->proxy_account = g_malloc (strlen (account) + 1);
  strcpy (request->proxy_account, account);
}


void
gftp_set_proxy_port (gftp_request * request, unsigned int port)
{
  g_return_if_fail (request != NULL);

  request->proxy_port = port;
}


int
gftp_remove_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->rmdir == NULL)
    return (-2);
  return (request->rmdir (request, directory));
}


int
gftp_remove_file (gftp_request * request, const char *file)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->rmfile == NULL)
    return (-2);
  return (request->rmfile (request, file));
}


int
gftp_make_directory (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->mkdir == NULL)
    return (-2);
  return (request->mkdir (request, directory));
}


int
gftp_rename_file (gftp_request * request, const char *oldname,
		  const char *newname)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->rename == NULL)
    return (-2);
  return (request->rename (request, oldname, newname));
}


int
gftp_chmod (gftp_request * request, const char *file, int mode)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->chmod == NULL)
    return (-2);
  return (request->chmod (request, file, mode));
}


int
gftp_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->set_file_time == NULL)
    return (-2);
  return (request->set_file_time (request, file, datetime));
}


char
gftp_site_cmd (gftp_request * request, const char *command)
{
  g_return_val_if_fail (request != NULL, -2);

  if (request->site == NULL)
    return (-2);
  return (request->site (request, command));
}


void
gftp_set_proxy_config (gftp_request * request, const char *proxy_config)
{
  int len;

  g_return_if_fail (request != NULL);
  g_return_if_fail (proxy_config != NULL);

  if (request->proxy_config != NULL)
    g_free (request->proxy_config);

  len = strlen (proxy_config);

  if (len > 0 && (proxy_config[len - 1] != 'n' ||
		  proxy_config[len - 2] != '%'))
    len += 2;

  request->proxy_config = g_malloc (len + 1);
  strcpy (request->proxy_config, proxy_config);
  if (len != strlen (proxy_config))
    {
      request->proxy_config[len - 2] = '%';
      request->proxy_config[len - 1] = 'n';
      request->proxy_config[len] = '\0';
    }
}


off_t
gftp_get_file_size (gftp_request * request, const char *filename)
{
  g_return_val_if_fail (request != NULL, 0);

  if (request->get_file_size == NULL)
    return (0);
  return (request->get_file_size (request, filename));
}


int
gftp_need_proxy (gftp_request * request, char *service)
{
  gftp_proxy_hosts * hostname;
  unsigned char addy[4];
  struct sockaddr *addr;
  GList * templist;
  gint32 netaddr;
  char *pos;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo hints;
  int port, errnum;
  char serv[8];

  request->hostp = NULL;
  if (proxy_hosts == NULL)
    return (request->proxy_hostname != NULL
	    && *request->proxy_hostname != '\0'
	    && request->proxy_config != NULL
	    && *request->proxy_config != '\0');

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  port = request->use_proxy ? request->proxy_port : request->port;
  if (port == 0)
    strcpy (serv, service);
  else
    snprintf (serv, sizeof (serv), "%d", port);

  request->logging_function (gftp_logging_misc, request->user_data,
                             _("Looking up %s\n"), request->hostname);

  if ((errnum = getaddrinfo (request->hostname, serv, &hints, 
                             &request->hostp)) != 0)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, gai_strerror (errnum));
      return (-1);
    }

  addr = request->hostp->ai_addr;

#else /* !HAVE_GETADDRINFO */

  request->hostp = NULL;
  if (proxy_hosts == NULL)
    return (request->proxy_hostname != NULL
            && *request->proxy_hostname != '\0'
            && request->proxy_config != NULL
            && *request->proxy_config != '\0');

  request->logging_function (gftp_logging_misc, request->user_data,
                             _("Looking up %s\n"), request->hostname);

  if (!(request->hostp = r_gethostbyname (request->hostname, &request->host,
                                          NULL)))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, g_strerror (errno));
      return (-1);
    }

  addr = (struct sockaddr *) request->host.h_addr_list[0];

#endif /* HAVE_GETADDRINFO */

  templist = proxy_hosts;
  while (templist != NULL)
    {
      hostname = templist->data;
      if (hostname->domain && 
          strlen (request->hostname) > strlen (hostname->domain))
	{
	  pos = request->hostname + strlen (request->hostname) -
	                         strlen (hostname->domain);
	  if (strcmp (hostname->domain, pos) == 0)
	    return (0);
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
  return (request->proxy_hostname != NULL && *request->proxy_hostname != '\0'
	  && request->proxy_config != NULL && *request->proxy_config != '\0');
}


char *
gftp_convert_ascii (char *buf, ssize_t *len, int direction)
{
  ssize_t i, j, newsize;
  char *tempstr;

  if (direction == GFTP_DIRECTION_DOWNLOAD)
    {
      for (i = 0, j = 0; i < *len; i++)
        {
          if (buf[i] != '\r')
            buf[j++] = buf[i];
          else
            --*len;
        }
      tempstr = buf;
    }
  else
    {
      newsize = 0;
      for (i = 0; i < *len; i++)
        {
          newsize++;
          if (i > 0 && buf[i] == '\n' && buf[i - 1] != '\r')
            newsize++;
        }
      tempstr = g_malloc (newsize);

      for (i = 0, j = 0; i < *len; i++)
        {
          if (i > 0 && buf[i] == '\n' && buf[i - 1] != '\r')
            tempstr[j++] = '\r';
          tempstr[j++] = buf[i];
        }
      *len = newsize;
    }
  return (tempstr);
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


static time_t
parse_time (char **str)
{
  const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
    "Aug", "Sep", "Oct", "Nov", "Dec" };
  char *startpos, *endpos, *datepos;
  struct tm curtime, tt;
  time_t t;
  int i;

  startpos = *str;
  memset (&tt, 0, sizeof (tt));
  tt.tm_isdst = -1;
  if (isdigit ((int) startpos[0]) && startpos[2] == '-')
    {
      /* This is how DOS will return the date/time */
      /* 07-06-99  12:57PM */
      if ((endpos = strchr (startpos, '-')) == NULL)
	{
	  g_free (str);
	  return (0);
	}
      tt.tm_mon = strtol (startpos, NULL, 10) - 1;

      startpos = endpos + 1;
      if ((endpos = strchr (startpos, '-')) == NULL)
	{
	  g_free (str);
	  return (0);
	}
      tt.tm_mday = strtol (startpos, NULL, 10);

      startpos = endpos + 1;
      if ((endpos = strchr (startpos, ' ')) == NULL)
	{
	  g_free (str);
	  return (0);
	}
      tt.tm_year = strtol (startpos, NULL, 10);

      while (*endpos == ' ')
	endpos++;

      startpos = endpos + 1;
      if ((endpos = strchr (startpos, ':')) == NULL)
	{
	  g_free (str);
	  return (0);
	}
      tt.tm_hour = strtol (startpos, NULL, 10);

      startpos = endpos + 1;
      while ((*endpos != 'A') && (*endpos != 'P'))
	endpos++;
      if (*endpos == 'P')
	{
	  if (tt.tm_hour == 12)
	    tt.tm_hour = 0;
	  else
	    tt.tm_hour += 12;
	}
      tt.tm_min = strtol (startpos, NULL, 10);
    }
  else
    {
      /* This is how most UNIX, Novell, and MacOS ftp servers send their time */
      /* Jul 06 12:57 */
      t = time (NULL);
      curtime = *localtime (&t);

      /* Get the month */
      if ((endpos = strchr (startpos, ' ')) == NULL)
	return (0);
      for (i = 0; i < 12; i++)
	{
	  if (strncmp (months[i], startpos, 3) == 0)
	    {
	      tt.tm_mon = i;
	      break;
	    }
	}

      /* Skip the blanks till we get to the next entry */
      startpos = endpos + 1;
      while (*startpos == ' ')
	startpos++;

      /* Get the day */
      if ((endpos = strchr (startpos, ' ')) == NULL)
	return (0);
      tt.tm_mday = strtol (startpos, NULL, 10);

      /* Skip the blanks till we get to the next entry */
      startpos = endpos + 1;
      while (*startpos == ' ')
	startpos++;

      if ((datepos = strchr (startpos, ':')) != NULL)
	{
	  /* No year was specified. We will use the current year */
	  tt.tm_year = curtime.tm_year;

	  /* If the date is in the future, than the year is from last year */
	  if ((tt.tm_mon > curtime.tm_mon) ||
	      ((tt.tm_mon == curtime.tm_mon) && tt.tm_mday > curtime.tm_mday))
	    tt.tm_year--;

	  /* Get the hours and the minutes */
	  tt.tm_hour = strtol (startpos, NULL, 10);
	  tt.tm_min = strtol (datepos + 1, NULL, 10);
	}
      else
	{
	  /* Get the year. The hours and minutes will be assumed to be 0 */
	  tt.tm_year = strtol (startpos, NULL, 10) - 1900;
	}
    }
  *str = startpos;
  return (mktime (&tt));
}


static int
gftp_parse_ls_eplf (char *str, gftp_file * fle)
{
  char *startpos;

  startpos = str;
  fle->attribs = g_malloc (11);
  strcpy (fle->attribs, "----------");
  while (startpos)
    {
      startpos++;
      switch (*startpos)
	{
	case '/':
	  *fle->attribs = 'd';
	  break;
	case 's':
	  fle->size = strtol (startpos + 1, NULL, 10);
	  break;
	case 'm':
	  fle->datetime = strtol (startpos + 1, NULL, 10);
	  break;
	}
      startpos = strchr (startpos, ',');
    }
  if ((startpos = strchr (str, 9)) == NULL)
    return (-2);
  fle->file = g_malloc (strlen (startpos));
  strcpy (fle->file, startpos + 1);
  fle->user = g_malloc (8);
  strcpy (fle->user, _("unknown"));
  fle->group = g_malloc (8);
  strcpy (fle->group, _("unknown"));
  return (0);
}


static int
gftp_parse_ls_unix (char *str, int cols, gftp_file * fle)
{
  char *endpos, *startpos;

  startpos = str;
  /* Copy file attributes */
  if ((startpos = copy_token (&fle->attribs, startpos)) == NULL)
    return (-2);

  if (cols >= 9)
    {
      /* Skip the number of links */
      startpos = goto_next_token (startpos);

      /* Copy the user that owns this file */
      if ((startpos = copy_token (&fle->user, startpos)) == NULL)
	return (-2);

      /* Copy the group that owns this file */
      if ((startpos = copy_token (&fle->group, startpos)) == NULL)
	return (-2);
    }
  else
    {
      fle->group = g_malloc (8);
      strcpy (fle->group, _("unknown"));
      if (cols == 8)
	{
	  if ((startpos = copy_token (&fle->user, startpos)) == NULL)
	    return (-2);
	}
      else
	{
	  fle->user = g_malloc (8);
	  strcpy (fle->user, _("unknown"));
	}
      startpos = goto_next_token (startpos);
    }

  /* FIXME - this is a bug if there is some extra spaces in the file */
  /* See if this is a Cray directory listing. It has the following format:
     drwx------     2 feiliu    g913     DK  common      4096 Sep 24  2001 wv */
  if (cols == 11 && strstr (str, "->") == NULL)
    {
      startpos = goto_next_token (startpos);
      startpos = goto_next_token (startpos);
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
	return (-2);
      fle->size |= strtol (startpos, NULL, 10) & 0xFF;
    }
  else
    {
      /* This is a regular file  */
      if ((endpos = strchr (startpos, ' ')) == NULL)
	return (-2);
      fle->size = strtol (startpos, NULL, 10);
    }

  /* Skip the blanks till we get to the next entry */
  startpos = endpos + 1;
  while (*startpos == ' ')
    startpos++;

  if ((fle->datetime = parse_time (&startpos)) == 0)
    return (-2);

  /* Skip the blanks till we get to the next entry */
  startpos = goto_next_token (startpos);

  /* Parse the filename. If this file is a symbolic link, remove the -> part */
  if (fle->attribs[0] == 'l' && ((endpos = strstr (startpos, "->")) != NULL))
    *(endpos - 1) = '\0';
  fle->file = g_malloc (strlen (startpos) + 1);

  strcpy (fle->file, startpos);

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
  if ((fle->datetime = parse_time (&startpos)) == 0)
    return (-2);

  /* No such thing on Windoze.. */
  fle->user = g_malloc (8);
  strcpy (fle->user, _("unknown"));
  fle->group = g_malloc (8);
  strcpy (fle->group, _("unknown"));

  startpos = goto_next_token (startpos);
  fle->attribs = g_malloc (11);
  if (startpos[0] == '<')
    strcpy (fle->attribs, "drwxrwxrwx");
  else
    {
      strcpy (fle->attribs, "-rw-rw-rw-");
      fle->size = strtol (startpos, NULL, 10);
    }
  startpos = goto_next_token (startpos);
  fle->file = g_malloc (strlen (startpos) + 1);
  strcpy (fle->file, startpos);
  return (0);
}


static int
gftp_parse_ls_novell (char *str, gftp_file * fle)
{
  char *startpos;

  if (str[12] != ' ')
    return (-2);
  str[12] = '\0';
  fle->attribs = g_malloc (13);
  strcpy (fle->attribs, str);
  startpos = str + 13;

  while ((*startpos == ' ' || *startpos == '\t') && *startpos != '\0')
    startpos++;

  if ((startpos = copy_token (&fle->user, startpos)) == NULL)
    return (-2);

  fle->group = g_malloc (8);
  strcpy (fle->group, _("unknown"));

  fle->size = strtol (startpos, NULL, 10);

  startpos = goto_next_token (startpos);
  if ((fle->datetime = parse_time (&startpos)) == 0)
    return (-2);

  startpos = goto_next_token (startpos);
  fle->file = g_malloc (strlen (startpos) + 1);
  strcpy (fle->file, startpos);
  return (0);
}


int
gftp_parse_ls (const char *lsoutput, gftp_file * fle)
{
  int result, cols;
  char *str, *pos;

  g_return_val_if_fail (lsoutput != NULL, -2);
  g_return_val_if_fail (fle != NULL, -2);

  str = g_malloc (strlen (lsoutput) + 1);
  strcpy (str, lsoutput);
  memset (fle, 0, sizeof (*fle));

  if (str[strlen (str) - 1] == '\n')
    str[strlen (str) - 1] = '\0';
  if (str[strlen (str) - 1] == '\r')
    str[strlen (str) - 1] = '\0';
  if (*lsoutput == '+') 			/* EPLF format */
    result = gftp_parse_ls_eplf (str, fle);
  else if (isdigit ((int) str[0]) && str[2] == '-') 	/* DOS/WinNT format */
    result = gftp_parse_ls_nt (str, fle);
  else if (str[1] == ' ' && str[2] == '[') 	/* Novell format */
    result = gftp_parse_ls_novell (str, fle);
  else
    {
      /* UNIX/MacOS format */

      /* If there is no space between the attribs and links field, just make one */
      if (strlen (str) > 10)
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

      if (cols > 6)
	result = gftp_parse_ls_unix (str, cols, fle);
      else
	result = -2;
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
  if (*fle->attribs == 'b')
    fle->isblock = 1;
  if (*fle->attribs == 'c')
    fle->ischar = 1;
  if (*fle->attribs == 's')
    fle->issocket = 1;
  if (*fle->attribs == 'p')
    fle->isfifo = 1;

  return (result);
}


static GHashTable *
gftp_gen_dir_hash (gftp_request * request)
{
  unsigned long *newsize;
  GHashTable * dirhash;
  gftp_file * fle;
  char * newname;


  dirhash = g_hash_table_new (string_hash_function, string_hash_compare);
  if (gftp_list_files (request) == 0)
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
gftp_get_dir_listing (gftp_transfer * transfer, int getothdir)
{
  unsigned long *newsize;
  GHashTable * dirhash;
  GList * templist;
  gftp_file * fle;
  char *newname;

  if (getothdir && transfer->toreq)
    dirhash = gftp_gen_dir_hash (transfer->toreq);
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

      if (transfer->toreq)
        fle->destfile = g_strconcat (transfer->toreq->directory, "/", 
                                     fle->file, NULL);
      newname = g_strconcat (transfer->fromreq->directory, "/", fle->file, NULL);
      g_free (fle->file);
      fle->file = newname;

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
  char *oldfromdir, *oldtodir, *newname;
  GList * templist, * lastlist;
  int forcecd, remotechanged;
  unsigned long *newsize;
  GHashTable * dirhash;
  gftp_file * curfle;

  g_return_val_if_fail (transfer != NULL, -1);
  g_return_val_if_fail (transfer->fromreq != NULL, -1);
  g_return_val_if_fail (transfer->files != NULL, -1);

  if (transfer->toreq != NULL)
    dirhash = gftp_gen_dir_hash (transfer->toreq);
  else
    dirhash = NULL;

  for (lastlist = transfer->files; ; lastlist = lastlist->next)
    {
      curfle = lastlist->data;
      if (dirhash && 
          (newsize = g_hash_table_lookup (dirhash, curfle->file)) != NULL)
        curfle->startsize = *newsize;

      if (transfer->toreq)
        curfle->destfile = g_strconcat (transfer->toreq->directory, "/", 
                                        curfle->file, NULL);
      newname = g_strconcat (transfer->fromreq->directory, transfer->fromreq->directory[strlen (transfer->fromreq->directory) - 1] == '/' ? "" : "/", curfle->file, NULL);
      g_free (curfle->file);
      curfle->file = newname;

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

              lastlist->next = gftp_get_dir_listing (transfer, 
                                                     curfle->startsize > 0);
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
        {
          curfle->ascii = gftp_get_file_transfer_mode (curfle->file, 
                              transfer->fromreq->data_type) == GFTP_TYPE_ASCII;
          transfer->numfiles++;
        }
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


int
gftp_get_file_transfer_mode (char *filename, int def)
{
  gftp_file_extensions * tempext;
  GList * templist;
  int stlen, ret;

  if (!use_default_dl_types)
    return (def);

  ret = def;
  stlen = strlen (filename);
  for (templist = registered_exts; templist != NULL; templist = templist->next)
    {
      tempext = templist->data;

      if (stlen >= tempext->stlen &&
          strcmp (&filename[stlen - tempext->stlen], tempext->ext) == 0)
        {
          if (toupper (*tempext->ascii_binary == 'A'))
            ret = GFTP_TYPE_ASCII;
          else if (toupper (*tempext->ascii_binary == 'B'))
            ret = GFTP_TYPE_BINARY;
          break;
        }
    }

  return (ret);
}


#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
int
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
gftp_connect_server (gftp_request * request, char *service)
{
  char *connect_host, *disphost;
  int port, sock;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo hints, *res;
  char serv[8];
  int errnum;

  if ((request->use_proxy = gftp_need_proxy (request, service)) < 0)
    return (-1);
  else if (request->use_proxy == 1)
    request->hostp = NULL;

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  connect_host = request->use_proxy ? request->proxy_hostname : 
                                      request->hostname;
  port = request->use_proxy ? request->proxy_port : request->port;

  if (request->hostp == NULL)
    {
      if (port == 0)
        strcpy (serv, service); 
      else
        snprintf (serv, sizeof (serv), "%d", port);

      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Looking up %s\n"), connect_host);
      if ((errnum = getaddrinfo (connect_host, serv, &hints, 
                                 &request->hostp)) != 0)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot look up hostname %s: %s\n"),
				     connect_host, gai_strerror (errnum));
	  return (-1);
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
	  request->logging_function (gftp_logging_error, request->user_data,
                                     _("Failed to create a socket: %s\n"),
                                     g_strerror (errno));
          continue; 
        }

      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Trying %s:%d\n"), disphost, port);

      if (connect (sock, res->ai_addr, res->ai_addrlen) == -1)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
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
      return (-1);
    }

#else /* !HAVE_GETADDRINFO */
  struct sockaddr_in remote_address;
  struct servent serv_struct;
  int curhost;

  if ((request->use_proxy = gftp_need_proxy (request, service)) < 0)
    return (-1);
  else if (request->use_proxy == 1)
    request->hostp = NULL;

  if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Failed to create a socket: %s\n"),
                                 g_strerror (errno));
      return (-1);
    }

  memset (&remote_address, 0, sizeof (remote_address));
  remote_address.sin_family = AF_INET;

  connect_host = request->use_proxy ? request->proxy_hostname : 
                                      request->hostname;
  port = htons (request->use_proxy ? request->proxy_port : request->port);

  if (port == 0)
    {
      if (!r_getservbyname (service, "tcp", &serv_struct, NULL))
        {
          port = htons (21);
          request->port = 21;
        }
      else
        {
          port = serv_struct.s_port;
          request->port = ntohs (serv_struct.s_port);
        }

      if (!request->use_proxy)
        request->port = ntohs (port);
    }
  remote_address.sin_port = port;

  if (request->hostp == NULL)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Looking up %s\n"), connect_host);
      if (!(request->hostp = r_gethostbyname (connect_host, &request->host,
                                              NULL)))
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                     _("Cannot look up hostname %s: %s\n"),
                                     connect_host, g_strerror (errno));
          close (sock);
          return (-1);
        }
    }

  disphost = NULL;
  for (curhost = 0; request->host.h_addr_list[curhost] != NULL; curhost++)
    {
      disphost = request->host.h_name;
      memcpy (&remote_address.sin_addr, request->host.h_addr_list[curhost],
              request->host.h_length);
      request->logging_function (gftp_logging_misc, request->user_data,
                                 _("Trying %s:%d\n"),
                                 request->host.h_name, ntohs (port));

      if (connect (sock, (struct sockaddr *) &remote_address,
                   sizeof (remote_address)) == -1)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                     _("Cannot connect to %s: %s\n"),
                                     connect_host, g_strerror (errno));
        }
      break;
    }

  if (request->host.h_addr_list[curhost] == NULL)
    {
      close (sock);
      return (-1);
    }
  port = ntohs (port);
#endif /* HAVE_GETADDRINFO */

  request->logging_function (gftp_logging_misc, request->user_data,
			     _("Connected to %s:%d\n"), connect_host, port);

  if (gftp_set_sockblocking (request, sock, 1) == -1)
    {
      close (sock);
      return (-1);
    }

  return (sock);
}


void
gftp_set_config_options (gftp_request * request)
{
  request->network_timeout = network_timeout;
  request->retries = retries;
  request->sleep_time = sleep_time;
  request->maxkbs = maxkbs;

  if (request->set_config_options != NULL)
    request->set_config_options (request);
}


void
gftp_set_sftpserv_path (gftp_request * request, char *path)
{
  g_return_if_fail (request != NULL);

  if (request->sftpserv_path)
    g_free (request->sftpserv_path);

  if (path != NULL && *path != '\0')
    {
      request->sftpserv_path = g_malloc (strlen (path) + 1);
      strcpy (request->sftpserv_path, path);
    }
  else
    request->sftpserv_path = NULL;
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
#else
      printf ("%s:%s:%ld:%ld:%s:%s:%s\n", 
#endif
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
      if (templist->next == NULL)
        break;
    }

  printf ("--START OF FILE LISTING - BOTTOM TO TOP--\n");
  for (; ; templist = templist->prev)
    {
      tempfle = templist->data;
#if defined (_LARGEFILE_SOURCE)
      printf ("%s:%s:%lld:%lld:%s:%s:%s\n", 
#else
      printf ("%s:%s:%ld:%ld:%s:%s:%s\n", 
#endif
              tempfle->file, tempfle->destfile, 
              tempfle->size, tempfle->startsize, 
              tempfle->user, tempfle->group, tempfle->attribs);
      if (templist == list)
        break;
    }
  printf ("--END OF FILE LISTING--\n");
}


static void
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
  ssize_t ret, retval, rlen, copysize;
  char *pos, *nextpos;

  if (*rbuf == NULL)
    {
      *rbuf = g_malloc0 (sizeof (**rbuf));
      (*rbuf)->max_bufsize = len;
      (*rbuf)->buffer = g_malloc ((*rbuf)->max_bufsize + 1);
      if ((ret = gftp_read (request, (*rbuf)->buffer, (*rbuf)->max_bufsize, 
                            fd)) <= 0)
        {
          gftp_free_getline_buffer (rbuf);
          return (ret);
        }
      (*rbuf)->buffer[ret] = '\0';
      (*rbuf)->cur_bufsize = ret;
      (*rbuf)->curpos = (*rbuf)->buffer;
    }

  retval = -2;

  do
    {
      if ((*rbuf)->cur_bufsize > 0 &&
          ((pos = strchr ((*rbuf)->curpos, '\n')) != NULL ||
           ((*rbuf)->curpos == (*rbuf)->buffer && 
            (*rbuf)->max_bufsize == (*rbuf)->cur_bufsize)))
        {
          if (pos != NULL)
            {
              nextpos = pos + 1;
              if (pos > (*rbuf)->curpos && *(pos - 1) == '\r')
                pos--;
              *pos = '\0';
            }
          else
            nextpos = NULL;

          strncpy (str, (*rbuf)->curpos, len);
          retval = pos - (*rbuf)->curpos > len ? len : pos - (*rbuf)->curpos;

          if (pos != NULL)
            {
              if (nextpos - (*rbuf)->buffer >= (*rbuf)->cur_bufsize)
                (*rbuf)->cur_bufsize = 0;
              else
                (*rbuf)->curpos = nextpos;
            }

          break;
        }
      else
        {
          if ((*rbuf)->cur_bufsize == 0 || *(*rbuf)->curpos == '\0')
            {
              rlen = (*rbuf)->max_bufsize;
              pos = (*rbuf)->buffer;
              copysize = 0;
            }
          else
            {
              copysize = (*rbuf)->cur_bufsize - ((*rbuf)->curpos - (*rbuf)->buffer);
              memmove ((*rbuf)->buffer, (*rbuf)->curpos, copysize);
              pos = (*rbuf)->buffer + copysize;
              rlen = (*rbuf)->max_bufsize - copysize;
            }
          (*rbuf)->curpos = (*rbuf)->buffer;

          if ((ret = gftp_read (request, pos, rlen, fd)) <= 0)
            {
              gftp_free_getline_buffer (rbuf);
              return (ret);
            }
          (*rbuf)->buffer[ret + copysize] = '\0';
          (*rbuf)->cur_bufsize = ret + copysize;
        }
    }
  while (retval == -2);

  return (retval);
}


ssize_t 
gftp_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  struct timeval tv;
  fd_set fset;
  ssize_t ret;

  errno = 0;
  ret = 0;
  do
    {
      FD_ZERO (&fset);
      FD_SET (fd, &fset);
      if (request != NULL)
        tv.tv_sec = request->network_timeout;
      else
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
              request->logging_function (gftp_logging_error, request->user_data,
                                         _("Connection to %s timed out\n"),
                                         request->hostname);
              gftp_disconnect (request);
            }
          return (-1);
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
              request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error: Could not read from socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }
          return (-1);
        }
    }
  while (errno == EINTR && !(request != NULL && request->cancel));

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (-1);
    }

  return (ret);
}


ssize_t 
gftp_write (gftp_request * request, const char *ptr, size_t size, int fd)
{
  struct timeval tv;
  size_t ret, w_ret;
  fd_set fset;

  errno = 0;
  ret = 0;
  do
    {
      FD_ZERO (&fset);
      FD_SET (fd, &fset);
      if (request != NULL)
        tv.tv_sec = request->network_timeout;
      else
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
              request->logging_function (gftp_logging_error, request->user_data,
                                         _("Connection to %s timed out\n"),
                                         request->hostname);
              gftp_disconnect (request);
            }
          return (-1);
        }

      if ((w_ret = write (fd, ptr, size)) < 0)
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
              request->logging_function (gftp_logging_error, request->user_data,
                                    _("Error: Could not write to socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }
          return (-1);
        }

      ptr += w_ret;
      size -= w_ret;
      ret += w_ret;
    }
  while (size > 0);

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (-1);
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

  ret = gftp_write (request, tempstr, strlen (tempstr), fd);
  g_free (tempstr);
  return (ret);
}


int
gftp_set_sockblocking (gftp_request * request, int fd, int non_blocking)
{
  int flags;

  if ((flags = fcntl (fd, F_GETFL, 0)) == -1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot get socket flags: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  if (non_blocking)
    flags |= O_NONBLOCK;
  else
    flags &= ~O_NONBLOCK;

  if (fcntl (fd, F_SETFL, flags) == -1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot set socket to non-blocking: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  return (0);
}


