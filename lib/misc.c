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

#include "gftp.h"
#include "options.h"
static const char cvsid[] = "$Id$";

#ifdef HAVE_INTL_PRINTF

char *
insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  if (dest_str != NULL)
    g_snprintf (dest_str, dest_len, GFTP_OFF_T_INTL_PRINTF_MOD, number);
  else
    dest_str = g_strdup_printf (GFTP_OFF_T_INTL_PRINTF_MOD, number);

  return (dest_str);
}

#else

char *
insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  char *frompos, *topos, src[50], *dest;
  size_t num, rem, srclen;
  int len, i;

  g_snprintf (src, sizeof (src), GFTP_OFF_T_PRINTF_MOD, number);

  if (dest_str != NULL)
    *dest_str = '\0';

  len = strlen (src);
  if (len == 0)
    {
      if (dest_str != NULL)
        {
          strncpy (dest_str, "0", dest_len);
          dest_str[dest_len - 1] = '\0';
        }
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

  srclen = strlen (src);
  num = srclen / 3 - 1;
  rem = srclen % 3;

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

#endif

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
gftp_expand_path (gftp_request * request, const char *src)
{
  char *str, *pos, *endpos, *prevpos, *newstr, *tempstr, *ntoken,
       tempchar;
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

      for (ntoken = pos; *ntoken == '/'; ntoken++);

      if ((endpos = strchr (ntoken, '/')) == NULL)
	endpos = pos + strlen (pos);

      tempchar = *endpos;
      *endpos = '\0';

      if (strcmp (ntoken, "..") == 0)
	{
	  if (newstr != NULL && (prevpos = strrchr (newstr, '/')) != NULL)
            {
	      *prevpos = '\0';
              if (*newstr == '\0')
                {
                  g_free (newstr);
                  newstr = NULL;
                }
            }
	}
      else if (strcmp (ntoken, ".") != 0)
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

  if (endpos != NULL && *endpos != '\0' && newstr == NULL)
    {
      if (strcmp (endpos, "..") == 0)
        newstr = g_strdup ("/");
      else
        newstr = g_strdup (endpos);
    }

  if (newstr == NULL || *newstr == '\0')
    {
      if (newstr != NULL)
	g_free (newstr);

      newstr = g_strdup ("/");
    }

  g_free (str);

  if (pw != NULL)
    {
      if ((pos = strchr (newstr, '/')) == NULL)
	str = g_strdup (pw->pw_dir);
      else
	str = gftp_build_path (request, pw->pw_dir, pos, NULL);

      g_free (newstr);
      newstr = str;
    }

  return (newstr);
}


void
make_nonnull (char **str)
{
  if (*str == NULL)
    *str = g_malloc0 (1);
}


/* FIXME - is there a replacement for this */
int
gftp_match_filespec (const char *filename, const char *filespec)
{
  const char *filepos, *wcpos, *pos;
  char search_str[20], *newpos;
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


static void
gftp_info (void)
{
  int i;

  printf ("%s\n", gftp_version);

#ifdef _REENTRANT
  printf ("#define _REENTRANT\n");
#endif

#ifdef _GNU_SOURCE
  printf ("#define _GNU_SOURCE\n");
#endif

#ifdef _LARGEFILE_SOURCE
  printf ("#define _LARGEFILE_SOURCE\n");
#endif

#ifdef _FILE_OFFSET_BITS
  printf ("#define _FILE_OFFSET_BITS %d\n", _FILE_OFFSET_BITS);
#endif

  printf ("sizeof (off_t) = %d\n", sizeof (off_t));

#ifdef HAVE_GETADDRINFO
  printf ("#define HAVE_GETADDRINFO\n");
#endif

#ifdef HAVE_GAI_STRERROR
  printf ("#define HAVE_GAI_STRERROR\n");
#endif

#ifdef HAVE_GETDTABLESIZE
  printf ("#define HAVE_GETDTABLESIZE\n");
#endif

#ifdef G_HAVE_GINT64
  printf ("#define G_HAVE_GINT64\n");
#endif

#ifdef HAVE_LIBREADLINE
  printf ("#define HAVE_LIBREADLINE\n");
#endif

#ifdef ENABLE_NLS
  printf ("#define ENABLE_NLS\n");
#endif

#ifdef HAVE_GETTEXT
  printf ("#define HAVE_GETTEXT\n");
#endif

#ifdef HAVE_INTL_PRINTF
  printf ("#define HAVE_INTL_PRINTF\n");
#endif

  printf ("glib version: %d.%d.%d\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION,
          GLIB_MICRO_VERSION);

  printf ("PTY implementation: %s\n", gftp_get_pty_impl ());

#ifdef USE_SSL
  printf ("OpenSSL version: 0x%lx\n", OPENSSL_VERSION_NUMBER);
#endif

  printf ("Enabled protocols: ");
  for (i=0; gftp_protocols[i].name != NULL; i++)
    {
      printf ("%s ", gftp_protocols[i].name);
    }
  printf ("\n");
}


int
gftp_parse_command_line (int *argc, char ***argv)
{
  if (*argc > 1)
    {
      if (strcmp (argv[0][1], "--help") == 0 || 
          strcmp (argv[0][1], "-h") == 0)
        {
          gftp_usage ();
          exit (0);
        }
      else if (strcmp (argv[0][1], "--version") == 0 || 
               strcmp (argv[0][1], "-v") == 0)
	{
          printf ("%s\n", gftp_version);
          exit (0);
	}
      else if (strcmp (argv[0][1], "--info") == 0)
	{
          gftp_info ();
          exit (0);
	}
    }
  return (0);
}


void
gftp_usage (void)
{
  printf (_("usage: gftp " GFTP_URL_USAGE "\n"));
  exit (0);
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
      gftp_file_destroy (tempfle, 1);
      templist = templist->next;
    }
  g_list_free (filelist);
}


gftp_file *
copy_fdata (gftp_file * fle)
{
  gftp_file * newfle;

  newfle = g_malloc0 (sizeof (*newfle));
  memcpy (newfle, fle, sizeof (*newfle));

  if (fle->file)
    newfle->file = g_strdup (fle->file);

  if (fle->utf8_file)
    newfle->utf8_file = g_strdup (fle->utf8_file);

  if (fle->user)
    newfle->user = g_strdup (fle->user);

  if (fle->group)
    newfle->group = g_strdup (fle->group);

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
#if GLIB_MAJOR_VERSION == 1
  static GStaticMutex init_mutex = G_STATIC_MUTEX_INIT;
#endif
  gftp_transfer * tdata;

  tdata = g_malloc0 (sizeof (*tdata));

#if GLIB_MAJOR_VERSION == 1
  tdata->statmutex = init_mutex;
  tdata->structmutex = init_mutex;
#else
  g_static_mutex_init (&tdata->statmutex);
  g_static_mutex_init (&tdata->structmutex);
#endif

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


gftp_request * 
gftp_copy_request (gftp_request * req)
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
  newreq->ai_family = req->ai_family;
  newreq->free_hostp = 0;
  newreq->hostp = NULL;

  gftp_copy_local_options (&newreq->local_options_vars, 
                           &newreq->local_options_hash,
                           &newreq->num_local_options_vars,
                           req->local_options_vars,
                           req->num_local_options_vars);

  if (req->init != NULL && req->init (newreq) < 0)
    {
      gftp_request_destroy (newreq, 1);
      return (NULL);
    }

  gftp_copy_param_options (newreq, req);

  return (newreq);
}


static gint
gftp_file_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->file, f2->file));
}


