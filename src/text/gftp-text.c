/*****************************************************************************/
/*  gftp-text.c - text port of gftp                                          */
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

#include "gftp-text.h"
static const char cvsid[] = "$Id$";

static gftp_request * gftp_text_locreq = NULL;
static gftp_request * gftp_text_remreq = NULL;
static volatile int cancel = 0;
static int configuration_changed = 0,
           number_commands = 30;

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
  gftp_request * request;
  size_t len, cmdlen;
  char *pos, *stpos;
  int i;
#ifdef HAVE_LIBREADLINE
  char *tempstr, prompt[20];
#else
  char tempstr[512];
#endif

#ifdef HAVE_GETTEXT
  setlocale (LC_ALL, "");
  bindtextdomain ("gftp", LOCALE_DIR);
  textdomain ("gftp");
#endif

  signal (SIGCHLD, sig_child);
  signal (SIGPIPE, SIG_IGN); 

  gftp_read_config_file (argv, 0);

  /* SSH doesn't support reading the password with askpass via the command 
     line */
  ssh_use_askpass = sshv2_use_sftp_subsys = 0;

  if (gftp_parse_command_line (&argc, &argv) != 0)
    exit (0);

  gftp_text_remreq = gftp_request_new ();
  gftp_text_remreq->logging_function = gftp_text_log;

  gftp_text_locreq = gftp_request_new ();
  gftp_text_locreq->logging_function = gftp_text_log;
  gftp_protocols[GFTP_LOCAL_NUM].init (gftp_text_locreq);
  if (startup_directory != NULL && *startup_directory != '\0')
    gftp_set_directory (gftp_text_locreq, startup_directory);
  gftp_connect (gftp_text_locreq);

  gftp_text_log (gftp_logging_misc, NULL, "%s, Copyright (C) 1998-2002 Brian Masney <", version);
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

#ifdef HAVE_LIBREADLINE
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
#ifndef HAVE_LIBREADLINE
          printf ("%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
#endif
          continue;
        }

#ifdef HAVE_LIBREADLINE
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

#ifdef HAVE_LIBREADLINE
     free (tempstr);
#else
     printf ("%sftp%s> ", COLOR_BLUE, COLOR_DEFAULT);
#endif
    }
 
  if (logfd != NULL)
    fclose (logfd);
  gftp_text_quit (NULL, NULL, NULL);

  return (0);
}


void
gftp_text_log (gftp_logging_level level, void *ptr, const char *string, ...)
{
  char tempstr[512], *stpos, *endpos;
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

  if (logfd != NULL)
    {
      fwrite (tempstr, 1, strlen (tempstr), logfd);
      if (ferror (logfd))
        {
          fclose (logfd);
          logfd = NULL;
        }
      else
        fflush (logfd);
    }

  sw = gftp_text_get_win_size ();
  stpos = tempstr;
  endpos = tempstr + 1;
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
      gftp_text_log (gftp_logging_error, NULL,
          _("usage: open [[ftp://][user:pass@]ftp-site[:port][/directory]]\n"));
      return (1);
    }
  
  if (gftp_parse_url (request, command) < 0)
    {
      gftp_text_log (gftp_logging_error, NULL, 
                     _("Could not parse URL %s\n"), command);
      return (1);
    }

  if (strcmp (GFTP_GET_USERNAME (request), "anonymous") == 0)
    {
      if ((pos = gftp_text_ask_question ("Username [anonymous]", 1, tempstr, 
                                         sizeof (tempstr))) != NULL)
        {
          gftp_set_username (request, pos);
          if (request->password)
            {
              g_free (request->password);
              request->password = NULL;
            }
        }
    }

  if (strcmp (GFTP_GET_USERNAME (request), "anonymous") != 0 && 
      (request->password == NULL || *request->password == '\0'))
    {
      if ((pos = gftp_text_ask_question ("Password", 0, tempstr, 
                                         sizeof (tempstr))) == NULL)
        return (1);
      gftp_set_password (request, pos);
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

  gftp_text_log (gftp_logging_misc, NULL,
      "%s. Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>\n", version);

  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    gftp_text_log (gftp_logging_misc, NULL, "%s\n", str);
  return (1);
}


