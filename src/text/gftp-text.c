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


static void
gftp_text_write_string (char *string)
{
  char *stpos, *endpos, savechar;
  int sw;

  sw = gftp_text_get_win_size ();

  stpos = string;
  do
    {
      if ((endpos = strchr (stpos, '\n')) == NULL)
        endpos = stpos + strlen (stpos);

      savechar = *endpos;
      *endpos = '\0';

      if (strlen (stpos) <= sw)
        {
          printf ("%s%c", stpos, savechar);
          *endpos = savechar;
          if (savechar == '\0')
            break;
          stpos = endpos + 1;
        }
      else
        {
          *endpos = savechar;
          for (endpos = stpos + sw - 1;
               *endpos != ' ' && endpos > stpos;
               endpos--);

          if (endpos != stpos)
            *endpos = '\0';

          printf ("%s\n", stpos);
          stpos = endpos + 1;
        }

      sw = sw;
    }
  while (stpos != endpos);
}


static void
gftp_text_log (gftp_logging_level level, gftp_request * request, 
               const char *string, ...)
{
  char tempstr[512], *utf8_str = NULL, *outstr;
  va_list argp;

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

  if (gftp_logfd != NULL && level != gftp_logging_misc_nolog)
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

  if (level == gftp_logging_misc_nolog)
    printf ("%s\n", outstr);
  else
    gftp_text_write_string (outstr);
  
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
  int singlechar;
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

  printf ("%s%s%s ", GFTPUI_COMMON_COLOR_BLUE, question, GFTPUI_COMMON_COLOR_DEFAULT);

  if (size == 1)
    {
      singlechar = fgetc (infd);
      *buf = singlechar;
    }
  else
    {
      if (fgets (buf, size, infd) == NULL)
        return (NULL);

      if (size > 1)
        buf[size - 1] = '\0';
    }

  if (!echo)
    {
      printf ("\n");
      tcsetattr (fileno (infd), TCSAFLUSH, &oldterm);
      fclose (infd);
      sigprocmask (SIG_SETMASK, &sigsave, NULL);
    }

  if (size > 1)
    {
      for (pos = buf + strlen (buf) - 1; *pos == ' ' || *pos == '\r' ||
                                         *pos == '\n'; pos--);
      *(pos+1) = '\0';

      for (pos = buf; *pos == ' '; pos++);  

      if (*pos == '\0')
        return (NULL);

      return (pos);
    }
  else
    return (buf);
}


int
main (int argc, char **argv)
{
  gftp_request * gftp_text_locreq, * gftp_text_remreq;
  void *locuidata, *remuidata;
  char *pos;
#if HAVE_LIBREADLINE
  char *tempstr, prompt[20];
#else
  char tempstr[512];
#endif

  gftpui_common_init (&argc, &argv, gftp_text_log);

  /* SSH doesn't support reading the password with askpass via the command 
     line */

  gftp_text_remreq = gftp_request_new ();
  remuidata = gftp_text_remreq;
  gftp_text_remreq->logging_function = gftp_text_log;

  gftp_text_locreq = gftp_request_new ();
  locuidata = gftp_text_locreq;
  gftp_text_locreq->logging_function = gftp_text_log;

  if (gftp_protocols[GFTP_LOCAL_NUM].init (gftp_text_locreq) == 0)
    {
      gftp_setup_startup_directory (gftp_text_locreq);
      gftp_connect (gftp_text_locreq);
    }

  gftpui_common_about (gftp_text_log, NULL);
  gftp_text_log (gftp_logging_misc, NULL, "\n");

  if (argc == 3 && strcmp (argv[1], "-d") == 0)
    {
      if ((pos = strrchr (argv[2], '/')) != NULL)
        *pos = '\0';

      gftpui_common_cmd_open (remuidata, gftp_text_remreq,
                              locuidata, gftp_text_locreq,
                              argv[2]);

      if (pos != NULL)
        *pos = '/';

      gftpui_common_cmd_mget_file (remuidata, gftp_text_remreq,
                                   locuidata, gftp_text_locreq,
                                   pos + 1);
      exit (0);
    }
  else if (argc == 2)
    {
      gftpui_common_cmd_open (remuidata, gftp_text_remreq,
                              locuidata, gftp_text_locreq,
                              argv[1]);
    }

#if HAVE_LIBREADLINE
  g_snprintf (prompt, sizeof (prompt), "%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
  while ((tempstr = readline (prompt)))
    {
      if (gftpui_common_process_command (locuidata, gftp_text_locreq,
                                         remuidata, gftp_text_remreq,
                                         tempstr) == 0)
        break;
   
      add_history (tempstr);
      free (tempstr);
    }
#else
  printf ("%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
  while (fgets (tempstr, sizeof (tempstr), stdin) != NULL)
    {
      if (gftpui_common_process_command (locuidata, gftp_text_locreq,
                                         remuidata, gftp_text_remreq,
                                         tempstr) == 0)
        break;

      printf ("%sftp%s> ", GFTPUI_COMMON_COLOR_BLUE, GFTPUI_COMMON_COLOR_DEFAULT);
    }
#endif
 
  return (0);
}

