/***********************************************************************************/
/*  transfer.c - functions to handle transfering files                             */
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

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "gftp-gtk.h"

static int num_transfers_in_progress = 0;

struct transfer_status
{
    int percent;
    char percent_str[10];
    char text[150];
};


#if defined(TRANSFER_GTK_TREEVIEW)
static GdkPixbuf *up_pixbuf = NULL;
static GdkPixbuf *down_pixbuf = NULL;

struct transfer_tview_t
{
    GtkTreeView *tv;
    GtkTreeModel *model;
    GtkListStore *store;
    GtkTreeSelection *sel;
    GtkTreeIter seliter;
};

struct transfer_tview_t transfer_tview;

enum
{
  TTREE_COL_ICON,          	/* col 0 */
  TTREE_COL_FILENAME,      	/*     0 */
  TTREE_COL_PROGRESS,      	/* col 1 */
  TTREE_COL_PROGRESS_TEXT, 	/*     1 */
  TTREE_COL_PROGRESS_INFO, 	/* col 2 */
  TTREE_COL_REMOTE_PATH,   	/* col 3 */
  TTREE_COL_TRANSFER_DATA, 	/* hidden, pointer to transfer data */
  TTREE_NUM_COLS
};
#endif

GtkWidget *  transfer_list_create (void)
{
  GtkWidget * transfer_list;

#if !defined(TRANSFER_GTK_TREEVIEW)
  char *dltitles[2];
  intptr_t tmplookup;
  dltitles[0] = _("Filename");
  dltitles[1] = _("Progress");
  transfer_list = gtk_ctree_new_with_titles (2, 0, dltitles);
  gtk_clist_set_selection_mode (GTK_CLIST (transfer_list), GTK_SELECTION_SINGLE);
  gtk_clist_set_reorderable (GTK_CLIST (transfer_list), 0);

  gftp_lookup_global_option ("file_trans_column", &tmplookup);
  if (tmplookup == 0)
    gtk_clist_set_column_auto_resize (GTK_CLIST (transfer_list), 0, TRUE);
  else
    gtk_clist_set_column_width (GTK_CLIST (transfer_list), 0, tmplookup);
#else
  GtkTreeViewColumn *col;
  GtkCellRenderer   *renderer;
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default();

  //up_pixbuf = gftp_get_pixbuf ("go-up.png");
  //down_pixbuf = gftp_get_pixbuf ("go-down.png");

  if (!up_pixbuf) up_pixbuf = gtk_icon_theme_load_icon (icon_theme,"go-up",16,0, NULL);
  if (!up_pixbuf) up_pixbuf = gtk_icon_theme_load_icon (icon_theme,"gtk-go-up",16,0, NULL);

  if (!down_pixbuf) down_pixbuf = gtk_icon_theme_load_icon (icon_theme,"go-down",16,0, NULL);
  if (!down_pixbuf) down_pixbuf = gtk_icon_theme_load_icon (icon_theme,"gtk-go-down",16,0, NULL);

  //intptr_t colwidth;

  /* tree model */
  transfer_tview.store = gtk_list_store_new (TTREE_NUM_COLS,
                              GDK_TYPE_PIXBUF, /* icon */
                              G_TYPE_STRING,   /* text */
                              G_TYPE_INT,      /* progress */
                              G_TYPE_STRING,   /* progress text */
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER); /* hidden: pointer to bookmark */
  /* tree view */
  transfer_tview.model = GTK_TREE_MODEL(transfer_tview.store);

  transfer_list = g_object_new (GTK_TYPE_TREE_VIEW,
                       "model",             transfer_tview.model,
                       "show-expanders",    FALSE,
                       "headers-visible",   TRUE,
                       "reorderable",       FALSE,
                       "enable-search",     FALSE,
                       NULL, NULL);
  transfer_tview.tv = GTK_TREE_VIEW(transfer_list);
  g_object_unref (transfer_tview.model);

  transfer_tview.sel = gtk_tree_view_get_selection (transfer_tview.tv);
  gtk_tree_selection_set_mode (transfer_tview.sel, GTK_SELECTION_SINGLE);

  /* COLUMNS */

  /* col0 */
  col = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                      "title",       _("Filename"),
                      "resizable",   TRUE,
                      "clickable",   TRUE,
                      "sizing",      GTK_TREE_VIEW_COLUMN_FIXED,
                      "fixed-width", 250,
                       NULL);
  /* col0 -> icon (#0) */
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_PIXBUF, "xalign", 0.5, NULL);
  gtk_tree_view_column_pack_start(col, renderer, FALSE);
  gtk_tree_view_column_set_attributes(col, renderer,
                                      "pixbuf", TTREE_COL_ICON, NULL);
  /* col0 -> filename (#1) */
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_attributes (col, renderer,
                                      "text", TTREE_COL_FILENAME, NULL);
  gtk_tree_view_append_column (transfer_tview.tv, col);

  /* col1 -> progress (#2) / progress_text (#3) */
  col = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                      "title",     _("Progress"),
                      "resizable", TRUE,
                      "clickable", TRUE,
                      "sizing",      GTK_TREE_VIEW_COLUMN_FIXED,
                      "fixed-width", 100,
                      NULL);
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_PROGRESS, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_attributes (col, renderer,
                                      "value", TTREE_COL_PROGRESS,
                                      "text",  TTREE_COL_PROGRESS_TEXT,
                                      NULL);
  gtk_tree_view_append_column (transfer_tview.tv, col);

  /* col2 -> progress_info (#4) */
  col = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                      //"title",     _("Progress info"),
                      "resizable", TRUE,
                      "clickable", TRUE,
                      "sizing",      GTK_TREE_VIEW_COLUMN_FIXED,
                      "fixed-width", 310,
                      NULL);
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_attributes (col, renderer,
                                      "text", TTREE_COL_PROGRESS_INFO, NULL);
  gtk_tree_view_append_column (transfer_tview.tv, col);

  /* col3 -> remote (#5) */
  col = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                      "title",     _("Remote path"),
                      "resizable", TRUE,
                      "clickable", TRUE,
                      NULL);
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "xalign", 0.0, NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_attributes (col, renderer,
                                      "text", TTREE_COL_REMOTE_PATH, NULL);
  gtk_tree_view_append_column (transfer_tview.tv, col);