static gint
gftp_file_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->file, f2->file);
  return (ret * -1);
}


static gint
gftp_user_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->user, f2->user));
}


static gint
gftp_user_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->user, f2->user);
  return (ret * -1);
}


static gint
gftp_group_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->group, f2->group));
}


static gint
gftp_group_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->group, f2->group);
  return (ret * -1);
}


static gint
gftp_attribs_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->st_mode < f2->st_mode)
    return (-1);
  else if (f1->st_mode == f2->st_mode)
    return (0);
  else
    return (1);
}


static gint
gftp_attribs_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  if (f1->st_mode < f2->st_mode)
    return (1);
  else if (f1->st_mode == f2->st_mode)
    return (0);
  else
    return (-1);
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
  intptr_t sort_dirs_first;

  if (filelist == NULL) /* nothing to sort */
    return (filelist);

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
      else if (sort_dirs_first && S_ISDIR (tempfle->st_mode))
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


char *
gftp_gen_ls_string (gftp_file * fle, char *file_prefixstr, char *file_suffixstr)
{
  char *tempstr1, *tempstr2, *ret, tstr[50], *attribs;
  struct tm *lt;
  time_t t;

  lt = localtime (&fle->datetime);

  attribs = gftp_convert_attributes_from_mode_t (fle->st_mode);
  tempstr1 = g_strdup_printf ("%10s %8s %8s", attribs, fle->user, fle->group);
  g_free (attribs);

  if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode))
    tempstr2 = g_strdup_printf ("%d, %d", major (fle->size), minor (fle->size));
  else
    tempstr2 = g_strdup_printf (GFTP_OFF_T_11PRINTF_MOD, fle->size);

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
                         file_prefixstr, 
                         fle->utf8_file != NULL ? fle->utf8_file : fle->file,
                         file_suffixstr);

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


