/*****************************************************************************/
/*  sshv2.c - functions that will use the sshv2 protocol                     */
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

#define SSH_MAX_HANDLE_SIZE		256
#define SSH_MAX_STRING_SIZE		34000

static gftp_config_vars config_vars[] =
{
  {"", N_("SSH"), gftp_option_type_notebook, NULL, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},

  {"ssh_prog_name", N_("SSH Prog Name:"), 
   gftp_option_type_text, "ssh", NULL, 0,
   N_("The path to the SSH executable"), GFTP_PORT_ALL, NULL},
  {"ssh_extra_params", N_("SSH Extra Params:"), 
   gftp_option_type_text, NULL, NULL, 0,  
   N_("Extra parameters to pass to the SSH program"), GFTP_PORT_ALL, NULL},

  {"ssh_need_userpass", N_("Need SSH User/Pass"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Require a username/password for SSH connections"), GFTP_PORT_ALL, NULL},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};


typedef struct sshv2_message_tag
{
  guint32 length;
  char command;
  char *buffer,
       *pos,
       *end;
} sshv2_message;

typedef struct sshv2_params_tag
{
  char handle[SSH_MAX_HANDLE_SIZE + 4], /* We'll encode the ID in here too */
       *read_buffer;

  guint32 id,
          count;
  size_t handle_len;
  sshv2_message message;

  unsigned int initialized : 1,
               dont_log_status : 1;              /* For uploading files */

#ifdef G_HAVE_GINT64
  guint64 offset;
#else
  guint32 offset;
#endif
} sshv2_params;


#define SSH_MY_VERSION              3

#define SSH_FXP_INIT                1
#define SSH_FXP_VERSION             2
#define SSH_FXP_OPEN                3
#define SSH_FXP_CLOSE               4
#define SSH_FXP_READ                5
#define SSH_FXP_WRITE               6
#define SSH_FXP_LSTAT               7
#define SSH_FXP_FSTAT               8
#define SSH_FXP_SETSTAT             9
#define SSH_FXP_FSETSTAT           10
#define SSH_FXP_OPENDIR            11
#define SSH_FXP_READDIR            12
#define SSH_FXP_REMOVE             13
#define SSH_FXP_MKDIR              14
#define SSH_FXP_RMDIR              15
#define SSH_FXP_REALPATH           16
#define SSH_FXP_STAT               17
#define SSH_FXP_RENAME             18
#define SSH_FXP_READLINK           19
#define SSH_FXP_SYMLINK            20

#define SSH_FXP_STATUS            101
#define SSH_FXP_HANDLE            102
#define SSH_FXP_DATA              103
#define SSH_FXP_NAME              104
#define SSH_FXP_ATTRS             105
#define SSH_FXP_EXTENDED          200
#define SSH_FXP_EXTENDED_REPLY    201

#define SSH_FILEXFER_ATTR_SIZE          0x00000001
#define SSH_FILEXFER_ATTR_UIDGID        0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS   0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME     0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED      0x80000000

#define SSH_FILEXFER_TYPE_REGULAR          1
#define SSH_FILEXFER_TYPE_DIRECTORY        2
#define SSH_FILEXFER_TYPE_SYMLINK          3
#define SSH_FILEXFER_TYPE_SPECIAL          4
#define SSH_FILEXFER_TYPE_UNKNOWN          5

#define SSH_FXF_READ            0x00000001
#define SSH_FXF_WRITE           0x00000002
#define SSH_FXF_APPEND          0x00000004
#define SSH_FXF_CREAT           0x00000008
#define SSH_FXF_TRUNC           0x00000010
#define SSH_FXF_EXCL            0x00000020

#define SSH_FX_OK                            0
#define SSH_FX_EOF                           1
#define SSH_FX_NO_SUCH_FILE                  2
#define SSH_FX_PERMISSION_DENIED             3
#define SSH_FX_FAILURE                       4
#define SSH_FX_BAD_MESSAGE                   5
#define SSH_FX_NO_CONNECTION                 6
#define SSH_FX_CONNECTION_LOST               7
#define SSH_FX_OP_UNSUPPORTED                8

#define SSH_LOGIN_BUFSIZE	200
#define SSH_ERROR_BADPASS	-1
#define SSH_ERROR_QUESTION	-2


static char *
sshv2_initialize_string (gftp_request * request, size_t len)
{
  sshv2_params * params;
  guint32 num;
  char *ret;

  params = request->protocol_data;
  ret = g_malloc0 (len + 1);

  num = htonl (params->id++);
  memcpy (ret, &num, 4);

  return (ret);
}


static void
sshv2_add_string_to_buf (char *buf, const char *str)
{
  guint32 num;

  num = htonl (strlen (str));
  memcpy (buf, &num, 4);
  strcpy (buf + 4, str);
}


static char *
sshv2_initialize_string_with_path (gftp_request * request, const char *path,
                                   size_t *len, char **endpos)
{
  char *ret, *tempstr;
  size_t pathlen;

  if (*path == '/')
    {
      pathlen = strlen (path);
      *len += pathlen + 8;
      ret = sshv2_initialize_string (request, *len);
      sshv2_add_string_to_buf (ret + 4, path);
    }
  else
    {
      tempstr = gftp_build_path (request->directory, path, NULL);
      pathlen = strlen (tempstr);
      *len += pathlen + 8;
      ret = sshv2_initialize_string (request, *len);
      sshv2_add_string_to_buf (ret + 4, tempstr);
      g_free (tempstr);
    }

  if (endpos != NULL)
    *endpos = ret + 8 + pathlen;

  return (ret);
}


static void
sshv2_add_exec_args (char **logstr, size_t *logstr_len, char ***args, 
                     size_t *args_len, size_t *args_max, char *first, ...)
{
  char tempstr[2048], *curpos, *endpos, save_char;
  va_list argp;
  int at_end;

  va_start (argp, first);
  g_vsnprintf (tempstr, sizeof (tempstr), first, argp);
  va_end (argp);

  *logstr_len += strlen (tempstr);
  *logstr = g_realloc (*logstr, *logstr_len + 1);
  strcat (*logstr, tempstr);

  curpos = tempstr;
  while (*curpos == ' ')
    curpos++;

  save_char = ' ';
  at_end = 0;
  while (!at_end)
    {
      if (*curpos == '"')
        {
          curpos++;
          endpos = strchr (curpos + 1, '"');
        }
      else
        endpos = strchr (curpos, ' ');

      if (endpos == NULL)
        at_end = 1;
      else
        {
          save_char = *endpos;
          *endpos = '\0';
        }

      if (*args_max == *args_len + 1)
        {
          *args_max += 10;
          *args = g_realloc (*args, sizeof (char *) * *args_max);
        }

      (*args)[(*args_len)++] = g_strdup (curpos);
      (*args)[*args_len] = NULL;

      if (!at_end)
        {
          *endpos = save_char;
          curpos = endpos + 1;
          while (*curpos == ' ')
            curpos++;
        }
    }
}


static char **
sshv2_gen_exec_args (gftp_request * request)
{
  size_t logstr_len, args_len, args_cur;
  char **args, *tempstr, *logstr;

  gftp_lookup_request_option (request, "ssh_prog_name", &tempstr);
  if (tempstr == NULL || *tempstr == '\0')
    tempstr = "ssh";

  args_len = 1;
  args_cur = 15;
  args = g_malloc (sizeof (char *) * args_cur);
  args[0] = g_strdup (tempstr);

  logstr = g_strdup (args[0]);
  logstr_len = strlen (logstr);

  gftp_lookup_request_option (request, "ssh_extra_params", &tempstr);
  if (tempstr != NULL && *tempstr != '\0')
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                         "%s", tempstr);

  sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                       " -e none");

  if (request->username && *request->username != '\0')
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                         " -l %s", request->username);

  sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                       " -p %d", request->port);

  sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                       " %s -s sftp", request->hostname);

  request->logging_function (gftp_logging_misc, request, 
                             _("Running program %s\n"), logstr);
  g_free (logstr);
  return (args);
}