#endif
  return transfer_list;
}


void  transfer_list_save_column_width (void)
{
  printf("transfer_list_save_column_width: Stub for now\n");
}


#ifdef TRANSFER_GTK_TREEVIEW
void transfer_tview_append (gftpui_common_curtrans_data *transdata,
                            gftp_file *tempfle,
                            char *filename, char *tinfo)
{
  GtkTreeIter iter;
  GdkPixbuf *pixbuf;
  if (transdata->transfer->fromreq->protonum == GFTP_PROTOCOL_LOCALFS) {
      pixbuf = up_pixbuf;
  } else {
      pixbuf = down_pixbuf;
  }
  gtk_list_store_append (transfer_tview.store, &iter);
  gtk_list_store_set (transfer_tview.store, &iter,
                      TTREE_COL_ICON,          pixbuf,
                      TTREE_COL_FILENAME,      filename,
                      TTREE_COL_PROGRESS_INFO, tinfo,
                      TTREE_COL_REMOTE_PATH,   transdata->transfer->fromreq->hostname,
                      TTREE_COL_TRANSFER_DATA, transdata,
                      -1);
  tempfle->user_data = calloc(1, sizeof(iter));
  tempfle->free_user_data = 1;
  memcpy (tempfle->user_data, &iter, sizeof(iter));
}
#endif


gftpui_common_curtrans_data *
transfer_list_get_selected_transfer_data (int log)
{
  gftpui_common_curtrans_data * transfer_data = NULL;

#if !defined(TRANSFER_GTK_TREEVIEW)
  GtkCTreeNode * node = NULL;
  if (GTK_CLIST (dlwdw)->selection)
  {
     node = GTK_CLIST (dlwdw)->selection->data;
     transfer_data = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);
  }
#else
  if (gtk_tree_selection_get_selected (transfer_tview.sel, NULL, &transfer_tview.seliter))
  {
     gtk_tree_model_get (transfer_tview.model, &transfer_tview.seliter,
                         TTREE_COL_TRANSFER_DATA, &transfer_data,  -1);
  }
#endif
  if (!transfer_data && log) {
      ftp_log (gftp_logging_error, NULL, _("There are no file transfers selected\n"));
  }
  return (transfer_data);
}


gftpui_common_curtrans_data *
transfer_list_get_transfer_data (gpointer user_data)
{
  gftpui_common_curtrans_data * transfer_data = NULL;
  if (!user_data) {
      return NULL;
  }
#if !defined(TRANSFER_GTK_TREEVIEW)
  GtkCTreeNode * node;
  node = (GtkCTreeNode *) user_data;
  transfer_data = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);
#else
  GtkTreeIter * iter;
  iter = (GtkTreeIter *) user_data;
  gtk_tree_model_get (transfer_tview.model, iter,
                      TTREE_COL_TRANSFER_DATA, &transfer_data,  -1);
#endif
  return transfer_data;
}


int gftp_gtk_list_files (gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata;

  gtk_label_set_text (GTK_LABEL (wdata->dirinfo_label), _("Receiving file names..."));

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_ls;
  cdata->dont_refresh = 1;

  if(gftpui_common_run_callback_function (cdata) == GFTP_ECANIGNORE)
  {
     g_free(cdata);
     update_window(wdata);
     return (1);
  }

  wdata->files = cdata->files;
  g_free (cdata);
  
  if (wdata->files == NULL || !GFTP_IS_CONNECTED (wdata->request))
    {
      gftpui_disconnect (wdata);
      return (0);
    }

  wdata->sorted = 0;
  listbox_sort_rows ((gpointer) wdata, -1);

  return (1);
}


int gftp_gtk_connect (gftp_window_data * wdata, gftp_request * request)
{
  DEBUG_PRINT_FUNC
  if (wdata->request == request)
    gtk_label_set_text (GTK_LABEL (wdata->dirinfo_label), _("Connecting..."));

  return (gftpui_common_cmd_open (wdata, request, NULL, NULL, NULL));
}


void  get_files (gpointer data)
{
  DEBUG_PRINT_FUNC
  transfer_window_files (&window2, &window1);
}


void put_files (gpointer data)
{
  DEBUG_PRINT_FUNC
  transfer_window_files (&window1, &window2);
}