int
gftp_text_quit (gftp_request * request, char *command, gpointer *data)
{
  gftp_clear_cache_files ();
  if (configuration_changed)
    gftp_write_config_file ();
  return (0);
}


int
gftp_text_pwd (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }
  gftp_text_log (gftp_logging_misc, NULL, "%s\n", request->directory);
  return (1);
}


int
gftp_text_cd (gftp_request * request, char *command, gpointer *data)
{
  char *newdir = NULL;

  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL, _("usage: chdir <directory>\n"));
      return (1);
    }
  else if (request->protonum == GFTP_LOCAL_NUM &&
           (newdir = expand_path (command)) == NULL)
    {
      gftp_text_log (gftp_logging_error, NULL, _("usage: chdir <directory>\n"));
      return (1);
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
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL,
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
      gftp_text_log (gftp_logging_error, NULL, 
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL, _("usage: rmdir <directory>\n"));
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
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL,_("usage: delete <file>\n"));
    }
  else
    {
      gftp_remove_file (request, command);
    }
  return (1);
}


int
gftp_text_rename (gftp_request * request, char *command, gpointer *data)
{
  char *pos;

  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL,
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
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL,
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
  char *color, buf[20], *filespec;
  int sortcol, sortasds;
  gftp_file * fle;
  time_t curtime;

  time (&curtime);
  if (!GFTP_IS_CONNECTED (request))
    {
      gftp_text_log (gftp_logging_error, NULL,
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
    return (1);

  if (request == gftp_text_locreq)
    {
      sortcol = local_sortcol;
      sortasds = local_sortasds;
    }
  else
    {
      sortcol = remote_sortcol;
      sortasds = remote_sortasds;
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

      if (curtime > fle->datetime + 6 * 30 * 24 * 60 * 60 ||
          curtime < fle->datetime - 60 * 60)
        strftime (buf, sizeof (buf), "%b %d  %Y", localtime (&fle->datetime));
      else
        strftime (buf, sizeof (buf), "%b %d %H:%M", localtime (&fle->datetime));
      
#if defined (_LARGEFILE_SOURCE)
      printf ("%s %8s %8s %10lld %s %s%s%s\n", 
#else
      printf ("%s %8s %8s %10ld %s %s%s%s\n", 
#endif
              fle->attribs, fle->user, fle->group,
              fle->size, buf, color, fle->file, COLOR_DEFAULT);
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
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  gftp_set_data_type (gftp_text_remreq, GFTP_TYPE_BINARY);
  gftp_set_data_type (gftp_text_locreq, GFTP_TYPE_BINARY);
  return (1);
}


int
gftp_text_ascii (gftp_request * request, char *command, gpointer *data)
{
  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  gftp_set_data_type (gftp_text_remreq, GFTP_TYPE_ASCII);
  gftp_set_data_type (gftp_text_locreq, GFTP_TYPE_ASCII);
  return (1);
}


int
gftp_text_mget_file (gftp_request * request, char *command, gpointer *data)
{
  gftp_transfer * transfer;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (gftp_text_remreq))
    {
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL, _("usage: mget <filespec>\n"));
      return (1);
    }

  transfer = g_malloc0 (sizeof (*transfer));
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
      gftp_text_log (gftp_logging_error, NULL,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  if (*command == '\0')
    {
      gftp_text_log (gftp_logging_error, NULL, _("usage: mput <filespec>\n"));
      return (1);
    }

  transfer = g_malloc0 (sizeof (*transfer));
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
  char *tempstr, buf[8192], *progress = "|/-\\";
  struct timeval updatetime;
  long fromsize, total;
  gftp_file * curfle;
  int i, j, sw, tot;
  ssize_t num_read;

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

      if (maxkbs > 0)
       {
          gftp_text_log (gftp_logging_misc, NULL, 
                         _("File transfer will be throttled to %.2f KB/s\n"), 
                         maxkbs);
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
      while (!cancel && (num_read = gftp_get_next_file_chunk (transfer->fromreq,
                                                        buf, sizeof (buf))) > 0)
        {
          printf ("\r%c ", progress[i++]);
          fflush (stdout);
          if (progress[i] == '\0')
            i = 0;

          total += num_read;
          gftp_text_calc_kbs (transfer, num_read);
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

          if (GFTP_GET_DATA_TYPE (transfer->fromreq) == GFTP_TYPE_ASCII)
            tempstr = gftp_convert_ascii (buf, &num_read, 1);
          else
            tempstr = buf;

          if (gftp_put_next_file_chunk (transfer->toreq, tempstr, num_read) < 0)
            {
              num_read = -1;
              break;
            }

          /* We don't have to free tempstr for a download because new memory is
             not allocated for it in that case */
          if (GFTP_GET_DATA_TYPE (transfer->fromreq) == 
              GFTP_TYPE_ASCII && !transfer->transfer_direction)
            g_free (tempstr);
        }
      printf ("\n");

      if (num_read < 0)
        {
          gftp_text_log (gftp_logging_misc, NULL, 
                         _("Could not download %s\n"), curfle->file);
          gftp_disconnect (transfer->fromreq);
          gftp_disconnect (transfer->toreq);
        }
      else
        {
          gftp_text_log (gftp_logging_misc, NULL, 
                         _("Successfully transferred %s\n"), curfle->file);
          gftp_end_transfer (transfer->fromreq);
          gftp_end_transfer (transfer->toreq);
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
  char *pos, *backpos;
  int i;

  if (command == NULL || *command == '\0')
    {
      for (i=0; config_file_vars[i].key != NULL; i++)
        {
          if (!(config_file_vars[i].ports_shown & GFTP_PORT_TEXT))
            continue;

          switch (config_file_vars[i].type)
            {
              case CONFIG_CHARTEXT:
                printf ("%s = %s\n", config_file_vars[i].key, 
                        *(char **) config_file_vars[i].var);
                break;
              case CONFIG_INTTEXT:
              case CONFIG_CHECKBOX:
                printf ("%s = %d\n", config_file_vars[i].key, 
                        *(int *) config_file_vars[i].var);
                break;
              case CONFIG_FLOATTEXT:
                printf ("%s = %.2f\n", config_file_vars[i].key, 
                        *(float *) config_file_vars[i].var);
                break;
            }
        }
    }
  else
    {
      if ((pos = strchr (command, '=')) == NULL)
        {
          gftp_text_log (gftp_logging_error, NULL,
                         _("usage: set [variable = value]\n"));
          return (1);
        }
      *pos = '\0';

      for (backpos = pos - 1; 
           (*backpos == ' ' || *backpos == '\t') && backpos > command; 
           backpos--)
        *backpos = '\0';
      for (++pos; *pos == ' ' || *pos == '\t'; pos++);

      for (i=0; config_file_vars[i].key != NULL; i++)
        {
          if (strcmp (config_file_vars[i].key, command) == 0)
            break;
           
        }

      if (config_file_vars[i].key == NULL)
        {
          gftp_text_log (gftp_logging_error, NULL,
                         _("Error: Variable %s is not a valid configuration variable.\n"), command);
          return (1);
        }

      if (!(config_file_vars[i].ports_shown & GFTP_PORT_TEXT))
        {
          gftp_text_log (gftp_logging_error, NULL,
                         _("Error: Variable %s is not available in the text port of gFTP\n"), command);
          return (1);
        }

      configuration_changed = 1;
      switch (config_file_vars[i].type)
        {
          case CONFIG_CHARTEXT:
            if (*(char **) config_file_vars[i].var != NULL)
              g_free (*(char **) config_file_vars[i].var);
            *(char **) config_file_vars[i].var = g_strconcat (pos, NULL);
            break;
          case CONFIG_CHECKBOX:
            *(int *) config_file_vars[i].var = *pos == '1' ? 1 : 0;
            break;
          case CONFIG_INTTEXT:
            *(int *) config_file_vars[i].var = strtol (pos, NULL, 10);
            break;
          case CONFIG_FLOATTEXT:
            *(float *) config_file_vars[i].var = strtod (pos, NULL);
            break;
          default:
            gftp_text_log (gftp_logging_error, NULL,
                           _("Error: You cannot change this variable\n"));
            break;
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
    gftp_text_log (gftp_logging_error, NULL, "Invalid argument\n");
  return (1);
}


char *
gftp_text_ask_question (const char *question, int echo, char *buf, size_t size)
{
  struct termios term, oldterm;
  sigset_t sig, sigsave;
  char *pos, *termname;
  FILE *infd, *outfd;

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
                         "Cannot open controlling terminal %s\n", termname);
          return (NULL);
        }
      outfd = infd;

      tcgetattr (0, &term);
      oldterm = term;
      term.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL); 
      tcsetattr (fileno (infd), TCSAFLUSH, &term);
    }
  else
    {
      infd = stdin;
      outfd = stdout;
    }

  fprintf (outfd, "%s%s%s: ", COLOR_BLUE, question, COLOR_DEFAULT);

  if (fgets (buf, size, infd) == NULL)
    return (NULL);
  buf[size - 1] = '\0';

  if (!echo)
    {
      fprintf (outfd, "\n");
      tcsetattr (fileno (infd), TCSAFLUSH, &oldterm);
      fclose (outfd);
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
gftp_text_calc_kbs (gftp_transfer * tdata, ssize_t num_read)
{
  unsigned long waitusecs;
  double difftime, curkbs;
  gftp_file * tempfle;
  struct timeval tv;
  unsigned long toadd;

  gettimeofday (&tv, NULL);

  tempfle = tdata->curfle->data;
  tdata->trans_bytes += num_read;
  tdata->curtrans += num_read;
  tdata->stalled = 0;

  difftime = (tv.tv_sec - tdata->starttime.tv_sec) + ((double) (tv.tv_usec - tdata->starttime.tv_usec) / 1000000.0);
  if (difftime == 0)
    tdata->kbs = (double) tdata->trans_bytes / 1024.0;
  else
    tdata->kbs = (double) tdata->trans_bytes / 1024.0 / difftime;

  difftime = (tv.tv_sec - tdata->lasttime.tv_sec) + ((double) (tv.tv_usec - tdata->lasttime.tv_usec) / 1000000.0);

  if (difftime <= 0)
    curkbs = (double) (num_read / 1024.0);
  else
    curkbs = (double) (num_read / 1024.0 / difftime);

  if (tdata->fromreq->maxkbs > 0 && 
      curkbs > tdata->fromreq->maxkbs)
    {
      waitusecs = (double) num_read / 1024.0 / tdata->fromreq->maxkbs * 1000000.0 - difftime;

      if (waitusecs > 0)
        {
          difftime += ((double) waitusecs / 1000000.0);
          usleep (waitusecs);
        }
    }

  /* I don't call gettimeofday (&tdata->lasttime) here because this will use
     less system resources. This will be close enough for what we need */
  difftime += tdata->lasttime.tv_usec / 1000000.0;
  toadd = (long) difftime;
  difftime -= toadd;
  tdata->lasttime.tv_sec += toadd;
  tdata->lasttime.tv_usec = difftime * 1000000.0;
}


void
sig_child (int signo)
{
}


int
gftp_text_set_show_subhelp (char *topic)
{
  int i;

  for (i=0; config_file_vars[i].key != NULL; i++)
    {
      if (strcmp (topic, config_file_vars[i].key) == 0)
        {
          printf ("%s\n", config_file_vars[i].comment);
          return (1);
        }
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


#if !defined (HAVE_GETADDRINFO) || !defined (HAVE_GAI_STRERROR)

struct hostent *
r_gethostbyname (const char *name, struct hostent *result_buf, int *h_errnop)
{
  struct hostent *hent;

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
  return (hent);
}

#endif /* HAVE_GETADDRINFO */


struct servent *
r_getservbyname (const char *name, const char *proto,
                 struct servent *result_buf, int *h_errnop)
{
  struct servent *sent;

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
  return (sent);
}