static int
sshv2_start_login_sequence (gftp_request * request, int fdm, int ptymfd)
{
  char *tempstr, *temp1str, *pwstr, *yesstr = "yes\n", *securid_pass;
  int wrotepw, ok, maxfd, ret, clear_tempstr;
  size_t rem, len, diff;
  fd_set rset, eset;
  ssize_t rd;

  rem = len = SSH_LOGIN_BUFSIZE;
  diff = 0;
  tempstr = g_malloc0 (len + 1);
  wrotepw = 0;
  ok = 1;

  if (gftp_fd_set_sockblocking (request, fdm, 1) == -1)
    return (GFTP_ERETRYABLE);

  if (gftp_fd_set_sockblocking (request, ptymfd, 1) == -1)
    return (GFTP_ERETRYABLE);

  if (request->password == NULL)
    pwstr = g_strdup ("\n");
  else
    pwstr = g_strconcat (request->password, "\n", NULL);

  FD_ZERO (&rset);
  FD_ZERO (&eset);
  maxfd = fdm > ptymfd ? fdm : ptymfd;

  errno = 0;
  while (1)
    {
      FD_SET (fdm, &rset);
      FD_SET (ptymfd, &rset);

      FD_SET (fdm, &eset);
      FD_SET (ptymfd, &eset);

      ret = select (maxfd + 1, &rset, NULL, &eset, NULL);
      if (ret < 0 && errno == EINTR)
        continue;

      if (ret < 0)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Connection to %s timed out\n"),
                                     request->hostname);
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      if (FD_ISSET (fdm, &eset) || FD_ISSET (ptymfd, &eset))
        {
          request->logging_function (gftp_logging_error, request,
                               _("Error: Could not read from socket: %s\n"),
                                g_strerror (errno));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }
        
      if (FD_ISSET (fdm, &rset))
        {
          ok = 1;
          break;
        }
      else if (!FD_ISSET (ptymfd, &rset))
        continue;

      rd = gftp_fd_read (request, tempstr + diff, rem - 1, ptymfd);
      if (rd < 0)
        return (rd);
      else if (rd == 0)
        continue;

      tempstr[diff + rd] = '\0'; 
      request->logging_function (gftp_logging_recv, request, "%s", tempstr + diff);
      rem -= rd;
      diff += rd;

      if (strcmp (tempstr, "Password:") == 0 || 
          (diff >= 10 && strcmp (tempstr + diff - 9, "assword: ") == 0) ||
          strstr (tempstr, "Enter passphrase for RSA key") != NULL ||
          strstr (tempstr, "Enter passphrase for key '") != NULL)
        {
          clear_tempstr = 1;
          if (wrotepw)
            {
              ok = SSH_ERROR_BADPASS;
              break;
            }

          wrotepw = 1;
          if (gftp_fd_write (request, pwstr, strlen (pwstr), ptymfd) < 0)
            {
              ok = 0;
              break;
            }
        }
      else if (diff > 10 && strcmp (tempstr + diff - 10, "(yes/no)? ") == 0)
        {
          clear_tempstr = 1;
          if (!gftpui_protocol_ask_yes_no (request, request->hostname, tempstr))
            {
              ok = SSH_ERROR_QUESTION;
              break;
            }
          else
            {
              if (gftp_fd_write (request, yesstr, strlen (yesstr), ptymfd) < 0)
                {
                  ok = 0;
                  break;
                }
            }
        }
      else if (strstr (tempstr, "Enter PASSCODE:") != NULL)
        {
          clear_tempstr = 1;
          securid_pass = gftpui_protocol_ask_user_input (request,
                                             _("Enter Password"),
                                             _("Enter SecurID Password:"), 0);

          if (securid_pass == NULL || *securid_pass == '\0')
            {
              ok = SSH_ERROR_BADPASS;
              break;
            }

          temp1str = g_strconcat (securid_pass, "\n", NULL);

          ret = gftp_fd_write (request, temp1str, strlen (temp1str), ptymfd);

          memset (temp1str, 0, strlen (temp1str));
          g_free (temp1str);
          memset (securid_pass, 0, strlen (securid_pass));
          g_free (securid_pass);

          if (ret <= 0)
            {
              ok = 0;
              break;
            }
        }
      else if (rem <= 1)
        {
          len += SSH_LOGIN_BUFSIZE;
          rem += SSH_LOGIN_BUFSIZE;
          tempstr = g_realloc (tempstr, len);
          continue;
        }
      else
        clear_tempstr = 0;

      if (clear_tempstr)
        {
          *tempstr = '\0';
          rem = SSH_LOGIN_BUFSIZE;
          diff = 0;
        }
    }

  g_free (pwstr);
  g_free (tempstr);

  if (ok <= 0)
    {
      request->logging_function (gftp_logging_error, request, "\n");

      if (ok == SSH_ERROR_BADPASS)
        request->logging_function (gftp_logging_error, request,
                               _("Error: An incorrect password was entered\n"));

      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }
 
  return (0);
}