void transfer_window_files (gftp_window_data * fromwdata, gftp_window_data * towdata)
{
  DEBUG_PRINT_FUNC
  gftp_file * tempfle, * newfle;
  GList * templist, * igl;
  gftp_transfer * transfer;
  int ret, disconnect;

  if (!check_status (_("Transfer Files"), fromwdata, 1, 0, 1,
       towdata->request->put_file != NULL && fromwdata->request->get_file != NULL))
    return;

  if (!GFTP_IS_CONNECTED (fromwdata->request) || 
      !GFTP_IS_CONNECTED (towdata->request))
    {
      ftp_log (gftp_logging_error, NULL,
               _("Retrieve Files: Not connected to a remote site\n"));
      return;
    }

  if (check_reconnect (fromwdata) < 0 || check_reconnect (towdata) < 0)
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (fromwdata->request);
  transfer->toreq = gftp_copy_request (towdata->request);
  transfer->fromwdata = fromwdata;
  transfer->towdata = towdata;

  templist = (GList *) listbox_get_selected_files (fromwdata, 0);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
     tempfle = (gftp_file *) igl->data;
     if (strcmp (tempfle->file, "..") == 0) //||
         //strcmp (tempfle->file, ".") == 0)
            continue;
     newfle = copy_fdata (tempfle);
     transfer->files = g_list_append (transfer->files, newfle);
  }
  g_list_free (templist);

  if (transfer->files != NULL)
    {
      gftp_swap_socks (transfer->fromreq, fromwdata->request);
      gftp_swap_socks (transfer->toreq, towdata->request);

      ret = gftp_gtk_get_subdirs (transfer);
      if (ret < 0)
        disconnect = 1;
      else
        disconnect = 0;

      if (!GFTP_IS_CONNECTED (transfer->fromreq))
        {
          gftpui_disconnect (fromwdata);
          disconnect = 1;
        } 

      if (!GFTP_IS_CONNECTED (transfer->toreq))
        {
          gftpui_disconnect (towdata);
          disconnect = 1;
        } 

      if (disconnect)
        {
          free_tdata (transfer);
          return;
        }

      gftp_swap_socks (fromwdata->request, transfer->fromreq);
      gftp_swap_socks (towdata->request, transfer->toreq);
    }

  if (transfer->files != NULL)
    {
      gftpui_common_add_file_transfer (transfer->fromreq, transfer->toreq, 
                                       transfer->fromwdata, transfer->towdata, 
                                       transfer->files);
      g_free (transfer);
    }
  else
    free_tdata (transfer);
}


static int
gftpui_gtk_tdata_connect (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  gftp_transfer * tdata;
  int ret;

  tdata = cdata->user_data;

  if (tdata->fromreq != NULL)
    {
      ret = gftp_connect (tdata->fromreq);
      if (ret < 0)
        return (ret);
    }

  if (tdata->toreq != NULL)
    {
      ret = gftp_connect (tdata->toreq);
      if (ret < 0)
        return (ret);
    }

  return (0);
}


static void
gftpui_gtk_tdata_disconnect (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  gftp_transfer * tdata;

  tdata = cdata->user_data;

  if (tdata->fromreq != NULL)
    gftp_disconnect (tdata->fromreq);

  if (tdata->toreq != NULL)
    gftp_disconnect (tdata->toreq);

  cdata->request->datafd = -1;
}


static int
_gftp_getdir_thread (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  return (gftp_get_all_subdirs (cdata->user_data, NULL));
}

static int
progress_timeout(gpointer data)
{ // required by gftp_gtk_get_subdirs()...
  return (1);
}


int gftp_gtk_get_subdirs (gftp_transfer * transfer)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata; 
  long numfiles, numdirs;
  guint timeout_num;
  int ret;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->user_data = transfer;
  cdata->uidata = transfer->fromwdata;
  cdata->request = ((gftp_window_data *) transfer->fromwdata)->request;
  cdata->run_function = _gftp_getdir_thread;
  cdata->connect_function = gftpui_gtk_tdata_connect;
  cdata->disconnect_function = gftpui_gtk_tdata_disconnect;
  cdata->dont_check_connection = 1;
  cdata->dont_refresh = 1;

  timeout_num = g_timeout_add (100, progress_timeout, transfer);
  ret = gftpui_common_run_callback_function (cdata);
  g_source_remove (timeout_num);

  numfiles = transfer->numfiles;
  numdirs = transfer->numdirs;
  transfer->numfiles = transfer->numdirs = -1; 
  transfer->numfiles = numfiles;
  transfer->numdirs = numdirs;

  g_free (cdata);

  return (ret);
}


static void remove_file (gftp_viewedit_data * ve_proc)
{
  DEBUG_PRINT_FUNC
  if (ve_proc->remote_filename == NULL)
    return;

  if (unlink (ve_proc->filename) == 0)
    ftp_log (gftp_logging_misc, NULL, _("Successfully removed %s\n"),
             ve_proc->filename);
  else
    ftp_log (gftp_logging_error, NULL,
             _("Error: Could not remove file %s: %s\n"), ve_proc->filename,
             g_strerror (errno));
}


static void free_edit_data (gftp_viewedit_data * ve_proc)
{
  DEBUG_PRINT_FUNC
  int i;

  if (ve_proc->torequest)
    gftp_request_destroy (ve_proc->torequest, 1);
  if (ve_proc->filename)
    g_free (ve_proc->filename);
  if (ve_proc->remote_filename)
    g_free (ve_proc->remote_filename);
  for (i = 0; ve_proc->argv[i] != NULL; i++)
    g_free (ve_proc->argv[i]);
  g_free (ve_proc->argv);
  g_free (ve_proc);
}


