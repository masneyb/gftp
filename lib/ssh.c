/*****************************************************************************/
/*  ssh.c - functions that will use the ssh protocol                         */
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

/* This will use Brian Wellington <bwelling@xbill.org>'s sftpserv program */
/* on the remote. Some of this code is derived from the sftp client */

/* 11/5/2002 - This protocol is now obsolete by the SSH2 protocol. I'm not 
   going to be adding any new features to it, but I'll keep it inside the code 
   for a little bit. I do plan on removing it completely though */

#include "gftp.h"
static const char cvsid[] = "$Id$";

#define CHDIR		10
#define GETDIR		11
#define TELLDIR		12
#define SENDFILE	15
#define NOFILEMATCH	16
#define FILESIZE	20
#define FILEMODE	21
#define DATA		22
#define ENDDATA		23
#define FILEOK		24
#define STREAM		25
#define REQUEST		26
#define FILENAME	27
#define EXEC		28
#define SKIPBYTES	29
#define ERROR		30
#define SUCCESS	 	31
#define CLOSE		32
#define SSH_VERSION 	33
#define CANCEL		34
#define FILETIME	40

typedef struct ssh_parms_tag
{
    char *buffer,		/* The buffer we are reading the data from */
         *pos;			/* Our position in this buffer */
    int enddata;		/* Are we done reading the data */
    char channel;		/* Data channel we are writing to */
} ssh_parms;


typedef struct ssh_message_tag
{
  char channel,
       command;
  gint32 len;
  void *data;
} ssh_message;


static void
ssh_log_command (gftp_request * request, int channel, int cmdnum, 
                 const char *command, size_t len, int direction)
{
  const char *pos;
  char *tempstr;
  int ok;

  switch (cmdnum)
    {
      case CHDIR:
        tempstr = "CHDIR ";
        break;
      case GETDIR:
        tempstr = "GETDIR ";
        break;
      case TELLDIR:
        tempstr = "TELLDIR ";
        break;
      case SENDFILE:
        tempstr = "SENDFILE ";
        break;
      case FILESIZE:
        tempstr = "FILESIZE ";
        break;
      case FILEMODE:
        tempstr = "FILEMODE ";
        break;
      case ENDDATA:
        tempstr = "ENDDATA ";
        break;
      case FILEOK:
        tempstr = "FILEOK ";
        break;
      case STREAM:
        tempstr = "STREAM ";
        break;
      case REQUEST:
        tempstr = "REQUEST ";
        break;
      case FILENAME:
        tempstr = "FILENAME ";
        break;
      case EXEC:
        tempstr = "EXEC ";
        break;
      case SKIPBYTES:
        tempstr = "SKIPBYTES ";
        break;
      case ERROR:
        tempstr = "ERROR: ";
        break;
      case SUCCESS:
        tempstr = "SUCCESS ";
        break;
      case CLOSE:
        tempstr = "CLOSE ";
        break;
      case SSH_VERSION:
        tempstr = "VERSION ";
        break;
      case CANCEL:
        tempstr = "CANCEL ";
        break;
      case FILETIME:
        tempstr = "FILETIME ";
        break;
      default:
        return;
    }

  ok = 0;
  if (command)
    {
      for (pos = command; pos < command + len; pos++)
        {
          if (*pos == '\0')
            {
              ok = 1;
              break;
            }
        }
    }

  request->logging_function (direction == GFTP_DIRECTION_DOWNLOAD ? 
                                 gftp_logging_send : gftp_logging_recv,
                             request->user_data, "%d: %s %s\n", channel, 
                             tempstr, ok ? command : "");
}


static int
ssh_send_command (gftp_request * request, int cmdnum, const char *command, 
                  size_t len)
{
  ssh_parms * params;
  char *buf;
  int clen;

  params = request->protocol_data;
  clen = htonl (len);
  buf = g_malloc (len + 6);
  buf[0] = params->channel;
  buf[1] = cmdnum;
  memcpy (&buf[2], &clen, 4);
  if (command)
    memcpy (&buf[6], command, len);
  ssh_log_command (request, params->channel, cmdnum, command, len, 1);

  if (gftp_write (request, buf, len + 6, request->sockfd) < 0)
    return (-2);

  return 0;

}


