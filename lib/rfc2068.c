/*****************************************************************************/
/*  rfc2068.c - General purpose routines for the HTTP protocol               */
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
#include "httpcommon.h"

static const char cvsid[] = "$Id$";

static gftp_config_vars config_vars[] =
{
  {"", N_("HTTP"), gftp_option_type_notebook, NULL, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},

  {"http_proxy_host", N_("Proxy hostname:"), 
   gftp_option_type_text, "", NULL, 0, 
   N_("Firewall hostname"), GFTP_PORT_ALL, 0},
  {"http_proxy_port", N_("Proxy port:"), 
   gftp_option_type_int, GINT_TO_POINTER(80), NULL, 0,
   N_("Port to connect to on the firewall"), GFTP_PORT_ALL, NULL},
  {"http_proxy_username", N_("Proxy username:"), 
   gftp_option_type_text, "", NULL, 0,
   N_("Your firewall username"), GFTP_PORT_ALL, NULL},
  {"http_proxy_password", N_("Proxy password:"), 
   gftp_option_type_hidetext, "", NULL, 0,
   N_("Your firewall password"), GFTP_PORT_ALL, NULL},

  {"use_http11", N_("Use HTTP/1.1"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Do you want to use HTTP/1.1 or HTTP/1.0"), GFTP_PORT_ALL, NULL},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};


static int
rfc2068_connect (gftp_request * request)
{
  char *proxy_hostname, *proxy_config;
  rfc2068_params * params;
  intptr_t proxy_port;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (request->datafd > 0)
    return (0);

  gftp_lookup_request_option (request, "proxy_config", &proxy_config);
  gftp_lookup_request_option (request, "http_proxy_host", &proxy_hostname);
  gftp_lookup_request_option (request, "http_proxy_port", &proxy_port);

  if (proxy_config != NULL && strcmp (proxy_config, "ftp") == 0)
    {
      g_free (request->url_prefix);
      request->url_prefix = g_strdup ("ftp");
    }

  if ((ret = gftp_connect_server (request, request->url_prefix, proxy_hostname, 
                                  proxy_port)) < 0)
    return (ret);

  if (request->directory && *request->directory == '\0')
    {
      g_free (request->directory);
      request->directory = NULL;
    }

  if (!request->directory)
    request->directory = g_strdup ("/");

  return (0);
}


static off_t 
rfc2068_read_response (gftp_request * request)
{
  rfc2068_params * params;
  char tempstr[8192];
  int ret, chunked;

  params = request->protocol_data;
  *tempstr = '\0';
  chunked = 0;
  params->chunk_size = 0;
  params->content_length = 0;
  params->eof = 0;

  if (request->last_ftp_response)
    {
      g_free (request->last_ftp_response); 
      request->last_ftp_response = NULL;
    }

  if (params->extra_read_buffer != NULL)
    {
      g_free (params->extra_read_buffer);
      params->extra_read_buffer = NULL;
      params->extra_read_buffer_len = 0;
    }

  do
    {
      if ((ret = gftp_get_line (request, &params->rbuf, tempstr, 
                                sizeof (tempstr), request->datafd)) < 0)
        return (ret);

      if (request->last_ftp_response == NULL)
        request->last_ftp_response = g_strdup (tempstr);

      if (*tempstr != '\0')
        {
          request->logging_function (gftp_logging_recv, request, "%s\n",
                                     tempstr);

          if (strncmp (tempstr, "Content-Length:", 15) == 0)
            params->content_length = gftp_parse_file_size (tempstr + 16);
          else if (strcmp (tempstr, "Transfer-Encoding: chunked") == 0)
            chunked = 1;
        }
    }
  while (*tempstr != '\0');

  if (chunked)
    {
      if ((ret = gftp_get_line (request, &params->rbuf, tempstr, 
                                sizeof (tempstr), request->datafd)) < 0)
        return (ret);

      if (sscanf ((char *) tempstr, "%lx", &params->chunk_size) != 1)
        {
          request->logging_function (gftp_logging_recv, request,
                                     _("Received wrong response from server, disconnecting\nInvalid chunk size '%s' returned by the remote server\n"), 
                                     tempstr);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      if (params->chunk_size == 0)
        return (0);

      params->chunk_size -= params->rbuf->cur_bufsize;
      if (params->chunk_size < 0)
        {
          params->extra_read_buffer_len = params->chunk_size * -1;
          params->chunk_size = 0;
          params->extra_read_buffer = g_malloc (params->extra_read_buffer_len + 1);
          memcpy (params->extra_read_buffer, params->rbuf->curpos + (params->rbuf->cur_bufsize - params->extra_read_buffer_len), params->extra_read_buffer_len);
          params->extra_read_buffer[params->extra_read_buffer_len] = '\0';
          params->rbuf->cur_bufsize -= params->extra_read_buffer_len;
          params->rbuf->curpos[params->rbuf->cur_bufsize] = '\0';
        }
    }

  params->chunked_transfer = chunked;
  return (params->content_length > 0 ? params->content_length : params->chunk_size);
}


static off_t 
rfc2068_send_command (gftp_request * request, const void *command, size_t len)
{
  char *tempstr, *str, *proxy_hostname, *proxy_username, *proxy_password;
  intptr_t proxy_port;
  int conn_ret;
  ssize_t ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (command != NULL, GFTP_EFATAL);

  if (request->datafd < 0 && (conn_ret = rfc2068_connect (request)) != 0)
    return (conn_ret);

  tempstr = g_strdup_printf ("%sUser-Agent: %s\nHost: %s\n", (char *) command,
                             gftp_version, request->hostname);

  request->logging_function (gftp_logging_send, request,
                             "%s", tempstr);

  ret = request->write_function (request, tempstr, strlen (tempstr), 
                                 request->datafd);
  g_free (tempstr);

  if (ret < 0)
    return (ret);

  gftp_lookup_request_option (request, "http_proxy_host", &proxy_hostname);
  gftp_lookup_request_option (request, "http_proxy_port", &proxy_port);
  gftp_lookup_request_option (request, "http_proxy_username", &proxy_username);
  gftp_lookup_request_option (request, "http_proxy_password", &proxy_password);

  if (request->use_proxy && proxy_username != NULL && *proxy_username != '\0')
    {
      tempstr = g_strconcat (proxy_username, ":", proxy_password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request,
                                 "Proxy-authorization: Basic xxxx:xxxx\n");
      ret = gftp_writefmt (request, request->datafd, 
                           "Proxy-authorization: Basic %s\n", str);
      g_free (str);
      if (ret < 0)
        return (ret);
    }

  if (request->username != NULL && *request->username != '\0')
    {
      tempstr = g_strconcat (request->username, ":", request->password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request,
                                 "Authorization: Basic xxxx\n");
      ret = gftp_writefmt (request, request->datafd, 
                           "Authorization: Basic %s\n", str);
      g_free (str);
      if (ret < 0)
        return (ret);
    }

  if ((ret = request->write_function (request, "\n", 1, request->datafd)) < 0)
    return (ret);

  return (rfc2068_read_response (request));
}


static void
rfc2068_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_HTTP_NUM);

  if (request->datafd > 0)
    {
      request->logging_function (gftp_logging_misc, request,
				 _("Disconnecting from site %s\n"),
				 request->hostname);

      if (close (request->datafd) < 0)
        request->logging_function (gftp_logging_error, request,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));

      request->datafd = -1;
    }
}


static off_t
rfc2068_get_file (gftp_request * request, const char *filename, int fd,
                  off_t startsize)
{
  char *tempstr, *oldstr, *hf;
  int restarted;
  intptr_t use_http11;
  rfc2068_params * params;
  off_t size;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  gftp_lookup_request_option (request, "use_http11", &use_http11);

  if (fd > 0)
    request->datafd = fd;

  hf = gftp_build_path (request->hostname, filename, NULL);

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("GET ", request->url_prefix, "://", hf,
                     use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);
  else 
    tempstr = g_strconcat ("GET ", request->url_prefix, "://", 
                     request->username, "@", hf,
                     use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);

  g_free (hf);

  if (use_http11 && startsize > 0)
    {
#if defined (_LARGEFILE_SOURCE)
      request->logging_function (gftp_logging_misc, request,
                              _("Starting the file transfer at offset %lld\n"),
                              startsize);

      oldstr = tempstr;
      tempstr = g_strdup_printf ("%sRange: bytes=%lld-\n", tempstr, startsize);
      g_free (oldstr);
#else
      request->logging_function (gftp_logging_misc, request,
			       _("Starting the file transfer at offset %ld\n"), 
                               startsize);

      oldstr = tempstr;
      tempstr = g_strdup_printf ("%sRange: bytes=%ld-\n", tempstr, startsize);
      g_free (oldstr);
#endif
    }

  size = rfc2068_send_command (request, tempstr, strlen (tempstr));
  g_free (tempstr);
  if (size < 0)
    return (size);

  restarted = 0;
  if (strlen (request->last_ftp_response) > 9 
      && strncmp (request->last_ftp_response + 9, "206", 3) == 0)
    restarted = 1;
  else if (strlen (request->last_ftp_response) < 9 ||
           strncmp (request->last_ftp_response + 9, "200", 3) != 0)
    {
      request->logging_function (gftp_logging_error, request,
			         _("Cannot retrieve file %s\n"), filename);
      return (GFTP_ERETRYABLE);
    }

  params->read_bytes = 0;

  return (restarted ? size + startsize : size);
}


static ssize_t
rfc2068_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  rfc2068_params * params;
  size_t len;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);

  params = request->protocol_data;
  if (params->rbuf != NULL && params->rbuf->curpos != NULL)
    {
      len = params->rbuf->cur_bufsize > size ? size : params->rbuf->cur_bufsize;
      memcpy (buf, params->rbuf->curpos, len);

      if (len == params->rbuf->cur_bufsize)
        gftp_free_getline_buffer (&params->rbuf);
      else
        {
          params->rbuf->curpos += len;
          params->rbuf->cur_bufsize -= len;
        }

      return (len);
    }

  return (request->read_function (request, buf, size, request->datafd));
}


