/*****************************************************************************/
/*  rfc959.c - General purpose routines for the FTP protocol (RFC 959)       */
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

static int rfc959_connect 			( gftp_request * request );
static void rfc959_disconnect 			( gftp_request * request );
static long rfc959_get_file 			( gftp_request * request, 
						  const char *filename,
						  FILE * fd,
						  off_t startsize );
static int rfc959_put_file 			( gftp_request * request, 
						  const char *filename,
						  FILE * fd,
						  off_t startsize,
						  off_t totalsize );
static long rfc959_transfer_file 		( gftp_request *fromreq, 
						  const char *fromfile, 
						  off_t fromsize, 
						  gftp_request *toreq, 
						  const char *tofile, 
						  off_t tosize );
static int rfc959_end_transfer 			( gftp_request * request );
static int rfc959_list_files 			( gftp_request * request );
static int rfc959_set_data_type 		( gftp_request * request, 
						  int data_type );
static off_t rfc959_get_file_size 		( gftp_request * request, 
						  const char *filename );
static int rfc959_data_connection_new 		( gftp_request * request );
static int rfc959_accept_active_connection 	( gftp_request * request );
static int rfc959_send_command 			( gftp_request * request, 
						  const char *command );
static int write_to_socket 			( gftp_request *request, 
						  const char *command );
static int rfc959_read_response 		( gftp_request * request );
static int rfc959_chdir 			( gftp_request * request, 
						  const char *directory );
static int rfc959_rmdir 			( gftp_request * request, 
						  const char *directory );
static int rfc959_rmfile 			( gftp_request * request, 
						  const char *file );
static int rfc959_mkdir 			( gftp_request * request, 
						  const char *directory );
static int rfc959_rename 			( gftp_request * request, 
						  const char *oldname, 
						  const char *newname );
static int rfc959_chmod 			( gftp_request * request, 
						  const char *file, 
						  int mode );
static int rfc959_site 				( gftp_request * request, 
						  const char *command );
static char *parse_ftp_proxy_string 		( gftp_request * request );

void
rfc959_init (gftp_request * request)
{
  g_return_if_fail (request != NULL);

  request->protonum = GFTP_FTP_NUM;
  request->init = rfc959_init;
  request->destroy = NULL; 
  request->connect = rfc959_connect;
  request->disconnect = rfc959_disconnect;
  request->get_file = rfc959_get_file;
  request->put_file = rfc959_put_file;
  request->transfer_file = rfc959_transfer_file;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = rfc959_end_transfer;
  request->list_files = rfc959_list_files;
  request->get_next_file = rfc959_get_next_file;
  request->set_data_type = rfc959_set_data_type;
  request->get_file_size = rfc959_get_file_size;
  request->chdir = rfc959_chdir;
  request->rmdir = rfc959_rmdir;
  request->rmfile = rfc959_rmfile;
  request->mkdir = rfc959_mkdir;
  request->rename = rfc959_rename;
  request->chmod = rfc959_chmod;
  request->set_file_time = NULL;
  request->site = rfc959_site;
  request->parse_url = NULL;
  request->url_prefix = "ftp";
  request->protocol_name = "FTP";
  request->need_hostport = 1;
  request->need_userpass = 1;
  request->use_cache = 1;
  request->use_threads = 1;
  request->always_connected = 0;
  gftp_set_config_options (request);
}


