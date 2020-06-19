/*****************************************************************************/
/*  listbox.c - GtkTreeView displaying a file list                           */
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
   LISTBOX_NUM_COLUMNS
};

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
                               G_TYPE_UINT);

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
    *  gftp implements its own logic to sort rows
    *  set the "clickable" property to true and call listbox_sort_rows()
    */

   GtkTreeView     *treeview = GTK_TREE_VIEW (wdata->listbox);
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;

   /* icon */
   //renderer = gtk_cell_renderer_pixbuf_new ();
   renderer = g_object_new (GTK_TYPE_CELL_RENDERER_PIXBUF,
                           "xalign", 0.5,     /* justify left */
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
                          "title",          "Filename",
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
                          "title",          "Size",
                          "resizable",      TRUE,
                          "clickable",      TRUE,
                          //"sort-column-id", LISTBOX_COL_SIZE,
                          NULL);
   gtk_tree_view_column_pack_start (column, renderer, FALSE);
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
                          "title",          "Date",
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
                          "title",          "User",
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
                          "title",          "Group",
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
                          "title",          "Atribs",
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
      templist = listbox_get_selected_files (wdata);
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
   gftp_file        *gftpFile;
   GList            *templist;
   int i;
   GtkTreeView      *tree  = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel  = gtk_tree_view_get_selection (tree);
   GtkTreePath      *tpath;

   i = 0;
   templist = wdata->files;
   while (templist != NULL)
   {
      gftpFile = (gftp_file *) templist->data;
      if (gftpFile->shown)
      {
         tpath = gtk_tree_path_new_from_indices (i, -1);
         if (S_ISDIR (gftpFile->st_mode)) {
            gtk_tree_selection_unselect_path (tsel, tpath);
         } else {
            gtk_tree_selection_select_path (tsel, tpath);
         }
         gtk_tree_path_free (tpath);
         i++;
      }
      templist = templist->next;
   }
}

// ==============================================================

/*
 listbox_get_selected_file1()
    retrieve existing gftp fileinfo / does not create a glist..
     https://developer.gnome.org/gtk2/stable/GtkTreeSelection.html#gtk-tree-selection-get-selected-rows
*/

gftp_file *
find_gftp_file_by_name (GList *filelist, char *filename, gboolean filelist_ignore_directory)
{
   //GList   *filelist = wdata->files;
   gftp_file *gftpFile = NULL;
   gftp_file *found    = NULL;
   char *p = NULL;
   while (filelist)
   {
      gftpFile = (gftp_file *) filelist->data;
      if (strcmp (gftpFile->file, filename) == 0) {
         //fprintf(stderr, "gftp: %s\n", gftpFile->file);
         found = gftpFile;
         break;
      } else {
         p = strrchr(gftpFile->file, '/');
         // file transfers
         if (p && filelist_ignore_directory == TRUE) {
            p++;
            if (strcmp (p, filename) == 0) {
               found = gftpFile;
               break;
            }
         }
      }
      filelist = filelist->next;
   }
   return found;
}

static void
selected_1_foreach_func (GtkTreeModel *model,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            gpointer      userdata)
{
   char **filename = (char **) userdata;
   gtk_tree_model_get (model, iter, LISTBOX_COL_FILENAME, filename, -1);
}

gftp_file *
listbox_get_selected_file1 (gftp_window_data *wdata)
{
   // retrieve selected file name from listbox
   char *filename = NULL;
   GtkTreeView      *tree = GTK_TREE_VIEW (wdata->listbox);
   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);

   gtk_tree_selection_selected_foreach(tsel, selected_1_foreach_func, &filename);
   if (!filename) {
      fprintf(stderr, "listbox.c: ERROR, could not retrieve filename...\n");
   }

   // find the corresponding gftp_file
   return (find_gftp_file_by_name (wdata->files, filename, FALSE));
}

/* listbox_get_selected_files() */
// returns a GList that must be freed

GList *
listbox_get_selected_files (gftp_window_data *wdata)
{
   GtkTreeView      *tree = GTK_TREE_VIEW  (wdata->listbox);

   GtkTreeSelection *tsel = gtk_tree_view_get_selection (tree);
   GList         *selrows = gtk_tree_selection_get_selected_rows (tsel, NULL);
   GList               *i = selrows;

   GtkTreeModel    *model = GTK_TREE_MODEL (gtk_tree_view_get_model (tree));
   GtkTreeIter       iter;
   GtkTreePath     *tpath = NULL;
   char         *filename = NULL;

   gftp_file    *gftpFile = NULL;
   GList    *out_filelist = NULL;

   while (i)
   {
      tpath = (GtkTreePath *) i->data;

      gtk_tree_model_get_iter (model, &iter, tpath);
      gtk_tree_model_get      (model, &iter,
                               LISTBOX_COL_FILENAME, &filename, -1);
      //fprintf(stderr, "list: %s\n", filename);

      // find the corresponding gftp_file
      gftpFile = find_gftp_file_by_name (wdata->files, filename, FALSE);
      if (gftpFile) {
         out_filelist = g_list_append(out_filelist, gftpFile);
      }

      g_free(filename);
      i = i->next;
   }

   g_list_free_full (selrows, (GDestroyNotify) gtk_tree_path_free);

   return (out_filelist);
}

// ==============================================================

static void
listbox_set_default_column_width (gftp_window_data *wdata)
{
   // set column width from config
   GtkTreeView *tree = GTK_TREE_VIEW  (wdata->listbox);

   char tempstr[50];
   int colwidth;
   GtkTreeViewColumn *tcol;

   static char *column_str[LISTBOX_NUM_COLUMNS] = {
      "icon", "file", "size", "date", "user", "group", "attribs"
   };

   for (int ncol = 1; ncol < LISTBOX_NUM_COLUMNS; ncol++)
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

