/*****************************************************************************/
/*  mkdir_dialog.c - make directory dialog box and ftp routines              */
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


static void *
do_make_dir_thread (void * data)
{
  int success, sj, network_timeout;
  gftp_window_data * wdata;

  wdata = data;

  gftp_lookup_request_option (wdata->request, "network_timeout", 
                              &network_timeout);

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
      if (network_timeout > 0)
        alarm (network_timeout);
      success = gftp_make_directory (wdata->request, edttext) == 0;
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
domkdir (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("Mkdir: Operation canceled...you must enter a string\n"));
      return;
    }

  if (check_reconnect (wdata) < 0)
    return;

  if ((int) generic_thread (do_make_dir_thread, wdata))
    {
      gftp_delete_cache_entry (wdata->request, 0);
      refresh (wdata);
    }
}


void
mkdir_dialog (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!check_status (_("Mkdir"), wdata, wdata->request->use_threads, 0, 0, 
                     wdata->request->mkdir != NULL))
    return;

  MakeEditDialog (_("Make Directory"), _("Enter name of directory to create"),
		  NULL, 1, NULL, gftp_dialog_button_create, domkdir, wdata, 
                  NULL, NULL);
}

