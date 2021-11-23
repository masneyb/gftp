/***********************************************************************************/
/*  gtkui_transfer.c - GTK+ UI transfer related functions for gFTP                 */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                        */
/*                                                                                 */
/*  Permission is hereby granted, free of charge, to any person obtaining a copy   */
/*  of this software and associated documentation files (the "Software"), to deal  */
/*  in the Software without restriction, including without limitation the rights   */
/*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      */
/*  copies of the Software, and to permit persons to whom the Software is          */
/*  furnished to do so, subject to the following conditions:                       */
/*                                                                                 */
/*  The above copyright notice and this permission notice shall be included in all */
/*  copies or substantial portions of the Software.                                */
/*                                                                                 */
/*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     */
/*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       */
/*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    */
/*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         */
/*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  */
/*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  */
/*  SOFTWARE.                                                                      */
/***********************************************************************************/

#include "gftp-gtk.h"

enum
{
   TRANSFER_DLG_COL_FILENAME,
   TRANSFER_DLG_COL_FROM,
   TRANSFER_DLG_COL_TO,
   TRANSFER_DLG_COL_ACTION,
   TRANSFER_DLG_COL_GFTPFILE, // hidden
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
  DEBUG_PRINT_FUNC
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
  DEBUG_PRINT_FUNC
  if (tdata->thread_id != NULL)
    pthread_kill (*(pthread_t *) tdata->thread_id, SIGINT);

  tdata->cancel = 1; /* FIXME */
  tdata->fromreq->cancel = 1;
  tdata->toreq->cancel = 1;
}

/* ==================================================================== */
/*                           GTK callbacks                              */
/* ==================================================================== */

static void
on_gtk_button_clicked_selectall (GtkButton *button, gpointer data)
{
  DEBUG_PRINT_FUNC
  GtkTreeView *treeview  = GTK_TREE_VIEW (data);
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_select_all (tsel);
}

static void
on_gtk_button_clicked_unselectall (GtkButton *button, gpointer data)
{
  DEBUG_PRINT_FUNC
  GtkTreeView *treeview  = GTK_TREE_VIEW (data);
  GtkTreeSelection *tsel = gtk_tree_view_get_selection (treeview);
  gtk_tree_selection_unselect_all (tsel);
}


static void
gftp_gtk_set_transfer_action (gftp_transfer * tdata, char * transfer_str,
                              int transfer_action)
{
   DEBUG_PRINT_FUNC
   Wg_mutex_lock (&tdata->structmutex);

   GtkTreeView      *tree = GTK_TREE_VIEW (tdata->clist);
   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   GtkTreeModel    *model = gtk_tree_view_get_model (tree);
   GtkListStore    *store = GTK_LIST_STORE (model);
   GtkTreeIter iter;
   gboolean   valid;
   gftp_file * gftpFile = NULL;

   valid = gtk_tree_model_get_iter_first (model, &iter);

   while (valid)
   {
      if (gtk_tree_selection_iter_is_selected (tsel, &iter)) {
         gtk_tree_model_get (model, &iter,  TRANSFER_DLG_COL_GFTPFILE, &gftpFile,  -1);
         if (gftpFile) {
            gftpFile->transfer_action = transfer_action;
            // change transfer action text in TreeView
            gtk_list_store_set (store, &iter,  TRANSFER_DLG_COL_ACTION, transfer_str,  -1);
         }
      }
      valid = gtk_tree_model_iter_next (model, &iter);
   }

   Wg_mutex_unlock (&tdata->structmutex);
}

static void
on_gtk_button_clicked_overwrite (GtkButton *button, gpointer data)
{
  DEBUG_PRINT_FUNC
  gftp_gtk_set_transfer_action (data, _("Overwrite"), GFTP_TRANS_ACTION_OVERWRITE);
}

static void
on_gtk_button_clicked_resume (GtkButton *button, gpointer data)
{
  DEBUG_PRINT_FUNC
  gftp_gtk_set_transfer_action (data, _("Resume"), GFTP_TRANS_ACTION_RESUME);
}

static void
on_gtk_button_clicked_skip (GtkButton *button, gpointer data)
{
  DEBUG_PRINT_FUNC
  gftp_gtk_set_transfer_action (data, _("Skip"), GFTP_TRANS_ACTION_SKIP);
}


