/*****************************************************************************/
/*  rfc959.c - General purpose routines for the FTP protocol (RFC 959)       */
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
#include "ftpcommon.h"

static const char cvsid[] = "$Id$";

static gftp_textcomboedt_data gftp_proxy_type[] = {
  {N_("none"), "", 0},
  {N_("SITE command"), "USER %pu\nPASS %pp\nSITE %hh\nUSER %hu\nPASS %hp\n", 0},
  {N_("user@host"), "USER %pu\nPASS %pp\nUSER %hu@%hh\nPASS %hp\n", 0},
  {N_("user@host:port"), "USER %hu@%hh:%ho\nPASS %hp\n", 0},
  {N_("AUTHENTICATE"), "USER %hu@%hh\nPASS %hp\nSITE AUTHENTICATE %pu\nSITE RESPONSE %pp\n", 0},
  {N_("user@host port"), "USER %hu@%hh %ho\nPASS %hp\n", 0},
  {N_("user@host NOAUTH"), "USER %hu@%hh\nPASS %hp\n", 0},
  {N_("HTTP Proxy"), "http", 0},
  {N_("Custom"), "", GFTP_TEXTCOMBOEDT_EDITABLE},
  {NULL, NULL, 0}
};

static gftp_config_vars config_vars[] = 
{
  {"", N_("FTP"), gftp_option_type_notebook, NULL, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},

  {"email", N_("Email address:"), 
   gftp_option_type_text, "", NULL, GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("This is the password that will be used whenever you log into a remote FTP server as anonymous"), 
   GFTP_PORT_ALL, NULL},
  {"ftp_proxy_host", N_("Proxy hostname:"), 
   gftp_option_type_text, "", NULL, 0,
   N_("Firewall hostname"), GFTP_PORT_ALL, NULL},
  {"ftp_proxy_port", N_("Proxy port:"), 
   gftp_option_type_int, GINT_TO_POINTER(21), NULL, 0,
   N_("Port to connect to on the firewall"), GFTP_PORT_ALL, NULL},
  {"ftp_proxy_username", N_("Proxy username:"), 
   gftp_option_type_text, "", NULL, 0,
   N_("Your firewall username"), GFTP_PORT_ALL, NULL},
  {"ftp_proxy_password", N_("Proxy password:"), 
   gftp_option_type_hidetext, "", NULL, 0,
   N_("Your firewall password"), GFTP_PORT_ALL, NULL},
  {"ftp_proxy_account", N_("Proxy account:"), 
   gftp_option_type_text, "", NULL, 0,
   N_("Your firewall account (optional)"), GFTP_PORT_ALL, NULL},
  
  {"proxy_config", N_("Proxy server type:"),
   gftp_option_type_textcomboedt, "", gftp_proxy_type, 0,
   /* xgettext:no-c-format */
   N_("This specifies how your proxy server expects us to log in. You can specify a 2 character replacement string prefixed by a % that will be replaced with the proper data. The first character can be either p for proxy or h for the host of the FTP server. The second character can be u (user), p (pass), h (host), o (port) or a (account). For example, to specify the proxy user, you can you type in %pu"), 
   GFTP_PORT_ALL, NULL},

  {"passive_transfer", N_("Passive file transfers"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("If this is enabled, then the remote FTP server will open up a port for the data connection. If you are behind a firewall, you will need to enable this. Generally, it is a good idea to keep this enabled unless you are connecting to an older FTP server that doesn't support this. If this is disabled, then gFTP will open up a port on the client side and the remote server will attempt to connect to it."),
   GFTP_PORT_ALL, NULL},
  {"resolve_symlinks", N_("Resolve Remote Symlinks (LIST -L)"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The remote FTP server will attempt to resolve symlinks in the directory listings. Generally, this is a good idea to leave enabled. The only time you will want to disable this is if the remote FTP server doesn't support the -L option to LIST"), 
   GFTP_PORT_ALL, NULL},
  {"ascii_transfers", N_("Transfer files in ASCII mode"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("If you are transfering a text file from Windows to UNIX box or vice versa, then you should enable this. Each system represents newlines differently for text files. If you are transfering from UNIX to UNIX, then it is safe to leave this off. If you are downloading binary data, you will want to disable this."), 
   GFTP_PORT_ALL, NULL},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};

         
static int
rfc959_read_response (gftp_request * request, int disconnect_on_42x)
{
  char tempstr[255], code[4];
  rfc959_parms * parms;
  ssize_t num_read;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  *code = '\0';
  if (request->last_ftp_response)
    {
      g_free (request->last_ftp_response);
      request->last_ftp_response = NULL;
    }

  parms = request->protocol_data;

  do
    {
      if ((num_read = gftp_get_line (request, &parms->datafd_rbuf, tempstr, 
                                     sizeof (tempstr), request->datafd)) <= 0)
	break;

      if (isdigit ((int) *tempstr) && isdigit ((int) *(tempstr + 1))
	  && isdigit ((int) *(tempstr + 2)))
	{
	  strncpy (code, tempstr, 3);
	  code[3] = ' ';
	}
      request->logging_function (gftp_logging_recv, request,
				 "%s\n", tempstr);
    }
  while (strncmp (code, tempstr, 4) != 0);

  if (num_read < 0)
    return ((int) num_read);

  request->last_ftp_response = g_strdup (tempstr);

  if (request->last_ftp_response[0] == '4' &&
      request->last_ftp_response[1] == '2' &&
      disconnect_on_42x)
    {
      gftp_disconnect (request);
      return (GFTP_ETIMEDOUT);
    }

  return (*request->last_ftp_response);
}


int
rfc959_send_command (gftp_request * request, const char *command, 
                     int read_response)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (command != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if (strncmp (command, "PASS", 4) == 0)
    {
      request->logging_function (gftp_logging_send, request, 
                                 "PASS xxxx\n");
    }
  else if (strncmp (command, "ACCT", 4) == 0)
    {
      request->logging_function (gftp_logging_send, request, 
                                 "ACCT xxxx\n");
    }
  else
    {
      request->logging_function (gftp_logging_send, request, "%s",
                                 command);
    }

  if ((ret = request->write_function (request, command, strlen (command), 
                                      request->datafd)) < 0)
    return (ret);

  if (read_response)
    return (rfc959_read_response (request, 1));
  else
    return (0);
}


static char *
parse_ftp_proxy_string (gftp_request * request)
{
  char *startpos, *endpos, *newstr, *newval, tempport[6], *proxy_config, *utf8,
       savechar;
  size_t len;
  intptr_t tmp;

  g_return_val_if_fail (request != NULL, NULL);

  gftp_lookup_request_option (request, "proxy_config", &proxy_config);

  newstr = g_malloc0 (1);
  len = 0;
  startpos = endpos = proxy_config;
  while (*endpos != '\0')
    {
      if (*endpos == '%' && tolower ((int) *(endpos + 1)) == 'p')
	{
	  switch (tolower ((int) *(endpos + 2)))
	    {
	    case 'u':
              gftp_lookup_request_option (request, "ftp_proxy_username", &newval);
	      break;
	    case 'p':
              gftp_lookup_request_option (request, "ftp_proxy_password", &newval);
	      break;
	    case 'h':
              gftp_lookup_request_option (request, "ftp_proxy_host", &newval);
	      break;
	    case 'o':
              gftp_lookup_request_option (request, "ftp_proxy_port", &tmp);
              g_snprintf (tempport, sizeof (tempport), "%d", (int)tmp);
	      newval = tempport;
	      break;
	    case 'a':
              gftp_lookup_request_option (request, "ftp_proxy_account", &newval);
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
              g_snprintf (tempport, sizeof (tempport), "%d", request->port);
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
          savechar = *endpos;
          *endpos = '\0';

          len += strlen (startpos) + 2;
          newstr = g_realloc (newstr, sizeof (char) * (len + 1));
          strcat (newstr, startpos);
          strcat (newstr, "\r\n");

          *endpos = savechar;
	  endpos += 2;
	  startpos = endpos;
	  continue;
	}
      else
	{
	  endpos++;
	  continue;
	}

      savechar = *endpos;
      *endpos = '\0';
      len += strlen (startpos);
      if (!newval)
        {
          newstr = g_realloc (newstr, sizeof (char) * (len + 1));
          strcat (newstr, startpos);
        }
      else
        {
          utf8 = gftp_string_from_utf8 (request, newval);
          if (utf8 != NULL)
            len += strlen (utf8);
          else
            len += strlen (newval);

          newstr = g_realloc (newstr, sizeof (char) * (len + 1));
          strcat (newstr, startpos);

          if (utf8 != NULL)
            {
              strcat (newstr, utf8);
              g_free (utf8);
            }
          else
            strcat (newstr, newval);
        }
   
      *endpos = savechar;
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

  ret = rfc959_send_command (request, "PWD\r\n", 1);
  if (ret < 0)
    return (ret);
  else if (ret != '2')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Invalid response '%c' received from server.\n"),
                                 ret);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if ((pos = strchr (request->last_ftp_response, '"')) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Invalid response '%c' received from server.\n"),
                                 ret);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  dir = pos + 1;

  if ((pos = strchr (dir, '"')) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Invalid response '%c' received from server.\n"),
                                 ret);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  *pos = '\0';

  if (request->directory)
    g_free (request->directory);

  request->directory = g_strdup (dir);
  return (0);
}


static int
rfc959_chdir (gftp_request * request, const char *directory)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (strcmp (directory, "..") == 0)
    ret = rfc959_send_command (request, "CDUP\r\n", 1);
  else
    {
      tempstr = g_strconcat ("CWD ", directory, "\r\n", NULL);
      ret = rfc959_send_command (request, tempstr, 1);
      g_free (tempstr);
    }

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  if (directory != request->directory)
    {
      if ((ret = rfc959_getcwd (request)) < 0)
        return (ret);
    }

  return (0);
}


static int
rfc959_syst (gftp_request * request)
{
  rfc959_parms * parms;
  char *stpos, *endpos;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;
  ret = rfc959_send_command (request, "SYST\r\n", 1);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  if ((stpos = strchr (request->last_ftp_response, ' ')) == NULL)
    return (GFTP_ERETRYABLE);

  stpos++;

  if ((endpos = strchr (stpos, ' ')) == NULL)
    return (GFTP_ERETRYABLE);

  *endpos = '\0';
  parms->quote_filename = 0;

  if (strcmp (stpos, "UNIX") == 0)
    {
      request->server_type = GFTP_DIRTYPE_UNIX;
      if (strstr (endpos + 1, "BSD") == NULL)
        parms->quote_filename = 1;
    }
  else if (strcmp (stpos, "VMS") == 0)
    request->server_type = GFTP_DIRTYPE_VMS;
  else if (strcmp (stpos, "MVS") == 0 ||
           strcmp (stpos, "OS/MVS") == 0)
    request->server_type = GFTP_DIRTYPE_MVS;
  else if (strcmp (stpos, "NETWARE") == 0)
    request->server_type = GFTP_DIRTYPE_NOVELL;
  else if (strcmp (stpos, "CRAY") == 0)
    request->server_type = GFTP_DIRTYPE_CRAY;
  else
    request->server_type = GFTP_DIRTYPE_OTHER;

  return (0);
}


int
rfc959_connect (gftp_request * request)
{
  char tempchar, *startpos, *endpos, *tempstr, *email, *proxy_hostname, *utf8;
  intptr_t ascii_transfers, proxy_port;
  rfc959_parms * parms;
  int ret, resp;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);

  if (request->datafd > 0)
    return (0);

  parms = request->protocol_data;

  gftp_lookup_request_option (request, "ftp_proxy_host", &proxy_hostname);
  gftp_lookup_request_option (request, "ftp_proxy_port", &proxy_port);

  if (request->username == NULL || *request->username == '\0')
    gftp_set_username (request, GFTP_ANONYMOUS_USER);

  if (strcasecmp (request->username, GFTP_ANONYMOUS_USER) == 0 &&
      (request->password == NULL || *request->password == '\0'))
    {
      gftp_lookup_request_option (request, "email", &email);
      gftp_set_password (request, email);
    }
   
  if ((ret = gftp_connect_server (request, "ftp", proxy_hostname, proxy_port)) < 0)
    return (ret);

  /* Get the banner */
  if ((ret = rfc959_read_response (request, 1)) != '2')
    {
      gftp_disconnect (request);
      if (ret < 0)
        return (ret);
      else
        return (GFTP_ERETRYABLE);
    }

  if (parms->auth_tls_start != NULL)
    {
      if ((ret = parms->auth_tls_start (request)) < 0)
        {
          gftp_disconnect (request);
          return (ret);
        }
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
	      if ((resp = rfc959_send_command (request, startpos, 1)) < 0)
                return (resp);
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
      resp = rfc959_send_command (request, tempstr, 1);
      g_free (tempstr);
      if (resp < 0)
        return (resp);

      if (resp == '3')
	{
          utf8 = gftp_string_from_utf8 (request, request->password);
          if (utf8 != NULL)
            {
              tempstr = g_strconcat ("PASS ", utf8, "\r\n", NULL);
              g_free (utf8);
            }
          else
            tempstr = g_strconcat ("PASS ", request->password, "\r\n", NULL);

	  resp = rfc959_send_command (request, tempstr, 1);
	  g_free (tempstr);
          if (resp < 0)
            return (resp);
        }

      if (resp == '3' && request->account != NULL)
	{
	  tempstr = g_strconcat ("ACCT ", request->account, "\r\n", NULL);
	  resp = rfc959_send_command (request, tempstr, 1);
	  g_free (tempstr);
          if (resp < 0)
            return (resp);
	}
    }

  if (resp != '2')
    {
      gftp_disconnect (request);

      if (resp == '5')
        return (GFTP_EFATAL);
      else
        return (GFTP_ERETRYABLE);
    }

  if ((ret = rfc959_syst (request)) < 0 && request->datafd < 0)
    return (ret);

  gftp_lookup_request_option (request, "ascii_transfers", &ascii_transfers);
  if (ascii_transfers)
    {
      tempstr = "TYPE A\r\n";
      parms->is_ascii_transfer = 1;
    }
  else
    {
      tempstr = "TYPE I\r\n";
      parms->is_ascii_transfer = 0;
    }

  if ((ret = rfc959_send_command (request, tempstr, 1)) < 0)
    return (ret);

  ret = -1;
  if (request->directory != NULL && *request->directory != '\0')
    {
      ret = rfc959_chdir (request, request->directory);
      if (request->datafd < 0)
        return (ret);
    }

  if (ret != 0)
    {
      if ((ret = rfc959_getcwd (request)) < 0)
        return (ret);
    }

  if (request->datafd < 0)
    return (GFTP_EFATAL);

  return (0);
}


static void
rfc959_disconnect (gftp_request * request)
{
  rfc959_parms * parms;

  g_return_if_fail (request != NULL);

  parms = request->protocol_data;

  if (request->datafd > 0)
    {
      request->logging_function (gftp_logging_misc, request,
				 _("Disconnecting from site %s\n"),
				 request->hostname);
      close (request->datafd);
      request->datafd = -1;
    }

  if (parms->data_connection > 0)
    {
      close (parms->data_connection);
      parms->data_connection = -1;
    }
}


static int
rfc959_ipv4_data_connection_new (gftp_request * request)
{
  struct sockaddr_in data_addr;
  char *pos, *pos1, *command;
  intptr_t passive_transfer;
  rfc959_parms * parms;
  socklen_t data_addr_len;
  unsigned int temp[6];
  unsigned char ad[6];
  int i, resp;

  parms = request->protocol_data;

  if ((parms->data_connection = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
				 _("Failed to create a IPv4 socket: %s\n"),
				 g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (fcntl (parms->data_connection, F_SETFD, 1) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot set close on exec flag: %s\n"),
                                 g_strerror (errno));

      return (GFTP_ERETRYABLE);
    }

  data_addr_len = sizeof (data_addr);
  memset (&data_addr, 0, data_addr_len);
  data_addr.sin_family = AF_INET;

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (passive_transfer)
    {
      resp = rfc959_send_command (request, "PASV\r\n", 1);
      if (resp < 0)
        return (resp);
      else if (resp != '2')
	{
          gftp_set_request_option (request, "passive_transfer",
                                   GINT_TO_POINTER(0));
	  return (rfc959_ipv4_data_connection_new (request));
	}

      pos = request->last_ftp_response + 4;
      while (!isdigit ((int) *pos) && *pos != '\0')
        pos++;

      if (*pos == '\0')
        {
          request->logging_function (gftp_logging_error, request,
                      _("Cannot find an IP address in PASV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      if (sscanf (pos, "%u,%u,%u,%u,%u,%u", &temp[0], &temp[1], &temp[2],
                  &temp[3], &temp[4], &temp[5]) != 6)
        {
          request->logging_function (gftp_logging_error, request,
                      _("Cannot find an IP address in PASV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      for (i = 0; i < 6; i++)
        ad[i] = (unsigned char) (temp[i] & 0xff);

      memcpy (&data_addr.sin_addr, &ad[0], 4);
      memcpy (&data_addr.sin_port, &ad[4], 2);
      if (connect (parms->data_connection, (struct sockaddr *) &data_addr, 
                   data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                    _("Cannot create a data connection: %s\n"),
                                    g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
	}
    }
  else
    {
      if (getsockname (request->datafd, (struct sockaddr *) &data_addr,
                       &data_addr_len) == -1)
        {
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot get socket name: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
        }

      data_addr.sin_port = 0;
      if (bind (parms->data_connection, (struct sockaddr *) &data_addr, 
                data_addr_len) == -1)
	{
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot bind a port: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}

      if (getsockname (parms->data_connection, (struct sockaddr *) &data_addr, 
                       &data_addr_len) == -1)
        {
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot get socket name: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
        }

      if (listen (parms->data_connection, 1) == -1)
	{
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot listen on port %d: %s\n"),
				     ntohs (data_addr.sin_port),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}

      pos = (char *) &data_addr.sin_addr;
      pos1 = (char *) &data_addr.sin_port;
      command = g_strdup_printf ("PORT %u,%u,%u,%u,%u,%u\r\n",
				 pos[0] & 0xff, pos[1] & 0xff, pos[2] & 0xff,
				 pos[3] & 0xff, pos1[0] & 0xff,
				 pos1[1] & 0xff);
      resp = rfc959_send_command (request, command, 1);
      g_free (command);

      if (resp < 0)
        return (resp);
      else if (resp != '2')
	{
          request->logging_function (gftp_logging_error, request,
                                     _("Invalid response '%c' received from server.\n"),
                                     resp);
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}
    }

  return (0);
}


#ifdef HAVE_IPV6

static int
rfc959_ipv6_data_connection_new (gftp_request * request)
{
  struct sockaddr_in6 data_addr;
  char *pos, buf[64], *command;
  intptr_t passive_transfer;
  socklen_t data_addr_len;
  rfc959_parms * parms;
  unsigned int port;
  int resp;

  parms = request->protocol_data;
  if ((parms->data_connection = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
				 _("Failed to create a IPv6 socket: %s\n"),
				 g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (fcntl (parms->data_connection, F_SETFD, 1) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot set close on exec flag: %s\n"),
                                 g_strerror (errno));

      return (GFTP_ERETRYABLE);
    }

  data_addr_len = sizeof (data_addr);
  /* This condition shouldn't happen. We better check anyway... */
  if (data_addr_len != request->current_hostp->ai_addrlen) 
    {
      request->logging_function (gftp_logging_error, request,
				 _("Error: It doesn't look like we are connected via IPv6. Aborting connection.\n"));
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  memset (&data_addr, 0, data_addr_len);
  data_addr.sin6_family = AF_INET6;

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (passive_transfer)
    {
      resp = rfc959_send_command (request, "EPSV\r\n", 1);
      if (resp < 0)
        return (resp);
      else if (resp != '2')
	{
          gftp_set_request_option (request, "passive_transfer", 
                                   GINT_TO_POINTER(0));
	  return (rfc959_ipv6_data_connection_new (request));
	}

      pos = request->last_ftp_response + 4;
      while (*pos != '(' && *pos != '\0')
        pos++;

      if (*pos == '\0' || *(pos + 1) == '\0')
        {
          request->logging_function (gftp_logging_error, request,
                      _("Invalid EPSV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      if (sscanf (pos + 1, "|||%u|", &port) != 1)
        {
          request->logging_function (gftp_logging_error, request,
                      _("Invalid EPSV response '%s'\n"),
                      request->last_ftp_response);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      memcpy (&data_addr, request->current_hostp->ai_addr, data_addr_len);
      data_addr.sin6_port = htons (port);

      if (connect (parms->data_connection, (struct sockaddr *) &data_addr, 
                   data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                    _("Cannot create a data connection: %s\n"),
                                    g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
	}
    }
  else
    {
      memcpy (&data_addr, request->current_hostp->ai_addr, data_addr_len);
      data_addr.sin6_port = 0;

      if (bind (parms->data_connection, (struct sockaddr *) &data_addr, 
                data_addr_len) == -1)
	{
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot bind a port: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}

      if (getsockname (parms->data_connection, (struct sockaddr *) &data_addr, 
                       &data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
				     _("Cannot get socket name: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
        }

      if (listen (parms->data_connection, 1) == -1)
	{
	  request->logging_function (gftp_logging_error, request,
				     _("Cannot listen on port %d: %s\n"),
				     ntohs (data_addr.sin6_port),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}

      if (inet_ntop (AF_INET6, &data_addr.sin6_addr, buf, sizeof (buf)) == NULL)
        {
          request->logging_function (gftp_logging_error, request,
				     _("Cannot get address of local socket: %s\n"),
				     g_strerror (errno));
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
        }

      command = g_strdup_printf ("EPRT |2|%s|%d|\n", buf,
                                 ntohs (data_addr.sin6_port));

      resp = rfc959_send_command (request, command, 1);
      g_free (command);

      if (resp < 0)
        return (resp);
      else if (resp != '2')
	{
          gftp_disconnect (request);
	  return (GFTP_ERETRYABLE);
	}
    }

  return (0);
}

#endif /* HAVE_IPV6 */


static int
rfc959_data_connection_new (gftp_request * request)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

#ifdef HAVE_IPV6
  if (request->ai_family == AF_INET6)
    return (rfc959_ipv6_data_connection_new (request));
  else
    return (rfc959_ipv4_data_connection_new (request));
#else
  return (rfc959_ipv4_data_connection_new (request));
#endif
}


static int
rfc959_accept_active_connection (gftp_request * request)
{
  int infd, ret;
  intptr_t passive_transfer;
  rfc959_parms * parms;
#ifdef HAVE_IPV6
  struct sockaddr_in6 cli_addr;
#else
  struct sockaddr_in cli_addr;
#endif
  socklen_t cli_addr_len;

  parms = request->protocol_data;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (parms->data_connection > 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  g_return_val_if_fail (!passive_transfer, GFTP_EFATAL);

  cli_addr_len = sizeof (cli_addr);

  if ((ret = gftp_fd_set_sockblocking (request, parms->data_connection, 0)) < 0)
    return (ret);

  if ((infd = accept (parms->data_connection, (struct sockaddr *) &cli_addr,
       &cli_addr_len)) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                _("Cannot accept connection from server: %s\n"),
                                g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  close (parms->data_connection);

  parms->data_connection = infd;
  if ((ret = gftp_fd_set_sockblocking (request, parms->data_connection, 1)) < 0)
    return (ret);

  return (0);
}


static int
rfc959_is_ascii_transfer (gftp_request * request, const char *filename)
{
  gftp_config_list_vars * tmplistvar;
  gftp_file_extensions * tempext;
  intptr_t ascii_transfers;
  GList * templist;
  size_t stlen;
  
  gftp_lookup_global_option ("ext", &tmplistvar);
  gftp_lookup_request_option (request, "ascii_transfers", &ascii_transfers);

  stlen = strlen (filename);
  for (templist = tmplistvar->list; templist != NULL; templist = templist->next)
    {
      tempext = templist->data;

      if (stlen >= tempext->stlen &&
          strcmp (&filename[stlen - tempext->stlen], tempext->ext) == 0)
        {
          if (toupper (*tempext->ascii_binary == 'A'))
            ascii_transfers = 1; 
          else if (toupper (*tempext->ascii_binary == 'B'))
            ascii_transfers = 0; 
          break;
        }
    }

  return (ascii_transfers);
}


static int
rfc959_set_data_type (gftp_request * request, const char *filename)
{
  rfc959_parms * parms;
  int new_ascii, ret;
  char *tempstr;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  parms = request->protocol_data;
  new_ascii = rfc959_is_ascii_transfer (request, filename);

  if (request->datafd > 0 && new_ascii != parms->is_ascii_transfer)
    {
      if (new_ascii)
        {
	  tempstr = "TYPE A\r\n";
          parms->is_ascii_transfer = 1;
        }
      else
        {
	  tempstr = "TYPE I\r\n";
          parms->is_ascii_transfer = 0;
        }

      if ((ret = rfc959_send_command (request, tempstr, 1)) < 0)
        return (ret);
    }

  return (0);
}


static off_t
rfc959_get_file (gftp_request * request, const char *filename, int fd,
                 off_t startsize)
{
  char *command, *tempstr;
  intptr_t passive_transfer;
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;
  if (fd > 0)
    parms->data_connection = fd;

  if ((ret = rfc959_set_data_type (request, filename)) < 0)
    return (ret);

  if (parms->data_connection < 0 && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  if ((ret = gftp_fd_set_sockblocking (request, parms->data_connection, 1)) < 0)
    return (ret);

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST " GFTP_OFF_T_PRINTF_MOD "\r\n",
                                 startsize);
      ret = rfc959_send_command (request, command, 1);
      g_free (command);

      if (ret < 0)
        return (ret);
      else if (ret != '3')
        {
          close (parms->data_connection);
          parms->data_connection = -1;
	  return (GFTP_ERETRYABLE);
        }
    }

  tempstr = g_strconcat ("RETR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '1')
    {
      close (parms->data_connection);
      parms->data_connection = -1;
      return (GFTP_ERETRYABLE);
    }

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (!passive_transfer &&
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

  return (gftp_parse_file_size (tempstr) + startsize);
}


static int
rfc959_put_file (gftp_request * request, const char *filename, int fd,
                 off_t startsize, off_t totalsize)
{
  intptr_t passive_transfer;
  char *command, *tempstr;
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;
  if (fd > 0)
    fd = parms->data_connection;

  if ((ret = rfc959_set_data_type (request, filename)) < 0)
    return (ret);

  if (parms->data_connection < 0 && 
      (ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  if ((ret = gftp_fd_set_sockblocking (request, parms->data_connection, 1)) < 0)
    return (ret);

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST " GFTP_OFF_T_PRINTF_MOD "\r\n",
                                 startsize);
      ret = rfc959_send_command (request, command, 1);
      g_free (command);
      if (ret < 0)
        return (ret);
      else if (ret != '3')
        {
          close (parms->data_connection);
          parms->data_connection = -1;
	  return (GFTP_ERETRYABLE);
        }
    }

  tempstr = g_strconcat ("STOR ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);
  if (ret < 0)
    return (ret);
  else if (ret != '1')
    {
      close (parms->data_connection);
      parms->data_connection = -1;
      return (GFTP_ERETRYABLE);
    }

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (!passive_transfer &&
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
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromfile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (toreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (tofile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromreq->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (toreq->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_send_command (fromreq, "PASV\r\n", 1)) < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  pos = fromreq->last_ftp_response + 4;
  while (!isdigit ((int) *pos) && *pos != '\0') 
    pos++;
  if (*pos == '\0') 
    return (GFTP_EFATAL);

  endpos = pos;
  while (*endpos != ')' && *endpos != '\0') 
    endpos++;
  if (*endpos == ')') 
    *endpos = '\0';

  tempstr = g_strconcat ("PORT ", pos, "\r\n", NULL);
  ret = rfc959_send_command (toreq, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  tempstr = g_strconcat ("RETR ", fromfile, "\r\n", NULL);
  ret = rfc959_send_command (fromreq, tempstr, 0);
  g_free (tempstr);

  if (ret < 0)
    return (ret);

  tempstr = g_strconcat ("STOR ", tofile, "\r\n", NULL);
  ret = rfc959_send_command (toreq, tempstr, 0);
  g_free (tempstr);

  if (ret < 0)
    return (ret);

  if ((ret = rfc959_read_response (fromreq, 1)) < 0)
    return (ret);

  if ((ret = rfc959_read_response (toreq, 1)) < 0)
    return (ret);

  parms = fromreq->protocol_data;
  parms->is_fxp_transfer = 1;

  parms = toreq->protocol_data;
  parms->is_fxp_transfer = 1;

  return (0);
}


static int
rfc959_end_transfer (gftp_request * request)
{
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;
  parms->is_fxp_transfer = 0;

  if (parms->data_connection > 0)
    {
      close (parms->data_connection);
      parms->data_connection = -1;
    }

  ret = rfc959_read_response (request, 1);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_abort_transfer (gftp_request * request)
{
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;

  if ((ret = rfc959_send_command (request, "ABOR\r\n", 0)) < 0)
    return (ret);

  if (parms->data_connection > 0)
    {
      close (parms->data_connection);
      parms->data_connection = -1;
    }

  if (request->datafd > 0)
    {
      if ((ret = rfc959_read_response (request, 0)) < 0)
        gftp_disconnect (request);
    }

  return (0);
}


static int
rfc959_list_files (gftp_request * request)
{
  intptr_t show_hidden_files, resolve_symlinks, passive_transfer;
  char *tempstr, parms[3];
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_data_connection_new (request)) < 0)
    return (ret);

  gftp_lookup_request_option (request, "show_hidden_files", &show_hidden_files);
  gftp_lookup_request_option (request, "resolve_symlinks", &resolve_symlinks);
  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  *parms = '\0';
  strcat (parms, show_hidden_files ? "a" : "");
  strcat (parms, resolve_symlinks ? "L" : "");
  tempstr = g_strconcat ("LIST", *parms != '\0' ? " -" : "", parms, "\r\n", 
                         NULL); 

  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '1')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Invalid response '%c' received from server.\n"),
                                 ret);
      return (GFTP_ERETRYABLE);
    }

  ret = 0;
  if (!passive_transfer)
    ret = rfc959_accept_active_connection (request);

  return (ret);
}


static ssize_t
rfc959_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  ssize_t num_read, ret;
  rfc959_parms * parms;
  int i, j;

  parms = request->protocol_data;
  if (parms->is_fxp_transfer)
    return (GFTP_ENOTRANS);

  num_read = parms->data_conn_read (request, buf, size, parms->data_connection);
  if (num_read < 0)
    return (num_read);

  ret = num_read;
  if (parms->is_ascii_transfer)
    {
      for (i = 0, j = 0; i < num_read; i++)
        {
          if (buf[i] != '\r')
            buf[j++] = buf[i];
          else
            ret--;
        }
    }

  return (ret);
}


static ssize_t
rfc959_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  rfc959_parms * parms;
  size_t rsize, i, j;
  ssize_t num_wrote;
  char *tempstr;

  if (size == 0)
    return (0);

  parms = request->protocol_data;

  if (parms->is_fxp_transfer)
    return (GFTP_ENOTRANS);

  if (parms->is_ascii_transfer)
    {
      rsize = 0;
      for (i = 0; i < size; i++)
        {
          rsize++;
          if (i > 0 && buf[i] == '\n' && buf[i - 1] != '\r')
            rsize++;
        }

      if (rsize != size)
        {
          tempstr = g_malloc (rsize);

          for (i = 0, j = 0; i < size; i++)
            {
              if (i > 0 && buf[i] == '\n' && buf[i - 1] != '\r')
                tempstr[j++] = '\r';
              tempstr[j++] = buf[i];
            }
        }
      else
        tempstr = buf;
    }
  else
    {
      rsize = size;
      tempstr = buf;
    }

  num_wrote = parms->data_conn_write (request, tempstr, rsize,
                                      parms->data_connection);

  if (tempstr != buf)
    g_free (tempstr);

  return (num_wrote);
}


static ssize_t
rfc959_get_next_dirlist_line (gftp_request * request, int fd,
                              char *buf, size_t buflen)
{
  ssize_t (*oldread_func) (gftp_request * request, void *ptr, size_t size,
                           int fd);
  rfc959_parms * parms;
  ssize_t len;

  parms = request->protocol_data;

  oldread_func = request->read_function;
  request->read_function = parms->data_conn_read;
  len = gftp_get_line (request, &parms->dataconn_rbuf, buf, buflen, fd);
  request->read_function = oldread_func;

  return (len);
}

int
rfc959_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc959_parms * parms;
  char tempstr[1024];
  size_t stlen;
  ssize_t len;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fd > 0, GFTP_EFATAL);

  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
    }

  parms = request->protocol_data;

  if (fd == request->datafd)
    fd = parms->data_connection;

  do
    {
      len = rfc959_get_next_dirlist_line (request, fd, tempstr,
                                          sizeof (tempstr));
      if (len <= 0)
	{
          gftp_file_destroy (fle);
	  return (len);
	} 

      if (gftp_parse_ls (request, tempstr, fle, fd) != 0)
	{
	  if (strncmp (tempstr, "total", strlen ("total")) != 0 &&
	      strncmp (tempstr, _("total"), strlen (_("total"))) != 0)
	    request->logging_function (gftp_logging_error, request,
				       _("Warning: Cannot parse listing %s\n"),
				       tempstr);
	  gftp_file_destroy (fle);
	  continue;
	}
      else
	break;
    }
  while (1);

  stlen = strlen (tempstr);
  if (!request->cached)
    {
      request->last_dir_entry = g_strdup_printf ("%s\n", tempstr);
      request->last_dir_entry_len = stlen + 1;
    }
  return (len);
}


static off_t
rfc959_get_file_size (gftp_request * request, const char *filename)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, 0);
  g_return_val_if_fail (filename != NULL, 0);
  g_return_val_if_fail (request->datafd > 0, 0);

  tempstr = g_strconcat ("SIZE ", filename, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);
  if (ret < 0)
    return (ret);

  if (*request->last_ftp_response != '2')
    return (0);
  return (strtol (request->last_ftp_response + 4, NULL, 10));
}


static int
rfc959_rmdir (gftp_request * request, const char *directory)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  tempstr = g_strconcat ("RMD ", directory, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_rmfile (gftp_request * request, const char *file)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  tempstr = g_strconcat ("DELE ", file, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_mkdir (gftp_request * request, const char *directory)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  tempstr = g_strconcat ("MKD ", directory, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_rename (gftp_request * request, const char *oldname,
	       const char *newname)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  tempstr = g_strconcat ("RNFR ", oldname, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '3')
    return (GFTP_ERETRYABLE);

  tempstr = g_strconcat ("RNTO ", newname, "\r\n", NULL);
  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_chmod (gftp_request * request, const char *file, mode_t mode)
{
  rfc959_parms * parms;
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;
  if (parms->quote_filename)
    tempstr = g_strdup_printf ("SITE CHMOD %o \"%s\"\r\n", mode, file);
  else
    tempstr = g_strdup_printf ("SITE CHMOD %o %s\r\n", mode, file);

  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_site (gftp_request * request, int specify_site, const char *command)
{
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (command != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if (specify_site)
    tempstr = g_strconcat ("SITE ", command, "\r\n", NULL);
  else
    tempstr = g_strconcat (command, "\r\n", NULL);

  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static char *
rfc959_time_t_to_mdtm (gftp_request * request, time_t datetime)
{
  struct tm gt;
  char *ret;

  if (localtime_r (&datetime, &gt) != NULL)
    {
      ret = g_strdup_printf ("%04d%02d%02d%02d%02d%02d", gt.tm_year + 1900,
                             gt.tm_mon + 1, gt.tm_mday, gt.tm_hour, gt.tm_min,
                             gt.tm_sec);
      return (ret);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 "Cannot parse UNIX timestamp %d: %s\n",
                                 datetime, g_strerror (errno));
      return (NULL);
    }
}


static int
rfc959_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  char *tempstr, *datestr;
  rfc959_parms * parms;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  parms = request->protocol_data;

  datestr = rfc959_time_t_to_mdtm (request, datetime);
  if (datestr == NULL)
    return (GFTP_EFATAL);

  if (parms->quote_filename)
    tempstr = g_strconcat ("SITE UTIME ", datestr, " \"", file, "\"\r\n", NULL);
  else
    tempstr = g_strconcat ("SITE UTIME ", datestr, " ", file, "\r\n", NULL);

  g_free (datestr);

  ret = rfc959_send_command (request, tempstr, 1);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int
rfc959_set_config_options (gftp_request * request)
{
  char *proxy_config;
  int ret;

  gftp_lookup_request_option (request, "proxy_config", &proxy_config);
  if (strcmp (proxy_config, "http") == 0)
    {
      if ((ret = gftp_protocols[GFTP_HTTP_NUM].init (request)) < 0)
        return (ret);

      gftp_set_request_option (request, "proxy_config", "ftp");
    }

  return (0);
}


void 
rfc959_register_module (void)
{
  gftp_register_config_vars (config_vars);
}


static void
rfc959_request_destroy (gftp_request * request)
{
  rfc959_parms * parms;

  parms = request->protocol_data;

  if (parms->datafd_rbuf != NULL)
    gftp_free_getline_buffer (&parms->datafd_rbuf);

  if (parms->dataconn_rbuf != NULL)
    gftp_free_getline_buffer (&parms->dataconn_rbuf);
}


static void
rfc959_copy_param_options (gftp_request * dest_request,
                           gftp_request * src_request)
{
  rfc959_parms * dparms, * sparms;

  dparms = dest_request->protocol_data;
  sparms = src_request->protocol_data;

  dparms->data_connection = -1;
  dparms->quote_filename = sparms->quote_filename;
  dparms->is_ascii_transfer = sparms->is_ascii_transfer;
  dparms->is_fxp_transfer = sparms->is_fxp_transfer;
  dparms->auth_tls_start = sparms->auth_tls_start;
  dparms->data_conn_read = sparms->data_conn_read;
  dparms->data_conn_write = sparms->data_conn_write;

  dest_request->read_function = src_request->read_function;
  dest_request->write_function = src_request->write_function;
}


int
rfc959_init (gftp_request * request)
{
  rfc959_parms * parms;
  struct hostent *hent;
  struct utsname unme;
  struct passwd *pw;
  char *tempstr;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  gftp_lookup_global_option ("email", &tempstr);
  if (tempstr == NULL || *tempstr == '\0')
    {
      /* If there is no email address specified, then we'll just use the
         currentuser@currenthost */
      uname (&unme);
      pw = getpwuid (geteuid ());
      hent = gethostbyname (unme.nodename);
      if (strchr (unme.nodename, '.') == NULL && hent != NULL)
        tempstr = g_strconcat (pw->pw_name, "@", hent->h_name, NULL);
      else
        tempstr = g_strconcat (pw->pw_name, "@", unme.nodename, NULL);
      gftp_set_global_option ("email", tempstr);
      g_free (tempstr);
    }

  request->protonum = GFTP_FTP_NUM;
  request->init = rfc959_init;
  request->copy_param_options = rfc959_copy_param_options;
  request->destroy = rfc959_request_destroy; 
  request->read_function = gftp_fd_read;
  request->write_function = gftp_fd_write;
  request->connect = rfc959_connect;
  request->post_connect = NULL;
  request->disconnect = rfc959_disconnect;
  request->get_file = rfc959_get_file;
  request->put_file = rfc959_put_file;
  request->transfer_file = rfc959_transfer_file;
  request->get_next_file_chunk = rfc959_get_next_file_chunk;
  request->put_next_file_chunk = rfc959_put_next_file_chunk;
  request->end_transfer = rfc959_end_transfer;
  request->abort_transfer = rfc959_abort_transfer;
  request->stat_filename = NULL;
  request->list_files = rfc959_list_files;
  request->get_next_file = rfc959_get_next_file;
  request->get_next_dirlist_line = rfc959_get_next_dirlist_line;
  request->get_file_size = rfc959_get_file_size;
  request->chdir = rfc959_chdir;
  request->rmdir = rfc959_rmdir;
  request->rmfile = rfc959_rmfile;
  request->mkdir = rfc959_mkdir;
  request->rename = rfc959_rename;
  request->chmod = rfc959_chmod;
  request->set_file_time = rfc959_set_file_time;
  request->site = rfc959_site;
  request->parse_url = NULL;
  request->swap_socks = NULL;
  request->set_config_options = rfc959_set_config_options;
  request->url_prefix = "ftp";
  request->need_hostport = 1;
  request->need_username = 0;
  request->need_password = 1;
  request->use_cache = 1;
  request->always_connected = 0;

  request->protocol_data = g_malloc0 (sizeof (rfc959_parms));
  parms = request->protocol_data;
  parms->data_connection = -1; 
  parms->auth_tls_start = NULL;
  parms->data_conn_read = gftp_fd_read;
  parms->data_conn_write = gftp_fd_write;

  return (gftp_set_config_options (request));
}

