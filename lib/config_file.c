/*****************************************************************************/
/*  config_file.c - config file routines                                     */
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

void
gftp_add_bookmark (gftp_bookmarks_var * newentry)
{
  gftp_bookmarks_var * preventry, * folderentry, * endentry;
  char *curpos;

  if (!newentry->protocol)
    newentry->protocol = g_strdup ("FTP");

  /* We have to create the folders. For example, if we have 
     Debian Sites/Debian, we have to create a Debian Sites entry */
  preventry = gftp_bookmarks;
  if (preventry->children != NULL)
    {
      endentry = preventry->children;
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
	  folderentry->prev = preventry;
	  folderentry->isfolder = 1;
	  g_hash_table_insert (gftp_bookmarks_htable, folderentry->path,
			       folderentry);
	  if (preventry->children == NULL)
	    preventry->children = folderentry;
	  else
	    endentry->next = folderentry;
	  preventry = folderentry;
	  endentry = NULL;
	}
      else
	{
	  preventry = folderentry;
	  if (preventry->children != NULL)
	    {
	      endentry = preventry->children;
	      while (endentry->next != NULL)
		endentry = endentry->next;
	    }
	  else
	    endentry = NULL;
	}
      *curpos = '/';
      curpos++;
    }

  /* Get the parent node */
  if ((curpos = strrchr (newentry->path, '/')) == NULL)
    preventry = gftp_bookmarks;
  else
    {
      *curpos = '\0';
      preventry = (gftp_bookmarks_var *)
	g_hash_table_lookup (gftp_bookmarks_htable, newentry->path);
      *curpos = '/';
    }

  if (preventry->children != NULL)
    {
      endentry = preventry->children;
      while (endentry->next != NULL)
	endentry = endentry->next;
      endentry->next = newentry;
    }
  else
    preventry->children = newentry;
  newentry->prev = preventry;
  newentry->next = NULL;
  g_hash_table_insert (gftp_bookmarks_htable, newentry->path, newentry);
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

  if ((tempstr = expand_path (BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), BOOKMARKS_FILE);
      exit (1);
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
      exit (1);
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
	  newentry->pass = g_strdup (curpos);
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
                                                    sizeof (gftp_config_vars) * newentry->num_local_options_vars);

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

      *dest = g_malloc (endpos - curpos + 1);
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
      *dest = g_malloc (1);
      **dest = '\0';
      numargs--;
    }
  va_end (argp);
  return (1);
}


static void *
gftp_config_read_str (char *buf, int line)
{
  char *ret;

  ret = g_strdup (buf);
  return (ret);
}


static void
gftp_config_write_str (FILE *fd, void *data)
{
  fprintf (fd, "%s", (char *) data);
}


static void *
gftp_config_read_proxy (char *buf, int line)
{
  gftp_proxy_hosts * host;
  unsigned int nums[4];
  char *pos;

  host = g_malloc0 (sizeof (*host));
  if ((pos = strchr (buf, '/')) == NULL)
    host->domain = g_strdup (buf);
  else
    {
      *pos = '\0';
      sscanf (buf, "%u.%u.%u.%u", &nums[0], &nums[1], &nums[2], &nums[3]);
      host->ipv4_network_address = 
                      nums[0] << 24 | nums[1] << 16 | nums[2] << 8 | nums[3];

      if (strchr (pos + 1, '.') == NULL)
        host->ipv4_netmask = 0xffffffff << (32 - strtol (pos + 1, NULL, 10));
      else
        {
          sscanf (pos + 1, "%u.%u.%u.%u", &nums[0], &nums[1], &nums[2], 
                  &nums[3]);
          host->ipv4_netmask =
		    nums[0] << 24 | nums[1] << 16 | nums[2] << 8 | nums[3];
        }
    }

  return (host);
}


static void
gftp_config_write_proxy (FILE *fd, void *data)
{
  gftp_proxy_hosts * host;

  host = data;

  if (host->domain)
    fprintf (fd, "%s", host->domain);
  else
    fprintf (fd, "%d.%d.%d.%d/%d.%d.%d.%d",
             host->ipv4_network_address >> 24 & 0xff,
             host->ipv4_network_address >> 16 & 0xff,
             host->ipv4_network_address >> 8 & 0xff,
             host->ipv4_network_address & 0xff,
             host->ipv4_netmask >> 24 & 0xff,
             host->ipv4_netmask >> 16 & 0xff,
             host->ipv4_netmask >> 8 & 0xff, 
             host->ipv4_netmask & 0xff);
}