static void
sshv2_log_command (gftp_request * request, gftp_logging_level level,
                   char type, char *message, size_t length)
{
  guint32 id, num, attr, stattype;
  char *descr, *pos, oldchar;
  sshv2_params * params;

  params = request->protocol_data;
  memcpy (&id, message, 4);
  id = ntohl (id);
  switch (type)
    {
      case SSH_FXP_DATA:
      case SSH_FXP_READ:
      case SSH_FXP_WRITE:
        break;  
      case SSH_FXP_INIT:
        request->logging_function (level, request, 
                                   _("%d: Protocol Initialization\n"), id);
        break;
      case SSH_FXP_VERSION:
        request->logging_function (level, request, 
                                   _("%d: Protocol version %d\n"), id, id);
        break;
      case SSH_FXP_OPEN:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        pos = message + 12 + num - 1;
        oldchar = *pos;
        *pos = '\0';
        request->logging_function (level, request,
                                   _("%d: Open %s\n"), id, message + 8);
        *pos = oldchar;
        break;
      case SSH_FXP_CLOSE:
        request->logging_function (level, request, 
                                   _("%d: Close\n"), id);
      case SSH_FXP_OPENDIR:
        request->logging_function (level, request, 
                                   _("%d: Open Directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_READDIR:
        request->logging_function (level, request, 
                                   _("%d: Read Directory\n"), id);
        break;
      case SSH_FXP_REMOVE:
        request->logging_function (level, request, 
                                   _("%d: Remove file %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_MKDIR:
        request->logging_function (level, request, 
                                   _("%d: Make directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_RMDIR:
        request->logging_function (level, request, 
                                   _("%d: Remove directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_REALPATH:
        request->logging_function (level, request, 
                                   _("%d: Realpath %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_ATTRS:
        request->logging_function (level, request,
                                   _("%d: File attributes\n"), id);
        break;
      case SSH_FXP_STAT:
        request->logging_function (level, request, 
                                   _("%d: Stat %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_SETSTAT:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);

        memcpy (&stattype, message + 8 + num, 4);
        stattype = ntohl (stattype);
        memcpy (&attr, message + 12 + num, 4);
        attr = ntohl (attr);

        pos = message + 8 + num;
        oldchar = *pos;
        *pos = '\0';

        switch (stattype)
          {
            case SSH_FILEXFER_ATTR_PERMISSIONS:
              request->logging_function (level, request,
                                         _("%d: Chmod %s %o\n"), id,
                                         message + 8, attr);
              break;
            case SSH_FILEXFER_ATTR_ACMODTIME:
              request->logging_function (level, request,
                                         _("%d: Utime %s %d\n"), id,
                                         message + 8, attr);
          }

        *pos = oldchar;
        break;
      case SSH_FXP_STATUS:
        if (params->dont_log_status)
          break;
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        switch (num)
          {
            case SSH_FX_OK:
              descr = _("OK");
              break;
            case SSH_FX_EOF:
              descr = _("EOF");
              break;
            case SSH_FX_NO_SUCH_FILE:
              descr = _("No such file or directory");
              break;
            case SSH_FX_PERMISSION_DENIED:
              descr = _("Permission denied");
              break;
            case SSH_FX_FAILURE:
              descr = _("Failure");
              break;
            case SSH_FX_BAD_MESSAGE:
              descr = _("Bad message");
              break;
            case SSH_FX_NO_CONNECTION:
              descr = _("No connection");
              break;
            case SSH_FX_CONNECTION_LOST:
              descr = _("Connection lost");
              break;
            case SSH_FX_OP_UNSUPPORTED:
              descr = _("Operation unsupported");
              break;
            default:
              descr = _("Unknown message returned from server");
              break;
          }
        request->logging_function (level, request,
                                   "%d: %s\n", id, descr);
        break;
      case SSH_FXP_HANDLE:
        request->logging_function (level, request, 
                                   "%d: File handle\n", id);
        break;
      case SSH_FXP_NAME:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        request->logging_function (level, request, 
                                   "%d: Filenames (%d entries)\n", id,
                                   num);
        break;
      default:
        request->logging_function (level, request, 
                                   "Command: %x\n", type);
    }
}


static int
sshv2_send_command (gftp_request * request, char type, char *command, 
                    size_t len)
{
  char buf[34000];
  guint32 clen;
  int ret;

  if (len > 33995)
    {
      request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big\n"), len);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  clen = htonl (len + 1);
  memcpy (buf, &clen, 4);
  buf[4] = type;
  memcpy (&buf[5], command, len);
  buf[len + 5] = '\0';

#ifdef DEBUG
  printf ("\rSending to FD %d: ", request->datafd);
  for (clen=0; clen<len + 5; clen++)
    printf ("%x ", buf[clen] & 0xff);
  printf ("\n");
#endif

  sshv2_log_command (request, gftp_logging_send, type, buf + 5, len);

  if ((ret = gftp_fd_write (request, buf, len + 5, request->datafd)) < 0)
    return (ret);

  return (0);
}


static int
sshv2_read_response (gftp_request * request, sshv2_message * message,
                     int fd)
{
  char buf[6], error_buffer[255], *pos;
  sshv2_params * params;
  ssize_t numread;
  size_t rem;

  params = request->protocol_data;

  if (fd <= 0)
    fd = request->datafd;

  pos = buf;
  rem = 5;
  while (rem > 0)
    {
      if ((numread = gftp_fd_read (request, pos, rem, fd)) < 0)
        return (numread);
      rem -= numread;
      pos += numread;
    }
  buf[5] = '\0';

  memcpy (&message->length, buf, 4);
  message->length = ntohl (message->length);
  if (message->length > 34000)
    {
      if (params->initialized)
        {
          request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big from server\n"),
                             message->length);
        }
      else
        {
          request->logging_function (gftp_logging_error, request,
                     _("There was an error initializing a SSH connection with the remote server. The error message from the remote server follows:\n"), buf);

          request->logging_function (gftp_logging_error, request, "%s", buf);

          if (gftp_fd_set_sockblocking (request, fd, 0) == -1)
            return (GFTP_EFATAL);

          if ((numread = gftp_fd_read (NULL, error_buffer, 
                                          sizeof (error_buffer) - 1, 
                                          fd)) > 0)
            {
              error_buffer[numread] = '\0';
              request->logging_function (gftp_logging_error, request,
                                         "%s", error_buffer);
            }
        }

      memset (message, 0, sizeof (*message));
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  message->command = buf[4];
  message->buffer = g_malloc (message->length + 1);

  message->pos = message->buffer;
  message->end = message->buffer + message->length - 1;

  pos = message->buffer;
  rem = message->length - 1;
  while (rem > 0)
    {
      if ((numread = gftp_fd_read (request, pos, rem, fd)) < 0)
        return (numread);
      rem -= numread;
      pos += numread;
    }

  message->buffer[message->length] = '\0';

  sshv2_log_command (request, gftp_logging_recv, message->command, 
                     message->buffer, message->length);
  
  return (message->command);
}


static void
sshv2_destroy (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_SSHV2_NUM);

  g_free (request->protocol_data);
  request->protocol_data = NULL;
}


static void
sshv2_message_free (sshv2_message * message)
{
  if (message->buffer)
    g_free (message->buffer);
  memset (message, 0, sizeof (*message));
}


static int
sshv2_wrong_response (gftp_request * request, sshv2_message * message)
{
  request->logging_function (gftp_logging_error, request,
                     _("Received wrong response from server, disconnecting\n"));
  gftp_disconnect (request);

  if (message != NULL)
    sshv2_message_free (message);

  return (GFTP_EFATAL);
}


static int
sshv2_read_status_response (gftp_request * request, sshv2_message * message,
                            int fd, int fxp_is_retryable, int fxp_is_wrong)
{
  int ret;

  memset (message, 0, sizeof (*message));
  ret = sshv2_read_response (request, message, -1);
  if (ret < 0)
    return (ret);
  else if (fxp_is_retryable > 0 && ret == fxp_is_retryable)
    {
      sshv2_message_free (message);
      return (GFTP_ERETRYABLE);
    }
  else if (fxp_is_wrong > 0 && ret != fxp_is_wrong)
    return (sshv2_wrong_response (request, message));

  return (ret);
}


static int
sshv2_buffer_get_int32 (gftp_request * request, sshv2_message * message,
                        int expected_response, guint32 * num)
{
  guint32 snum;

  if (message->end - message->pos < 4)
    return (sshv2_wrong_response (request, message));
      
  memcpy (&snum, message->pos, 4);
  snum = ntohl (snum);
  message->pos += 4;

  if (expected_response >= 0 && snum != expected_response)
    {
      switch (snum)
        {
          case SSH_FX_OK:
          case SSH_FX_EOF:
          case SSH_FX_NO_SUCH_FILE:
          case SSH_FX_PERMISSION_DENIED:
          case SSH_FX_FAILURE:
          case SSH_FX_OP_UNSUPPORTED:
            return (GFTP_ERETRYABLE);
          default:
            return (sshv2_wrong_response (request, message));
        }
    }

  if (num != NULL)
    *num = snum;

  return (0);
}


static char *
sshv2_buffer_get_string (gftp_request * request, sshv2_message * message)
{
  char *string;
  guint32 len;

  if (sshv2_buffer_get_int32 (request, message, -1, &len) < 0)
    return (NULL);

  if (len > SSH_MAX_STRING_SIZE || (message->end - message->pos < len))
    {
      sshv2_wrong_response (request, message);
      return (NULL);
    }

  string = g_malloc (len + 1);
  memcpy (string, message->pos, len);
  string[len] = '\0';
  message->pos += len;
  return (string);
}


static int
sshv2_getcwd (gftp_request * request)
{
  sshv2_message message;
  char *tempstr, *dir;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  if (request->directory == NULL || *request->directory == '\0')
    dir = ".";
  else
    dir = request->directory;

  len = strlen (dir) + 8;
  tempstr = sshv2_initialize_string (request, len);
  sshv2_add_string_to_buf (tempstr + 4, dir);

  ret = sshv2_send_command (request, SSH_FXP_REALPATH, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  if (request->directory)
    {
      g_free (request->directory);
      request->directory = NULL;
    }

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_NAME);
  if (ret < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, 1, NULL)) < 0)
    return (ret);

  if ((request->directory = sshv2_buffer_get_string (request, &message)) == NULL)
    return (GFTP_EFATAL);

  sshv2_message_free (&message);
  return (0);
}


static void
sshv2_free_args (char **args)
{
  int i;

  for (i=0; args[i] != NULL; i++)
    g_free (args[i]);
  g_free (args);
}


static int
sshv2_connect (gftp_request * request)
{
  struct servent serv_struct;
  sshv2_params * params;
  sshv2_message message;
  int ret, fdm, ptymfd;
  guint32 version;
  char **args;
  pid_t child;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);
  
  if (request->datafd > 0)
    return (0);

  params = request->protocol_data;

  request->logging_function (gftp_logging_misc, request,
			     _("Opening SSH connection to %s\n"),
                             request->hostname);

  if (request->port == 0)
    {
      if (!r_getservbyname ("ssh", "tcp", &serv_struct, NULL))
        {
         request->logging_function (gftp_logging_error, request,
                                    _("Cannot look up service name %s/tcp. Please check your services file\n"),
                                    "ssh");
        }
      else
        request->port = ntohs (serv_struct.s_port);
    }

  args = sshv2_gen_exec_args (request);

  child = gftp_exec (request, &fdm, &ptymfd, args);

  if (child == 0)
    exit (0);

  sshv2_free_args (args);

  if (child < 0)
    return (GFTP_ERETRYABLE);

  request->datafd = fdm;

  version = htonl (SSH_MY_VERSION);
  if ((ret = sshv2_send_command (request, SSH_FXP_INIT, (char *) 
                                 &version, 4)) < 0)
    return (ret);

  ret = sshv2_start_login_sequence (request, fdm, ptymfd);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    {
      sshv2_message_free (&message);
      return (ret);
    }
  else if (ret != SSH_FXP_VERSION)
    return (sshv2_wrong_response (request, &message));

  sshv2_message_free (&message);

  params->initialized = 1;
  request->logging_function (gftp_logging_misc, request,
                             _("Successfully logged into SSH server %s\n"),
                             request->hostname);

  if (sshv2_getcwd (request) < 0)
    {
      if (request->directory)
        g_free (request->directory);

      request->directory = g_strdup (".");
      if ((ret = sshv2_getcwd (request)) < 0)
        {
          gftp_disconnect (request);
          return (ret);
        }
    }

  return (0);
}


static void
sshv2_disconnect (gftp_request * request)
{
  sshv2_params * params;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_SSHV2_NUM);

  params = request->protocol_data;

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

  if (params->message.buffer != NULL)
    {
      sshv2_message_free (&params->message);
      params->message.buffer = NULL;
    }
}


static int
sshv2_end_transfer (gftp_request * request)
{
  sshv2_params * params;
  sshv2_message message;
  guint32 len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  params = request->protocol_data;
  if (params->message.buffer != NULL)
    {
      sshv2_message_free (&params->message);
      params->message.buffer = NULL;
      params->count = 0;
    }

  if (params->handle_len > 0)
    {
      len = htonl (params->id++);
      memcpy (params->handle, &len, 4);

      if ((ret = sshv2_send_command (request, SSH_FXP_CLOSE, params->handle, 
                                     params->handle_len)) < 0)
        return (ret);

      ret = sshv2_read_status_response (request, &message, -1, 0,
                                         SSH_FXP_STATUS);
      if (ret < 0)
        return (ret);

      sshv2_message_free (&message);
      params->handle_len = 0;
    }  

  if (params->read_buffer != NULL)
    {
      g_free (params->read_buffer);
      params->read_buffer = NULL;
    }

  return (0);
}


static int
sshv2_list_files (gftp_request * request)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  params = request->protocol_data;

  request->logging_function (gftp_logging_misc, request,
			     _("Retrieving directory listing...\n"));

  len = strlen (request->directory) + 8;
  tempstr = sshv2_initialize_string (request, len);
  sshv2_add_string_to_buf (tempstr + 4, request->directory);

  ret = sshv2_send_command (request, SSH_FXP_OPENDIR, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_HANDLE);
  if (ret < 0)
    return (ret);

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big from server\n"),
                             message.length - 4);
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  memset (params->handle, 0, 4);
  memcpy (params->handle + 4, message.buffer + 4, message.length - 5);
  params->handle_len = message.length - 1;
  sshv2_message_free (&message);
  params->count = 0;
  return (0);
}


static int
sshv2_decode_file_attributes (gftp_request * request, sshv2_message * message,
                              gftp_file * fle)
{
  guint32 attrs, hinum, num, count, i;
  int ret;

  if ((ret = sshv2_buffer_get_int32 (request, message, -1, &attrs)) < 0)
    return (ret);

  if (attrs & SSH_FILEXFER_ATTR_SIZE)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, -1, &hinum)) < 0)
        return (ret);

      if ((ret = sshv2_buffer_get_int32 (request, message, -1, &num)) < 0)
        return (ret);

#if G_HAVE_GINT64
      fle->size = (gint64) num | ((gint64) hinum >> 32);
#else
      fle->size = num;
#endif
    }

  if (attrs & SSH_FILEXFER_ATTR_UIDGID)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, -1, NULL)) < 0)
        return (ret);

      if ((ret = sshv2_buffer_get_int32 (request, message, -1, NULL)) < 0)
        return (ret);
    }

  if (attrs & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, -1, &num)) < 0)
        return (ret);

      fle->st_mode = num;
    }

  if (attrs & SSH_FILEXFER_ATTR_ACMODTIME)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, -1, NULL)) < 0)
        return (num);

      if ((ret = sshv2_buffer_get_int32 (request, message, -1, &num)) < 0)
        return (ret);

      fle->datetime = num;
    }

  if (attrs & SSH_FILEXFER_ATTR_EXTENDED)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, -1, &count)) < 0)
        return (ret);

      for (i=0; i<count; i++)
        {
          if ((ret = sshv2_buffer_get_int32 (request, message, -1, &num)) < 0 ||
              message->pos + num + 4 > message->end)
            return (GFTP_EFATAL);

          message->pos += num + 4;

          if ((ret = sshv2_buffer_get_int32 (request, message, -1, &num)) < 0 ||
              message->pos + num + 4 > message->end)
            return (GFTP_EFATAL);
       
          message->pos += num + 4;
        }
    }

  return (0);
}


