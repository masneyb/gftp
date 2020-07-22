/*****************************************************************************/
/*  chmod_dialog.c - the chmod dialog box                                    */
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

static GtkWidget *suid, *sgid, *sticky, *ur, *uw, *ux, *gr, *gw, *gx, *or, *ow,
                 *ox;
static mode_t mode; 


static int
do_chmod_thread (gftpui_callback_data * cdata)
{
  gftp_window_data * wdata;
  GList * templist, *igl;
  gftp_file * tempfle;
  int error;

  wdata = cdata->uidata;
  error = 0;

  templist = listbox_get_selected_files(wdata);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
     tempfle = (gftp_file *) igl->data;
     if (gftp_chmod (wdata->request, tempfle->file, mode) != 0) {
        error = 1;
     }
     if (!GFTP_IS_CONNECTED (wdata->request)) {
        break;
     }
  }
  g_list_free (templist);

  return (error);
}

static gboolean
tb_get_active(GtkWidget *w)
{
  GtkToggleButton *tb = GTK_TOGGLE_BUTTON (w);
  return (gtk_toggle_button_get_active (tb));
}

static void
tb_set_active(GtkWidget *w, gboolean is_active)
{
  GtkToggleButton *tb = GTK_TOGGLE_BUTTON (w);
  gtk_toggle_button_set_active (tb, is_active);
}

static void
dochmod (gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  mode = 0;
  if (tb_get_active (suid))   mode |= S_ISUID;
  if (tb_get_active (sgid))   mode |= S_ISGID;
  if (tb_get_active (sticky)) mode |= S_ISVTX;

  if (tb_get_active (ur))  mode |= S_IRUSR;
  if (tb_get_active (uw))  mode |= S_IWUSR;
  if (tb_get_active (ux))  mode |= S_IXUSR;

  if (tb_get_active (gr))  mode |= S_IRGRP;
  if (tb_get_active (gw))  mode |= S_IWGRP;
  if (tb_get_active (gx))  mode |= S_IXGRP;

  if (tb_get_active (or))  mode |= S_IROTH;
  if (tb_get_active (ow))  mode |= S_IWOTH;
  if (tb_get_active (ox))  mode |= S_IXOTH;

  if (check_reconnect (wdata) < 0)
    return;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = do_chmod_thread;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
}

static void
on_gtk_dialog_response_chmod (GtkDialog *dialog, gint response, gpointer wdata)
{ /* chmod action */
  switch (response)
    {
      case GTK_RESPONSE_OK:
        dochmod (wdata);
        /* no break */
      default:
        gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}


void
chmod_dialog (gpointer data)
{
  GtkWidget *tempwid, *dialog, *hbox, *vbox, *main_vbox;
  gftp_window_data * wdata;
  gftp_file * tempfle;

  wdata = data;
  if (!check_status (_("Chmod"), wdata, gftpui_common_use_threads (wdata->request), 0, 1, wdata->request->chmod != NULL))
    return;

  dialog = gtk_dialog_new_with_buttons (_("Chmod"), NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);

  gtk_window_set_role (GTK_WINDOW(dialog), "Chmod");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_widget_realize (dialog);

  set_window_icon(GTK_WINDOW(dialog), NULL);

  tempwid = gtk_label_new (_("You can now adjust the attributes of your file(s)\nNote: Not all ftp servers support the chmod feature"));
  gtk_box_pack_start (GTK_BOX (main_vbox), tempwid, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  tempwid = gtk_frame_new (_("Special"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

  suid = gtk_check_button_new_with_label (_("SUID"));
  gtk_box_pack_start (GTK_BOX (vbox), suid, FALSE, FALSE, 0);

  sgid = gtk_check_button_new_with_label (_("SGID"));
  gtk_box_pack_start (GTK_BOX (vbox), sgid, FALSE, FALSE, 0);

  sticky = gtk_check_button_new_with_label (_("Sticky"));
  gtk_box_pack_start (GTK_BOX (vbox), sticky, FALSE, FALSE, 0);

  tempwid = gtk_frame_new (_("User"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

  ur = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), ur, FALSE, FALSE, 0);

  uw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), uw, FALSE, FALSE, 0);

  ux = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ux, FALSE, FALSE, 0);

  tempwid = gtk_frame_new (_("Group"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

  gr = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), gr, FALSE, FALSE, 0);

  gw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), gw, FALSE, FALSE, 0);

  gx = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), gx, FALSE, FALSE, 0);

  tempwid = gtk_frame_new (_("Other"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);

  or = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), or, FALSE, FALSE, 0);

  ow = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), ow, FALSE, FALSE, 0);

  ox = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ox, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (dialog), // GtkDialog
                    "response",        // signal
                    G_CALLBACK (on_gtk_dialog_response_chmod),
                    wdata);

  if (listbox_num_selected (wdata) == 1)
    {
      tempfle = listbox_get_selected_file1 (wdata);

      tb_set_active (suid, tempfle->st_mode & S_ISUID);
      tb_set_active (ur,   tempfle->st_mode & S_IRUSR);
      tb_set_active (uw,   tempfle->st_mode & S_IWUSR);
      tb_set_active (ux,   tempfle->st_mode & S_IXUSR);

      tb_set_active (sgid, tempfle->st_mode & S_ISGID);
      tb_set_active (gr,   tempfle->st_mode & S_IRGRP);
      tb_set_active (gw,   tempfle->st_mode & S_IWGRP);
      tb_set_active (gx,   tempfle->st_mode & S_IXGRP);

      tb_set_active (sticky, tempfle->st_mode & S_ISVTX);
      tb_set_active (or,     tempfle->st_mode & S_IROTH);
      tb_set_active (ow,     tempfle->st_mode & S_IWOTH);
      tb_set_active (ox,     tempfle->st_mode & S_IXOTH);
    }

  gtk_widget_show_all (dialog);
}

