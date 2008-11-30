/*****************************************************************************/
/*  gftpui.c - UI related functions for gFTP                                 */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
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

#include "gftpui.h"
static const char cvsid[] = "$Id$";

GStaticMutex gftpui_common_transfer_mutex = G_STATIC_MUTEX_INIT;
volatile sig_atomic_t gftpui_common_child_process_done = 0;
volatile sig_atomic_t gftpui_common_num_child_threads = 0;
static gftp_logging_func gftpui_common_logfunc;

static void *
_gftpui_common_thread_callback (void * data)
{ 
  intptr_t network_timeout, sleep_time;
  gftpui_callback_data * cdata;
  struct timespec ts;
  int success;

  cdata = data;
  gftpui_common_num_child_threads++;

  gftp_lookup_request_option (cdata->request, "network_timeout",
                              &network_timeout);
  gftp_lookup_request_option (cdata->request, "sleep_time",
                              &sleep_time);

  success = GFTP_ERETRYABLE;
  while (1)
    {
      if (network_timeout > 0)
        alarm (network_timeout);

      success = cdata->run_function (cdata);
      alarm (0);

      if (cdata->request->cancel)
        {
          cdata->request->logging_function (gftp_logging_error, cdata->request,
                                            _("Operation canceled\n"));
          break;
        }
        
      if (success == GFTP_EFATAL || success == 0 || cdata->retries == 0)
        break;

      cdata->retries--;
      cdata->request->logging_function (gftp_logging_error, cdata->request,
                   _("Waiting %d seconds until trying to connect again\n"),
                   sleep_time);

      ts.tv_sec = sleep_time;
      ts.tv_nsec = 0;
      nanosleep (&ts, NULL);
    }

  cdata->request->stopable = 0;
  gftpui_common_num_child_threads--;

  return (GINT_TO_POINTER (success));
}


int
gftpui_common_run_callback_function (gftpui_callback_data * cdata)
{
  int ret;

  if (!cdata->dont_check_connection && gftpui_check_reconnect (cdata) < 0)
    return (0);

  if (gftp_protocols[cdata->request->protonum].use_threads)
    ret = GPOINTER_TO_INT (gftpui_generic_thread (_gftpui_common_thread_callback, cdata));
  else
    ret = GPOINTER_TO_INT (cdata->run_function (cdata));

  if (ret == 0 && !cdata->dont_refresh)
    gftpui_refresh (cdata->uidata, !cdata->dont_clear_cache);

  return (ret == 0);
}


static RETSIGTYPE
gftpui_common_signal_handler (int signo)
{
  signal (signo, gftpui_common_signal_handler);

  if (!gftpui_common_num_child_threads && signo == SIGINT)
    exit (EXIT_FAILURE);
}


static RETSIGTYPE
gftpui_common_sig_child (int signo)
{
  int ret;

  if (gftpui_common_child_process_done == -1)
    {
      /* Running from text port */
      while (waitpid (-1, &ret, WNOHANG) > 0)
        {
          /* Nothing */
        }
    }
  else
    gftpui_common_child_process_done = 1;
}


void
gftpui_common_init (int *argc, char ***argv, gftp_logging_func logfunc)
{
  char *share_dir;

  gftp_locale_init ();

  signal (SIGCHLD, gftpui_common_sig_child);
  signal (SIGPIPE, SIG_IGN);
  signal (SIGALRM, gftpui_common_signal_handler);
  signal (SIGINT, gftpui_common_signal_handler);

  share_dir = gftp_get_share_dir ();
  gftp_read_config_file (share_dir);
  if (gftp_parse_command_line (argc, argv) != 0)
    exit (EXIT_FAILURE);

  gftpui_common_logfunc = logfunc;
  gftpui_common_child_process_done = -1;
}


void
gftpui_common_about (gftp_logging_func logging_function, gpointer logdata)
{
  char *str;

  logging_function (gftp_logging_misc, logdata, "%s, Copyright (C) 1998-2007 Brian Masney <", gftp_version);
  logging_function (gftp_logging_recv, logdata, "masneyb@gftp.org");
  logging_function (gftp_logging_misc, logdata, _(">. If you have any questions, comments, or suggestions about this program, please feel free to email them to me. You can always find out the latest news about gFTP from my website at http://www.gftp.org/\n"));
  logging_function (gftp_logging_misc, logdata, _("gFTP comes with ABSOLUTELY NO WARRANTY; for details, see the COPYING file. This is free software, and you are welcome to redistribute it under certain conditions; for details, see the COPYING file\n"));

  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    logging_function (gftp_logging_misc, logdata, "%s\n", str);
}


static int
gftpui_common_cmd_about (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftpui_common_about (gftpui_common_logfunc, NULL);
  return (1);
}


static int
gftpui_common_cmd_ascii (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(1));
  return (1);
}


static int
gftpui_common_cmd_binary (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(0));
  return (1);
}


