/*****************************************************************************/
/*  delete_dialog.c - the delete dialog                                      */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
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

static void
yesCB (gftp_transfer * transfer)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  g_return_if_fail (transfer != NULL);
  g_return_if_fail (transfer->files != NULL);

  wdata = transfer->fromwdata;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->files = transfer->files;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_delete;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
  free_tdata (transfer); //_gftp_gtk_free_del_data (transfer);
}


void
delete_dialog (gpointer data)
{
  gftp_window_data * wdata = (gftp_window_data *) data;
  if (!wdata) {
     return;
  }
  long int num = listbox_num_selected (wdata);
  if (num > 0)
  {
      char * tempstr = g_strdup_printf (_("Are you sure you want to delete the %ld selected item(s)"), num);
      MakeYesNoDialog (_("Delete Files/Directories"), tempstr,
                   do_delete_dialog, data, NULL, NULL);
      g_free (tempstr);  
  }
}


void
do_delete_dialog (gpointer data)
{
  gftp_file * tempfle, * newfle;
  GList * templist, *igl;
  gftp_transfer * transfer;
  gftp_window_data * wdata;
  int ret;

  wdata = data;

  if (!check_status (_("Delete"), wdata,
      gftpui_common_use_threads (wdata->request), 0, 1, 1))
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (wdata->request);
  transfer->fromwdata = wdata;

  templist = listbox_get_selected_files(wdata);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
     tempfle = (gftp_file *) igl->data;
     if (strcmp (tempfle->file, "..") == 0 ||
         strcmp (tempfle->file, ".") == 0)
            continue;
     newfle = copy_fdata (tempfle);
     transfer->files = g_list_append (transfer->files, newfle);
  }
  g_list_free (templist);

  if (transfer->files == NULL)
    {
      free_tdata (transfer);
      return;
    }

  gftp_swap_socks (transfer->fromreq, wdata->request);

  ret = gftp_gtk_get_subdirs (transfer);

  gftp_swap_socks (wdata->request, transfer->fromreq);

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      gftpui_disconnect (wdata);
      return;
    }

  if (!ret)
    return;

  yesCB (transfer);
}
