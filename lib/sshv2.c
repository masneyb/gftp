/***********************************************************************************/
/*  sshv2.c - functions that will use the sshv2 protocol                           */
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
       *transfer_buffer;
  size_t handle_len, transfer_buffer_len;

  guint32 id,
          count;
  sshv2_message message;

  unsigned int initialized : 1,
               dont_log_status : 1;              /* For uploading files */

  guint64 offset;
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

/* responses from the server to the client */
// https://tools.ietf.org/html/draft-ietf-secsh-filexfer-03#section-7
#define SSH_FX_OK                            0
#define SSH_FX_EOF                           1
#define SSH_FX_NO_SUCH_FILE                  2
#define SSH_FX_PERMISSION_DENIED             3
#define SSH_FX_FAILURE                       4
#define SSH_FX_BAD_MESSAGE                   5
#define SSH_FX_NO_CONNECTION                 6
#define SSH_FX_CONNECTION_LOST               7
#define SSH_FX_OP_UNSUPPORTED                8
#define SSH_FX_INVALID_HANDLE                9
#define SSH_FX_NO_SUCH_PATH                  10
#define SSH_FX_FILE_ALREADY_EXISTS           11
#define SSH_FX_WRITE_PROTECT                 12
#define SSH_FX_COUNT                         13 //
static char * ssh_response_str[SSH_FX_COUNT] = { NULL };

#define SSH_LOGIN_BUFSIZE	200
#define SSH_ERROR_BADPASS	-1
#define SSH_ERROR_QUESTION	-2


static char *
sshv2_initialize_buffer (gftp_request * request, size_t len)
{
  sshv2_params * params;
  guint32 num;
  char *ret;

  params = request->protocol_data;
  ret = g_malloc0 ((gulong) len + 1);

  num = htonl (params->id++);
  memcpy (ret, &num, 4);

  return (ret);
}


static void
sshv2_add_string_to_buf (char *buf, const char *str, size_t len)
{
  guint32 num;

  num = htonl (strlen (str));
  memcpy (buf, &num, 4);
  memcpy (buf + 4, str, len);
}


static char *
sshv2_initialize_buffer_with_i18n_string (gftp_request * request,
                                          const char *str, size_t *msglen)
{
  const char *addstr;
  char *utf8, *ret;
  size_t pathlen;

  utf8 = gftp_filename_from_utf8 (request, str, &pathlen);
  if (utf8 != NULL)
    addstr = utf8;
  else
    {
      addstr = str;
      pathlen = strlen (str);
    }

  *msglen += pathlen + 8;
  ret = sshv2_initialize_buffer (request, *msglen);
  sshv2_add_string_to_buf (ret + 4, addstr, pathlen);

  if (utf8 != NULL)
    g_free (utf8);

  return (ret);
}


static char *
_sshv2_generate_utf8_path (gftp_request * request, const char *str, size_t *len)
{
  char *path, *utf8;

  if (*str == '/')
    path = g_strdup (str);
  else
    path = gftp_build_path (request, request->directory, str, NULL);

  utf8 = gftp_filename_from_utf8 (request, path, len);
  if (utf8 != NULL)
    {
      g_free (path);
      return (utf8);
    }
  else
    {
      *len = strlen (path);
      return (path);
    }
}


static char *
sshv2_initialize_buffer_with_two_i18n_paths (gftp_request * request,
                                             const char *str1,
                                             const char *str2, size_t *msglen)
{
  char *first_utf8, *sec_utf8, *ret;
  size_t pathlen1, pathlen2;

  first_utf8 = _sshv2_generate_utf8_path (request, str1, &pathlen1);
  sec_utf8 = _sshv2_generate_utf8_path (request, str2, &pathlen2);

  *msglen += pathlen1 + pathlen2 + 12;
  ret = sshv2_initialize_buffer (request, *msglen);
  sshv2_add_string_to_buf (ret + 4, first_utf8, pathlen1);
  sshv2_add_string_to_buf (ret + 8 + pathlen1, sec_utf8, pathlen2);

  if (first_utf8 != str1)
    g_free (first_utf8);

  if (sec_utf8 != str2)
    g_free (sec_utf8);

  return (ret);
}