static char *
ssh_read_message (gftp_request * request, char *buf, size_t len, int fd)
{
  if (fd <= 0)
    fd = request->sockfd;

  if (gftp_read (request, buf, len, fd) < 0)
    return (NULL);

  return (buf);
}


static int
ssh_read_response (gftp_request * request, ssh_message *message, int fd)
{
  char buf[6];

  if (ssh_read_message (request, buf, 6, fd) == NULL)
    return (-1);

  message->channel = buf[0];
  message->command = buf[1];
  memcpy (&message->len, buf + 2, 4);
  message->len = ntohl (message->len);
  if (message->len > 8192)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                             _("Error: Message size %d too big from server\n"),
                             message->len);
      memset (message, 0, sizeof (*message));
      gftp_disconnect (request);
      return (-1);
    }
  
  message->data = g_malloc (message->len + 1); 

  if (message->len > 0 && ssh_read_message (request, 
                           (char *) message->data, message->len, fd) == NULL)
    return (-1);

  ((char *) message->data)[message->len] = '\0'; 
  ssh_log_command (request, message->channel, message->command, message->data,
                   message->len, 0);
  return (message->command);
}


static char *
ssh_read_line (gftp_request * request)
{
  char *retstr, *pos, tempchar;
  ssh_parms *buffer;

  pos = NULL;
  buffer = request->protocol_data;
  if (!buffer->enddata && (pos = strchr (buffer->pos, '\n')) == NULL)
    return (NULL);

  if (pos == NULL)
    {
      pos = buffer->pos + strlen (buffer->pos) - 1;
      tempchar = '\0';
    }
  else
    tempchar = pos + 1 == '\0' ? '\0' : '1';

  if (*(pos-1) == '\r') 
    *(pos-1) = '\0';
  else 
    *pos = '\0';

  retstr = g_malloc (strlen (buffer->pos) + 1);
  strcpy (retstr, buffer->pos);

  if (tempchar != '\0' && *buffer->pos != '\0')
    {
      buffer->pos = pos + 1;
      while (*buffer->pos == '\r' || *buffer->pos == '\n')
        buffer->pos++;
    }
  else
    {
      g_free (buffer->buffer);
      buffer->buffer = buffer->pos = NULL;
    }
  return (retstr);
}


static void
ssh_destroy (gftp_request * request)
{
  ssh_parms *params;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_SSH_NUM);

  params = request->protocol_data;
  if (params->buffer)
    {
      g_free (params->buffer);
      params->buffer = params->pos = NULL;
    }
}


static int
ssh_chdir (gftp_request * request, const char *directory)
{
  ssh_message message;
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);

  if (directory != NULL && *directory != '\0')
    {  
      if (ssh_send_command (request, CHDIR, directory, 
                            strlen (directory) + 1) < 0) 
        return (-1);

      if ((ret = ssh_read_response (request, &message, -1)) != SUCCESS)
        {
          request->logging_function (gftp_logging_error, request->user_data,
			    _("Could not change remote directory to %s: %s\n"),
	  	            directory, (char *) message.data);
          g_free (message.data);
          return (-1);
        }
      g_free (message.data);
    }

  if (directory != request->directory)
    {
      if (ssh_send_command (request, GETDIR, NULL, 0) < 0)
        return (-1);

      if (ssh_read_response (request, &message, -1) != TELLDIR)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                            _("Could not get current working directory: %s\n"),
		  	    (char *) message.data);
          g_free (message.data);
          return (-1);
        }

      if (request->directory)
        g_free (request->directory);
      request->directory = message.data;
    }

  return (0);
}