static int
rfc2068_end_transfer (gftp_request * request)
{
  rfc2068_params * params;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);

  if (request->datafd < 0)
    return (GFTP_EFATAL);

  gftp_disconnect (request);

  params = request->protocol_data;
  params->content_length = 0;
  params->chunked_transfer = 0;
  params->chunk_size = 0;
  params->eof = 0;

  return (0);
}


static int
rfc2068_list_files (gftp_request * request)
{
  rfc2068_params *params;
  char *tempstr, *hd;
  intptr_t use_http11;
  off_t ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);

  params = request->protocol_data;
  gftp_lookup_request_option (request, "use_http11", &use_http11);

  hd = gftp_build_path (request->hostname, request->directory, NULL);

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("GET ", request->url_prefix, "://", hd,
                           use_http11 ? "/ HTTP/1.1\n" : "/ HTTP/1.0\n", NULL);
  else
    tempstr = g_strconcat ("GET ", request->url_prefix, "://", 
                           request->username, "@", hd,
                           use_http11 ? "/ HTTP/1.1\n" : "/ HTTP/1.0\n", NULL);

  g_free (hd);

  ret = rfc2068_send_command (request, tempstr, strlen (tempstr));
  g_free (tempstr);
  if (ret < 0)
    return ((int) ret);

  params->read_bytes = 0;
  if (strlen (request->last_ftp_response) > 9 &&
      strncmp (request->last_ftp_response + 9, "200", 3) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Retrieving directory listing...\n"));
      return (0);
    }

  gftp_end_transfer (request);

  return (GFTP_ERETRYABLE);
}


