/*****************************************************************************/
/*  misc.c - general purpose routines                                        */
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
#include "options.h"
static const char cvsid[] = "$Id$";

/* FIXME - this isn't right for all locales. Use glib's printf instead */
char *
insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  char *frompos, *topos, src[50], *dest;
  int len, num, rem, i;

  if (dest_str != NULL)
    *dest_str = '\0';
  len = (number > 0 ? log10 (number) : 0) + 2;

  if (len <= 0) 
    {
      if (dest_str != NULL)
        strncpy (dest_str, "0", dest_len);
      else
        dest_str = g_strdup ("0");
      return (dest_str);
    }

  len += len / 3;
  if (dest_str != NULL && len > dest_len)
    {
      
      for (i=0; i<dest_len - 1; i++)
        dest_str[i] = 'X';
      dest_str[dest_len - 1] = '\0';
      return (dest_str);
    }

  if (dest_str == NULL)
    dest = g_malloc0 (len);
  else
    dest = dest_str;

#if defined (_LARGEFILE_SOURCE)
  g_snprintf (src, sizeof (src), "%lld", number);
#else
  g_snprintf (src, sizeof (src), "%ld", number);
#endif

  num = strlen (src) / 3 - 1;
  rem = strlen (src) % 3;
  frompos = src;
  topos = dest;
  for (i = 0; i < rem; i++)
    *topos++ = *frompos++;

  if (*frompos != '\0')
    {
      if (rem != 0)
	*topos++ = ',';
      while (num > 0)
	{
	  for (i = 0; i < 3; i++)
	    *topos++ = *frompos++;
	  *topos++ = ',';
	  num--;
	}
      for (i = 0; i < 3; i++)
	*topos++ = *frompos++;
    }
  *topos = '\0';
  return (dest);
}


char *
alltrim (char *str)
{
  char *pos, *newpos;
  int diff;

  pos = str + strlen (str) - 1;
  while (pos >= str && (*pos == ' ' || *pos == '\t'))
    *pos-- = '\0';

  pos = str;
  diff = 0;
  while (*pos++ == ' ')
    diff++;

  if (diff == 0)
    return (str);

  pos = str + diff;
  newpos = str;
  while (*pos != '\0')
    *newpos++ = *pos++;
  *newpos = '\0';

  return (str);
}


char *
expand_path (const char *src)
{
  char *str, *pos, *endpos, *prevpos, *newstr, *tempstr, tempchar;
  struct passwd *pw;

  pw = NULL;
  str = g_malloc (strlen (src) + 1);
  strcpy (str, src);

  if (*str == '~')
    {
      if (*(str + 1) == '/' || *(str + 1) == '\0')
	pw = getpwuid (geteuid ());
      else
	{
          if ((pos = strchr (str, '/')) != NULL)
	    *pos = '\0';

	  pw = getpwnam (str + 1);

          if (pos != NULL)
            *pos = '/';
	}
    }

  endpos = str;
  newstr = NULL;
  while ((pos = strchr (endpos, '/')) != NULL)
    {
      pos++;
      while (*pos == '/')
        pos++;
      if ((endpos = strchr (pos, '/')) == NULL)
	endpos = pos + strlen (pos);
      tempchar = *endpos;
      *endpos = '\0';
      if (strcmp (pos, "..") == 0)
	{
	  *(pos - 1) = '\0';
	  if (newstr != NULL && (prevpos = strrchr (newstr, '/')) != NULL)
	    *prevpos = '\0';
	}
      else if (strcmp (pos, ".") != 0)
	{
	  if (newstr == NULL)
	    newstr = g_strdup (pos - 1);
	  else
	    {
	      tempstr = g_strconcat (newstr, pos - 1, NULL);
	      g_free (newstr);
	      newstr = tempstr;
	    }
	}
      *endpos = tempchar;
      if (*endpos == '\0')
	break;
      endpos = pos + 1;
    }

  if (newstr == NULL || *newstr == '\0')
    {
      if (newstr != NULL)
	g_free (newstr);
      newstr = g_malloc0 (2);
      *newstr = '/';
    }

  g_free (str);

  if (pw != NULL)
    {
      if ((pos = strchr (newstr, '/')) == NULL)
	{
	  str = g_malloc (strlen (pw->pw_dir) + 1);
	  strcpy (str, pw->pw_dir);
	}
      else
	str = g_strconcat (pw->pw_dir, pos, NULL);

      g_free (newstr);
      newstr = str;
    }

  return (newstr);
}