static int
gftpui_common_cmd_chmod (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftpui_callback_data * cdata;
  char *pos;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));

      return (1);
    }

  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';

  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: chmod <mode> <file>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = (char *) command;
      cdata->source_string = pos;
      cdata->run_function = gftpui_common_run_chmod;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_rename (void *uidata, gftp_request * request,
                          void *other_uidata, gftp_request * other_request,
                          const char *command)
{
  gftpui_callback_data * cdata;
  char *pos;
  
  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
    
  if ((pos = strchr (command, ' ')) != NULL)
    *pos++ = '\0';
    
  if (*command == '\0' || pos == NULL || *pos == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: rename <old name> <new name>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->source_string = (char *) command;
      cdata->input_string = pos;
      cdata->run_function = gftpui_common_run_rename;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_delete (void *uidata, gftp_request * request,
                          void *other_uidata, gftp_request * other_request,
                          const char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: delete <file>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = (char *) command;
      cdata->run_function = gftpui_common_run_delete;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_rmdir (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: rmdir <directory>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = (char *) command;
      cdata->run_function = gftpui_common_run_rmdir;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_site (void *uidata, gftp_request * request,
                        void *other_uidata, gftp_request * other_request,
                        const char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                     _("usage: site <site command>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = (char *) command;
      cdata->run_function = gftpui_common_run_site;
      cdata->toggled = 1;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_mkdir (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftpui_callback_data * cdata;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                     _("usage: mkdir <new directory>\n"));
    }
  else
    {
      cdata = g_malloc0 (sizeof (*cdata));
      cdata->request = request;
      cdata->uidata = uidata;
      cdata->input_string = (char *) command;
      cdata->run_function = gftpui_common_run_mkdir;

      gftpui_common_run_callback_function (cdata);

      g_free (cdata);
    }

  return (1);
}


static int
gftpui_common_cmd_chdir (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftpui_callback_data * cdata;
  char *tempstr, *newdir = NULL;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }
  else if (*command == '\0')
    {
      request->logging_function (gftp_logging_error, request,
                                 _("usage: chdir <directory>\n"));
      return (1);
    }
  else if (request->protonum == GFTP_LOCAL_NUM)
    {
      if (*command != '/' && request->directory != NULL)
        {
          tempstr = gftp_build_path (request, request->directory, command,
                                     NULL);
          newdir = gftp_expand_path (request, tempstr);
          g_free (tempstr);
        }
      else
        newdir = gftp_expand_path (request, command);

      if (newdir == NULL)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("usage: chdir <directory>\n"));
          return (1);
        }
    }

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->input_string = newdir != NULL ? newdir : (char *) command;
  cdata->run_function = gftpui_common_run_chdir;
  cdata->dont_clear_cache = 1;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);

  if (newdir != NULL)
    g_free (newdir);

  return (1);
}


static int
gftpui_common_cmd_close (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  gftp_disconnect (request);
  return (1);
}


static int
gftpui_common_cmd_pwd (void *uidata, gftp_request * request,
                       void *other_uidata, gftp_request * other_request,
                       const char *command)
{
  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Not connected to a remote site\n"));
      return (1);
    }

  request->logging_function (gftp_logging_misc, request,
                             "%s\n", request->directory);

  return (1);
}


static int
gftpui_common_cmd_quit (void *uidata, gftp_request * request,
                        void *other_uidata, gftp_request * other_request,
                        const char *command)
{
  gftp_shutdown();

  return (0);
}


static int
gftpui_common_cmd_clear (void *uidata, gftp_request * request,
                         void *other_uidata, gftp_request * other_request,
                         const char *command)
{
  if (strcasecmp (command, "cache") == 0)
    gftp_clear_cache_files ();
  else
    {
      gftpui_common_logfunc (gftp_logging_error, request,
                             _("Invalid argument\n"));
    }

  return (1);
}


static int
gftpui_common_clear_show_subhelp (const char *topic)
{
  if (strcmp (topic, "cache") == 0)
    {
      gftpui_common_logfunc (gftp_logging_misc, NULL,
                             _("Clear the directory cache\n"));
      return (1);
    }

  return (0);
}


static int
gftpui_common_set_show_subhelp (const char *topic)
{ 
  gftp_config_vars * cv;
    
  if ((cv = g_hash_table_lookup (gftp_global_options_htable, topic)) != NULL)
    {
      gftpui_common_logfunc (gftp_logging_misc, NULL, "%s\n", cv->comment);
      return (1);
    }

  return (0);
}   


static int
gftpui_common_cmd_ls (void *uidata, gftp_request * request,
                      void *other_uidata, gftp_request * other_request,
                      const char *command)
{
  char *startcolor, *endcolor, *tempstr;
  gftpui_callback_data * cdata;
  GList * templist;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (request))
    {
      request->logging_function (gftp_logging_error, request,
                     _("Error: Not connected to a remote site\n"));
      return (1);
    }

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->source_string = *command != '\0' ? (char *) command : NULL;
  cdata->run_function = gftpui_common_run_ls;
  cdata->dont_refresh = 1;

  gftpui_common_run_callback_function (cdata);

  templist = cdata->files;
  while (templist != NULL)
    {
      fle = templist->data;

      gftpui_lookup_file_colors (fle, &startcolor, &endcolor);
      tempstr = gftp_gen_ls_string (request, fle, startcolor, endcolor);
      request->logging_function (gftp_logging_misc_nolog, request, "%s\n",
                                 tempstr);
      g_free (tempstr);

      templist = templist->next;
      gftp_file_destroy (fle, 1);
    }


  if (cdata->files != NULL)
    g_list_free (cdata->files);
  g_free (cdata);

  return (1);
}


