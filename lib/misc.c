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

char *
insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  char *frompos, *topos, src[50], *dest;
  int len, num, rem, i;

  if (dest_str != NULL)
    *dest_str = '\0';
  len = (number > 0 ? log10 (number) : 0) + 1;

  if (len <= 0) 
    {
      if (dest_str != NULL)
        strncpy (dest_str, "0", dest_len);
      else
        dest_str = g_strconcat ("0", NULL);
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


long
file_countlf (int filefd, long endpos)
{
  char tempstr[255];
  long num, mypos;
  ssize_t n;
  int i;

  mypos = num = 0;
  lseek (filefd, 0, SEEK_SET);
  while ((n = read (filefd, tempstr, sizeof (tempstr))) > 0)
    {
      for (i = 0; i < n; i++)
	{
	  if ((tempstr[i] == '\n') && (i > 0) && (tempstr[i - 1] != '\r')
	      && (endpos == 0 || mypos + i <= endpos))
	    ++num;
	}
      mypos += n;
    }
  lseek (filefd, 0, SEEK_SET);
  return (num);
}


char *
alltrim (char *str)
{
  char *pos, *newpos;
  int diff;

  pos = str + strlen (str) - 1;
  while (pos >= str && *pos == ' ')
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
	    newstr = g_strconcat (pos - 1, NULL);
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
  if (string[strlen (string) - 1] == '/')
    string[strlen (string) - 1] = '\0';
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
  FILE *srcfd, *destfd;
  char buf[8192];
  size_t n;

  if ((srcfd = fopen (source, "rb")) == NULL)
    return (0);

  if ((destfd = fopen (dest, "wb")) == NULL)
    {
      fclose (srcfd);
      return (0);
    }

  while ((n = fread (buf, 1, sizeof (buf), srcfd)) > 0)
    fwrite (buf, 1, n, destfd);

  fclose (srcfd);
  fclose (destfd);

  return (1);
}


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
      if (strcmp (argv[0][1], "--help") == 0 || strcmp (argv[0][1], "-h") == 0)
	gftp_usage ();
      else if (strcmp (argv[0][1], "--version") == 0 || strcmp (argv[0][1], "-v") == 0)
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
  printf (_("usage: gftp [[ftp://][user:[pass]@]ftp-site[:port][/directory]]\n"));
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
	      exit (-1);
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
  return (((char *) key)[0] + ((char *) key)[1] + ((char *) key)[2]);
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
  if (fle->fd)
    fclose (fle->fd);
  g_free (fle);
}


gftp_file *
copy_fdata (gftp_file * fle)
{
  gftp_file * newfle;

  newfle = g_malloc0 (sizeof (*newfle));
  memcpy (newfle, fle, sizeof (*newfle));

  if (fle->file)
    {
      newfle->file = g_malloc (strlen (fle->file) + 1);
      strcpy (newfle->file, fle->file);
    }

  if (fle->user)
    {
      newfle->user = g_malloc (strlen (fle->user) + 1);
      strcpy (newfle->user, fle->user);
    }

  if (fle->group)
    {
      newfle->group = g_malloc (strlen (fle->group) + 1);
      strcpy (newfle->group, fle->group);
    }

  if (fle->attribs)
    {
      newfle->attribs = g_malloc (strlen (fle->attribs) + 1);
      strcpy (newfle->attribs, fle->attribs);
    }

  if (fle->destfile)
    {
      newfle->destfile = g_malloc (strlen (fle->destfile) + 1);
      strcpy (newfle->destfile, fle->destfile);
    }
  return (newfle);
}


void
swap_socks (gftp_request * dest, gftp_request * source)
{
  dest->sockfd = source->sockfd;
  dest->datafd = source->datafd;
  dest->sockfd_write = source->sockfd_write;
  dest->cached = 0;
  if (!source->always_connected)
    {
      source->sockfd = NULL;
      source->datafd = NULL;
      source->sockfd_write = NULL;
      source->cached = 1;
    }
}


