/*****************************************************************************/
/*  gftp-text.c - text port of gftp                                          */
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

#include "gftp-text.h"
static const char cvsid[] = "$Id$";

static gftp_request * gftp_text_locreq = NULL;
static gftp_request * gftp_text_remreq = NULL;
static volatile int cancel = 0;
static int number_commands = 30;

struct _gftp_text_methods gftp_text_methods[] = {
        {N_("about"), 	2, gftp_text_about,	NULL,
         N_("Shows gFTP information"), NULL},
        {N_("ascii"),	2, gftp_text_ascii,	&gftp_text_remreq,
         N_("Sets the current file transfer mode to Ascii (only for FTP)"), NULL},
	{N_("binary"),	1, gftp_text_binary,	&gftp_text_remreq,
         N_("Sets the current file transfer mode to Binary (only for FTP)"), NULL},
        {N_("cd"), 	2, gftp_text_cd, 	&gftp_text_remreq,
         N_("Changes the remote working directory"), NULL},
        {N_("chdir"), 	3, gftp_text_cd, 	&gftp_text_remreq,
         N_("Changes the remote working directory"), NULL},
        {N_("chmod"), 	3, gftp_text_chmod,	&gftp_text_remreq,
         N_("Changes the permissions of a remote file"), NULL},
        {N_("clear"),	3, gftp_text_clear,	NULL,
         N_("Available options: cache"), 	gftp_text_clear_show_subhelp},
        {N_("close"), 	3, gftp_text_close, 	&gftp_text_remreq,
         N_("Disconnects from the remote site"), NULL},
        {N_("delete"), 	1, gftp_text_delete,	&gftp_text_remreq,
         N_("Removes a remote file"), NULL},
        {N_("get"),	1, gftp_text_mget_file,	NULL,
         N_("Downloads remote file(s)"), NULL},
        {N_("help"), 	1, gftp_text_help, 	NULL,
         N_("Shows this help screen"), NULL},
        {N_("lcd"), 	3, gftp_text_cd, 	&gftp_text_locreq,
         N_("Changes the local working directory"), NULL},
        {N_("lchdir"), 	4, gftp_text_cd, 	&gftp_text_locreq,
         N_("Changes the local working directory"), NULL},
        {N_("lchmod"), 	4, gftp_text_chmod, 	&gftp_text_locreq,
         N_("Changes the permissions of a local file"), NULL},
        {N_("ldelete"), 2, gftp_text_delete, 	&gftp_text_locreq,
         N_("Removes a local file"), NULL},
	{N_("lls"), 	2, gftp_text_ls, 	&gftp_text_locreq,
         N_("Shows the directory listing for the current local directory"), NULL},
        {N_("lmkdir"), 	2, gftp_text_mkdir, 	&gftp_text_locreq,
         N_("Creates a local directory"), NULL},
        {N_("lpwd"), 	2, gftp_text_pwd, 	&gftp_text_locreq,
         N_("Show current local directory"), NULL},
        {N_("lrename"), 3, gftp_text_rename, 	&gftp_text_locreq,
         N_("Rename a local file"), NULL},
        {N_("lrmdir"), 	3, gftp_text_rmdir, 	&gftp_text_locreq,
         N_("Remove a local directory"), NULL},
	{N_("ls"), 	2, gftp_text_ls,	&gftp_text_remreq,
         N_("Shows the directory listing for the current remote directory"), NULL},
        {N_("mget"),	2, gftp_text_mget_file,	NULL,
         N_("Downloads remote file(s)"), NULL},
        {N_("mkdir"), 	2, gftp_text_mkdir,	&gftp_text_remreq,
         N_("Creates a remote directory"), NULL},
        {N_("mput"),	2, gftp_text_mput_file,	NULL,
         N_("Uploads local file(s)"), NULL},
        {N_("open"), 	1, gftp_text_open, 	&gftp_text_remreq,
         N_("Opens a connection to a remote site"), NULL},
        {N_("put"),	2, gftp_text_mput_file,	NULL,
         N_("Uploads local file(s)"), NULL},
        {N_("pwd"), 	2, gftp_text_pwd, 	&gftp_text_remreq,
         N_("Show current remote directory"), NULL},
        {N_("quit"), 	1, gftp_text_quit, 	NULL,
         N_("Exit from gFTP"), NULL},
        {N_("rename"), 	2, gftp_text_rename,	&gftp_text_remreq,
         N_("Rename a remote file"), NULL},
        {N_("rmdir"), 	2, gftp_text_rmdir,	&gftp_text_remreq,
         N_("Remove a remote directory"), NULL},
        {N_("set"), 	1, gftp_text_set, 	NULL,
         N_("Show configuration file variables. You can also set variables by set var=val"), gftp_text_set_show_subhelp},
        {NULL, 		0, NULL,		NULL, 	NULL}};