static off_t 
rfc2068_get_file_size (gftp_request * request, const char *filename)
{
  rfc2068_params *params;
  char *tempstr, *hf;
  intptr_t use_http11;
  off_t size;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  params = request->protocol_data;
  gftp_lookup_request_option (request, "use_http11", &use_http11);

  hf = gftp_build_path (request->hostname, filename, NULL);

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("HEAD ", request->url_prefix, "://", hf,
                           use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);
  else
    tempstr = g_strconcat ("HEAD ", request->url_prefix, "://", 
                           request->username, "@", hf,
                           use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);

  g_free (hf);

  size = rfc2068_send_command (request, tempstr, strlen (tempstr));
  g_free (tempstr);
  return (size);
}


static int
parse_html_line (char *tempstr, gftp_file * fle)
{
  char *stpos, *kpos, *mpos, *pos;
  long units;

  memset (fle, 0, sizeof (*fle));

  if ((pos = strstr (tempstr, "<A HREF=")) == NULL && 
      (pos = strstr (tempstr, "<a href=")) == NULL)
    return (0);

  /* Find the filename */
  while (*pos != '"' && *pos != '\0')
    pos++;
  if (*pos == '\0')
    return (0);
  pos++;

  for (stpos = pos; *pos != '"' && *pos != '\0'; pos++);
  if (*pos == '\0')
    return (0);
  *pos = '\0';

  /* Copy file attributes. Just about the only thing we can get is whether it
     is a directory or not */
  fle->attribs = g_strdup ("-rw-------");
  if (*(pos - 1) == '/')
    {
      *(pos - 1) = '\0';
      *fle->attribs = 'd';
      fle->isdir = 1;
    }

  /* Copy filename */
  if (strchr (stpos, '/') != NULL || strncmp (stpos, "mailto:", 7) == 0 ||
      *stpos == '\0' || *stpos == '?')
    return (0);

  fle->file = g_strdup (stpos);

  if (*(pos - 1) == '\0')
    *(pos - 1) = '/';
  *pos = '"';
  pos++;

  /* Skip whitespace and html tags after file and before date */
  stpos = pos;
  if ((pos = strstr (stpos, "</A>")) == NULL &&
      (pos = strstr (stpos, "</a>")) == NULL)
    return (0);

  pos += 4;

  while (*pos == ' ' || *pos == '\t' || *pos == '.' || *pos == '<')
    {
      if (*pos == '<')
	{
	  if (strncmp (pos, "<A ", 3) == 0 || strncmp (pos, "<a ", 3) == 0)
	    {
	      stpos = pos;
	      if ((pos = strstr (stpos, "</A>")) == NULL 
                   && (pos = strstr (stpos, "</a>")) == NULL)
		return (0);
	      pos += 4;
	    }
	  else
	    {
	      while (*pos != '>' && *pos != '\0')
		pos++;
	      if (*pos == '\0')
		return (0);
	    }
	}
      pos++;
    }

  if (*pos == '[')
    pos++;

  fle->datetime = parse_time (pos, &pos);

  fle->user = g_strdup (_("<unknown>"));
  fle->group = g_strdup (_("<unknown>"));

  if (pos == NULL)
    return (1);

  while (*pos == ' ' || *pos == ']')
    pos++;

  /* Get the size */

  kpos = strchr (pos, 'k');
  mpos = strchr (pos, 'M');
  if (kpos == NULL)
    stpos = mpos;
  else if (mpos == NULL)
    stpos = kpos;
  else if (kpos < stpos)
    stpos = kpos;
  else
    stpos = mpos;

  if (stpos == NULL || !isdigit (*(stpos - 1)))
     return (1);			/* Return successfully
					   since we got the file */

  if (*stpos == 'k')
    units = 1024;
  else
    units = 1048576;

  fle->size = 0;

  while (*(stpos - 1) != ' ' && *(stpos - 1) != '\t' && stpos > tempstr) 
    {
      stpos--;
      if ((*stpos == '.') && isdigit (*(stpos + 1))) 
        {  /* found decimal point */
          fle->size = units * gftp_parse_file_size (stpos + 1) / 10;;
        }
    }

  fle->size += units * gftp_parse_file_size (stpos);

  return (1);
}