static int
rfc959_connect (gftp_request * request)
{
  char tempchar, *startpos, *endpos, *tempstr, *dir;
  int sock, ret, resp;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->hostname != NULL, -2);
  g_return_val_if_fail (request->username != NULL, -2);

  if (request->sockfd != NULL)
    return (0);

  if ((sock = gftp_connect_server (request, "ftp")) < 0)
    return (-1);

  if ((request->sockfd = fdopen (sock, "rb")) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fdopen() socket: %s\n"),
                                 g_strerror (errno));
      close (sock);
      return (-2);
    }

  if ((request->sockfd_write = fdopen (dup (sock), "wb")) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fdopen() socket: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-2);
    }

  /* Get the banner */
  if (rfc959_read_response (request) != '2')
    {
      gftp_disconnect (request);
      return (-2);
    }

  /* Login the proxy server if available */
  if (request->use_proxy)
    {
      resp = '3';
      startpos = endpos = tempstr = parse_ftp_proxy_string (request);
      while ((resp == '3' || resp == '2') && *startpos != '\0')
	{
	  if (*endpos == '\n' || *endpos == '\0')
	    {
	      tempchar = *(endpos + 1);
	      if (*endpos != '\0')
		*(endpos + 1) = '\0';
	      if ((resp = rfc959_send_command (request, startpos)) < 0)
                return (-2);
	      if (*endpos != '\0')
		*(endpos + 1) = tempchar;
	      else
		break;
	      startpos = endpos + 1;
	    }
	  endpos++;
	}
      g_free (tempstr);
    }
  else
    {
      tempstr = g_strconcat ("USER ", request->username, "\r\n", NULL);
      resp = rfc959_send_command (request, tempstr);
      g_free (tempstr);
      if (resp < 0)
        return (-2);
      if (resp == '3')
	{
	  tempstr = g_strconcat ("PASS ", request->password, "\r\n", NULL);
	  resp = rfc959_send_command (request, tempstr);
	  g_free (tempstr);
          if (resp < 0)
            return (-2);
        }
      if (resp == '3' && request->account)
	{
	  tempstr = g_strconcat ("ACCT ", request->account, "\r\n", NULL);
	  resp = rfc959_send_command (request, tempstr);
	  g_free (tempstr);
          if (resp < 0)
            return (-2);
	}
    }

  if (resp != '2')
    {
      gftp_disconnect (request);
      return (-2);
    }

  if (request->data_type == GFTP_TYPE_BINARY)
    tempstr = "TYPE I\r\n";
  else
    tempstr = "TYPE A\r\n";

  if (rfc959_send_command (request, tempstr) < 0)
    return (-2);

  ret = -1;
  if (request->directory != NULL && *request->directory != '\0')
    {
      ret = rfc959_chdir (request, request->directory);
      if (request->sockfd == NULL)
        return (-2);
    }

  if (ret != 0)
    {
      if (rfc959_send_command (request, "PWD\r\n") != '2' || 
          request->sockfd == NULL)
        {
          gftp_disconnect (request);
  	  return (-2);
        }

      if ((tempstr = strchr (request->last_ftp_response, '"')) == NULL)
        {
          gftp_disconnect (request);
	  return (-2);
        }
      dir = tempstr + 1;

      if ((tempstr = strchr (dir, '"')) == NULL)
	{
          gftp_disconnect (request);
          return (0);
        }
      if (tempstr != NULL)
	*tempstr = '\0';

      request->directory = g_malloc (strlen (dir) + 1);
      strcpy (request->directory, dir);
    }

  if (request->sockfd == NULL)
    return (-2);

  return (0);
}


static void
rfc959_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_FTP_NUM);

  if (request->sockfd != NULL)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Disconnecting from site %s\n"),
				 request->hostname);
      fclose (request->sockfd);
      fclose (request->sockfd_write);
      request->sockfd = request->sockfd_write = NULL;
      if (request->datafd)
	{
	  fclose (request->datafd);
	  request->datafd = NULL;
	}
    }
}


