/*****************************************************************************/
/*  delete_dialog.c - the delete dialog                                      */
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

#include "gftp-gtk.h"
static const char cvsid[] = "$Id$";

static void
yesCB (gftp_transfer * transfer, gftp_dialog_data * ddata)
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
  free_tdata (transfer);

  if (!GFTP_IS_CONNECTED (wdata->request))
    gftpui_disconnect (wdata);
  else
    gftpui_refresh (wdata);

  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
}


static void
askdel (gftp_transfer * transfer)
{
  char *tempstr;

  if (transfer->numfiles > 0 && transfer->numdirs > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld files and %ld directories"), transfer->numfiles, transfer->numdirs);
    }
  else if (transfer->numfiles > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld files"), transfer->numfiles, transfer->numdirs);
    }
  else if (transfer->numdirs > 0)
    {
      tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld directories"), transfer->numfiles, transfer->numdirs);
    }
  else
    return;

  MakeYesNoDialog (_("Delete Files/Directories"), tempstr, 
                   yesCB, transfer, NULL, NULL);
  g_free (tempstr);
}


void
delete_dialog (gpointer data)
{
  gftp_file * tempfle, * newfle;
  GList * templist, * filelist;
  gftp_transfer * transfer;
  gftp_window_data * wdata;
  int num, ret;

  wdata = data;
  if (!check_status (_("Delete"), wdata, gftpui_common_use_threads (wdata->request), 0, 1, 1))
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (wdata->request);
  transfer->fromwdata = wdata;

  num = 0;
  templist = GTK_CLIST (wdata->listbox)->selection;
  filelist = wdata->files;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      if (strcmp (tempfle->file, "..") == 0 ||
          strcmp (tempfle->file, ".") == 0)
        continue;
      newfle = copy_fdata (tempfle);
      transfer->files = g_list_append (transfer->files, newfle);
    }

  if (transfer->files == NULL)
    {
      free_tdata (transfer);
      return;
    }

  gftp_swap_socks (transfer->fromreq, wdata->request);

  ret = gftp_gtk_get_subdirs (transfer, &wdata->tid);

  if (!GFTP_IS_CONNECTED (transfer->fromreq))
    {
      gftpui_disconnect (wdata);
      return;
    }

  gftp_swap_socks (wdata->request, transfer->fromreq);

  if (!ret)
    return;

  askdel (transfer);
}