void
remove_double_slashes (char *string)
{
  char *newpos, *oldpos;
  size_t len;

  oldpos = newpos = string;
  while (*oldpos != '\0')
    {
      *newpos++ = *oldpos++;
      if (*oldpos == '\0')
	break;
      while (*(newpos - 1) == '/' && *(oldpos) == '/')
	oldpos++;
    }
  *newpos = '\0';

  len = strlen (string);
  if (string[len - 1] == '/')
    string[len - 1] = '\0';
}


void
make_nonnull (char **str)
{
  if (*str == NULL)
    *str = g_malloc0 (1);
}


int
copyfile (char *source, char *dest)
{
  int srcfd, destfd;
  char buf[8192];
  ssize_t n;

  if ((srcfd = open (source, O_RDONLY)) == -1)
    {
      printf (_("Error: Cannot open local file %s: %s\n"),
              source, g_strerror (errno));
      exit (1);
    }

  if ((destfd = open (dest, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
    {
      printf (_("Error: Cannot open local file %s: %s\n"),
              dest, g_strerror (errno));
      close (srcfd);
      exit (1);
    }

  while ((n = read (srcfd, buf, sizeof (buf))) > 0)
    {
      if (write (destfd, buf, n) == -1)
        {
          printf (_("Error: Could not write to socket: %s\n"), 
                  g_strerror (errno));
          exit (1);
        }
    }

  if (n == -1)
    {
      printf (_("Error: Could not read from socket: %s\n"), g_strerror (errno));
      exit (1);
    }

  if (close (srcfd) == -1)
    {
      printf (_("Error closing file descriptor: %s\n"), g_strerror (errno));
      exit (1);
    }

  if (close (destfd) == -1)
    {
      printf (_("Error closing file descriptor: %s\n"), g_strerror (errno));
      exit (1);
    }

  return (1);
}


/* FIXME - is there a replacement for this */
int
gftp_match_filespec (char *filename, char *filespec)
{
  char *filepos, *wcpos, *pos, *newpos, search_str[20];
  size_t len, curlen;
  
  if (filename == NULL || *filename == '\0' || 
      filespec == NULL || *filespec == '\0') 
    return(1);

  filepos = filename;
  wcpos = filespec;
  while(1) 
    {
      if (*wcpos == '\0') 
        return (1);
      else if (*filepos == '\0') 
        return(0);
      else if(*wcpos == '?') 
        {
          wcpos++;
          filepos++;
        }
      else if(*wcpos == '*' && *(wcpos+1) == '\0') 
        return(1);
      else if(*wcpos == '*') 
        {
          len = sizeof (search_str);
          for (pos = wcpos + 1, newpos = search_str, curlen = 0;
               *pos != '*' && *pos != '?' && *pos != '\0' && curlen < len;
               curlen++, *newpos++ = *pos++);
          *newpos = '\0';

          if ((filepos = strstr (filepos, search_str)) == NULL)
            return(0);
          wcpos += curlen + 1;
          filepos += curlen;
        }
      else if(*wcpos++ != *filepos++) 
        return(0);
    }
  return (1);
}


int
gftp_parse_command_line (int *argc, char ***argv)
{
  if (*argc > 1)
    {
      if (strcmp (argv[0][1], "--help") == 0 || 
          strcmp (argv[0][1], "-h") == 0)
	gftp_usage ();
      else if (strcmp (argv[0][1], "--version") == 0 || 
               strcmp (argv[0][1], "-v") == 0)
	{
	  printf ("%s\n", version);
	  exit (0);
	}
    }
  return (0);
}


void
gftp_usage (void)
{
  printf (_("usage: gftp [[protocol://][user:[pass]@]site[:port][/directory]]\n"));
  exit (0);
}


char *
get_xpm_path (char *filename, int quit_on_err)
{
  char *tempstr, *exfile;

  tempstr = g_strconcat (BASE_CONF_DIR, "/", filename, NULL);
  exfile = expand_path (tempstr);
  g_free (tempstr);
  if (access (exfile, F_OK) != 0)
    {
      g_free (exfile);
      tempstr = g_strconcat (SHARE_DIR, "/", filename, NULL);
      exfile = expand_path (tempstr);
      g_free (tempstr);
      if (access (exfile, F_OK) != 0)
	{
	  g_free (exfile);
	  exfile = g_strconcat ("/usr/share/icons/", filename, NULL);
	  if (access (exfile, F_OK) != 0)
	    {
	      g_free (exfile);
	      if (!quit_on_err)
		return (NULL);

	      printf (_("gFTP Error: Cannot find file %s in %s or %s\n"),
		      filename, SHARE_DIR, BASE_CONF_DIR);
	      exit (1);
	    }
	}
    }
  return (exfile);
}


gint
string_hash_compare (gconstpointer path1, gconstpointer path2)
{
  return (strcmp ((char *) path1, (char *) path2) == 0);
}


guint
string_hash_function (gconstpointer key)
{
  guint ret;
  int i;

  ret = 0;
  for (i=0; ((char *) key)[i] != '\0' && i < 3; i++)
    ret += ((char *) key)[i];

  return (ret);
}


void
free_file_list (GList * filelist)
{
  gftp_file * tempfle;
  GList * templist;

  templist = filelist;
  while (templist != NULL)
    {
      tempfle = templist->data;
      free_fdata (tempfle);
      templist = templist->next;
    }
  g_list_free (filelist);
}


void
free_fdata (gftp_file * fle)
{
  if (fle->file)
    g_free (fle->file);
  if (fle->user)
    g_free (fle->user);
  if (fle->group)
    g_free (fle->group);
  if (fle->attribs)
    g_free (fle->attribs);
  if (fle->destfile)
    g_free (fle->destfile);
  if (fle->fd > 0)
    close (fle->fd); /* FIXME - need to log a failure */
  g_free (fle);
}


gftp_file *
copy_fdata (gftp_file * fle)
{
  gftp_file * newfle;

  newfle = g_malloc0 (sizeof (*newfle));
  memcpy (newfle, fle, sizeof (*newfle));

  if (fle->file)
    newfle->file = g_strdup (fle->file);

  if (fle->user)
    newfle->user = g_strdup (fle->user);

  if (fle->group)
    newfle->group = g_strdup (fle->group);

  if (fle->attribs)
    newfle->attribs = g_strdup (fle->attribs);

  if (fle->destfile)
    newfle->destfile = g_strdup (fle->destfile);

  return (newfle);
}


int
compare_request (gftp_request * request1, gftp_request * request2,
                 int compare_dirs)
{
  char *strarr[3][2];
  int i, ret;

  ret = 1;
  if (request1->protonum == request2->protonum &&
      request1->port == request2->port)
    {
      strarr[0][0] = request1->hostname;
      strarr[0][1] = request2->hostname;
      strarr[1][0] = request1->username;
      strarr[1][1] = request2->username;
      if (compare_dirs)
        {
          strarr[2][0] = request1->directory;
          strarr[2][1] = request2->directory;
        }
      else
        strarr[2][0] = strarr[2][1] = "";

      for (i = 0; i < 3; i++)
        {
          if ((strarr[i][0] && !strarr[i][1]) ||
              (!strarr[i][0] && strarr[i][1]))
            {
              ret = 0;
              break;
            }

          if (strarr[i][0] && strarr[i][1] &&
              strcmp (strarr[i][0], strarr[i][1]) != 0)
            {
              ret = 0;
              break;
            }
        }
    }
  else
    ret = 0;
  return (ret);
}


void
free_tdata (gftp_transfer * tdata)
{
  if (tdata->statmutex)
    g_free (tdata->statmutex);
  if (tdata->structmutex)
    g_free (tdata->structmutex);
  if (tdata->fromreq != NULL)
    gftp_request_destroy (tdata->fromreq, 1);
  if (tdata->toreq != NULL)
    gftp_request_destroy (tdata->toreq, 1);
  free_file_list (tdata->files);
  g_free (tdata);
}


gftp_request * 
copy_request (gftp_request * req)
{
  gftp_request * newreq;

  newreq = gftp_request_new ();

  if (req->hostname)
    newreq->hostname = g_strdup (req->hostname);
  if (req->username)
    newreq->username = g_strdup (req->username);
  if (req->password)
    newreq->password = g_strdup (req->password);
  if (req->account)
    newreq->account = g_strdup (req->account);
  if (req->directory)
    newreq->directory = g_strdup (req->directory);
  newreq->port = req->port;
  newreq->data_type = req->data_type;
  newreq->use_proxy = req->use_proxy;
  gftp_set_proxy_config (newreq, req->proxy_config);
  newreq->transfer_type = req->transfer_type;
  newreq->network_timeout = req->network_timeout;
  newreq->retries = req->retries;
  newreq->sleep_time = req->sleep_time;
  newreq->passive_transfer = req->passive_transfer;
  newreq->maxkbs = req->maxkbs;
  newreq->logging_function = req->logging_function;

  if (req->sftpserv_path != NULL)
    newreq->sftpserv_path = g_strdup (req->sftpserv_path);

  req->init (newreq);

  return (newreq);
}


int
ptym_open (char *pts_name)
{
  int fd;

#ifdef __sgi
  char *tempstr;

  if ((tempstr = _getpty (&fd, O_RDWR, 0600, 0)) == NULL)
    return (-1);

  strcpy (pts_name, tempstr);
  return (fd);

#else /* !__sgi */

#ifdef HAVE_GRANTPT

  char *tempstr;

  strcpy (pts_name, "/dev/ptmx");
  if ((fd = open (pts_name, O_RDWR)) < 0)
    return (-1);

  if (grantpt (fd) < 0)
    {
      close (fd);
      return (-1);
    }

  if (unlockpt (fd) < 0)
    {
      close (fd);
      return (-1);
    }

  if ((tempstr = ptsname (fd)) == NULL)
    {
      close (fd);
      return (-1);
    }

  strcpy (pts_name, tempstr);
  return (fd);

#else /* !GRANTPT */

  char *pos1, *pos2;

  strcpy (pts_name, "/dev/ptyXY");
  for (pos1 = "pqrstuvwxyzPQRST"; *pos1 != '\0'; pos1++) 
    {
      pts_name[8] = *pos1;
      for (pos2 = "0123456789abcdef"; *pos2 != '\0'; pos2++)
        {
          pts_name[9] = *pos2;
          if ((fd = open (pts_name, O_RDWR)) < 0)
            continue;
          pts_name[5] = 't';
          return (fd);
        }
    }
  return (-1);

#endif /* GRANTPT */

#endif /* __sgi */

}


int
ptys_open (int fdm, char *pts_name)
{
  int fds;

#if !defined (HAVE_GRANTPT) && !defined (__sgi)

  chmod (pts_name, S_IRUSR | S_IWUSR);
  chown (pts_name, getuid (), -1);

#endif

  if ((fds = open (pts_name, O_RDWR)) < 0)
    {
      close (fdm);
      return (-1);
    }

#ifdef HAVE_GRANTPT
  /* I intentionally ignore these errors */
  ioctl (fds, I_PUSH, "ptem");
  ioctl (fds, I_PUSH, "ldterm");
  ioctl (fds, I_PUSH, "ttcompat");
#endif

#if !defined(HAVE_GRANTPT) && !defined (__sgi) && defined(TIOCSCTTY) && !defined(CIBAUD)

  if (ioctl (fds, TIOCSCTTY, (char *) 0) < 0)
    {
      close (fdm);
      return (-1);
    }

#endif

  return (fds);
}


int
tty_raw (int fd)
{
  struct termios buf;

  if (tcgetattr (fd, &buf) < 0)
    return (-1);

  buf.c_iflag |= IGNPAR;
  buf.c_iflag &= ~(ICRNL | ISTRIP | IXON | IGNCR | IXANY | IXOFF | INLCR);
  buf.c_lflag &= ~(ECHO | ICANON | ISIG | ECHOE | ECHOK | ECHONL);
#ifdef IEXTEN
  buf.c_lflag &= ~(IEXTEN);
#endif

  buf.c_oflag &= ~(OPOST);
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;

  if (tcsetattr (fd, TCSADRAIN, &buf) < 0)
    return (-1);
  return (0);
}


/* We have the caller send us a pointer to a string so we can write the port
   into it. It makes it easier so we don't have to worry about freeing it 
   later on, the caller can just send us an auto variable, The string
   should be at least 6 chars. I know this is messy... */

char **
make_ssh_exec_args (gftp_request * request, char *execname, 
                    int use_sftp_subsys, char *portstring)
{
  char **args, *oldstr, *tempstr;
  struct servent serv_struct;
  int i, j;

  args = g_malloc (sizeof (char *) * (num_ssh_extra_params + 15));

  args[0] = ssh_prog_name != NULL && *ssh_prog_name != '\0' ? 
            ssh_prog_name : "ssh";
  i = 1;
  tempstr = g_strdup (args[0]);

  if (ssh_extra_params_list != NULL)
    {
      for (j=0; ssh_extra_params_list[j] != NULL; j++)
        {
          oldstr = tempstr;
          args[i++] = ssh_extra_params_list[j];
          tempstr = g_strconcat (oldstr, " ", ssh_extra_params_list[j], NULL);
          g_free (oldstr);
        }
    }

  oldstr = tempstr;
  tempstr = g_strconcat (oldstr, " -e none", NULL);
  g_free (oldstr);
  args[i++] = "-e";
  args[i++] = "none";

  if (request->username && *request->username != '\0')
    {
      oldstr = tempstr;
      tempstr = g_strconcat (oldstr, " -l ", request->username, NULL);
      g_free (oldstr);
      args[i++] = "-l";
      args[i++] = request->username;
    }

  if (request->port != 0)
    {
      g_snprintf (portstring, 6, "%d", request->port);
      oldstr = tempstr;
      tempstr = g_strconcat (oldstr, " -p ", portstring, NULL);
      g_free (oldstr);
      args[i++] = "-p";
      args[i++] = portstring;
    }
  else
    {
      if (!r_getservbyname ("ssh", "tcp", &serv_struct, NULL))
        request->port = 22;
      else
        request->port = ntohs (serv_struct.s_port);
    }

  if (use_sftp_subsys)
    {
      oldstr = tempstr;
      tempstr = g_strconcat (oldstr, " ", request->hostname, " -s sftp", NULL);
      g_free (oldstr);
      args[i++] = request->hostname;
      args[i++] = "-s";
      args[i++] = "sftp";
      args[i] = NULL;
    }
  else
    {
      oldstr = tempstr;
      tempstr = g_strconcat (oldstr, " ", request->hostname, " \"", execname, 
                             "\"", NULL);
      g_free (oldstr);
      args[i++] = request->hostname;
      args[i++] = execname;
      args[i] = NULL;
    }

  request->logging_function (gftp_logging_misc, request->user_data, 
                             _("Running program %s\n"), tempstr);
  g_free (tempstr);
  return (args);
}

#define SSH_LOGIN_BUFSIZE	200
#define SSH_ERROR_BADPASS	-1
#define SSH_ERROR_QUESTION	-2
#define SSH_WARNING 		-3

char *
ssh_start_login_sequence (gftp_request * request, int fd)
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

  if (gftp_set_sockblocking (request, fd, 1) == -1)
    return (NULL);

  pwstr = g_strconcat (request->password, "\n", NULL);

  errno = 0;
  while (1)
    {
      if ((rd = gftp_read (request, tempstr + diff, rem - 1, fd)) <= 0)
        {
          ok = 0;
          break;
        }
      rem -= rd;
      diff += rd;
      tempstr[diff] = '\0'; 

      if (diff > 11 && strcmp (tempstr + diff - 10, "password: ") == 0)
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
          if (gftp_write (request, pwstr, strlen (pwstr), fd) < 0)
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
          if (gftp_write (request, pwstr, strlen (pwstr), fd) < 0)
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

gint64
hton64 (gint64 val)
{
  gint64 num;
  char *pos;

  num = 0;
  pos = (char *) &num;
  pos[0] = (val >> 56) & 0xff;
  pos[1] = (val >> 48) & 0xff;
  pos[2] = (val >> 40) & 0xff;
  pos[3] = (val >> 32) & 0xff;
  pos[4] = (val >> 24) & 0xff;
  pos[5] = (val >> 16) & 0xff;
  pos[6] = (val >> 8) & 0xff;
  pos[7] = val & 0xff;
  return (num);
}

#endif


static gint
gftp_file_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcmp (f1->file, f2->file));
}


static gint
gftp_file_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcmp (f1->file, f2->file);
  return (ret * -1);
}


static gint
gftp_user_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcmp (f1->user, f2->user));
}