static long
rfc959_get_file (gftp_request * request, const char *filename, FILE * fd,
                 off_t startsize)
{
  char *command, *tempstr, resp;
  int ret, flags;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if (fd != NULL)
    request->datafd = fd;

  if (request->datafd == NULL && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  flags = fcntl (fileno (request->datafd), F_GETFL, 0);
  if (fcntl (fileno (request->datafd), F_SETFL, flags | O_NONBLOCK) < 0)
    {
      fclose (request->datafd);
      request->datafd = NULL;
      return (-1);
    }

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST %ld\r\n", startsize); 
      resp = rfc959_send_command (request, command);
      g_free (command);

      if (resp != '3')
        {
          fclose (request->datafd);
          request->datafd = NULL;
	  return (-2);
        }
    }

  tempstr = g_strconcat ("RETR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);

  if (ret != '1')
    {
      fclose (request->datafd);
      request->datafd = NULL;
      return (-2);
    }

  if (request->transfer_type == gftp_transfer_active &&
      (ret = rfc959_accept_active_connection (request)) < 0)
    return (ret);

  if ((tempstr = strrchr (request->last_ftp_response, '(')) == NULL)
    {
      tempstr = request->last_ftp_response + 4;
      while (!isdigit ((int) *tempstr) && *tempstr != '\0')
	tempstr++;
    }
  else
    tempstr++;

  return (strtol (tempstr, NULL, 10) + startsize);
}


static int
rfc959_put_file (gftp_request * request, const char *filename, FILE * fd,
                 off_t startsize, off_t totalsize)
{
  char *command, *tempstr, resp;
  int ret, flags;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if (fd != NULL)
    fd = request->datafd;

  if (request->datafd == NULL && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  flags = fcntl (fileno (request->datafd), F_GETFL, 0);
  if (fcntl (fileno (request->datafd), F_SETFL, flags | O_NONBLOCK) < 0)
    {
      fclose (request->datafd);
      request->datafd = NULL;
      return (-1);
    }

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST %ld\r\n", startsize);  
      resp = rfc959_send_command (request, command);
      g_free (command);
      if (resp != '3')
        {
          fclose (request->datafd);
          request->datafd = NULL;
	  return (-2);
        }
    }

  tempstr = g_strconcat ("STOR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  if (ret != '1')
    {
      fclose (request->datafd);
      request->datafd = NULL;
      return (-2);
    }

  if (request->transfer_type == gftp_transfer_active && 
      (ret = rfc959_accept_active_connection (request)) < 0)
    return (ret);

  return (0);
}

static long 
rfc959_transfer_file (gftp_request *fromreq, const char *fromfile, 
                      off_t fromsize, gftp_request *toreq, 
                      const char *tofile, off_t tosize)
{
  char *tempstr, *pos, *endpos;

  g_return_val_if_fail (fromreq != NULL, -2);
  g_return_val_if_fail (fromfile != NULL, -2);
  g_return_val_if_fail (toreq != NULL, -2);
  g_return_val_if_fail (tofile != NULL, -2);
  g_return_val_if_fail (fromreq->sockfd != NULL, -2);
  g_return_val_if_fail (toreq->sockfd != NULL, -2);

  fromreq->transfer_type = gftp_transfer_passive;
  toreq->transfer_type = gftp_transfer_active;

  if (rfc959_send_command (fromreq, "PASV\r\n") != '2' ||
      fromreq->sockfd == NULL)
    return (-2);

  pos = fromreq->last_ftp_response + 4;
  while (!isdigit ((int) *pos) && *pos != '\0') 
    pos++;
  if (*pos == '\0') 
    return (-2);

  endpos = pos;
  while (*endpos != ')' && *endpos != '\0') 
    endpos++;
  if (*endpos == ')') 
    *endpos = '\0';

  tempstr = g_strconcat ("PORT ", pos, "\r\n", NULL);
  g_free (tempstr);
  if (rfc959_send_command (toreq, tempstr) != '2')
    return (-2);

  tempstr = g_strconcat ("RETR ", fromfile, "\r\n", NULL);
  g_free (tempstr);
  if (write_to_socket (fromreq, tempstr) < 0)
    return (-1);

  tempstr = g_strconcat ("STOR ", tofile, "\r\n", NULL);
  g_free (tempstr);
  if (write_to_socket (toreq, tempstr) < 0)
    return (-1);

  if (rfc959_read_response (fromreq) < 0)
    return (-2);
  if (rfc959_read_response (toreq) < 0)
    return (-2);

  return (0);
}


static int
rfc959_end_transfer (gftp_request * request)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if (request->datafd)
    {
      fclose (request->datafd);
      request->datafd = NULL;
    }
  return (rfc959_read_response (request) == '2' ? 0 : -2);
}


static int
rfc959_list_files (gftp_request * request)
{
  char *tempstr, parms[3];
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if ((ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  *parms = '\0';
  strcat (parms, show_hidden_files ? "a" : "");
  strcat (parms, resolve_symlinks ? "L" : "");
  tempstr = g_strconcat ("LIST", *parms != '\0' ? " -" : "", parms, "\r\n", 
                         NULL); 

  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);

  if (ret != '1')
    return (-2);

  ret = 0;
  if (request->transfer_type == gftp_transfer_active)
    ret = rfc959_accept_active_connection (request);

  return (ret);
}


int
rfc959_get_next_file (gftp_request * request, gftp_file * fle, FILE * fd)
{
  char tempstr[255];
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (fle != NULL, -2);
  g_return_val_if_fail (fd != NULL, -2);

  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
    }
  do
    {
      /* I don't run select() here because select could return 
         successfully saying there is data, but the fgets call could block if 
         there is no carriage return */
      if (!fgets (tempstr, sizeof (tempstr), fd))
	{
          gftp_file_destroy (fle);
          if (ferror (fd))
            gftp_disconnect (request);
	  return (-2);
	} 
      tempstr[sizeof (tempstr) - 1] = '\0';

      if (gftp_parse_ls (tempstr, fle) != 0)
	{
	  if (strncmp (tempstr, "total", strlen("total")) != 0 &&
	      strncmp (tempstr, _("total"), strlen(_("total"))) != 0)
	    request->logging_function (gftp_logging_misc, request->user_data,
				       _("Warning: Cannot parse listing %s\n"),
				       tempstr);
	  gftp_file_destroy (fle);
	  continue;
	}
      else
	break;
    }
  while (1);

  len = strlen (tempstr);
  if (!request->cached)
    {
      request->last_dir_entry = g_malloc (len + 1);
      strcpy (request->last_dir_entry, tempstr);
      request->last_dir_entry_len = len;
    }
  return (len);
}


static int
rfc959_set_data_type (gftp_request * request, int data_type)
{
  char *tempstr;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);

  if (request->sockfd != NULL && request->data_type != data_type)
    {
      if (data_type == GFTP_TYPE_BINARY)
	tempstr = "TYPE I\r\n";
      else
	tempstr = "TYPE A\r\n";

      if (rfc959_send_command (request, tempstr) != '2')
	return (-2);
    }
  request->data_type = data_type;
  return (0);
}