static void
on_gtk_dialog_response_transferdlg (GtkDialog *dialog,
                                    gint response,
                                    gpointer user_data)
{
  DEBUG_PRINT_FUNC
  gftp_transfer * tdata = (gftp_transfer *) user_data;

  /* click on OK */
  if (response == GTK_RESPONSE_OK)
  {
     gftp_file * tempfle;
     GList * templist;

     Wg_mutex_lock (&tdata->structmutex);
     for (templist = tdata->files; templist != NULL; templist = templist->next)
     {
        tempfle = templist->data;
        if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
           break;
     }

     tdata->ready = 1;
     if (templist == NULL) {
        tdata->show = 0; 
        tdata->done = 1;
     } else {
        tdata->show = 1;
     }
     Wg_mutex_unlock (&tdata->structmutex);

     gtk_widget_destroy (GTK_WIDGET (dialog));
     return;
  }

  /* cancel transfers */
  Wg_mutex_lock (&tdata->structmutex);
  tdata->show = 0;
  tdata->done = tdata->ready = 1;
  Wg_mutex_unlock (&tdata->structmutex);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* ==================================================================== */
/*                      'ASK TRANSFER' DIALOG                           */
/* ==================================================================== */

void
gftpui_ask_transfer (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  char *add_data[4] = { NULL, NULL, NULL, NULL };
  char tempstr[50], temp1str[50], *pos;
  GtkWidget * dialog, * tempwid, * scroll, * hbox, *main_vbox;
  gftp_file * tempfle;
  GList * templist;
  size_t len;

  dialog = gtk_dialog_new_with_buttons (_("Transfer Files"), NULL, 0, 
                                        "gtk-cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "gtk-ok",
                                        GTK_RESPONSE_OK,
                                        NULL);

  gtk_window_set_role (GTK_WINDOW(dialog), "transfer");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);

  tempwid = gtk_label_new (_("The following file(s) exist on both the local and remote computer\nPlease select what you would like to do"));
  gtk_box_pack_start (GTK_BOX (main_vbox), tempwid, FALSE, FALSE, 0);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scroll, 600, 200);

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
                              G_TYPE_STRING,
                              G_TYPE_POINTER);
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
                         "sizing",         GTK_TREE_VIEW_COLUMN_FIXED,
                         "fixed-width",    190,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_FILENAME);
  gtk_tree_view_append_column (treeview, column);

  // from
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 1.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          tdata->fromreq->hostname,
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_FROM,
                         "sizing",         GTK_TREE_VIEW_COLUMN_FIXED,
                         "fixed-width",    140,
                         "alignment",      1.0, /* header: right alignment */
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_FROM);
  gtk_tree_view_append_column (treeview, column);

  // to
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 1.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          tdata->toreq->hostname,
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_TO,
                         "sizing",         GTK_TREE_VIEW_COLUMN_FIXED,
                         "fixed-width",    140,
                         "alignment",      1.0, /* header: right alignment */
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_TO);
  gtk_tree_view_append_column (treeview, column);

  // action
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "title",          _("Action"),
                         "resizable",      TRUE,
                         "clickable",      TRUE,
                         "sort-column-id", TRANSFER_DLG_COL_ACTION,
                         "sizing",         GTK_TREE_VIEW_COLUMN_AUTOSIZE,
                         NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", TRANSFER_DLG_COL_ACTION);
  gtk_tree_view_append_column (treeview, column);

  // ===============================================================

  tdata->clist = treeview;
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET(tdata->clist));

  gtk_box_pack_start (GTK_BOX (main_vbox), scroll, TRUE, TRUE, 0);

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
                          TRANSFER_DLG_COL_GFTPFILE, tempfle,
                          -1);
    }

  gtk_tree_selection_select_all (tree_sel);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

  tempwid = gtk_button_new_with_label (_("Overwrite"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), // GtkButton
                    "clicked",          // signal
                    G_CALLBACK (on_gtk_button_clicked_overwrite),
                    (gpointer) tdata);

  tempwid = gtk_button_new_with_label (_("Resume"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), // GtkButton
                    "clicked",          // signal
                    G_CALLBACK (on_gtk_button_clicked_resume),
                    (gpointer) tdata);

  tempwid = gtk_button_new_with_label (_("Skip File"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), // GtkButton
                    "clicked",          // signal
                    G_CALLBACK (on_gtk_button_clicked_skip),
                    (gpointer) tdata);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

  tempwid = gtk_button_new_with_label (_("Select All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), // GtkButton
                    "clicked",          // signal
                    G_CALLBACK (on_gtk_button_clicked_selectall),
                    (gpointer) treeview);

  tempwid = gtk_button_new_with_label (_("Deselect All"));
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), // GtkButton
                    "clicked",          // signal
                    G_CALLBACK (on_gtk_button_clicked_unselectall),
                    (gpointer) treeview);

  g_signal_connect (G_OBJECT (dialog), // GtkDialog
                    "response",        // signal
                    G_CALLBACK (on_gtk_dialog_response_transferdlg),
                    (gpointer) tdata);

  gtk_widget_show_all (dialog);
  dialog = NULL;
}

