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
  {"", N_("SSH"), gftp_option_type_notebook, NULL, NULL, 0, NULL, 
   GFTP_PORT_GTK, NULL},

  {"ssh_prog_name", N_("SSH Prog Name:"), 
   gftp_option_type_text, "ssh", NULL, 0,
   N_("The path to the SSH executable"), GFTP_PORT_ALL, NULL},
  {"ssh_extra_params", N_("SSH Extra Params:"), 
   gftp_option_type_text, NULL, NULL, 0,  
   N_("Extra parameters to pass to the SSH program"), GFTP_PORT_ALL, NULL},
  {"ssh2_sftp_path", N_("SSH2 sftp-server path:"), 
   gftp_option_type_text, NULL, NULL, 0,
   N_("Default remote SSH2 sftp-server path"), GFTP_PORT_ALL, NULL},

  {"ssh_need_userpass", N_("Need SSH User/Pass"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 0,
   N_("Require a username/password for SSH connections"), GFTP_PORT_ALL, NULL},
  {"ssh_use_askpass", N_("Use ssh-askpass utility"),
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 0,
   N_("Use the ssh-askpass utility to supply the remote password"), GFTP_PORT_GTK,
   NULL},
  {"sshv2_use_sftp_subsys", N_("Use SSH2 SFTP subsys"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 0,
   N_("Call ssh with the -s sftp flag. This is helpful because you won't have to know the remote path to the remote sftp-server"), 
   GFTP_PORT_GTK, NULL},
  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};


typedef struct sshv2_attribs_tag
{
  gint32 flags;
  gint64 size;
  gint32 uid;
  gint32 gid;
  gint32 perm;
  gint32 atime;
  gint32 mtime;
} sshv2_attribs;

typedef struct sshv2_message_tag
{
  gint32 length;
  char command;
  char *buffer,
       *pos,
       *end;
} sshv2_message;

typedef struct sshv2_params_tag
{
  char handle[SSH_MAX_HANDLE_SIZE + 4]; /* We'll encode the ID in here too */
  int handle_len,
      dont_log_status : 1;              /* For uploading files */
  sshv2_message message;

  gint32 id,				
         count;
#ifdef G_HAVE_GINT64
  gint64 offset;
#else
  gint32 offset;
#endif
  char *read_buffer;
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
#define SSH_WARNING 		-3

static void
sshv2_add_exec_args (char **logstr, size_t *logstr_len, char ***args, 
                     size_t *args_len, size_t *args_cur, char *first, ...)
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

      if (*args_cur == *args_len + 1)
        {
          *args_cur += 10;
          *args = g_realloc (*args, sizeof (char *) * *args_cur);
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
sshv2_gen_exec_args (gftp_request * request, char *execname, 
                    int use_sftp_subsys)
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

  if (use_sftp_subsys)
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                         " %s -s sftp", request->hostname);
  else
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                         " %s \"%s\"", request->hostname, execname);

  request->logging_function (gftp_logging_misc, request->user_data, 
                             _("Running program %s\n"), logstr);
  g_free (logstr);
  return (args);
}