char *
base64_encode (char *str)
{

/* The standard to Base64 encoding can be found in RFC2045 */

  char *newstr, *newpos, *fillpos, *pos;
  unsigned char table[64], encode[3];
  size_t slen, num;
  int i;

  for (i = 0; i < 26; i++)
    {
      table[i] = 'A' + i;
      table[i + 26] = 'a' + i;
    }

  for (i = 0; i < 10; i++)
    table[i + 52] = '0' + i;

  table[62] = '+';
  table[63] = '/';

  slen = strlen (str);
  num = slen / 3;
  if (slen % 3 > 0)
    num++;

  newstr = g_malloc (num * 4 + 1);
  newstr[num * 4] = '\0';
  newpos = newstr;

  pos = str;
  while (*pos != '\0')
    {
      memset (encode, 0, sizeof (encode));
      for (i = 0; i < 3 && *pos != '\0'; i++)
	encode[i] = *pos++;

      fillpos = newpos;
      *newpos++ = table[encode[0] >> 2];
      *newpos++ = table[(encode[0] & 3) << 4 | encode[1] >> 4];
      *newpos++ = table[(encode[1] & 0xF) << 2 | encode[2] >> 6];
      *newpos++ = table[encode[2] & 0x3F];
      while (i < 3)
	fillpos[++i] = '=';
    }
  return (newstr);
}


void
gftp_free_bookmark (gftp_bookmarks_var * entry, int free_node)
{
  gftp_bookmarks_var * tempentry;

  if (entry->hostname)
    g_free (entry->hostname);
  if (entry->remote_dir)
    g_free (entry->remote_dir);
  if (entry->local_dir)
    g_free (entry->local_dir);
  if (entry->user)
    g_free (entry->user);
  if (entry->pass)
    g_free (entry->pass);
  if (entry->acct)
    g_free (entry->acct);
  if (entry->protocol)
    g_free (entry->protocol);

  if (entry->local_options_vars != NULL)
    {
      gftp_config_free_options (entry->local_options_vars,
                                entry->local_options_hash,
                                entry->num_local_options_vars);

      entry->local_options_vars = NULL;
      entry->local_options_hash = NULL;
      entry->num_local_options_vars = 0;
    }

  if (free_node)
    g_free (entry);
  else
    {
      tempentry = entry->children;
      memset (entry, 0, sizeof (*entry));
      entry->children = tempentry;
    }
}


void
gftp_shutdown (void)
{
#ifdef WITH_DMALLOC
  gftp_config_vars * cv;
  GList * templist;
#endif

  if (gftp_logfd != NULL)
    fclose (gftp_logfd);

  gftp_clear_cache_files ();

  if (gftp_configuration_changed)
    gftp_write_config_file ();

#ifdef WITH_DMALLOC
  if (gftp_global_options_htable != NULL)
    g_hash_table_destroy (gftp_global_options_htable);

  if (gftp_config_list_htable != NULL)
    g_hash_table_destroy (gftp_config_list_htable);

  if (gftp_bookmarks_htable != NULL)
    g_hash_table_destroy (gftp_bookmarks_htable);

  for (templist = gftp_options_list; 
       templist != NULL; 
       templist = templist->next)
    {
      cv = templist->data;
      gftp_config_free_options (cv, NULL, -1);
    }

  gftp_bookmarks_destroy (gftp_bookmarks);

  dmalloc_shutdown ();
#endif

  exit (0);
}


GList *
get_next_selection (GList * selection, GList ** list, int *curnum)
{
  gftp_file * tempfle;
  int i, newpos;

  newpos = GPOINTER_TO_INT (selection->data);
  i = *curnum - newpos;

  if (i < 0)
    {
      while (i != 0)
        {
          tempfle = (*list)->data;
          if (tempfle->shown)
            {
	      ++*curnum;
	      i++;
            }
	  *list = (*list)->next;
        }     
    }
  else if (i > 0)
    {
      while (i != 0)
        {
          tempfle = (*list)->data;
          if (tempfle->shown)
            {
	      --*curnum;
	      i--;
            }
	  *list = (*list)->prev;
        }
    }

  tempfle = (*list)->data;
  while ((*list)->next && !tempfle->shown)
    {
      *list = (*list)->next;
      tempfle = (*list)->data;
    }
  return (selection->next);
}


