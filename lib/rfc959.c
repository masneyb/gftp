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
static const char cvsid[] = "$Id$";

typedef struct rfc959_params_tag
{
  gftp_getline_buffer * sockfd_rbuf,
                      * datafd_rbuf;
} rfc959_parms;


static int
rfc959_read_response (gftp_request * request)
{
  char tempstr[255], code[4];
  rfc959_parms * parms;
  ssize_t num_read;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

  *code = '\0';
  if (request->last_ftp_response)
    {
      g_free (request->last_ftp_response);
      request->last_ftp_response = NULL;
    }

  parms = request->protocol_data;

  do
    {
      if ((num_read = gftp_get_line (request, &parms->sockfd_rbuf, tempstr, 
                                     sizeof (tempstr), request->sockfd)) <= 0)
	break;

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

  if (num_read < 0)
    return (-1);

  request->last_ftp_response = g_malloc (strlen (tempstr) + 1);
  strcpy (request->last_ftp_response, tempstr);

  if (request->last_ftp_response[0] == '4' &&
      request->last_ftp_response[1] == '2')
    gftp_disconnect (request);

  return (*request->last_ftp_response);
}


static int
rfc959_send_command (gftp_request * request, const char *command)
{
  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (command != NULL, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

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

  if (gftp_write (request, command, strlen (command), 
                   request->sockfd) < 0)
    return (-1);

  return (rfc959_read_response (request));
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


static int
rfc959_getcwd (gftp_request * request)
{
  char *pos, *dir;
  int ret;

  ret = rfc959_send_command (request, "PWD\r\n");
  if (ret < 0)
    return (-1);
  else if (ret != '2')
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Received invalid response to PWD command: '%s'\n"),
                                 request->last_ftp_response);
      gftp_disconnect (request);
      return (-2);
    }

  if ((pos = strchr (request->last_ftp_response, '"')) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Received invalid response to PWD command: '%s'\n"),
                                 request->last_ftp_response);
      gftp_disconnect (request);
      return (-2);
    }

  dir = pos + 1;

  if ((pos = strchr (dir, '"')) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Received invalid response to PWD command: '%s'\n"),
                                 request->last_ftp_response);
      gftp_disconnect (request);
      return (-2);
    }

  *pos = '\0';

  if (request->directory)
    g_free (request->directory);

  request->directory = g_malloc (strlen (dir) + 1);
  strcpy (request->directory, dir);
  return (0);
}


static int
rfc959_chdir (gftp_request * request, const char *directory)
{
  char ret, *tempstr;

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
      if (rfc959_getcwd (request) < 0)
        return (-1);
    }

  return (0);
}


static int
rfc959_connect (gftp_request * request)
{
  char tempchar, *startpos, *endpos, *tempstr;
  int ret, resp;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->hostname != NULL, -2);

  if (request->sockfd > 0)
    return (0);

  if (request->username == NULL || *request->username == '\0')
    {
      gftp_set_username (request, "anonymous");
      gftp_set_password (request, emailaddr);
    }
  else if (strcasecmp (request->username, "anonymous") == 0)
    gftp_set_password (request, emailaddr);
    
  if ((request->sockfd = gftp_connect_server (request, "ftp")) < 0)
    return (-1);

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
      if (request->sockfd < 0)
        return (-2);
    }

  if (ret != 0)
    {
      if (rfc959_getcwd (request) < 0)
        return (-1);
    }

  if (request->sockfd < 0)
    return (-2);

  return (0);
}


static void
rfc959_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_FTP_NUM);

  if (request->sockfd > 0)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Disconnecting from site %s\n"),
				 request->hostname);
      close (request->sockfd);
      request->sockfd = -1;
      if (request->datafd > 0)
	{
	  close (request->datafd);
	  request->datafd = -1;
	}
    }
}