static char *
sshv2_start_login_sequence (gftp_request * request, int fd)
{
  char *tempstr, *pwstr, *tmppos;
  size_t rem, len, diff, lastdiff, key_pos;
  int wrotepw, ok;
  ssize_t rd;

  rem = len = SSH_LOGIN_BUFSIZE;
  tempstr = g_malloc0 (len + 1);
  key_pos = diff = lastdiff = 0;
  wrotepw = 0;
  ok = 1;

  if (gftp_fd_set_sockblocking (request, fd, 1) == -1)
    return (NULL);

  pwstr = g_strconcat (request->password, "\n", NULL);

  errno = 0;
  while (1)
    {
      if ((rd = gftp_fd_read (request, tempstr + diff, rem - 1, fd)) <= 0)
        {
          ok = 0;
          break;
        }
      rem -= rd;
      diff += rd;
      tempstr[diff] = '\0'; 

      if (diff >= 10 && strcmp (tempstr + diff - 9, "assword: ") == 0)
        {
          if (wrotepw)
            {
              ok = SSH_ERROR_BADPASS;
              break;
            }

          if (strstr (tempstr, "WARNING") != NULL ||
              strstr (tempstr, _("WARNING")) != NULL)
            {
              ok = SSH_WARNING;
              break;
            }
              
          wrotepw = 1;
          if (gftp_fd_write (request, pwstr, strlen (pwstr), fd) < 0)
            {
              ok = 0;
              break;
            }
        }
      else if (diff > 2 && strcmp (tempstr + diff - 2, ": ") == 0 &&
               ((tmppos = strstr (tempstr + key_pos, "Enter passphrase for RSA key")) != NULL ||
                ((tmppos = strstr (tempstr + key_pos, "Enter passphrase for key '")) != NULL)))
        {
          key_pos = diff;
          if (wrotepw)
            {
              ok = SSH_ERROR_BADPASS;
              break;
            }

          if (strstr (tempstr, "WARNING") != NULL ||
              strstr (tempstr, _("WARNING")) != NULL)
            {
              ok = SSH_WARNING;
              break;
            }

          wrotepw = 1;
          if (gftp_fd_write (request, pwstr, strlen (pwstr), fd) < 0)
            {
              ok = 0;
              break;
            }
        }
      else if (diff > 10 && strcmp (tempstr + diff - 10, "(yes/no)? ") == 0)
        {
          ok = SSH_ERROR_QUESTION;
          break;
        }
      else if (diff >= 5 && strcmp (tempstr + diff - 5, "xsftp") == 0)
        break;
      else if (rem <= 1)
        {
          request->logging_function (gftp_logging_recv, request->user_data,
                                     "%s", tempstr + lastdiff);
          len += SSH_LOGIN_BUFSIZE;
          rem += SSH_LOGIN_BUFSIZE;
          lastdiff = diff;
          tempstr = g_realloc (tempstr, len);
          continue;
        }
    }

  g_free (pwstr);

  if (*(tempstr + lastdiff) != '\0')
    request->logging_function (gftp_logging_recv, request->user_data,
                               "%s\n", tempstr + lastdiff);

  if (ok <= 0)
    {
      if (ok == SSH_ERROR_BADPASS)
        request->logging_function (gftp_logging_error, request->user_data,
                               _("Error: An incorrect password was entered\n"));
      else if (ok == SSH_ERROR_QUESTION)
        request->logging_function (gftp_logging_error, request->user_data,
                               _("Please connect to this host with the command line SSH utility and answer this question appropriately.\n"));
      else if (ok == SSH_WARNING)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Please correct the above warning to connect to this host.\n"));

      g_free (tempstr);
      return (NULL);
    }
 
  return (tempstr);
}


#ifdef G_HAVE_GINT64

static gint64
hton64 (gint64 val)
{
  if (G_BYTE_ORDER != G_BIG_ENDIAN)
    return (GINT64_TO_BE (val));
  else
    return (val);
}

#endif


