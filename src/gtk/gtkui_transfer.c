/*****************************************************************************/
/*  gtkui_transfer.c - GTK+ UI transfer related functions for gFTP           */
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
trans_selectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  gtk_clist_select_all (GTK_CLIST (tdata->clist));
}


static void
trans_unselectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  gtk_clist_unselect_all (GTK_CLIST (tdata->clist));
}


static void
overwrite (GtkWidget * widget, gpointer data)
{
  GList * templist, * filelist;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int curpos;

  tdata = data;
  curpos = 0;
  filelist = tdata->files;
  templist = GTK_CLIST (tdata->clist)->selection;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &curpos);
      tempfle = filelist->data;
      tempfle->transfer_action = GFTP_TRANS_ACTION_OVERWRITE;
      gtk_clist_set_text (GTK_CLIST (tdata->clist), curpos, 3, _("Overwrite"));
    }
}


static void
resume (GtkWidget * widget, gpointer data)
{
  GList * templist, * filelist;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int curpos;

  tdata = data;
  curpos = 0;
  filelist = tdata->files;
  templist = GTK_CLIST (tdata->clist)->selection;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &curpos);
      tempfle = filelist->data;
      tempfle->transfer_action = GFTP_TRANS_ACTION_RESUME;
      gtk_clist_set_text (GTK_CLIST (tdata->clist), curpos, 3, _("Resume"));
    }
}


static void
skip (GtkWidget * widget, gpointer data)
{
  GList * templist, * filelist;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  int curpos;

  tdata = data;
  curpos = 0;
  filelist = tdata->files;
  templist = GTK_CLIST (tdata->clist)->selection;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &curpos);
      tempfle = filelist->data;
      tempfle->transfer_action = GFTP_TRANS_ACTION_SKIP;
      gtk_clist_set_text (GTK_CLIST (tdata->clist), curpos, 3, _("Skip"));
    }
}


static void
ok (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  gftp_file * tempfle;
  GList * templist;

  tdata = data;
  g_static_mutex_lock (&tdata->structmutex);
  for (templist = tdata->files; templist != NULL; templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
        break;
    }

  if (templist == NULL)
    {
      tdata->show = 0; 
      tdata->ready = tdata->done = 1;
    }
  else
    tdata->show = tdata->ready = 1;
  g_static_mutex_unlock (&tdata->structmutex);
}


static void
cancel (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;

  tdata = data;
  g_static_mutex_lock (&tdata->structmutex);
  tdata->show = 0;
  tdata->done = tdata->ready = 1;
  g_static_mutex_unlock (&tdata->structmutex);
}


#if GTK_MAJOR_VERSION > 1
static void
transfer_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        ok (widget, user_data);
        gtk_widget_destroy (widget);
        break;
      case GTK_RESPONSE_CANCEL:
        cancel (widget, user_data);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}   
#endif


void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  char *dltitles[4], *add_data[4] = { NULL, NULL, NULL, NULL },
       tempstr[50], temp1str[50], *pos;
  GtkWidget * dialog, * tempwid, * scroll, * hbox;
  intptr_t overwrite_default;
  gftp_file * tempfle;
  GList * templist;
  size_t len;
  int i;

  dltitles[0] = _("Filename");
  dltitles[1] = tdata->fromreq->hostname;
  dltitles[2] = tdata->toreq->hostname;
  dltitles[3] = _("Action");

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_grab_add (dialog);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Transfer Files"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 35);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);

  gtk_signal_connect_object (GTK_OBJECT (dialog), "delete_event",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (dialog));
#else
  dialog = gtk_dialog_new_with_buttons (_("Transfer Files"), NULL, 0, 
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "transfer", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);

  tempwid = gtk_label_new (_("The following file(s) exist on both the local and remote computer\nPlease select what you would like to do"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, FALSE,
		      FALSE, 0);
  gtk_widget_show (tempwid);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 450, 200);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  tdata->clist = gtk_clist_new_with_titles (4, dltitles);
  gtk_container_add (GTK_CONTAINER (scroll), tdata->clist);

#if GTK_MAJOR_VERSION == 1
  gtk_clist_set_selection_mode (GTK_CLIST (tdata->clist),
				GTK_SELECTION_EXTENDED);
#else
  gtk_clist_set_selection_mode (GTK_CLIST (tdata->clist),
				GTK_SELECTION_MULTIPLE);
#endif
  gtk_clist_set_column_width (GTK_CLIST (tdata->clist), 0, 100);
  gtk_clist_set_column_justification (GTK_CLIST (tdata->clist), 1,
				      GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_width (GTK_CLIST (tdata->clist), 1, 85);
  gtk_clist_set_column_justification (GTK_CLIST (tdata->clist), 2,
				      GTK_JUSTIFY_RIGHT);
  gtk_clist_set_column_width (GTK_CLIST (tdata->clist), 2, 85);
  gtk_clist_set_column_width (GTK_CLIST (tdata->clist), 3, 85);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scroll, TRUE, TRUE,
		      0);
  gtk_widget_show (tdata->clist);
  gtk_widget_show (scroll);

  gftp_lookup_request_option (tdata->fromreq, "overwrite_default",
                              &overwrite_default);

  for (templist = tdata->files; templist != NULL; 
       templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->startsize == 0 || tempfle->isdir)
        {
           tempfle->shown = 0;
           continue;
        }
      tempfle->shown = 1;

      pos = tempfle->destfile;
      len = strlen (tdata->toreq->directory);
      if (strncmp (pos, tdata->toreq->directory, len) == 0)
        pos = tempfle->destfile + len + 1;
      add_data[0] = pos;

      if (overwrite_default)
        {
          add_data[3] = _("Overwrite");
          tempfle->transfer_action = GFTP_TRANS_ACTION_OVERWRITE;
        }
      else if (tempfle->startsize == tempfle->size)
        {
          add_data[3] = _("Skip");
          tempfle->transfer_action = GFTP_TRANS_ACTION_SKIP;
        }
      else if (tempfle->startsize > tempfle->size)
        {
          add_data[3] = _("Overwrite");
          tempfle->transfer_action = GFTP_TRANS_ACTION_OVERWRITE;
        }
      else
        {
          add_data[3] = _("Resume");
          tempfle->transfer_action = GFTP_TRANS_ACTION_RESUME;
        }

      add_data[1] = insert_commas (tempfle->size, tempstr, sizeof (tempstr));
      add_data[2] = insert_commas (tempfle->startsize, temp1str,
                                   sizeof (temp1str));

      i = gtk_clist_append (GTK_CLIST (tdata->clist), add_data);
      gtk_clist_set_row_data (GTK_CLIST (tdata->clist), i, tempfle);
    }

  gtk_clist_select_all (GTK_CLIST (tdata->clist));

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Overwrite"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (overwrite), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Resume"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (resume), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Skip File"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked", GTK_SIGNAL_FUNC (skip),
		      (gpointer) tdata);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (trans_selectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Deselect All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (trans_unselectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked", GTK_SIGNAL_FUNC (ok),
		      (gpointer) tdata);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (cancel), (gpointer) tdata);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (transfer_action), (gpointer) tdata);
#endif

  gtk_widget_show (dialog);
  dialog = NULL;
}