static off_t
rfc959_get_file_size (gftp_request * request, const char *filename)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, 0);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, 0);
  g_return_val_if_fail (request->sockfd != NULL, 0);

  tempstr = g_strconcat ("SIZE ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  if (ret < 0)
    return (-2);

  if (*request->last_ftp_response != '2')
    return (0);
  return (strtol (request->last_ftp_response + 4, NULL, 10));
}


static int
rfc959_data_connection_new (gftp_request * request)
{
  char *pos, *pos1, resp, *command;
  struct sockaddr_in data_addr;
  size_t data_addr_len;
  unsigned int temp[6];
  unsigned char ad[6];
  int i, sock;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Failed to create a socket: %s\n"),
				 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  data_addr_len = sizeof (data_addr);
  memset (&data_addr, 0, data_addr_len);
  data_addr.sin_family = AF_INET;

  if (request->transfer_type == gftp_transfer_passive)
    {
      if ((resp = rfc959_send_command (request, "PASV\r\n")) != '2')
	{
          if (request->sockfd == NULL)
            return (-2);

	  request->transfer_type = gftp_transfer_active;
	  return (rfc959_data_connection_new (request));
	}
      pos = request->last_ftp_response + 4;
      while (!isdigit ((int) *pos) && *pos != '\0')
        pos++;
      if (*pos == '\0')
        {
          gftp_disconnect (request);
          close (sock);
          return (-2);
        }
      if (sscanf (pos, "%u,%u,%u,%u,%u,%u", &temp[0], &temp[1], &temp[2],
                  &temp[3], &temp[4], &temp[5]) != 6)
        {
          gftp_disconnect (request);
          close (sock);
          return (-2);
        }
      for (i = 0; i < 6; i++)
        ad[i] = (unsigned char) (temp[i] & 0xff);

      memcpy (&data_addr.sin_addr, &ad[0], 4);
      memcpy (&data_addr.sin_port, &ad[4], 2);
      if (connect (sock, (struct sockaddr *) &data_addr, data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                    _("Cannot create a data connection: %s\n"),
                                    g_strerror (errno));
          gftp_disconnect (request);
          close (sock);
          return (-1);
	}
    }
  else
    {
      getsockname (fileno (request->sockfd), (struct sockaddr *) &data_addr,
		   &data_addr_len);
      data_addr.sin_port = 0;
      if (bind (sock, (struct sockaddr *) &data_addr, data_addr_len) == -1)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot bind a port: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  close (sock);
	  return (-1);
	}

      getsockname (sock, (struct sockaddr *) &data_addr, &data_addr_len);
      if (listen (sock, 1) == -1)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot listen on port %d: %s\n"),
				     ntohs (data_addr.sin_port),
				     g_strerror (errno));
          gftp_disconnect (request);
	  close (sock);
	  return (-1);
	}
      pos = (char *) &data_addr.sin_addr;
      pos1 = (char *) &data_addr.sin_port;
      command = g_strdup_printf ("PORT %u,%u,%u,%u,%u,%u\r\n",
				 pos[0] & 0xff, pos[1] & 0xff, pos[2] & 0xff,
				 pos[3] & 0xff, pos1[0] & 0xff,
				 pos1[1] & 0xff);
      resp = rfc959_send_command (request, command);
      g_free (command);
      if (resp != '2')
	{
          gftp_disconnect (request);
	  close (sock);
	  return (-2);
	}
    }

  if ((request->datafd = fdopen (sock, "rb+")) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fdopen() socket: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-2);
    }

  return (0);
}