static char *
sshv2_initialize_string_with_path (gftp_request * request, const char *path,
                                   size_t *len, char **endpos)
{
  char *ret, *tempstr;
  size_t origlen;

  origlen = *len;
  if (*path == '/')
    ret = sshv2_initialize_buffer_with_i18n_string (request, path, len);
  else
    {
      tempstr = gftp_build_path (request, request->directory, path, NULL);
      ret = sshv2_initialize_buffer_with_i18n_string (request, tempstr, len);
      g_free (tempstr);
    }

  if (endpos != NULL)
    *endpos = ret + *len - origlen;

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
  *logstr = g_realloc (*logstr, (gulong) *logstr_len + 1);
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
  args = g_malloc0 (sizeof (char *) * args_cur);
  args[0] = g_strdup (tempstr);

  logstr = g_strdup (args[0]);
  logstr_len = strlen (logstr);

  gftp_lookup_request_option (request, "ssh_extra_params", &tempstr);
  if (tempstr != NULL && *tempstr != '\0') {
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                         "%s", tempstr);
  } else {
    // `ssh_extra_params` is empty and we need to add `-o StrictHostKeyChecking=ask`
    sshv2_add_exec_args (&logstr, &logstr_len, &args, &args_len, &args_cur,
                       " -o StrictHostKeyChecking=ask");
  }

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
  static char *pwstrs[] = { "Enter passphrase for RSA key",
                            "Enter passphrase for key '",
                            "Password",
                            "password",
                            NULL };
  char *tempstr, *temp1str, *pwstr, *yesstr = "yes\n", *securid_pass;
  int wrotepw, ok, maxfd, ret, clear_tempstr, pwidx;
  size_t rem, len, diff;
  fd_set rset, eset;
  ssize_t rd;

  rem = len = SSH_LOGIN_BUFSIZE;
  diff = 0;
  tempstr = g_malloc0 ((gulong) len + 1);
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
      if (ret < 0)
        {
          if (errno == EINTR || errno == EAGAIN)
            {
              if (request->cancel)
                {
                  gftp_disconnect (request);
                  return (GFTP_ERETRYABLE);
                }

              continue;
            }
          else
            {
              request->logging_function (gftp_logging_error, request,
                                         _("Connection to %s timed out\n"),
                                         request->hostname);
              gftp_disconnect (request);
              return (GFTP_ERETRYABLE);
            }
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

      /* See if we are at the enter password prompt... */
      for (pwidx = 0; pwstrs[pwidx] != NULL; pwidx++)
        {
          if (strstr (tempstr, pwstrs[pwidx]) != NULL ||
              strstr (tempstr, _(pwstrs[pwidx])) != NULL)
            break;
        }

      clear_tempstr = 0;
      if (pwstrs[pwidx] != NULL)
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
      else if (strstr (tempstr, "(yes/no)?") != NULL ||
               strstr (tempstr, "(yes/no/[") != NULL)
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

  if (!ssh_response_str[0])
  {
    ssh_response_str[0]  = _("OK");                         /* SSH_FX_OK  */
    ssh_response_str[1]  = _("EOF");                        /* SSH_FX_EOF */
    ssh_response_str[2]  = _("No such file or directory");  /* SSH_FX_NO_SUCH_FILE */
    ssh_response_str[3]  = _("Permission denied");          /* SSH_FX_PERMISSION_DENIED */
    ssh_response_str[4]  = _("Failure");                    /* SSH_FX_FAILURE */
    ssh_response_str[5]  = _("Bad message");                /* SSH_FX_BAD_MESSAGE */
    ssh_response_str[6]  = _("No connection");              /* SSH_FX_NO_CONNECTION */
    ssh_response_str[7]  = _("Connection lost");            /* SSH_FX_CONNECTION_LOST */
    ssh_response_str[8]  = _("Operation unsupported");      /* SSH_FX_OP_UNSUPPORTED */
    ssh_response_str[9]  = _("Invalid file handle");        /* SSH_FX_INVALID_HANDLE */
    ssh_response_str[10] = _("Invalid file path");          /* SSH_FX_NO_SUCH_PATH */
    ssh_response_str[11] = _("File already exists");        /* SSH_FX_FILE_ALREADY_EXISTS */
    ssh_response_str[12] = _("Write-protected filesystem"); /* SSH_FX_WRITE_PROTECT */
  };

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
        break;
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
      case SSH_FXP_RENAME:
        request->logging_function (level, request, 
                                   _("%d: Rename %s\n"), id, message + 8);
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
        if (num < SSH_FX_COUNT) {
           descr = ssh_response_str[num];
        } else {
           descr = _("Unknown message returned from server");
        }
        if (num > 1) {
           level = gftp_logging_error;
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
  message->buffer = g_malloc0 (message->length + 1);

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

#ifdef DEBUG
  printf ("\rReceived message: ");
  for (rem=0; rem<message->length; rem++)
    printf ("%x ", message->buffer[rem] & 0xff);
  printf ("\n");
#endif

  message->buffer[message->length] = '\0';

  sshv2_log_command (request, gftp_logging_recv, message->command, 
                     message->buffer, message->length);
  
  return (message->command);
}


static void
sshv2_destroy (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_SSH2);

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
  guint32 num;
  int ret;

  memset (message, 0, sizeof (*message));
  ret = sshv2_read_response (request, message, -1);
  if (ret < 0)
    return (ret);
  else if (fxp_is_retryable > 0 && ret == fxp_is_retryable)
    {
      memcpy (&num, message->buffer + 4, 4);
      num = ntohl (num);

      sshv2_message_free (message);

      if ( num == SSH_FX_PERMISSION_DENIED 
        || num == SSH_FX_NO_SUCH_FILE
        || num == SSH_FX_FAILURE)
        return (GFTP_ECANIGNORE);
      else
        return (GFTP_ERETRYABLE);
    }
  else if (fxp_is_wrong > 0 && ret != fxp_is_wrong)
    return (sshv2_wrong_response (request, message));

  return (ret);
}