static int
rfc959_data_connection_new (gftp_request * request)
{
  char *pos, *pos1, resp, *command;
  struct sockaddr_in data_addr;
  size_t data_addr_len;
  unsigned int temp[6];
  unsigned char ad[6];
  int i;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

  if ((request->datafd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
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
          if (request->sockfd < 0)
            return (-2);

	  request->transfer_type = gftp_transfer_active;
	  return (rfc959_data_connection_new (request));
	}

      pos = request->last_ftp_response + 4;
      while (!isdigit ((int) *pos) && *pos != '\0')
        pos++;

      if (*pos == '\0')
        {
          request->logging_function (gftp_logging_error, request->user_data,
                      _("Cannot find an IP address in PASV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (-2);
        }

      if (sscanf (pos, "%u,%u,%u,%u,%u,%u", &temp[0], &temp[1], &temp[2],
                  &temp[3], &temp[4], &temp[5]) != 6)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                      _("Cannot find an IP address in PASV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (-2);
        }

      for (i = 0; i < 6; i++)
        ad[i] = (unsigned char) (temp[i] & 0xff);

      memcpy (&data_addr.sin_addr, &ad[0], 4);
      memcpy (&data_addr.sin_port, &ad[4], 2);
      if (connect (request->datafd, (struct sockaddr *) &data_addr, 
                   data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                    _("Cannot create a data connection: %s\n"),
                                    g_strerror (errno));
          gftp_disconnect (request);
          return (-1);
	}
    }
  else
    {
      if (getsockname (request->sockfd, (struct sockaddr *) &data_addr,
                       &data_addr_len) == -1)
        {
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot get socket name: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (-1);
        }

      data_addr.sin_port = 0;
      if (bind (request->datafd, (struct sockaddr *) &data_addr, 
                data_addr_len) == -1)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot bind a port: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (-1);
	}

      if (getsockname (request->datafd, (struct sockaddr *) &data_addr, 
                       &data_addr_len) == -1)
        {
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot get socket name: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (-1);
        }

      if (listen (request->datafd, 1) == -1)
	{
	  request->logging_function (gftp_logging_error, request->user_data,
				     _("Cannot listen on port %d: %s\n"),
				     ntohs (data_addr.sin_port),
				     g_strerror (errno));
          gftp_disconnect (request);
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
	  return (-2);
	}
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
  g_return_val_if_fail (request->datafd > 0, -2);
  g_return_val_if_fail (request->transfer_type == gftp_transfer_active, -2);

  cli_addr_len = sizeof (cli_addr);

  if (gftp_set_sockblocking (request, request->datafd, 0) == -1)
    return (-1);

  if ((infd = accept (request->datafd, (struct sockaddr *) &cli_addr,
       &cli_addr_len)) == -1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                _("Cannot accept connection from server: %s\n"),
                                g_strerror (errno));
      gftp_disconnect (request);
      return (-1);
    }

  close (request->datafd);

  request->datafd = infd;
  if (gftp_set_sockblocking (request, request->datafd, 1) == -1)
    return (-1);

  return (0);
}


static off_t
rfc959_get_file (gftp_request * request, const char *filename, int fd,
                 off_t startsize)
{
  char *command, *tempstr, resp;
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

  if (fd > 0)
    request->datafd = fd;

  if (request->datafd < 0 && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  if (gftp_set_sockblocking (request, request->datafd, 1) == -1)
    return (-1);

  if (startsize > 0)
    {
#if defined (_LARGEFILE_SOURCE)
      command = g_strdup_printf ("REST %lld\r\n", startsize); 
#else
      command = g_strdup_printf ("REST %ld\r\n", startsize); 
#endif
      resp = rfc959_send_command (request, command);
      g_free (command);

      if (resp != '3')
        {
          close (request->datafd);
          request->datafd = -1;
	  return (-2);
        }
    }

  tempstr = g_strconcat ("RETR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);

  if (ret != '1')
    {
      close (request->datafd);
      request->datafd = -1;
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
rfc959_put_file (gftp_request * request, const char *filename, int fd,
                 off_t startsize, off_t totalsize)
{
  char *command, *tempstr, resp;
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

  if (fd > 0)
    fd = request->datafd;

  if (request->datafd < 0 && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  if (gftp_set_sockblocking (request, request->datafd, 1) == -1)
    return (-1);

  if (startsize > 0)
    {
#if defined (_LARGEFILE_SOURCE)
      command = g_strdup_printf ("REST %lld\r\n", startsize); 
#else
      command = g_strdup_printf ("REST %ld\r\n", startsize); 
#endif
      resp = rfc959_send_command (request, command);
      g_free (command);
      if (resp != '3')
        {
          close (request->datafd);
          request->datafd = -1;
	  return (-2);
        }
    }

  tempstr = g_strconcat ("STOR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  if (ret != '1')
    {
      close (request->datafd);
      request->datafd = -1;
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
  g_return_val_if_fail (fromreq->sockfd > 0, -2);
  g_return_val_if_fail (toreq->sockfd > 0, -2);

  fromreq->transfer_type = gftp_transfer_passive;
  toreq->transfer_type = gftp_transfer_active;

  if (rfc959_send_command (fromreq, "PASV\r\n") != '2')
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
  if (rfc959_send_command (toreq, tempstr) != '2')
     {
       g_free (tempstr);
       return (-2);
     }
  g_free (tempstr);

  tempstr = g_strconcat ("RETR ", fromfile, "\r\n", NULL);
  if (gftp_write (fromreq, tempstr, strlen (tempstr), 
                   fromreq->sockfd) < 0)
    {
      g_free (tempstr);
      return (-2);
    }
  g_free (tempstr);

  tempstr = g_strconcat ("STOR ", tofile, "\r\n", NULL);
  if (gftp_write (toreq, tempstr, strlen (tempstr), toreq->sockfd) < 0)
    {
      g_free (tempstr);
      return (-2);
    }
  g_free (tempstr);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

  if (request->datafd > 0)
    {
      close (request->datafd);
      request->datafd = -1;
    }
  return (rfc959_read_response (request) == '2' ? 0 : -2);
}


static int
rfc959_abort_transfer (gftp_request * request)
{
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

  if (request->datafd > 0)
    {
      close (request->datafd);
      request->datafd = -1;
    }

  /* We need to read two lines of output. The first one is acknowleging
     the transfer and the second line acknowleges the ABOR command */
  if (rfc959_send_command (request, "ABOR\r\n") < 0)
    return (-2);

  if (request->sockfd > 0)
    {
      if ((ret = rfc959_read_response (request)) < 0)
        gftp_disconnect (request);
    }
  
  return (0);
}


static int
rfc959_list_files (gftp_request * request)
{
  char *tempstr, parms[3];
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

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
rfc959_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc959_parms * parms;
  char tempstr[255];
  ssize_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (fle != NULL, -2);
  g_return_val_if_fail (fd > 0, -2);

  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
    }

  parms = request->protocol_data;

  do
    {
      if ((len = gftp_get_line (request, &parms->datafd_rbuf,
                                tempstr, sizeof (tempstr), fd)) <= 0)
	{
          gftp_file_destroy (fle);
	  return ((int) len);
	} 

      if (gftp_parse_ls (tempstr, fle) != 0)
	{
	  if (strncmp (tempstr, "total", strlen ("total")) != 0 &&
	      strncmp (tempstr, _("total"), strlen (_("total"))) != 0)
	    request->logging_function (gftp_logging_error, request->user_data,
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
      request->last_dir_entry = g_strdup_printf ("%s\n", tempstr);
      request->last_dir_entry_len = len + 1;
    }
  return (len);
}


static int
rfc959_set_data_type (gftp_request * request, int data_type)
{
  char *tempstr;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);

  if (request->sockfd > 0 && request->data_type != data_type)
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
  g_return_val_if_fail (request->sockfd > 0, 0);

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
rfc959_rmdir (gftp_request * request, const char *directory)
{
  char *tempstr, ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_FTP_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);
  g_return_val_if_fail (request->sockfd > 0, -2);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

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
  g_return_val_if_fail (request->sockfd > 0, -2);

  tempstr = g_strconcat ("SITE ", command, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr);
  g_free (tempstr);
  return (request->sockfd > 0 ? ret : -2);
}


static void
rfc959_set_config_options (gftp_request * request)
{
  request->transfer_type = passive_transfer ? gftp_transfer_passive : gftp_transfer_active;

  if (strcmp (proxy_config, "http") != 0)
    {
      gftp_set_proxy_hostname (request, firewall_host);
      gftp_set_proxy_port (request, firewall_port);
      gftp_set_proxy_username (request, firewall_username);
      gftp_set_proxy_password (request, firewall_password);
      gftp_set_proxy_account (request, firewall_account);
      gftp_set_proxy_config (request, proxy_config);
    }
  else
    {
      gftp_set_proxy_hostname (request, http_proxy_host);
      gftp_set_proxy_port (request, http_proxy_port);
      gftp_set_proxy_username (request, http_proxy_username);
      gftp_set_proxy_password (request, http_proxy_password);


      if (request->proxy_config == NULL)
        {
          gftp_protocols[GFTP_HTTP_NUM].init (request);
          request->proxy_config = g_strdup ("ftp");
        }
    }
}


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
  request->abort_transfer = rfc959_abort_transfer;
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
  request->set_config_options = rfc959_set_config_options;
  request->url_prefix = "ftp";
  request->protocol_name = "FTP";
  request->need_hostport = 1;
  request->need_userpass = 1;
  request->use_cache = 1;
  request->use_threads = 1;
  request->always_connected = 0;
  request->protocol_data = g_malloc0 (sizeof (rfc959_parms));
  gftp_set_config_options (request);
}