static int
rfc959_accept_active_connection (gftp_request * request)
{
  struct sockaddr_in cli_addr;
  size_t cli_addr_len;
  int infd;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->datafd != NULL, -2);
  g_return_val_if_fail (request->transfer_type == gftp_transfer_active, -2);

  cli_addr_len = sizeof (cli_addr);
  if ((infd = accept (fileno (request->datafd), (struct sockaddr *) &cli_addr,
       &cli_addr_len)) == -1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                _("Cannot accept connection from server: %s\n"),
                                g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  fclose (request->datafd);

  if ((request->datafd = fdopen (infd, "rb+")) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fdopen() socket: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-2);
    }
  return (0);
}


static int
rfc959_send_command (gftp_request * request, const char *command)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (command != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  if (strncmp (command, "PASS", 4) == 0)
    {
      request->logging_function (gftp_logging_send, request->user_data, 
                                 "PASS xxxx\n");
    }
  else if (strncmp (command, "ACCT", 4) == 0)
    {
      request->logging_function (gftp_logging_send, request->user_data, 
                                 "ACCT xxxx\n");
    }
  else
    {
      request->logging_function (gftp_logging_send, request->user_data, "%s",
                                 command);
    }

  if (write_to_socket (request, command) < 0)
    {
      gftp_disconnect (request);
      return (-1);
    }

  return (rfc959_read_response (request));
}

static int
write_to_socket (gftp_request *request, const char *command)
{
  struct timeval tv;
  fd_set fset;

  FD_ZERO (&fset);
  FD_SET (fileno (request->sockfd_write), &fset);
  tv.tv_sec = request->network_timeout;
  tv.tv_usec = 0;
  if (!select (fileno (request->sockfd_write) + 1, NULL, &fset, NULL, &tv))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Connection to %s timed out\n"),
                                 request->hostname);
      gftp_disconnect (request);
      return (-1);
    }

  fwrite (command, strlen (command), 1, request->sockfd_write);
  if (ferror (request->sockfd_write))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: Could not write to socket: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  fflush (request->sockfd_write);
  if (ferror (request->sockfd_write))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: Could not write to socket: %s\n"),
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  return (0);
}
    