static int
sshv2_response_return_code (gftp_request * request, sshv2_message * message,
                            unsigned int ret)
{
  switch (ret)
    {
      case SSH_FX_OK:
      case SSH_FX_EOF:
      case SSH_FX_OP_UNSUPPORTED:
        return (GFTP_ERETRYABLE);
      case SSH_FX_FAILURE:
      case SSH_FX_NO_SUCH_FILE:
      case SSH_FX_INVALID_HANDLE:
      case SSH_FX_NO_SUCH_PATH:
      case SSH_FX_PERMISSION_DENIED:
      case SSH_FX_FILE_ALREADY_EXISTS:
      case SSH_FX_WRITE_PROTECT:
        return (GFTP_ECANIGNORE);
      default:
        return (sshv2_wrong_response (request, message));
    }
}


static int
sshv2_buffer_get_int32 (gftp_request * request, sshv2_message * message,
                        unsigned int expected_response, int check_response,
                        guint32 * num)
{
  guint32 snum;

  if (message->end - message->pos < 4)
    return (sshv2_wrong_response (request, message));
      
  memcpy (&snum, message->pos, 4);
  snum = ntohl (snum);
  message->pos += 4;

  if (check_response && snum != expected_response)
    return (sshv2_response_return_code (request, message, snum));

  if (num != NULL)
    *num = snum;

  return (0);
}


static int
sshv2_buffer_get_int64 (gftp_request * request, sshv2_message * message,
                        unsigned int expected_response, int check_response,
                        gint64 * num)
{
  guint64 snum;
  guint32 hinum = 0, lonum = 0;
  int ret;

  if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &hinum)) < 0)
    return (ret);

  if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &lonum)) < 0)
    return (ret);

  snum = (gint64) lonum | ((gint64) hinum << 32);

  if (check_response && snum != expected_response)
    return (sshv2_response_return_code (request, message, snum));

  if (num != NULL)
    *num = snum;

  return (0);
}


static char *
sshv2_buffer_get_string (gftp_request * request, sshv2_message * message,
                         int return_string)
{
  guint32 len = 0, buflen = 0;
  char *string;

  if (sshv2_buffer_get_int32 (request, message, 0, 0, &len) < 0)
    return (NULL);

  buflen = message->end - message->pos;
  if (len > SSH_MAX_STRING_SIZE || (buflen < len))
    {
      sshv2_wrong_response (request, message);
      return (NULL);
    }

  if (return_string)
    {
      string = g_malloc0 (len + 1);
      memcpy (string, message->pos, len);
      string[len] = '\0';
    }
  else
    string = NULL;

  message->pos += len;
  return (string);
}


