/*****************************************************************************/
/*  rfc2068.c - General purpose routines for the HTTP protocol               */
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

static int rfc2068_connect 			( gftp_request * request );
static void rfc2068_disconnect 			( gftp_request * request );
static long rfc2068_get_file 			( gftp_request * request, 
						  const char *filename, 
						  FILE * fd,
						  off_t startsize );
static size_t rfc2068_get_next_file_chunk 	( gftp_request * request, 
						  char *buf, 
						  size_t size );
static int rfc2068_end_transfer 		( gftp_request * request );
static int rfc2068_list_files 			( gftp_request * request );
static off_t rfc2068_get_file_size 		( gftp_request * request, 
						  const char *filename );
static int rfc2068_get_next_file 		( gftp_request * request, 
						  gftp_file * fle, 
						  FILE * fd );
static int rfc2068_chdir 			( gftp_request * request, 
						  const char *directory );
static unsigned long rfc2068_send_command 	( gftp_request * request, 
						  const char *command, 
						  const char *extrahdr );
static unsigned long rfc2068_read_response 	( gftp_request * request );
static int parse_html_line 			( char *tempstr, 
						  gftp_file * fle );
static char *base64_encode 			( char *str );

typedef struct rfc2068_params_tag
{
  unsigned long read_bytes,
                max_bytes;
  int chunked_transfer : 1;
} rfc2068_params;

void
rfc2068_init (gftp_request * request)
{
  g_return_if_fail (request != NULL);

  request->protonum = GFTP_HTTP_NUM;
  request->init = rfc2068_init;
  request->destroy = NULL;
  request->connect = rfc2068_connect;
  request->disconnect = rfc2068_disconnect;
  request->get_file = rfc2068_get_file;
  request->put_file = NULL;
  request->transfer_file = NULL;
  request->get_next_file_chunk = rfc2068_get_next_file_chunk;
  request->put_next_file_chunk = NULL;
  request->end_transfer = rfc2068_end_transfer;
  request->list_files = rfc2068_list_files;
  request->get_next_file = rfc2068_get_next_file;
  request->set_data_type = NULL;
  request->get_file_size = rfc2068_get_file_size;
  request->chdir = rfc2068_chdir;
  request->rmdir = NULL;
  request->rmfile = NULL;
  request->mkdir = NULL;
  request->rename = NULL;
  request->chmod = NULL;
  request->site = NULL;
  request->url_prefix = "http";
  request->protocol_name = "HTTP";
  request->need_hostport = 1;
  request->need_userpass = 0;
  request->use_cache = 1;
  request->use_threads = 1;
  request->always_connected = 0;
  request->protocol_data = g_malloc0 (sizeof (rfc2068_params));
  gftp_set_config_options (request);
}


static int
rfc2068_connect (gftp_request * request)
{
  char *service;
  int sock;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (request->hostname != NULL, -2);

  if (request->datafd != NULL)
    return (0);

  service = request->use_proxy && request->proxy_config != NULL && *request->proxy_config != '\0' ? request->proxy_config : "http";
  if ((sock = gftp_connect_server (request, service)) < 0)
    return (-1);

  if (request->directory && *request->directory == '\0')
    {
      g_free (request->directory);
      request->directory = NULL;
    }

  if (!request->directory)
    {
      request->directory = g_malloc (2);
      strcpy (request->directory, "/");
    }

  request->sockfd = request->datafd = fdopen (sock, "rb+");
  return (0);
}


static void
rfc2068_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_HTTP_NUM);

  if (request->datafd != NULL)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Disconnecting from site %s\n"),
				 request->hostname);
      fclose (request->datafd);
      request->datafd = NULL;
    }
  request->sockfd = NULL;
}


static long 
rfc2068_get_file (gftp_request * request, const char *filename, FILE * fd,
                  off_t startsize)
{
  char *tempstr, *extrahdr, *pos, *proto;
  int restarted;
  off_t size;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);

  if (fd != NULL)
    request->datafd = fd;

  if (request->datafd == NULL && rfc2068_connect (request) != 0)
    return (-2);

  if (request->proxy_config != NULL && *request->proxy_config != '\0')
    proto = request->proxy_config;
  else
    proto = "http";

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("GET ", proto, "://", 
                     request->hostname, "/", filename, 
                     use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);
  else 
    tempstr = g_strconcat ("GET ", proto, "://", 
                     request->username, "@", request->hostname, "/", filename, 
                     use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);

  if ((pos = strstr (tempstr, "://")) != NULL)
    remove_double_slashes (pos + 3);
  else
    remove_double_slashes (tempstr);

  if (!use_http11 || startsize == 0)
    extrahdr = NULL;
  else
    {
      extrahdr = g_strdup_printf ("Range: bytes=%ld-\n", startsize);
      request->logging_function (gftp_logging_misc, request->user_data,
			       _("Starting the file transfer at offset %ld\n"), 
                               startsize);
    }

  size = rfc2068_send_command (request, tempstr, extrahdr);
  g_free (tempstr);
  if (extrahdr)
    g_free (extrahdr);
  if (request->datafd == NULL)
    return (-2);

  restarted = 0;
  if (strlen (request->last_ftp_response) > 9 
      && strncmp (request->last_ftp_response + 9, "206", 3) == 0)
    restarted = 1;
  else if (strlen (request->last_ftp_response) < 9 
           || strncmp (request->last_ftp_response + 9, "200", 3) != 0)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
			         _("Cannot retrieve file %s\n"), filename);
      return (-2);
    }

  return (restarted ? size + startsize : size);
}