int
rfc2068_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc2068_params * params;
  char tempstr[8192];
  ssize_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  params = request->protocol_data;
  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
    }

  if (fd < 0)
    fd = request->datafd;

  while (1)
    {
      if ((ret = gftp_get_line (request, &params->rbuf, tempstr, sizeof (tempstr), fd)) <= 0)
        return (ret);

      if (parse_html_line (tempstr, fle) == 0 || fle->file == NULL)
	gftp_file_destroy (fle);
      else
	break;
    }

  if (fle->file == NULL)
    {
      gftp_file_destroy (fle);
      return (0);
    }

  len = strlen (tempstr);
  if (!request->cached)
    {
      request->last_dir_entry = g_strdup_printf ("%s\n", tempstr);
      request->last_dir_entry_len = len + 1;
    }

  return (len);
}


static int
rfc2068_chdir (gftp_request * request, const char *directory)
{
  char *tempstr, *olddir;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (request->directory != directory)
    {
      olddir = request->directory;

      if (*directory != '/' && request->directory != NULL)
        {
          tempstr = g_strconcat (request->directory, "/", directory, NULL);
          request->directory = expand_path (tempstr);
          g_free (tempstr);
        }
      else
        request->directory = expand_path (directory);

      if (olddir != NULL)
        g_free (olddir);
    }
  return (0);
}