static int
ssh_connect (gftp_request * request)
{
  char **args, *tempstr, pts_name[20], *p1, p2, *exepath, port[6];
  int version, fdm, fds, s[2];
  ssh_message message;
  ssh_parms *params;
  pid_t child;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (request->hostname != NULL, -2);
  
  params = request->protocol_data;
  if (request->sockfd > 0)
    return (0);

  request->logging_function (gftp_logging_misc, request->user_data,
			     _("Opening SSH connection to %s\n"),
                             request->hostname);

  if (request->sftpserv_path == NULL ||
      *request->sftpserv_path == '\0') 
    {
      p1 = "";
      p2 = ' ';
    }
  else
    {
      p1 = request->sftpserv_path;
      p2 = '/';
    }

  *port = '\0';
  exepath = g_strdup_printf ("%s%csftpserv", p1, p2);
  args = make_ssh_exec_args (request, exepath, 0, port);

   if (ssh_use_askpass)
    {
      fdm = fds = 0;
      if (socketpair (AF_LOCAL, SOCK_STREAM, 0, s) < 0)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                     _("Cannot create a socket pair: %s\n"),
                                     g_strerror (errno));
          return (-2);
        }
    }
  else
    {
      s[0] = s[1] = 0;
      if ((fdm = ptym_open (pts_name)) < 0)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                _("Cannot open master pty %s: %s\n"), pts_name,
                                g_strerror (errno));
          return (-2);
        }
    }

  if ((child = fork ()) == 0)
    {
      setsid ();
      if (ssh_use_askpass)
        {
          close (s[0]);
          fds = s[1];
        }
      else
        {
          if ((fds = ptys_open (fdm, pts_name)) < 0)
            {
              printf ("Cannot open slave pts %s: %s\n", pts_name,
                      g_strerror (errno));
              return (-1);
            }
          close (fdm);
        }

      dup2 (fds, 0);
      dup2 (fds, 1);
      dup2 (fds, 2);
      if (!ssh_use_askpass && fds > 2)
        close (fds);
      execvp (ssh_prog_name != NULL && *ssh_prog_name != '\0' ?
              ssh_prog_name : "ssh", args);  

      printf (_("Error: Cannot execute ssh: %s\n"), g_strerror (errno));
      return (-1);
    }
  else if (child > 0)
    {
      if (ssh_use_askpass)
        {
          close (s[1]);
          fdm = s[0];
        }
      tty_raw (fdm);
      tempstr = ssh_start_login_sequence (request, fdm);
      if (!tempstr || 
          !(strlen (tempstr) > 4 && strcmp (tempstr + strlen (tempstr) - 5, 
                                            "xsftp") == 0))
        {
          g_free (args);
          g_free (exepath);
          return (-2);
        }
      g_free (args);
      g_free (exepath);
      g_free (tempstr);

      request->sockfd = fdm;

      params->channel = 0;
      version = htonl (7 << 4);
      if (ssh_send_command (request, SSH_VERSION, (char *) &version, 4) < 0)
        return (-2);
      if (ssh_read_response (request, &message, -1) != SSH_VERSION)
        return (-2);
      g_free (message.data);

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
      return (-1);
    }

  ssh_chdir (request, request->directory);

  if (ssh_send_command (request, GETDIR, NULL, 0) < 0)
    return (-1);

  if (ssh_read_response (request, &message, -1) != TELLDIR)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                            _("Could not get current working directory: %s\n"),
                            (char *) message.data);
      g_free (message.data);
      return (-1);
    }

  if (request->directory)
    g_free (request->directory);
  request->directory = message.data;

  return (0);
}


static void
ssh_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_SSH_NUM);

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
}