static size_t
rfc2068_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  rfc2068_params * params;
  struct timeval tv;
  fd_set fset;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);

  params = request->protocol_data;
  if (params->max_bytes == params->read_bytes || feof (request->datafd))
    return (0);

  FD_ZERO (&fset);
  FD_SET (fileno (request->datafd), &fset);
  tv.tv_sec = request->network_timeout;
  tv.tv_usec = 0;
  if (select (fileno (request->datafd) + 1, &fset, NULL, NULL, &tv) <= 0)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Connection to %s timed out\n"),
                                 request->hostname);
      gftp_disconnect (request);
      return (-1);
    }

  if (params->max_bytes > 0 && size + params->read_bytes > params->max_bytes)
    size = params->max_bytes - params->read_bytes;

  len = fread (buf, 1, size, request->datafd);
  if (ferror (request->datafd))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error reading from host %s: %s\n"),
                                 request->hostname,
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  params->read_bytes += len;
  return (len);
}


static int
rfc2068_end_transfer (gftp_request * request)
{
  rfc2068_params * params;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (request->datafd != NULL, -2);

  if (request->datafd == NULL)
    return (-2);

  params = request->protocol_data;
  fclose (request->datafd);
  request->datafd = NULL;
  params->max_bytes = 0;
  request->logging_function (gftp_logging_misc, request->user_data,
                             _("Finished retrieving data\n"));
  return (0);
}


static int
rfc2068_list_files (gftp_request * request)
{
  char *tempstr, *pos, *proto;
  rfc2068_params *params;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);

  params = request->protocol_data;
  if (request->datafd == NULL && rfc2068_connect (request) != 0)
    return (-2);

  if (request->proxy_config != NULL && *request->proxy_config != '\0')
    proto = request->proxy_config;
  else
    proto = "http";

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("GET ", proto, "://", 
                           request->hostname, "/", request->directory, 
                           use_http11 ? "/ HTTP/1.1\n" : "/ HTTP/1.0\n", NULL);
  else
    tempstr = g_strconcat ("GET ", proto, "://", request->username, "@", 
                           request->hostname, "/", request->directory, 
                           use_http11 ? "/ HTTP/1.1\n" : "/ HTTP/1.0\n", NULL);

  if ((pos = strstr (tempstr, "://")) != NULL)
    remove_double_slashes (pos + 3);
  else
    remove_double_slashes (tempstr);

  rfc2068_send_command (request, tempstr, NULL);
  g_free (tempstr);
  if (request->datafd == NULL)
    return (-2);

  params->read_bytes = 0;
  if (strlen (request->last_ftp_response) > 9 &&
      strncmp (request->last_ftp_response + 9, "200", 3) == 0)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
                                 _("Retrieving directory listing...\n"));
      return (0);
    }
  
  return (-2);
}


static off_t 
rfc2068_get_file_size (gftp_request * request, const char *filename)
{
  char *tempstr, *pos, *proto;
  unsigned long size;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);

  if (request->datafd == NULL && rfc2068_connect (request) != 0)
    return (-2);

  if (request->proxy_config != NULL && *request->proxy_config != '\0')
    proto = request->proxy_config;
  else
    proto = "http";

  if (request->username == NULL || *request->username == '\0')
    tempstr = g_strconcat ("HEAD ", proto, request->hostname, "/", filename, 
                           use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);
  else
    tempstr = g_strconcat ("HEAD ", proto, request->username, "@", 
                           request->hostname, "/", filename, 
                           use_http11 ? " HTTP/1.1\n" : " HTTP/1.0\n", NULL);

  if ((pos = strstr (tempstr, "://")) != NULL)
    remove_double_slashes (pos + 3);
  else
    remove_double_slashes (tempstr);

  size = rfc2068_send_command (request, tempstr, NULL);
  g_free (tempstr);
  if (request->datafd == NULL)
    return (-2);

  return (size);
}


