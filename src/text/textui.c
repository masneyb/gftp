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
  /* FIXME - clear the cache entry */
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


char *
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  char tempstr[256], *ret;

  ret = g_strdup (gftp_text_ask_question (_("Username [anonymous]:"), 1,
                                          tempstr, sizeof (tempstr)));
  return (ret);
}



char *
gftpui_prompt_password (void *uidata, gftp_request * request)
{
  char tempstr[256], *ret;

  ret = g_strdup (gftp_text_ask_question (_("Password:"), 0,
                                          tempstr, sizeof (tempstr)));
  return (ret);
}


void
gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle,
                                  char *filepos )
{
  /* FIXME */
}


void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  /* FIXME */
}