static void
dont_upload (gftp_viewedit_data * ve_proc, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  remove_file (ve_proc);
  free_edit_data (ve_proc);
}


static void
do_upload (gftp_viewedit_data * ve_proc, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftp_transfer * tdata;
  gftp_file * tempfle;
  GList * newfile;

  tempfle = g_malloc0 (sizeof (*tempfle));
  tempfle->destfile = gftp_build_path (ve_proc->torequest,
                                       ve_proc->torequest->directory,
                                       ve_proc->remote_filename, NULL);
  ve_proc->remote_filename = NULL;
  tempfle->file = ve_proc->filename;
  ve_proc->filename = NULL;
  tempfle->done_rm = 1;
  newfile = g_list_append (NULL, tempfle);
  tdata = gftpui_common_add_file_transfer (ve_proc->fromwdata->request,
                                           ve_proc->torequest,
                                           ve_proc->fromwdata,
                                           ve_proc->towdata, newfile);
  free_edit_data (ve_proc);

  if (tdata != NULL)
    tdata->conn_error_no_timeout = 1;
}


static int
_check_viewedit_process_status (gftp_viewedit_data * ve_proc, int ret)
{
  DEBUG_PRINT_FUNC
  int procret;

  if (WIFEXITED (ret))
    {
      procret = WEXITSTATUS (ret);
      if (procret != 0)
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Error: Child %d returned %d\n"), ve_proc->pid, procret);
          if (ve_proc->view)
            remove_file (ve_proc);

          return (0);
        }
      else
        {
          ftp_log (gftp_logging_misc, NULL,
                   _("Child %d returned successfully\n"), ve_proc->pid);
          return (1);
        }
    }
  else
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Child %d did not terminate properly\n"),
               ve_proc->pid);
      return (0);
    }
}


static int
_prompt_to_upload_edited_file (gftp_viewedit_data * ve_proc)
{
  DEBUG_PRINT_FUNC
  struct stat st;
  char *str;

  if (stat (ve_proc->filename, &st) == -1)
    {
      ftp_log (gftp_logging_error, NULL,
               _("Error: Cannot get information about file %s: %s\n"),
               ve_proc->filename, g_strerror (errno));
      return (0);
    }
  else if (st.st_mtime == ve_proc->st.st_mtime)
    {
      ftp_log (gftp_logging_misc, NULL, _("File %s was not changed\n"),
               ve_proc->filename);
      remove_file (ve_proc);
      return (0);
    }
  else
    {
      memcpy (&ve_proc->st, &st, sizeof (ve_proc->st));
      str = g_strdup_printf (_("File %s has changed.\nWould you like to upload it?"),
                             ve_proc->remote_filename);

      YesNoDialog (NULL, _("Edit File"), str,
                   do_upload,   ve_proc,
                   dont_upload, ve_proc);
      g_free (str);
      return (1);
    }
}


static void
do_check_done_process (pid_t pid, int ret)
{
  DEBUG_PRINT_FUNC
  gftp_viewedit_data * ve_proc;
  GList * curdata, *deldata;
  int ok;

  curdata = viewedit_processes;
  while (curdata != NULL)
    {
      ve_proc = curdata->data;
      if (ve_proc->pid != pid)
        continue;

      deldata = curdata;
      curdata = curdata->next;

      viewedit_processes = g_list_remove_link (viewedit_processes, 
                                               deldata);

      ok = _check_viewedit_process_status (ve_proc, ret);
      if (!ve_proc->view && ve_proc->dontupload)
        gftpui_refresh (ve_proc->fromwdata, 1);

      if (ok && !ve_proc->view && !ve_proc->dontupload)
        {
          /* We were editing the file. Upload it */
          if (_prompt_to_upload_edited_file (ve_proc))
            break; /* Don't free the ve_proc structure */
        }
      else if (ve_proc->view && ve_proc->rm)
        {
          /* After viewing the file delete the tmp file */
          remove_file (ve_proc);
        }

      free_edit_data (ve_proc);
      break;
    }
}


static void
check_done_process (void)
{
  DEBUG_PRINT_FUNC
  pid_t pid;
  int ret;

  gftpui_common_child_process_done = 0;
  while ((pid = waitpid (-1, &ret, WNOHANG)) > 0)
    {
      do_check_done_process (pid, ret);
    }
}


static void  on_next_transfer (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  intptr_t refresh_files;
  gftp_file * tempfle;
  char *transfer_txt = NULL;

  tdata->next_file = 0;
  for (; tdata->updfle != tdata->curfle; tdata->updfle = tdata->updfle->next)
  {
      tempfle = tdata->updfle->data;

      if (tempfle->done_view || tempfle->done_edit)
      {
          if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP)
            {
              view_file (tempfle->destfile, 0, tempfle->done_view,
                         tempfle->done_rm, 1, 0, tempfle->file, NULL);
            }
      } else if (tempfle->done_rm) {
        tdata->fromreq->rmfile (tdata->fromreq, tempfle->file);
      }
      if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP) {
          transfer_txt = _("Skipped");
      } else {
          transfer_txt = _("Finished");
      }
