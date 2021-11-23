/***********************************************************************************/
/*  listbox.c - GtkTreeView displaying a file list                                 */
/*  Copyright (C) 2020-2021 wdlkmpx <wdlkmpx@gmail.com>                            */
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

/*
  https://www.kksou.com/php-gtk2/sample-codes/process-multiple-selections-in-GtkTreeView.php
  https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Events
  https://en.wikibooks.org/wiki/GTK%2B_By_Example/Tree_View/Tree_Models#Retrieving_Row_Data

  - A GtkTreeview with GTK_SELECTION_MULTIPLE 
  - GtkTreeSelection is used to get/set selected rows..
*/

#include "gftp-gtk.h"

typedef struct
{
   GdkPixbuf *icon;
   gchar *filename;
   gchar *size;
   gchar *date;
   gchar *user;
   gchar *group;
   gchar *attribs;
   gpointer gftp_file;
} listbox_columns;

enum
{
   LISTBOX_COL_ICON,
   LISTBOX_COL_FILENAME,
   LISTBOX_COL_SIZE,
   LISTBOX_COL_DATE,
   LISTBOX_COL_USER,
   LISTBOX_COL_GROUP,
   LISTBOX_COL_ATTRIBS,
   LISTBOX_COL_GFTPFILE, // hidden
   LISTBOX_NUM_COLUMNS
};

#define LB_NUM_VISIBLE_COLUMNS (LISTBOX_NUM_COLUMNS - 1)

/* ============================================================== *
 * create_listbox()
 * ============================================================== */

static GtkWidget *
create_listbox(gftp_window_data *wdata) {

   GtkTreeModel     *tree_model;
   GtkTreeView      *treeview;
   GtkTreeSelection *tree_sel;

   /* model defines data types and number of "columns" */
   // see gtk_list_store_set
   GtkListStore *store;
   store = gtk_list_store_new (LISTBOX_NUM_COLUMNS,
                               GDK_TYPE_PIXBUF,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_POINTER);

   tree_model = GTK_TREE_MODEL (store);

   //-------------------------------------------------------------------
   treeview = GTK_TREE_VIEW (gtk_tree_view_new_with_model (tree_model));
   //-------------------------------------------------------------------

   /* multiple selection */
   tree_sel = gtk_tree_view_get_selection (treeview);
   gtk_tree_selection_set_mode (tree_sel, GTK_SELECTION_MULTIPLE); //GTK_SELECTION_EXTENDED

   gtk_tree_view_set_search_column (GTK_TREE_VIEW (treeview), LISTBOX_COL_FILENAME);
   g_object_unref (tree_model);

   gtk_widget_show (GTK_WIDGET (treeview));

   return (GTK_WIDGET(treeview));
}

/* ============================================================== *
 * listbox_add_columns()
 * ============================================================== */

static void on_treeview_column_0_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 0);
}
static void on_treeview_column_1_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 1);
}
static void on_treeview_column_2_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 2);
}
static void on_treeview_column_3_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 3);
}
static void on_treeview_column_4_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 4);
}
static void on_treeview_column_5_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 5);
}
static void on_treeview_column_6_clicked_cb (GtkTreeViewColumn *c, gpointer wdata) {
   listbox_sort_rows (wdata, 6);
}