static int
rfc2068_get_next_file (gftp_request * request, gftp_file * fle, FILE * fd)
{
  rfc2068_params * params;
  char tempstr[255];
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (fle != NULL, -2);

  params = request->protocol_data;
  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
    }

  while (1)
    {
      /* I don't run select() here because select could return 
         successfully saying there is data, but the fgets call could block if 
         there is no carriage return */
      if (!fgets (tempstr, sizeof (tempstr), fd))
	{
	  gftp_file_destroy (fle);
	  return (-2);
	}
      tempstr[sizeof (tempstr) - 1] = '\0';
      params->read_bytes += strlen (tempstr);

      if (params->chunked_transfer && strcmp (tempstr, "0\r\n") == 0)
        {
          /* I don't run select() here because select could return 
             successfully saying there is data, but the fgets call could block 
             if there is no carriage return */
          while (fgets (tempstr, sizeof (tempstr), fd))
            {
              if (strcmp (tempstr, "\r\n") == 0)
                break;
            }
	  gftp_file_destroy (fle);
          return (0);
        }

      if (parse_html_line (tempstr, fle) == 0 || fle->file == NULL)
	gftp_file_destroy (fle);
      else
	break;

      if (params->max_bytes != 0 && params->read_bytes == params->max_bytes)
	break;
    }

  if (fle->file == NULL)
    {
      gftp_file_destroy (fle);
      return (-2);
    }

  len = strlen (tempstr);
  if (!request->cached)
    {
      request->last_dir_entry = g_malloc (len + 1);
      strcpy (request->last_dir_entry, tempstr);
      request->last_dir_entry_len = len;
    }
  return (feof (fd) ? 0 : len);
}


static int
rfc2068_chdir (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);

  if (request->directory != directory)
    {
      if (request->directory)
        g_free (request->directory);
      request->directory = g_malloc (strlen (directory) + 1);
      strcpy (request->directory, directory);
    }
  return (0);
}


static unsigned long
rfc2068_send_command (gftp_request * request, const char *command,
                      const char *extrahdr)
{
  char *tempstr, *str;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_HTTP_NUM, -2);
  g_return_val_if_fail (command != NULL, -2);
  g_return_val_if_fail (request->datafd != NULL, -2);

  request->logging_function (gftp_logging_send, request->user_data, "%s",
                             command);

  request->logging_function (gftp_logging_send, request->user_data,
                             "User-Agent: %s\n", version);
  fprintf (request->datafd, "%sUser-Agent: %s\nHost: %s\n", command, version,
           request->hostname);
  if (ferror (request->datafd) != 0)
    {
      gftp_disconnect (request);
      return (-2);
    }

  if (request->use_proxy && request->proxy_username != NULL &&
      *request->proxy_username != '\0')
    {
      tempstr = g_strconcat (request->proxy_username, ":", 
                             request->proxy_password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request->user_data,
                                 "Proxy-authorization: Basic xxxx:xxxx\n");
      fprintf (request->datafd, "Proxy-authorization: Basic %s\n", str);
      g_free (str);
      if (ferror (request->datafd) != 0)
        {
          gftp_disconnect (request);
	  return (-2);
        }
    }

  if (request->username != NULL && *request->username != '\0')
    {
      tempstr = g_strconcat (request->username, ":", request->password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request->user_data,
                                 "Authorization: Basic xxxx\n");
      fprintf (request->datafd, "Authorization: Basic %s\n", str);
      g_free (str);
      if (ferror (request->datafd) != 0)
        {
          gftp_disconnect (request);
	  return (-2);
        }
    }

  if (extrahdr)
    {
      request->logging_function (gftp_logging_send, request->user_data,
                                 "%s", extrahdr);
      request->logging_function (gftp_logging_send, request->user_data, "%s",
                                 extrahdr);
      fprintf (request->datafd, "%s", extrahdr);
    }

  fprintf (request->datafd, "\n");
  if (ferror (request->datafd) != 0)
    {
      gftp_disconnect (request);
      return (-2);
    }
  return (rfc2068_read_response (request));
}


static unsigned long
rfc2068_read_response (gftp_request * request)
{
  rfc2068_params * params;
  char tempstr[255];

  params = request->protocol_data;
  params->max_bytes = 0;
  /* I don't run select() here because select could return 
     successfully saying there is data, but the fgets call could block if 
     there is no carriage return */
  if (!fgets (tempstr, sizeof (tempstr), request->datafd))
    {
      gftp_disconnect (request);
      return (0);
    }
  if (request->last_ftp_response)
    g_free (request->last_ftp_response);
  request->last_ftp_response = g_malloc (strlen (tempstr) + 1);
  strcpy (request->last_ftp_response, tempstr);

  request->logging_function (gftp_logging_recv, request->user_data, "%s",
                             tempstr);

  params->chunked_transfer = 0;
  while (1) 
    {
      /* I don't run select() here because select could return 
         successfully saying there is data, but the fgets call could block if 
         there is no carriage return */
      /* Read rest of proxy header */
      if (!fgets (tempstr, sizeof (tempstr), request->datafd))
        {
          gftp_disconnect (request);
	  return (0);
        }

      if (*tempstr == '\r' || *tempstr == '\n')
        break;

      request->logging_function (gftp_logging_recv, request->user_data, "%s",
                                 tempstr);

      if (strncmp (tempstr, "Content-Length:", 15) == 0)
	params->max_bytes = strtol (tempstr + 16, NULL, 10);
      if (strncmp (tempstr, "Transfer-Encoding: chunked", 26) == 0)
        params->chunked_transfer = 1;
    }

  return (params->max_bytes);
}