static off_t 
ssh_get_file (gftp_request * request, const char *filename, int fd,
              off_t startsize)
{
  ssh_message message;
  ssh_parms *params;
  off_t retsize;
  int ret;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->channel = 0; 
  if (ssh_send_command (request, REQUEST, filename, strlen (filename) + 1) < 0)
    return (-1);

  retsize = 0;
  while (1)
    {
      ret = ssh_read_response (request, &message, -1);
      switch (ret)
        {
          case FILESIZE:
            retsize = strtol ((char *) message.data, NULL, 10);
            break;
          case NOFILEMATCH:
            request->logging_function (gftp_logging_misc, request->user_data,
			              _("Remote host could not find file %s\n"),
                                      filename);
            g_free (message.data);
            return (-1);
          case FILENAME:
            params->channel = message.channel;
            startsize = htonl (startsize);
            if (ssh_send_command (request, SKIPBYTES, (char *) &startsize, 4) < 0)
              {
                g_free (message.data);
                return (-1);
              }
            break;
          case -1:
            g_free (message.data);
            return (-2);
        }

      g_free (message.data);
      if (ret == FILETIME)
        break;
    }

  return (retsize);
}


static int
ssh_put_file (gftp_request * request, const char *filename, int fd,
              off_t startsize, off_t totalsize)
{
  ssh_message message;
  ssh_parms *params;
  char tempchar;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (filename != NULL, -2);
  /* fd ignored for this protocol */

  params = request->protocol_data;
  params->channel = 0;
  tempchar = 1;
  if (ssh_send_command (request, SENDFILE, &tempchar, 1) < 0) 
    return (-1);

  params->channel = tempchar;
  if (ssh_send_command (request, FILENAME, filename, strlen (filename) + 1) < 0)
    return (-1);

  if (ssh_read_response (request, &message, -1) < 0)
    {
      g_free (message.data);
      return (-1);
    }

  if (!(message.command == FILEOK && message.len == 1 
       && *(char *) message.data != 0))
    {
      g_free (message.data);
      return (-1);
    }
  g_free (message.data);

  if (ssh_send_command (request, FILESIZE, (void *) &totalsize, 4) < 0)
    return (-1);

  if (ssh_send_command (request, FILEMODE, "rw-r--r--", 9) < 0)
    return (-1);

  if (ssh_send_command (request, FILETIME, (char *) 0, 4) < 0)
    return (-1);

  if (startsize > 0)
    {
      startsize = htonl (startsize);
      /* This protocol only supports files up to 2 gig in size. I truncate
         the file size here just to suppress compiler warnings  */
      if (ssh_send_command (request, SKIPBYTES, 
                            GINT_TO_POINTER ((gint32) startsize), 4) < 0)
        return (-1);
    }

  return (0);
}


static ssize_t 
ssh_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  ssh_message message;
  size_t len;

  switch (ssh_read_response (request, &message, -1))
    {
      case DATA:
        len = size > message.len ? message.len : size;
        memcpy (buf, message.data, len);
        g_free (message.data);
        return (len);
      case ENDDATA:
        g_free (message.data);
        if (ssh_read_response (request, &message, -1) < 0)
          {
            g_free (message.data);
            return (-1);
          }
        g_free (message.data);
        return (0);
      case -1:
        g_free (message.data);
        return (-1);
      default:
        g_free (message.data);
        request->logging_function (gftp_logging_misc, request->user_data,
			      _("Received unexpected response from server\n"));
        gftp_disconnect (request);
        return (0);
    }
}


static ssize_t 
ssh_put_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
  if (size == 0)
    {
      if (ssh_send_command (request, ENDDATA, NULL, 0) < 0)
        return (-2);
      if (ssh_send_command (request, NOFILEMATCH, NULL, 0) < 0)
        return (-2);
      return (0);
    }
  return (ssh_send_command (request, DATA, buf, size) == 0 ? size : 0);
}


static int
ssh_end_transfer (gftp_request * request)
{
  ssh_parms *params;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);

  params = request->protocol_data;
  if (params->buffer)
    g_free (params->buffer);
  params->buffer = params->pos = NULL;
  params->enddata = 0;

  return (0);
}


static size_t 
ssh_remove_spaces ( char *string )
{
  size_t len;
  char *pos;

  for (pos = string, len = 0; *pos != '\0'; len++, pos++)
    {
      if (*pos == ' ')
        *pos = '\0';
    }
  return (len);
}