int
gftpui_common_cmd_open (void *uidata, gftp_request * request,
                        void *other_uidata, gftp_request * other_request,
                        const char *command)
{
  gftpui_callback_data * cdata;
  intptr_t retries;

  if (GFTP_IS_CONNECTED (request))
    gftpui_disconnect (uidata);
  
  if (command != NULL)
    {
      if (*command == '\0')
        {
          request->logging_function (gftp_logging_error, request,
                                     _("usage: open " GFTP_URL_USAGE "\n"));
          return (1);
        }
    
      if (gftp_parse_url (request, command) < 0)
        return (1);
    }

  if (gftp_need_username (request))
    gftpui_prompt_username (uidata, request);

  if (gftp_need_password (request))
    gftpui_prompt_password (uidata, request);

  gftp_lookup_request_option (request, "retries", &retries);

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = request;
  cdata->uidata = uidata;
  cdata->run_function = gftpui_common_run_connect;
  cdata->retries = retries;
  cdata->dont_check_connection = 1;

  if (request->refreshing)
    cdata->dont_refresh = 1;

  gftpui_show_busy (TRUE);
  gftpui_common_run_callback_function (cdata);
  gftpui_show_busy (FALSE);

  g_free (cdata);

  return (1);
}


static int
gftpui_common_cmd_set (void *uidata, gftp_request * request,
                       void *other_uidata, gftp_request * other_request,
                       const char *command)
{
  char *pos, *backpos, buf[256];
  gftp_config_vars * cv, newcv;
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

              gftp_option_types[cv[i].otype].write_function (&cv[i], buf,
                                                             sizeof (buf), 0);

              gftpui_common_logfunc (gftp_logging_misc_nolog, request,
                                     "%s = %s\n", cv[i].key, buf);
            }
        }
    }
  else
    {
      if ((pos = strchr (command, '=')) == NULL)
        {
          gftpui_common_logfunc (gftp_logging_error, request,
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
          gftpui_common_logfunc (gftp_logging_error, request,
                                 _("Error: Variable %s is not a valid configuration variable.\n"), command);
          return (1);
        }

      if (!(cv->ports_shown & GFTP_PORT_TEXT))
        {
          gftpui_common_logfunc (gftp_logging_error, request,
                                 _("Error: Variable %s is not available in the text port of gFTP\n"), command);
          return (1);
        }

      if (gftp_option_types[cv->otype].read_function != NULL)
        {
          memcpy (&newcv, cv, sizeof (newcv));
          newcv.flags &= ~GFTP_CVARS_FLAGS_DYNMEM;

          gftp_option_types[cv->otype].read_function (pos, &newcv, 1);

          gftp_set_global_option (command, newcv.value);

          gftp_option_types[newcv.otype].write_function (&newcv, buf,
                                                         sizeof (buf), 0);

          gftpui_common_logfunc (gftp_logging_misc_nolog, request,
                                 "%s = %s\n", newcv.key, buf);

          if (newcv.flags & GFTP_CVARS_FLAGS_DYNMEM)
            g_free (newcv.value);

          gftp_configuration_changed = 1;
        }
    }

  return (1);
}


static int
gftpui_common_cmd_help (void *uidata, gftp_request * request,
                        void *other_uidata, gftp_request * other_request,
                        const char *command)
{
  int i, j, ele, numrows, numcols = 6, handled, number_commands, cmdlen,
      found;
  char commands[128], cmdstr[30];
  const char *pos;

  for (number_commands=0;
       gftpui_common_commands[number_commands].command != NULL;
       number_commands++);

  if (command != NULL && *command != '\0')
    {
      for (pos = command; *pos != ' ' && *pos != '\0'; pos++);
      cmdlen = pos - command;

      for (i=0; gftpui_common_commands[i].command != NULL; i++)
        {
          if (strncmp (gftpui_common_commands[i].command, command, cmdlen) == 0)
            break;
        }

      if (gftpui_common_commands[i].cmd_description != NULL)
        {
          found = 1;

          if (*pos != '\0' && *(pos + 1) != '\0' &&
              gftpui_common_commands[i].subhelp_func != NULL)
            handled = gftpui_common_commands[i].subhelp_func (pos + 1);
          else
            handled = 0;

          if (!handled)
            gftpui_common_logfunc (gftp_logging_misc_nolog, request, "%s\n",
                                 _(gftpui_common_commands[i].cmd_description));
        }
      else
        found = 0;
    }
  else
    found = 0;
  
  if (!found)
    {
      numrows = number_commands / numcols;
      if (number_commands % numcols != 0)
        numrows++;

      gftpui_common_logfunc (gftp_logging_misc_nolog, request,
                             _("Supported commands:\n\n"));
      
      for (i=0; i<numrows; i++)
        {
          strncpy (commands, "\t", sizeof (commands));
          size_t cmd_len = sizeof(commands) - strlen (commands);

          for (j=0; j<numcols; j++)
            {
              ele = i + j * numrows;
              if (ele >= number_commands)
                break;

              g_snprintf (cmdstr, sizeof (cmdstr), "%-10s",
                          gftpui_common_commands[ele].command);
              strncat (commands, cmdstr, cmd_len);
              cmd_len -= strlen(cmdstr);
            }
          gftpui_common_logfunc (gftp_logging_misc_nolog, request, "%s\n",
                                 commands);
        }
    }
  return (1);
}


