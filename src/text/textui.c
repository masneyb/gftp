/***********************************************************************************/
/*  textui.c - Text UI related functions for gFTP                                  */
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

void
gftpui_lookup_file_colors (gftp_file * fle, char **start_color,
                           char ** end_color)
{
  if (S_ISDIR (fle->st_mode))
    *start_color = GFTPUI_COMMON_COLOR_BLUE;
  else if (S_ISLNK (fle->st_mode))
    *start_color = GFTPUI_COMMON_COLOR_WHITE;
  else if ((fle->st_mode & S_IXUSR) ||
           (fle->st_mode & S_IXGRP) ||
           (fle->st_mode & S_IXOTH))
    *start_color = GFTPUI_COMMON_COLOR_GREEN;
  else
    *start_color = GFTPUI_COMMON_COLOR_DEFAULT;

  *end_color = GFTPUI_COMMON_COLOR_DEFAULT;
}


void
gftpui_refresh (void *uidata, int clear_cache_entry)
{
  gftp_request * request;

  request = uidata; /* Note: uidata is set to the request in gftp_text.c */

  if (clear_cache_entry)
    gftp_delete_cache_entry (request, NULL, 0);
}


void *
gftpui_generic_thread (void * (*func) (void *), void *data)
{
  return (func (data));
}


int
gftpui_check_reconnect (gftpui_callback_data * cdata)
{
  return (1);
}

void
gftpui_show_busy (gboolean busy)
{
  /* do nothing for text based */
}

void
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  char tempstr[256];

  gftp_set_username (request, 
                     gftp_text_ask_question (request,
                                             _("Username [anonymous]:"), 1,
                                             tempstr, sizeof (tempstr)));
}



void
gftpui_prompt_password (void *uidata, gftp_request * request)
{
  char tempstr[256];

  gftp_set_password (request,
                     gftp_text_ask_question (request, _("Password:"), 0,
                                             tempstr, sizeof (tempstr)));
}


void
gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle)
{
}


void
gftpui_cancel_file_transfer (gftp_transfer * tdata)
{
  tdata->cancel = 1;
  tdata->fromreq->cancel = 1;
  tdata->toreq->cancel = 1;
}


void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  char buf, question[1024], srcsize[50], destsize[50], *pos, defaction;
  int action, newaction;
  gftp_file * tempfle;
  GList * templist;

  action = newaction = -1;

  for (templist = tdata->files; templist != NULL; templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->startsize == 0 || S_ISDIR (tempfle->st_mode))
        continue;

      while (action == -1)
        {
          insert_commas (tempfle->size, srcsize, sizeof (srcsize));
          insert_commas (tempfle->startsize, destsize, sizeof (destsize));

          if ((pos = strrchr (tempfle->file, '/')) != NULL)
            pos++;
          else
            pos = tempfle->file;

          gftp_get_transfer_action (tdata->fromreq, tempfle);
          switch (tempfle->transfer_action)
            {
              case GFTP_TRANS_ACTION_OVERWRITE:
                action = GFTP_TRANS_ACTION_OVERWRITE;
                defaction = 'o';
                break;
              case GFTP_TRANS_ACTION_SKIP:
                action = GFTP_TRANS_ACTION_SKIP;
                defaction = 's';
                break;
              case GFTP_TRANS_ACTION_RESUME:
                action = GFTP_TRANS_ACTION_RESUME;
                defaction = 'r';
                break;
              default:
                defaction = ' ';
                break;
            }
    
          g_snprintf (question, sizeof (question), _("%s already exists. (%s source size, %s destination size):\n(o)verwrite, (r)esume, (s)kip, (O)verwrite All, (R)esume All, (S)kip All: (%c)"), pos, srcsize, destsize, defaction);

          gftp_text_ask_question (tdata->fromreq, question, 1, &buf, 1);

          switch (buf)
            {
              case 'o':
                action = GFTP_TRANS_ACTION_OVERWRITE;
                break;
              case 'O':
                action = newaction = GFTP_TRANS_ACTION_OVERWRITE;
                break;
              case 'r':
                action = GFTP_TRANS_ACTION_RESUME;
                break;
              case 'R':
                action = newaction = GFTP_TRANS_ACTION_RESUME;
                break;
              case 's':
                action = GFTP_TRANS_ACTION_SKIP;
                break;
              case 'S':
                action = newaction = GFTP_TRANS_ACTION_SKIP;
                break;
              case '\0':
              case '\n':
                break;
              default:
                action = -1;
                break;
            }
        }

      tempfle->transfer_action = action;
      action = newaction;
    }
}


static void
_gftpui_text_print_status (gftp_transfer * tdata)
{
  static int progress_pos = 0;
  char *progress = "|/-\\";
  unsigned int sw, tot, i;

  printf ("\r%c [", progress[progress_pos++]);

  if (progress[progress_pos] == '\0')
    progress_pos = 0;

  sw = gftp_text_get_win_size () - 20;
  tot = (unsigned int) ((float) tdata->curtrans / (float) tdata->tot_file_trans * (float) sw);
                        
  if (tot > sw)
    tot = sw;

  for (i = 0; i < tot; i++)
    printf ("=");

  for (i = 0; i < sw - tot; i++)
    printf (" ");

  printf ("] @ %.2fKB/s", tdata->kbs);

  fflush (stdout);
}


void
gftpui_start_current_file_in_transfer (gftp_transfer * tdata)
{
  _gftpui_text_print_status (tdata);
}


void
gftpui_update_current_file_in_transfer (gftp_transfer * tdata)
{
  _gftpui_text_print_status (tdata);
}


void
gftpui_finish_current_file_in_transfer (gftp_transfer * tdata)
{
  _gftpui_text_print_status (tdata);
  printf ("\n");
}


void
gftpui_start_transfer (gftp_transfer * tdata)
{
  gftpui_common_transfer_files (tdata);
}


void
gftpui_disconnect (void *uidata)
{
  gftp_request * request;

  request = uidata; /* Note: uidata is set to the request in gftp_text.c */
  gftp_disconnect (request);
}


int
gftpui_protocol_ask_yes_no (gftp_request * request, char *title, char *question)
{
  char buf[10];
  int ret;

  do
    {
      gftp_text_ask_question (request, question, 1, buf, sizeof (buf));
      if (strcasecmp (buf, "yes") == 0)
        ret = 1;
      else if (strcasecmp (buf, "no") == 0)
        ret = 0;
      else
        ret = -1;
    }
  while (ret == -1);

  return (ret);
}


char *
gftpui_protocol_ask_user_input (gftp_request * request, char *title,
                                char *question, int shown)
{
  char buf[255];

  do
    {
      gftp_text_ask_question (request, question, shown, buf, sizeof (buf));
    }
  while (*buf == '\0');

  return (g_strdup (buf));
}