static int
sshv2_send_command_and_check_response (gftp_request * request, char type,
                                       char *command, size_t len)
{
  sshv2_message message;
  int ret;

  ret = sshv2_send_command (request, type, command, len);
  if (ret < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, 1,
                                     NULL)) < 0)
    return (ret);

  sshv2_message_free (&message);

  return (0);
}


static int
sshv2_getcwd (gftp_request * request)
{
  char *tempstr, *dir, *utf8;
  sshv2_message message;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);

  if (request->directory == NULL || *request->directory == '\0')
    dir = ".";
  else
    dir = request->directory;

  len = 0;
  tempstr = sshv2_initialize_buffer_with_i18n_string (request, dir, &len);
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
  if ((ret = sshv2_buffer_get_int32 (request, &message, 1, 1, NULL)) < 0)
    return (ret);

  if ((dir = sshv2_buffer_get_string (request, &message, 1)) == NULL)
    return (GFTP_EFATAL);

  utf8 = gftp_filename_to_utf8 (request, dir, &len);
  if (utf8 != NULL)
    {
      request->directory = utf8;
      g_free (dir);
    }
  else
    request->directory = dir;

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
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
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
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_SSH2);

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
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);

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

  if (params->transfer_buffer != NULL)
    {
      g_free (params->transfer_buffer);
      params->transfer_buffer = NULL;
    }

  return (0);
}


static int
sshv2_list_files (gftp_request * request)
{
  sshv2_params * params;
  sshv2_message message;
  char *tempstr;
  size_t msglen;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  params = request->protocol_data;

  request->logging_function (gftp_logging_misc, request,
			     _("Retrieving directory listing...\n"));

  msglen = 0;
  tempstr = sshv2_initialize_buffer_with_i18n_string (request,
                                                      request->directory,
                                                      &msglen);
  ret = sshv2_send_command (request, SSH_FXP_OPENDIR, tempstr, msglen);
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
  guint32 attrs = 0, num = 0, count = 0, i = 0;
  int ret;

  if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &attrs)) < 0)
    return (ret);

  if (attrs & SSH_FILEXFER_ATTR_SIZE)
    {
      if ((ret = sshv2_buffer_get_int64 (request, message, 0, 0, &fle->size)) < 0)
        return (ret);
    }

  if (attrs & SSH_FILEXFER_ATTR_UIDGID)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &num)) < 0)
        return (ret);
      fle->user = g_strdup_printf ("%d", num);

      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &num)) < 0)
        return (ret);
      fle->group = g_strdup_printf ("%d", num);
    }

  if (attrs & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &num)) < 0)
        return (ret);

      fle->st_mode = num;
    }

  if (attrs & SSH_FILEXFER_ATTR_ACMODTIME)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, NULL)) < 0)
        return (num);

      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &num)) < 0)
        return (ret);

      fle->datetime = num;
    }

  if (attrs & SSH_FILEXFER_ATTR_EXTENDED)
    {
      if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0, &count)) < 0)
        return (ret);

      for (i=0; i<count; i++)
        {
          if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0,
                                             &num)) < 0 ||
              message->pos + num + 4 > message->end)
            return (GFTP_EFATAL);

          message->pos += num + 4;

          if ((ret = sshv2_buffer_get_int32 (request, message, 0, 0,
                                             &num)) < 0 ||
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
  int ret, retsize, iret;
  sshv2_params *params;
  char *stpos;
  guint32 len;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
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
          request->last_dir_entry = g_malloc0 (params->message.length + 4);
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
                                              &params->message, 0, 0,
                                              &params->count)) < 0)
            return (iret);
        }
    }

  if (ret == SSH_FXP_NAME)
    {
      stpos = params->message.pos;
      if ((fle->file = sshv2_buffer_get_string (request, &params->message, 1)) == NULL)
        return (GFTP_EFATAL);

      sshv2_buffer_get_string (request, &params->message, 0);

      if ((ret = sshv2_decode_file_attributes (request, &params->message,
                                               fle)) < 0)
        {
          gftp_file_destroy (fle, 0);
          return (ret);
        }

      retsize = params->message.pos - stpos;
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
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, directory, &len, NULL);

  ret = sshv2_send_command (request, SSH_FXP_REALPATH, tempstr, len);

  g_free (tempstr);
  if (ret < 0)
    return (ret);

  ret = sshv2_read_status_response (request, &message, -1, SSH_FXP_STATUS,
                                    SSH_FXP_NAME);
  if (ret < 0)
    return (ret);

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, 1, 1, NULL)) < 0)
    return (ret);

  if ((dir = sshv2_buffer_get_string (request, &message, 1)) == NULL)
    return (GFTP_EFATAL);

  if (request->directory)
    g_free (request->directory);

  request->directory = dir;
  sshv2_message_free (&message);
  return (0);
}