static void
listbox_add_columns (gftp_window_data *wdata)
{
   /*
    *  gftp implements its own logic to sort rows (lib/misc.c)
    *      that is called by listbox_sort_rows()
    *  set the "clickable" property to true and call listbox_sort_rows()
    *  and don't set the "sort-column-id" property
    */

   GtkTreeView     *treeview = GTK_TREE_VIEW (wdata->listbox);
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   /* icon */
   //renderer = gtk_cell_renderer_pixbuf_new ();
   renderer = g_object_new (GTK_TYPE_CELL_RENDERER_PIXBUF,
                           "xalign", 0.5,     /* justify center */
                           NULL);
   column = gtk_tree_view_column_new_with_attributes ("",
                                                      renderer,
                                                      "pixbuf",
                                                      LISTBOX_COL_ICON,
                                                      NULL);
   gtk_tree_view_column_set_clickable (column, TRUE);
   gtk_tree_view_column_set_resizable (column, FALSE);
   gtk_tree_view_column_set_alignment (column, 0.5);
   //gtk_tree_view_column_set_sort_column_id (column, LISTBOX_COL_FILENAME);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_0_clicked_cb), wdata);

   /* filename */
   //renderer = gtk_cell_renderer_text_new ();
   renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 0.0,     /* justify left */
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "title",          _("Filename"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_FILENAME,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_FILENAME);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_1_clicked_cb), wdata);

   /* size */
   renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 1.0,     /* justify right */
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "alignment",      1.0,    /* header: right alignment */
                          "title",          _("Size"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_SIZE,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, TRUE); // TRUE: required for `xalign=1.0`
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_SIZE);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_2_clicked_cb), wdata);

   /* date */
   renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 0.0,
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "title",          _("Date"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_DATE,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_DATE);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_3_clicked_cb), wdata);

   /* user */
   renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 0.0,
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "title",          _("User"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_USER,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_USER);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_4_clicked_cb), wdata);

   /* group */
   renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 0.0,
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "title",          _("Group"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_GROUP,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_GROUP);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_5_clicked_cb), wdata);

   /* attributes */
   renderer = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
                           "xalign", 0.0,
                           NULL);
   column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                          "title",          _("Attribs"),
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_ATTRIBS,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
   gtk_tree_view_column_add_attribute (column, renderer,
                                       "text", LISTBOX_COL_ATTRIBS);
   gtk_tree_view_append_column (treeview, column);
   g_signal_connect (G_OBJECT (column), "clicked",
                     G_CALLBACK (on_treeview_column_6_clicked_cb), wdata);
}

/* ============================================================== *
 * listbox_sort_rows()
 * ============================================================== */

void
listbox_sort_rows (gpointer data, gint column)
{
   /* gftp_sort_filelist() does the job, then listbox_update_filelist()
    * populates the listbox with the new info from wdata->files */

   //fprintf(stderr, "listbox_sort_rows\n");
   char sortcol_name[25];
   char sortasds_name[25];
   gftp_window_data * wdata = data;
   intptr_t sortcol, sortasds;
   GtkWidget * icon0;
   int swap_col;
   GtkTreeView *listbox = GTK_TREE_VIEW (wdata->listbox);

   g_snprintf (sortcol_name, sizeof (sortcol_name), "%s_sortcol",
              wdata->prefix_col_str);
   gftp_lookup_global_option (sortcol_name, &sortcol);
   g_snprintf (sortasds_name, sizeof (sortasds_name), "%s_sortasds",
              wdata->prefix_col_str);
   gftp_lookup_global_option (sortasds_name, &sortasds);

   if (column == -1)
      column = sortcol;

   if (column == 0 || (column == sortcol && wdata->sorted)) {
      sortasds = !sortasds;
      gftp_set_global_option (sortasds_name, GINT_TO_POINTER (sortasds));
      swap_col = 1;
   } else {
      swap_col = 0;
   }

   if (swap_col || !wdata->sorted)
   {
      icon0 = gtk_tree_view_column_get_widget (gtk_tree_view_get_column (listbox,0));
      if (sortasds) {
         icon0 = gtk_image_new_from_icon_name ("view-sort-ascending",
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);
      } else {
         icon0 = gtk_image_new_from_icon_name ("view-sort-descending",
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);
      }
      gtk_tree_view_column_set_widget (gtk_tree_view_get_column (listbox,0), icon0);
      gtk_widget_show (icon0);
   }
   else {
      sortcol = column;
      gftp_set_global_option (sortcol_name, GINT_TO_POINTER (sortcol));
   }

   if (!GFTP_IS_CONNECTED (wdata->request))
      return;

   wdata->files = gftp_sort_filelist (wdata->files, sortcol, sortasds);

   listbox_update_filelist(wdata);

   wdata->sorted = 1;
}

/* ============================================================== *
 *     listbox_add_file() + listbox_update_filelist()
 * ============================================================== */ 