static void *
gftp_config_read_ext (char *buf, int line)
{
  gftp_file_extensions * tempext;

  tempext = g_malloc (sizeof (*tempext));
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


gftp_config_list_vars gftp_config_list[] = {
  {"dont_use_proxy",	gftp_config_read_proxy,	gftp_config_write_proxy, 
   NULL, 0,
   N_("This section specifies which hosts are on the local subnet and won't need to go out the proxy server (if available). Syntax: dont_use_proxy=.domain or dont_use_proxy=network number/netmask")},
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
  int line, i;
  size_t len;

  gftp_global_options_htable = g_hash_table_new (string_hash_function, 
                                                 string_hash_compare);

  gftp_register_config_vars (gftp_global_config_vars);

  protocol_list = NULL;
  for (i=0; gftp_protocols[i].register_options != NULL; i++)
    {
      if (gftp_protocols[i].shown)
        {
          protocol_list = g_realloc (protocol_list, sizeof (char *) * (i + 2));
          protocol_list[i] = gftp_protocols[i].name;
          protocol_list[i + 1] = NULL;
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

  if ((tempstr = expand_path (CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (1);
    }

  if (access (tempstr, F_OK) == -1)
    {
      temp1str = expand_path (BASE_CONF_DIR);
      if (access (temp1str, F_OK) == -1)
	{
	  if (mkdir (temp1str, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
	    {
	      printf (_("gFTP Error: Could not make directory %s: %s\n"),
		      temp1str, g_strerror (errno));
	      exit (1);
	    }
	}
      g_free (temp1str);

      temp1str = g_strdup_printf ("%s/gftprc", global_data_path);
      if (access (temp1str, F_OK) == -1)
	{
	  printf (_("gFTP Error: Cannot find master config file %s\n"),
		  temp1str);
	  printf (_("Did you do a make install?\n"));
	  exit (1);
	}
      copyfile (temp1str, tempstr);
      g_free (temp1str);
    }

  if ((conffile = fopen (tempstr, "r")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
	      g_strerror (errno));
      exit (1);
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
              exit (1);
            }
        }
      else
	{
	  printf (_("gFTP Warning: Skipping line %d in config file: %s\n"),
                  line, buf);
	}
    }

  if ((tempstr = expand_path (LOG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad log file name %s\n"), LOG_FILE);
      exit (1);
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
      fwrite ("\n# ", 1, 3, fd);
      if (*endpos == '\0')
	{
	  pos = endpos;
	  break;
	}
      else
	pos = endpos + 1;
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
  gftp_bookmarks_var * tempentry;
  char *bmhdr, *tempstr;
  FILE * bmfile;
  int i;

  bmhdr = N_("Bookmarks file for gFTP. Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>. Warning: Any comments that you add to this file WILL be overwritten");

  if ((tempstr = expand_path (BOOKMARKS_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad bookmarks file name %s\n"), CONFIG_FILE);
      exit (1);
    }

  if ((bmfile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open bookmarks file %s: %s\n"),
	      CONFIG_FILE, g_strerror (errno));
      exit (1);
    }

  g_free (tempstr);

  write_comment (bmfile, _(bmhdr));
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
      fprintf (bmfile,
	       "[%s]\nhostname=%s\nport=%d\nprotocol=%s\nremote directory=%s\nlocal directory=%s\nusername=%s\npassword=%s\naccount=%s\n",
	       tempstr, tempentry->hostname == NULL ? "" : tempentry->hostname,
	       tempentry->port, tempentry->protocol == NULL
	       || *tempentry->protocol ==
	       '\0' ? gftp_protocols[0].name : tempentry->protocol,
	       tempentry->remote_dir == NULL ? "" : tempentry->remote_dir,
	       tempentry->local_dir == NULL ? "" : tempentry->local_dir,
	       tempentry->user == NULL ? "" : tempentry->user,
	       !tempentry->save_password
	       || tempentry->pass == NULL ? "" : tempentry->pass,
	       tempentry->acct == NULL ? "" : tempentry->acct);

      if (tempentry->local_options_vars != NULL)
        {
          for (i=0; i<tempentry->num_local_options_vars; i++)
            {
              fprintf (bmfile, "%s=", tempentry->local_options_vars[i].key);
              gftp_option_types[tempentry->local_options_vars[i].otype].write_function (&tempentry->local_options_vars[i], bmfile, 1);
              fprintf (bmfile, "\n");
            }
        }

      fprintf (bmfile, "\n");
 
      if (tempentry->next == NULL)
	{
	  tempentry = tempentry->prev;
	  while (tempentry->next == NULL && tempentry->prev != NULL)
	    tempentry = tempentry->prev;
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
  gftp_config_vars * cv;
  GList *templist;
  FILE *conffile;
  char *tempstr;
  int i;

  if ((tempstr = expand_path (CONFIG_FILE)) == NULL)
    {
      printf (_("gFTP Error: Bad config file name %s\n"), CONFIG_FILE);
      exit (1);
    }

  if ((conffile = fopen (tempstr, "w+")) == NULL)
    {
      printf (_("gFTP Error: Cannot open config file %s: %s\n"), CONFIG_FILE,
	      g_strerror (errno));
      exit (1);
    }

  g_free (tempstr);

  write_comment (conffile, _("Config file for gFTP. Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>. Warning: Any comments that you add to this file WILL be overwritten. If a entry has a (*) in it's comment, you can't change it inside gFTP"));

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

          fprintf (conffile, "%s=", cv[i].key);
          gftp_option_types[cv[i].otype].write_function (&cv[i], conffile, 1);
          fprintf (conffile, "\n");
        }
    }
    
  for (i=0; gftp_config_list[i].list != NULL; i++)
    {
      fprintf (conffile, "\n");
      if (gftp_config_list[i].header != NULL)
        write_comment (conffile, _(gftp_config_list[i].header));

      for (templist = gftp_options_list;
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
      while (tempentry->next == NULL && tempentry->prev != NULL)
	tempentry = tempentry->prev;
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
	  while (tempentry->next == NULL && tempentry->prev != NULL)
            tempentry = tempentry->prev;
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
gftp_config_file_write_text (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  char *outstr;

  if (cv->value != NULL)
    {
      outstr = cv->value;
      if (*outstr != '\0')
        fprintf (fd, "%s", outstr);
      return (0);
    }
  else
    return (-1);
}


static int
gftp_config_file_write_hidetext (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  char *outstr;

  if (cv->value != NULL)
    {
      outstr = cv->value;
      if (*outstr != '\0')
        {
          if (to_config_file)
            fprintf (fd, "%s", outstr);
          else
            fprintf (fd, "*****");
        }
      return (0);
    }
  else
    return (-1);
}


static int
gftp_config_file_read_int (char *str, gftp_config_vars * cv, int line)
{
  cv->value = GINT_TO_POINTER(strtol (str, NULL, 10));
  return (0);
}


static int
gftp_config_file_write_int (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  fprintf (fd, "%d", GPOINTER_TO_INT(cv->value));
  return (0);
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
  float f;

  f = strtod (str, NULL);
  memcpy (&cv->value, &f, sizeof (cv->value));
  return (0);
}


static int
gftp_config_file_write_float (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  float f;

  memcpy (&f, &cv->value, sizeof (f));
  fprintf (fd, "%.2f", f);
  return (0);
}


static int
gftp_config_file_read_color (char *str, gftp_config_vars * cv, int line)
{
  char *red, *green, *blue;
  gftp_color * color;

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM && cv->value != NULL)
    g_free (cv->value);

  gftp_config_parse_args (str, 3, line, &red, &green, &blue);

  color = g_malloc (sizeof (*color));
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
gftp_config_file_write_color (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  gftp_color * color;

  color = cv->value;
  fprintf (fd, "%x:%x:%x", color->red, color->green, color->blue);
  return (0);
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
gftp_config_file_write_intcombo (gftp_config_vars * cv, FILE * fd, int to_config_file)
{
  char **clist;

  clist = cv->listdata;
  if (clist != NULL)
    fprintf (fd, "%s", clist[GPOINTER_TO_INT(cv->value)]);
  else
    fprintf (fd, _("<unknown>"));

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
  {gftp_config_file_read_text, gftp_config_file_write_text, NULL, NULL, NULL},
  {gftp_config_file_read_textcombo, gftp_config_file_write_text, NULL, NULL, NULL},
  {gftp_config_file_read_text, gftp_config_file_write_text, NULL, NULL, NULL},
  {gftp_config_file_read_text, gftp_config_file_write_hidetext, NULL, NULL, NULL},
  {gftp_config_file_read_int, gftp_config_file_write_int, NULL, NULL, NULL},
  {gftp_config_file_read_checkbox, gftp_config_file_write_int, NULL, NULL, NULL},
  {gftp_config_file_read_intcombo, gftp_config_file_write_intcombo, NULL, NULL, NULL},
  {gftp_config_file_read_float, gftp_config_file_write_float, NULL, NULL, NULL},
  {gftp_config_file_read_color, gftp_config_file_write_color, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL},
  {NULL, NULL, NULL, NULL, NULL}
};


void
gftp_lookup_global_option (char * key, void *value)
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
      exit (1);
    }
}


void
gftp_lookup_request_option (gftp_request * request, char * key, void *value)
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
gftp_set_global_option (char * key, void *value)
{
  gftp_config_vars * tmpconfigvar;

  if (gftp_global_options_htable != NULL &&
      (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                           key)) != NULL)
    {
      memcpy (&tmpconfigvar->value, value, sizeof (tmpconfigvar->value));

      /* FIXME - only set this variable if the value has changed */
      gftp_configuration_changed = 1;
    }
  else
    {
      fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
      exit (1);
    }
}


void
gftp_set_request_option (gftp_request * request, char * key, void *value)
{
  gftp_config_vars * tmpconfigvar;

  if (request->local_options_hash == NULL)
    request->local_options_hash = g_hash_table_new (string_hash_function,
                                                    string_hash_compare);

  if ((tmpconfigvar = g_hash_table_lookup (request->local_options_hash,
                                           key)) != NULL)
    memcpy (&tmpconfigvar->value, &value, sizeof (tmpconfigvar->value));
  else
    {
      if (gftp_global_options_htable == NULL ||
          (tmpconfigvar = g_hash_table_lookup (gftp_global_options_htable,
                                               key)) == NULL)
        {
          fprintf (stderr, _("FATAL gFTP Error: Config option '%s' not found in global hash table\n"), key);
          exit (1);
        }
      
      request->num_local_options_vars++;
      request->local_options_vars = g_realloc (request->local_options_vars, 
                                               sizeof (gftp_config_vars) * request->num_local_options_vars);

      memcpy (&request->local_options_vars[request->num_local_options_vars - 1],
              tmpconfigvar, sizeof (*tmpconfigvar));
      memcpy (&request->local_options_vars[request->num_local_options_vars - 1].value, &value, sizeof (value));

      g_hash_table_insert (request->local_options_hash, request->local_options_vars[request->num_local_options_vars - 1].key, &request->local_options_vars[request->num_local_options_vars - 1]);
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
                         gftp_config_vars * orig_options,
                         int num_local_options_vars)
{
  int i;

  if (orig_options == NULL || num_local_options_vars == 0)
    {
      *new_options_vars = NULL;
      *new_options_hash = NULL;
      return;
    }

  *new_options_hash = g_hash_table_new (string_hash_function,
                                        string_hash_compare);

  *new_options_vars = g_malloc (sizeof (gftp_config_vars) * num_local_options_vars);
  memcpy (*new_options_vars, orig_options,
          sizeof (gftp_config_vars) * num_local_options_vars);

  for (i=0; i<num_local_options_vars; i++)
    {
      g_hash_table_insert (*new_options_hash, (*new_options_vars)[i].key,
                           &(*new_options_vars)[i]);

      /* FIXME 2.0.15 - copy option values */
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
      gftp_free_bookmark (tempentry);
      g_free (tempentry->path);

      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }

      while (tempentry->next == NULL && tempentry->prev != NULL)
        {
          delentry = tempentry;
          tempentry = tempentry->prev;
          g_free (delentry);
        }

      delentry = tempentry;
      tempentry = tempentry->next;
      g_free (delentry);
    }
}