static int
sshv2_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  guint32 len, longnamelen;
  int ret, retsize, iret;
  sshv2_params *params;
  char *longname;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
      request->last_dir_entry_len = 0;
    }
  retsize = 0;

  if (params->count > 0)
    ret = SSH_FXP_NAME;
  else
    {
      if (!request->cached)
        {
          if (params->message.buffer != NULL)
            sshv2_message_free (&params->message);

          len = htonl (params->id++);
          memcpy (params->handle, &len, 4);

          if ((ret = sshv2_send_command (request, SSH_FXP_READDIR,  
                                         params->handle,
                                         params->handle_len)) < 0)
            return (ret);
        }

      if ((ret = sshv2_read_response (request, &params->message, fd)) < 0)
        return (ret);

      if (!request->cached)
        {
          request->last_dir_entry = g_malloc (params->message.length + 4);
          len = htonl (params->message.length);
          memcpy (request->last_dir_entry, &len, 4);
          request->last_dir_entry[4] = params->message.command;
          memcpy (request->last_dir_entry + 5, params->message.buffer, 
                  params->message.length - 1);
          request->last_dir_entry_len = params->message.length + 4;
        }

      if (ret == SSH_FXP_NAME)
        {
          params->message.pos = params->message.buffer + 4;
          if ((iret = sshv2_buffer_get_int32 (request, 
                                              &params->message, -1,
                                              &params->count)) < 0)
            return (iret);
        }
    }

  if (ret == SSH_FXP_NAME)
    {
      if (sshv2_buffer_get_int32 (request, &params->message, -1, &len) < 0 ||
          params->message.pos + len > params->message.end)
        return (GFTP_EFATAL);

      params->message.pos += len;

      if (sshv2_buffer_get_int32 (request, &params->message, -1,
                                  &longnamelen) < 0 || 
           params->message.pos + longnamelen > params->message.end)
        return (GFTP_EFATAL);

      longname = params->message.pos;
      longname[longnamelen] = '\0';

      /* The commercial SSH2 puts a / and * after some entries */
      if (longname[longnamelen - 1] == '*')
        longname[--longnamelen] = '\0';
      if (longname[longnamelen - 1] == '/')
        longname[--longnamelen] = '\0';

      params->message.pos += longnamelen;

      if ((ret = gftp_parse_ls (request, longname, fle, 0)) < 0)
        {
          gftp_file_destroy (fle);
          return (ret);
        }

      if ((ret = sshv2_decode_file_attributes (request, &params->message,
                                               fle)) < 0)
        {
          gftp_file_destroy (fle);
          return (ret);
        }

      retsize = strlen (longname);
      params->count--;
    }
  else if (ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&params->message);
      return (0);
    }
  else
    return (sshv2_wrong_response (request, &params->message));

  return (retsize);
}