int
main (int argc, char **argv)
{
  char *pos, *stpos, *startup_directory;
  gftp_request * request;
  size_t len, cmdlen;
  int i;
#if HAVE_LIBREADLINE
  char *tempstr, prompt[20];
#else
  char tempstr[512];
#endif

#ifdef HAVE_GETTEXT
  setlocale (LC_ALL, "");
  bindtextdomain ("gftp", LOCALE_DIR);
#endif

  signal (SIGCHLD, sig_child);
  signal (SIGPIPE, SIG_IGN); 

  gftp_read_config_file (SHARE_DIR);

  if (gftp_parse_command_line (&argc, &argv) != 0)
    exit (0);

  /* SSH doesn't support reading the password with askpass via the command 
     line */

  gftp_text_remreq = gftp_request_new ();
  gftp_set_request_option (gftp_text_remreq, "ssh_use_askpass", 
                           GINT_TO_POINTER(0));
  gftp_set_request_option (gftp_text_remreq, "sshv2_use_sftp_subsys", 
                           GINT_TO_POINTER(0));
  gftp_text_remreq->logging_function = gftp_text_log;

  gftp_text_locreq = gftp_request_new ();
  gftp_set_request_option (gftp_text_locreq, "ssh_use_askpass", 
                           GINT_TO_POINTER(0));
  gftp_set_request_option (gftp_text_locreq, "sshv2_use_sftp_subsys", 
                           GINT_TO_POINTER(0));

  gftp_text_locreq->logging_function = gftp_text_log;
  if (gftp_protocols[GFTP_LOCAL_NUM].init (gftp_text_locreq) == 0)
    {
      gftp_lookup_request_option (gftp_text_locreq, "startup_directory", 
                                  &startup_directory);
      if (*startup_directory != '\0')
        gftp_set_directory (gftp_text_locreq, startup_directory);

      gftp_connect (gftp_text_locreq);
    }

  gftp_text_log (gftp_logging_misc, NULL, "%s, Copyright (C) 1998-2003 Brian Masney <", gftp_version);
  gftp_text_log (gftp_logging_recv, NULL, "masneyb@gftp.org");
  gftp_text_log (gftp_logging_misc, NULL, _(">.\nIf you have any questions, comments, or suggestions about this program, please feel free to email them to me. You can always find out the latest news about gFTP from my website at http://www.gftp.org/\n"));
  gftp_text_log (gftp_logging_misc, NULL, "\n");
  gftp_text_log (gftp_logging_misc, NULL, _("gFTP comes with ABSOLUTELY NO WARRANTY; for details, see the COPYING file. This is free software, and you are welcome to redistribute it under certain conditions; for details, see the COPYING file\n"));
  gftp_text_log (gftp_logging_misc, NULL, "\n");

  if (argc == 3 && strcmp (argv[1], "-d") == 0)
    {
      if ((pos = strrchr (argv[2], '/')) != NULL)
        *pos = '\0';
      gftp_text_open (gftp_text_remreq, argv[2], NULL);

      if (pos != NULL)
        *pos = '/';

      gftp_text_mget_file (gftp_text_remreq, pos + 1, NULL);
      exit (0);
    }
  else if (argc == 2)
    gftp_text_open (gftp_text_remreq, argv[1], NULL);

#if HAVE_LIBREADLINE
  g_snprintf (prompt, sizeof (prompt), "%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
  while ((tempstr = readline (prompt)))
#else
  printf ("%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
  while (fgets (tempstr, sizeof (tempstr), stdin) != NULL)
#endif
    {
      len = strlen (tempstr);
      if (tempstr[len - 1] == '\n')
        tempstr[--len] = '\0';
      if (tempstr[len - 1] == '\r')
        tempstr[--len] = '\0';

      for (stpos = tempstr; *stpos == ' '; stpos++);

      for (pos = tempstr + len - 1; 
           (*pos == ' ' || *pos == '\t') && pos > tempstr; 
           pos--)
        *pos = '\0';

      if (*stpos == '\0')
        {
#if !HAVE_LIBREADLINE
          printf ("%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
#endif
          continue;
        }

#if HAVE_LIBREADLINE
      add_history (tempstr);
#endif

      if ((pos = strchr (stpos, ' ')) != NULL)
        *pos = '\0';
      cmdlen = strlen (stpos);

      for (i=0; gftp_text_methods[i].command != NULL; i++)
        {
          if (strcmp (gftp_text_methods[i].command, stpos) == 0)
            break;
          else if (cmdlen >= gftp_text_methods[i].minlen &&
                   strncmp (gftp_text_methods[i].command, stpos, cmdlen) == 0)
            break;
        }

      if (pos != NULL)
        pos++;
      else
        pos = "";

      if (gftp_text_methods[i].command != NULL)
        {
          if (gftp_text_methods[i].request != NULL)
            request = *gftp_text_methods[i].request;
          else
            request = NULL;

          if (gftp_text_methods[i].func (request, pos, NULL) == 0)
            break;
        }
      else
        gftp_text_log (gftp_logging_error, NULL, 
                       _("Error: Command not recognized\n"));

#if HAVE_LIBREADLINE
     free (tempstr);
#else
     printf ("%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
#endif
    }
 
  gftp_text_quit (NULL, NULL, NULL);

  return (0);
}


void
gftp_text_log (gftp_logging_level level, gftp_request * request, 
               const char *string, ...)
{
  char tempstr[512], *stpos, *endpos, *utf8_str = NULL, *outstr;
  va_list argp;
  int sw;

  g_return_if_fail (string != NULL);

  switch (level)
    {
      case gftp_logging_send:
        printf ("%s", COLOR_GREEN);
        break;
      case gftp_logging_recv:
        printf ("%s", COLOR_YELLOW);
        break;
      case gftp_logging_error:
        printf ("%s", COLOR_RED);
        break;
      default:
        printf ("%s", COLOR_DEFAULT);
        break;
    }

  va_start (argp, string);
  g_vsnprintf (tempstr, sizeof (tempstr), string, argp);
  va_end (argp);

#if GLIB_MAJOR_VERSION > 1
  if (!g_utf8_validate (tempstr, -1, NULL))
    utf8_str = gftp_string_to_utf8 (request, tempstr);
#endif

  if (utf8_str != NULL)
    outstr = utf8_str;
  else
    outstr = tempstr;

  if (gftp_logfd != NULL)
    {
      fwrite (outstr, 1, strlen (outstr), gftp_logfd);
      if (ferror (gftp_logfd))
        {
          fclose (gftp_logfd);
          gftp_logfd = NULL;
        }
      else
        fflush (gftp_logfd);
    }

  sw = gftp_text_get_win_size ();
  stpos = outstr;
  endpos = outstr + 1;
  do
    {
      if (strlen (stpos) <= sw)
        {
          printf ("%s", stpos);
          break;
        }
      for (endpos = stpos + sw - 1; *endpos != ' ' && endpos > stpos; endpos--);
      if (endpos != stpos)
        {
          *endpos = '\0';
        }
      printf ("%s\n", stpos);
      stpos = endpos + 1;
    }
  while (stpos != endpos);
  
  printf ("%s", COLOR_DEFAULT);

  if (utf8_str != NULL)
    g_free (utf8_str);
}


int
gftp_text_open (gftp_request * request, char *command, gpointer *data)
{
  char tempstr[255], *pos;

  if (GFTP_IS_CONNECTED (request))
    {
      gftp_disconnect (request);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request,
          _("usage: open [[ftp://][user:pass@]ftp-site[:port][/directory]]\n"));
      return (1);
    }
  
  if (gftp_parse_url (request, command) < 0)
    return (1);

  if (request->need_userpass)
    {
      if (request->username == NULL || *request->username == '\0')
        {
          if ((pos = gftp_text_ask_question ("Username [anonymous]", 1, tempstr, 
                                             sizeof (tempstr))) != NULL)
            {
              gftp_set_username (request, pos);
              gftp_set_password (request, NULL);
            }
        }

      if (request->username != NULL &&
          strcmp (request->username, "anonymous") != 0 && 
          (request->password == NULL || *request->password == '\0'))
        {
          if ((pos = gftp_text_ask_question ("Password", 0, tempstr, 
                                             sizeof (tempstr))) == NULL)
            return (1);
          gftp_set_password (request, pos);
        }
    }

  gftp_connect (request);
  return (1);
}


int
gftp_text_close (gftp_request * request, char *command, gpointer *data)
{
  gftp_disconnect (request);
  return (1);
}


int
gftp_text_about (gftp_request * request, char *command, gpointer *data)
{
  char *str;

  gftp_text_log (gftp_logging_misc, request,
      "%s. Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>\n", 
      gftp_version);

  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    gftp_text_log (gftp_logging_misc, request, "%s\n", str);
  return (1);
}


int
gftp_text_quit (gftp_request * request, char *command, gpointer *data)
{
  gftp_request_destroy (gftp_text_locreq, 1);
  gftp_request_destroy (gftp_text_remreq, 1);
  gftp_shutdown();

  return (0);
}


int
gftp_text_pwd (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }
  gftp_text_log (gftp_logging_misc, request, "%s\n", request->directory);
  return (1);
}


int
gftp_text_cd (gftp_request * request, char *command, gpointer *data)
{
  char *tempstr, *newdir = NULL;

  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request, 
                     _("usage: chdir <directory>\n"));
      return (1);
    }
  else if (request->protonum == GFTP_LOCAL_NUM)
    {
      if (*command != '/' && request->directory != NULL)
        {
          tempstr = g_strconcat (request->directory, "/", command, NULL);
          newdir = expand_path (tempstr);
          g_free (tempstr);
        }
      else
        newdir = expand_path (command);

      if (newdir == NULL)
        {
          gftp_text_log (gftp_logging_error, request, 
                         _("usage: chdir <directory>\n"));
          return (1);
        }
    }

  gftp_set_directory (request, newdir != NULL ? newdir : command);

  if (newdir != NULL)
    g_free (newdir);

  return (1);
}