static int 
sshv2_rmdir (gftp_request * request, const char *directory)
{
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, directory, &len, NULL);

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_RMDIR,
                                               tempstr, len);
  g_free (tempstr);
  return (ret);
}


static int 
sshv2_rmfile (gftp_request * request, const char *file)
{
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 0;
  tempstr = sshv2_initialize_string_with_path (request, file, &len, NULL);

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_REMOVE,
                                               tempstr, len);
  g_free (tempstr);
  return (ret);
}


static int 
sshv2_chmod (gftp_request * request, const char *file, mode_t mode)
{
  char *tempstr, *endpos;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 8; /* For attributes */
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (SSH_FILEXFER_ATTR_PERMISSIONS);
  memcpy (endpos, &num, 4);

  num = htonl (mode);
  memcpy (endpos + 4, &num, 4);

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_SETSTAT,
                                               tempstr, len);
  g_free (tempstr);
  return (ret);
}


static int 
sshv2_mkdir (gftp_request * request, const char *newdir)
{
  char *tempstr;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (newdir != NULL, GFTP_EFATAL);

  len = 4; /* For attributes */
  tempstr = sshv2_initialize_string_with_path (request, newdir, &len, NULL);

  /* No need to set attributes since all characters of the tempstr buffer is
     initialized to 0 */

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_MKDIR,
                                               tempstr, len);
  g_free (tempstr);
  return (ret);
}


static int 
sshv2_rename (gftp_request * request, const char *oldname, const char *newname)
{
  char *tempstr;
  size_t msglen;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  msglen = 0;
  tempstr = sshv2_initialize_buffer_with_two_i18n_paths (request,
                                                         oldname, newname,
                                                         &msglen);

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_RENAME,
                                               tempstr, msglen);
  g_free (tempstr);
  return (ret);
}


static int 
sshv2_set_file_time (gftp_request * request, const char *file, time_t datetime)
{
  char *tempstr, *endpos;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  len = 12; /* For date/time */
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (SSH_FILEXFER_ATTR_ACMODTIME);
  memcpy (endpos, &num, 4);

  num = htonl (datetime);
  memcpy (endpos + 4, &num, 4);
  memcpy (endpos + 8, &num, 4);

  ret = sshv2_send_command_and_check_response (request, SSH_FXP_SETSTAT,
                                               tempstr, len);
  g_free (tempstr);
  return (ret);
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
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
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
      gftp_file_destroy (fle, 0);
      return (ret);
    }

  sshv2_message_free (&message);

  return (0);
}


static int
sshv2_stat_filename (gftp_request * request, const char *filename,
                     mode_t * mode, off_t * filesize)
{
  gftp_file fle;
  int ret;

  memset (&fle, 0, sizeof (fle));
  ret = sshv2_send_stat_command (request, filename, &fle);
  if (ret < 0)
    return (ret);

  *mode = fle.st_mode;
  *filesize = fle.size;

  gftp_file_destroy (&fle, 0);

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
  gftp_file_destroy (&fle, 0);

  return (size);
}


