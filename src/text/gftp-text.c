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
        printf ("%s", GFTPUI_COMMON_COLOR_GREEN);
        break;
      case gftp_logging_recv:
        printf ("%s", GFTPUI_COMMON_COLOR_YELLOW);
        break;
      case gftp_logging_error:
        printf ("%s", GFTPUI_COMMON_COLOR_RED);
        break;
      default:
        printf ("%s", GFTPUI_COMMON_COLOR_DEFAULT);
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
  
  printf ("%s", GFTPUI_COMMON_COLOR_DEFAULT);

  if (utf8_str != NULL)
    g_free (utf8_str);
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

  printf ("%s%s%s: ", GFTPUI_COMMON_COLOR_BLUE, question, GFTPUI_COMMON_COLOR_DEFAULT);

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
main (int argc, char **argv)
{
  char *startup_directory;
#if HAVE_LIBREADLINE
  char *tempstr, prompt[20];
#else
  char tempstr[512];
#endif

  gftp_locale_init ();

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

  gftpui_common_about (gftp_text_log, NULL);
  gftp_text_log (gftp_logging_misc, NULL, "\n");

/* FIXME
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
*/

  gftpui_common_init (NULL, gftp_text_locreq,
                      NULL, gftp_text_remreq);

#if HAVE_LIBREADLINE
  g_snprintf (prompt, sizeof (prompt), "%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
  while ((tempstr = readline (prompt)))
    {
      if (gftpui_common_process_command (tempstr) == 0)
        break;
   
      add_history (tempstr);
      free (tempstr);
    }
#else
  printf ("%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
  while (fgets (tempstr, sizeof (tempstr), stdin) != NULL)
    {
      if (gftpui_common_process_command (tempstr) == 0)
        break;

      printf ("%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
    }
#endif
 
  return (0);
}


#if 0
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
  int i, j, sw, tot;
  intptr_t preserve_permissions;
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

#endif