int
gftp_text_mkdir (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request,
                     _("usage: mkdir <new directory>\n"));
    }
  else
    {
      gftp_make_directory (request, command);
    }
  return (1);
}


int
gftp_text_rmdir (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request, 
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request, 
                     _("usage: rmdir <directory>\n"));
    }
  else
    {
      gftp_remove_directory (request, command);
    }
  return (1);
}


int
gftp_text_delete (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request,
                     _("usage: delete <file>\n"));
    }
  else
    {
      if (gftp_remove_file (request, command) == 0)
        gftp_delete_cache_entry (request, NULL, 0);
    }
  return (1);
}


int
gftp_text_rename (gftp_request * request, char *command, gpointer *data)
{
  char *pos;

  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      gftp_text_log (gftp_logging_error, request,
                     _("usage: rename <old name> <new name>\n"));
    }
  else
    {
      gftp_rename_file (request, command, pos);
    }
  return (1);
}


int
gftp_text_chmod (gftp_request * request, char *command, gpointer *data)
{
  char *pos;

  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      gftp_text_log (gftp_logging_error, request,
                     _("usage: chmod <mode> <file>\n"));
    }
  else
    {
      gftp_chmod (request, pos, strtol (command, NULL, 10));
    }
  return (1);
}


int
gftp_text_ls (gftp_request * request, char *command, gpointer *data)
{
  GList * files, * templist, * delitem;
  char *color, *filespec, *tempstr;
  int sortcol, sortasds;
  gftp_file * fle;
  time_t curtime;

  time (&curtime);
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  filespec = *command != '\0' ? command : NULL;
  if (gftp_list_files (request) != 0)
    return (1);

  files = NULL;
  fle = g_malloc0 (sizeof (*fle));
  while (gftp_get_next_file (request, NULL, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0)
        {
          gftp_file_destroy (fle);
          continue;
        }
      files = g_list_prepend (files, fle);
      fle = g_malloc0 (sizeof (*fle));
    }
  g_free (fle);

  if (files == NULL)
    {
      gftp_end_transfer (request);
      return (1);
    }

  if (request == gftp_text_locreq)
    {
      gftp_lookup_request_option (request, "local_sortcol", &sortcol);
      gftp_lookup_request_option (request, "local_sortasds", &sortasds);
    }
  else
    {
      gftp_lookup_request_option (request, "remote_sortcol", &sortcol);
      gftp_lookup_request_option (request, "remote_sortasds", &sortasds);
    }

  files = gftp_sort_filelist (files, sortcol, sortasds);
  delitem = NULL;
  for (templist = files; templist != NULL; templist = templist->next)
    {
      if (delitem != NULL)
        {
          fle = delitem->data;
          gftp_file_destroy (fle);
          g_free (fle);
          delitem = NULL;
        }

      fle = templist->data;

      if (*fle->attribs == 'd')
        color = COLOR_BLUE;
      else if (*fle->attribs == 'l')
        color = COLOR_WHITE;
      else if (strchr (fle->attribs, 'x') != NULL)
        color = COLOR_GREEN;
      else
        color = COLOR_DEFAULT;

      tempstr = gftp_gen_ls_string (fle, color, COLOR_DEFAULT);
      printf ("%s\n", tempstr);
      g_free (tempstr);

      delitem = templist;
    }

  gftp_end_transfer (request);

  if (delitem != NULL)
    {
      fle = delitem->data;
      gftp_file_destroy (fle);
      g_free (fle);
      delitem = NULL;
    }

  if (files != NULL)
    g_list_free (files);

  return (1);
}