static void
_gftpui_common_cmd_transfer_files (void *fromuidata, gftp_request * fromrequest,
                                   void *touidata, gftp_request * torequest,
                                   const char *cmd, const char *filespec)
{
  gftp_transfer * tdata;
  gftp_file * fle;

  if (!GFTP_IS_CONNECTED (fromrequest) ||
      !GFTP_IS_CONNECTED (torequest))
    {
      fromrequest->logging_function (gftp_logging_error, fromrequest,
                                  _("Error: Not connected to a remote site\n"));
      return;
    }

  if (*filespec == '\0')
    {
      fromrequest->logging_function (gftp_logging_error, fromrequest, 
                                     _("usage: %s <filespec>\n"), cmd);
      return;
    }

  tdata = gftp_tdata_new ();
  tdata->fromreq = fromrequest;
  tdata->toreq = torequest;

  if (gftp_list_files (tdata->fromreq) != 0)
    {
      tdata->fromreq = tdata->toreq = NULL;
      free_tdata (tdata);
      return;
    }

  fle = g_malloc0 (sizeof (*fle));
  while (gftp_get_next_file (tdata->fromreq, filespec, fle) > 0)
    {
      if (strcmp (fle->file, ".") == 0 || strcmp (fle->file, "..") == 0)
        {
          gftp_file_destroy (fle, 0);
          continue;
        }

      tdata->files = g_list_append (tdata->files, fle);
      fle = g_malloc0 (sizeof (*fle));
    }

  g_free (fle);

  gftp_end_transfer (tdata->fromreq);

  if (tdata->files == NULL)
    {
      tdata->fromreq = tdata->toreq = NULL;
      free_tdata (tdata);
      return;
    }

  if (gftp_get_all_subdirs (tdata, NULL) != 0)
    {
      tdata->fromreq = tdata->toreq = NULL;
      free_tdata (tdata);
      return;
    }

  if (tdata->files == NULL)
    {
      tdata->fromreq = tdata->toreq = NULL;
      free_tdata (tdata);
      return;
    }

  gftpui_common_add_file_transfer (tdata->fromreq, tdata->toreq,
                                   fromuidata, touidata, tdata->files);

  g_free (tdata);

  return;
}


int
gftpui_common_cmd_mget_file (void *uidata, gftp_request * request,
                             void *other_uidata, gftp_request * other_request,
                             const char *command)
{
  _gftpui_common_cmd_transfer_files (uidata, request, other_uidata,
                                     other_request, "mget", command);
  return (1);
}


int
gftpui_common_cmd_mput_file (void *uidata, gftp_request * request,
                             void *other_uidata, gftp_request * other_request,
                             const char *command)
{
  _gftpui_common_cmd_transfer_files (other_uidata, other_request, uidata,
                                     request, "mput", command);
  return (1);
}