static int
ssh_list_files (gftp_request * request)
{
  ssh_message message;
  ssh_parms *params;
  char *tempstr;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);

  params = request->protocol_data;
  params->enddata = 0;
  params->channel = 0;
  request->logging_function (gftp_logging_misc, request->user_data,
			     _("Retrieving directory listing...\n"));

  tempstr = g_strdup ("/bin/ls -al");
  len = ssh_remove_spaces (tempstr);

  if (ssh_send_command (request, EXEC, tempstr, len) < 0) 
    {
      g_free (tempstr);
      return (-1);
    }
  g_free (tempstr);

  if (ssh_read_response (request, &message, -1) != STREAM)
    {
      g_free (message.data);
      request->logging_function (gftp_logging_misc, request->user_data,
			      _("Received unexpected response from server\n"));
      gftp_disconnect (request);
      return (-2);
    }
  g_free (message.data);

  return (0);
}


static int
ssh_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  char *tempstr, *pos;
  ssh_message message;
  ssh_parms *params;
  int ret, len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (fle != NULL, -2);

  params = request->protocol_data;
  if (request->last_dir_entry)
    {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
      request->last_dir_entry_len = 0;
    }

  len = 0;
  while (1)
    {
      if (params->enddata && params->buffer == NULL)
        {
          request->logging_function (gftp_logging_misc, request->user_data,
			         _("Finished retrieving directory listing\n"));
          params->enddata = 0;
          return (0);
        }

      if (!params->enddata && 
          (!params->pos || strchr (params->pos, '\n') == NULL)) 
        {
          if ((ret = ssh_read_response (request, &message, fd)) < 0)
            {
              if (message.data)
                g_free (message.data);
              return (-2);
            }

          if (!request->cached)
            {
              request->last_dir_entry_len = message.len + 6;
              request->last_dir_entry = g_malloc (request->last_dir_entry_len + 1);
              request->last_dir_entry[0] = message.channel;
              request->last_dir_entry[1] = message.command;
              len = htonl (message.len);
              memcpy (&request->last_dir_entry[2], &len, 4);
              memcpy (&request->last_dir_entry[6], message.data, message.len);
            }

          if (ret == DATA)
            {
              if (params->pos == NULL || *params->pos == '\0')
                {
                  if (params->buffer)
                    g_free (params->buffer);
                  params->buffer = params->pos = message.data;
                }
              else
                {
                  tempstr = params->buffer;
                  params->buffer = g_malloc (strlen (params->pos) + 
                                     strlen ((char *) message.data) + 1);
                  strcpy (params->buffer, params->pos);
                  strcat (params->buffer, (char *) message.data);
                  g_free (tempstr);
                  g_free (message.data);
                  params->pos = params->buffer;
                } 
            }
          else
            g_free (message.data);
        }
      else
        ret = DATA;

      switch (ret)
        {
          case DATA:
            if ((tempstr = ssh_read_line (request)) == NULL)
              continue;
            pos = tempstr;
            while (*pos == ' ' || *pos == '\t')
              pos++;
            if (*pos == '\0')
              {
                g_free (tempstr);
                break;
              }

            if (gftp_parse_ls (tempstr, fle) != 0)
	      {
	        if (strncmp (tempstr, "total", strlen ("total")) &&
		    strncmp (tempstr, _("total"), strlen (_("total"))))
	          request->logging_function (gftp_logging_misc, 
                                       request->user_data,
				       _("Warning: Cannot parse listing %s\n"),
				       tempstr);
	        gftp_file_destroy (fle);
                g_free (tempstr);
                tempstr = NULL;
	        continue;
              }
            len = strlen (tempstr);
            g_free (tempstr);
            break;
          case ENDDATA:
            params->enddata = 1;
            break;
          default:
            request->logging_function (gftp_logging_misc, request->user_data,
			      _("Received unexpected response from server\n"));
            gftp_disconnect (request);
            return (-2);
	}

      if (ret == DATA)
        break;
    }

  return (len);
}