int
gftp_text_binary (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  gftp_set_request_option (gftp_text_remreq, "ascii_transfers", 
                           GINT_TO_POINTER(0));
  gftp_set_request_option (gftp_text_locreq, "ascii_transfers", 
                           GINT_TO_POINTER(0));
  return (1);
}


int
gftp_text_ascii (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  gftp_set_request_option (gftp_text_remreq, "ascii_transfers", 
                           GINT_TO_POINTER(1));
  gftp_set_request_option (gftp_text_locreq, "ascii_transfers", 
                           GINT_TO_POINTER(1));
  return (1);
}


int
gftp_text_mget_file (gftp_request * request, char *command, gpointer *data)
{
  gftp_transfer * transfer;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request, 
                     _("usage: mget <filespec>\n"));
      return (1);
    }

  transfer = gftp_tdata_new ();
  transfer->fromreq = gftp_text_remreq;
  transfer->toreq = gftp_text_locreq;
  transfer->transfer_direction = GFTP_DIRECTION_DOWNLOAD;

  /* FIXME - ask whether to resume/skip/overwrite */
  if (gftp_list_files (transfer->fromreq) != 0)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }
  fle = g_malloc0 (sizeof (*fle));
  while (gftp_get_next_file (transfer->fromreq, command, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0 || strcmp (fle->file, "..") == 0)
        {
          gftp_file_destroy (fle);
          continue;
        }
      transfer->files = g_list_append (transfer->files, fle);
      fle = g_malloc (sizeof (*fle));
    }
  g_free (fle);
  gftp_end_transfer (transfer->fromreq);

  if (transfer->files == NULL)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  if (gftp_get_all_subdirs (transfer, NULL) != 0)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  if (transfer->files == NULL)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  gftp_text_transfer_files (transfer);
  transfer->fromreq = transfer->toreq = NULL;
  free_tdata (transfer);
  return (1);
}


