/***********************************************************************************/
/*  config_file.c - config file routines                                           */
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

void
gftp_add_bookmark (gftp_bookmarks_var * newentry)
{
  gftp_bookmarks_var * parententry, * folderentry, * endentry;
  char *curpos;

  if (!newentry->protocol)
    newentry->protocol = g_strdup ("FTP");

  /* We have to create the folders. For example, if we have 
     Debian Sites/Debian, we have to create a Debian Sites entry */
  parententry = gftp_bookmarks;
  if (parententry->children != NULL)
    {
      endentry = parententry->children;
      while (endentry->next != NULL)
        endentry = endentry->next;
    }
  else
    endentry = NULL;
  curpos = newentry->path;
  while ((curpos = strchr (curpos, '/')) != NULL)
    {
      *curpos = '\0';
      /* See if we already made this folder */
      if ((folderentry = (gftp_bookmarks_var *)
           g_hash_table_lookup (gftp_bookmarks_htable, newentry->path)) == NULL)
        {
          /* Allocate the individual folder. We have to do this for the edit 
             bookmarks feature */
          folderentry = g_malloc0 (sizeof (*folderentry));
          folderentry->path = g_strdup (newentry->path);
          folderentry->parent = parententry;
          folderentry->isfolder = 1;
          g_hash_table_insert (gftp_bookmarks_htable, folderentry->path,
                               folderentry);
          if (parententry->children == NULL)
            parententry->children = folderentry;
          else
            endentry->next = folderentry;
          parententry = folderentry;
          endentry = NULL;
        }
      else
        {
          parententry = folderentry;
          if (parententry->children != NULL)
            {
              endentry = parententry->children;
              while (endentry->next != NULL)
                endentry = endentry->next;
            }
          else
            endentry = NULL;
        }
      *curpos = '/';
      curpos++;
    }

  if (newentry->path[strlen (newentry->path) - 1] == '/')
    {
      newentry->path[strlen (newentry->path) - 1] = '\0';
      newentry->isfolder = 1;
    }
  else
    {
      /* Get the parent node */
      if ((curpos = strrchr (newentry->path, '/')) == NULL)
        parententry = gftp_bookmarks;
      else
        {
          *curpos = '\0';
          parententry = (gftp_bookmarks_var *)
                         g_hash_table_lookup (gftp_bookmarks_htable, newentry->path);
          *curpos = '/';
        }
    
      if (parententry->children != NULL)
        {
          endentry = parententry->children;
          while (endentry->next != NULL)
              endentry = endentry->next;
          endentry->next = newentry;
        }
      else
        parententry->children = newentry;

      newentry->parent = parententry;
      newentry->next = NULL;
      g_hash_table_insert (gftp_bookmarks_htable, newentry->path, newentry);
    }
}