gftpui_common_methods gftpui_common_commands[] = {
        {N_("about"),   2, gftpui_common_cmd_about, gftpui_common_request_none,
         N_("Shows gFTP information"), NULL},
        {N_("ascii"),   2, gftpui_common_cmd_ascii, gftpui_common_request_remote,
         N_("Sets the current file transfer mode to Ascii (only for FTP)"), NULL},
        {N_("binary"),  1, gftpui_common_cmd_binary, gftpui_common_request_remote,
         N_("Sets the current file transfer mode to Binary (only for FTP)"), NULL},
        {N_("cd"),      2, gftpui_common_cmd_chdir, gftpui_common_request_remote,
         N_("Changes the remote working directory"), NULL},
        {N_("chdir"),   3, gftpui_common_cmd_chdir, gftpui_common_request_remote,
         N_("Changes the remote working directory"), NULL},
        {N_("chmod"),   3, gftpui_common_cmd_chmod, gftpui_common_request_remote,
         N_("Changes the permissions of a remote file"), NULL},
        {N_("clear"),   3, gftpui_common_cmd_clear, gftpui_common_request_none,
         N_("Available options: cache"), gftpui_common_clear_show_subhelp},
        {N_("close"),   3, gftpui_common_cmd_close, gftpui_common_request_remote,
         N_("Disconnects from the remote site"), NULL},
        {N_("delete"),  1, gftpui_common_cmd_delete, gftpui_common_request_remote,
         N_("Removes a remote file"), NULL},
        {N_("dir"),     3, gftpui_common_cmd_ls, gftpui_common_request_remote,
         N_("Shows the directory listing for the current remote directory"), NULL},
        {N_("get"),     1, gftpui_common_cmd_mget_file, gftpui_common_request_remote,
         N_("Downloads remote file(s)"), NULL},
        {N_("help"),    1, gftpui_common_cmd_help, gftpui_common_request_none,
         N_("Shows this help screen"), NULL},
        {N_("lcd"),     3, gftpui_common_cmd_chdir, gftpui_common_request_local,
         N_("Changes the local working directory"), NULL},
        {N_("lchdir"),  4, gftpui_common_cmd_chdir, gftpui_common_request_local,
         N_("Changes the local working directory"), NULL},
        {N_("lchmod"),  4, gftpui_common_cmd_chmod,     gftpui_common_request_local,
         N_("Changes the permissions of a local file"), NULL},
        {N_("ldelete"), 2, gftpui_common_cmd_delete,    gftpui_common_request_local,
         N_("Removes a local file"), NULL},
        {N_("ldir"),    4, gftpui_common_cmd_ls, gftpui_common_request_local,
         N_("Shows the directory listing for the current local directory"), NULL},
        {N_("lls"),     2, gftpui_common_cmd_ls, gftpui_common_request_local,
         N_("Shows the directory listing for the current local directory"), NULL},
        {N_("lmkdir"),  2, gftpui_common_cmd_mkdir, gftpui_common_request_local,
         N_("Creates a local directory"), NULL},
        {N_("lpwd"),    2, gftpui_common_cmd_pwd, gftpui_common_request_local,
         N_("Show current local directory"), NULL},
        {N_("lrename"), 3, gftpui_common_cmd_rename, gftpui_common_request_local,
         N_("Rename a local file"), NULL},
        {N_("lrmdir"),  3, gftpui_common_cmd_rmdir, gftpui_common_request_local,
         N_("Remove a local directory"), NULL},
        {N_("ls"),      2, gftpui_common_cmd_ls, gftpui_common_request_remote,
         N_("Shows the directory listing for the current remote directory"), NULL},
        {N_("mget"),    2, gftpui_common_cmd_mget_file, gftpui_common_request_remote,
         N_("Downloads remote file(s)"), NULL},
        {N_("mkdir"),   2, gftpui_common_cmd_mkdir, gftpui_common_request_remote,
         N_("Creates a remote directory"), NULL},
        {N_("mput"),    2, gftpui_common_cmd_mput_file, gftpui_common_request_remote,
         N_("Uploads local file(s)"), NULL},
        {N_("open"),    1, gftpui_common_cmd_open, gftpui_common_request_remote,
         N_("Opens a connection to a remote site"), NULL},
        {N_("put"),     2, gftpui_common_cmd_mput_file, gftpui_common_request_remote,
         N_("Uploads local file(s)"), NULL},
        {N_("pwd"),     2, gftpui_common_cmd_pwd, gftpui_common_request_remote,
         N_("Show current remote directory"), NULL},
        {N_("quit"),    1, gftpui_common_cmd_quit, gftpui_common_request_none,
         N_("Exit from gFTP"), NULL},
        {N_("rename"),  2, gftpui_common_cmd_rename, gftpui_common_request_remote,
         N_("Rename a remote file"), NULL},
        {N_("rmdir"),   2, gftpui_common_cmd_rmdir, gftpui_common_request_remote,
         N_("Remove a remote directory"), NULL},
        {N_("set"),     1, gftpui_common_cmd_set, gftpui_common_request_none,
         N_("Show configuration file variables. You can also set variables by set var=val"),
         gftpui_common_set_show_subhelp},
        {N_("site"),    2, gftpui_common_cmd_site, gftpui_common_request_remote,
         N_("Run a site specific command"), NULL},
        {NULL,          0, NULL,                gftpui_common_request_none,
	 NULL, NULL}};