int
gftp_text_mput_file (gftp_request * request, char *command, gpointer *data)
{
  gftp_transfer * transfer;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, request, 
                     _("usage: mput <filespec>\n"));
      return (1);
    }

  transfer = gftp_tdata_new ();
  transfer->fromreq = gftp_text_locreq;
  transfer->toreq = gftp_text_remreq;
  transfer->transfer_direction = GFTP_DIRECTION_UPLOAD;

  if (gftp_list_files (transfer->fromreq) != 0)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }
  fle = g_malloc (sizeof (*fle));
  while (gftp_get_next_file (transfer->fromreq, command, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0 || strcmp (fle->file, "..") == 0)
        {
          gftp_file_destroy (fle);
          continue;
        }
      transfer->files = g_list_append (transfer->files, fle);
      fle = g_malloc (sizeof (*fle));
    }
  g_free (fle);
  gftp_end_transfer (transfer->fromreq);

  if (transfer->files == NULL)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  if (gftp_get_all_subdirs (transfer, NULL) != 0)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  if (transfer->files == NULL)
    {
      transfer->fromreq = transfer->toreq = NULL;
      free_tdata (transfer);
      return (1);
    }

  gftp_text_transfer_files (transfer);
  transfer->fromreq = transfer->toreq = NULL;
  free_tdata (transfer);
  return (1);
}