static int
rfc2068_set_config_options (gftp_request * request)
{
  return (0);
}


static void
rfc2068_destroy (gftp_request * request)
{
  rfc2068_params * params;

  params = request->protocol_data;

  if (request->url_prefix)
    {
      g_free (request->url_prefix);
      request->url_prefix = NULL;
    }

  if (params->rbuf != NULL)
    gftp_free_getline_buffer (&params->rbuf);

  if (params->extra_read_buffer != NULL)
    {
      g_free (params->extra_read_buffer);
      params->extra_read_buffer = NULL;
      params->extra_read_buffer_len = 0;
    }
}


static ssize_t 
rfc2068_chunked_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  size_t read_size, begin_ptr_len, current_size, crlfsize;
  rfc2068_params * params;
  char *stpos, *crlfpos;
  void *read_ptr_pos;
  ssize_t retval;

  params = request->protocol_data;
  params->read_ref_cnt++;

  if (params->extra_read_buffer != NULL)
    {
      g_return_val_if_fail (params->extra_read_buffer_len <= size, GFTP_EFATAL);

      memcpy (ptr, params->extra_read_buffer, params->extra_read_buffer_len);

      begin_ptr_len = params->extra_read_buffer_len;
      read_ptr_pos = (char *) ptr + begin_ptr_len;

      /* Check for end of chunk */
      if (begin_ptr_len > 5 && strncmp ("\r\n0\r\n", (char *) ptr, 5) == 0)
        read_size = 0;
      else
        read_size = size - begin_ptr_len;

      g_free (params->extra_read_buffer);
      params->extra_read_buffer = NULL;
      params->extra_read_buffer_len = 0;
    }
  else
    {
      begin_ptr_len = 0;
      read_ptr_pos = ptr;

      read_size = size;
      if (params->content_length > 0)
        {
          if (params->content_length == params->read_bytes)
            {
              params->read_ref_cnt--;
              return (0);
            }

          if (read_size + params->read_bytes > params->content_length)
            read_size = params->content_length - params->read_bytes;
        }
      else if (params->chunked_transfer && params->chunk_size > 0 &&
               params->chunk_size < read_size)
        read_size = params->chunk_size;
    }

  if (read_size > 0 && !params->eof)
    {
      if (size == read_size)
        read_size--; /* decrement by one so that we can put the NUL character
                        in the buffer */

      retval = params->real_read_function (request, read_ptr_pos, read_size,
                                           fd);

      if (retval <= 0)
        {
          params->read_ref_cnt--;
          return (retval);
        }
      else if (retval > 0)
        params->read_bytes += retval;

      if (params->chunk_size > 0)
        {
          params->chunk_size -= retval;
          params->read_ref_cnt--;
          return (retval);
        }

      retval += begin_ptr_len;
    }
  else
    retval = begin_ptr_len;

  ((char *) ptr)[retval] = '\0';

  if (!params->chunked_transfer)
    {
      params->read_ref_cnt--;
      return (retval);
    }

  stpos = (char *) ptr;
  while (params->chunk_size == 0)
    {
      current_size = retval - (stpos - (char *) ptr);
      if (current_size == 0)
        break;

      if (*stpos == '\r' && *(stpos + 1) == '\n')
        crlfpos = strstr (stpos + 2, "\r\n");
      else
        crlfpos = NULL;

      if (crlfpos == NULL)
        {
          /* The current chunk size is split between multiple packets.
             Save this chunk and read the next */

          params->extra_read_buffer = g_malloc (current_size + 1);
          memcpy (params->extra_read_buffer, stpos, current_size);
          params->extra_read_buffer[current_size] = '\0';
          params->extra_read_buffer_len = current_size;
          retval -= current_size;

          if (retval == 0)
            {
              /* Don't let a hostile web server send us in an infinite recursive
                 loop */

              if (params->read_ref_cnt > 2)
                {
                  request->logging_function (gftp_logging_error, request, _("Received wrong response from server, disconnecting\n"));
                  gftp_disconnect (request);
                  params->read_ref_cnt--;
                  return (GFTP_EFATAL);
                }

              retval = rfc2068_chunked_read (request, ptr, size, fd);
            }

          params->read_ref_cnt--;
          return (retval);
        }

      *crlfpos = '\0';
      crlfpos++; /* advance to line feed */

      if (sscanf (stpos + 2, "%lx", &params->chunk_size) != 1)
        {
          request->logging_function (gftp_logging_recv, request,
                                     _("Received wrong response from server, disconnecting\nInvalid chunk size '%s' returned by the remote server\n"), 
                                     stpos + 2);
          gftp_disconnect (request);
          params->read_ref_cnt--;
          return (GFTP_EFATAL);
        }

      crlfsize = crlfpos - (char *) stpos + 1;
      retval -= crlfsize;
      current_size -= crlfsize;

      if (params->chunk_size == 0)
        {
          if (params->eof)
            {
              params->read_ref_cnt--;
              return (0);
            }

          params->eof = 1;
          params->read_ref_cnt--;
          return (retval);
        }

      memmove (stpos, crlfpos + 1, current_size + 1);

      if (params->chunk_size < current_size)
        {
          stpos += params->chunk_size;
          params->chunk_size = 0;
        }
      else
        params->chunk_size -= current_size;
    }

  params->read_ref_cnt--;
  return (retval);
}