int
gftpui_common_process_command (void *locui, gftp_request * locreq,
                               void *remui, gftp_request * remreq,
                               const char *command)
{
  gftp_request * request, * other_request;
  void *uidata, *other_uidata;
  char *pos, *newstr;
  const char *stpos;
  size_t cmdlen;
  int ret, i;
  size_t len;

  for (stpos = command; *stpos == ' ' || *stpos == '\t'; stpos++);

  newstr = g_strdup (stpos);
  len = strlen (newstr);

  if (len > 0 && newstr[len - 1] == '\n')
    newstr[--len] = '\0';
  if (len > 0 && newstr[len - 1] == '\r')
    newstr[--len] = '\0';

  for (pos = newstr + len - 1;
       (*pos == ' ' || *pos == '\t') && pos > newstr; 
       *pos-- = '\0');

  if (*newstr == '\0')
    {
      g_free (newstr);
      return (1);
    }

  if ((pos = strchr (newstr, ' ')) != NULL)
    *pos = '\0'; 

  cmdlen = strlen (newstr);
  for (i=0; gftpui_common_commands[i].command != NULL; i++)
    {
      if (strcmp (gftpui_common_commands[i].command, newstr) == 0)
        break;
      else if (cmdlen >= gftpui_common_commands[i].minlen &&
               strncmp (gftpui_common_commands[i].command, newstr, cmdlen) == 0)
        break; 
    }

  if (pos != NULL)
    pos++;
  else
    pos = "";
  
  if (gftpui_common_commands[i].reqtype == gftpui_common_request_local)
    {
      request = locreq;
      uidata = locui;

      other_request = remreq;
      other_uidata = remui;
    }
  else if (gftpui_common_commands[i].reqtype == gftpui_common_request_remote)
    {
      request = remreq;
      uidata = remui;

      other_request = locreq;
      other_uidata = locui;
    }
  else
    {
      request = other_request = NULL;
      uidata = other_uidata = NULL;
    }
 
  if (gftpui_common_commands[i].command != NULL)
    {
      ret = gftpui_common_commands[i].func (uidata, request,
                                            other_uidata, other_request, pos);

      if (request != NULL && !GFTP_IS_CONNECTED (request))
        gftpui_disconnect (uidata);
    }
  else
    {
      gftpui_common_logfunc (gftp_logging_error, request,
                             _("Error: Command not recognized\n"));
      ret = 1;
    }

  g_free (newstr);
  return (ret);
}


gftp_transfer *
gftpui_common_add_file_transfer (gftp_request * fromreq, gftp_request * toreq,
                                 void *fromuidata, void *touidata,
                                 GList * files)
{
  intptr_t append_transfers, one_transfer, overwrite_default;
  GList * templist, *curfle;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int show_dialog;
  
  gftp_lookup_request_option (fromreq, "overwrite_default", &overwrite_default);
  gftp_lookup_request_option (fromreq, "append_transfers", &append_transfers);
  gftp_lookup_request_option (fromreq, "one_transfer", &one_transfer);

  if (!overwrite_default)
    {
      for (templist = files; templist != NULL; templist = templist->next)
        { 
          tempfle = templist->data;
          if (tempfle->startsize > 0 && !S_ISDIR (tempfle->st_mode))
            break;
        }

      show_dialog = templist != NULL;
    }
  else
    show_dialog = 0;

  tdata = NULL;
  if (append_transfers && one_transfer && !show_dialog)
    {
      if (g_thread_supported ())
        g_static_mutex_lock (&gftpui_common_transfer_mutex);

      for (templist = gftp_file_transfers;
           templist != NULL;
           templist = templist->next)
        {
          tdata = templist->data;

          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          if (!compare_request (tdata->fromreq, fromreq, 0) ||
              !compare_request (tdata->toreq, toreq, 0) ||
              tdata->curfle == NULL)
            {
              if (g_thread_supported ())
                g_static_mutex_unlock (&tdata->structmutex);

              continue;
            }

          tdata->files = g_list_concat (tdata->files, files);

          for (curfle = files; curfle != NULL; curfle = curfle->next)
            {
              tempfle = curfle->data;

              if (S_ISDIR (tempfle->st_mode))
                tdata->numdirs++;
              else
                tdata->numfiles++;

              if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
                tdata->total_bytes += tempfle->size;

              gftpui_add_file_to_transfer (tdata, curfle);
            }

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);

          break;
        }

      if (g_thread_supported ())
        g_static_mutex_unlock (&gftpui_common_transfer_mutex);
    }
  else
    templist = NULL;

  if (templist == NULL)
    {
      tdata = gftp_tdata_new ();
      tdata->fromreq = gftp_copy_request (fromreq);
      tdata->toreq = gftp_copy_request (toreq);

      tdata->fromwdata = fromuidata;
      tdata->towdata = touidata;

      if (!show_dialog)
        tdata->show = tdata->ready = 1;

      tdata->files = files;
      for (curfle = files; curfle != NULL; curfle = curfle->next)
        {
          tempfle = curfle->data;
          if (S_ISDIR (tempfle->st_mode))
            tdata->numdirs++;
          else
            tdata->numfiles++;

          if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
            tdata->total_bytes += tempfle->size;
        }

      if (g_thread_supported ())
        g_static_mutex_lock (&gftpui_common_transfer_mutex);

      gftp_file_transfers = g_list_append (gftp_file_transfers, tdata);

      if (g_thread_supported ())
        g_static_mutex_unlock (&gftpui_common_transfer_mutex);

      if (show_dialog)
        gftpui_ask_transfer (tdata);
    }

  gftpui_start_transfer (tdata);
  return (tdata);
}