int
gftp_text_transfer_files (gftp_transfer * transfer)
{
  int i, j, sw, tot, preserve_permissions;
  char buf[8192], *progress = "|/-\\";
  struct timeval updatetime;
  long fromsize, total;
  gftp_file * curfle;
  ssize_t num_read;
  mode_t mode;

  gftp_lookup_request_option (transfer->fromreq, "preserve_permissions",
                              &preserve_permissions);

  for (transfer->curfle = transfer->files;
       transfer->curfle != NULL;
       transfer->curfle = transfer->curfle->next)
    {
      curfle = transfer->curfle->data;
      if (curfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
        continue;

      if (curfle->isdir && transfer->toreq->mkdir != NULL)
        {
          transfer->toreq->mkdir (transfer->toreq, curfle->destfile);
          continue;
        }

      transfer->curtrans = curfle->startsize;
      fromsize = gftp_transfer_file (transfer->fromreq, curfle->file, -1,
                                     curfle->startsize, transfer->toreq, curfle->destfile, 
                                     -1, curfle->startsize);
      if (fromsize < 0)
        return (1);

      gettimeofday (&transfer->starttime, NULL);
      memcpy (&transfer->lasttime, &transfer->starttime, 
              sizeof (transfer->lasttime));
      memset (&updatetime, 0, sizeof (updatetime));
  
      total = 0;
      i = 0;
      num_read = -1;
      while (!cancel && (num_read = gftp_get_next_file_chunk (transfer->fromreq,
                                                        buf, sizeof (buf))) > 0)
        {
          printf ("\r%c ", progress[i++]);
          fflush (stdout);
          if (progress[i] == '\0')
            i = 0;

          total += num_read;
          gftp_calc_kbs (transfer, num_read);
          if (transfer->lasttime.tv_sec - updatetime.tv_sec >= 1 || total >= fromsize)
            {
              sw = gftp_text_get_win_size () - 20;
              tot = (float) total / (float) fromsize * (float) sw;
                        
              if (tot > sw)
                tot = sw;
              printf ("[");
              for (j=0; j<tot; j++)
                printf ("=");
              for (j=0; j<sw-tot; j++)
                printf (" ");
              printf ("] @ %.2fKB/s", transfer->kbs);
              fflush (stdout);
              memcpy (&updatetime, &transfer->lasttime, sizeof (updatetime));
            }

          if (gftp_put_next_file_chunk (transfer->toreq, buf, num_read) < 0)
            {
              num_read = -1;
              break;
            }
        }
      printf ("\n");

      if (num_read < 0)
        {
          gftp_text_log (gftp_logging_misc, transfer->fromreq, 
                         _("Could not download %s\n"), curfle->file);
          gftp_disconnect (transfer->fromreq);
          gftp_disconnect (transfer->toreq);
        }
      else
        {
          gftp_text_log (gftp_logging_misc, transfer->fromreq, 
                         _("Successfully transferred %s\n"), curfle->file);
          gftp_end_transfer (transfer->fromreq);
          gftp_end_transfer (transfer->toreq);
        }

      if (!curfle->is_fd && preserve_permissions)
        {
          if (curfle->attribs)
            {
              mode = gftp_parse_attribs (curfle->attribs);
              if (mode != 0)
                gftp_chmod (transfer->toreq, curfle->destfile, mode);
            } 

          if (curfle->datetime != 0)
            gftp_set_file_time (transfer->toreq, curfle->destfile,
                                curfle->datetime);
        }
    }
  return (1);
}


int
gftp_text_help (gftp_request * request, char *command, gpointer *data)
{
  int i, j, ele, numrows, numcols = 6, handled;
  char *pos;

  if (command != NULL && *command != '\0')
    {
      for (pos = command; *pos != ' ' && *pos != '\0'; pos++);
      if (*pos == ' ')
        {
          *pos++ = '\0';
          if (*pos == '\0')
            pos = NULL;
        }
      else
        pos = NULL;

      for (i=0; gftp_text_methods[i].command != NULL; i++)
        {
          if (strcmp (gftp_text_methods[i].command, command) == 0)
            break;
        }

      if (gftp_text_methods[i].cmd_description != NULL)
        {
          if (pos != NULL && gftp_text_methods[i].subhelp_func != NULL)
            handled = gftp_text_methods[i].subhelp_func (pos);
          else
            handled = 0;

          if (!handled)
            printf ("%s\n", _(gftp_text_methods[i].cmd_description));
        }
      else
        *command = '\0';
    }

  if (command == NULL || *command == '\0')
    {
      numrows = number_commands / numcols;
      if (number_commands % numcols != 0)
        numrows++;

      printf (_("Supported commands:\n\n"));
      for (i=0; i<numrows; i++)
        {
          printf ("     ");
          for (j=0; j<numcols; j++)
            { 
              ele = i + j * numrows;
              if (ele >= number_commands)
                break;
              printf ("%-10s", gftp_text_methods[ele].command);
            }
         printf ("\n");
        }

      printf ("\n");
    }
  return (1);
}


int
gftp_text_set (gftp_request * request, char *command, gpointer *data)
{
  gftp_config_vars * cv, newcv;
  char *pos, *backpos;
  GList * templist;
  int i;

  if (command == NULL || *command == '\0')
    {
      for (templist = gftp_options_list; 
           templist != NULL; 
           templist = templist->next)
        {
          cv = templist->data;

          for (i=0; cv[i].key != NULL; i++)
            {
              if (!(cv[i].ports_shown & GFTP_PORT_TEXT))
                continue;

              if (*cv[i].key == '\0' || 
                  gftp_option_types[cv[i].otype].write_function == NULL)
                continue;

              printf ("%s = ", cv[i].key);
              gftp_option_types[cv[i].otype].write_function (&cv[i], stdout, 0);
              printf ("\n");
            }
        }
    }
  else
    {
      if ((pos = strchr (command, '=')) == NULL)
        {
          gftp_text_log (gftp_logging_error, request,
                         _("usage: set [variable = value]\n"));
          return (1);
        }
      *pos = '\0';

      for (backpos = pos - 1; 
           (*backpos == ' ' || *backpos == '\t') && backpos > command; 
           backpos--)
        *backpos = '\0';
      for (++pos; *pos == ' ' || *pos == '\t'; pos++);

      if ((cv = g_hash_table_lookup (gftp_global_options_htable, command)) == NULL)
        {
          gftp_text_log (gftp_logging_error, request,
                         _("Error: Variable %s is not a valid configuration variable.\n"), command);
          return (1);
        }

      if (!(cv->ports_shown & GFTP_PORT_TEXT))
        {
          gftp_text_log (gftp_logging_error, request,
                         _("Error: Variable %s is not available in the text port of gFTP\n"), command);
          return (1);
        }

      if (gftp_option_types[cv->otype].read_function != NULL)
        {
          memcpy (&newcv, cv, sizeof (newcv));
          newcv.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

          gftp_option_types[cv->otype].read_function (pos, &newcv, 1);

          gftp_set_global_option (command, newcv.value);

          if (newcv.flags & GFTP_CVARS_FLAGS_DYNMEM)
            g_free (newcv.value);
        }
    }

  return (1);
}


int
gftp_text_clear (gftp_request * request, char *command, gpointer *data)
{
  if (strcasecmp (command, "cache") == 0)
    gftp_clear_cache_files ();
  else
    gftp_text_log (gftp_logging_error, request, _("Invalid argument\n"));
  return (1);
}


char *
gftp_text_ask_question (const char *question, int echo, char *buf, size_t size)
{
  struct termios term, oldterm;
  sigset_t sig, sigsave;
  char *pos, *termname;
  FILE *infd;

  if (!echo)
    {
      sigemptyset (&sig);
      sigaddset (&sig, SIGINT);
      sigaddset (&sig, SIGTSTP);
      sigprocmask (SIG_BLOCK, &sig, &sigsave);

      termname = ctermid (NULL);
      if ((infd = fopen (termname, "r+")) == NULL)
        {
          
          gftp_text_log (gftp_logging_error, NULL, 
                         _("Cannot open controlling terminal %s\n"), termname);
          return (NULL);
        }

      tcgetattr (0, &term);
      oldterm = term;
      term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL); 
      tcsetattr (fileno (infd), TCSAFLUSH, &term);
    }
  else
    infd = stdin;

  printf ("%s%s%s: ", COLOR_BLUE, question, COLOR_DEFAULT);

  if (fgets (buf, size, infd) == NULL)
    return (NULL);
  buf[size - 1] = '\0';

  if (!echo)
    {
      printf ("\n");
      tcsetattr (fileno (infd), TCSAFLUSH, &oldterm);
      fclose (infd);
      sigprocmask (SIG_SETMASK, &sigsave, NULL);
    }

  for (pos = buf + strlen (buf) - 1; *pos == ' ' || *pos == '\r' ||
                                     *pos == '\n'; pos--);
  *(pos+1) = '\0';

  for (pos = buf; *pos == ' '; pos++);  
  if (*pos == '\0')
    return (NULL);

  return (pos);
}


int
gftp_text_get_win_size (void)
{
  struct winsize size;
  int ret;

  if (ioctl (0, TIOCGWINSZ, (char *) &size) < 0)
    ret = 80;
  else
    ret = size.ws_col;
  return (ret);
}


void
sig_child (int signo)
{
}


int
gftp_text_set_show_subhelp (char *topic)
{
  gftp_config_vars * cv;

  if ((cv = g_hash_table_lookup (gftp_global_options_htable, topic)) != NULL)
    {
      printf ("%s\n", cv->comment);
      return (1);
    }

  return (0);
}


int
gftp_text_clear_show_subhelp (char *topic)
{
  if (strcmp (topic, "cache") == 0)
    {
      printf (_("Clear the directory cache\n"));
      return (1);
    }

  return (0);
}

