/*****************************************************************************/
/*  rename_dialog.c - rename dialog box and ftp routines                     */
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

#include "gftp-gtk.h"
static const char cvsid[] = "$Id$";

static const char *edttext;
static gftp_file * curfle;


static void *
do_rename_thread (void * data)
{
  gftp_window_data * wdata;
  int success, sj;

  wdata = data;

  if (wdata->request->use_threads)
    { 
      sj = sigsetjmp (jmp_environment, 1);
      use_jmp_environment = 1;
    }
  else
    sj = 0;

  success = 0;
  if (sj == 0)
    {
      if (wdata->request->network_timeout > 0)
        alarm (wdata->request->network_timeout);
      success = gftp_rename_file (wdata->request, curfle->file, edttext) == 0;
      alarm (0);
    }
  else
    {
      gftp_disconnect (wdata->request);
      wdata->request->logging_function (gftp_logging_error,
                                        wdata->request->user_data,
                                        _("Operation canceled\n"));
    }

  if (wdata->request->use_threads)
    use_jmp_environment = 0;

  wdata->request->stopable = 0;
  return ((void *) success);
}


static void
dorenCB (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("Rename: Operation canceled...you must enter a string\n"));
      return;
    }

  if (check_reconnect (wdata) < 0) 
    return;

  if ((int) generic_thread (do_rename_thread, wdata))
    refresh ((gpointer) wdata);
}


void
rename_dialog (gpointer data)
{
  GList *templist, *filelist;
  gftp_window_data * wdata;
  char *tempstr;
  int num;

  wdata = data;
  if (!check_status (_("Rename"), wdata, wdata->request->use_threads, 1, 1, 
                     wdata->request->rename != NULL))
    return;

  templist = GTK_CLIST (wdata->listbox)->selection;
  num = 0;
  filelist = wdata->files;
  templist = get_next_selection (templist, &filelist, &num);
  curfle = filelist->data;

  tempstr = g_strdup_printf (_("What would you like to rename %s to?"), 
                             curfle->file);
  MakeEditDialog (_("Rename"), tempstr, curfle->file, 1, NULL,
                  gftp_dialog_button_rename, dorenCB, wdata, NULL, NULL);
  g_free (tempstr);
}