#if !defined(TRANSFER_GTK_TREEVIEW)
      gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tempfle->user_data, 1, transfer_txt);
#else
      gtk_list_store_set (transfer_tview.store, (GtkTreeIter *)tempfle->user_data,
                          TTREE_COL_PROGRESS_INFO, transfer_txt, -1);
      if (tempfle->transfer_action != GFTP_TRANS_ACTION_SKIP) {
          // finished
          gtk_list_store_set (transfer_tview.store, (GtkTreeIter *)tempfle->user_data,
                              TTREE_COL_PROGRESS, 100, -1);
      }
#endif
  }

  gftp_lookup_request_option (tdata->fromreq, "refresh_files", &refresh_files);

  if (refresh_files && tdata->curfle && tdata->curfle->next &&
      compare_request (tdata->toreq, 
                       ((gftp_window_data *) tdata->towdata)->request, 1))
    gftpui_refresh (tdata->towdata, 1);
}


static void
get_trans_password (gftp_request * request, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftp_set_password (request, ddata->entry_text);
  request->stopable = 0;
}


static void
cancel_get_trans_password (gftp_transfer * tdata, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  if (tdata->fromreq->stopable == 0)
    return;

  gftpui_common_cancel_file_transfer (tdata);
}


static void  show_transfer (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;
  gftp_file * tempfle;
  GList * templist;
  char *text[2];
#if !defined(TRANSFER_GTK_TREEVIEW)
  GdkPixmap * closedir_pixmap, * opendir_pixmap;
  GdkBitmap * closedir_bitmap, * opendir_bitmap;

  gftp_get_pixmap (dlwdw, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
  gftp_get_pixmap (dlwdw, "dir.xpm", &closedir_pixmap, &closedir_bitmap);
#endif

  text[0] = tdata->fromreq->hostname;
  text[1] = _("Waiting...");

  transdata = g_malloc0 (sizeof (*transdata));
  transdata->transfer = tdata;
  transdata->curfle = NULL;

#if !defined(TRANSFER_GTK_TREEVIEW)
  tdata->user_data = gtk_ctree_insert_node (GTK_CTREE (dlwdw), NULL, NULL,
                                       text, 5,
                                       closedir_pixmap, closedir_bitmap, 
                                       opendir_pixmap, opendir_bitmap, 
                                       FALSE, 
                                       tdata->numdirs + tdata->numfiles < 50);
  gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), tdata->user_data, transdata);
#endif

  tdata->show = 0;
  tdata->curfle = tdata->updfle = tdata->files;

  tdata->total_bytes = 0;
  for (templist = tdata->files; templist != NULL; templist = templist->next)
  {
      tempfle = templist->data;

      text[0] = gftpui_gtk_get_utf8_file_pos (tempfle);
      if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP) {
        text[1] = _("Skipped");
      } else {
          tdata->total_bytes += tempfle->size;
          text[1] = _("Waiting...");
      }

      transdata = g_malloc0 (sizeof (*transdata));
      transdata->transfer = tdata;
      transdata->curfle = templist;

#if !defined(TRANSFER_GTK_TREEVIEW)
      tempfle->user_data = gtk_ctree_insert_node (GTK_CTREE (dlwdw), 
                                             tdata->user_data, 
                                             NULL, text, 5, NULL, NULL, NULL, 
                                             NULL, FALSE, FALSE);
      gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), tempfle->user_data, transdata);
#else
      transfer_tview_append (transdata, tempfle, text[0], text[1]);
#endif
  }

  if (!tdata->toreq->stopable && gftp_need_password (tdata->toreq))
    {
      tdata->toreq->stopable = 1;
      TextEntryDialog (NULL, _("Enter Password"),
                       _("Please enter your password for this site"), NULL, 0,
                       NULL, gftp_dialog_button_connect, 
                       get_trans_password, tdata->toreq,
                       cancel_get_trans_password, tdata);
    }

  if (!tdata->fromreq->stopable && gftp_need_password (tdata->fromreq))
    {
      tdata->fromreq->stopable = 1;
      TextEntryDialog (NULL, _("Enter Password"),
                       _("Please enter your password for this site"), NULL, 0,
                       NULL, gftp_dialog_button_connect, 
                       get_trans_password, tdata->fromreq,
                       cancel_get_trans_password, tdata);
    }
}


