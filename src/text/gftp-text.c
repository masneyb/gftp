/***********************************************************************************/
/*  gftp-text.c - text port of gftp                                                */
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

#include "gftp-text.h"

unsigned int
gftp_text_get_win_size (void)
{
  struct winsize size;
  unsigned int ret;

  if (ioctl (0, TIOCGWINSZ, (char *) &size) < 0)
    ret = 80;
  else
    ret = size.ws_col;

  return (ret);
}


static void
gftp_text_write_string (gftp_request * request, char *string)
{
  gchar *stpos, *endpos, *locale_str, savechar;
  unsigned int sw;
  size_t destlen;

  sw = gftp_text_get_win_size ();

  locale_str = gftp_string_from_utf8 (request, 1, string, &destlen);
  if (locale_str == NULL)
    stpos = string;
  else
    stpos = locale_str;

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
    }
  while (stpos != endpos);

  if (locale_str != NULL)
    g_free (locale_str);
}


static void
gftp_text_log (gftp_logging_level level, gftp_request * request, 
               const char *string, ...)
{
  char tempstr[512];
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

  if (gftp_logfd != NULL && level != gftp_logging_misc_nolog)
    {
      fwrite (tempstr, 1, strlen (tempstr), gftp_logfd);
      if (ferror (gftp_logfd))
        {
          fclose (gftp_logfd);
          gftp_logfd = NULL;
        }
      else
        fflush (gftp_logfd);
    }

  if (level == gftp_logging_misc_nolog)
    printf ("%s", tempstr);
  else
    gftp_text_write_string (request, tempstr);
  
  printf ("%s", GFTPUI_COMMON_COLOR_DEFAULT);
}


char *
gftp_text_ask_question (gftp_request * request, const char *question, int echo,
                        char *buf, size_t size)
{
  struct termios term, oldterm;
  gchar *locale_question;
  sigset_t sig, sigsave;
  char *pos, *termname;
  size_t destlen;
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

  locale_question = gftp_string_from_utf8 (request, 1, question, &destlen);
  if (locale_question != NULL)
    {
      printf ("%s%s%s ", GFTPUI_COMMON_COLOR_BLUE, locale_question,
              GFTPUI_COMMON_COLOR_DEFAULT);
      g_free (locale_question);
    }
  else
    printf ("%s%s%s ", GFTPUI_COMMON_COLOR_BLUE, question,
            GFTPUI_COMMON_COLOR_DEFAULT);

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

  if (gftp_protocols[GFTP_PROTOCOL_LOCALFS].init (gftp_text_locreq) == 0)
    {
      gftp_setup_startup_directory (gftp_text_locreq,
                                    "local_startup_directory");
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

      gftp_setup_startup_directory (gftp_text_remreq,
                                    "remote_startup_directory");
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
 
  gftp_shutdown ();
  return (0);
}

