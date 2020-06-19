/*****************************************************************************/
/*  gtkui_transfer.c - GTK+ UI transfer related functions for gFTP           */
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

enum
{
   TRANSFER_DLG_COL_FILENAME,
   TRANSFER_DLG_COL_FROM,
   TRANSFER_DLG_COL_TO,
   TRANSFER_DLG_COL_ACTION,
   TRANSFER_DLG_NUM_COLUMNS
};

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
  transdata = g_malloc0 (sizeof (*transdata));
  transdata->transfer = tdata;
  transdata->curfle = curfle;

  gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), fle->user_data, transdata);
}


void
gftpui_cancel_file_transfer (gftp_transfer * tdata)
{
  if (tdata->thread_id != NULL)
    pthread_kill (*(pthread_t *) tdata->thread_id, SIGINT);

  tdata->cancel = 1; /* FIXME */
  tdata->fromreq->cancel = 1;
  tdata->toreq->cancel = 1;
}


static void
gftpui_gtk_trans_selectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->clist));
  gtk_tree_selection_select_all (tsel);
}


static void
gftpui_gtk_trans_unselectall (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;
  tdata = data;
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tdata->clist));
  gtk_tree_selection_unselect_all (tsel);
}


static void
gftpui_gtk_set_action (gftp_transfer * tdata, char * transfer_str,
                       int transfer_action)
{
   GList *filelist = tdata->files;

   g_mutex_lock (&tdata->structmutex);

   GtkTreeView      *tree = GTK_TREE_VIEW  (tdata->clist);

   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   GList         *selrows = gtk_tree_selection_get_selected_rows (tsel, NULL);
   GList               *i = selrows;

   GtkTreeModel    *model = GTK_TREE_MODEL (gtk_tree_view_get_model (tree));
   GtkListStore    *store = GTK_LIST_STORE (model);
   GtkTreeIter       iter;
   GtkTreePath     *tpath = NULL;
   char         *filename = NULL;
   gftp_file    *gftpFile = NULL;

   while (i)
   {
      tpath = (GtkTreePath *) i->data;

      gtk_tree_model_get_iter (model, &iter, tpath);
      gtk_tree_model_get      (model, &iter,
                               TRANSFER_DLG_COL_FILENAME, &filename, -1);

      // find the corresponding gftp_file
      gftpFile = find_gftp_file_by_name (filelist, filename, TRUE); // listbox.c
      if (gftpFile) {
         gftpFile->transfer_action = transfer_action;
         gtk_list_store_set (store, &iter, TRANSFER_DLG_COL_ACTION, transfer_str, -1);
      }

      g_free(filename);
      i = i->next;
   }

   g_list_free_full (selrows, (GDestroyNotify) gtk_tree_path_free);

   g_mutex_unlock (&tdata->structmutex);
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
  g_mutex_lock (&tdata->structmutex);
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

  g_mutex_unlock (&tdata->structmutex);
}


static void
gftpui_gtk_cancel (GtkWidget * widget, gpointer data)
{
  gftp_transfer * tdata;

  tdata = data;
  g_mutex_lock (&tdata->structmutex);
  tdata->show = 0;
  tdata->done = tdata->ready = 1;
  g_mutex_unlock (&tdata->structmutex);
}


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


void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  char *add_data[4] = { NULL, NULL, NULL, NULL };
  char tempstr[50], temp1str[50], *pos;
  GtkWidget * dialog, * tempwid, * scroll, * hbox;
  gftp_file * tempfle;
  GList * templist;
  size_t len;

  dialog = gtk_dialog_new_with_buttons (_("Transfer Files"), NULL, 0, 
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);

  gtk_window_set_wmclass (GTK_WINDOW(dialog), "transfer", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);

  tempwid = gtk_label_new (_("The following file(s) exist on both the local and remote computer\nPlease select what you would like to do"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, FALSE,
		      FALSE, 0);
  gtk_widget_show (tempwid);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 550, 200);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  // ===============================================================

  GtkTreeModel     *tree_model;
  GtkTreeView      *treeview;
  GtkTreeSelection *tree_sel;

  GtkListStore *store;
  store = gtk_list_store_new (TRANSFER_DLG_NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);
  tree_model = GTK_TREE_MODEL (store);

  treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (tree_model));

  /* multiple selection */
  tree_sel = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_set_mode (tree_sel, GTK_SELECTION_MULTIPLE); //GTK_SELECTION_EXTENDED

  gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), TRANSFER_DLG_COL_FILENAME);
  g_object_unref (tree_model);

  /* COLUMNS */
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  // filename
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          _("Filename"),
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_FILENAME,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_FILENAME);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (column, 140);

  // from
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          tdata->fromreq->hostname,
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_FROM,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_FROM);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (column, 140);

  // to
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          tdata->toreq->hostname,
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_TO,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_TO);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (column, 140);

  // action
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          _("Action"),
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_ACTION,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_ACTION);
  gtk_tree_view_append_column (treeview, column);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

  // ===============================================================

  tdata->clist = treeview;
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(tdata->clist));

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scroll, TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (tdata->clist));
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
      GtkTreeIter iter;
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          TRANSFER_DLG_COL_FILENAME, add_data[0],
                          TRANSFER_DLG_COL_FROM,     add_data[1],
                          TRANSFER_DLG_COL_TO,       add_data[2],
                          TRANSFER_DLG_COL_ACTION,   add_data[3],
                          -1);
    }

  gtk_tree_selection_select_all (tree_sel);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Overwrite"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
		      G_CALLBACK (gftpui_gtk_overwrite), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Resume"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
		      G_CALLBACK (gftpui_gtk_resume), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Skip File"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
                      G_CALLBACK (gftpui_gtk_skip), (gpointer) tdata);
  gtk_widget_show (tempwid);

  hbox = gtk_hbox_new (TRUE, 20);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
		      G_CALLBACK (gftpui_gtk_trans_selectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Deselect All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
		      G_CALLBACK (gftpui_gtk_trans_unselectall), (gpointer) tdata);
  gtk_widget_show (tempwid);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gftpui_gtk_transfer_action),(gpointer) tdata);

  gtk_widget_show (dialog);
  dialog = NULL;
}