static void  transfer_done (GList * node)
{
  // this is called after all transfers are finished
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;
  gftp_transfer * tdata;
  gftp_file * tempfle;
  GList * templist;

  tdata = node->data;
  if (tdata->started)
  {
      if (GFTP_IS_SAME_HOST_STOP_TRANS ((gftp_window_data *) tdata->fromwdata,
                                         tdata->fromreq))
      {
          gftp_copy_param_options (((gftp_window_data *) tdata->fromwdata)->request, tdata->fromreq);

          gftp_swap_socks (((gftp_window_data *) tdata->fromwdata)->request, 
                           tdata->fromreq);
      }
      else
        gftp_disconnect (tdata->fromreq);

      if (GFTP_IS_SAME_HOST_STOP_TRANS ((gftp_window_data *) tdata->towdata,
                                         tdata->toreq))
      {
          gftp_copy_param_options (((gftp_window_data *) tdata->towdata)->request, tdata->toreq);

          gftp_swap_socks (((gftp_window_data *) tdata->towdata)->request, 
                           tdata->toreq);
      }
      else
          gftp_disconnect (tdata->toreq);

      if (tdata->towdata != NULL && compare_request (tdata->toreq,
                           ((gftp_window_data *) tdata->towdata)->request, 1))
          gftpui_refresh (tdata->towdata, 1);

      num_transfers_in_progress--;
  }

  if ((!tdata->show && tdata->started) ||
      (tdata->done && !tdata->started))
  {
      transdata = transfer_list_get_transfer_data (tdata->user_data);
      if (transdata != NULL) {
          g_free (transdata);
      }
      for (templist = tdata->files; templist != NULL; templist = templist->next)
      {
          tempfle = templist->data;
          transdata = transfer_list_get_transfer_data (tempfle->user_data);
          if (transdata != NULL) {
               g_free (transdata);
          }
#ifdef TRANSFER_GTK_TREEVIEW
          if (tempfle->user_data) {
              g_free (tempfle->user_data);
              tempfle->user_data = NULL;
          }
#endif
      }
#if !defined(TRANSFER_GTK_TREEVIEW)
      gtk_ctree_remove_node (GTK_CTREE (dlwdw), tdata->user_data);
#else
      //gtk_list_store_remove (transfer_tview.store, (GtkTreeIter *) tdata->user_data);
      gtk_list_store_clear (transfer_tview.store);
#endif
  }

  g_mutex_lock (&gftpui_common_transfer_mutex);
  gftp_file_transfers = g_list_remove_link (gftp_file_transfers, node);
  g_mutex_unlock (&gftpui_common_transfer_mutex);

  gdk_window_set_title (gtk_widget_get_parent_window (GTK_WIDGET(dlwdw)),
                        gftp_version);

  free_tdata (tdata);
}


static void *
_gftpui_transfer_files (void *data)
{
  DEBUG_PRINT_FUNC
  int ret;
  pthread_detach (pthread_self ());
  ret = gftpui_common_transfer_files (data);
  return (GINT_TO_POINTER(ret));
}


static void  create_transfer (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  if (tdata->fromreq->stopable)
    return;

  if (GFTP_IS_SAME_HOST_START_TRANS ((gftp_window_data *) tdata->fromwdata,
                                     tdata->fromreq))
    {
      gftp_swap_socks (tdata->fromreq, 
                       ((gftp_window_data *) tdata->fromwdata)->request);
      update_window (tdata->fromwdata);
    }

  if (GFTP_IS_SAME_HOST_START_TRANS ((gftp_window_data *) tdata->towdata,
                                     tdata->toreq))
    {
      gftp_swap_socks (tdata->toreq, 
                       ((gftp_window_data *) tdata->towdata)->request);
      update_window (tdata->towdata);
    }

  num_transfers_in_progress++;
  tdata->started = 1;
  tdata->stalled = 1;
#if !defined(TRANSFER_GTK_TREEVIEW)
  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tdata->user_data, 1,
                           _("Connecting..."));
#else
  /// This is for the status bar?
  //gtk_list_store_set (transfer_tview.store, (GtkTreeIter *)tdata->user_data,
  //                    TTREE_COL_PROGRESS_INFO, _("Connecting..."), -1);
#endif
  if (tdata->thread_id == NULL)
    tdata->thread_id = g_malloc0 (sizeof (pthread_t));

  if (pthread_create (tdata->thread_id, NULL, _gftpui_transfer_files, tdata) != 0)
    perror("pthread_create failed");
}


static void _setup_dlstr (gftp_transfer * tdata, gftp_file * fle,
                          struct transfer_status *tstatus)
{
  DEBUG_PRINT_FUNC
  int hours, mins, secs, stalled, usesentdescr;
  unsigned long remaining_secs, lkbs;
  char gotstr[50], ofstr[50];
  struct timeval tv;
  char *dlstr = tstatus->text;
  size_t dlstr_len = sizeof(tstatus->text);

  stalled = 1;
  gettimeofday (&tv, NULL);
  usesentdescr = (tdata->fromreq->protonum == GFTP_PROTOCOL_LOCALFS);

  insert_commas (fle->size, ofstr, sizeof (ofstr));
  insert_commas (tdata->curtrans + tdata->curresumed, gotstr, sizeof (gotstr));

  tstatus->percent = (int) ((double) (tdata->curtrans + tdata->curresumed) / (double) fle->size * 100.0);

  if (tv.tv_sec - tdata->lasttime.tv_sec <= 5)
    {
      remaining_secs = (fle->size - tdata->curtrans - tdata->curresumed) / 1024;

      lkbs = (unsigned long) tdata->kbs;
      if (lkbs > 0)
        remaining_secs /= lkbs;

      hours = remaining_secs / 3600;
      remaining_secs -= hours * 3600;
      mins = remaining_secs / 60;
      remaining_secs -= mins * 60;
      secs = remaining_secs;

      if (!(hours < 0 || mins < 0 || secs < 0))
        {
          stalled = 0;
          if (usesentdescr)
            {
              g_snprintf (dlstr, dlstr_len,
                          _("Sent %s of %s at %.2fKB/s, %02d:%02d:%02d est. time remaining"), gotstr, ofstr, tdata->kbs, hours, mins, secs);
            }
          else
            {
              g_snprintf (dlstr, dlstr_len,
                          _("Recv %s of %s at %.2fKB/s, %02d:%02d:%02d est. time remaining"), gotstr, ofstr, tdata->kbs, hours, mins, secs);
            }
        }
    }