static int
parse_html_line (char *tempstr, gftp_file * fle)
{
  char months[13][3] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
                        "Sep", "Oct", "Nov", "Dec"};
  char *stpos, *pos, month[4];
  struct tm t;
  long units;
  int i;

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
  fle->attribs = g_malloc (11);
  strcpy (fle->attribs, "----------");
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

  fle->file = g_malloc (strlen (stpos) + 1);
  strcpy (fle->file, stpos);

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

  while (*pos == ' ' || *pos == '.' || *pos == '<')
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

  /* Now get the date */
  memset (&t, 0, sizeof (t));
  memset (month, 0, sizeof (month));
  if (strchr (pos, ':') != NULL)
    {
      if (*pos == '[')
	pos++;
      sscanf (pos, "%02d-%3s-%04d %02d:%02d", &t.tm_mday, month, &t.tm_year,
	      &t.tm_hour, &t.tm_min);
      while (*pos != ' ' && *pos != '\0')
	pos++;
      if (*pos == '\0')
	return (1);

      while (*pos == ' ')
	pos++;

      while (*pos != ' ' && *pos != '\0')
	pos++;
      if (*pos == '\0')
	return (1);

      t.tm_year -= 1900;
    }
  else
    {
      pos++;
      strncpy (month, pos, 3);
      for (i=0; i<3 && *pos != '\0'; i++)
        pos++;
      if (*pos == '\0')
	return (1);

      while (*pos == ' ')
	pos++;

      t.tm_mday = strtol (pos, NULL, 10);
      while (*pos != ' ' && *pos != '\0')
	pos++;
      if (*pos == '\0')
	return (1);

      while (*pos == ' ')
	pos++;

      t.tm_year = strtol (pos, NULL, 10) - 1900;
      while (*pos != ' ' && *pos != '\0')
	pos++;
      if (*pos == '\0')
	return (1);
    }

  for (i=0; i<12; i++)
    {
      if (strncmp (month, months[i], 3) == 0)
        {
          t.tm_mon = i;
          break;
        }
    }

  fle->datetime = mktime (&t);

  while (*pos == ' ' || *pos == ']')
    pos++;

  /* Get the size */
  /* This gets confusing on lines like "... 1.1M RedHat RPM package" */
  /* We need to avoid finding the 'k' in package */
 
  stpos = strchr (pos, 'k');
  if (stpos == NULL || !isdigit (*(stpos - 1)))
     stpos = strchr (pos, 'M');
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
      if ((*stpos == '.') && isdigit (*(stpos + 1)) ) 
        {  /* found decimal point */
          fle->size = units * strtol (stpos + 1, NULL, 10);
          fle->size /= 10;
        }
    }

  fle->size += units * strtol (stpos, NULL, 10);
  return (1);
}


static char *
base64_encode (char *str)
{

/* The standard to Base64 encoding can be found in RFC2045 */

  char *newstr, *newpos, *fillpos, *pos;
  unsigned char table[64], encode[3];
  int i, num;

  for (i = 0; i < 26; i++)
    {
      table[i] = 'A' + i;
      table[i + 26] = 'a' + i;
    }

  for (i = 0; i < 10; i++)
    table[i + 52] = '0' + i;

  table[62] = '+';
  table[63] = '-';

  num = strlen (str) / 3;
  if (strlen (str) % 3 > 0)
    num++;
  newstr = g_malloc (num * 4 + 1);
  newstr[num * 4] = '\0';
  newpos = newstr;

  pos = str;
  while (*pos != '\0')
    {
      memset (encode, 0, sizeof (encode));
      for (i = 0; i < 3 && *pos != '\0'; i++)
	encode[i] = *pos++;

      fillpos = newpos;
      *newpos++ = table[encode[0] >> 2];
      *newpos++ = table[(encode[0] & 3) << 4 | encode[1] >> 4];
      *newpos++ = table[(encode[1] & 0xF) << 2 | encode[2] >> 6];
      *newpos++ = table[encode[2] & 0x3F];
      while (i < 3)
	fillpos[++i] = '=';
    }
  return (newstr);
}