static char *
ssh_exec (gftp_request * request, const char *command, size_t len)
{
  ssh_message message;
  char *err, *pos;
  int ret;

  g_return_val_if_fail (request != NULL, NULL);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, NULL);
  g_return_val_if_fail (command != NULL, NULL);

  err = NULL;
  if (ssh_send_command (request, EXEC, command, len) < 0)
    return (NULL);

  while (1)
    {
      ret = ssh_read_response (request, &message, -1);
      switch (ret)
        {
          case -1: 
            g_free (message.data);
            gftp_disconnect (request);
            return (message.data);
          case DATA:
          case ERROR:
            pos = (char *) message.data+message.len-1;
            if (*pos == '\n')
              *pos = '\0';

            if (err != NULL)
              {
                err = g_realloc (err, strlen (message.data) + strlen (err) + 1);
                strcat (err, message.data);
              }
            else
              {
                err = g_malloc (strlen (message.data) + 1);
                strcpy (err, message.data);
              }
            if (ret == ERROR)
              {
                g_free (message.data);
                return (err);
              }
            break;
          case SUCCESS:
            g_free (message.data);
            return (err);
          case STREAM:
            break;
          case ENDDATA:
            g_free (message.data);
            return (err);
        }
      g_free (message.data);
    }
}


static int 
ssh_rmdir (gftp_request * request, const char *directory)
{ 
  char *tempstr, *pos, *ret;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (directory != NULL, -2);

  tempstr = g_strconcat ("/bin/rmdir ", directory, NULL);
  len = strlen (tempstr);
  pos = tempstr;
  while ((pos = strchr (pos, ' ')) != NULL)
    *pos++ = '\0';
  if ((ret = ssh_exec (request, tempstr, len)) != NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				_("Error: Could not remove directory %s: %s\n"),
				directory, ret);
      g_free (tempstr);
      g_free (ret);
      return (-2);
    }
  else 
    request->logging_function (gftp_logging_misc, request->user_data,
			       _("Successfully removed %s\n"), directory);
  g_free (tempstr);
  return (0);
}


static int 
ssh_rmfile (gftp_request * request, const char *file)
{ 
  char *tempstr, *pos, *ret;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (file != NULL, -2);

  tempstr = g_strconcat ("/bin/rm -f ", file, NULL);
  len = strlen (tempstr);
  pos = tempstr;
  while ((pos = strchr (pos, ' ')) != NULL)
    *pos++ = '\0';
  if ((ret = ssh_exec (request, tempstr, len)) != NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Error: Could not remove file %s: %s\n"),
				 file, ret);
      g_free (tempstr);
      g_free (ret);
      return (-2);
    }
  else
    request->logging_function (gftp_logging_misc, request->user_data,
			       _("Successfully removed %s\n"), file);
  g_free (tempstr);
  return (0);
}


static int 
ssh_mkdir (gftp_request * request, const char *newdir)
{
  char *tempstr, *pos, *ret;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (newdir != NULL, -2);

  tempstr = g_strconcat ("/bin/mkdir ", newdir, NULL);
  len = strlen (tempstr);
  pos = tempstr;
  while ((pos = strchr (pos, ' ')) != NULL)
    *pos++ = '\0';
  if ((ret = ssh_exec (request, tempstr, len)) != NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
				 _("Error: Could not make directory %s: %s\n"),
				 newdir, ret);
      g_free (tempstr);
      g_free (ret);
      return (-2);
    }
  else
    request->logging_function (gftp_logging_misc, request->user_data,
                               _("Successfully made directory %s\n"),
                               newdir);
  g_free (tempstr);
  return (0);
}