void 
rfc2068_register_module (void)
{
  gftp_register_config_vars (config_vars);
}


int
rfc2068_init (gftp_request * request)
{
  rfc2068_params * params;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_HTTP_NUM;
  request->init = rfc2068_init;
  request->copy_param_options = NULL;
  request->read_function = rfc2068_chunked_read;
  request->write_function = gftp_fd_write;
  request->destroy = rfc2068_destroy;
  request->connect = rfc2068_connect;
  request->post_connect = NULL;
  request->disconnect = rfc2068_disconnect;
  request->get_file = rfc2068_get_file;
  request->put_file = NULL;
  request->transfer_file = NULL;
  request->get_next_file_chunk = rfc2068_get_next_file_chunk;
  request->put_next_file_chunk = NULL;
  request->end_transfer = rfc2068_end_transfer;
  request->abort_transfer = rfc2068_end_transfer; /* NOTE: uses end_transfer */
  request->list_files = rfc2068_list_files;
  request->get_next_file = rfc2068_get_next_file;
  request->get_file_size = rfc2068_get_file_size;
  request->chdir = rfc2068_chdir;
  request->rmdir = NULL;
  request->rmfile = NULL;
  request->mkdir = NULL;
  request->rename = NULL;
  request->chmod = NULL;
  request->site = NULL;
  request->parse_url = NULL;
  request->swap_socks = NULL;
  request->set_config_options = rfc2068_set_config_options;
  request->url_prefix = g_strdup ("http");
  request->need_hostport = 1;
  request->need_userpass = 0;
  request->use_cache = 1;
  request->use_threads = 1;
  request->always_connected = 1;

  request->protocol_data = g_malloc0 (sizeof (rfc2068_params));
  params = request->protocol_data;
  params->real_read_function = gftp_fd_read;

  return (gftp_set_config_options (request));
}

