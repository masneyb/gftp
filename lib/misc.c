/***********************************************************************************/
/*  misc.c - general purpose routines                                              */
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
#include "options.h"
#include <fnmatch.h>

char * BASE_CONF_DIR = NULL;
char * CONFIG_FILE = NULL;
char * BOOKMARKS_FILE = NULL;
char * LOG_FILE = NULL;

static char *gftp_share_dir = NULL;
static char *gftp_doc_dir = NULL;

static int free_share_dir = 0;
static int free_doc_dir = 0;

#ifdef HAVE_INTL_PRINTF

char * insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  //DEBUG_PRINT_FUNC
  if (dest_str != NULL)
    g_snprintf (dest_str, dest_len, GFTP_OFF_T_INTL_PRINTF_MOD, number);
  else
    dest_str = g_strdup_printf (GFTP_OFF_T_INTL_PRINTF_MOD, number);

  return (dest_str);
}

#else

char * insert_commas (off_t number, char *dest_str, size_t dest_len)
{
  //DEBUG_PRINT_FUNC
  char *frompos, *topos, src[50], *dest;
  size_t num, rem, srclen;
  size_t len, i;

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

char * gftp_expand_path (gftp_request * request, const char *src)
{
  //DEBUG_PRINT_FUNC
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


int gftp_match_filespec (gftp_request * request, const char *filename,
                         const char *filespec)
{
   //DEBUG_PRINT_FUNC
   intptr_t show_hidden_files;
  
   if (!filename || !*filename || !filespec || !*filespec) {
      return (1);
   }
   gftp_lookup_request_option (request, "show_hidden_files", &show_hidden_files);
   if (!show_hidden_files && *filename == '.' && strcmp (filename, "..") != 0) {
      return (0);
   }
   if (fnmatch (filespec, filename, 0) == 0) {
      return 1;
   } else {
      return 0;
   }
}


static void gftp_info (void)
{
  DEBUG_PRINT_FUNC
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

  printf ("sizeof (off_t) = %i\n", (int)sizeof (off_t));

#ifdef HAVE_INTL_PRINTF
  printf ("#define HAVE_INTL_PRINTF\n");
#endif

  printf ("GFTP_OFF_T_HEX_PRINTF_MOD = %s\n", GFTP_OFF_T_HEX_PRINTF_MOD);
  printf ("GFTP_OFF_T_INTL_PRINTF_MOD = %s\n", GFTP_OFF_T_INTL_PRINTF_MOD);
  printf ("GFTP_OFF_T_PRINTF_MOD = %s\n", GFTP_OFF_T_PRINTF_MOD);
  printf ("GFTP_OFF_T_11PRINTF_MOD = %s\n", GFTP_OFF_T_11PRINTF_MOD);

#ifdef HAVE_GETDTABLESIZE
  printf ("#define HAVE_GETDTABLESIZE\n");
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

  printf ("glib version: %d.%d.%d\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION,
          GLIB_MICRO_VERSION);

  printf ("PTY implementation: %s\n", gftp_get_pty_impl ());

#ifdef USE_SSL
  printf ("OpenSSL version: %s\n", OPENSSL_VERSION_TEXT);
#endif

  printf ("Enabled protocols: ");
  for (i=0; gftp_protocols[i].name != NULL; i++)
    {
      printf ("%s ", gftp_protocols[i].name);
    }
  printf ("\n");
}


int gftp_parse_command_line (int *argc, char ***argv)
{
  DEBUG_PRINT_FUNC
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


void gftp_usage (void)
{
  DEBUG_PRINT_FUNC
  printf (_("usage: gftp " GFTP_URL_USAGE "\n"));
  exit (0);
}


gint string_hash_compare (gconstpointer path1, gconstpointer path2)
{
  return (strcmp ((char *) path1, (char *) path2) == 0);
}


guint string_hash_function (gconstpointer key)
{
  guint ret;
  int i;

  ret = 0;
  for (i=0; ((char *) key)[i] != '\0' && i < 3; i++)
    ret += ((char *) key)[i];

  return (ret);
}


gint uint_hash_compare (gconstpointer path1, gconstpointer path2)
{
  return (GPOINTER_TO_UINT (path1) == GPOINTER_TO_UINT (path2));
}


guint uint_hash_function (gconstpointer key)
{
  return (GPOINTER_TO_UINT (key));
}


void free_file_list (GList * filelist)
{
  DEBUG_PRINT_FUNC
  gftp_file * tempfle;
  GList * templist;

  templist = filelist;
  while (templist != NULL)
    {
      tempfle = templist->data;
      templist->data = NULL;

      gftp_file_destroy (tempfle, 1);
      templist = templist->next;
    }
  g_list_free (filelist);
}


gftp_file * copy_fdata (gftp_file * fle)
{
  DEBUG_PRINT_FUNC
  gftp_file * newfle;

  newfle = g_malloc0 (sizeof (*newfle));
  memcpy (newfle, fle, sizeof (*newfle));

  if (fle->file)
    newfle->file = g_strdup (fle->file);

  if (fle->user)
    newfle->user = g_strdup (fle->user);

  if (fle->group)
    newfle->group = g_strdup (fle->group);

  if (fle->destfile)
    newfle->destfile = g_strdup (fle->destfile);

  newfle->user_data = NULL;
  return (newfle);
}


int compare_request (gftp_request * request1, gftp_request * request2,
                     int compare_dirs)
{
  DEBUG_PRINT_FUNC
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


gftp_transfer * gftp_tdata_new (void)
{
  DEBUG_PRINT_FUNC
  gftp_transfer * tdata;

  tdata = g_malloc0 (sizeof (*tdata));

  g_mutex_init (&tdata->statmutex);
  g_mutex_init (&tdata->structmutex);

  return (tdata);
}


void free_tdata (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  if (tdata->fromreq)   gftp_request_destroy (tdata->fromreq, 1);
  if (tdata->toreq)     gftp_request_destroy (tdata->toreq, 1);
  if (tdata->thread_id) g_free (tdata->thread_id);
  free_file_list (tdata->files);
  g_free (tdata);
}


gftp_request * gftp_copy_request (gftp_request * req)
{
  DEBUG_PRINT_FUNC
  gftp_request * newreq;

  newreq = gftp_request_new ();

  if (req->hostname)   newreq->hostname   = g_strdup (req->hostname);
  if (req->username)   newreq->username   = g_strdup (req->username);
  if (req->password)   newreq->password   = g_strdup (req->password);
  if (req->account)    newreq->account    = g_strdup (req->account);
  if (req->directory)  newreq->directory  = g_strdup (req->directory);
  if (req->url_prefix) newreq->url_prefix = req->url_prefix; /* this is not freed */

  newreq->port = req->port;
  newreq->use_proxy = req->use_proxy;
  newreq->logging_function = req->logging_function;
  newreq->ai_family = req->ai_family;
  newreq->ai_socktype = req->ai_socktype;
  newreq->use_udp = req->use_udp;

  if (req->remote_addr == NULL)
    {
      newreq->remote_addr = NULL;
      newreq->remote_addr_len = 0;
    }
  else
    {
      newreq->remote_addr = g_malloc0 (req->remote_addr_len);
      memcpy (newreq->remote_addr, req->remote_addr, req->remote_addr_len);
      newreq->remote_addr_len = req->remote_addr_len;
    }

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


static gint gftp_file_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->file, f2->file));
}


static gint gftp_file_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->file, f2->file);
  return (ret * -1);
}