static int
sshv2_chdir (gftp_request * request, const char *directory)
{
  sshv2_message message;
  char *tempstr, *dir;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  if (request->directory != directory)
    {
      len = 0;
      tempstr = sshv2_initialize_string_with_path (request, directory,
                                                   &len, NULL);

      ret = sshv2_send_command (request, SSH_FXP_REALPATH, tempstr, len);

      g_free (tempstr);
      if (ret < 0)
        return (ret);

      ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                        SSH_FXP_NAME);
      if (ret < 0)
        return (ret);

      message.pos += 4;
      if ((ret = sshv2_buffer_get_int32 (request, &message, 1, NULL)) < 0)
        return (ret);

      if ((dir = sshv2_buffer_get_string (request, &message)) == NULL)
        return (GFTP_EFATAL);

      if (request->directory)
        g_free (request->directory);

      request->directory = dir;
      sshv2_message_free (&message);
      return (0);
    }

  return (0);
}


static int 
sshv2_rmdir (gftp_request * request, const char *directory)
{
  sshv2_message message;
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, directory, &len, NULL);

  ret = sshv2_send_command (request, SSH_FXP_RMDIR, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_rmfile (gftp_request * request, const char *file)
{
  sshv2_message message;
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, file, &len, NULL);

  ret = sshv2_send_command (request, SSH_FXP_REMOVE, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_chmod (gftp_request * request, const char *file, mode_t mode)
{
  char *tempstr, *endpos;
  sshv2_message message;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 8; /* For attributes */
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (SSH_FILEXFER_ATTR_PERMISSIONS);
  memcpy (endpos, &num, 4);

  num = htonl (mode);
  memcpy (endpos + 4, &num, 4);

  ret = sshv2_send_command (request, SSH_FXP_SETSTAT, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_mkdir (gftp_request * request, const char *newdir)
{
  sshv2_message message;
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (newdir != NULL, GFTP_EFATAL);

  len = 4; /* For attributes */
  tempstr = sshv2_initialize_string_with_path (request, newdir, &len, NULL);

  /* No need to set attributes since all characters of the tempstr buffer is
     initialized to 0 */

  ret = sshv2_send_command (request, SSH_FXP_MKDIR, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_rename (gftp_request * request, const char *oldname, const char *newname)
{
  char *tempstr, *oldstr, *newstr;
  size_t oldlen, newlen, len;
  sshv2_message message;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  if (*oldname == '/')
    oldstr = g_strdup (oldname);
  else
    oldstr = gftp_build_path (request->directory, oldname, NULL);

  if (*newname == '/')
    newstr = g_strdup (newname);
  else
    newstr = gftp_build_path (request->directory, newname, NULL);

  oldlen = strlen (oldstr); 
  newlen = strlen (newname); 

  len = oldlen + newlen + 12;
  tempstr = sshv2_initialize_string (request, len);
  sshv2_add_string_to_buf (tempstr + 4, oldstr);
  sshv2_add_string_to_buf (tempstr + 8 + oldlen, newstr);

  g_free (oldstr);
  g_free (newstr);

  ret = sshv2_send_command (request, SSH_FXP_RENAME, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  char *tempstr, *endpos;
  sshv2_message message;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 12; /* For date/time */
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (SSH_FILEXFER_ATTR_ACMODTIME);
  memcpy (endpos, &num, 4);

  num = htonl (datetime);
  memcpy (endpos + 4, &num, 4);
  memcpy (endpos + 8, &num, 4);

  ret = sshv2_send_command (request, SSH_FXP_SETSTAT, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int
sshv2_send_stat_command (gftp_request * request, const char *filename,
                         gftp_file * fle)
{
  sshv2_message message;
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, filename, &len, NULL);

  ret = sshv2_send_command (request, SSH_FXP_STAT, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_ATTRS);
  if (ret < 0)
    return (ret);

  if (message.length < 5)
    return (GFTP_EFATAL);

  message.pos += 4;
  if ((ret = sshv2_decode_file_attributes (request, &message, fle)) < 0)
    {
      gftp_file_destroy (fle);
      return (ret);
    }

  sshv2_message_free (&message);

  return (0);
}


static int
sshv2_stat_filename (gftp_request * request, const char *filename,
                     mode_t * mode)
{
  gftp_file fle;
  int ret;

  memset (&fle, 0, sizeof (fle));
  ret = sshv2_send_stat_command (request, filename, &fle);
  if (ret < 0)
    return (ret);

  *mode = fle.st_mode;
  gftp_file_destroy (&fle);

  return (0);
}


static off_t
sshv2_get_file_size (gftp_request * request, const char *file)
{
  gftp_file fle;
  off_t size;
  int ret;

  memset (&fle, 0, sizeof (fle));
  ret = sshv2_send_stat_command (request, file, &fle);
  if (ret < 0)
    return (ret);

  size = fle.size;
  gftp_file_destroy (&fle);

  return (size);
}


static off_t
sshv2_get_file (gftp_request * request, const char *file, int fd,
                off_t startsize)
{
  char *tempstr, *endpos;
  sshv2_params * params;
  sshv2_message message;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->offset = startsize;

  len = 8;
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (SSH_FXF_READ);
  memcpy (endpos, &num, 4);

  ret = sshv2_send_command (request, SSH_FXP_OPEN, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_HANDLE);
  if (ret < 0)
    return (ret);

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big from server\n"),
                             message.length - 4);
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
        
    }

  memset (params->handle, 0, 4);
  memcpy (params->handle + 4, message.buffer+ 4, message.length - 5);
  params->handle_len = message.length - 1;
  sshv2_message_free (&message);

  return (sshv2_get_file_size (request, file));
}


static int
sshv2_put_file (gftp_request * request, const char *file, int fd,
                off_t startsize, off_t totalsize)
{
  char *tempstr, *endpos;
  sshv2_params * params;
  sshv2_message message;
  size_t len, num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->offset = startsize;

  len = 8;
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  if (startsize > 0)
    num = htonl (SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_APPEND);
  else
    num = htonl (SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC);
  memcpy (endpos, &num, 4);

  ret = sshv2_send_command (request, SSH_FXP_OPEN, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_HANDLE);
  if (ret < 0)
    return (ret);

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big from server\n"),
                             message.length - 4);
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
        
    }

  memset (params->handle, 0, 4);
  memcpy (params->handle + 4, message.buffer + 4, message.length - 5);
  params->handle_len = message.length - 1;
  sshv2_message_free (&message);

  return (0);
}


#ifdef G_HAVE_GINT64

static gint64
sshv2_hton64 (gint64 val)
{
#if G_BYTE_ORDER != G_BIG_ENDIAN
  return (GINT64_TO_BE (val));
#else
  return (val);
#endif
}

#endif


static void
sshv2_setup_file_offset (sshv2_params * params, char *buf)
{
  guint32 hinum, lownum;
#ifdef G_HAVE_GINT64
  gint64 offset;

  offset = sshv2_hton64 (params->offset);
  lownum = offset >> 32;
  hinum = (guint32) offset;
#else
  hinum = 0;
  lownum = htonl (params->offset);
#endif

  memcpy (buf + params->handle_len, &hinum, 4);
  memcpy (buf + params->handle_len + 4, &lownum, 4);
}


static ssize_t 
sshv2_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  sshv2_params * params;
  sshv2_message message;
  guint32 num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (params->read_buffer == NULL)
    {
      params->read_buffer = g_malloc (params->handle_len + 12);
      num = htonl (params->handle_len);
      memcpy (params->read_buffer, params->handle, params->handle_len);
    }

  num = htonl (params->id++);
  memcpy (params->read_buffer, &num, 4);

  sshv2_setup_file_offset (params, params->read_buffer);

  num = htonl (size);
  memcpy (params->read_buffer + params->handle_len + 8, &num, 4);
  
  if (sshv2_send_command (request, SSH_FXP_READ, params->read_buffer,
                          params->handle_len + 12) < 0)
    return (GFTP_ERETRYABLE);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) != SSH_FXP_DATA)
    {
      if (ret < 0)
        return (ret);

      message.pos += 4;
      if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_EOF,
                                         NULL)) < 0)
        return (ret); 

      sshv2_message_free (&message);
      return (0);
    }

  memcpy (&num, message.buffer + 4, 4);
  num = ntohl (num);
  if (num > size)
    {
      request->logging_function (gftp_logging_error, request,
                             _("Error: Message size %d too big from server\n"),
                             num);
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
        
    }

  memcpy (buf, message.buffer + 8, num);
  sshv2_message_free (&message);
  params->offset += num;
  return (num);
}