static int
sshv2_open_file (gftp_request * request, const char *file, off_t startsize,
                 guint32 mode)
{
  char *tempstr, *endpos;
  sshv2_params * params;
  sshv2_message message;
  guint32 num;
  size_t len;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  params = request->protocol_data;
  params->offset = startsize;

  len = 8; /* For mode */
  tempstr = sshv2_initialize_string_with_path (request, file, &len, &endpos);

  num = htonl (mode);
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


static off_t
sshv2_get_file (gftp_request * request, const char *file, off_t startsize)
{
  int ret;

  if ((ret = sshv2_open_file (request, file, startsize, SSH_FXF_READ)) < 0)
    return (ret);

  return (sshv2_get_file_size (request, file));
}


static int
sshv2_put_file (gftp_request * request, const char *file,
                off_t startsize, off_t totalsize)
{
  guint32 mode;
  int ret;

  if (startsize > 0)
    mode = SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_APPEND;
  else
    mode = SSH_FXF_WRITE | SSH_FXF_CREAT | SSH_FXF_TRUNC;

  if ((ret = sshv2_open_file (request, file, startsize, mode)) < 0)
    return (ret);

  return (0);
}


static void
sshv2_setup_file_offset (sshv2_params * params, char *buf)
{
  guint32 hinum, lownum;
  hinum = htonl(params->offset >> 32);
  lownum = htonl((guint32) params->offset);

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
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (params->transfer_buffer == NULL)
    {
      params->transfer_buffer_len = params->handle_len + 12;
      params->transfer_buffer = g_malloc0 (params->transfer_buffer_len);
      memcpy (params->transfer_buffer, params->handle, params->handle_len);
    }

  num = htonl (params->id++);
  memcpy (params->transfer_buffer, &num, 4);

  sshv2_setup_file_offset (params, params->transfer_buffer);

  num = htonl (size);
  memcpy (params->transfer_buffer + params->handle_len + 8, &num, 4);
  
  if ((ret = sshv2_send_command (request, SSH_FXP_READ, params->transfer_buffer,
                                 params->handle_len + 12)) < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  if ((ret = sshv2_read_response (request, &message, -1)) != SSH_FXP_DATA)
    {
      if (ret < 0)
        return (ret);

      message.pos += 4;
      if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_EOF, 1,
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
  guint32 num;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_SSH2, GFTP_EFATAL);
  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);
  g_return_val_if_fail (buf != NULL, GFTP_EFATAL);

  params = request->protocol_data;

  if (params->transfer_buffer == NULL)
    {
      params->transfer_buffer_len = params->handle_len + size + 12;
      params->transfer_buffer = g_malloc0 (params->transfer_buffer_len);
      memcpy (params->transfer_buffer, params->handle, params->handle_len);
    }

  num = htonl (params->id++);
  memcpy (params->transfer_buffer, &num, 4);

  sshv2_setup_file_offset (params, params->transfer_buffer);

  num = htonl (size);
  memcpy (params->transfer_buffer + params->handle_len + 8, &num, 4);
  memcpy (params->transfer_buffer + params->handle_len + 12, buf, size);
  
  if ((ret = sshv2_send_command (request, SSH_FXP_WRITE,
                                 params->transfer_buffer,
                                 params->transfer_buffer_len)) < 0)
    return (ret);

  memset (&message, 0, sizeof (message));
  params->dont_log_status = 1;
  ret = sshv2_read_response (request, &message, -1);
  if (ret < 0)
    return (ret);

  params->dont_log_status = 0;
  if (ret != SSH_FXP_STATUS)
    return (sshv2_wrong_response (request, &message));

  message.pos += 4;
  if ((ret = sshv2_buffer_get_int32 (request, &message, SSH_FX_OK, 1, &num)) < 0)
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
  request->need_username = ssh_need_userpass;
  request->need_password = ssh_need_userpass;
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
  if (sparms->transfer_buffer != NULL)
    {
      dparms->transfer_buffer = g_malloc0 (sparms->transfer_buffer_len);
      memcpy (dparms->transfer_buffer, sparms->transfer_buffer,
              sparms->transfer_buffer_len);
    }
  else
    dparms->transfer_buffer = NULL;

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

  request->protonum = GFTP_PROTOCOL_SSH2;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_SSH2].url_prefix;

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
  request->need_hostport = 1;
  request->need_username = 1;
  request->need_password = 1;
  request->use_cache = 1;
  request->always_connected = 0;
  request->use_local_encoding = 0;
  request->protocol_data = g_malloc0 (sizeof (sshv2_params));

  params = request->protocol_data;
  params->id = 1;

  return (gftp_set_config_options (request));
}