static gint gftp_user_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->user, f2->user));
}


static gint gftp_user_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->user, f2->user);
  return (ret * -1);
}


static gint gftp_group_sort_function_as (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;

  f1 = a;
  f2 = b;
  return (strcasecmp (f1->group, f2->group));
}


static gint gftp_group_sort_function_ds (gconstpointer a, gconstpointer b)
{
  const gftp_file * f1, * f2;
  gint ret;

  f1 = a;
  f2 = b;
  ret = strcasecmp (f1->group, f2->group);
  return (ret * -1);
}


static gint gftp_attribs_sort_function_as (gconstpointer a, gconstpointer b)
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


static gint gftp_attribs_sort_function_ds (gconstpointer a, gconstpointer b)
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


static gint gftp_size_sort_function_as (gconstpointer a, gconstpointer b)
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


static gint gftp_size_sort_function_ds (gconstpointer a, gconstpointer b)
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


static gint gftp_datetime_sort_function_as (gconstpointer a, gconstpointer b)
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


static gint gftp_datetime_sort_function_ds (gconstpointer a, gconstpointer b)
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


GList * gftp_sort_filelist (GList * filelist, int column, int asds)
{
  DEBUG_PRINT_FUNC
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
gftp_gen_ls_string (gftp_request * request, gftp_file * fle,
                    char *file_prefixstr, char *file_suffixstr)
{
  char *tempstr1, *tempstr2, *ret, tstr[50], *attribs, *utf8;
  size_t destlen;
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

  utf8 = gftp_string_from_utf8 (request, 1, fle->file, &destlen);
  if (utf8 != NULL)
    {
      ret = g_strdup_printf ("%s %s %s %s%s%s", tempstr1, tempstr2, tstr,
                             file_prefixstr, utf8, file_suffixstr);
      g_free (utf8);
    }
  else
    ret = g_strdup_printf ("%s %s %s %s%s%s", tempstr1, tempstr2, tstr,
                           file_prefixstr, fle->file, file_suffixstr);

  g_free (tempstr1);
  g_free (tempstr2);

  return (ret);
}


void gftp_free_bookmark (gftp_bookmarks_var * entry, int free_node)
{
  DEBUG_PRINT_FUNC
  gftp_bookmarks_var * tempentry;

  if (entry->path)       g_free (entry->path);
  if (entry->hostname)   g_free (entry->hostname);
  if (entry->remote_dir) g_free (entry->remote_dir);
  if (entry->local_dir)  g_free (entry->local_dir);
  if (entry->user)       g_free (entry->user);
  if (entry->pass)       g_free (entry->pass);
  if (entry->acct)       g_free (entry->acct);
  if (entry->protocol)   g_free (entry->protocol);

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


void gftp_shutdown (void)
{
  DEBUG_PRINT_FUNC
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

  if (BASE_CONF_DIR)  free (BASE_CONF_DIR);
  if (CONFIG_FILE)    free (CONFIG_FILE);
  if (BOOKMARKS_FILE) free (BOOKMARKS_FILE);
  if (LOG_FILE)       free (LOG_FILE);
  if (free_share_dir) free (gftp_share_dir);
  if (free_doc_dir)   free (gftp_doc_dir);

  exit (0);
}


char * gftp_build_path (gftp_request * request, const char *first_element, ...) 
{
  //DEBUG_PRINT_FUNC
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
      ret = g_realloc (ret, (gulong) retlen + 1);

      if (add_separator) {
         strncat (ret, "/", retlen);
      }
      strncat (ret, element, retlen);
    }

  return (ret);
}


void gftp_locale_init (void)
{
  DEBUG_PRINT_FUNC
#ifdef HAVE_GETTEXT
  setlocale (LC_ALL, "");
  textdomain ("gftp");
  bindtextdomain ("gftp", LOCALE_DIR);
  bind_textdomain_codeset ("gftp", "UTF-8");
#endif /* HAVE_GETTEXT */
  // XDG SPEC (XDG_CONFIG_HOME defaults to $HOME/.config/ + gftp)
  BASE_CONF_DIR = g_build_filename (g_get_user_config_dir(), "gftp", NULL);
  if (g_mkdir_with_parents (BASE_CONF_DIR, 0755) == -1)
  {
    printf (_("gFTP Error: Could not make directory %s: %s\n"),
              BASE_CONF_DIR, g_strerror (errno));
    g_free (BASE_CONF_DIR);
    exit (EXIT_FAILURE);
  }
  CONFIG_FILE    = g_strconcat (BASE_CONF_DIR, "/gftprc",    NULL);
  BOOKMARKS_FILE = g_strconcat (BASE_CONF_DIR, "/bookmarks", NULL);
  LOG_FILE       = g_strconcat (BASE_CONF_DIR, "/gftp.log",  NULL);
}

/* Very primary encryption/decryption to make the passwords unreadable
   with 'cat ~/.gftp/bookmarks'.
   
   Each character is separated in two nibbles. Then each nibble is stored
   under the form 01xxxx01. The resulted string is prefixed by a '$'.
*/


char * gftp_scramble_password (const char *password)
{
  //DEBUG_PRINT_FUNC
  char *newstr, *newpos;

  if (strcmp (password, "@EMAIL@") == 0)
    return (g_strdup (password));

  newstr = g_malloc0 ((gulong) strlen (password) * 2 + 2);
  newpos = newstr;
  
  *newpos++ = '$';

  while (*password != '\0')
    {
      *newpos++ = ((*password >> 2) & 0x3c) | 0x41;
      *newpos++ = ((*password << 2) & 0x3c) | 0x41;
      password++;
    }
  *newpos = 0;
  
  return (newstr);
}


char * gftp_descramble_password (const char *password)
{
  //DEBUG_PRINT_FUNC
  const char *passwordpos;
  char *newstr, *newpos;
  int error;

  if (*password != '$')
    return (g_strdup (password));

  passwordpos = password + 1;
  newstr = g_malloc0 ((gulong) strlen (passwordpos) / 2 + 1);
  newpos = newstr;
 
  error = 0;
  while (*passwordpos != '\0' && *(passwordpos + 1) != '\0')
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


int gftp_get_transfer_action (gftp_request * request, gftp_file * fle)
{
  DEBUG_PRINT_FUNC
  intptr_t overwrite_default;

  gftp_lookup_request_option (request, "overwrite_default", &overwrite_default);

  /* FIXME - add code to compare the file times and make a decision based
     on that. Also if overwrite_default is enabled and the file sizes/dates are
     the same, then skip the file */

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

char * gftp_get_share_dir (void)
{
  DEBUG_PRINT_FUNC
  char *envval;
  if (gftp_share_dir == NULL)
  {
      envval = getenv ("GFTP_SHARE_DIR");
      if (envval && *envval) {
          gftp_share_dir = g_strdup (envval);
          free_share_dir = 1;
      } else {
          gftp_share_dir = SHARE_DIR;
      }
  }
  return (gftp_share_dir);
}

char * gftp_get_doc_dir (void)
{
  DEBUG_PRINT_FUNC
  char *envval;
  if (gftp_doc_dir == NULL)
  {
      envval = getenv ("GFTP_DOC_DIR");
      if (envval && *envval) {
          gftp_doc_dir = g_strdup (envval);
          free_doc_dir = 1;
      } else {
          gftp_doc_dir = DOC_DIR;
      }
  }
  return (gftp_doc_dir);
}

void gftp_format_file_size(off_t bytes, char *out_buffer, size_t buffer_size)
{
  //DEBUG_PRINT_FUNC
  char tmp[20];
  char *pointzero = 0;
  off_t TB = 1024LL * 1024LL * 1024LL * 1024LL;
  off_t GB = 1024LL * 1024LL * 1024LL;
  off_t MB = 1024LL * 1024LL;
  if (bytes >= TB) {
    snprintf(tmp, sizeof(tmp), "%.1f", (double) bytes / TB);
    pointzero = strstr(tmp, ".0");
    if (pointzero) *pointzero = 0;
    snprintf(out_buffer, buffer_size, "%s TiB", tmp);
  }
  else if (bytes >= GB) {
    snprintf(tmp, sizeof(tmp), "%.1f", (double) bytes / GB);
    pointzero = strstr(tmp, ".0");
    if (pointzero) *pointzero = 0;
    snprintf(out_buffer, buffer_size, "%s GiB", tmp);
  }
  else if (bytes >= MB) {
    snprintf(tmp, sizeof(tmp), "%.1f", (double) bytes / MB);
    pointzero = strstr(tmp, ".0");
    if (pointzero) *pointzero = 0;
    snprintf(out_buffer, buffer_size, "%s MiB", tmp);
  }
  else if (bytes >= 1024) {
    snprintf(out_buffer, buffer_size, "%jd KiB", bytes / 1024);
  }
  else {
    snprintf(out_buffer, buffer_size, "%jd B", bytes);
  }
}


char * str_get_next_line_pointer (char * buf, char **pos, int *line_ending)
{
   // modified buf, new lines become '\0'
   // start: *pos must be NULL, buf = line1\r\nline2\r\n, etc
   // empty lines are treated as eof
   char * ret = NULL;
   char * nextpos = NULL;
   char * p;
   if (buf == NULL) {
      return NULL;
   }
   if (*pos == NULL) {
      *pos = buf;
   }
   p = *pos;
   if (!*p) {
      return NULL;
   }
   nextpos = strchr (*pos, '\n');
   if (nextpos && nextpos != *pos) {
      *nextpos = 0;
      if (*(nextpos-1) == '\r') {
         *(nextpos-1) = 0;
      }
      nextpos++;
      ret = *pos;
      if (line_ending) {
         *line_ending = 1;
      }
   } else {
      // handle last line without \r\n
      ret = *pos;
      p = *pos;
      if (*p) {
         while (*p) p++;
         *pos = p;
      }
      if (line_ending) {
         *line_ending = 0;
      }
   }

   if (nextpos && nextpos != *pos) {
      *pos = nextpos;
   }
   return ret;
}
