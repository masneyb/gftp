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


void
gftpui_start_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_update_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_finish_current_file_in_transfer (gftp_transfer * tdata)
{
}


void
gftpui_start_transfer (gftp_transfer * tdata)
{
  /* Not used in GTK+ port. This is polled instead */
}


void
gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle)
{
  gftpui_common_curtrans_data * transdata;
  char *text[2];
  gftp_file * fle;

  fle = curfle->data;
  text[0] = gftpui_gtk_get_utf8_file_pos (fle);

  if (fle->transfer_action == GFTP_TRANS_ACTION_SKIP)
    text[1] = _("Skipped");
  else
    text[1] = _("Waiting...");

  fle->user_data = gtk_ctree_insert_node (GTK_CTREE (dlwdw),
                                          tdata->user_data, NULL, text, 5,
                                          NULL, NULL, NULL, NULL,
                                          FALSE, FALSE);
  transdata = g_malloc (sizeof (*transdata));
  transdata->transfer = tdata;
  transdata->curfle = curfle;

  gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), fle->user_data, transdata);
}


static void
gftpui_gtk_trans_selectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  gtk_clist_select_all (GTK_CLIST (tdata->clist));
}


static void
gftpui_gtk_trans_unselectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;

  gtk_clist_unselect_all (GTK_CLIST (tdata->clist));
}


static void
gftpui_gtk_set_action (gftp_transfer * tdata, char * transfer_str,
                       int transfer_action)
{
  GList * templist, * filelist;
  gftp_file * tempfle;
  int curpos;

  g_static_mutex_lock (&tdata->structmutex);

  curpos = 0;
  filelist = tdata->files;
  templist = GTK_CLIST (tdata->clist)->selection;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &curpos);
      tempfle = filelist->data;
      tempfle->transfer_action = transfer_action;
      gtk_clist_set_text (GTK_CLIST (tdata->clist), curpos, 3, transfer_str);
    }

  g_static_mutex_unlock (&tdata->structmutex);
}


static void
gftpui_gtk_overwrite (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, _("Overwrite"), GFTP_TRANS_ACTION_OVERWRITE);
}


static void
gftpui_gtk_resume (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, _("Resume"), GFTP_TRANS_ACTION_RESUME);
}


static void
gftpui_gtk_skip (GtkWidget * widget, gpointer data)
{
  gftpui_gtk_set_action (data, _("Skip"), GFTP_TRANS_ACTION_SKIP);
}


static void
gftpui_gtk_ok (GtkWidget * widget, gpointer data)
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

  tdata->ready = 1;
  if (templist == NULL)
    {
      tdata->show = 0; 
      tdata->done = 1;
    }
  else
    tdata->show = 1;

  g_static_mutex_unlock (&tdata->structmutex);
}


static void
gftpui_gtk_cancel (GtkWidget * widget, gpointer data)
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
gftpui_gtk_transfer_action (GtkWidget * widget, gint response,
                            gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        gftpui_gtk_ok (widget, user_data);
        gtk_widget_destroy (widget);
        break;
      case GTK_RESPONSE_CANCEL:
        gftpui_gtk_cancel (widget, user_data);
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

  for (templist = tdata->files; templist != NULL; 
       templist = templist->next)
    {
      tempfle = templist->data;
      if (tempfle->startsize == 0 || S_ISDIR (tempfle->st_mode))
        {
           tempfle->shown = 0;
           continue;
        }
      tempfle->shown = 1;

      len = strlen (tdata->toreq->directory);
      pos = tempfle->destfile;
      if (len == 1 && (*tdata->toreq->directory) == '/')
        pos++;
      if (strncmp (pos, tdata->toreq->directory, len) == 0)
        pos += len + 1;

      add_data[0] = pos;

      gftp_get_transfer_action (tdata->fromreq, tempfle);
      switch (tempfle->transfer_action)
        {
          case GFTP_TRANS_ACTION_OVERWRITE:
            add_data[3] = _("Overwrite");
            break;
          case GFTP_TRANS_ACTION_SKIP:
            add_data[3] = _("Skip");
            break;
          case GFTP_TRANS_ACTION_RESUME:
            add_data[3] = _("Resume");
            break;
          default:
            add_data[3] = _("Error");
            break;
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
		      GTK_SIGNAL_FUNC (gftpui_gtk_overwrite), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Resume"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (gftpui_gtk_resume), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Skip File"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (gftpui_gtk_skip), (gpointer) tdata);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (gftpui_gtk_trans_selectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Deselect All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (gftpui_gtk_trans_unselectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (gftpui_gtk_ok), (gpointer) tdata);
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
		      GTK_SIGNAL_FUNC (gftpui_gtk_cancel), (gpointer) tdata);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (gftpui_gtk_transfer_action),(gpointer) tdata);
#endif

  gtk_widget_show (dialog);
  dialog = NULL;
}

