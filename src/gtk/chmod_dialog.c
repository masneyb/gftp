/*****************************************************************************/
/*  chmod_dialog.c - the chmod dialog box                                    */
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

static GtkWidget *suid, *sgid, *sticky, *ur, *uw, *ux, *gr, *gw, *gx, *or, *ow,
                 *ox;
static mode_t mode; 


static int
do_chmod_thread (gftpui_callback_data * cdata)
{
  GList * filelist, * templist;
  gftp_window_data * wdata;
  gftp_file * tempfle;
  int error, num;

  wdata = cdata->uidata;
  error = 0;

  filelist = wdata->files;
  templist = gftp_gtk_get_list_selection (wdata);
  num = 0;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;

      if (gftp_chmod (wdata->request, tempfle->file, mode) != 0)
        error = 1;

      if (!GFTP_IS_CONNECTED (wdata->request))
        break;
    }

  return (error);
}


static void
dochmod (GtkWidget * widget, gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  mode = 0;
  if (GTK_TOGGLE_BUTTON (suid)->active)
    mode |= S_ISUID;
  if (GTK_TOGGLE_BUTTON (sgid)->active)
    mode |= S_ISGID;
  if (GTK_TOGGLE_BUTTON (sticky)->active)
    mode |= S_ISVTX;

  if (GTK_TOGGLE_BUTTON (ur)->active)
    mode |= S_IRUSR;
  if (GTK_TOGGLE_BUTTON (uw)->active)
    mode |= S_IWUSR;
  if (GTK_TOGGLE_BUTTON (ux)->active)
    mode |= S_IXUSR;

  if (GTK_TOGGLE_BUTTON (gr)->active)
    mode |= S_IRGRP;
  if (GTK_TOGGLE_BUTTON (gw)->active)
    mode |= S_IWGRP;
  if (GTK_TOGGLE_BUTTON (gx)->active)
    mode |= S_IWOTH;

  if (GTK_TOGGLE_BUTTON (or)->active)
    mode |= S_IROTH;
  if (GTK_TOGGLE_BUTTON (ow)->active)
    mode |= S_IWOTH;
  if (GTK_TOGGLE_BUTTON (ox)->active)
    mode |= S_IXOTH;

  if (check_reconnect (wdata) < 0)
    return;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = do_chmod_thread;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
}


#if GTK_MAJOR_VERSION > 1
static void
chmod_action (GtkWidget * widget, gint response, gpointer wdata)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        dochmod (widget, wdata);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}
#endif


void
chmod_dialog (gpointer data)
{
  GtkWidget *tempwid, *dialog, *hbox, *vbox;
  GList * templist, * filelist;
  gftp_window_data * wdata;
  gftp_file * tempfle;
  int num;

  wdata = data;
  if (!check_status (_("Chmod"), wdata, gftpui_common_use_threads (wdata->request), 0, 1, wdata->request->chmod != NULL))
    return;

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Chmod"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area),
                              5);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
#else
  dialog = gtk_dialog_new_with_buttons (_("Chmod"), NULL, 0,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "Chmod", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, gftp_version);
    }

  tempwid = gtk_label_new (_("You can now adjust the attributes of your file(s)\nNote: Not all ftp servers support the chmod feature"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, FALSE,
		      FALSE, 0);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE,
		      0);
  gtk_widget_show (hbox);

  tempwid = gtk_frame_new (_("Special"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  suid = gtk_check_button_new_with_label (_("SUID"));
  gtk_box_pack_start (GTK_BOX (vbox), suid, FALSE, FALSE, 0);
  gtk_widget_show (suid);

  sgid = gtk_check_button_new_with_label (_("SGID"));
  gtk_box_pack_start (GTK_BOX (vbox), sgid, FALSE, FALSE, 0);
  gtk_widget_show (sgid);

  sticky = gtk_check_button_new_with_label (_("Sticky"));
  gtk_box_pack_start (GTK_BOX (vbox), sticky, FALSE, FALSE, 0);
  gtk_widget_show (sticky);

  tempwid = gtk_frame_new (_("User"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  ur = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), ur, FALSE, FALSE, 0);
  gtk_widget_show (ur);

  uw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), uw, FALSE, FALSE, 0);
  gtk_widget_show (uw);

  ux = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ux, FALSE, FALSE, 0);
  gtk_widget_show (ux);

  tempwid = gtk_frame_new (_("Group"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  gr = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), gr, FALSE, FALSE, 0);
  gtk_widget_show (gr);

  gw = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), gw, FALSE, FALSE, 0);
  gtk_widget_show (gw);

  gx = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), gx, FALSE, FALSE, 0);
  gtk_widget_show (gx);

  tempwid = gtk_frame_new (_("Other"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  vbox = gtk_vbox_new (TRUE, 5);
  gtk_container_add (GTK_CONTAINER (tempwid), vbox);
  gtk_widget_show (vbox);

  or = gtk_check_button_new_with_label (_("Read"));
  gtk_box_pack_start (GTK_BOX (vbox), or, FALSE, FALSE, 0);
  gtk_widget_show (or);

  ow = gtk_check_button_new_with_label (_("Write"));
  gtk_box_pack_start (GTK_BOX (vbox), ow, FALSE, FALSE, 0);
  gtk_widget_show (ow);

  ox = gtk_check_button_new_with_label (_("Execute"));
  gtk_box_pack_start (GTK_BOX (vbox), ox, FALSE, FALSE, 0);
  gtk_widget_show (ox);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (dochmod), (gpointer) wdata);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (chmod_action), wdata);
#endif

  if (IS_ONE_SELECTED (wdata))
    {
      filelist = wdata->files;
      templist = gftp_gtk_get_list_selection (wdata);
      num = 0;
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (suid),
                                    tempfle->st_mode & S_ISUID);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ur),
                                    tempfle->st_mode & S_IRUSR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uw),
                                    tempfle->st_mode & S_IWUSR);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ux),
                                    tempfle->st_mode & S_IXUSR);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sgid),
                                    tempfle->st_mode & S_ISGID);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gr),
                                    tempfle->st_mode & S_IRGRP);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gw),
                                    tempfle->st_mode & S_IWGRP);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gx),
                                    tempfle->st_mode & S_IXGRP);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sticky),
                                    tempfle->st_mode & S_ISVTX);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (or),
                                    tempfle->st_mode & S_IROTH);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ow),
                                    tempfle->st_mode & S_IWOTH);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ox),
                                    tempfle->st_mode & S_IXOTH);
    }
  gtk_widget_show (dialog);
}