  if (stalled)
    {
      tdata->stalled = 1;
      if (usesentdescr)
        {
          g_snprintf (dlstr, dlstr_len,
                      _("Sent %s of %s, transfer stalled, unknown time remaining"),
                      gotstr, ofstr);
        }
      else
        {
          g_snprintf (dlstr, dlstr_len,
                      _("Recv %s of %s, transfer stalled, unknown time remaining"),
                      gotstr, ofstr);
        }
    }
}


static void  update_file_status (gftp_transfer * tdata)
{
  DEBUG_PRINT_FUNC
  char totstr[150], winstr[150];
  struct transfer_status tstatus;
  unsigned long remaining_secs, lkbs;
  int hours, mins, secs, pcent;
  intptr_t show_trans_in_title;
  gftp_file * tempfle;

  tstatus.percent = 0;
  tstatus.percent_str[0] = '\0';
  tstatus.text[0] = '\0';

  g_mutex_lock (&tdata->statmutex);
  tempfle = tdata->curfle->data;

  remaining_secs = (tdata->total_bytes - tdata->trans_bytes - tdata->resumed_bytes) / 1024;

  lkbs = (unsigned long) tdata->kbs;
  if (lkbs > 0)
    remaining_secs /= lkbs;

  hours = remaining_secs / 3600;
  remaining_secs -= hours * 3600;
  mins = remaining_secs / 60;
  remaining_secs -= mins * 60;
  secs = remaining_secs;

  if (hours < 0 || mins < 0 || secs < 0)
    {
      g_mutex_unlock (&tdata->statmutex);
      return;
    }

  if ((double) tdata->total_bytes > 0)
    pcent = (int) ((double) (tdata->trans_bytes + tdata->resumed_bytes) / (double) tdata->total_bytes * 100.0);
  else
    pcent = 0;

  if (pcent > 100)
    g_snprintf (totstr, sizeof (totstr),
        _("Unknown percentage complete. (File %ld of %ld)"),
        tdata->current_file_number, tdata->numdirs + tdata->numfiles);
  else
    g_snprintf (totstr, sizeof (totstr),
        _("%d%% complete, %02d:%02d:%02d est. time remaining. (File %ld of %ld)"),
        pcent, hours, mins, secs, tdata->current_file_number,
        tdata->numdirs + tdata->numfiles);

  if (!tdata->stalled) {
      _setup_dlstr (tdata, tempfle, &tstatus);
  }
  g_mutex_unlock (&tdata->statmutex);

#if !defined(TRANSFER_GTK_TREEVIEW)
  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tdata->user_data, 1, totstr);
#endif
  gftp_lookup_global_option ("show_trans_in_title", &show_trans_in_title);
  if (gftp_file_transfers->data == tdata && show_trans_in_title)
  {
      g_snprintf (winstr, sizeof(winstr),  "%s: %s", gftp_version, totstr);
      gdk_window_set_title (gtk_widget_get_parent_window (GTK_WIDGET(dlwdw)),
                            winstr);
  }
  if (tstatus.text[0])
  {
#if !defined(TRANSFER_GTK_TREEVIEW)
      gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tempfle->user_data, 1, tstatus.text);
#else
      gtk_list_store_set (transfer_tview.store, (GtkTreeIter *)tempfle->user_data,
                          TTREE_COL_PROGRESS, tstatus.percent,
//                        TTREE_COL_PROGRESS_TEXT, tstatus.percent_str,
                          TTREE_COL_PROGRESS_INFO, tstatus.text, -1);
#endif
  }
}


static void  update_window_transfer_bytes (gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
  char *tempstr, *temp1str;

  if (wdata->request->gotbytes == -1)
    {
      update_window (wdata);
      wdata->request->gotbytes = 0;
    }
  else
    {
      tempstr = insert_commas (wdata->request->gotbytes, NULL, 0);
      temp1str = g_strdup_printf (_("Retrieving file names...%s bytes"), 
                                  tempstr);
      gtk_label_set_text (GTK_LABEL (wdata->dirinfo_label), temp1str);
      g_free (tempstr);
      g_free (temp1str);
    }
}


gint  update_downloads (gpointer data)
{
  //DEBUG_PRINT_FUNC
  /* endless loop 1 second */
  intptr_t do_one_transfer_at_a_time, start_transfers;
  GList * templist, * next;
  gftp_transfer * tdata;

  if (gftp_file_transfer_logs != NULL)
    display_cached_logs ();

  if (window1.request->gotbytes != 0)
    update_window_transfer_bytes (&window1);
  if (window2.request->gotbytes != 0)
    update_window_transfer_bytes (&window2);

  if (gftpui_common_child_process_done)
    check_done_process ();

  for (templist = gftp_file_transfers; templist != NULL;)
    {
      tdata = templist->data;
      if (tdata->ready)
        {
          g_mutex_lock (&tdata->structmutex);

          if (tdata->next_file)
            on_next_transfer (tdata);
          else if (tdata->show) 
            show_transfer (tdata);
          else if (tdata->done)
            {
              next = templist->next;
              g_mutex_unlock (&tdata->structmutex);
              transfer_done (templist);
              templist = next;
              continue;
            }

          if (tdata->curfle != NULL)
            {
              gftp_lookup_global_option ("one_transfer", 
                                         &do_one_transfer_at_a_time);
              gftp_lookup_global_option ("start_transfers", &start_transfers);

              if (!tdata->started && start_transfers &&
                 (num_transfers_in_progress == 0 || !do_one_transfer_at_a_time))
                create_transfer (tdata);

              if (tdata->started)
                update_file_status (tdata);
            }
          g_mutex_unlock (&tdata->structmutex);
        }
      templist = templist->next;
    }

  g_timeout_add (1000, update_downloads, NULL);
  return (0);
}