static void
listbox_add_file (gftp_window_data * wdata, gftp_file * fle)
{
   char time_str[80] = "";
   struct tm timeinfo;
   gftp_config_list_vars *tmplistvar;
   gftp_file_extensions  *tempext;
   GList                 *templist;
   size_t stlen;
   int empty_size = 0;

   GtkTreeView  *tree  = GTK_TREE_VIEW(wdata->listbox);
   GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
   GtkTreeIter  iter;

   listbox_columns col_data;
   memset(&col_data, 0, sizeof(col_data));
   col_data.gftp_file = (gpointer) fle;

   if (wdata->show_selected) {
      fle->shown = fle->was_sel;
      if (!fle->shown) {
         return;
      }
   } else if (!gftp_match_filespec (wdata->request, fle->file, wdata->filespec)) {
      fle->shown = 0;
      fle->was_sel = 0;
      return;
   } else {
      fle->shown = 1;
   }

   gtk_list_store_append (store, &iter);

   if (strcmp (fle->file, "..") == 0) {
      col_data.icon = gftp_get_pixbuf("dotdot.xpm");
      empty_size = 1;
   } else if (S_ISLNK (fle->st_mode) && S_ISDIR (fle->st_mode)) {
      col_data.icon = gftp_get_pixbuf("linkdir.xpm");
      empty_size = 1;
   } else if (S_ISLNK (fle->st_mode)) {
      col_data.icon = gftp_get_pixbuf("linkfile.xpm");
   } else if (S_ISDIR (fle->st_mode)) {
      col_data.icon = gftp_get_pixbuf("dir.xpm");
      empty_size = 1;
   } else if ((fle->st_mode & S_IXUSR) ||
           (fle->st_mode & S_IXGRP) ||
           (fle->st_mode & S_IXOTH)) {
      col_data.icon = gftp_get_pixbuf("exe.xpm");
   } else {
      stlen = strlen (fle->file);
      gftp_lookup_global_option ("ext", &tmplistvar);
      templist = tmplistvar->list;
      while (templist != NULL)
      {
         tempext = templist->data;
         if (stlen >= tempext->stlen &&
             strcmp (&fle->file[stlen - tempext->stlen], tempext->ext) == 0)
         {
            col_data.icon = gftp_get_pixbuf(tempext->filename);
            break;
         }
         templist = templist->next;
      }
   }

   if (!col_data.icon) {
      col_data.icon = gftp_get_pixbuf ("doc.xpm");
   }

   if (fle->file) {
      col_data.filename = fle->file;
   }

   if (empty_size) {
      col_data.size = NULL;
   }
   else {
      if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode)) {
         col_data.size = g_strdup_printf ("%d, %d", major (fle->size),
                               minor (fle->size));
      }else{
         col_data.size = insert_commas (fle->size, NULL, 0);
      }
   }

   if (localtime_r( &fle->datetime, &timeinfo )) {
      strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S ", &timeinfo);
      char *zeroseconds = strstr(time_str, ":00 ");
      if (zeroseconds) *zeroseconds = 0;
      col_data.date = time_str;
   }

   if (fle->user) {
      col_data.user = fle->user;
   }
   if (fle->group) {
      col_data.group = fle->group;
   }

   col_data.attribs = gftp_convert_attributes_from_mode_t (fle->st_mode);

   gtk_list_store_set (store, &iter,
                       LISTBOX_COL_ICON,     col_data.icon,
                       LISTBOX_COL_FILENAME, col_data.filename,
                       LISTBOX_COL_SIZE,     col_data.size,
                       LISTBOX_COL_DATE,     col_data.date,
                       LISTBOX_COL_USER,     col_data.user,
                       LISTBOX_COL_GROUP,    col_data.group,
                       LISTBOX_COL_ATTRIBS,  col_data.attribs,
                       LISTBOX_COL_GFTPFILE, col_data.gftp_file,
                      -1);

   if (col_data.size)    g_free (col_data.size);
   if (col_data.attribs) g_free (col_data.attribs);
}