static void
sshv2_log_command (gftp_request * request, gftp_logging_level level,
                   char type, char *message, size_t length)
{
  gint32 id, num, attr, stattype;
  char *descr, *pos, oldchar;
  sshv2_params * params;

  params = request->protocol_data;
  memcpy (&id, message, 4);
  id = ntohl (id);
  switch (type)
    {
      case SSH_FXP_INIT:
        request->logging_function (level, request->user_data, 
                                   _("%d: Protocol Initialization\n"), id);
        break;
      case SSH_FXP_VERSION:
        memcpy (&num, message, 4);
        num = ntohl (num);
        request->logging_function (level, request->user_data, 
                                   _("%d: Protocol version %d\n"), id, num);
        break;
      case SSH_FXP_OPEN:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        pos = message + 12 + num - 1;
        oldchar = *pos;
        *pos = '\0';
        request->logging_function (level, request->user_data,
                                   _("%d: Open %s\n"), id, message + 8);
        *pos = oldchar;
        break;
      case SSH_FXP_CLOSE:
        request->logging_function (level, request->user_data, 
                                   _("%d: Close\n"), id);
      case SSH_FXP_READ:
      case SSH_FXP_WRITE:
        break;  
      case SSH_FXP_OPENDIR:
        request->logging_function (level, request->user_data, 
                                   _("%d: Open Directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_READDIR:
        request->logging_function (level, request->user_data, 
                                   _("%d: Read Directory\n"), id);
        break;
      case SSH_FXP_REMOVE:
        request->logging_function (level, request->user_data, 
                                   _("%d: Remove file %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_MKDIR:
        request->logging_function (level, request->user_data, 
                                   _("%d: Make directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_RMDIR:
        request->logging_function (level, request->user_data, 
                                   _("%d: Remove directory %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_REALPATH:
        request->logging_function (level, request->user_data, 
                                   _("%d: Realpath %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_ATTRS:
        request->logging_function (level, request->user_data,
                                   _("%d: File attributes\n"), id);
        break;
      case SSH_FXP_STAT:
        request->logging_function (level, request->user_data, 
                                   _("%d: Stat %s\n"), id,
                                   message + 8);
        break;
      case SSH_FXP_SETSTAT:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        pos = message + 12 + num - 1;
        oldchar = *pos;
        *pos = '\0';
        memcpy (&stattype, message + 8 + num, 4);
        stattype = ntohl (stattype);
        memcpy (&attr, message + 12 + num, 4);
        attr = ntohl (attr);
        switch (stattype)
          {
            case SSH_FILEXFER_ATTR_PERMISSIONS:
              request->logging_function (level, request->user_data,
                                         _("%d: Chmod %s %o\n"), id,
                                         message + 8, attr);
              break;
            case SSH_FILEXFER_ATTR_ACMODTIME:
              request->logging_function (level, request->user_data,
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
        request->logging_function (level, request->user_data,
                                   "%d: %s\n", id, descr);
        break;
      case SSH_FXP_HANDLE:
        request->logging_function (level, request->user_data, 
                                   "%d: File handle\n", id);
        break;
      case SSH_FXP_DATA:
        break;
      case SSH_FXP_NAME:
        memcpy (&num, message + 4, 4);
        num = ntohl (num);
        request->logging_function (level, request->user_data, 
                                   "%d: Filenames (%d entries)\n", id,
                                   num);
        break;
      default:
        request->logging_function (level, request->user_data, 
                                   "Command: %x\n", type);
    }
}


static int
sshv2_send_command (gftp_request * request, char type, char *command, 
                    gint32 len)
{
  char buf[34000];
  gint32 clen;
  int ret;

  if (len > 33995)
    {
      request->logging_function (gftp_logging_error, request->user_data,
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
  printf ("\rSending: ");
  for (clen=0; clen<len + 5; clen++)
    printf ("%x ", buf[clen] & 0xff);
  printf ("\n");
#endif

  sshv2_log_command (request, gftp_logging_send, type, buf + 5, len);

  if ((ret = gftp_fd_write (request, buf, len + 5, request->sockfd)) < 0)
    return (ret);

  return (0);
}


static int
sshv2_read_response (gftp_request * request, sshv2_message * message,
                     int fd)
{
  ssize_t numread, rem;
  char buf[5], *pos;

  if (fd <= 0)
    fd = request->sockfd;

  pos = buf;
  rem = 5;
  while (rem > 0)
    {
      if ((numread = gftp_fd_read (request, pos, rem, fd)) < 0)
        return ((int) numread);
      rem -= numread;
      pos += numread;
    }

  memcpy (&message->length, buf, 4);
  message->length = ntohl (message->length);
  if (message->length > 34000)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                             _("Error: Message size %d too big from server\n"),
                             message->length);
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
        return ((int) numread);
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


static gint32
sshv2_buffer_get_int32 (gftp_request * request, sshv2_message * message,
                        int expected_response)
{
  gint32 ret;

  if (message->end - message->pos < 4)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  memcpy (&ret, message->pos, 4);
  ret = ntohl (ret);
  message->pos += 4;

  if (expected_response > 0 && ret != expected_response)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  return (ret);
}


static char *
sshv2_buffer_get_string (gftp_request * request, sshv2_message * message)
{
  char *string;
  gint32 len;

  if ((len = sshv2_buffer_get_int32 (request, message, -1)) < 0)
    return (NULL);

  if (len > SSH_MAX_STRING_SIZE || (message->end - message->pos < len))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (message);
      gftp_disconnect (request);
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
  sshv2_params * params;
  char *tempstr, *dir;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  if (request->directory == NULL || *request->directory == '\0')
    dir = ".";
  else
    dir = request->directory;

  params = request->protocol_data;
  len = strlen (dir);
  tempstr = g_malloc (len + 9);
  strcpy (tempstr + 8, dir);

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len);
  memcpy (tempstr + 4, &num, 4);
  if ((ret = sshv2_send_command (request, SSH_FXP_REALPATH, tempstr, 
                                 len + 8)) < 0)
    {
      g_free (tempstr);
      return (ret);
    }

  g_free (tempstr);
  if (request->directory)
    {
      g_free (request->directory);
      request->directory = NULL;
    }

  memset (&message, 0, sizeof (message));
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);
  else if (ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&message);
      return (GFTP_ERETRYABLE);
    }
  else if (ret != SSH_FXP_NAME)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, 1)) < 0)
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
  int version, s[2], ret, ssh_use_askpass, sshv2_use_sftp_subsys;
  char **args, *tempstr, *p1, p2, *exepath, *ssh2_sftp_path;
  struct servent serv_struct;
  sshv2_message message;
  pid_t child;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);
  
  if (request->sockfd > 0)
    return (0);

  request->logging_function (gftp_logging_misc, request->user_data,
			     _("Opening SSH connection to %s\n"),
                             request->hostname);

  /* Ugh!! We don't get a login banner from sftp-server, and if we are
     using ssh-agent to cache a users password, then we won't receive
     any initial text from the server, and we'll block. So I just send a 
     xsftp server banner over. I hope this works on most Unices */

  gftp_lookup_request_option (request, "ssh2_sftp_path", &ssh2_sftp_path);
  gftp_lookup_request_option (request, "ssh_use_askpass", &ssh_use_askpass);
  gftp_lookup_request_option (request, "sshv2_use_sftp_subsys", 
                              &sshv2_use_sftp_subsys);

  if (ssh2_sftp_path == NULL || *ssh2_sftp_path == '\0')
    {
      p1 = "";
      p2 = ' ';
    }
  else
    {
      p1 = ssh2_sftp_path;
      p2 = '/';
    }

  if (request->port == 0)
    {
      if (!r_getservbyname ("ssh", "tcp", &serv_struct, NULL))
        {
         request->logging_function (gftp_logging_error, request->user_data,
                                    _("Cannot look up service name %s/tcp. Please check your services file\n"),
                                    "ssh");
        }
      else
        request->port = ntohs (serv_struct.s_port);
    }

  exepath = g_strdup_printf ("echo -n xsftp ; %s%csftp-server", p1, p2);
  args = sshv2_gen_exec_args (request, exepath, sshv2_use_sftp_subsys);

  if (ssh_use_askpass || sshv2_use_sftp_subsys)
    {
      if (socketpair (AF_LOCAL, SOCK_STREAM, 0, s) < 0)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                     _("Cannot create a socket pair: %s\n"),
                                     g_strerror (errno));
          return (GFTP_ERETRYABLE);
        }
    }
  else
    {
      if ((ret = open_ptys (request, &s[0], &s[1])) < 0)
        return (ret);
    }
 
  if ((child = fork ()) == 0)
    {
      setsid ();
      close (s[0]);

      tty_raw (s[1]);
      dup2 (s[1], 0);
      dup2 (s[1], 1);
      dup2 (s[1], 2);
      execvp (args[0], args);

      printf (_("Error: Cannot execute ssh: %s\n"), g_strerror (errno));
      exit (1);
    }
  else if (child > 0)
    {
      close (s[1]);
      tty_raw (s[0]);

      if (!sshv2_use_sftp_subsys)
        {
          tempstr = sshv2_start_login_sequence (request, s[0]);
          if (!tempstr ||
              !(strlen (tempstr) > 4 && strcmp (tempstr + strlen (tempstr) - 5,
                                                "xsftp") == 0))
            {
              sshv2_free_args (args);
              g_free (exepath);
              return (GFTP_EFATAL);
            }
          g_free (tempstr);
        }
      sshv2_free_args (args);
      g_free (exepath);

      request->sockfd = s[0];

      version = htonl (SSH_MY_VERSION);
      if ((ret = sshv2_send_command (request, SSH_FXP_INIT, (char *) 
                                     &version, 4)) < 0)
        return (ret);

      memset (&message, 0, sizeof (message));
      if ((ret = sshv2_read_response (request, &message, -1)) != SSH_FXP_VERSION)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                   _("Received wrong response from server, disconnecting\n"));
          sshv2_message_free (&message);
          gftp_disconnect (request);

          if (ret < 0)
            return (ret);
          else
            return (GFTP_ERETRYABLE);
        }
      sshv2_message_free (&message);

      request->logging_function (gftp_logging_misc, request->user_data,
			         _("Successfully logged into SSH server %s\n"),
                                 request->hostname);
    }
  else
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot fork another process: %s\n"),
                                 g_strerror (errno));
      g_free (args);
      return (GFTP_ERETRYABLE);
    }

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

  if (request->sockfd > 0)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
			         _("Disconnecting from site %s\n"),
                                 request->hostname);

      if (close (request->sockfd) < 0)
        request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));

      request->sockfd = -1;
    }

  if (params->message.buffer != NULL)
    sshv2_message_free (&params->message);
}