int
compare_request (gftp_request * request1, gftp_request * request2,
                 int compare_dirs)
{
  char *strarr[3][2];
  int i, ret;

  ret = 1;
  if (strcmp (request1->protocol_name, request2->protocol_name) == 0 &&
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
    gftp_request_destroy (tdata->fromreq);
  if (tdata->toreq != NULL)
    gftp_request_destroy (tdata->toreq);
  free_file_list (tdata->files);
  g_free (tdata);
}


gftp_request * 
copy_request (gftp_request * req)
{
  gftp_request * newreq;

  newreq = g_malloc0 (sizeof (*newreq));
  memcpy (newreq, req, sizeof (*newreq));

  if (req->hostname)
    newreq->hostname = g_strconcat (req->hostname, NULL);
  if (req->username)
    newreq->username = g_strconcat (req->username, NULL);
  if (req->password)
    newreq->password = g_strconcat (req->password, NULL);
  if (req->account)
    newreq->account = g_strconcat (req->account, NULL);
  if (req->directory)
    newreq->directory = g_strconcat (req->directory, NULL);

  newreq->url_prefix = NULL;
  newreq->protocol_name = NULL;
  newreq->sftpserv_path = NULL;
  newreq->proxy_config = NULL;
  newreq->proxy_hostname = NULL;
  newreq->proxy_username = NULL;
  newreq->proxy_password = NULL;
  newreq->proxy_account = NULL;
  newreq->last_ftp_response = NULL;
  newreq->last_dir_entry = NULL;
  newreq->sockfd = NULL;
  newreq->sockfd_write = NULL;
  newreq->datafd = NULL;
  newreq->cachefd = NULL;
  newreq->hostp = NULL;
  newreq->protocol_data = NULL;
  
  if (req->proxy_config != NULL)
    newreq->proxy_config = g_strconcat (req->proxy_config, NULL);

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

#ifdef SYSV

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

#else /* !SYSV */

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

#endif

#endif

}


int
ptys_open (int fdm, char *pts_name)
{
  int fds;

#if !defined (SYSV) && !defined (__sgi)

  chmod (pts_name, S_IRUSR | S_IWUSR);
  chown (pts_name, getuid (), -1);

#endif

  if ((fds = open (pts_name, O_RDWR)) < 0)
    {
      close (fdm);
      return (-1);
    }

#ifdef SYSV

  if (ioctl (fds, I_PUSH, "ptem") < 0)
    {
      close (fdm);
      close (fds);
      return (-1);
    }

  if (ioctl (fds, I_PUSH, "ldterm") < 0)
    {
      close (fdm);
      close (fds);
      return (-1);
    }

  if (ioctl (fds, I_PUSH, "ttcompat") < 0)
    {
      close (fdm);
      close (fds);
      return (-1);
    }

#endif

#if !defined(SYSV) && !defined (__sgi) && defined(TIOCSCTTY) && !defined(CIBAUD)

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
  int i, j;

  args = g_malloc (sizeof (char *) * (num_ssh_extra_params + 10));

  args[0] = ssh_prog_name != NULL && *ssh_prog_name != '\0' ? 
            ssh_prog_name : "ssh";
  i = 1;
  tempstr = g_strconcat (args[0], NULL);

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


char *
ssh_start_login_sequence (gftp_request * request, int fd)
{
  size_t rem, len, diff, lastdiff;
  int flags, wrotepw, ok;
  struct timeval tv;
  char *tempstr;
  fd_set rdfds;
  ssize_t rd;

  rem = len = 100;
  tempstr = g_malloc0 (len);
  diff = lastdiff = 0;
  wrotepw = 0;
  ok = 1;

  if ((flags = fcntl (fd, F_GETFL, 0)) < 0) 
    {
      g_free (tempstr);
      return (NULL);
    }

  if (fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      g_free (tempstr);
      return (NULL);
    }

  while (1)
    {
      FD_ZERO (&rdfds);
      FD_SET (fd, &rdfds);
      tv.tv_sec = 5;
      tv.tv_usec = 0;
      if (select (fd + 1, &rdfds, NULL, NULL, &tv) < 0)
        {
          if (errno == EINTR)
            continue;
          ok = 0;
          break;
        }

      if ((rd = read (fd, tempstr + diff, rem - 1)) < 0)
        {
          if (errno == EINTR)
            continue;
          ok = 0;
          break;
        }
      else if (rd == 0)
        { 
          ok = 0;
          break;
        }
      tempstr[diff + rd] = '\0';
      rem -= rd;
      diff += rd;
      if (rem <= 1)
        {
          tempstr = g_realloc (tempstr, len + 100);
          tempstr[diff] = '\0';
          request->logging_function (gftp_logging_recv, request->user_data,
                                     "%s", tempstr + lastdiff);
          lastdiff = diff;
          len += 100;
          rem = 100;
        }

      if (!wrotepw && 
          strlen (tempstr) > 11 && strcmp (tempstr + strlen (tempstr) - 10, 
                                           "password: ") == 0)
        {
          wrotepw = 1;
          write (fd, request->password, strlen (request->password));
          write (fd, "\n", 1);
        }

      else if (!wrotepw && 
               (strstr (tempstr, "Enter passphrase for RSA key") != NULL ||
               strstr (tempstr, "Enter passphrase for key '") != NULL))
        {
          wrotepw = 1;
          write (fd, request->password, strlen (request->password));
          write (fd, "\n", 1);
        }
      else if (strlen (tempstr) >= 5 && 
               strcmp (tempstr + strlen (tempstr) - 5, "xsftp") == 0)
        break;
    }

  tempstr[diff] = '\0';
  request->logging_function (gftp_logging_recv, request->user_data,
                             "%s\n", tempstr + lastdiff);

  if (ok && fcntl (fd, F_SETFL, flags) < 0)
    ok = 0;

  if (!ok)
    {
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