static ssize_t 
sshv2_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  sshv2_params * params;
  sshv2_message message;
  char tempstr[32768];
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);
  g_return_val_if_fail (size <= 32500, GFTP_EFATAL);

  params = request->protocol_data;

  memcpy (tempstr, params->handle, params->handle_len);

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  sshv2_setup_file_offset (params, tempstr);
 
  num = htonl (size);
  memcpy (tempstr + params->handle_len + 8, &num, 4);
  memcpy (tempstr + params->handle_len + 12, buf, size);
  
  len = params->handle_len + size + 12;
  if (sshv2_send_command (request, SSH_FXP_WRITE, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }

  memset (&message, 0, sizeof (message));
  params->dont_log_status = 1;
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);

  params->dont_log_status = 0;
  if (ret != SSH_FXP_STATUS)
    return (sshv2_wrong_response (request, &message));

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, &num)) < 0)
    {
      if (num == SSH_FX_EOF)
        return (0);

      return (ret);
    }

  sshv2_message_free (&message);
  params->offset += size;
  return (size);
}


static int
sshv2_set_config_options (gftp_request * request)
{
  intptr_t ssh_need_userpass;

  gftp_lookup_request_option (request, "ssh_need_userpass", &ssh_need_userpass);
  request->need_userpass = ssh_need_userpass;
  return (0);
}