static int
sshv2_end_transfer (gftp_request * request)
{
  sshv2_params * params;
  sshv2_message message;
  gint32 len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  params = request->protocol_data;
  if (params->message.buffer != NULL)
    {
      sshv2_message_free (&params->message);
      params->count = 0;
    }

  if (params->handle_len > 0)
    {
      len = htonl (params->id++);
      memcpy (params->handle, &len, 4);

      if ((ret = sshv2_send_command (request, SSH_FXP_CLOSE, params->handle, 
                                     params->handle_len)) < 0)
        return (ret);

      memset (&message, 0, sizeof (message));
      if ((ret = sshv2_read_response (request, &message, -1)) != SSH_FXP_STATUS)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
          sshv2_message_free (&message);
          gftp_disconnect (request);
          if (ret < 0)
            return (ret);
          else
            return (GFTP_ERETRYABLE);
        }
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
  gint32 len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->sockfd > 0, GFTP_EFATAL);

  params = request->protocol_data;

  request->logging_function (gftp_logging_misc, request->user_data,
			     _("Retrieving directory listing...\n"));

  tempstr = g_malloc (strlen (request->directory) + 9);

  len = htonl (params->id++);
  memcpy (tempstr, &len, 4);

  len = htonl (strlen (request->directory));
  memcpy (tempstr + 4, &len, 4);
  strcpy (tempstr + 8, request->directory);
  if ((ret = sshv2_send_command (request, SSH_FXP_OPENDIR, tempstr, 
                                 strlen (request->directory) + 8)) < 0)
    {
      g_free (tempstr);
      return (ret);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);
  else if (ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&message);
      return (GFTP_ERETRYABLE);
    }
  else if (ret != SSH_FXP_HANDLE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
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
sshv2_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  gint32 len, attrs, longnamelen;
  int ret, i, count, retsize;
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
          if ((params->count = sshv2_buffer_get_int32 (request, 
                       &params->message, -1)) < 0)
            return (params->count);
        }
    }

  if (ret == SSH_FXP_NAME)
    {
      if ((len = sshv2_buffer_get_int32 (request, &params->message, -1)) < 0 ||
          params->message.pos + len > params->message.end)
        return (GFTP_EFATAL);

      params->message.pos += len;

      if ((longnamelen = sshv2_buffer_get_int32 (request, 
                                                 &params->message, -1)) < 0 || 
           params->message.pos + longnamelen > params->message.end)
        return (GFTP_EFATAL);

      longname = params->message.pos;
      params->message.pos += longnamelen;

      if ((attrs = sshv2_buffer_get_int32 (request, &params->message, -1)) < 0)
        return (attrs);

      if (attrs & SSH_FILEXFER_ATTR_SIZE)
        {
          params->message.pos += 8;
          if (params->message.pos > params->message.end)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
              sshv2_message_free (&params->message);
              gftp_disconnect (request);
              return (GFTP_EFATAL);
            }
        }

      if (attrs & SSH_FILEXFER_ATTR_UIDGID)
        {
          params->message.pos += 8;
          if (params->message.pos > params->message.end)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
              sshv2_message_free (&params->message);
              gftp_disconnect (request);
              return (GFTP_EFATAL);
            }
        }

      if (attrs & SSH_FILEXFER_ATTR_PERMISSIONS)
        {
          params->message.pos += 4;
          if (params->message.pos > params->message.end)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
              sshv2_message_free (&params->message);
              gftp_disconnect (request);
              return (GFTP_EFATAL);
            }
        }

      if (attrs & SSH_FILEXFER_ATTR_ACMODTIME)
        {
          params->message.pos += 8;
          if (params->message.pos > params->message.end)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
              sshv2_message_free (&params->message);
              gftp_disconnect (request);
              return (GFTP_EFATAL);
            }
        }

      if (attrs & SSH_FILEXFER_ATTR_EXTENDED)
        {
          if ((count = sshv2_buffer_get_int32 (request,
                                               &params->message, -1)) < 0)
            return (GFTP_EFATAL);

          for (i=0; i<count; i++)
            {
              if ((len = sshv2_buffer_get_int32 (request, 
                                                 &params->message, -1)) < 0 ||
                  params->message.pos + len + 4 > params->message.end)
                return (GFTP_EFATAL);

              params->message.pos += len + 4;

              if ((len = sshv2_buffer_get_int32 (request, 
                                                 &params->message, -1)) < 0 ||
                  params->message.pos + len + 4 > params->message.end)
                return (GFTP_EFATAL);
           
              params->message.pos += len + 4;
            }
        }

      longname[longnamelen] = '\0';

      /* The commercial SSH2 puts a / and * after some entries */
      if (longname[longnamelen - 1] == '*')
        longname[--longnamelen] = '\0';
      if (longname[longnamelen - 1] == '/')
        longname[--longnamelen] = '\0';

      if ((ret = gftp_parse_ls (request, longname, fle)) < 0)
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
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&params->message);
      gftp_disconnect (request);
      return (GFTP_EFATAL);
    }

  return (retsize);
}


