/*****************************************************************************/
/*  textui.c - Text UI related functions for gFTP                            */
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

void
gftpui_lookup_file_colors (gftp_file * fle, char **start_color,
                           char ** end_color)
{
  if (*fle->attribs == 'd')
    *start_color = GFTPUI_COMMON_COLOR_BLUE;
  else if (*fle->attribs == 'l')
    *start_color = GFTPUI_COMMON_COLOR_WHITE;
  else if (strchr (fle->attribs, 'x') != NULL)
    *start_color = GFTPUI_COMMON_COLOR_GREEN;
  else
    *start_color = GFTPUI_COMMON_COLOR_DEFAULT;

  *end_color = GFTPUI_COMMON_COLOR_DEFAULT;
}


void
gftpui_refresh (void *uidata)
{
  gftp_request * request;

  request = uidata; /* Note: uidata is set to the request in gftp_text.c */
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
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  char tempstr[256];

  gftp_set_username (request, 
                     gftp_text_ask_question (_("Username [anonymous]:"), 1,
                                             tempstr, sizeof (tempstr)));
}



void
gftpui_prompt_password (void *uidata, gftp_request * request)
{
  char tempstr[256];

  gftp_set_password (request,
                     gftp_text_ask_question (_("Password:"), 0,
                                             tempstr, sizeof (tempstr)));
}


void
gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle)
{
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
      if (tempfle->startsize == 0 || tempfle->isdir)
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

          gftp_text_ask_question (question, 1, &buf, 1);

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
  int sw, tot, i;

  printf ("\r%c [", progress[progress_pos++]);

  if (progress[progress_pos] == '\0')
    progress_pos = 0;

  sw = gftp_text_get_win_size () - 20;
  tot = (float) tdata->curtrans / (float) tdata->tot_file_trans * (float) sw;
                        
  if (tot > sw)
    tot = sw;

  for (i=0; i<tot; i++)
    printf ("=");

  for (i=0; i<sw-tot; i++)
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
      gftp_text_ask_question (question, 1, buf, sizeof (buf));
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