static void
sshv2_swap_socks (gftp_request * dest, gftp_request * source)
{
  sshv2_params * sparams, * dparams;

  sparams = source->protocol_data;
  dparams = dest->protocol_data;
  dparams->id = sparams->id;
}


void 
sshv2_register_module (void)
{
  gftp_register_config_vars (config_vars);
}


static void
sshv2_copy_message (sshv2_message * src_message, sshv2_message * dest_message)
{
  dest_message->length = src_message->length;
  dest_message->command = src_message->command;
  dest_message->buffer = g_strdup (src_message->buffer);
  dest_message->pos = dest_message->buffer + (src_message->pos - src_message->buffer);
  dest_message->end = dest_message->buffer + (src_message->end - src_message->buffer);
}


static void
sshv2_copy_param_options (gftp_request * dest_request,
                          gftp_request * src_request)
{ 
  sshv2_params * dparms, * sparms;
  
  dparms = dest_request->protocol_data;
  sparms = src_request->protocol_data;

  memcpy (dparms->handle, sparms->handle, sizeof (*dparms->handle));
  if (sparms->read_buffer)
    dparms->read_buffer = g_strdup (sparms->read_buffer);
  else
    dparms->read_buffer = NULL;

  sshv2_copy_message (&sparms->message, &dparms->message);

  dparms->id = sparms->id;
  dparms->count = sparms->count;
  dparms->handle_len = sparms->handle_len;
  dparms->initialized = sparms->initialized;
  dparms->dont_log_status = sparms->dont_log_status;
  dparms->offset = sparms->offset;
}