void  start_transfer (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata) {
      return;
  }

  g_mutex_lock (&transdata->transfer->structmutex);
  if (!transdata->transfer->started) {
      create_transfer (transdata->transfer);
  }
  g_mutex_unlock (&transdata->transfer->structmutex);
}


void  stop_transfer (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata) {
      return;
  }

  gftpui_common_cancel_file_transfer (transdata->transfer);
}


void  skip_transfer (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata || !transdata->curfle) {
      return;
  }

  gftpui_common_skip_file_transfer (transdata->transfer,
                                    transdata->transfer->curfle->data);
}


void  remove_file_transfer (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_common_curtrans_data * transdata;
  gftp_file * curfle;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata || !transdata->curfle || !transdata->curfle->data) {
      return;
  }

  curfle = transdata->curfle->data;
  gftpui_common_skip_file_transfer (transdata->transfer, curfle);

#if !defined(TRANSFER_GTK_TREEVIEW)
  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), curfle->user_data, 1, _("Skipped"));
#else
  gtk_list_store_set (transfer_tview.store, (GtkTreeIter *)curfle->user_data,
                      TTREE_COL_PROGRESS_INFO, _("Skipped"), -1);
#endif

}


void  move_transfer_up (gpointer data)
{
  DEBUG_PRINT_FUNC
  GList * firstentry, * secentry, * lastentry;
  gftpui_common_curtrans_data * transdata;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata || !transdata->curfle) {
      return;
  }

  g_mutex_lock (&transdata->transfer->structmutex);
  if (transdata->curfle->prev != NULL && (!transdata->transfer->started ||
      (transdata->transfer->curfle != transdata->curfle && 
       transdata->transfer->curfle != transdata->curfle->prev)))
  {
      if (transdata->curfle->prev->prev == NULL)
        {
          firstentry = transdata->curfle->prev;
          lastentry = transdata->curfle->next;
          transdata->transfer->files = transdata->curfle;
          transdata->curfle->next = firstentry;
          transdata->transfer->files->prev = NULL;
          firstentry->prev = transdata->curfle;
          firstentry->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = firstentry;
        }
      else
        {
          firstentry = transdata->curfle->prev->prev;
          secentry = transdata->curfle->prev;
          lastentry = transdata->curfle->next;
          firstentry->next = transdata->curfle;
          transdata->curfle->prev = firstentry;
          transdata->curfle->next = secentry;
          secentry->prev = transdata->curfle;
          secentry->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = secentry;
        }
#if !defined(TRANSFER_GTK_TREEVIEW)
      gtk_ctree_move (GTK_CTREE (dlwdw), 
                      ((gftp_file *) transdata->curfle->data)->user_data,
                      transdata->transfer->user_data, 
                      transdata->curfle->next != NULL ?
                          ((gftp_file *) transdata->curfle->next->data)->user_data: NULL);
#endif
  }
  g_mutex_unlock (&transdata->transfer->structmutex);
}


void  move_transfer_down (gpointer data)
{
  DEBUG_PRINT_FUNC
  GList * firstentry, * secentry, * lastentry;
  gftpui_common_curtrans_data * transdata;

  transdata = transfer_list_get_selected_transfer_data (1);
  if (!transdata || !transdata->curfle) {
      return;
  }

  g_mutex_lock (&transdata->transfer->structmutex);
  if (transdata->curfle->next != NULL && (!transdata->transfer->started ||
      (transdata->transfer->curfle != transdata->curfle && 
       transdata->transfer->curfle != transdata->curfle->next)))
  {
      if (transdata->curfle->prev == NULL)
      {
          firstentry = transdata->curfle->next;
          lastentry = transdata->curfle->next->next;
          transdata->transfer->files = firstentry;
          transdata->transfer->files->prev = NULL;
          transdata->transfer->files->next = transdata->curfle;
          transdata->curfle->prev = transdata->transfer->files;
          transdata->curfle->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = transdata->curfle;
      }
      else
      {
          firstentry = transdata->curfle->prev;
          secentry = transdata->curfle->next;
          lastentry = transdata->curfle->next->next;
          firstentry->next = secentry;
          secentry->prev = firstentry;
          secentry->next = transdata->curfle;
          transdata->curfle->prev = secentry;
          transdata->curfle->next = lastentry;
          if (lastentry != NULL)
            lastentry->prev = transdata->curfle;
      }
#if !defined(TRANSFER_GTK_TREEVIEW)
      gtk_ctree_move (GTK_CTREE (dlwdw), 
                      ((gftp_file *) transdata->curfle->data)->user_data,
                      transdata->transfer->user_data, 
                      transdata->curfle->next != NULL ?
                          ((gftp_file *) transdata->curfle->next->data)->user_data: NULL);
#endif
  }
  g_mutex_unlock (&transdata->transfer->structmutex);
}

