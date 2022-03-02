/***********************************************************************************/
/*  protocol_ftp.c - General purpose routines for the FTP protocol (RFC 959)       */
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
#include "protocol_ftp.h"

// RFC959

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
  {"", "FTP", gftp_option_type_notebook, NULL, NULL, 
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

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};


// ==========================================================================

struct ftp_supported_feature ftp_supported_features[] =
{
   // If a server supports MLST, it must also support MLSD
   // some (most?) servers don't report MLSD even though it's supported
   // Should only use MLST to test if MLSD is supported
   { FTP_FEAT_MLSD, "MLST" },
   { FTP_FEAT_PRET, "PRET" },
   { FTP_FEAT_EPSV, "EPSV" }, /* rfc2428 */
   { FTP_FEAT_EPRT, "EPRT" }, /* rfc2428 */
   { -1, NULL },
};

static void rfc2389_feat_supported_cmd (ftp_protocol_data * ftpdat, char * cmd)
{
   DEBUG_PUTS(cmd)
   char *p = cmd;
   int i;
   while (*p <= 32) p++; // strip leading spaces
   for (i = 0; ftp_supported_features[i].str; i++)
   {
      if (strcmp (p, ftp_supported_features[i].str) == 0) {
          ftpdat->feat[i] = 1;
          DEBUG_TRACE("Supported feature: %s\n", ftp_supported_features[i].str)
          return;
      }
      if (strlen (p) > 4) {
         if (strncmp (p, ftp_supported_features[i].str, 4) == 0) {
             ftpdat->feat[i] = 1;
             DEBUG_TRACE("Supported feature: %s\n", ftp_supported_features[i].str)
             return;
         }
      }
   }
}

// ==========================================================================

static int ftp_read_response (gftp_request * request, int disconnect_on_42x)
{
  //DEBUG_PRINT_FUNC
  // The retrieved string from the socket may contain 1 or more lines, 1 or more numeric codes
  //   but the function must return the first numeric code
  // The buffer should retain the rest of the string to be processed
  //   in the next call to this function
  // Only after processing the stored lines, the function must retrieve
  //   more data from the socket
  char code[4];
  ftp_protocol_data * ftpdat;
  ssize_t ret;
  int lines = 0;
  char *p;
  char fixed_line[256];
  char * line = NULL;
  char * last_line = NULL;
  char * pos = NULL;
  int line_ending = 0;
  size_t buffer_size = 2048; //sizeof(ftpdat->response_buffer);

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if (request->last_ftp_response) {
      g_free (request->last_ftp_response);
      request->last_ftp_response = NULL;
  }

  ftpdat = request->protocol_data;

  if (ftpdat->response_buffer == NULL) {
     ftpdat->response_buffer = malloc (buffer_size);
     ftpdat->response_buffer[0] = 0;
  }
  pos = ftpdat->response_buffer_pos;

  ftpdat->last_response_code = 0;
  *code = '\0';
  *fixed_line = 0;

  while (1)
  {
     line = str_get_next_line_pointer (ftpdat->response_buffer, &pos, &line_ending);
     if (!line)
     {
        // read text chunk, may not contain the whole response
        //DEBUG_TRACE("$$$  request->read_function: %p\n", request->read_function)
        ret = request->read_function (request, ftpdat->response_buffer, buffer_size, request->datafd);
        if (ret <= 0) { /* reset buffer */
           ftpdat->response_buffer_pos = NULL;
           ftpdat->response_buffer[0] = 0;
           break;
        }
        ftpdat->response_buffer[ret] = '\0';
        /* reset pos & lines */
        pos = NULL;
        lines = 0;
        /* count lines */
        for (p = ftpdat->response_buffer; *p; p++) {
           if (*p == '\n') lines++;
        }
        if (!lines) { /* reset buffer */
           ftpdat->response_buffer_pos = NULL;
           ftpdat->response_buffer[0] = 0;
           break;
        }
        //DEBUG_TRACE("%d##%d>>> %s <<<\n",lines, strlen(ftpdat->response_buffer), ftpdat->response_buffer)
        continue;
     }
     ftpdat->response_buffer_pos = pos;
/*
     if (pos)
     {
        if (!line_ending) {
           // incomplete line, will need to fix
           snprintf (fixed_line, sizeof(fixed_line), "%s", line);
           DEBUG_PUTS ("@ incomplete line, will need to fix\n")
           continue;
        } else if (*fixed_line) {
           // copy and complete line
           snprintf (fixed_line + strlen(fixed_line), sizeof(fixed_line) - strlen(fixed_line) - 1,
                     "%s", line);
           line = fixed_line;
           DEBUG_PUTS ("@@ copy and complete line\n")
        }
     }
*/
     if (isdigit((int)line[0]) && isdigit((int)line[1]) && isdigit((int)line[2]))
     {
        if (!*code) {
           // reply code has been identified, it will be used to determine when to stop
           strncpy (code, line, 3);
           code[3] = ' ';
        }
     }

     if (ftpdat->last_cmd == FTP_CMD_FEAT) {
         rfc2389_feat_supported_cmd (ftpdat, line);
     } else {
        if (*line == '4' || *line == '5')
           request->logging_function (gftp_logging_error, request, "%s\n", line);
        else
           request->logging_function (gftp_logging_recv, request, "%s\n", line);
     }

     if (*code && strncmp (code, line, 4) == 0) {
        code[3] = '\0';
        ftpdat->last_response_code = atoi (code);
        code[3] = ' ';
        last_line = line;
        break; /* 'XXX ' end of current response */
     }
/*
     if (line_ending && *fixed_line) {
        *fixed_line = 0;
     }
*/
  }

  if (!last_line) {
     DEBUG_MSG("no last line!!!!\n\n")
     return (GFTP_ERETRYABLE);
  }
  DEBUG_TRACE("RESPONSE: %s\n", last_line);
  request->last_ftp_response = g_strdup (last_line);

  if (request->last_ftp_response[0] == '4' &&
      request->last_ftp_response[1] == '2' &&
      disconnect_on_42x)
    {
      gftp_disconnect (request);
      return (GFTP_ETIMEDOUT);
    }

  return (*request->last_ftp_response);
}