static int 
ssh_rename (gftp_request * request, const char *oldname, const char *newname )
{
  char *tempstr, *pos, *ret;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (oldname != NULL, -2);
  g_return_val_if_fail (newname != NULL, -2);

  tempstr = g_strconcat ("/bin/mv ", oldname, " ", newname, NULL);
  len = strlen (tempstr);
  pos = tempstr;
  while ((pos = strchr (pos, ' ')) != NULL)
    *pos++ = '\0';
  if ((ret = ssh_exec (request, tempstr, len)) != NULL)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
				 _("Error: Could not rename %s to %s: %s\n"),
				 oldname, newname, ret);
      g_free (tempstr);
      g_free (ret);
      return (-2);
    }
  else
    request->logging_function (gftp_logging_misc, request->user_data,
			       _("Successfully renamed %s to %s\n"),
                               oldname, newname);
  g_free (tempstr);
  return (0);
}


static int 
ssh_chmod (gftp_request * request, const char *file, int mode)
{
  char *tempstr, *pos, *ret;
  size_t len;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (request->protonum == GFTP_SSH_NUM, -2);
  g_return_val_if_fail (file != NULL, -2);

  tempstr = g_malloc (strlen (file) + (mode / 10) + 14);
  sprintf (tempstr, "/bin/chmod %d %s", mode, file);
  len = strlen (tempstr);
  pos = tempstr;
  while ((pos = strchr (pos, ' ')) != NULL)
    *pos++ = '\0';
  if ((ret = ssh_exec (request, tempstr, len)) != NULL)
    {
      request->logging_function (gftp_logging_misc, request->user_data,
			_("Error: Could not change mode of %s to %d: %s\n"),
			file, mode, ret);
      g_free (tempstr);
      g_free (ret);
      return (-2);
    }
  else
    request->logging_function (gftp_logging_misc, request->user_data,
			       _("Successfully changed mode of %s to %d\n"),
                               file, mode);
  g_free (tempstr);
  return (0);
}


static void
ssh_set_config_options (gftp_request * request)
{
  if (request->sftpserv_path != NULL)
    {
      if (ssh1_sftp_path != NULL &&
          strcmp (ssh1_sftp_path, request->sftpserv_path) == 0)
        return;

      g_free (request->sftpserv_path);
      request->sftpserv_path = NULL;
    }

  if (ssh1_sftp_path != NULL)
    request->sftpserv_path = g_strdup (ssh1_sftp_path);

  request->need_userpass = ssh_need_userpass;
}


void
ssh_init (gftp_request * request)
{
  g_return_if_fail (request != NULL);

  request->protonum = GFTP_SSH_NUM;
  request->init = ssh_init;
  request->destroy = ssh_destroy;
  request->connect = ssh_connect;
  request->disconnect = ssh_disconnect;
  request->get_file = ssh_get_file;
  request->put_file = ssh_put_file;
  request->transfer_file = NULL;
  request->get_next_file_chunk = ssh_get_next_file_chunk;
  request->put_next_file_chunk = ssh_put_next_file_chunk;
  request->end_transfer = ssh_end_transfer;
  request->abort_transfer = NULL; /* FIXME */
  request->list_files = ssh_list_files;
  request->get_next_file = ssh_get_next_file;
  request->set_data_type = NULL;
  request->get_file_size = NULL;
  request->chdir = ssh_chdir;
  request->rmdir = ssh_rmdir;
  request->rmfile = ssh_rmfile;
  request->mkdir = ssh_mkdir;
  request->rename = ssh_rename;
  request->chmod = ssh_chmod;
  request->set_file_time = NULL;
  request->site = NULL;
  request->parse_url = NULL;
  request->set_config_options = ssh_set_config_options;
  request->swap_socks = NULL;
  request->url_prefix = "ssh";
  request->protocol_name = "SSH";
  request->need_hostport = 1;
  request->need_userpass = ssh_need_userpass;
  request->use_cache = 1;
  request->use_threads = 1;
  request->always_connected = 0;
  request->protocol_data = g_malloc0 (sizeof (ssh_parms));
  gftp_set_config_options (request);
}


