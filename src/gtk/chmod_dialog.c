/*****************************************************************************/
/*  chmod_dialog.c - the chmod dialog box                                    */
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

static void dochmod 				( GtkWidget * widget, 
						  gftp_window_data * wdata );
static void *do_chmod_thread 			( void * data );

static GtkWidget *suid, *sgid, *sticky, *ur, *uw, *ux, *gr, *gw, *gx, *or, *ow,
                 *ox;
static int mode; 

#if !(GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2)
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
  if (!check_status (_("Chmod"), wdata, wdata->request->use_threads, 1, 1, 
                     wdata->request->chmod != NULL))
    return;

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
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
      gdk_window_set_icon_name (dialog->window, _("gFTP Icon"));
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

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
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
      templist = GTK_CLIST (wdata->listbox)->selection;
      num = 0;
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (suid),
				    tolower (tempfle->attribs[3]) == 's');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sgid),
				    tolower (tempfle->attribs[6]) == 's');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sticky),
				    tolower (tempfle->attribs[9]) == 't');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ur),
				    tempfle->attribs[1] == 'r');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (uw),
				    tempfle->attribs[2] == 'w');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ux),
				    tempfle->attribs[3] == 'x' ||
                                    tempfle->attribs[3] == 's');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gr),
				    tempfle->attribs[4] == 'r');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gw),
				    tempfle->attribs[5] == 'w');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gx),
				    tempfle->attribs[6] == 'x' ||
                                    tempfle->attribs[6] == 's');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (or),
				    tempfle->attribs[7] == 'r');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ow),
				    tempfle->attribs[8] == 'w');
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ox),
				    tempfle->attribs[9] == 'x' ||
                                    tempfle->attribs[9] == 't');
    }
  gtk_widget_show (dialog);
}


static void
dochmod (GtkWidget * widget, gftp_window_data * wdata)
{
  int cur;

  mode = 0;
  if (GTK_TOGGLE_BUTTON (suid)->active)
    mode += 4;
  if (GTK_TOGGLE_BUTTON (sgid)->active)
    mode += 2;
  if (GTK_TOGGLE_BUTTON (sticky)->active)
    mode += 1;

  cur = 0;
  if (GTK_TOGGLE_BUTTON (ur)->active)
    cur += 4;
  if (GTK_TOGGLE_BUTTON (uw)->active)
    cur += 2;
  if (GTK_TOGGLE_BUTTON (ux)->active)
    cur += 1;
  mode = mode * 10 + cur;

  cur = 0;
  if (GTK_TOGGLE_BUTTON (gr)->active)
    cur += 4;
  if (GTK_TOGGLE_BUTTON (gw)->active)
    cur += 2;
  if (GTK_TOGGLE_BUTTON (gx)->active)
    cur += 1;
  mode = mode * 10 + cur;

  cur = 0;
  if (GTK_TOGGLE_BUTTON (or)->active)
    cur += 4;
  if (GTK_TOGGLE_BUTTON (ow)->active)
    cur += 2;
  if (GTK_TOGGLE_BUTTON (ox)->active)
    cur += 1;
  mode = mode * 10 + cur;

  if (check_reconnect (wdata) < 0)
    return;

   if ((int) generic_thread (do_chmod_thread, wdata))
    refresh (wdata);
}


static void *
do_chmod_thread (void * data)
{
  GList * filelist, * templist;
  gftp_window_data * wdata;
  int success, num, sj;
  gftp_file * tempfle;

  wdata = data;
  wdata->request->user_data = (void *) 0x01;

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
      filelist = wdata->files;
      templist = GTK_CLIST (wdata->listbox)->selection;
      num = 0;
      while (templist != NULL)
        {
          templist = get_next_selection (templist, &filelist, &num);
          tempfle = filelist->data;
          if (wdata->request->network_timeout > 0)
            alarm (wdata->request->network_timeout);
          if (gftp_chmod (wdata->request, tempfle->file, mode) == 0)
            success = 1;
          if (!GFTP_IS_CONNECTED (wdata->request))
            break;
        }
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

  wdata->request->user_data = NULL;
  wdata->request->stopable = 0;
  return ((void *) success);
}

