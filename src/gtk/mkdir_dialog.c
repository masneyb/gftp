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

static void domkdir 				( GtkWidget * widget, 
						  gftp_dialog_data * data );
static void *do_make_dir_thread 		( void * data );
static RETSIGTYPE sig_mkdirquit 		( int signo );

static const char *edttext;
static sigjmp_buf mkdirenvir;

void
mkdir_dialog (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!check_status (_("Mkdir"), wdata, wdata->request->use_threads, 0, 0, 
                     wdata->request->mkdir != NULL))
    return;

  MakeEditDialog (_("Make Directory"), _("Enter name of directory to create"),
		  NULL, 1, 1, NULL, _("Create"), domkdir, wdata, 
                  _("  Cancel  "), NULL, NULL);
}


static void
domkdir (GtkWidget * widget, gftp_dialog_data * data)
{
  gftp_window_data * wdata;

  wdata = data->data;
  edttext = gtk_entry_get_text (GTK_ENTRY (data->edit));
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
      gftp_delete_cache_entry (wdata->request);
      refresh (wdata);
    }
}



static void *
do_make_dir_thread (void * data)
{
  gftp_window_data * wdata;
  int success, sj;

  wdata = data;
  wdata->request->user_data = (void *) 0x01;

  if (wdata->request->use_threads)
    {
      sj = sigsetjmp (mkdirenvir, 1);
      signal (SIGINT, sig_mkdirquit);
      signal (SIGALRM, sig_mkdirquit);
    }
  else
    sj = 0;

  success = 0;
  if (sj == 0)
    {
      if (wdata->request->network_timeout > 0)
        alarm (wdata->request->network_timeout);
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
    {
      signal (SIGINT, SIG_DFL);
      signal (SIGALRM, SIG_IGN);
    }

  wdata->request->user_data = NULL;
  wdata->request->stopable = 0;
  return ((void *) success);
}


static RETSIGTYPE
sig_mkdirquit (int signo)
{
  signal (signo, sig_mkdirquit);
  siglongjmp (mkdirenvir, signo == SIGINT ? 1 : 2);
}