static ssize_t
_do_transfer_block (gftp_transfer * tdata, gftp_file * curfle, char *buf,
                    size_t trans_blksize)
{
  ssize_t num_read, num_wrote, ret;
  char *bufpos;

  num_read = gftp_get_next_file_chunk (tdata->fromreq, buf, trans_blksize);
  if (num_read < 0)
    return (num_read);

  bufpos = buf;
  num_wrote = 0;
  while (num_wrote < num_read)
    {
      if ((ret = gftp_put_next_file_chunk (tdata->toreq, bufpos,
                                           num_read - num_wrote)) <= 0)
        return (ret);

      num_wrote += ret;
      bufpos += ret;
    }

  return (num_read);
}


int
_gftpui_common_do_transfer_file (gftp_transfer * tdata, gftp_file * curfle)
{
  struct timeval updatetime;
  intptr_t trans_blksize;
  ssize_t num_trans;
  char *buf;
  int ret;

  gftp_lookup_request_option (tdata->fromreq, "trans_blksize", &trans_blksize);
  buf = g_malloc0 (trans_blksize);

  memset (&updatetime, 0, sizeof (updatetime));
  gftpui_start_current_file_in_transfer (tdata);

  num_trans = 0;
  while (!tdata->cancel &&
         (num_trans = _do_transfer_block (tdata, curfle, buf,
                                          trans_blksize)) > 0)
    {
      gftp_calc_kbs (tdata, num_trans);

      if (tdata->lasttime.tv_sec - updatetime.tv_sec >= 1 ||
          tdata->curtrans >= tdata->tot_file_trans)
        {
          gftpui_update_current_file_in_transfer (tdata);
          memcpy (&updatetime, &tdata->lasttime, sizeof (updatetime));

          if (tdata->current_file_retries > 0)
            tdata->current_file_retries = 0;
        }
    }

  if (num_trans == GFTP_ENOTRANS)
    num_trans = 0;

  g_free (buf);
  gftpui_finish_current_file_in_transfer (tdata);

  if ((int) num_trans == 0)
    {
      if ((ret = gftp_end_transfer (tdata->fromreq)) < 0)
        return (ret);

      if ((ret = gftp_end_transfer (tdata->toreq)) < 0)
        return (ret);

      tdata->fromreq->logging_function (gftp_logging_misc,
                     tdata->fromreq,
                     _("Successfully transferred %s at %.2f KB/s\n"),
                     curfle->file, tdata->kbs);

      return (0);
    }
  else
    return ((int) num_trans);
}


void
gftpui_common_skip_file_transfer (gftp_transfer * tdata, gftp_file * curfle)
{
  g_static_mutex_lock (&tdata->structmutex);

  if (tdata->started && !(curfle->transfer_action & GFTP_TRANS_ACTION_SKIP))
    {
      curfle->transfer_action = GFTP_TRANS_ACTION_SKIP;
      if (tdata->curfle != NULL && curfle == tdata->curfle->data)
        {
          gftpui_cancel_file_transfer (tdata);
          tdata->skip_file = 1;
        }
      else if (!curfle->transfer_done)
        tdata->total_bytes -= curfle->size;
    }

  g_static_mutex_unlock (&tdata->structmutex);

  if (curfle != NULL)
    tdata->fromreq->logging_function (gftp_logging_misc, tdata->fromreq,
                                      _("Skipping file %s on host %s\n"),
                                      curfle->file, tdata->toreq->hostname);
}


void
gftpui_common_cancel_file_transfer (gftp_transfer * tdata)
{
  g_static_mutex_lock (&tdata->structmutex);

  if (tdata->started)
    {
      gftpui_cancel_file_transfer (tdata);
      tdata->skip_file = 0;
    }
  else
    tdata->done = 1;

  tdata->fromreq->stopable = 0;
  tdata->toreq->stopable = 0;

  g_static_mutex_unlock (&tdata->structmutex);

  tdata->fromreq->logging_function (gftp_logging_misc, tdata->fromreq,
                                    _("Stopping the transfer on host %s\n"),
                                    tdata->toreq->hostname);
}


static void
_gftpui_common_next_file_in_trans (gftp_transfer * tdata)
{
  gftp_file * curfle;

  if (g_thread_supported ())
    g_static_mutex_lock (&tdata->structmutex);

  tdata->curtrans = 0;
  tdata->next_file = 1;

  curfle = tdata->curfle->data;
  curfle->transfer_done = 1;
  tdata->curfle = tdata->curfle->next;

  if (g_thread_supported ())
    g_static_mutex_unlock (&tdata->structmutex);
}


