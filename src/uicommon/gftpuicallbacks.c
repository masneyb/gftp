/*****************************************************************************/
/*  gftpui.c - UI related functions for gFTP. All of these functions must be */
/*             reentrant.                                                    */
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

#include "gftpui.h"
static const char cvsid[] = "$Id$";

int
gftpui_common_run_mkdir (gftpui_callback_data * cdata)
{
  int ret;

  ret = gftp_make_directory (cdata->request, cdata->input_string) == 0;

  return (ret);
}


int
gftpui_common_run_rename (gftpui_callback_data * cdata)
{
  int ret;

  ret = gftp_rename_file (cdata->request, cdata->source_string,
                          cdata->input_string) == 0;

  return (ret);
}


int
gftpui_common_run_site (gftpui_callback_data * cdata)
{
  int ret;

  ret = gftp_site_cmd (cdata->request, cdata->input_string) == 0;

  return (ret);
}


int
gftpui_common_run_chdir (gftpui_callback_data * cdata)
{
  int ret;

  ret = gftp_set_directory (cdata->request, cdata->input_string) == 0;

  return (ret);
}


int
gftpui_common_run_chmod (gftpui_callback_data * cdata)
{
  int ret;

  ret = gftp_chmod (cdata->request, cdata->source_string,
                    strtol (cdata->input_string, NULL, 10)) == 0;

  return (ret);
}


int
gftpui_common_run_delete (gftpui_callback_data * cdata)
{
  int ret;

  if (cdata->input_string != NULL)
    {
      ret = gftp_remove_file (cdata->request, cdata->input_string) == 0;
    }
  else
    ret = 0; /* FIXME */

  return (ret);
}


int
gftpui_common_run_rmdir (gftpui_callback_data * cdata)
{
  int ret;

  if (cdata->input_string != NULL)
    {
      ret = gftp_remove_directory (cdata->request, cdata->input_string) == 0;
    }
  else
    ret = 0; /* FIXME */

  return (ret);
}