static int
copyfile (char *source, char *dest)
{
  int srcfd, destfd;
  char buf[8192];
  ssize_t n;

  if ((srcfd = gftp_fd_open (NULL, source, O_RDONLY, 0)) == -1)
    {
      printf (_("Error: Cannot open local file %s: %s\n"),
              source, g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  if ((destfd = gftp_fd_open (NULL, dest, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == -1)
    {
      printf (_("Error: Cannot open local file %s: %s\n"),
              dest, g_strerror (errno));
      close (srcfd);
      exit (EXIT_FAILURE);
    }

  while ((n = read (srcfd, buf, sizeof (buf))) > 0)
    {
      if (write (destfd, buf, n) == -1)
        {
          printf (_("Error: Could not write to socket: %s\n"), 
                  g_strerror (errno));
          exit (EXIT_FAILURE);
        }
    }

  if (n == -1)
    {
      printf (_("Error: Could not read from socket: %s\n"), g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (close (srcfd) == -1)
    {
      printf (_("Error closing file descriptor: %s\n"), g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (close (destfd) == -1)
    {
      printf (_("Error closing file descriptor: %s\n"), g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  return (1);
}


static void
gftp_read_bookmarks (char *global_data_path)
{
  char *tempstr, *temp1str, buf[255], *curpos;
  gftp_config_vars * global_entry;
  gftp_bookmarks_var * newentry;
  FILE *bmfile;
  size_t len;
  int line;

  if ((tempstr = gftp_expand_path (NULL, BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), BOOKMARKS_FILE);
      exit (EXIT_FAILURE);
    }

  if (access (tempstr, F_OK) == -1)
    {
      temp1str = g_strdup_printf ("%s/bookmarks", global_data_path);
      if (access (temp1str, F_OK) == -1)
        {
          printf (_("Warning: Cannot find master bookmark file %s\n"),
                  temp1str);
          g_free (temp1str);
          return;
        }
      copyfile (temp1str, tempstr);
      g_free (temp1str);
    }

  if ((bmfile = fopen (tempstr, "r")) == NULL)
    {
      printf (_("gFTP Error: Cannot open bookmarks file %s: %s\n"), tempstr,
              g_strerror (errno));
      exit (EXIT_FAILURE);
    }
  g_free (tempstr);

  line = 0;
  newentry = NULL;
  while (fgets (buf, sizeof (buf), bmfile))
    {
      len = strlen (buf);
      if (len > 0 && buf[len - 1] == '\n')
        buf[--len] = '\0';
      if (len > 0 && buf[len - 1] == '\r')
        buf[--len] = '\0';
      line++;

      if (*buf == '[')
        {
          newentry = g_malloc0 (sizeof (*newentry));
          for (; buf[len - 1] == ' ' || buf[len - 1] == ']'; buf[--len] = '\0');
          newentry->path = g_strdup (buf + 1);
          newentry->isfolder = 0;
          gftp_add_bookmark (newentry);
        }
      else if (strncmp (buf, "hostname", 8) == 0 && newentry)
        {
          curpos = buf + 9;
          if (newentry->hostname)
            g_free (newentry->hostname);
          newentry->hostname = g_strdup (curpos);
        }
      else if (strncmp (buf, "port", 4) == 0 && newentry)
        newentry->port = strtol (buf + 5, NULL, 10);
      else if (strncmp (buf, "protocol", 8) == 0 && newentry)
        {
          curpos = buf + 9;
          if (newentry->protocol)
            g_free (newentry->protocol);
          newentry->protocol = g_strdup (curpos);
        }
      else if (strncmp (buf, "remote directory", 16) == 0 && newentry)
        {
          curpos = buf + 17;
          if (newentry->remote_dir)
            g_free (newentry->remote_dir);
          newentry->remote_dir = g_strdup (curpos);
        }
      else if (strncmp (buf, "local directory", 15) == 0 && newentry)
        {
          curpos = buf + 16;
          if (newentry->local_dir)
            g_free (newentry->local_dir);
          newentry->local_dir = g_strdup (curpos);
        }
      else if (strncmp (buf, "username", 8) == 0 && newentry)
        {
          curpos = buf + 9;
          if (newentry->user)
            g_free (newentry->user);
          newentry->user = g_strdup (curpos);
        }
      else if (strncmp (buf, "password", 8) == 0 && newentry)
        {
          curpos = buf + 9;
          if (newentry->pass)
            g_free (newentry->pass);

          /* Always try to descramble passords. If the password is not
             scrambled, descramble_password returns the string unchanged */
          newentry->pass = gftp_descramble_password (curpos);
          newentry->save_password = *newentry->pass != '\0';
        }
      else if (strncmp (buf, "account", 7) == 0 && newentry)
        {
          curpos = buf + 8;
          if (newentry->acct)
            g_free (newentry->acct);
          newentry->acct = g_strdup (curpos);
        }
      else if (*buf == '#' || *buf == '\0')
        continue;
      else
        {
          if ((curpos = strchr (buf, '=')) == NULL)
            continue;
          *curpos = '\0';

          if ((global_entry = g_hash_table_lookup (gftp_global_options_htable,
                                                   buf)) == NULL ||
               gftp_option_types[global_entry->otype].read_function == NULL)
            {
              printf (_("gFTP Warning: Skipping line %d in bookmarks file: %s\n"),
                      line, buf);
              continue;
            }

          if (newentry->local_options_hash == NULL)
            newentry->local_options_hash = g_hash_table_new (string_hash_function,
                                                             string_hash_compare);

          newentry->num_local_options_vars++;
          newentry->local_options_vars = g_realloc (newentry->local_options_vars,
                                                    (gulong) sizeof (gftp_config_vars) * newentry->num_local_options_vars);

          memcpy (&newentry->local_options_vars[newentry->num_local_options_vars - 1], global_entry, 
                  sizeof (newentry->local_options_vars[newentry->num_local_options_vars - 1]));

          newentry->local_options_vars[newentry->num_local_options_vars - 1].flags &= ~GFTP_CVARS_FLAGS_DYNMEM;
          newentry->local_options_vars[newentry->num_local_options_vars - 1].value = NULL;

          if (gftp_option_types[global_entry->otype].read_function (curpos + 1,
                                &newentry->local_options_vars[newentry->num_local_options_vars - 1], line) != 0)
            {
              printf (_("gFTP Warning: Skipping line %d in bookmarks file: %s\n"),
                      line, buf);
              continue;
            }

          g_hash_table_insert (newentry->local_options_hash, newentry->local_options_vars[newentry->num_local_options_vars - 1].key, &newentry->local_options_vars[newentry->num_local_options_vars - 1]);
        }
    }
}


int
gftp_config_parse_args (char *str, int numargs, int lineno, char **first, ...)
{
  char *curpos, *endpos, *pos, **dest, tempchar;
  int ret, has_colon;
  va_list argp;

  ret = 1;
  va_start (argp, first);
  curpos = str;
  dest = first;
  *dest = NULL;
  while (numargs > 0)
    {
      has_colon = 0;
      if (numargs > 1)
        {
          if ((endpos = strchr (curpos, ':')) == NULL)
            {
              printf (_("gFTP Warning: Line %d doesn't have enough arguments\n"), 
                      lineno);
              ret = 0;
              endpos = curpos + strlen (curpos);
            }
          else
            {
              /* Allow colons inside the fields. If you want a colon inside a 
                 field, just put 2 colons in there */
              while (endpos != NULL && *(endpos - 1) == '\\')
                {
                  endpos = strchr (endpos + 1, ':');
                  has_colon = 1;
                }
            }
        }
      else
        endpos = curpos + strlen (curpos);

      *dest = g_malloc0 ((gulong) (endpos - curpos + 1));
      tempchar = *endpos;
      *endpos = '\0';
      strcpy (*dest, curpos);
      *endpos = tempchar;
      if (has_colon)
        {
          pos = *dest;
          curpos = *dest;
          while (*pos != '\0')
            {
              if (*pos != '\\' && *(pos + 1) != ':')
                *curpos++ = *pos++;
              else
                pos++;
            }
          *curpos = '\0';
        }
      if (*endpos == '\0')
        break;
      curpos = endpos + 1;
      if (numargs > 1)
        {
          dest = va_arg (argp, char **);
          *dest = NULL;
        }
      numargs--;
    }

  while (numargs > 1)
    {
      dest = va_arg (argp, char **);
      *dest = g_malloc0 (1);
      numargs--;
    }
  va_end (argp);
  return (ret);
}


static void *
gftp_config_read_str (char *buf, int line)
{
  return (g_strdup (buf));
}


static void
gftp_config_write_str (FILE *fd, void *data)
{
  fprintf (fd, "%s", (char *) data);
}


static void *
gftp_config_read_ext (char *buf, int line)
{
  gftp_file_extensions * tempext;

  tempext = g_malloc0 (sizeof (*tempext));
  gftp_config_parse_args (buf, 4, line, &tempext->ext, &tempext->filename,
                          &tempext->ascii_binary, &tempext->view_program);
 
  tempext->stlen = strlen (tempext->ext);

  return (tempext);
}


static void
gftp_config_write_ext (FILE *fd, void *data)
{
  gftp_file_extensions * tempext;

  tempext = data;
  fprintf (fd, "%s:%s:%c:%s", tempext->ext, tempext->filename,
           *tempext->ascii_binary == '\0' ? ' ' : *tempext->ascii_binary,
           tempext->view_program);
}


static gftp_config_list_vars gftp_config_list[] = {
  {"ext",		gftp_config_read_ext,	gftp_config_write_ext,	
   NULL, 0,
   N_("ext=file extenstion:XPM file:Ascii or Binary (A or B):viewer program. Note: All arguments except the file extension are optional")},
  {"localhistory",	gftp_config_read_str,	gftp_config_write_str,	
   NULL, 0, NULL},
  {"remotehistory",	gftp_config_read_str,   gftp_config_write_str,	
    NULL, 0, NULL},
  {"hosthistory",	gftp_config_read_str,   gftp_config_write_str,	
    NULL, 0, NULL},
  {"porthistory",	gftp_config_read_str,   gftp_config_write_str,	
    NULL, 0, NULL},
  {"userhistory",	gftp_config_read_str,   gftp_config_write_str,	
    NULL, 0, NULL},
  {NULL,		NULL,			NULL,			
    NULL, 0, NULL}
};


static void
gftp_setup_global_options (gftp_config_vars * cvars)
{
  int i;

  for (i=0; cvars[i].key != NULL; i++)
    {
      if (cvars[i].key != NULL && *cvars[i].key != '\0')
        g_hash_table_insert (gftp_global_options_htable, 
                             cvars[i].key, &cvars[i]);
    }
}


void
gftp_read_config_file (char *global_data_path)
{
  char *tempstr, *temp1str, *curpos, buf[255];
  gftp_config_list_vars * tmplistvar;
  gftp_config_vars * tmpconfigvar;
  char **protocol_list;
  FILE *conffile;
  int line, i, j;
  size_t len;

  gftp_global_options_htable = g_hash_table_new (string_hash_function, 
                                                 string_hash_compare);

  gftp_register_config_vars (gftp_global_config_vars);

  protocol_list = NULL;
  for (i=0, j=0; gftp_protocols[i].register_options != NULL; i++)
    {
      if (gftp_protocols[i].shown)
        {
          protocol_list = g_realloc (protocol_list,
                                     (gulong) sizeof (char *) * (j + 2));
          protocol_list[j] = gftp_protocols[i].name;
          protocol_list[j + 1] = NULL;
          j++;
        }

      if (gftp_protocols[i].register_options != NULL)
        gftp_protocols[i].register_options ();
    }

  if ((tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                           "default_protocol")) != NULL)
    {
      tmpconfigvar->listdata = protocol_list;
      tmpconfigvar->flags |= GFTP_CVARS_FLAGS_DYNLISTMEM;
    }

  gftp_config_list_htable = g_hash_table_new (string_hash_function, 
                                              string_hash_compare);

  for (i=0; gftp_config_list[i].key != NULL; i++)
    {
      g_hash_table_insert (gftp_config_list_htable, gftp_config_list[i].key, 
                           &gftp_config_list[i]);
    }

  if ((tempstr = gftp_expand_path (NULL, CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (EXIT_FAILURE);
    }

  if (access (tempstr, F_OK) == -1)
    {
      temp1str = gftp_expand_path (NULL, BASE_CONF_DIR);
      if (access (temp1str, F_OK) == -1)
        {
          if (mkdir (temp1str, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
            {
              printf (_("gFTP Error: Could not make directory %s: %s\n"),
                      temp1str, g_strerror (errno));
              exit (EXIT_FAILURE);
            }
        }
      g_free (temp1str);

      temp1str = g_strdup_printf ("%s/gftprc", global_data_path);
      if (access (temp1str, F_OK) == -1)
        {
          printf (_("gFTP Error: Cannot find master config file %s\n"),
                  temp1str);
          printf (_("Did you do a make install?\n"));
          exit (EXIT_FAILURE);
        }
      copyfile (temp1str, tempstr);
      g_free (temp1str);
    }

  if ((conffile = fopen (tempstr, "r")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
              g_strerror (errno));
      exit (EXIT_FAILURE);
    }
  g_free (tempstr);
 
  line = 0;
  while (fgets (buf, sizeof (buf), conffile))
    {
      len = strlen (buf);
      if (len > 0 && buf[len - 1] == '\n')
        buf[--len] = '\0';
      if (len > 0 && buf[len - 1] == '\r')
        buf[--len] = '\0';
      line++;

      if (*buf == '#' || *buf == '\0')
        continue;

      if ((curpos = strchr (buf, '=')) == NULL)
        continue;

      *curpos = '\0';

      if ((tmplistvar = g_hash_table_lookup (gftp_config_list_htable, 
                                             buf)) != NULL)
        {
          tmplistvar->list = g_list_append (tmplistvar->list, 
                                            tmplistvar->read_func (curpos + 1,
                                                                   line));
          tmplistvar->num_items++;
        }
      else if ((tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                                    buf)) != NULL &&
               gftp_option_types[tmpconfigvar->otype].read_function != NULL)
        {
          if (gftp_option_types[tmpconfigvar->otype].read_function (curpos + 1, 
                                tmpconfigvar, line) != 0)
            {
              printf (_("Terminating due to parse errors at line %d in the config file\n"), line);
              exit (EXIT_FAILURE);
            }
        }
      else
        {
          printf (_("gFTP Warning: Skipping line %d in config file: %s\n"),
                  line, buf);
        }
    }

  if ((tempstr = gftp_expand_path (NULL, LOG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad log file name %s\n"), LOG_FILE);
      exit (EXIT_FAILURE);
    }

  if ((gftp_logfd = fopen (tempstr, "w")) == NULL)
    {
      printf (_("gFTP Warning: Cannot open %s for writing: %s\n"),
              tempstr, g_strerror (errno));
    }
  g_free (tempstr);

  gftp_bookmarks = g_malloc0 (sizeof (*gftp_bookmarks));
  gftp_bookmarks->isfolder = 1;
  gftp_bookmarks->path = g_malloc0 (1);
  gftp_bookmarks_htable = g_hash_table_new (string_hash_function, string_hash_compare);

  gftp_read_bookmarks (global_data_path);
}


static void
write_comment (FILE * fd, const char *comment)
{
  const char *pos, *endpos;

  fwrite ("# ", 1, 2, fd);
  pos = comment;
  while (strlen (pos) > 76)
    {
      for (endpos = pos + 76; *endpos != ' ' && endpos > pos; endpos--);
      if (endpos == pos)
        {
          for (endpos = pos + 76; *endpos != ' ' && *endpos != '\0';
               endpos++);
        }
      fwrite (pos, 1, endpos - pos, fd);
      fwrite ("\n", 1, 1, fd);
      if (*endpos == '\0')
        {
          pos = endpos;
          break;
        }
      pos = endpos + 1;
      fwrite ("# ", 1, 2, fd);
    }
  if (strlen (pos) > 1)
    {
      fwrite (pos, 1, strlen (pos), fd);
      fwrite ("\n", 1, 1, fd);
    }
}


void
gftp_write_bookmarks_file (void)
{
  char *bmhdr, *pwhdr, *tempstr, *password, buf[256];
  gftp_bookmarks_var * tempentry;
  FILE * bmfile;
  int i;

  bmhdr = "gFTP Bookmarks. Warning: Any comments that you add to this file WILL be overwritten";
  pwhdr = N_("Note: The passwords contained inside this file are scrambled. This algorithm is not secure. This is to avoid your password being easily remembered by someone standing over your shoulder while you're editing this file. Prior to this, all passwords were stored in plaintext.");

  if ((tempstr = gftp_expand_path (NULL, BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), CONFIG_FILE);
      exit (EXIT_FAILURE);
    }

  if ((bmfile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open bookmarks file %s: %s\n"),
              CONFIG_FILE, g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  g_free (tempstr);

  write_comment (bmfile, _(bmhdr));
  write_comment (bmfile, _(pwhdr));
  fwrite ("\n", 1, 1, bmfile);

  tempentry = gftp_bookmarks->children;
  while (tempentry != NULL)
    {
      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }

      tempstr = tempentry->path;
      while (*tempstr == '/')
        tempstr++;

      if (tempentry->isfolder)
        {
          fprintf (bmfile, "[%s/]\n", tempstr);
        }
      else
        {
          if (tempentry->save_password && tempentry->pass != NULL)
            password = gftp_scramble_password (tempentry->pass);
          else
            password = NULL;

          fprintf (bmfile,
                   "[%s]\nhostname=%s\nport=%u\nprotocol=%s\nremote directory=%s\nlocal directory=%s\nusername=%s\npassword=%s\naccount=%s\n",
                   tempstr, tempentry->hostname == NULL ? "" : tempentry->hostname,
                   tempentry->port, tempentry->protocol == NULL
                   || *tempentry->protocol ==
                   '\0' ? gftp_protocols[0].name : tempentry->protocol,
                   tempentry->remote_dir == NULL ? "" : tempentry->remote_dir,
                   tempentry->local_dir == NULL ? "" : tempentry->local_dir,
                   tempentry->user == NULL ? "" : tempentry->user,
                   password == NULL ? "" : password,
                   tempentry->acct == NULL ? "" : tempentry->acct);
          if (password != NULL)
            g_free(password);

          if (tempentry->local_options_vars != NULL)
            {
              for (i=0; i<tempentry->num_local_options_vars; i++)
                {
                  gftp_option_types[tempentry->local_options_vars[i].otype].write_function (&tempentry->local_options_vars[i], buf, sizeof (buf), 1);
    
                  fprintf (bmfile, "%s=%s\n", tempentry->local_options_vars[i].key,
                           buf);
                }
            }
        }

      fprintf (bmfile, "\n");
 
      if (tempentry->next == NULL)
        {
          tempentry = tempentry->parent;
          while (tempentry->next == NULL && tempentry->parent != NULL)
            tempentry = tempentry->parent;
          tempentry = tempentry->next;
        }
      else
        tempentry = tempentry->next;
    }

  fclose (bmfile);
}


void
gftp_write_config_file (void)
{
  char *tempstr, buf[256];
  gftp_config_vars * cv;
  GList *templist;
  FILE *conffile;
  int i;

  if ((tempstr = gftp_expand_path (NULL, CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (EXIT_FAILURE);
    }

  if ((conffile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
              g_strerror (errno));
      exit (EXIT_FAILURE);
    }

  g_free (tempstr);

  write_comment (conffile, _("gFTP. Warning: Any comments that you add to this file WILL be overwritten. If a entry has a (*) in it's comment, you can't change it inside gFTP"));

  for (templist = gftp_options_list;
       templist != NULL;
       templist = templist->next)
    {
      cv = templist->data;

      for (i=0; cv[i].key != NULL; i++)
        {
          if (gftp_option_types[cv[i].otype].write_function == NULL ||
              *cv[i].key == '\0')
            continue;

          fprintf (conffile, "\n");
          if (cv[i].comment != NULL)
            write_comment (conffile, _(cv[i].comment));

          gftp_option_types[cv[i].otype].write_function (&cv[i], buf,
                                                         sizeof (buf), 1);
          fprintf (conffile, "%s=%s\n", cv[i].key, buf);
        }
    }
    
  for (i=0; gftp_config_list[i].key != NULL; i++)
    {
      if (gftp_config_list[i].header == NULL &&
          gftp_config_list[i].list == NULL)
        continue;

      fprintf (conffile, "\n");
      if (gftp_config_list[i].header != NULL)
        write_comment (conffile, _(gftp_config_list[i].header));

      for (templist = gftp_config_list[i].list;
           templist != NULL;
           templist = templist->next)
        {
          fprintf (conffile, "%s=", gftp_config_list[i].key);
          gftp_config_list[i].write_func (conffile, templist->data);
          fprintf (conffile, "\n");
        }
    }

  fclose (conffile);
}


GHashTable *
build_bookmarks_hash_table (gftp_bookmarks_var * entry)
{
  gftp_bookmarks_var * tempentry;
  GHashTable * htable;

  htable = g_hash_table_new (string_hash_function, string_hash_compare);
  tempentry = entry;
  while (tempentry != NULL)
    {
      g_hash_table_insert (htable, tempentry->path, tempentry);
      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }
      while (tempentry->next == NULL && tempentry->parent != NULL)
        tempentry = tempentry->parent;
      tempentry = tempentry->next;
    }
  return (htable);
}


void
print_bookmarks (gftp_bookmarks_var * bookmarks)
{
  gftp_bookmarks_var * tempentry;

  tempentry = bookmarks->children;
  while (tempentry != NULL)
    {
      printf ("Path: %s (%d)\n", tempentry->path, tempentry->children != NULL);
      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }

      if (tempentry->next == NULL)
        {
          while (tempentry->next == NULL && tempentry->parent != NULL)
            tempentry = tempentry->parent;
          tempentry = tempentry->next;
        }
      else
        tempentry = tempentry->next;
    }
}


static int
gftp_config_file_read_text (char *str, gftp_config_vars * cv, int line)
{
  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM && cv->value != NULL)
    g_free (cv->value);

  if (str != NULL)
    {
      cv->value = g_strdup (str);
      cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;
      return (0);
    }
  else
    {
      cv->value = NULL;
      cv->flags &= ~GFTP_CVARS_FLAGS_DYNMEM;
      return (-1);
    }
}


static int
gftp_config_file_write_text (gftp_config_vars * cv, char *buf, size_t buflen,
                             int to_config_file)
{
  char *outstr;

  if (cv->value != NULL)
    {
      outstr = cv->value;
      g_snprintf (buf, buflen, "%s", outstr);
      return (0);
    }
  else
    return (-1);
}


static int
gftp_config_file_write_hidetext (gftp_config_vars * cv, char *buf,
                                 size_t buflen, int to_config_file)
{
  char *outstr;

  if (cv->value != NULL)
    {
      outstr = cv->value;
      if (*outstr != '\0')
        {
          if (to_config_file)
            g_snprintf (buf, buflen, "%s", outstr);
          else
            g_snprintf (buf, buflen, "*****");
        }
      return (0);
    }
  else
    return (-1);
}


static void
gftp_config_file_copy_text (gftp_config_vars * cv, gftp_config_vars * dest_cv)
{
  if (dest_cv->flags & GFTP_CVARS_FLAGS_DYNMEM && dest_cv->value != NULL)
    g_free (dest_cv->value);

  if (cv->value != NULL)
    {
      dest_cv->value = g_strdup ((char *) cv->value);
      dest_cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;
    }
  else
    dest_cv->value = NULL;
}


static int
gftp_config_file_compare_text (gftp_config_vars * cv1, gftp_config_vars * cv2)
{
  char *str1, *str2;

  str1 = cv1->value;
  str2 = cv2->value;

  if (cv1->value == NULL && cv2->value == NULL)
    return (0);

  if ((cv1->value == NULL && cv2->value != NULL) ||
      (cv1->value != NULL && cv2->value == NULL))
    return (-1);

  return (strcmp (str1, str2));
}


static void
gftp_config_file_copy_ptr_contents (gftp_config_vars * cv, gftp_config_vars * dest_cv)
{
  memcpy (&dest_cv->value, &cv->value, sizeof (dest_cv->value));
}

static int
gftp_config_file_read_int (char *str, gftp_config_vars * cv, int line)
{
  cv->value = GINT_TO_POINTER(strtol (str, NULL, 10));
  return (0);
}


static int
gftp_config_file_write_int (gftp_config_vars * cv, char *buf, size_t buflen,
                            int to_config_file)
{
  g_snprintf (buf, buflen, "%d", GPOINTER_TO_INT (cv->value));
  return (0);
}


static int
gftp_config_file_compare_int (gftp_config_vars * cv1, gftp_config_vars * cv2)
{
  return (GPOINTER_TO_INT(cv1->value) == GPOINTER_TO_INT(cv2->value) ? 0 : -1);
}


static int
gftp_config_file_read_checkbox (char *str, gftp_config_vars * cv, int line)
{
  cv->value = GINT_TO_POINTER(strtol (str, NULL, 10) ? 1 : 0);
  return (0);
}


static int
gftp_config_file_read_float (char *str, gftp_config_vars * cv, int line)
{
  union { intptr_t i; float f; } r;

  r.f = strtod (str, NULL);
  memcpy (&cv->value, &r.i, sizeof (cv->value));
  return (0);
}


static int
gftp_config_file_write_float (gftp_config_vars * cv, char *buf, size_t buflen,
                              int to_config_file)
{
  float f;

  memcpy (&f, &cv->value, sizeof (f));
  g_snprintf (buf, buflen, "%.2f", f);
  return (0);
}


static int
gftp_config_file_compare_float (gftp_config_vars * cv1, gftp_config_vars * cv2)
{
  float f1, f2;

  memcpy (&f1, &cv1->value, sizeof (f1));
  memcpy (&f2, &cv2->value, sizeof (f2));
  return (f1 == f2 ? 0 : -1);
}


static int
gftp_config_file_read_color (char *str, gftp_config_vars * cv, int line)
{
  char *red, *green, *blue;
  gftp_color * color;

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM && cv->value != NULL)
    g_free (cv->value);

  gftp_config_parse_args (str, 3, line, &red, &green, &blue);

  color = g_malloc0 (sizeof (*color));
  color->red = strtol (red, NULL, 16);
  color->green = strtol (green, NULL, 16);
  color->blue = strtol (blue, NULL, 16);
  g_free (red);
  g_free (green);
  g_free (blue);

  cv->value = color;
  cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;

  return (0);
}


static int
gftp_config_file_write_color (gftp_config_vars * cv, char *buf, size_t buflen,
                              int to_config_file)
{
  gftp_color * color;

  color = cv->value;
  g_snprintf (buf, buflen, "%x:%x:%x", color->red, color->green, color->blue);
  return (0);
}


static void
gftp_config_file_copy_color (gftp_config_vars * cv, gftp_config_vars * dest_cv)
{
  if (dest_cv->flags & GFTP_CVARS_FLAGS_DYNMEM && dest_cv->value != NULL)
    g_free (dest_cv->value);

  dest_cv->value = g_malloc0 (sizeof (gftp_color));
  memcpy (dest_cv->value, cv->value, sizeof (gftp_color));
  dest_cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;
}


static int
gftp_config_file_compare_color (gftp_config_vars * cv1, gftp_config_vars * cv2)
{
  gftp_color * color1, * color2;

  color1 = cv1->value;
  color2 = cv2->value;

  return (color1->red == color2->red && color1->green == color2->green &&
          color1->blue == color2->blue ? 0 : -1);
}


static int
gftp_config_file_read_intcombo (char *str, gftp_config_vars * cv, int line)
{
  char **clist;
  int i;

  cv->value = 0;
  if (cv->listdata != NULL)
    {
      clist = cv->listdata;
      for (i=0; clist[i] != NULL; i++)
        {
          if (strcasecmp (clist[i], str) == 0)
            {
              cv->value = GINT_TO_POINTER(i);
              break;
            }
        }
    }

  return (0);
}


static int
gftp_config_file_write_intcombo (gftp_config_vars * cv, char *buf,
                                 size_t buflen, int to_config_file)
{
  char **clist;

  clist = cv->listdata;
  if (clist != NULL)
    g_snprintf (buf, buflen, "%s", clist[GPOINTER_TO_INT(cv->value)]);
  else
    g_snprintf (buf, buflen, _("<unknown>"));

  return (0);
}


static int
gftp_config_file_read_textcombo (char *str, gftp_config_vars * cv, int line)
{
  char **clist;
  int i;

  cv->value = NULL;
  if (cv->listdata != NULL)
    {
      clist = cv->listdata;
      for (i=0; clist[i] != NULL; i++)
        {
          if (strcasecmp (clist[i], str) == 0)
            {
              cv->value = clist[i];
              break;
            }
        }
    }

  return (0);
}


/* Note, the index numbers of this array must match up to the numbers in
   gftp_option_type_enum in gftp.h */
gftp_option_type_var gftp_option_types[] = {
  {gftp_config_file_read_text, gftp_config_file_write_text, 
   gftp_config_file_copy_text, gftp_config_file_compare_text, NULL, NULL, NULL,
   NULL},
  {gftp_config_file_read_textcombo, gftp_config_file_write_text, 
   gftp_config_file_copy_text, gftp_config_file_compare_text, NULL, NULL, NULL,
   NULL},
  {gftp_config_file_read_text, gftp_config_file_write_text, 
   gftp_config_file_copy_text, gftp_config_file_compare_text, NULL, NULL, NULL,
   NULL},
  {gftp_config_file_read_text, gftp_config_file_write_hidetext, 
   gftp_config_file_copy_text, gftp_config_file_compare_text, NULL, NULL, NULL,
   NULL},
  {gftp_config_file_read_int, gftp_config_file_write_int, 
   gftp_config_file_copy_ptr_contents, gftp_config_file_compare_int, 
   NULL, NULL, NULL, NULL},
  {gftp_config_file_read_checkbox, gftp_config_file_write_int, 
   gftp_config_file_copy_ptr_contents, gftp_config_file_compare_int,
   NULL, NULL, NULL, NULL},
  {gftp_config_file_read_intcombo, gftp_config_file_write_intcombo, 
   gftp_config_file_copy_ptr_contents, gftp_config_file_compare_int, 
   NULL, NULL, NULL, NULL},
  {gftp_config_file_read_float, gftp_config_file_write_float, 
   gftp_config_file_copy_ptr_contents, gftp_config_file_compare_float,
   NULL, NULL, NULL, NULL},
  {gftp_config_file_read_color, gftp_config_file_write_color, 
   gftp_config_file_copy_color, gftp_config_file_compare_color, 
   NULL, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};


void
gftp_lookup_global_option (const char * key, void *value)
{
  gftp_config_list_vars * tmplistvar;
  gftp_config_vars * tmpconfigvar;

  if (gftp_global_options_htable != NULL &&
      (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                           key)) != NULL)
    memcpy (value, &tmpconfigvar->value, sizeof (value));
  else if ((tmplistvar = g_hash_table_lookup (gftp_config_list_htable, 
                                              key)) != NULL)
    *(gftp_config_list_vars **) value = tmplistvar;
  else
    {
      fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
      exit (EXIT_FAILURE);
    }
}


void
gftp_lookup_request_option (gftp_request * request, const char * key,
                            void *value)
{
  gftp_config_vars * tmpconfigvar;

  if (request != NULL && request->local_options_hash != NULL &&
      (tmpconfigvar = g_hash_table_lookup (request->local_options_hash,
                                           key)) != NULL)
    memcpy (value, &tmpconfigvar->value, sizeof (value));
  else
    gftp_lookup_global_option (key, value);
}


void
gftp_lookup_bookmark_option (gftp_bookmarks_var * bm, const char * key,
                             void *value)
{
  gftp_config_vars * tmpconfigvar;

  if (bm != NULL && bm->local_options_hash != NULL &&
      (tmpconfigvar = g_hash_table_lookup (bm->local_options_hash,
                                           key)) != NULL)
    memcpy (value, &tmpconfigvar->value, sizeof (value));
  else
    gftp_lookup_global_option (key, value);
}


void
gftp_set_global_option (const char * key, const void *value)
{
  gftp_config_vars * tmpconfigvar, newconfigvar;
  void *nc_ptr;
  int ret;

  if (gftp_global_options_htable != NULL &&
      (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                           key)) != NULL)
    {
      memcpy (&newconfigvar, tmpconfigvar, sizeof (newconfigvar));

      /* Cheap warning fix for const pointer... */
      memcpy (&nc_ptr, &value, sizeof (nc_ptr));
      newconfigvar.value = nc_ptr;
      newconfigvar.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

      ret = gftp_option_types[tmpconfigvar->otype].compare_function (&newconfigvar, tmpconfigvar);
      if (ret != 0)
        {
          gftp_option_types[tmpconfigvar->otype].copy_function (&newconfigvar, tmpconfigvar);
          gftp_configuration_changed = 1;
        }
    }
  else
    {
      fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
      exit (EXIT_FAILURE);
    }
}


static void
_gftp_set_option_value (gftp_config_vars * cv, const void * newval)
{
  gftp_config_vars newconfigvar;
  void *nc_ptr;

  memcpy (&newconfigvar, cv, sizeof (newconfigvar));

  /* Cheap warning fix for const pointer... */
  memcpy (&nc_ptr, &newval, sizeof (nc_ptr));
  newconfigvar.value = nc_ptr;
  newconfigvar.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

  gftp_option_types[cv->otype].copy_function (&newconfigvar, cv);
}


void
gftp_set_request_option (gftp_request * request, const char * key,
                         const void *value)
{
  gftp_config_vars * tmpconfigvar;

  if (request->local_options_hash == NULL)
    request->local_options_hash = g_hash_table_new (string_hash_function,
                                                    string_hash_compare);

  if ((tmpconfigvar = g_hash_table_lookup (request->local_options_hash,
                                           key)) != NULL)
    _gftp_set_option_value (tmpconfigvar, value);
  else
    {
      if (gftp_global_options_htable == NULL ||
          (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                               key)) == NULL)
        {
          fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
          exit (EXIT_FAILURE);
        }
      
      request->num_local_options_vars++;
      request->local_options_vars = g_realloc (request->local_options_vars, 
                                               (gulong) sizeof (gftp_config_vars) * request->num_local_options_vars);

      memcpy (&request->local_options_vars[request->num_local_options_vars - 1], tmpconfigvar, sizeof (*tmpconfigvar));
      _gftp_set_option_value (&request->local_options_vars[request->num_local_options_vars - 1], value);

      g_hash_table_insert (request->local_options_hash, request->local_options_vars[request->num_local_options_vars - 1].key, &request->local_options_vars[request->num_local_options_vars - 1]);
    }
}


void
gftp_set_bookmark_option (gftp_bookmarks_var * bm, const char * key,
                          const void *value)
{
  gftp_config_vars * tmpconfigvar, newconfigvar;
  int ret;

  if (bm->local_options_hash != NULL &&
      (tmpconfigvar = g_hash_table_lookup (bm->local_options_hash,
                                           key)) != NULL)
    _gftp_set_option_value (tmpconfigvar, value);
  else
    {
      if (gftp_global_options_htable == NULL ||
          (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                               key)) == NULL)
        {
          fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
          exit (EXIT_FAILURE);
        }

      /* Check to see if this is set to the same value as the global option. 
         If so, don't add it to the bookmark preferences */
      memcpy (&newconfigvar, tmpconfigvar, sizeof (newconfigvar));
      memcpy (&newconfigvar.value, &value, sizeof (newconfigvar.value));
      newconfigvar.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

      ret = gftp_option_types[tmpconfigvar->otype].compare_function (&newconfigvar, tmpconfigvar);
      if (ret == 0)
        return;
      
      if (bm->local_options_hash == NULL)
        bm->local_options_hash = g_hash_table_new (string_hash_function,
                                                   string_hash_compare);

      bm->num_local_options_vars++;
      bm->local_options_vars = g_realloc (bm->local_options_vars, 
                                          (gulong) sizeof (gftp_config_vars) * bm->num_local_options_vars);

      memcpy (&bm->local_options_vars[bm->num_local_options_vars - 1], tmpconfigvar, sizeof (*tmpconfigvar));
      _gftp_set_option_value (&bm->local_options_vars[bm->num_local_options_vars - 1], value);

      g_hash_table_insert (bm->local_options_hash, bm->local_options_vars[bm->num_local_options_vars - 1].key, &bm->local_options_vars[bm->num_local_options_vars - 1]);
    }
}


void
gftp_register_config_vars (gftp_config_vars * config_vars)
{
  gftp_options_list = g_list_append (gftp_options_list, config_vars);
  gftp_setup_global_options (config_vars);
}


void
gftp_copy_local_options (gftp_config_vars ** new_options_vars, 
                         GHashTable ** new_options_hash,
                         int *new_num_local_options_vars,
                         gftp_config_vars * orig_options,
                         int num_local_options_vars)
{
  int i;

  *new_num_local_options_vars = num_local_options_vars;
  if (orig_options == NULL || num_local_options_vars == 0)
    {
      *new_options_vars = NULL;
      *new_options_hash = NULL;
      return;
    }

  *new_options_hash = g_hash_table_new (string_hash_function,
                                        string_hash_compare);

  *new_options_vars = g_malloc0 ((gulong) sizeof (gftp_config_vars) * num_local_options_vars);
  memcpy (*new_options_vars, orig_options,
          sizeof (gftp_config_vars) * num_local_options_vars);

  for (i=0; i<num_local_options_vars; i++)
    {
      g_hash_table_insert (*new_options_hash, (*new_options_vars)[i].key,
                           &(*new_options_vars)[i]);
      
      (*new_options_vars)[i].value = NULL;
      (*new_options_vars)[i].flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

      gftp_option_types[(*new_options_vars)[i].otype].copy_function (&(orig_options)[i], &(*new_options_vars)[i]);
    }
}


void
gftp_config_free_options (gftp_config_vars * options_vars,
                          GHashTable * options_hash,
                          int num_options_vars)
{
  int i;

  if (num_options_vars == 0)
    return;

  if (num_options_vars > 0)
    {
      /* If num_options_vars is 0, then the options was allocated with malloc */

      for (i=0; i<num_options_vars; i++)
        {
          if (options_vars[i].flags & GFTP_CVARS_FLAGS_DYNMEM &&
              options_vars[i].value != NULL)
            g_free (options_vars[i].value);
        }

      g_free (options_vars);
    }
  else if (num_options_vars < 0)
    {
      /* Otherwise we are freeing the global options */

      for (i=0; options_vars[i].key != NULL; i++)
        {
          if (options_vars[i].flags & GFTP_CVARS_FLAGS_DYNMEM &&
              options_vars[i].value != NULL)
            g_free (options_vars[i].value);

          if (options_vars[i].flags & GFTP_CVARS_FLAGS_DYNLISTMEM &&
              options_vars[i].listdata != NULL)
            g_free (options_vars[i].listdata);
        }
    }

  if (options_hash != NULL)
    g_hash_table_destroy (options_hash);
}


void
gftp_bookmarks_destroy (gftp_bookmarks_var * bookmarks)
{
  gftp_bookmarks_var * tempentry, * delentry;
  
  if (bookmarks == NULL)
    return;

  tempentry = bookmarks;
  while (tempentry != NULL)
    {
      if (tempentry->children != NULL)
        tempentry = tempentry->children;
      else
        {
          while (tempentry->next == NULL && tempentry->parent != NULL)
            {
              delentry = tempentry;
              tempentry = tempentry->parent;
              gftp_free_bookmark (delentry, 1);
            }
    
          delentry = tempentry;
          tempentry = tempentry->next;
          gftp_free_bookmark (delentry, 1);
        }
    }
}


