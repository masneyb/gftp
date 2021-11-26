/*****************************************************************************/
/*  rfc959.c - General purpose routines for the FTP protocol (RFC 959)       */
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
#include "protocol_ftp.h"

#define FTP_CMD_FEAT 100

static gftp_textcomboedt_data gftp_proxy_type[] = {
  {N_("none"), "", 0},
  {N_("SITE command"), "USER %pu\nPASS %pp\nSITE %hh\nUSER %hu\nPASS %hp\n", 0},
  {N_("user@host"), "USER %pu\nPASS %pp\nUSER %hu@%hh\nPASS %hp\n", 0},
  {N_("user@host:port"), "USER %hu@%hh:%ho\nPASS %hp\n", 0},
  {N_("AUTHENTICATE"), "USER %hu@%hh\nPASS %hp\nSITE AUTHENTICATE %pu\nSITE RESPONSE %pp\n", 0},
  {N_("user@host port"), "USER %hu@%hh %ho\nPASS %hp\n", 0},
  {N_("user@host NOAUTH"), "USER %hu@%hh\nPASS %hp\n", 0},
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

  {"ftp_use_proxy", N_("Use FTP proxy server"),
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL,
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("You must specify a FTP proxy hostname, port and probably other details"),
   GFTP_PORT_ALL, NULL},
  {"ignore_pasv_address", N_("Ignore PASV address"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("If this is enabled, then the remote FTP server's PASV IP address field will be ignored and the host's IP address will be used instead. This is often needed for routers giving their internal rather then their external IP address in a PASV reply."),
   GFTP_PORT_ALL, NULL},
  {"passive_transfer", N_("Passive file transfers"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("If this is enabled, then the remote FTP server will open up a port for the data connection. If you are behind a firewall, you will need to enable this. Generally, it is a good idea to keep this enabled unless you are connecting to an older FTP server that doesn't support this. If this is disabled, then gFTP will open up a port on the client side and the remote server will attempt to connect to it."),
   GFTP_PORT_ALL, NULL},
  {"ascii_transfers", N_("Transfer files in ASCII mode"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("If you are transferring a text file from Windows to UNIX box or vice versa, then you should enable this. Each system represents newlines differently for text files. If you are transferring from UNIX to UNIX, then it is safe to leave this off. If you are downloading binary data, you will want to disable this."), 
   GFTP_PORT_ALL, NULL},
   {"pretransfer_command", N_("PRET before transfer"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("With this option checked, gFTP will try to tell the server what it intends to do before a transfer actually happens. By way of the PRET command, an extension for distributed FTP servers, you will be able to connect to a master server and receive files from a slave node, possibly avoiding bandwidth problems."),
   GFTP_PORT_ALL, NULL},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};


// ==========================================================================

static void rfc2389_feat_supported_cmd (ftp_protocol_data * ftpdat, char * cmd)
{
   DEBUG_PUTS(cmd)
   char *p = cmd;
   while (*p <= 32) p++; // strip leading spaces
   if (strncmp (p, "MLST", 4) == 0) {
      // If a server supports MLST, it must also support MLSD
      // some (most?) servers don't report MLSD even though it's supported
      // Should only use MLST to test if MLSD is supported
      ftpdat->use_mlsd_cmd = 1; /* RFC 3659 */
      return;
   }
}
         
static int
rfc959_read_response (gftp_request * request, int disconnect_on_42x)
{
  char tempstr[255], code[4];
  ftp_protocol_data * ftpdat;
  ssize_t num_read;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  *code = '\0';
  if (request->last_ftp_response)
    {
      g_free (request->last_ftp_response);
      request->last_ftp_response = NULL;
    }

  ftpdat = request->protocol_data;

  do
  {
      if ((num_read = gftp_get_line (request, &ftpdat->datafd_rbuf, tempstr, 
                                     sizeof (tempstr), request->datafd)) <= 0) {
         break;
      }

      if (isdigit ((int) *tempstr) && isdigit ((int) *(tempstr + 1))
          && isdigit ((int) *(tempstr + 2)))
      {
         strncpy (code, tempstr, 3);
         code[3] = ' ';
      }

      if (ftpdat->last_cmd == FTP_CMD_FEAT) {
         rfc2389_feat_supported_cmd (ftpdat, tempstr);
         continue;
      }

      if (*tempstr == '4' || *tempstr == '5')
        request->logging_function (gftp_logging_error, request,
                                   "%s\n", tempstr);
      else
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
                     ssize_t command_len, int read_response,
                     int dont_try_to_reconnect)
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

  if (command_len == -1)
    command_len = strlen (command);

  if ((ret = request->write_function (request, command, command_len, 
                                      request->datafd)) < 0)
    return (ret);

  if (read_response)
    {
      ret = rfc959_read_response (request, 1);
      if (ret == GFTP_ETIMEDOUT && !dont_try_to_reconnect)
        {
          ret = gftp_connect (request);
          if (ret < 0)
            return (ret);

          return (rfc959_send_command (request, command, command_len, 1, 1));
        }
      else
        return (ret);
    }
  else
    return (0);
}

static int
rfc959_generate_and_send_command (gftp_request * request, const char *command,
                                  const char *argument, int read_response,
                                  int dont_try_to_reconnect)
{
  char *tempstr, *utf8;
  size_t len;
  int resp;

  if (argument != NULL)
    {
      utf8 = gftp_filename_from_utf8 (request, argument, &len);
      if (utf8 != NULL)
        {
          tempstr = g_strconcat (command, " ", utf8, "\r\n", NULL);
          g_free (utf8);
        }
      else
        {
          tempstr = g_strconcat (command, " ", argument, "\r\n", NULL);
          len = strlen (argument);
        }

      len += strlen (command) + 3;
    }
  else
    {
      tempstr = g_strconcat (command, "\r\n", NULL);
      len = strlen (command) + 2;
    }

  resp = rfc959_send_command (request, tempstr, len, read_response,
                              dont_try_to_reconnect);
  g_free (tempstr);
  return (resp);
}


static char *
parse_ftp_proxy_string (gftp_request * request)
{
  char *startpos, *endpos, *newstr, *newval, tempport[6], *proxy_config, *utf8,
       savechar;
  size_t len, destlen;
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
              g_snprintf (tempport, sizeof (tempport), "%d",
                          request->port ? request->port : 21);
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
          newstr = g_realloc (newstr, (gulong) sizeof (char) * (len + 1));
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
          newstr = g_realloc (newstr, (gulong) sizeof (char) * (len + 1));
          strcat (newstr, startpos);
        }
      else
        {
          utf8 = gftp_string_from_utf8 (request, -1, newval, &destlen);
          if (utf8 != NULL)
            len += strlen (utf8);
          else
            len += strlen (newval);

          newstr = g_realloc (newstr, (gulong) sizeof (char) * (len + 1));
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
  char *pos, *dir, *utf8;
  size_t destlen;
  int ret;

  ret = rfc959_send_command (request, "PWD\r\n", -1, 1, 0);
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

  utf8 = gftp_filename_to_utf8 (request, dir, &destlen);
  if (utf8 != NULL)
    request->directory = utf8;
  else
    request->directory = g_strdup (dir);

  return (0);
}


static int
rfc959_chdir (gftp_request * request, const char *directory)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (strcmp (directory, "..") == 0)
    ret = rfc959_send_command (request, "CDUP\r\n", -1, 1, 0);
  else
    ret = rfc959_generate_and_send_command (request, "CWD", directory, 1, 0);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  if ((ret = rfc959_getcwd (request)) < 0)
    return (ret);

  return (0);
}


static int rfc959_syst (gftp_request * request)
{
  int ret;
  char *stpos, *endpos;
  ftp_protocol_data * ftpdat;
  int dt;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  // if MLSD has been detected, then use GFTP_DIRTYPE_MLSD
  ftpdat = request->protocol_data;
  if (ftpdat->use_mlsd_cmd) {
     request->server_type = GFTP_DIRTYPE_MLSD;
     return 0;
  }

  ret = rfc959_send_command (request, "SYST\r\n", -1, 1, 0);

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

  if      (strcmp (stpos, "UNIX") == 0)    dt = GFTP_DIRTYPE_UNIX;
  else if (strcmp (stpos, "VMS") == 0)     dt = GFTP_DIRTYPE_VMS;
  else if (strcmp (stpos, "MVS") == 0)     dt = GFTP_DIRTYPE_MVS;
  else if (strcmp (stpos, "OS/MVS") == 0)  dt = GFTP_DIRTYPE_MVS;
  else if (strcmp (stpos, "NETWARE") == 0) dt = GFTP_DIRTYPE_NOVELL;
  else if (strcmp (stpos, "CRAY") == 0)    dt = GFTP_DIRTYPE_CRAY;
  else                                     dt = GFTP_DIRTYPE_OTHER;

  request->server_type = dt;
  return (0);
}


int rfc959_connect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  char tempchar, *startpos, *endpos, *tempstr, *email, *proxy_hostname;
  intptr_t ascii_transfers, proxy_port;
  ftp_protocol_data * ftpdat;
  int ret, resp;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);

  if (request->datafd > 0)
    return (0);

  ftpdat = request->protocol_data;

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
   
  if ((ret = gftp_connect_server (request, "ftp", proxy_hostname,
                                  proxy_port)) < 0)
    return (ret);

  if (ftpdat->auth_tls_start && ftpdat->implicit_ssl)
  {
     // implicit SSL
     // need to setup SSL session here, otherwise it will not work
     ret = gftp_ssl_session_setup (request);
     if (ret < 0) {
        return (ret);
     }
  }

  /* Get the banner */
  if ((ret = rfc959_read_response (request, 1)) != '2')
    {
      gftp_disconnect (request);
      if (ret < 0)
        return (ret);
      else
        return (GFTP_ERETRYABLE);
    }

  if (ftpdat->auth_tls_start)
  {
     if (ftpdat->implicit_ssl) {
        // buggy servers might require this (vsftpd)
        // this is not need for servers with proper implicit SSL support
        //  (pure-FTPd, Rebex FTPS)
        ret = rfc959_send_command (request, "PBSZ 0\r\n", -1, 1, 0);
        ret = rfc959_send_command (request, "PROT P\r\n", -1, 1, 0);
     } else {
        // explicit SSL
        // this is where the negotiation to switch to a secure mode (SSL) starts
        ret = ftpdat->auth_tls_start (request);
        if (ret < 0) {
           gftp_disconnect (request);
           return (ret);
        }
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
            if ((resp = rfc959_send_command (request, startpos, -1, 1, 0)) < 0)
                return (resp);
            if (*endpos != '\0') {
               *(endpos + 1) = tempchar;
            } else {
               break;
            }
            startpos = endpos + 1;
         }
         endpos++;
      }
      g_free (tempstr);
    }
  else
    {
      resp = rfc959_generate_and_send_command (request, "USER",
                                               request->username, 1, 0);
      if (resp < 0)
        return (resp);

      if (resp == '3')
        {
          resp = rfc959_generate_and_send_command (request, "PASS",
                                                   request->password, 1, 0);
          if (resp < 0)
            return (resp);
        }

      if (resp == '3' && request->account != NULL)
        {
          resp = rfc959_generate_and_send_command (request, "ACCT",
                                                   request->account, 1, 0);
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

  // determine server features, the response to this is not logged
  ftpdat->last_cmd = FTP_CMD_FEAT;
  ret = rfc959_send_command (request, "FEAT\r\n", -1, 1, 0);
  ftpdat->last_cmd = 0;

  // determine system type
  if ((ret = rfc959_syst (request)) < 0 && request->datafd < 0) {
    return (ret);
  }

  gftp_lookup_request_option (request, "ascii_transfers", &ascii_transfers);
  if (ascii_transfers)
    {
      tempstr = "TYPE A\r\n";
      ftpdat->is_ascii_transfer = 1;
    }
  else
    {
      tempstr = "TYPE I\r\n";
      ftpdat->is_ascii_transfer = 0;
    }

  if ((ret = rfc959_send_command (request, tempstr, -1, 1, 0)) < 0)
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
rfc959_close_data_connection (gftp_request * request)
{
  ftp_protocol_data * ftpdat;

  g_return_if_fail (request != NULL);

  ftpdat = request->protocol_data;
  if (ftpdat->data_connection != -1)
    {
      if (ftpdat->data_conn_tls_close != NULL) {
         ftpdat->data_conn_tls_close(request);
      }
      close (ftpdat->data_connection);
      ftpdat->data_connection = -1;
    }
}


static void
rfc959_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);

  if (request->datafd > 0)
    {
      rfc959_close_data_connection (request);

      request->logging_function (gftp_logging_misc, request,
				 _("Disconnecting from site %s\n"),
				 request->hostname);
#ifdef USE_SSL
      gftp_ssl_session_close (request);
#endif
      close (request->datafd);
      request->datafd = -1;
    }
}


static int
rfc959_ipv4_data_connection_new (gftp_request * request)
{
  struct sockaddr_in data_addr;
  intptr_t ignore_pasv_address;
  char *pos, *pos1, *command;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  socklen_t data_addr_len;
  unsigned int temp[6];
  unsigned char ad[6];
  int i, resp;

  ftpdat = request->protocol_data;

  ftpdat->data_connection = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ftpdat->data_connection == -1)
    {
      request->logging_function (gftp_logging_error, request,
                   _("Failed to create a IPv4 socket: %s\n"),
                   g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (fcntl (ftpdat->data_connection, F_SETFD, 1) == -1)
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
      resp = rfc959_send_command (request, "PASV\r\n", -1, 1, 1);
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

      memcpy (&data_addr.sin_port, &ad[4], 2);

      gftp_lookup_request_option (request, "ignore_pasv_address",
                                  &ignore_pasv_address);
      if (ignore_pasv_address)
        {
          memcpy (&data_addr.sin_addr, request->remote_addr,
                  request->remote_addr_len);

          pos = (char *) &data_addr.sin_addr;
          request->logging_function (gftp_logging_error, request,
               _("Ignoring IP address in PASV response, connecting to %d.%d.%d.%d:%d\n"),
               pos[0] & 0xff, pos[1] & 0xff, pos[2] & 0xff, pos[3] & 0xff,
               ntohs (data_addr.sin_port));
        }
      else
        memcpy (&data_addr.sin_addr, &ad[0], 4);

      if (connect (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
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
      if (bind (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
                data_addr_len) == -1)
      {
        request->logging_function (gftp_logging_error, request,
                  _("Cannot bind a port: %s\n"), g_strerror (errno));
        gftp_disconnect (request);
        return (GFTP_ERETRYABLE);
      }

      if (getsockname (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
                       &data_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                   _("Cannot get socket name: %s\n"), g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      if (listen (ftpdat->data_connection, 1) == -1)
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
      resp = rfc959_send_command (request, command, -1, 1, 1);
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


static int
rfc959_ipv6_data_connection_new (gftp_request * request)
{
  struct sockaddr_in6 data_addr;
  char *pos, buf[64], *command;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  unsigned int port;
  int resp;

  ftpdat = request->protocol_data;
  if ((ftpdat->data_connection = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
               _("Failed to create a IPv6 socket: %s\n"), g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (fcntl (ftpdat->data_connection, F_SETFD, 1) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot set close on exec flag: %s\n"),
                                 g_strerror (errno));

      return (GFTP_ERETRYABLE);
    }

  /* This condition shouldn't happen. We better check anyway... */
  if (sizeof (data_addr) != request->remote_addr_len) 
    {
      request->logging_function (gftp_logging_error, request,
               _("Error: It doesn't look like we are connected via IPv6. Aborting connection.\n"));
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  memset (&data_addr, 0, sizeof (data_addr));
  data_addr.sin6_family = AF_INET6;

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (passive_transfer)
    {
      resp = rfc959_send_command (request, "EPSV\r\n", -1, 1, 1);
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

      memcpy (&data_addr, request->remote_addr, request->remote_addr_len);
      data_addr.sin6_port = htons (port);

      if (connect (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
                   request->remote_addr_len) == -1)
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
      memcpy (&data_addr, request->remote_addr, request->remote_addr_len);
      data_addr.sin6_port = 0;

      if (bind (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
                request->remote_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                  _("Cannot bind a port: %s\n"), g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      if (getsockname (ftpdat->data_connection, (struct sockaddr *) &data_addr, 
                       &request->remote_addr_len) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                  _("Cannot get socket name: %s\n"), g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      if (listen (ftpdat->data_connection, 1) == -1)
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
                   _("Cannot get address of local socket: %s\n"), g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      command = g_strdup_printf ("EPRT |2|%s|%d|\n", buf,
                                 ntohs (data_addr.sin6_port));

      resp = rfc959_send_command (request, command, -1, 1, 1);
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


static int
rfc959_data_connection_new (gftp_request * request, int dont_try_to_reconnect)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if (request->ai_family == AF_INET6)
    ret = rfc959_ipv6_data_connection_new (request);
  else
    ret = rfc959_ipv4_data_connection_new (request);

  if (ret == GFTP_ETIMEDOUT && !dont_try_to_reconnect)
    {
      ret = gftp_connect (request);
      if (ret < 0)
        return (ret);

      return (rfc959_data_connection_new (request, 1));
    }
  else
    return (ret);
}


static int
rfc959_accept_active_connection (gftp_request * request)
{
  int infd, ret;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  struct sockaddr_in6 cli_addr;
  //struct sockaddr_in cli_addr; //?

  socklen_t cli_addr_len;

  ftpdat = request->protocol_data;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (ftpdat->data_connection > 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  g_return_val_if_fail (!passive_transfer, GFTP_EFATAL);

  cli_addr_len = sizeof (cli_addr);

  if ((ret = gftp_fd_set_sockblocking (request, ftpdat->data_connection, 0)) < 0)
    return (ret);

  if ((infd = accept (ftpdat->data_connection, (struct sockaddr *) &cli_addr,
       &cli_addr_len)) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                _("Cannot accept connection from server: %s\n"),
                                g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  close (ftpdat->data_connection);
  ftpdat->data_connection = infd;

  if(ftpdat->data_conn_tls_start != NULL &&
     (ret = ftpdat->data_conn_tls_start(request)) < 0) {
    return ret;
  }
  if ((ret = gftp_fd_set_sockblocking (request, ftpdat->data_connection, 1)) < 0)
    return (ret);

  return (0);
}


static unsigned int
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
  unsigned int new_ascii;
  ftp_protocol_data * ftpdat;
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  new_ascii = rfc959_is_ascii_transfer (request, filename);

  if (request->datafd > 0 && new_ascii != ftpdat->is_ascii_transfer)
    {
      if (new_ascii)
        {
          tempstr = "TYPE A\r\n";
          ftpdat->is_ascii_transfer = 1;
        }
      else
        {
          tempstr = "TYPE I\r\n";
          ftpdat->is_ascii_transfer = 0;
        }

      if ((ret = rfc959_send_command (request, tempstr, -1, 1, 0)) < 0)
        return (ret);
    }

  return (0);
}


static int
rfc959_setup_file_transfer (gftp_request * request, const char *filename,
                            off_t startsize, char *transfer_command)
{
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  char *command;
  int ret;

  ftpdat = request->protocol_data;

  if ((ret = rfc959_set_data_type (request, filename)) < 0)
    return (ret);

  if (ftpdat->data_connection < 0 && 
      (ret = rfc959_data_connection_new (request, 0)) < 0)
    return (ret);

  if ((ret = gftp_fd_set_sockblocking (request, ftpdat->data_connection, 1)) < 0)
    return (ret);

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST " GFTP_OFF_T_PRINTF_MOD "\r\n",
                                 startsize);
      ret = rfc959_send_command (request, command, -1, 1, 0);
      g_free (command);

      if (ret < 0)
        return (ret);
      else if (ret != '3')
        {
          rfc959_close_data_connection (request);
          return (GFTP_ERETRYABLE);
        }
    }

  ret = rfc959_generate_and_send_command (request, transfer_command, filename,
                                          1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '1')
    {
      rfc959_close_data_connection (request);

      if (ret == '5')
        return (GFTP_EFATAL);
      else
        return (GFTP_ERETRYABLE);
    }

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (!passive_transfer &&
      (ret = rfc959_accept_active_connection (request)) < 0)
    return (ret);

  if (passive_transfer &&
      ftpdat->data_conn_tls_start != NULL &&
      (ret = ftpdat->data_conn_tls_start (request)) < 0)
    return ret;

  return (0);
}


static off_t
rfc959_get_file (gftp_request * request, const char *filename,
                 off_t startsize)
{
  char *tempstr;
  int ret;
  intptr_t pretransfer;
  intptr_t passive_transfer;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "pretransfer_command", &pretransfer);
  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  if (passive_transfer && pretransfer) {
    ret = rfc959_generate_and_send_command (request, "PRET RETR", filename,1, 0);
  }

  ret = rfc959_setup_file_transfer (request, filename, startsize, "RETR");
  if (ret < 0)
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
rfc959_put_file (gftp_request * request, const char *filename,
                 off_t startsize, off_t totalsize)
{
  ftp_protocol_data * ftpdat;
  int ret;
  intptr_t pretransfer;
  intptr_t passive_transfer;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_set_data_type (request, filename)) < 0)
    return (ret);

  gftp_lookup_request_option (request, "pretransfer_command", &pretransfer);
  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  if (passive_transfer && pretransfer) {
    ret = rfc959_generate_and_send_command (request, "PRET STOR", filename,1, 0);
  }

  ftpdat = request->protocol_data;
  if (ftpdat->data_connection < 0 && 
      (ret = rfc959_data_connection_new (request, 0)) < 0)
    return (ret);

  return (rfc959_setup_file_transfer (request, filename, startsize, "STOR"));
}


static off_t
rfc959_transfer_file (gftp_request *fromreq, const char *fromfile, 
                      off_t fromsize, gftp_request *toreq, 
                      const char *tofile, off_t tosize)
{
  char *tempstr, *pos, *endpos;
  ftp_protocol_data * ftpdat;
  int ret;

  g_return_val_if_fail (fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromfile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (toreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (tofile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromreq->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (toreq->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_send_command (fromreq, "PASV\r\n", -1, 1, 0)) < 0)
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
  ret = rfc959_send_command (toreq, tempstr, -1, 1, 0);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  ret = rfc959_generate_and_send_command (fromreq, "RETR", fromfile, 0, 0);
  if (ret < 0)
    return (ret);

  ret = rfc959_generate_and_send_command (toreq, "STOR", tofile, 0, 0);
  if (ret < 0)
    return (ret);

  if ((ret = rfc959_read_response (fromreq, 1)) < 0)
    return (ret);

  if ((ret = rfc959_read_response (toreq, 1)) < 0)
    return (ret);

  ftpdat = fromreq->protocol_data;
  ftpdat->is_fxp_transfer = 1;

  ftpdat = toreq->protocol_data;
  ftpdat->is_fxp_transfer = 1;

  return (0);
}


static int
rfc959_end_transfer (gftp_request * request)
{
  ftp_protocol_data * ftpdat;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  ftpdat->is_fxp_transfer = 0;

  rfc959_close_data_connection (request);

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
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_send_command (request, "ABOR\r\n", -1, 0, 0)) < 0)
    return (ret);

  rfc959_close_data_connection (request);

  if (request->datafd > 0)
    {
      if ((ret = rfc959_read_response (request, 0)) < 0)
      gftp_disconnect (request);
    }

  return (0);
}


static int rfc959_list_files (gftp_request * request)
{
  ftp_protocol_data * ftpdat = request->protocol_data;
  intptr_t passive_transfer;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = rfc959_data_connection_new (request, 0)) < 0)
    return (ret);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  if (ftpdat->use_mlsd_cmd) {
     ret = rfc959_send_command (request, "MLSD\r\n", -1, 1, 0);
  } else {
     ret = rfc959_send_command (request, "LIST -al\r\n", -1, 1, 0);
  }
  // fall back to LIST if an error response is sent..
  if (ret > 0 && ret != '1') {
     ret = rfc959_send_command (request, "LIST\r\n", -1, 1, 0);
  }

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
  else if (ftpdat->data_conn_tls_start != NULL)
    ret = ftpdat->data_conn_tls_start(request);

  return (ret);
}


static ssize_t
rfc959_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  ssize_t num_read, ret;
  ftp_protocol_data * ftpdat;
  int i, j;

  ftpdat = request->protocol_data;
  if (ftpdat->is_fxp_transfer)
    return (GFTP_ENOTRANS);

  num_read = ftpdat->data_conn_read (request, buf, size, ftpdat->data_connection);
  if (num_read < 0)
    return (num_read);

  ret = num_read;
  if (ftpdat->is_ascii_transfer)
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
  ssize_t num_wrote, ret;
  ftp_protocol_data * ftpdat;
  char *tempstr, *pos;
  size_t rsize, i, j;

  ftpdat = request->protocol_data;

  if (ftpdat->is_fxp_transfer)
    return (GFTP_ENOTRANS);

  if (ftpdat->is_ascii_transfer)
    {
      rsize = size;
      for (i = 1; i < size; i++)
        {
          if (buf[i] == '\n' && buf[i - 1] != '\r')
            rsize++;
        }

      if (rsize != size)
        {
          tempstr = g_malloc0 (rsize);

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

  /* I need to ensure that the entire buffer has been transferred properly due
     to the ascii conversion that may occur. 
     FIXME - the ascii conversion doesn't occur properly when the \n occurs
     at the beginning of the buffer. I need to remember the last character
     in the previous buffer */

  ret = rsize;
  pos = tempstr;
  while (rsize > 0)
    {
      num_wrote = ftpdat->data_conn_write (request, pos, rsize,
                                          ftpdat->data_connection);
      if (num_wrote < 0)
        {
          ret = num_wrote;
          break;
        }

      pos += num_wrote;
      rsize -= num_wrote;
    }

  if (tempstr != buf)
    g_free (tempstr);

  return (ret);
}


static ssize_t
rfc959_get_next_dirlist_line (gftp_request * request, int fd,
                              char *buf, size_t buflen)
{
  ssize_t (*oldread_func) (gftp_request * request, void *ptr, size_t size,
                           int fd);
  ftp_protocol_data * ftpdat;
  ssize_t len;

  ftpdat = request->protocol_data;

  oldread_func = request->read_function;
  request->read_function = ftpdat->data_conn_read;
  len = gftp_get_line (request, &ftpdat->dataconn_rbuf, buf, buflen, fd);
  request->read_function = oldread_func;

  return (len);
}

int
rfc959_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  ftp_protocol_data * ftpdat;
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

  ftpdat = request->protocol_data;

  if (fd == request->datafd)
    fd = ftpdat->data_connection;

  do
    {
      len = rfc959_get_next_dirlist_line (request, fd, tempstr,
                                          sizeof (tempstr));
      if (len <= 0)
        {
          gftp_file_destroy (fle, 0);
          return (len);
        } 

      if (gftp_parse_ls (request, tempstr, fle, fd) != 0)
        {
          if (strncmp (tempstr, "total", strlen ("total")) != 0 &&
              strncmp (tempstr, _("total"), strlen (_("total"))) != 0)
          {
            request->logging_function (gftp_logging_error, request,
                     _("Warning: Cannot parse listing %s\n"), tempstr);
          }
            gftp_file_destroy (fle, 0);
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
  int ret;

  g_return_val_if_fail (request != NULL, 0);
  g_return_val_if_fail (filename != NULL, 0);
  g_return_val_if_fail (request->datafd > 0, 0);

  ret = rfc959_generate_and_send_command (request, "SIZE", filename, 1, 0);
  if (ret < 0)
    return (ret);

  if (*request->last_ftp_response != '2')
    return (0);

  return (strtol (request->last_ftp_response + 4, NULL, 10));
}


static int
rfc959_rmdir (gftp_request * request, const char *directory)
{
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = rfc959_generate_and_send_command (request, "RMD", directory, 1, 0);
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
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = rfc959_generate_and_send_command (request, "DELE", file, 1, 0);
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
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = rfc959_generate_and_send_command (request, "MKD", directory, 1, 0);
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
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = rfc959_generate_and_send_command (request, "RNFR", oldname, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '3')
    return (GFTP_ERETRYABLE);

  ret = rfc959_generate_and_send_command (request, "RNTO", newname, 1, 0);
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
  char *tempstr, *utf8;
  size_t destlen;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  utf8 = gftp_filename_from_utf8 (request, file, &destlen);
  if (utf8 != NULL)
    {
      tempstr = g_strdup_printf ("SITE CHMOD %o %s\r\n", mode, utf8);
      g_free (utf8);
    }
  else
    tempstr = g_strdup_printf ("SITE CHMOD %o %s\r\n", mode, file);

  ret = rfc959_send_command (request, tempstr, -1, 1, 0);
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
  char *tempstr, *utf8;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (command != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  utf8 = gftp_string_from_utf8 (request, -1, command, &len);
  if (utf8 != NULL)
    {
      if (specify_site)
        {
          len += 7;
          tempstr = g_strconcat ("SITE ", utf8, "\r\n", NULL);
        }
      else
        {
          len += 2;
          tempstr = g_strconcat (utf8, "\r\n", NULL);
        }

      g_free (utf8);
    }
  else
    {
      if (specify_site)
        {
          len = strlen (command) + 7;
          tempstr = g_strconcat ("SITE ", command, "\r\n", NULL);
        }
      else
        {
          len = strlen (command) + 2;
          tempstr = g_strconcat (command, "\r\n", NULL);
        }
    }

  ret = rfc959_send_command (request, tempstr, len, 1, 0);
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
  gftp_lookup_request_option (request, "proxy_config", &proxy_config);
  if (strcmp (proxy_config, "http") == 0)
     gftp_set_request_option (request, "proxy_config", "ftp");

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
  ftp_protocol_data * ftpdat;

  ftpdat = request->protocol_data;

  if (ftpdat->datafd_rbuf != NULL)
    gftp_free_getline_buffer (&ftpdat->datafd_rbuf);

  if (ftpdat->dataconn_rbuf != NULL)
    gftp_free_getline_buffer (&ftpdat->dataconn_rbuf);
}


static void
rfc959_copy_param_options (gftp_request * dest_request,
                           gftp_request * src_request)
{
  ftp_protocol_data * dftpdat, * sftpdat;

  dftpdat = dest_request->protocol_data;
  sftpdat = src_request->protocol_data;

  dftpdat->data_connection     = -1;
  dftpdat->is_ascii_transfer   = sftpdat->is_ascii_transfer;
  dftpdat->is_fxp_transfer     = sftpdat->is_fxp_transfer;
  dftpdat->auth_tls_start      = sftpdat->auth_tls_start;
  dftpdat->data_conn_tls_start = sftpdat->data_conn_tls_start;
  dftpdat->data_conn_read      = sftpdat->data_conn_read;
  dftpdat->data_conn_write     = sftpdat->data_conn_write;
  dftpdat->data_conn_tls_close = sftpdat->data_conn_tls_close;
  dftpdat->implicit_ssl        = sftpdat->implicit_ssl;
  dftpdat->use_mlsd_cmd        = sftpdat->use_mlsd_cmd;
  dftpdat->last_cmd            = sftpdat->last_cmd;
  dftpdat->flags               = sftpdat->flags;

  dest_request->read_function = src_request->read_function;
  dest_request->write_function = src_request->write_function;
}


int
rfc959_init (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
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

  request->protonum = GFTP_PROTOCOL_FTP;
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
  request->set_file_time = NULL;
  request->site = rfc959_site;
  request->parse_url = NULL;
  request->swap_socks = NULL;
  request->set_config_options = rfc959_set_config_options;
  request->url_prefix = "ftp";
  request->need_hostport = 1;
  request->need_username = 1;
  request->need_password = 1;
  request->use_cache = 1;
  request->always_connected = 0;
  request->use_local_encoding = 0;

  request->protocol_data = g_malloc0 (sizeof (ftp_protocol_data));
  ftpdat = request->protocol_data;
  ftpdat->data_connection = -1; 
  ftpdat->auth_tls_start = NULL;
  ftpdat->data_conn_tls_start = NULL;
  ftpdat->data_conn_read = gftp_fd_read;
  ftpdat->data_conn_write = gftp_fd_write;
  ftpdat->data_conn_tls_close = NULL;

  return (gftp_set_config_options (request));
}