static int
sshv2_chdir (gftp_request * request, const char *directory)
{
  sshv2_message message;
  sshv2_params * params;
  char *tempstr, *dir;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);

  params = request->protocol_data;
  if (request->directory != directory)
    {
      if (*directory == '/')
        {
          len = strlen (directory) + 8;
          tempstr = g_malloc (len + 1);
          strcpy (tempstr + 8, directory);
        }
      else
        {
          len = strlen (directory) + strlen (request->directory) + 9;
          tempstr = g_malloc (len + 1);
          strcpy (tempstr + 8, request->directory);
          strcat (tempstr + 8, "/");
          strcat (tempstr + 8, directory);
        }

      num = htonl (params->id++);
      memcpy (tempstr, &num, 4);

      num = htonl (len - 8);
      memcpy (tempstr + 4, &num, 4);
      if ((ret = sshv2_send_command (request, SSH_FXP_REALPATH, tempstr, len)) < 0)
        {
          g_free (tempstr);
          return (ret);
        }
      g_free (tempstr);

      memset (&message, 0, sizeof (message));
      ret = sshv2_read_response (request, &message, -1);
      if (ret < 0)
        return (ret);
      else if (ret == SSH_FXP_STATUS)
        {
          sshv2_message_free (&message);
          return (GFTP_ERETRYABLE);
        }
      else if (ret != SSH_FXP_NAME)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
          sshv2_message_free (&message);
          gftp_disconnect (request);
          return (GFTP_EFATAL);
        }

      message.pos += 4;
      if (sshv2_buffer_get_int32 (request, &message, 1) != 1)
        return (GFTP_EFATAL);

      if ((dir = sshv2_buffer_get_string (request, &message)) == NULL)
        return (GFTP_EFATAL);

      if (request->directory)
        g_free (request->directory);
      request->directory = dir;
      sshv2_message_free (&message);
      return (0);
    }
  else
    return (sshv2_getcwd (request));
}