static int
_gftpui_common_preserve_perm_time (gftp_transfer * tdata, gftp_file * curfle)
{
  intptr_t preserve_permissions, preserve_time;
  int ret, tmpret;

  gftp_lookup_request_option (tdata->fromreq, "preserve_permissions",
                              &preserve_permissions);
  gftp_lookup_request_option (tdata->fromreq, "preserve_time",
                              &preserve_time);

  ret = 0;
  if (GFTP_IS_CONNECTED (tdata->toreq) && preserve_permissions &&
      curfle->st_mode != 0)
    {
      tmpret = gftp_chmod (tdata->toreq, curfle->destfile,
                           curfle->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
      if (tmpret < 0)
        ret = tmpret;
    }

  if (GFTP_IS_CONNECTED (tdata->toreq) && preserve_time &&
      curfle->datetime != 0)
    {
      tmpret = gftp_set_file_time (tdata->toreq, curfle->destfile,
                                   curfle->datetime);
      if (tmpret < 0)
        ret = tmpret;
    }

  if (!GFTP_IS_CONNECTED (tdata->toreq))
    return (ret);
  else
    return (0);
}


static int
_gftpui_common_trans_file_or_dir (gftp_transfer * tdata)
{
  gftp_file * curfle;
  int ret;

  if (g_thread_supported ())
    g_static_mutex_lock (&tdata->structmutex);

  curfle = tdata->curfle->data;
  tdata->current_file_number++;

  if (g_thread_supported ())
    g_static_mutex_unlock (&tdata->structmutex);

  if (curfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
    {
      tdata->tot_file_trans = 0;
      return (0);
    }

  if ((ret = gftp_connect (tdata->fromreq)) < 0)
    return (ret);

  if ((ret = gftp_connect (tdata->toreq)) < 0)
    return (ret);

  if (S_ISDIR (curfle->st_mode))
    {
      tdata->tot_file_trans = 0;
      if (curfle->startsize > 0)
        ret = 1;
      else
        ret = gftp_make_directory (tdata->toreq, curfle->destfile);
    }
  else
    {
      if (curfle->size == 0)
        {
          curfle->size = gftp_get_file_size (tdata->fromreq, curfle->file);
          if (curfle->size < 0)
            return ((int) curfle->size);

          tdata->total_bytes += curfle->size;
        }

      if (curfle->retry_transfer)
        {
          curfle->transfer_action = GFTP_TRANS_ACTION_RESUME;
          curfle->startsize = gftp_get_file_size (tdata->toreq, curfle->destfile);
          if (curfle->startsize < 0)
            return ((int) curfle->startsize);
        }

      tdata->tot_file_trans = gftp_transfer_file (tdata->fromreq, curfle->file,
                                                  curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ?
                                                          curfle->startsize : 0,
                                                  tdata->toreq, curfle->destfile,
                                                  curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ?
                                                          curfle->startsize : 0);
      if (tdata->tot_file_trans < 0)
        ret = tdata->tot_file_trans;
      else
        {
          if (g_thread_supported ())
            g_static_mutex_lock (&tdata->structmutex);

          tdata->curtrans = 0;
          tdata->curresumed = curfle->transfer_action == GFTP_TRANS_ACTION_RESUME ? curfle->startsize : 0;
          tdata->resumed_bytes += tdata->curresumed;

          if (g_thread_supported ())
            g_static_mutex_unlock (&tdata->structmutex);

          ret = _gftpui_common_do_transfer_file (tdata, curfle);
        }
    }

  if (ret == 0)
    ret = _gftpui_common_preserve_perm_time (tdata, curfle);
  else
    {
      curfle->retry_transfer = 1;
      tdata->fromreq->logging_function (gftp_logging_error, tdata->fromreq,
                                        _("Could not download %s from %s\n"),
                                        curfle->file, tdata->fromreq->hostname);
    }

  return (ret);
}


int
gftpui_common_transfer_files (gftp_transfer * tdata)
{
  int ret, skipped_files;

  tdata->curfle = tdata->files;
  gftpui_common_num_child_threads++;

  gettimeofday (&tdata->starttime, NULL);
  memcpy (&tdata->lasttime, &tdata->starttime, sizeof (tdata->lasttime));

  skipped_files = 0;
  while (tdata->curfle != NULL)
    {
      ret = _gftpui_common_trans_file_or_dir (tdata);
      if (tdata->cancel)
        {
          if (gftp_abort_transfer (tdata->toreq) != 0)
            gftp_disconnect (tdata->toreq);

          if (gftp_abort_transfer (tdata->fromreq) != 0)
            gftp_disconnect (tdata->fromreq);
        }
      else if (ret == GFTP_EFATAL)
        skipped_files++;
      else if (ret < 0)
        {
          if (gftp_get_transfer_status (tdata, ret) == GFTP_ERETRYABLE)
            continue;

          break;
        }

      _gftpui_common_next_file_in_trans (tdata);

      if (tdata->cancel)
        {
          if (!tdata->skip_file)
            break;

          tdata->cancel = 0;
          tdata->fromreq->cancel = 0;
          tdata->toreq->cancel = 0;
        }
    }

  if (skipped_files)
    tdata->fromreq->logging_function (gftp_logging_error, tdata->fromreq,
                                      _("There were %d files or directories that could not be transferred. Check the log for which items were not properly transferred."),
                                      skipped_files);

  tdata->done = 1;
  gftpui_common_num_child_threads--;

  return (1);
}


void
gftpui_protocol_update_timeout (gftp_request * request)
{
  intptr_t network_timeout;

  gftp_lookup_request_option (request, "network_timeout", &network_timeout);

  if (network_timeout > 0)
    alarm (network_timeout);
}