static int
rfc959_read_response (gftp_request * request)
{
  char tempstr[255], code[4];

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  *code = '\0';
  if (request->last_ftp_response)
    {
      g_free (request->last_ftp_response);
      request->last_ftp_response = NULL;
    }

  do
    {
      /* I don't run select() here because select could return 
         successfully saying there is data, but the fgets call could block if 
         there is no carriage return */
      if (!fgets (tempstr, sizeof (tempstr), request->sockfd))
	break;
      tempstr[strlen (tempstr) - 1] = '\0';
      if (tempstr[strlen (tempstr) - 1] == '\r')
	tempstr[strlen (tempstr) - 1] = '\0';
      if (isdigit ((int) *tempstr) && isdigit ((int) *(tempstr + 1))
	  && isdigit ((int) *(tempstr + 2)))
	{
	  strncpy (code, tempstr, 3);
	  code[3] = ' ';
	}
      request->logging_function (gftp_logging_recv, request->user_data,
				 "%s\n", tempstr);
    }
  while (strncmp (code, tempstr, 4) != 0);

  if (ferror (request->sockfd)) 
    {
      request->logging_function (gftp_logging_send, request->user_data,
                                 "Error reading from socket: %s\n",
                                 g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  request->last_ftp_response = g_malloc (strlen (tempstr) + 1);
  strcpy (request->last_ftp_response, tempstr);

  if (*request->last_ftp_response == '4')
    gftp_disconnect (request);

  return (*request->last_ftp_response);
}


static int
rfc959_chdir (gftp_request * request, const char *directory)
{
  char ret, *tempstr, *dir;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);

  if (strcmp (directory, "..") == 0)
    ret = rfc959_send_command (request, "CDUP\r\n");
  else
    {
      tempstr = g_strconcat ("CWD ", directory, "\r\n", NULL);
      ret = rfc959_send_command (request, tempstr);
      g_free (tempstr);
    }

  if (ret != '2')
    return (-2);

  if (directory != request->directory)
    {
      if (request->directory)
        {
          g_free (request->directory);
          request->directory = NULL;
        }

      if (rfc959_send_command (request, "PWD\r\n") != '2')
        return (-2);

      tempstr = strchr (request->last_ftp_response, '"');
      if (tempstr != NULL)
        dir = tempstr + 1;
      else
        return (-2);

      tempstr = strchr (dir, '"');
      if (tempstr != NULL)
        *tempstr = '\0';
      else
        {
          gftp_disconnect (request);
          return (-2);
        }

      request->directory = g_malloc (strlen (dir) + 1);
      strcpy (request->directory, dir);
    }

  return (0);
}


static int
rfc959_rmdir (gftp_request * request, const char *directory)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_strconcat ("RMD ", directory, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (ret == '2' ? 0 : -2);
}


static int
rfc959_rmfile (gftp_request * request, const char *file)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (file != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_strconcat ("DELE ", file, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (ret == '2' ? 0 : -2);
}


static int
rfc959_mkdir (gftp_request * request, const char *directory)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_strconcat ("MKD ", directory, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (ret == '2' ? 0 : -2);
}


static int
rfc959_rename (gftp_request * request, const char *oldname,
	       const char *newname)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (oldname != NULL, -2);
  g_return_val_if_fail (newname != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_strconcat ("RNFR ", oldname, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  if (ret != '3')
    return (-2);

  tempstr = g_strconcat ("RNTO ", newname, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (ret == '2' ? 0 : -2);
}


static int
rfc959_chmod (gftp_request * request, const char *file, int mode)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (file != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_malloc (strlen (file) + (mode / 10) + 16);
  sprintf (tempstr, "SITE CHMOD %d %s\r\n", mode, file);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (ret == '2' ? 0 : -2);
}


static int
rfc959_site (gftp_request * request, const char *command)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (command != NULL, -2);
  g_return_val_if_fail (request->sockfd != NULL, -2);

  tempstr = g_strconcat ("SITE ", command, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (request->sockfd != NULL ? ret : -2);
}


static char *
parse_ftp_proxy_string (gftp_request * request)
{
  char *startpos, *endpos, *oldstr, *newstr, *newval, *tempport;

  g_return_val_if_fail (request != NULL, NULL);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, NULL);

  newstr = g_malloc (1);
  *newstr = '\0';
  startpos = endpos = request->proxy_config;
  while (*endpos != '\0')
    {
      tempport = NULL;
      if (*endpos == '%' && tolower ((int) *(endpos + 1)) == 'p')
	{
	  switch (tolower ((int) *(endpos + 2)))
	    {
	    case 'u':
	      newval = request->proxy_username;
	      break;
	    case 'p':
	      newval = request->proxy_password;
	      break;
	    case 'h':
	      newval = request->proxy_hostname;
	      break;
	    case 'o':
	      tempport = g_strdup_printf ("%d", request->proxy_port);
	      newval = tempport;
	      break;
	    case 'a':
	      newval = request->proxy_account;
	      break;
	    default:
	      endpos++;
	      continue;
	    }
	}
      else if (*endpos == '%' && tolower ((int) *(endpos + 1)) == 'h')
	{
	  switch (tolower ((int) *(endpos + 2)))
	    {
	    case 'u':
	      newval = request->username;
	      break;
	    case 'p':
	      newval = request->password;
	      break;
	    case 'h':
	      newval = request->hostname;
	      break;
	    case 'o':
	      tempport = g_strdup_printf ("%d", request->port);
	      newval = tempport;
	      break;
	    case 'a':
	      newval = request->account;
	      break;
	    default:
	      endpos++;
	      continue;
	    }
	}
      else if (*endpos == '%' && tolower ((int) *(endpos + 1)) == 'n')
	{
	  *endpos = '\0';
	  oldstr = newstr;
	  newstr = g_strconcat (oldstr, startpos, "\r\n", NULL);
	  g_free (oldstr);
	  endpos += 2;
	  startpos = endpos;
	  continue;
	}
      else
	{
	  endpos++;
	  continue;
	}

      *endpos = '\0';
      oldstr = newstr;
      if (!newval)
	newstr = g_strconcat (oldstr, startpos, NULL);
      else
	newstr = g_strconcat (oldstr, startpos, newval, NULL);
      if (tempport)
	{
	  g_free (tempport);
	  tempport = NULL;
	}
      g_free (oldstr);
      endpos += 3;
      startpos = endpos;
    }
  return (newstr);
}