static int 
sshv2_rmdir (gftp_request * request, const char *directory)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*directory == '/')
    {
      len = strlen (directory) + 8;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, directory);
    }
  else
    {
      len = strlen (directory) + strlen (request->directory) + 9;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, directory);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len - 8);
  memcpy (tempstr + 4, &num, 4);

  if (sshv2_send_command (request, SSH_FXP_RMDIR, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_rmfile (gftp_request * request, const char *file)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*file == '/')
    {
      len = strlen (file) + 8;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, file);
    }
  else
    {
      len = strlen (file) + strlen (request->directory) + 9;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len - 8);
  memcpy (tempstr + 4, &num, 4);

  if (sshv2_send_command (request, SSH_FXP_REMOVE, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_chmod (gftp_request * request, const char *file, int mode)
{
  char *tempstr, buf[10];
  sshv2_params * params;
  sshv2_message message;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*file == '/')
    {
      len = strlen (file) + 16;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, file);
    }
  else
    {
      len = strlen (file) + strlen (request->directory) + 17;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len - 16);
  memcpy (tempstr + 4, &num, 4);

  num = htonl (SSH_FILEXFER_ATTR_PERMISSIONS);
  memcpy (tempstr + len - 8, &num, 4);

  g_snprintf (buf, sizeof (buf), "%d", mode);
  num = htonl (strtol (buf, NULL, 8));
  memcpy (tempstr + len - 4, &num, 4);

  if (sshv2_send_command (request, SSH_FXP_SETSTAT, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_mkdir (gftp_request * request, const char *newdir)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (newdir != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*newdir == '/')
    {
      len = strlen (newdir) + 12;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, newdir);
    }
  else
    {
      len = strlen (newdir) + strlen (request->directory) + 13;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, newdir);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len - 12);
  memcpy (tempstr + 4, &num, 4);
  memset (tempstr + len - 4, 0, 4);  /* attributes */

  if (sshv2_send_command (request, SSH_FXP_MKDIR, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_rename (gftp_request * request, const char *oldname, const char *newname)
{
  char *tempstr, *oldstr, *newstr;
  sshv2_params * params;
  sshv2_message message;
  size_t oldlen, newlen;
  gint32 num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*oldname == '/')
    {
      oldlen = strlen (oldname); 
      oldstr = g_strdup (oldname);
    }
  else
    {
      oldlen = strlen (request->directory) + strlen (oldname) + 1;
      oldstr = g_strconcat (request->directory, "/", oldname, NULL);
    }

  if (*newname == '/')
    {
      newlen = strlen (newname); 
      newstr = g_strdup (newname);
    }
  else
    {
      newlen = strlen (request->directory) + strlen (newname) + 1;
      newstr = g_strconcat (request->directory, "/", newname, NULL);
    }

  tempstr = g_malloc (oldlen + newlen + 13);
  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (oldlen);
  memcpy (tempstr + 4, &num, 4);
  strcpy (tempstr + 8, oldstr);
  
  num = htonl (newlen);
  memcpy (tempstr + 8 + oldlen, &num, 4);
  strcpy (tempstr + 12 + oldlen, newstr);

  if (sshv2_send_command (request, SSH_FXP_RENAME, tempstr, oldlen + newlen + 12) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int 
sshv2_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  gint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*file == '/')
    {
      len = strlen (file) + 20;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, file);
    }
  else
    {
      len = strlen (file) + strlen (request->directory) + 21;
      tempstr = g_malloc (len + 1);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len - 20);
  memcpy (tempstr + 4, &num, 4);

  num = htonl (SSH_FILEXFER_ATTR_ACMODTIME);
  memcpy (tempstr + len - 12, &num, 4);

  num = htonl (datetime);
  memcpy (tempstr + len - 8, &num, 4);

  num = htonl (datetime);
  memcpy (tempstr + len - 4, &num, 4);

  if (sshv2_send_command (request, SSH_FXP_SETSTAT, tempstr, len) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static off_t
sshv2_get_file_size (gftp_request * request, const char *file)
{
  gint32 len, highnum, lownum, attrs, num;
  sshv2_params * params;
  char *tempstr;
  int serv_ret;
#ifdef G_HAVE_GINT64
  gint64 ret;
#endif

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (*file == '/')
    {
      len = strlen (file);
      tempstr = g_malloc (len + 9);
      strcpy (tempstr + 8, file);
    }
  else
    {
      len = strlen (file) + strlen (request->directory) + 1;
      tempstr = g_malloc (len + 9);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (len);
  memcpy (tempstr + 4, &num, 4);

  if (sshv2_send_command (request, SSH_FXP_STAT, tempstr, len + 8) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }
  g_free (tempstr);

  memset (&params->message, 0, sizeof (params->message));
  serv_ret = sshv2_read_response (request, &params->message, -1);
  if (serv_ret < 0)
    return (serv_ret);
  else if (serv_ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&params->message);
      return (GFTP_ERETRYABLE);
    }
  else if (serv_ret != SSH_FXP_ATTRS)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&params->message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (params->message.length < 5)
    return (GFTP_ERETRYABLE);
  params->message.pos += 4;

  if ((attrs = sshv2_buffer_get_int32 (request, &params->message, -1)) < 0)
    return (GFTP_ERETRYABLE);

  if (attrs & SSH_FILEXFER_ATTR_SIZE)
    {
      if ((highnum = sshv2_buffer_get_int32 (request, &params->message, -1)) < 0)
        return (GFTP_ERETRYABLE);

      if ((lownum = sshv2_buffer_get_int32 (request, &params->message, -1)) < 0)
        return (GFTP_ERETRYABLE);

      sshv2_message_free (&params->message);

#if G_HAVE_GINT64
      ret = (gint64) lownum | ((gint64) highnum >> 32);
      return (ret);
#else
      return (lownum);
#endif
    }

  sshv2_message_free (&params->message);

  return (0);

}


static off_t
sshv2_get_file (gftp_request * request, const char *file, int fd,
                off_t startsize)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  size_t stlen;
  gint32 num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->sockfd > 0, GFTP_EFATAL);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->offset = startsize;

  if (*file == '/')
    {
      stlen = strlen (file);
      tempstr = g_malloc (stlen + 16);
      strcpy (tempstr + 8, file);
    }
  else
    {
      stlen = strlen (file) + strlen (request->directory) + 1;
      tempstr = g_malloc (stlen + 16);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (stlen);
  memcpy (tempstr + 4, &num, 4);

  num = htonl (SSH_FXF_READ);
  memcpy (tempstr + 8 + stlen, &num, 4);

  memset (tempstr + 12 + stlen, 0, 4);
  
  if (sshv2_send_command (request, SSH_FXP_OPEN, tempstr, stlen + 16) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }

  g_free (tempstr);
  memset (&message, 0, sizeof (message));
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);
  else if (ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&message);
      return (GFTP_ERETRYABLE);
    }
  else if (ret != SSH_FXP_HANDLE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
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
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  size_t stlen;
  gint32 num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->sockfd > 0, GFTP_EFATAL);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->offset = 0;

  if (*file == '/')
    {
      stlen = strlen (file);
      tempstr = g_malloc (stlen + 16);
      strcpy (tempstr + 8, file);
    }
  else
    {
      stlen = strlen (file) + strlen (request->directory) + 1;
      tempstr = g_malloc (stlen + 16);
      strcpy (tempstr + 8, request->directory);
      strcat (tempstr + 8, "/");
      strcat (tempstr + 8, file);
    }

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

  num = htonl (stlen);
  memcpy (tempstr + 4, &num, 4);

  if (startsize > 0)
    num = htonl (SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_APPEND);
  else
    num = htonl (SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC);
  memcpy (tempstr + 8 + stlen, &num, 4);

  memset (tempstr + 12 + stlen, 0, 4);
  
  if (sshv2_send_command (request, SSH_FXP_OPEN, tempstr, stlen + 16) < 0)
    {
      g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }

  g_free (tempstr);
  memset (&message, 0, sizeof (message));
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);
  else if (ret == SSH_FXP_STATUS)
    {
      sshv2_message_free (&message);
      return (GFTP_ERETRYABLE);
    }
  else if (ret != SSH_FXP_HANDLE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
      sshv2_message_free (&message);
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (message.length - 4 > SSH_MAX_HANDLE_SIZE)
    {
      request->logging_function (gftp_logging_error, request->user_data,
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

  return (0);
}


static ssize_t 
sshv2_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  sshv2_params * params;
  sshv2_message message;
  gint32 num;
  int ret;

#ifdef G_HAVE_GINT64
  gint64 offset;
#else
  gint32 offset;
#endif

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->sockfd > 0, GFTP_EFATAL);
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

#ifdef G_HAVE_GINT64
  offset = hton64 (params->offset);
  memcpy (params->read_buffer + params->handle_len, &offset, 8);
#else
  memset (params->read_buffer + params->handle_len, 0, 4);
  offset = htonl (params->offset);
  memcpy (params->read_buffer + params->handle_len + 4, &offset, 4);
#endif
  
  num = htonl (size);
  memcpy (params->read_buffer + params->handle_len + 8, &num, 4);
  
  if (sshv2_send_command (request, SSH_FXP_READ, params->read_buffer, params->handle_len + 12) < 0)
    return (GFTP_ERETRYABLE);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) != SSH_FXP_DATA)
    {
      if (ret < 0)
        return (ret);

      message.pos += 4;
      if ((num = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
        return (num);
      sshv2_message_free (&message);
      if (num != SSH_FX_EOF)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }
      return (0);
    }

  memcpy (&num, message.buffer + 4, 4);
  num = ntohl (num);
  if (num > size)
    {
      request->logging_function (gftp_logging_error, request->user_data,
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
  gint32 num;
  int ret;

#ifdef G_HAVE_GINT64
  gint64 offset;
#else
  gint32 offset;
#endif

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_SSHV2_NUM, GFTP_EFATAL);
  g_return_val_if_fail (request->sockfd > 0, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);
  g_return_val_if_fail (size <= 32500, GFTP_EFATAL);

  params = request->protocol_data;

  num = htonl (params->handle_len);
  memcpy (tempstr, params->handle, params->handle_len);

  num = htonl (params->id++);
  memcpy (tempstr, &num, 4);

#ifdef G_HAVE_GINT64
  offset = hton64 (params->offset);
  memcpy (tempstr + params->handle_len, &offset, 8);
#else
  memset (tempstr + params->handle_len, 0, 4);
  offset = htonl (params->offset);
  memcpy (tempstr + params->handle_len + 4, &offset, 4);
#endif
 
  num = htonl (size);
  memcpy (tempstr + params->handle_len + 8, &num, 4);
  memcpy (tempstr + params->handle_len + 12, buf, size);
  
  if (sshv2_send_command (request, SSH_FXP_WRITE, tempstr, params->handle_len + size + 12) < 0)
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
   {
     request->logging_function (gftp_logging_error, request->user_data,
                     _("Received wrong response from server, disconnecting\n"));
     sshv2_message_free (&message);
     gftp_disconnect (request);
     return (GFTP_ERETRYABLE);
   }

  message.pos += 4;
  if ((num = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK)) < 0)
    return (GFTP_ERETRYABLE);
  sshv2_message_free (&message);

  if (num == SSH_FX_EOF)
    return (0);
  else if (num != SSH_FX_OK)
    return (GFTP_ERETRYABLE);

  params->offset += size;
  return (size);
}


static void
sshv2_set_config_options (gftp_request * request)
{
  int ssh_need_userpass;

  gftp_lookup_request_option (request, "ssh_need_userpass", &ssh_need_userpass);
  request->need_userpass = ssh_need_userpass;
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


void
sshv2_init (gftp_request * request)
{
  sshv2_params * params;

  g_return_if_fail (request != NULL);

  request->protonum = GFTP_SSHV2_NUM;
  request->init = sshv2_init;
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
  request->list_files = sshv2_list_files;
  request->get_next_file = sshv2_get_next_file;
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
  request->use_threads = 1;
  request->always_connected = 0;
  request->protocol_data = g_malloc0 (sizeof (sshv2_params));
  request->server_type = GFTP_DIRTYPE_UNIX;
  gftp_set_config_options (request);

  params = request->protocol_data;
  params->id = 1;
}