int ftp_send_command (gftp_request * request, const char *command, 
                      ssize_t command_len, int read_response,
                      int dont_try_to_reconnect)
{
  //DEBUG_PRINT_FUNC
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

  DEBUG_TRACE("SEND_CMD: %s", command)
  ret = request->write_function (request, command, command_len, request->datafd);
  if (ret < 0) {
     DEBUG_MSG("failed to write to socket!!!!\n\n");
     return (ret);
  }
  if (read_response)
    {
      ret = ftp_read_response (request, 1);
      if (ret == GFTP_ETIMEDOUT && !dont_try_to_reconnect)
        {
          ret = gftp_connect (request);
          if (ret < 0)
            return (ret);

          return (ftp_send_command (request, command, command_len, 1, 1));
        }
      else
        return (ret);
    }
  else
    return (0);
}


static int ftp_generate_and_send_command (gftp_request * request,
                                  const char *command,
                                  const char *argument, int read_response,
                                  int dont_try_to_reconnect)
{
  //DEBUG_PRINT_FUNC
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

  resp = ftp_send_command (request, tempstr, len, read_response,
                              dont_try_to_reconnect);
  g_free (tempstr);
  return (resp);
}


static char * parse_ftp_proxy_string (gftp_request * request)
{
  DEBUG_PRINT_FUNC
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


static int ftp_getcwd (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  char *pos, *dir, *utf8;
  size_t destlen;
  int ret;

  ret = ftp_send_command (request, "PWD\r\n", -1, 1, 0);
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


static int ftp_chdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (strcmp (directory, "..") == 0)
    ret = ftp_send_command (request, "CDUP\r\n", -1, 1, 0);
  else
    ret = ftp_generate_and_send_command (request, "CWD", directory, 1, 0);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  if ((ret = ftp_getcwd (request)) < 0)
    return (ret);

  return (0);
}


static int ftp_syst (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  int ret;
  ftp_protocol_data * ftpdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  ftpdat->list_dirtype_hint = -1;
  if (ftpdat->feat[FTP_FEAT_MLSD]) {
     return 0; // MLSD has been detected
  }

  ret = ftp_send_command (request, "SYST\r\n", -1, 1, 0);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  if (strchr (request->last_ftp_response, ' ')) {
     parse_syst_response (request->last_ftp_response, ftpdat);
     return 0;
  } else {
     return (GFTP_ERETRYABLE);
  }
}


int ftp_connect (gftp_request * request)
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

  /* Get the banner */
  if ((ret = ftp_read_response (request, 1)) != '2')
    {
      gftp_disconnect (request);
      if (ret < 0)
        return (ret);
      else
        return (GFTP_ERETRYABLE);
    }

  if (ftpdat->auth_tls_start) {
     ret = ftpdat->auth_tls_start (request);
     if (ret < 0) {
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
            if ((resp = ftp_send_command (request, startpos, -1, 1, 0)) < 0)
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
      resp = ftp_generate_and_send_command (request, "USER",
                                               request->username, 1, 0);
      if (resp < 0)
        return (resp);

      if (resp == '3')
        {
          resp = ftp_generate_and_send_command (request, "PASS",
                                                   request->password, 1, 0);
          if (resp < 0)
            return (resp);
        }

      if (resp == '3' && request->account != NULL)
        {
          resp = ftp_generate_and_send_command (request, "ACCT",
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
  ftpdat->feat[FTP_FEAT_LIST_AL] = 1; /* use LIST -al by default */
  ftpdat->feat[FTP_FEAT_SITE] = 1;
  ftpdat->last_cmd = FTP_CMD_FEAT;
  ret = ftp_send_command (request, "FEAT\r\n", -1, 1, 0);
  ftpdat->last_cmd = 0;

  // determine system type
  if ((ret = ftp_syst (request)) < 0 && request->datafd < 0) {
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

  if ((ret = ftp_send_command (request, tempstr, -1, 1, 0)) < 0)
    return (ret);

  ret = -1;
  if (request->directory != NULL && *request->directory != '\0')
    {
      ret = ftp_chdir (request, request->directory);
      if (request->datafd < 0)
        return (ret);
    }

  if (ret != 0)
    {
      if ((ret = ftp_getcwd (request)) < 0)
        return (ret);
    }

  if (request->datafd < 0)
    return (GFTP_EFATAL);

  return (0);
}


static void ftp_close_data_connection (gftp_request * request)
{
  DEBUG_PRINT_FUNC
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


static void ftp_disconnect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  g_return_if_fail (request != NULL);

  if (request->datafd > 0)
    {
      ftp_close_data_connection (request);

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


static int ftp_do_data_connection_new (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  // FTP Extensions for IPv6 and NATs (https://datatracker.ietf.org/doc/html/rfc2428)
  char *pos, *pos1, ipstr[64], *command;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  unsigned int port;
  int i, resp;
  int USE_EPSV = 0;
  int USE_EPRT = 0;
  int invalid_response = 0;
  intptr_t ignore_pasv_address; //PASV
  unsigned int temp[6];         //PASV
  unsigned char ad[6];          //PASV
  struct sockaddr_in * paddrin; //PASV
  int AFPROT = request->ai_family; // AF_INET6 or AF_INET;
  // request->remote_addr is copied from the main ->request
  // - it's addrinfo->ai_addr (sockaddr_in or sockaddr_in6)
  struct sockaddr * saddr = request->remote_addr;

  DEBUG_TRACE("## IP protocol is IPv%c\n", AFPROT == AF_INET6 ? '6' : '4');
  g_return_val_if_fail (AFPROT == saddr->sa_family, GFTP_EFATAL);
  w_sockaddr_get_ip_str (saddr, ipstr, sizeof(ipstr));
  DEBUG_TRACE("## IP: %s\n", ipstr);

  ftpdat = request->protocol_data;

  // use EPRT/EPSV if the feature is detected or IPv6
  if (ftpdat->feat[FTP_FEAT_EPRT] || AFPROT == AF_INET6) {
      USE_EPRT = 1;
  }
  if (ftpdat->feat[FTP_FEAT_EPSV] || AFPROT == AF_INET6) {
      USE_EPSV = 1;
  }

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  if (passive_transfer)
  {
      if (USE_EPSV) {
          resp = ftp_send_command (request, "EPSV\r\n", -1, 1, 1);
          if (ftpdat->last_response_code == 500) { // unrecognized command
              // - deal with a broken server or pure-ftpd -b
              // - this error is only possible if using ipv4
              USE_EPSV = USE_EPRT = 0;
              ftpdat->feat[FTP_FEAT_EPSV] = 0;
              ftpdat->feat[FTP_FEAT_EPRT] = 0;
              resp = ftp_send_command (request, "PASV\r\n", -1, 1, 1);
          }
      } else {
          resp = ftp_send_command (request, "PASV\r\n", -1, 1, 1);
      }
      if (resp < 0) {
          return (resp);
      } else if (resp != '2') {
          gftp_disconnect (request);
          return GFTP_ERETRYABLE;
      }

      if (USE_EPSV) {
          // need only port number
          // 229 Entering Extended Passive Mode (|||65113|)
          resp = -1;
          pos = strchr (request->last_ftp_response + 4, '(');
          if (pos) {
              resp = sscanf (pos + 1, "|||%u|", &port);
          }
          if (!pos || resp != 1) {
              invalid_response = 1;
          }
      } else {
          // PASV provides ip+port (ip0,ip1,ip2,ip3,port1,port2)
          resp = -1;
          pos = request->last_ftp_response + 4;
          while (!isdigit ((int) *pos) && *pos != '\0') {
              pos++;
          }
          if (*pos) {
              resp = sscanf (pos, "%u,%u,%u,%u,%u,%u", &temp[0], &temp[1], &temp[2], &temp[3], &temp[4], &temp[5]);
          }
          if (!*pos || resp != 6) {
              invalid_response = 1;
          }
      }

      if (invalid_response)
      {
          request->logging_function (gftp_logging_error, request, _("Invalid response '%s'\n"),
                                     request->last_ftp_response);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
      }

      if (USE_EPSV) {
          //w_sockaddr_reset (saddr);
          w_sockaddr_set_port (saddr, port);
      } else {
          // PASV - IPv4
          paddrin = (struct sockaddr_in *) request->remote_addr;
          for (i = 0; i < 6; i++) {
              ad[i] = (unsigned char) (temp[i] & 0xff);
          }
          gftp_lookup_request_option (request, "ignore_pasv_address", &ignore_pasv_address);
          if (ignore_pasv_address) {
              request->logging_function (gftp_logging_error, request, _("Ignoring IP address in PASV response\n"));
          } else {
              // change IP to the one specified in PASV response
              memcpy (&(paddrin->sin_addr), &ad[0], 4);
          }
          memcpy (&(paddrin->sin_port), &ad[4], 2); /* set port */
      }

      ftpdat->data_connection = gftp_data_connection_new (request);
      if (ftpdat->data_connection < 0)
      {
          gftp_disconnect (request);
          return GFTP_ERETRYABLE;
      }

  }
  else /* == active mode == */
  {
      ftpdat->data_connection = gftp_data_connection_new_listen (request);

      if (ftpdat->data_connection < 0)
      {
          gftp_disconnect (request);
          return GFTP_ERETRYABLE;
      }

      port = w_sockaddr_get_port (saddr);

      if (USE_EPRT) {
          // EPRT |X|ipstr|port|
          command = g_strdup_printf ("EPRT |%c|%s|%d|\r\n",
                                    AFPROT == AF_INET6 ? '2' : '1', // 1=IPV4 2=IPV6
                                    ipstr, port);
      } else {
          // PORT IPv4
          // PORT ip0,ip1,ip2.ip3.port1,port2
          pos = (char *) &(((struct sockaddr_in *)saddr)->sin_addr);
          pos1 = (char *) &(((struct sockaddr_in *)saddr)->sin_port);
          command = g_strdup_printf ("PORT %u,%u,%u,%u,%u,%u\r\n",
                                    pos[0] & 0xff, pos[1] & 0xff, pos[2] & 0xff,
                                    pos[3] & 0xff, pos1[0] & 0xff, pos1[1] & 0xff);
      }

      resp = ftp_send_command (request, command, -1, 1, 1);
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


static int ftp_data_connection_new (gftp_request * request, int dont_try_to_reconnect)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = ftp_do_data_connection_new (request);

  if (ret == GFTP_ETIMEDOUT && !dont_try_to_reconnect)
    {
      ret = gftp_connect (request);
      if (ret < 0)
        return (ret);

      return (ftp_data_connection_new (request, 1));
    }
  else
    return (ret);
}


static int ftp_accept_active_connection (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  int ret;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;

  ftpdat = request->protocol_data;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (ftpdat->data_connection > 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  g_return_val_if_fail (!passive_transfer, GFTP_EFATAL);

  ret = gftp_accept_connection (request, &ftpdat->data_connection);
  if (ret < 0) {
      return GFTP_ERETRYABLE;
  }

  if (ftpdat->data_conn_tls_start)
  {
     if (ftpdat->data_conn_tls_start(request) < 0)
         return GFTP_ERETRYABLE;
  }

  ret = gftp_fd_set_sockblocking (request, ftpdat->data_connection, 1);
  if (ret < 0) {
      return GFTP_ERETRYABLE;
  }

  return (0);
}


static unsigned int
ftp_is_ascii_transfer (gftp_request * request, const char *filename)
{
  DEBUG_PRINT_FUNC
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


static int ftp_set_data_type (gftp_request * request, const char *filename)
{
  DEBUG_PRINT_FUNC
  unsigned int new_ascii;
  ftp_protocol_data * ftpdat;
  char *tempstr;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  new_ascii = ftp_is_ascii_transfer (request, filename);

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

      if ((ret = ftp_send_command (request, tempstr, -1, 1, 0)) < 0)
        return (ret);
    }

  return (0);
}


static int ftp_setup_file_transfer (gftp_request * request,
                                    const char *filename,
                                    off_t startsize,
                                    char *transfer_command)
{
  DEBUG_PRINT_FUNC
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;
  char *command;
  int ret;

  ftpdat = request->protocol_data;

  if ((ret = ftp_set_data_type (request, filename)) < 0)
    return (ret);

  if (ftpdat->data_connection < 0 && 
      (ret = ftp_data_connection_new (request, 0)) < 0)
    return (ret);

  if ((ret = gftp_fd_set_sockblocking (request, ftpdat->data_connection, 1)) < 0)
    return (ret);

  if (startsize > 0)
    {
      command = g_strdup_printf ("REST " GFTP_OFF_T_PRINTF_MOD "\r\n",
                                 startsize);
      ret = ftp_send_command (request, command, -1, 1, 0);
      g_free (command);

      if (ret < 0)
        return (ret);
      else if (ret != '3')
        {
          ftp_close_data_connection (request);
          return (GFTP_ERETRYABLE);
        }
    }

  ret = ftp_generate_and_send_command (request, transfer_command, filename,
                                          1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '1')
    {
      ftp_close_data_connection (request);

      if (ret == '5')
        return (GFTP_EFATAL);
      else
        return (GFTP_ERETRYABLE);
    }

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);
  if (!passive_transfer &&
      (ret = ftp_accept_active_connection (request)) < 0)
    return (ret);

  if (passive_transfer &&
      ftpdat->data_conn_tls_start != NULL &&
      (ret = ftpdat->data_conn_tls_start (request)) < 0)
    return ret;

  return (0);
}


static off_t ftp_get_file (gftp_request * request, const char *filename,
                           off_t startsize)
{
  DEBUG_PRINT_FUNC
  char *tempstr;
  int ret;
  intptr_t passive_transfer;
  ftp_protocol_data * ftpdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  ftpdat = request->protocol_data;
  if (passive_transfer && ftpdat->feat[FTP_FEAT_PRET]) {
    ret = ftp_generate_and_send_command (request, "PRET RETR", filename,1, 0);
    if (ftpdat->last_response_code == 500) {
       ftpdat->feat[FTP_FEAT_PRET] = 0; // disable silently
    }
    if (ret < 0) {
       return (ret);
    }
  }

  ret = ftp_setup_file_transfer (request, filename, startsize, "RETR");
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


static int ftp_put_file (gftp_request * request, const char *filename,
                         off_t startsize, off_t totalsize)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
  int ret;
  intptr_t passive_transfer;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = ftp_set_data_type (request, filename)) < 0)
    return (ret);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  ftpdat = request->protocol_data;
  if (passive_transfer && ftpdat->feat[FTP_FEAT_PRET]) {
    ret = ftp_generate_and_send_command (request, "PRET STOR", filename,1, 0);
    if (ftpdat->last_response_code == 500) {
       ftpdat->feat[FTP_FEAT_PRET] = 0; // disable silently
    }
  }

  if (ftpdat->data_connection < 0 && 
      (ret = ftp_data_connection_new (request, 0)) < 0)
    return (ret);

  return (ftp_setup_file_transfer (request, filename, startsize, "STOR"));
}


static off_t ftp_transfer_file (gftp_request *fromreq, const char *fromfile, 
                                off_t fromsize, gftp_request *toreq, 
                                const char *tofile, off_t tosize)
{
  // FXP, transfer between FTP servers. Untested
  DEBUG_PRINT_FUNC
  char *tempstr, *pos, *endpos;
  ftp_protocol_data * ftpdat, * ftpfrom, * ftpto;
  int ret;

  g_return_val_if_fail (fromreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromfile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (toreq != NULL, GFTP_EFATAL);
  g_return_val_if_fail (tofile != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fromreq->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (toreq->datafd > 0, GFTP_EFATAL);

  ftpfrom = fromreq->protocol_data;
  ftpto   = toreq->protocol_data;
  if (ftpfrom->feat[FTP_FEAT_EPSV] && ftpto->feat[FTP_FEAT_EPSV]) {
     ret = ftp_send_command (fromreq, "EPSV\r\n", -1, 1, 0);
  } else {
     ret = ftp_send_command (fromreq, "PASV\r\n", -1, 1, 0);
  }
  if (ret < 0)
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
  ret = ftp_send_command (toreq, tempstr, -1, 1, 0);
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_ERETRYABLE);

  ret = ftp_generate_and_send_command (fromreq, "RETR", fromfile, 0, 0);
  if (ret < 0)
    return (ret);

  ret = ftp_generate_and_send_command (toreq, "STOR", tofile, 0, 0);
  if (ret < 0)
    return (ret);

  if ((ret = ftp_read_response (fromreq, 1)) < 0)
    return (ret);

  if ((ret = ftp_read_response (toreq, 1)) < 0)
    return (ret);

  ftpdat = fromreq->protocol_data;
  ftpdat->is_fxp_transfer = 1;

  ftpdat = toreq->protocol_data;
  ftpdat->is_fxp_transfer = 1;

  return (0);
}


static int ftp_end_transfer (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  ftpdat->is_fxp_transfer = 0;

  ftp_close_data_connection (request);

  ret = ftp_read_response (request, 1);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_abort_transfer (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = ftp_send_command (request, "ABOR\r\n", -1, 0, 0)) < 0)
    return (ret);

  ftp_close_data_connection (request);

  if (request->datafd > 0)
    {
      if ((ret = ftp_read_response (request, 0)) < 0)
      gftp_disconnect (request);
    }

  return (0);
}


static int ftp_list_files (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat = request->protocol_data;
  intptr_t passive_transfer;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if ((ret = ftp_data_connection_new (request, 0)) < 0)
    return (ret);

  gftp_lookup_request_option (request, "passive_transfer", &passive_transfer);

  ftpdat->list_dirtype = -1; /* detect dirtype */

  if (ftpdat->feat[FTP_FEAT_MLSD]) {
     ret = ftp_send_command (request, "MLSD\r\n", -1, 1, 0);
     if (ftpdat->last_response_code == 500) {
         ftpdat->feat[FTP_FEAT_MLSD] = 0;
         ret = ftp_send_command (request, "LIST\r\n", -1, 1, 0);
     }
  } else if (ftpdat->feat[FTP_FEAT_LIST_AL]) {
     ret = ftp_send_command (request, "LIST -al\r\n", -1, 1, 0);
     if (ftpdat->last_response_code == 550) {
        ftpdat->feat[FTP_FEAT_LIST_AL] = 0;
        ret = ftp_send_command (request, "LIST\r\n", -1, 1, 0);
     }
  } else {
     ret = ftp_send_command (request, "LIST\r\n", -1, 1, 0);
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
    ret = ftp_accept_active_connection (request);
  else if (ftpdat->data_conn_tls_start != NULL)
    ret = ftpdat->data_conn_tls_start(request);

  return (ret);
}


static ssize_t ftp_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  //DEBUG_PRINT_FUNC
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


static ssize_t ftp_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  //DEBUG_PRINT_FUNC
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


ssize_t ftp_get_next_dirlist_line (gftp_request * request, int fd,
                                          char *buf, size_t buflen)
{
  //DEBUG_PRINT_FUNC
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


int ftp_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  //DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
  char tempstr[2048];
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
      len = ftp_get_next_dirlist_line (request, fd, tempstr,
                                          sizeof (tempstr));
      if (len <= 0)
        {
          gftp_file_destroy (fle, 0);
          return (len);
        } 

      if (ftp_parse_ls (request, tempstr, fle, fd) != 0)
      {
          if (strncmp (tempstr, "total", strlen ("total")) != 0 &&
              strncmp (tempstr, _("total"), strlen (_("total"))) != 0)
          {
            if (strstr(tempstr,"-al") && (strstr(tempstr,"No such") || strstr(tempstr,"no such")))
            {
                // TODO: trigger a LIST again
                ftpdat->feat[FTP_FEAT_LIST_AL] = 0;
                request->logging_function (gftp_logging_error, request,
                         "It looks like LIST -al is not supported\n");

            } else {
                request->logging_function (gftp_logging_error, request,
                     _("Warning: Cannot parse listing %s\n"), tempstr);
            }
          }
            gftp_file_destroy (fle, 0);
            continue;
      }
      else
      {
         if (fle->file == NULL) {
            DEBUG_MSG("@@ entry has been discarded\n")
            gftp_file_destroy (fle, 0);
            continue;
         }
         break;
      }
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


static off_t ftp_get_file_size (gftp_request * request, const char *filename)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, 0);
  g_return_val_if_fail (filename != NULL, 0);
  g_return_val_if_fail (request->datafd > 0, 0);

  ret = ftp_generate_and_send_command (request, "SIZE", filename, 1, 0);
  if (ret < 0)
    return (ret);

  if (*request->last_ftp_response != '2')
    return (0);

  return (strtol (request->last_ftp_response + 4, NULL, 10));
}


static int ftp_rmdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = ftp_generate_and_send_command (request, "RMD", directory, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_rmfile (gftp_request * request, const char *file)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = ftp_generate_and_send_command (request, "DELE", file, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_mkdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = ftp_generate_and_send_command (request, "MKD", directory, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_rename (gftp_request * request, const char *oldname,
               const char *newname)
{
  DEBUG_PRINT_FUNC
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ret = ftp_generate_and_send_command (request, "RNFR", oldname, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '3')
    return (GFTP_ERETRYABLE);

  ret = ftp_generate_and_send_command (request, "RNTO", newname, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_chmod (gftp_request * request, const char *file, mode_t mode)
{
  DEBUG_PRINT_FUNC
  char *tempstr, *utf8;
  size_t destlen;
  int ret;
  ftp_protocol_data * ftpdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  if (!ftpdat->feat[FTP_FEAT_SITE]) {
     request->logging_function (gftp_logging_error, request,"The server doesn't support SITE CHMOD\n");
     return (GFTP_ERETRYABLE);
  }

  utf8 = gftp_filename_from_utf8 (request, file, &destlen);
  if (utf8 != NULL)
    {
      tempstr = g_strdup_printf ("SITE CHMOD %o %s\r\n", mode, utf8);
      g_free (utf8);
    }
  else
    tempstr = g_strdup_printf ("SITE CHMOD %o %s\r\n", mode, file);

  ret = ftp_send_command (request, tempstr, -1, 1, 0);
  if (ftpdat->last_response_code) {
      ftpdat->feat[FTP_FEAT_SITE] = 0;
  }
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_site (gftp_request * request, int specify_site, const char *command)
{
  DEBUG_PRINT_FUNC
  char *tempstr, *utf8;
  size_t len;
  int ret;
  ftp_protocol_data * ftpdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (command != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  ftpdat = request->protocol_data;
  if (!ftpdat->feat[FTP_FEAT_SITE]) {
     request->logging_function (gftp_logging_error, request,"The server doesn't support the SITE command\n");
     return (GFTP_ERETRYABLE);
  }

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

  ret = ftp_send_command (request, tempstr, len, 1, 0);
  if (ftpdat->last_response_code) {
      ftpdat->feat[FTP_FEAT_SITE] = 0;
  }
  g_free (tempstr);

  if (ret < 0)
    return (ret);
  else if (ret == '2')
    return (0);
  else
    return (GFTP_ERETRYABLE);
}


static int ftp_set_config_options (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  return (0);
}


void  ftp_register_module (void)
{
  DEBUG_PRINT_FUNC
  gftp_register_config_vars (config_vars);
  DEBUG_TRACE("~~  gftp_fd_read:  %p\n", gftp_fd_read)
  DEBUG_TRACE("~~  gftp_fd_write: %p\n", gftp_fd_write)
}


static void ftp_request_destroy (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;

  ftpdat = request->protocol_data;

  if (ftpdat->response_buffer) {
     free (ftpdat->response_buffer);
     ftpdat->response_buffer = NULL;
     ftpdat->response_buffer_pos = NULL;
  }
  if (ftpdat->dataconn_rbuf != NULL)
    gftp_free_getline_buffer (&ftpdat->dataconn_rbuf);
}


static void ftp_copy_param_options (gftp_request * dest_request,
                                    gftp_request * src_request)
{
  DEBUG_PRINT_FUNC
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
  dftpdat->list_dirtype        = sftpdat->list_dirtype;
  dftpdat->list_dirtype_hint   = sftpdat->list_dirtype_hint;
  dftpdat->last_cmd            = sftpdat->last_cmd;
  memcpy (dftpdat->feat, sftpdat->feat, sizeof(sftpdat->feat));

  dest_request->read_function = src_request->read_function;
  dest_request->write_function = src_request->write_function;
}


int ftp_init (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
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
      tempstr = g_strconcat (pw->pw_name, "@", unme.nodename, NULL);
      gftp_set_global_option ("email", tempstr);
      g_free (tempstr);
    }

  request->protonum = GFTP_PROTOCOL_FTP;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_FTP].url_prefix;

  request->init = ftp_init;
  request->copy_param_options = ftp_copy_param_options;
  request->destroy = ftp_request_destroy; 
  request->read_function = gftp_fd_read;
  request->write_function = gftp_fd_write;
  request->connect = ftp_connect;
  request->post_connect = NULL;
  request->disconnect = ftp_disconnect;
  request->get_file = ftp_get_file;
  request->put_file = ftp_put_file;
  request->transfer_file = ftp_transfer_file;
  request->get_next_file_chunk = ftp_get_next_file_chunk;
  request->put_next_file_chunk = ftp_put_next_file_chunk;
  request->end_transfer = ftp_end_transfer;
  request->abort_transfer = ftp_abort_transfer;
  request->stat_filename = NULL;
  request->list_files = ftp_list_files;
  request->get_next_file = ftp_get_next_file;
  request->get_file_size = ftp_get_file_size;
  request->chdir = ftp_chdir;
  request->rmdir = ftp_rmdir;
  request->rmfile = ftp_rmfile;
  request->mkdir = ftp_mkdir;
  request->rename = ftp_rename;
  request->chmod = ftp_chmod;
  request->set_file_time = NULL;
  request->site = ftp_site;
  request->parse_url = NULL;
  request->swap_socks = NULL;
  request->set_config_options = ftp_set_config_options;
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