void
listbox_update_filelist(gftp_window_data * wdata)
{
   // use wdata->files to populate the listbox
   GList     *templist, *igl;
   gftp_file *gftpFile;
   int        only_selected = 0;

   if (wdata->show_selected == 1 && listbox_num_selected(wdata) > 0) {
      only_selected = 1;
   }

   if (only_selected) {
      // mark files that were selected - required by "Show selected"
      //fprintf(stderr, "show_selected\n");
      templist = (GList *) listbox_get_selected_files (wdata, 0);
      for (igl = templist; igl != NULL; igl = igl->next)
      {
         gftpFile = (gftp_file *) igl->data;
         if (strcmp (gftpFile->file, "..") == 0) {
             continue;
         }
         gftpFile->was_sel = 1;
      }
      g_list_free (templist);
   }

   listbox_clear(wdata);

   // fill listbox again
   templist = wdata->files; 
   while (templist)
   {
      gftpFile = (gftp_file *) templist->data;
      listbox_add_file (wdata, gftpFile);
      templist = templist->next;
   }

   if (only_selected) {
      listbox_select_all (wdata);
   }
}

// ==============================================================

int
listbox_num_selected (gftp_window_data *wdata) {
   GtkTreeView      *tree = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   gint             count = gtk_tree_selection_count_selected_rows (tsel);
   return count;
}

void listbox_clear (gftp_window_data *wdata) {
   GtkTreeView  *tree  = GTK_TREE_VIEW  (wdata->listbox);
   GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
   gtk_list_store_clear (store);
}

void
listbox_select_all (gftp_window_data *wdata) {
   GtkTreeView      *tree = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   gtk_tree_selection_select_all (tsel);
}

void
listbox_deselect_all (gftp_window_data *wdata) {
   GtkTreeView      *tree = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   gtk_tree_selection_unselect_all (tsel);
}

void
listbox_select_row (gftp_window_data * wdata, int n) {
   //printf("selected_row: %d\n", n);
   GtkTreeView      *tree  = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel  = gtk_tree_view_get_selection (tree);
   GtkTreePath      *tpath = gtk_tree_path_new_from_indices (n, -1);
   gtk_tree_selection_select_path (tsel, tpath);
   gtk_tree_path_free (tpath);
}

static void
listbox_select_all_files (gftp_window_data *wdata) {
   gftp_file       * gftpFile;
   GtkTreeView     * tree  = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeModel    * model = gtk_tree_view_get_model (tree);
   GtkTreeSelection * tsel = gtk_tree_view_get_selection (tree);
   GtkTreeIter iter;
   gboolean    valid;

   valid = gtk_tree_model_get_iter_first (model, &iter);
   while (valid)
   {
      gtk_tree_model_get (model, &iter, LISTBOX_COL_GFTPFILE, &gftpFile, -1);
      if (gftpFile && gftpFile->shown)
      {
         if (S_ISDIR (gftpFile->st_mode)) {
            gtk_tree_selection_unselect_iter (tsel, &iter);
         } else {
            gtk_tree_selection_select_iter (tsel, &iter);
         }
      }
      valid = gtk_tree_model_iter_next (model, &iter);
   }
}

// ==============================================================

/* listbox_get_selected_files() */
// - only_one = 0: returns a (GList *) where item->data = (gftp_file *) [must be freed]
// - only_one = 1: returns a (gftp_file *)
// it may return NULL if no row is selected

void *
listbox_get_selected_files (gftp_window_data *wdata, int only_one)
{
   GtkTreeView      * tree = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection * tsel = gtk_tree_view_get_selection (tree);
   GtkTreeModel    * model = gtk_tree_view_get_model (tree);
   GtkTreeIter iter;
   gboolean   valid;
   gftp_file * gftpFile = NULL;
   GList * out_filelist = NULL;

   valid = gtk_tree_model_get_iter_first (model, &iter);

   while (valid)
   {
      if (gtk_tree_selection_iter_is_selected (tsel, &iter))
      {
         gtk_tree_model_get (model, &iter,  LISTBOX_COL_GFTPFILE, &gftpFile,  -1);
         ///fprintf(stderr, "list: %s\n", gftpFile->file);
         if (only_one) {
            return ((void *) gftpFile); /* only one */
         }
         if (gftpFile) {
            out_filelist = g_list_append (out_filelist, gftpFile);
         }
      }
      valid = gtk_tree_model_iter_next (model, &iter);
   }

   if (!out_filelist) {
      fprintf(stderr, "listbox.c: ERROR, could not retrieve filename(s)...\n"); //debug
   }

   return ((void *) out_filelist);
}