static gint
gftp_user_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcmp (f1->user, f2->user);
  return (ret * -1);
}


static gint
gftp_group_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcmp (f1->group, f2->group));
}


static gint
gftp_group_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcmp (f1->group, f2->group);
  return (ret * -1);
}


static gint
gftp_attribs_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcmp (f1->attribs, f2->attribs));
}


static gint
gftp_attribs_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcmp (f1->attribs, f2->attribs);
  return (ret * -1);
}


static gint
gftp_size_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->size < f2->size)
    return (-1);
  else if (f1->size == f2->size)
    return (0);
  else
    return (1);
}


static gint
gftp_size_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->size < f2->size)
    return (1);
  else if (f1->size == f2->size)
    return (0);
  else
    return (-1);
}


static gint
gftp_datetime_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->datetime < f2->datetime)
    return (-1);
  else if (f1->datetime == f2->datetime)
    return (0);
  else
    return (1);
}


static gint
gftp_datetime_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->datetime < f2->datetime)
    return (1);
  else if (f1->datetime == f2->datetime)
    return (0);
  else
    return (-1);
}


GList *
gftp_sort_filelist (GList * filelist, int column, int asds)
{
  GList * files, * dirs, * dotdot, * tempitem, * insitem;
  GCompareFunc sortfunc;
  gftp_file * tempfle;

  files = dirs = dotdot = NULL;

  if (column == GFTP_SORT_COL_FILE)
    sortfunc = asds ?  gftp_file_sort_function_as : gftp_file_sort_function_ds;
  else if (column == GFTP_SORT_COL_SIZE)
    sortfunc = asds ?  gftp_size_sort_function_as : gftp_size_sort_function_ds;
  else if (column == GFTP_SORT_COL_USER)
    sortfunc = asds ?  gftp_user_sort_function_as : gftp_user_sort_function_ds;
  else if (column == GFTP_SORT_COL_GROUP)
    sortfunc = asds ?
                gftp_group_sort_function_as : gftp_group_sort_function_ds;
  else if (column == GFTP_SORT_COL_DATETIME)
    sortfunc = asds ?
                gftp_datetime_sort_function_as : gftp_datetime_sort_function_ds;  else /* GFTP_SORT_COL_ATTRIBS */
    sortfunc = asds ? 
                gftp_attribs_sort_function_as : gftp_attribs_sort_function_ds;

  for (tempitem = filelist; tempitem != NULL; )
    {
      tempfle = tempitem->data;
      insitem = tempitem;
      tempitem = tempitem->next;
      insitem->next = NULL;

      if (dotdot == NULL && strcmp (tempfle->file, "..") == 0)
        dotdot = insitem;
      else if (sort_dirs_first && tempfle->isdir)
        {
          insitem->next = dirs;
          dirs = insitem;
        }
      else
        {
          insitem->next = files;
          files = insitem;
        }
    }

  if (dirs != NULL)
    dirs = g_list_sort (dirs, sortfunc);
  if (files != NULL)
    files = g_list_sort (files, sortfunc);

  filelist = dotdot;

  if (filelist == NULL)
    filelist = dirs;
  else
    filelist = g_list_concat (filelist, dirs);

  if (filelist == NULL)
    filelist = files;
  else
    filelist = g_list_concat (filelist, files);

  /* I haven't check this, but I'm pretty sure some older versions of glib
     had a bug that the prev pointer wasn't being sent to NULL */
  filelist->prev = NULL;
  return (filelist);
}

