/*****************************************************************************/
/*  misc.c - general purpose routines                                        */
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

static const char cvsid[] = "$Id$";

#include "gftp.h"
#include "options.h"

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
  str = g_strdup (src);

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
	str = g_strdup (pw->pw_dir);
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
	  printf ("%s\n", gftp_version);
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
    close (fle->fd);
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


gftp_transfer *
gftp_tdata_new (void)
{
  gftp_transfer * tdata;

  tdata = g_malloc0 (sizeof (*tdata));
/* FIXME
  tdata->statmutex = G_STATIC_MUTEX_INIT;
  tdata->structmutex = G_STATIC_MUTEX_INIT;
*/
  return (tdata);
}


void
free_tdata (gftp_transfer * tdata)
{
  if (tdata->fromreq != NULL)
    gftp_request_destroy (tdata->fromreq, 1);
  if (tdata->toreq != NULL)
    gftp_request_destroy (tdata->toreq, 1);
  free_file_list (tdata->files);
  g_free (tdata);
}


void
gftp_copy_local_options (gftp_request * dest, gftp_request * source)
{
  int i;

  if (source->local_options_vars == NULL)
    {
      dest->local_options_vars = NULL;
      dest->num_local_options_vars = 0;
      dest->local_options_hash = NULL;
      return;
    }

  dest->local_options_hash = g_hash_table_new (string_hash_function,
                                               string_hash_compare);

  for (i=0; source->local_options_vars[i].key != NULL; i++);

  dest->local_options_vars = g_malloc (sizeof (gftp_config_vars) * (i + 1));
  memcpy (dest, source, sizeof (gftp_config_vars) * (i + 1));
  dest->num_local_options_vars = i;

  for (i=0; dest->local_options_vars[i].key != NULL; i++)
    {
      g_hash_table_insert (dest->local_options_hash, 
                           dest->local_options_vars[i].key,
                           &dest->local_options_vars[i]);
    }
}


gftp_request * 
copy_request (gftp_request * req, int copy_local_options)
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
  newreq->use_proxy = req->use_proxy;
  newreq->logging_function = req->logging_function;
  newreq->free_hostp = 0;
  newreq->hostp = req->hostp;

  if (copy_local_options)
    gftp_copy_local_options (newreq, req);

  req->init (newreq);

  return (newreq);
}


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
  int sort_dirs_first;

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
                gftp_datetime_sort_function_as : gftp_datetime_sort_function_ds;
  else if (column == GFTP_SORT_COL_ATTRIBS)
    sortfunc = asds ? 
                gftp_attribs_sort_function_as : gftp_attribs_sort_function_ds;
  else /* Don't sort */
   return (filelist);

  sort_dirs_first = 1;
  gftp_lookup_global_option ("sort_dirs_first", &sort_dirs_first);

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


mode_t
gftp_parse_attribs (char *attribs)
{
  mode_t mode;
  int cur;

  cur = 0;
  if (attribs[1] == 'r')
    cur += 4;
  if (attribs[2] == 'w')
    cur += 2;
  if (attribs[3] == 'x' ||
      attribs[3] == 's')
    cur += 1;
  mode = cur;

  cur = 0;
  if (attribs[4] == 'r')
    cur += 4;
  if (attribs[5] == 'w')
    cur += 2;
  if (attribs[6] == 'x' ||
      attribs[6] == 's')
    cur += 1;
  mode = (mode * 10) + cur;

  cur = 0;
  if (attribs[7] == 'r')
    cur += 4;
  if (attribs[8] == 'w')
    cur += 2;
  if (attribs[9] == 'x' ||
      attribs[9] == 's')
    cur += 1;
  mode = (mode * 10) + cur;

  return (mode);
}


char *
gftp_gen_ls_string (gftp_file * fle, char *file_prefixstr, char *file_suffixstr)
{
  char *tempstr1, *tempstr2, *ret, tstr[50];
  struct tm *lt;
  time_t t;

  lt = localtime (&fle->datetime);

  tempstr1 = g_strdup_printf ("%10s %8s %8s", fle->attribs, fle->user,
                              fle->group);

  if (fle->attribs && (*fle->attribs == 'b' || *fle->attribs == 'c'))
    tempstr2 = g_strdup_printf ("%d, %d", major (fle->size), minor (fle->size));
  else
    {
#if defined (_LARGEFILE_SOURCE)
      tempstr2 = g_strdup_printf ("%11lld", fle->size);
#else
      tempstr2 = g_strdup_printf ("%11ld", fle->size);
#endif
    }

  time (&t);

  if (fle->datetime > t || t - 3600*24*90 > fle->datetime)
    strftime (tstr, sizeof (tstr), "%b %d  %Y", lt);
  else
    strftime (tstr, sizeof (tstr), "%b %d %H:%M", lt);

  if (file_prefixstr == NULL)
    file_prefixstr = "";
  if (file_suffixstr == NULL)
    file_suffixstr = "";

  ret = g_strdup_printf ("%s %s %s %s%s%s", tempstr1, tempstr2, tstr, 
                         file_prefixstr, fle->file, file_suffixstr);

  g_free (tempstr1);
  g_free (tempstr2);

  return (ret);
}


#if !defined (HAVE_GETADDRINFO) || !defined (HAVE_GAI_STRERROR)

struct hostent *
r_gethostbyname (const char *name, struct hostent *result_buf, int *h_errnop)
{
  static GStaticMutex hostfunclock = G_STATIC_MUTEX_INIT; 
  struct hostent *hent;

  if (g_thread_supported ())
    g_static_mutex_lock (&hostfunclock);

  if ((hent = gethostbyname (name)) == NULL)
    {
      if (h_errnop)
        *h_errnop = h_errno;
    }
  else
    {
      *result_buf = *hent;
      hent = result_buf;
    }

  if (g_thread_supported ())
    g_static_mutex_unlock (&hostfunclock);

  return (hent);
}

#endif /* !HAVE_GETADDRINFO */

struct servent *
r_getservbyname (const char *name, const char *proto,
                 struct servent *result_buf, int *h_errnop)
{
  static GStaticMutex servfunclock = G_STATIC_MUTEX_INIT;
  struct servent *sent;

  if (g_thread_supported ())
    g_static_mutex_lock (&servfunclock);

  if ((sent = getservbyname (name, proto)) == NULL)
    {
      if (h_errnop)
        *h_errnop = h_errno;
    }
  else
    {
      *result_buf = *sent;
      sent = result_buf;
    }

  if (g_thread_supported ())
    g_static_mutex_unlock (&servfunclock);
  return (sent);
}