char *
gftp_build_path (gftp_request * request, const char *first_element, ...) 
{
  const char *element;
  size_t len, retlen;
  int add_separator;
  va_list args;
  char *ret;

  g_return_val_if_fail (first_element != NULL, NULL);

  ret = g_strdup (first_element);
  retlen = strlen (ret);

  va_start (args, first_element);
  for (element = va_arg (args, char *);
       element != NULL;
       element = va_arg (args, char *))
    {
      len = strlen (element);

      if (len == 0)
        continue;

      if (retlen > 0 && (ret[retlen - 1] == '/' || element[0] == '/'))
        add_separator = 0;
      else
        {
          add_separator = 1;
          len++;
        }
      
      retlen += len;
      ret = g_realloc (ret, retlen + 1);

      /* Don't append a / for VMS servers... */
      if (add_separator &&
          (request == NULL || request->server_type != GFTP_DIRTYPE_VMS))
        strncat (ret, "/", retlen);

      strncat (ret, element, retlen);
    }

  return (ret);
}


void
gftp_locale_init (void)
{
#ifdef HAVE_GETTEXT

  setlocale (LC_ALL, "");
  textdomain ("gftp");
  bindtextdomain ("gftp", LOCALE_DIR);

#if GLIB_MAJOR_VERSION > 1
  bind_textdomain_codeset ("gftp", "UTF-8");
#endif

#endif /* HAVE_GETTEXT */
}

/* Very primary encryption/decryption to make the passwords unreadable
   with 'cat ~/.gftp/bookmarks'.
   
   Each character is separated in two nibbles. Then each nibble is stored
   under the form 01xxxx01. The resulted string is prefixed by a '$'.
*/


char * 
gftp_scramble_password (const char *password)
{
  char *newstr, *newpos;

  if (strcmp (password, "@EMAIL@") == 0)
    return (g_strdup (password));

  newstr = g_malloc (strlen (password) * 2 + 2);
  newpos = newstr;
  
  *newpos++ = '$';

  while (*password != 0)
    {
      *newpos++ = ((*password >> 2) & 0x3c) | 0x41;
      *newpos++ = ((*password << 2) & 0x3c) | 0x41;
      password++;
    }
  *newpos = 0;
  
  return (newstr);
}


char *
gftp_descramble_password (const char *password)
{
  const char *passwordpos;
  char *newstr, *newpos;
  int error;

  if (*password != '$')
    return (g_strdup (password));

  passwordpos = password + 1;
  newstr = g_malloc (strlen (passwordpos) / 2 + 1);
  newpos = newstr;
 
  error = 0;
  while (*passwordpos != '\0' && (*passwordpos + 1) != '\0')
    {
      if ((*passwordpos & 0xc3) != 0x41 ||
          (*(passwordpos + 1) & 0xc3) != 0x41)
        {
          error = 1;
          break;
        }

      *newpos++ = ((*passwordpos & 0x3c) << 2) | 
                  ((*(passwordpos + 1) & 0x3c) >> 2);

      passwordpos += 2;
    }

  if (error)
    {
      g_free (newstr);
      return (g_strdup (password));
    }

  *newpos = '\0';
  return (newstr);
}


int
gftp_get_transfer_action (gftp_request * request, gftp_file * fle)
{
  intptr_t overwrite_default;

  gftp_lookup_request_option (request, "overwrite_default", &overwrite_default);

  if (overwrite_default)
    fle->transfer_action = GFTP_TRANS_ACTION_OVERWRITE;
  else if (fle->startsize == fle->size)
    fle->transfer_action = GFTP_TRANS_ACTION_SKIP;
  else if (fle->startsize > fle->size)
    fle->transfer_action = GFTP_TRANS_ACTION_OVERWRITE;
  else
    fle->transfer_action = GFTP_TRANS_ACTION_RESUME;

  return (fle->transfer_action);
}


char *
gftp_get_share_dir (void)
{
  static char *gftp_share_dir = NULL;
  char *envval;

  if (gftp_share_dir == NULL)
    {
      envval = getenv ("GFTP_SHARE_DIR");

      if (envval != NULL && *envval != '\0')
        gftp_share_dir = g_strdup (envval);
      else
        gftp_share_dir = SHARE_DIR;
    }

  return (gftp_share_dir);
}