int
sshv2_init (gftp_request * request)
{
  sshv2_params * params;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_SSHV2_NUM;
  request->init = sshv2_init;
  request->copy_param_options = sshv2_copy_param_options;
  request->destroy = sshv2_destroy;
  request->read_function = gftp_fd_read;
  request->write_function = gftp_fd_write;
  request->connect = sshv2_connect;
  request->post_connect = NULL;
  request->disconnect = sshv2_disconnect;
  request->get_file = sshv2_get_file;
  request->put_file = sshv2_put_file;
  request->transfer_file = NULL;
  request->get_next_file_chunk = sshv2_get_next_file_chunk;
  request->put_next_file_chunk = sshv2_put_next_file_chunk;
  request->end_transfer = sshv2_end_transfer;
  request->abort_transfer = sshv2_end_transfer; /* NOTE: uses sshv2_end_transfer */
  request->stat_filename = sshv2_stat_filename;
  request->list_files = sshv2_list_files;
  request->get_next_file = sshv2_get_next_file;
  request->get_next_dirlist_line = NULL;
  request->get_file_size = sshv2_get_file_size;
  request->chdir = sshv2_chdir;
  request->rmdir = sshv2_rmdir;
  request->rmfile = sshv2_rmfile;
  request->mkdir = sshv2_mkdir;
  request->rename = sshv2_rename;
  request->chmod = sshv2_chmod;
  request->set_file_time = sshv2_set_file_time;
  request->site = NULL;
  request->parse_url = NULL;
  request->set_config_options = sshv2_set_config_options;
  request->swap_socks = sshv2_swap_socks;
  request->url_prefix = "ssh2";
  request->need_hostport = 1;
  request->need_userpass = 1;
  request->use_cache = 1;
  request->always_connected = 0;
  request->protocol_data = g_malloc0 (sizeof (sshv2_params));
  request->server_type = GFTP_DIRTYPE_UNIX;

  params = request->protocol_data;
  params->id = 1;

  return (gftp_set_config_options (request));
}