// ==============================================================

static void
listbox_set_default_column_width (gftp_window_data *wdata)
{
   // set column width from config
   GtkTreeView *tree = GTK_TREE_VIEW  (wdata->listbox);

   char tempstr[50];
   intptr_t colwidth;
   GtkTreeViewColumn *tcol;

   static char *column_str[LB_NUM_VISIBLE_COLUMNS] = {
      "icon", "file", "size", "date", "user", "group", "attribs"
   };

#ifndef __APPLE__
   int ncol;
   for (ncol = 1; ncol < LB_NUM_VISIBLE_COLUMNS; ncol++)
   {
      // e.g: local_file_width / remote_file_width
      g_snprintf (tempstr, sizeof (tempstr),
                  "%s_%s_width",
                  wdata->prefix_col_str, column_str[ncol]);
      gftp_lookup_global_option (tempstr, &colwidth);

      tcol = gtk_tree_view_get_column (tree, ncol);

      // width >  0: set width
      // width =  0: auto size
      // width = -1: hide column
      if (colwidth > 0) {
         gtk_tree_view_column_set_sizing      (tcol, GTK_TREE_VIEW_COLUMN_FIXED);
         //gtk_tree_view_column_set_min_width (tcol, colwidth);
         gtk_tree_view_column_set_fixed_width (tcol, colwidth);
      }
      else if (colwidth == 0) {
         gtk_tree_view_column_set_sizing (tcol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      }
      else if (colwidth == -1) {
         // make column invisible
         gtk_tree_view_column_set_visible (tcol, FALSE);
      }
   }
#endif
}

// ==============================================================

static void
lb_save_cwidth(GtkTreeView *tree, int ncol, char *opt)
{
   GtkTreeViewColumn *col;
   intptr_t w;

   col = gtk_tree_view_get_column (tree, ncol);
   w   = gtk_tree_view_column_get_width (col);

   if (!gtk_tree_view_column_get_visible (col)) {
      // only save width if column is visible
      return;
   }
   if (gtk_tree_view_column_get_sizing (col) == GTK_TREE_VIEW_COLUMN_AUTOSIZE) {
      return;
   }
   gftp_set_global_option (opt, GINT_TO_POINTER (w));
}

static void
listbox_save_column_width (gftp_window_data *local, gftp_window_data *remote)
{
   GtkTreeView *ltree = GTK_TREE_VIEW (local->listbox);
   GtkTreeView *rtree = GTK_TREE_VIEW (remote->listbox);

   lb_save_cwidth (ltree, LISTBOX_COL_FILENAME, "local_file_width");
   lb_save_cwidth (ltree, LISTBOX_COL_SIZE,     "local_size_width");
   lb_save_cwidth (ltree, LISTBOX_COL_DATE,     "local_date_width");
   lb_save_cwidth (ltree, LISTBOX_COL_USER,     "local_user_width");
   lb_save_cwidth (ltree, LISTBOX_COL_GROUP,    "local_group_width");
   lb_save_cwidth (ltree, LISTBOX_COL_ATTRIBS,  "local_attribs_width");

   lb_save_cwidth (rtree, LISTBOX_COL_FILENAME, "remote_file_width");
   lb_save_cwidth (rtree, LISTBOX_COL_SIZE,     "remote_size_width");
   lb_save_cwidth (rtree, LISTBOX_COL_DATE,     "remote_date_width");
   lb_save_cwidth (rtree, LISTBOX_COL_USER,     "remote_user_width");
   lb_save_cwidth (rtree, LISTBOX_COL_GROUP,    "remote_group_width");
   lb_save_cwidth (rtree, LISTBOX_COL_ATTRIBS,  "remote_attribs_width");
}

