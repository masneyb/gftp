/*****************************************************************************/
/*  misc-gtk.c - misc stuff for the gtk+ port of gftp                        */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

#include "gftp-gtk.h"

static GtkWidget * statuswid;

void
remove_files_window (gftp_window_data * wdata)
{
  wdata->show_selected = 0;
  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  gtk_clist_clear (GTK_CLIST (wdata->listbox));
  free_file_list (wdata->files);
  wdata->files = NULL;
  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
}


void
ftp_log (gftp_logging_level level, gftp_request * request, 
         const char *string, ...)
{
  uintptr_t max_log_window_size;
  int upd, free_logstr;
  gftp_log * newlog;
  gint delsize;
  char *logstr;
  va_list argp;
  size_t len;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  const char *descr;
  char *utf8_str;
  size_t destlen;

  va_start (argp, string);
  if (strcmp (string, "%s") == 0)
    {
      logstr = va_arg (argp, char *);
      free_logstr = 0;
    }
  else
    {
      logstr = g_strdup_vprintf (string, argp);
      free_logstr = 1;
    }
  va_end (argp);

  if ((utf8_str = gftp_string_to_utf8 (request, logstr, &destlen)) != NULL)
    {
      if (free_logstr)
        g_free (logstr);
      else
        free_logstr = 1;

      logstr = utf8_str;
    }

  if (pthread_self () != main_thread_id)
    {
      newlog = g_malloc0 (sizeof (*newlog));
      newlog->type = level;
      if (free_logstr)
        newlog->msg = logstr;
      else
        newlog->msg = g_strdup (logstr);

      pthread_mutex_lock (&log_mutex);
      gftp_file_transfer_logs = g_list_append (gftp_file_transfer_logs, newlog);
      pthread_mutex_unlock (&log_mutex);
      return;
    }

  if (gftp_logfd != NULL && level != gftp_logging_misc_nolog)
    {
      if (fwrite (logstr, strlen (logstr), 1, gftp_logfd) != 1)
        {
          fclose (gftp_logfd);
          gftp_logfd = NULL;
        }
      else
        {
          fflush (gftp_logfd);
          if (ferror (gftp_logfd))
            {
              fclose (gftp_logfd);
              gftp_logfd = NULL;
            }
        }
    }

  upd = logwdw_vadj->upper - logwdw_vadj->page_size == logwdw_vadj->value;

  gftp_lookup_global_option ("max_log_window_size", &max_log_window_size);

  switch (level)
    {
      case gftp_logging_send:
        descr = "send";
        break;
      case gftp_logging_recv:
        descr = "recv";
        break;
      case gftp_logging_error:
        descr = "error";
        break;
      default:
        descr = "misc";
        break;
    }

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len);
  gtk_text_buffer_insert_with_tags_by_name (textbuf, &iter, logstr, -1, 
                                            descr, NULL);

  if (upd)
    {
      gtk_text_buffer_move_mark (textbuf, logwdw_textmark, &iter);
      gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (logwdw), logwdw_textmark,
                                     0, 1, 1, 1);
    }

  if (max_log_window_size > 0)
    {
      delsize = len + g_utf8_strlen (logstr, -1) - max_log_window_size;

      if (delsize > 0)
        {
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
          gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, delsize);
          gtk_text_buffer_delete (textbuf, &iter, &iter2);
        }
    }

  if (free_logstr)
    g_free (logstr);
}


void
update_window_info (void)
{
  char *tempstr, empty[] = "";
  unsigned int port, i, j;
  GtkWidget *combo_entry;

  if (current_wdata->request != NULL)
    {
      if (GFTP_IS_CONNECTED (current_wdata->request))
        {
          if ((tempstr = current_wdata->request->hostname) == NULL)
            tempstr = empty;
          combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(hostedit)));
          gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);

          if ((tempstr = current_wdata->request->username) == NULL)
            tempstr = empty;
          combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(useredit)));
          gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);
    
          if ((tempstr = current_wdata->request->password) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (passedit), tempstr);
    
          port = gftp_protocol_default_port (current_wdata->request);
          if (current_wdata->request->port != 0 &&
              port != current_wdata->request->port)
            {
              tempstr = g_strdup_printf ("%d", current_wdata->request->port);
              combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
              gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);
              g_free (tempstr);
            }
          else {
            combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
            gtk_entry_set_text (GTK_ENTRY (combo_entry), "");
          }
    
          for (i=0, j=0; gftp_protocols[i].init != NULL; i++)
            {
              if (!gftp_protocols[i].shown) 
                continue;

              if (current_wdata->request->init == gftp_protocols[i].init)
                {
                  gtk_combo_box_set_active(GTK_COMBO_BOX(toolbar_combo_protocol), j);
                  break;
                }
              j++;
            }

          gtk_widget_set_tooltip_text (openurl_btn, _("Disconnect from the remote server"));
        }
      else
        gtk_widget_set_tooltip_text (openurl_btn,
                              _("Connect to the site specified in the host entry. If the host entry is blank, then a dialog is presented that will allow you to enter a URL."));
    }

  update_window (&window1);
  update_window (&window2);

  GtkAction *tempwid = gtk_action_group_get_action(menus, "ToolsCompareWindows");
  gtk_action_set_sensitive (tempwid, GFTP_IS_CONNECTED (window1.request) 
			    && GFTP_IS_CONNECTED (window2.request));
}

static void
set_menu_sensitive (gftp_window_data * wdata, char *path, int sensitive)
{
  GtkAction *tempwid = NULL;
  if (menus) {
    tempwid = gtk_action_group_get_action(menus, path);
  }
  if (tempwid) {
    gtk_action_set_sensitive (tempwid, sensitive);
  }
}

char *
report_list_info(gftp_window_data * wdata)
{
  GList *templist;
  int  filenum = 0, dirnum = 0, linknum = 0;
  off_t filesize = 0;
  gftp_file *fle;
  char listreport[256] = "";
  char files_str[50] = "";
  char links_str[50] = "";
  char dirs_str[50] = "";

  if(!wdata || !wdata->files)
     return NULL;

  templist=wdata->files;
  while (templist != NULL)
  {
      fle=(gftp_file*)(templist->data);
      if(fle->shown && strcmp(fle->file,".."))
      {
        if(S_ISLNK (fle->st_mode))      linknum++;
        else if(S_ISDIR (fle->st_mode)) dirnum++;
        else {
           filenum++;
           filesize += fle->size;
        }
      }
      templist = templist->next;
  }
  if (!filenum && !dirnum && !linknum) {
    return NULL;
  }
  if(filenum) {
     char fsize[40];
     gftp_format_file_size(filesize, fsize, sizeof(fsize));
     snprintf(files_str, sizeof(files_str), _(" [ %d Files (%s) ] "), filenum, fsize);
  }
  if(dirnum) {
     snprintf(dirs_str, sizeof(dirs_str), _(" [ %d Dirs ] "), dirnum);
  }
  if(linknum) {
     snprintf(links_str, sizeof(links_str), _(" [ %d Links ] "), linknum);
  }
  snprintf(listreport, sizeof(listreport), "%s%s%s",
                             files_str, dirs_str, links_str);
  return g_strdup(listreport);
}


void
update_window (gftp_window_data * wdata)
{
  char *tempstr, *hostname, *fspec, *listinfo;
  int connected;
  GtkWidget *combo_entry;
  
  connected = GFTP_IS_CONNECTED (wdata->request);
  if (connected)
    {
      fspec = wdata->show_selected ? "Selected" : strcmp (wdata->filespec, "*") == 0 ?  _("All Files") : wdata->filespec;

      if (wdata->request->hostname == NULL ||
          wdata->request->protonum == GFTP_LOCAL_NUM)
        hostname = "";
      else
        hostname = wdata->request->hostname;

      listinfo = report_list_info(wdata);
      tempstr = g_strconcat (hostname, *hostname == '\0' ? "[" : " [",
                             gftp_protocols[wdata->request->protonum].name,
                             wdata->request->cached ? _("] (Cached) [") : "] [",
                             fspec, "]",
                             listinfo == NULL ? "" : listinfo,
                             current_wdata == wdata ? "*" : "",
                             NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (listinfo);
      g_free (tempstr);

      if (wdata->request->directory != NULL) {
        combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(wdata->combo)));
        gtk_entry_set_text (GTK_ENTRY (combo_entry), wdata->request->directory);
      }
    }
  else if (wdata->hoststxt != NULL)
    {
      tempstr = g_strconcat (_("Not connected"), 
                             current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);
    }

  connected = GFTP_IS_CONNECTED (window2.request);
  set_menu_sensitive (NULL, "RemoteDisconnect", connected);
  set_menu_sensitive (NULL, "RemoteChangeFilespec", connected);
  set_menu_sensitive (NULL, "RemoteShowSelected", connected);
  set_menu_sensitive (NULL, "RemoteNavigateUp", connected);
  set_menu_sensitive (NULL, "RemoteSelectAll", connected);
  set_menu_sensitive (NULL, "RemoteSelectAllFiles", connected);
  set_menu_sensitive (NULL, "RemoteDeselectAll", connected);
  set_menu_sensitive (NULL, "RemoteSaveDirectoryListing", connected);
  set_menu_sensitive (NULL, "RemoteSendSITECommand", connected);
  set_menu_sensitive (NULL, "RemoteChangeDirectory", connected);
  set_menu_sensitive (NULL, "RemotePermissions", connected);
  set_menu_sensitive (NULL, "RemoteNewFolder", connected);
  set_menu_sensitive (NULL, "RemoteRename", connected);
  set_menu_sensitive (NULL, "RemoteDelete", connected);
  set_menu_sensitive (NULL, "RemoteEdit", connected);
  set_menu_sensitive (NULL, "RemoteView", connected);
  set_menu_sensitive (NULL, "RemoteRefresh", connected);

  connected = GFTP_IS_CONNECTED (window1.request) && GFTP_IS_CONNECTED (window2.request);
  gtk_widget_set_sensitive (download_left_arrow, connected);
  gtk_widget_set_sensitive (upload_right_arrow, connected);
}  


gftp_graphic *
open_xpm (GtkWidget * widget, char *filename)
{
  gftp_graphic * graphic;
  GtkStyle *style;
  char *exfile;

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) != NULL)
    return (graphic);

  style = gtk_widget_get_style (widget);

  if ((exfile = get_image_path (filename)) == NULL)
    return (NULL);

  graphic = g_malloc0 (sizeof (*graphic));
  graphic->pixmap = gdk_pixmap_create_from_xpm (widget->window, 
                        &graphic->bitmap, &style->bg[GTK_STATE_NORMAL], exfile);
  g_free (exfile);

  if (graphic->pixmap == NULL)
    {
      g_free (graphic);
      ftp_log (gftp_logging_error, NULL, _("Error opening file %s: %s\n"), 
               exfile, g_strerror (errno));
      return (NULL);
    }

  graphic->filename = g_strdup (filename);
  g_hash_table_insert (graphic_hash_table, graphic->filename, graphic);

  return (graphic);
}


void
gftp_free_pixmap (char *filename)
{
  gftp_graphic * graphic;

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) == NULL)
    return;

  g_object_unref (graphic->pixmap);
  g_object_unref (graphic->bitmap);

  g_hash_table_remove (graphic_hash_table, filename);
  g_free (graphic->filename);
  g_free (graphic);
}


void
gftp_get_pixmap (GtkWidget * widget, char *filename, GdkPixmap ** pix,
                 GdkBitmap ** bitmap)
{
  gftp_graphic * graphic;

  if (filename == NULL || *filename == '\0')
    {
      *pix = NULL;
      *bitmap = NULL;
      return;
    }

  if ((graphic = g_hash_table_lookup (graphic_hash_table, filename)) == NULL)
    graphic = open_xpm (widget, filename);

  if (graphic == NULL)
    {
      *pix = NULL;
      *bitmap = NULL;
      return;
    }

  *pix = graphic->pixmap;
  *bitmap = graphic->bitmap;
}

int
check_status (char *name, gftp_window_data *wdata,
              unsigned int check_other_stop, unsigned int only_one,
              unsigned int at_least_one, unsigned int func)
{
  gftp_window_data * owdata;

  owdata = wdata == &window1 ? &window2 : &window1;

  if (wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: Please hit the stop button first to do anything else\n"),
	       name);
      return (0);
    }

  if (check_other_stop && owdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: Please hit the stop button first to do anything else\n"),
	       name);
      return (0);
    }

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: Not connected to a remote site\n"), name);
      return (0);
    }

  if (!func)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: This feature is not available using this protocol\n"),
	       name);
      return (0);
    }

  if (only_one && !IS_ONE_SELECTED (wdata))
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: You must only have one item selected\n"), name);
      return (0);
    }

  if (at_least_one && !only_one && IS_NONE_SELECTED (wdata))
    {
      ftp_log (gftp_logging_error, NULL,
	       _("%s: You must have at least one item selected\n"), name);
      return (0);
    }
  return (1);
}


void
add_history (GtkWidget * widget, GList ** history, unsigned int *histlen, 
             const char *str)
{
  GList *node, *delnode;
  char *tempstr;
  int i;

  if (str == NULL || *str == '\0')
    return;

  for (node = *history; node != NULL; node = node->next)
    {
      if (strcmp ((char *) node->data, str) == 0)
	break;
    }

  if (node == NULL)
    {
      if (*histlen >= MAX_HIST_LEN)
	{
	  node = *history;
	  for (i = 1; i < MAX_HIST_LEN; i++)
	    node = node->next;
	  node->prev->next = NULL;
	  node->prev = NULL;
	  delnode = node;
	  while (delnode != NULL)
	    {
	      if (delnode->data)
		g_free (delnode->data);
	      delnode = delnode->next;
	    }
	  g_list_free (node);
	}
      tempstr = g_strdup (str);
      *history = g_list_prepend (*history, tempstr);
      ++*histlen;
    }
  else if (node->prev != NULL)
    {
      node->prev->next = node->next;
      if (node->next != NULL)
	node->next->prev = node->prev;
      node->prev = NULL;
      node->next = *history;
      if (node->next != NULL)
	node->next->prev = node;
      *history = node;
    }

  gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(widget) )) );
  GList *glist = *history;
  while (glist) {
    if (glist->data) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), glist->data);
    }
    glist = glist->next;
  }
}


int
check_reconnect (gftp_window_data *wdata)
{
  return (wdata->request->cached && wdata->request->datafd < 0 && 
          !wdata->request->always_connected &&
	  !ftp_connect (wdata, wdata->request) ? -1 : 0);
}


void
add_file_listbox (gftp_window_data * wdata, gftp_file * fle)
{
  char *add_data[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
  char *tempstr, *attribs, *zeroseconds;
  char time_str[60] = "";
  struct tm timeinfo;
  gftp_config_list_vars * tmplistvar;
  gftp_file_extensions * tempext;
  GdkBitmap * bitmap;
  GList * templist;
  GdkPixmap * pix;
  int clist_num;
  size_t stlen;

  if (wdata->show_selected)
    {
      fle->shown = fle->was_sel;
      if (!fle->shown)
        return;
    }
  else if (!gftp_match_filespec (wdata->request, fle->file, wdata->filespec))
    {
      fle->shown = 0;
      fle->was_sel = 0;
      return;
    }
  else
    fle->shown = 1;

  clist_num = gtk_clist_append (GTK_CLIST (wdata->listbox), add_data);

  if (fle->was_sel)
    {
      fle->was_sel = 0;
      gtk_clist_select_row (GTK_CLIST (wdata->listbox), clist_num, 0);
    }

  pix = NULL;
  bitmap = NULL;
  if (strcmp (fle->file, "..") == 0)
    gftp_get_pixmap (wdata->listbox, "dotdot.xpm", &pix, &bitmap);
  else if (S_ISLNK (fle->st_mode) && S_ISDIR (fle->st_mode))
    gftp_get_pixmap (wdata->listbox, "linkdir.xpm", &pix, &bitmap);
  else if (S_ISLNK (fle->st_mode))
    gftp_get_pixmap (wdata->listbox, "linkfile.xpm", &pix, &bitmap);
  else if (S_ISDIR (fle->st_mode))
    gftp_get_pixmap (wdata->listbox, "dir.xpm", &pix, &bitmap);
  else if ((fle->st_mode & S_IXUSR) ||
           (fle->st_mode & S_IXGRP) ||
           (fle->st_mode & S_IXOTH))
    gftp_get_pixmap (wdata->listbox, "exe.xpm", &pix, &bitmap); 
  else
    {
      stlen = strlen (fle->file);
      gftp_lookup_global_option ("ext", &tmplistvar);
      templist = tmplistvar->list;
      while (templist != NULL)
        {
          tempext = templist->data;
          if (stlen >= tempext->stlen &&
              strcmp (&fle->file[stlen - tempext->stlen], tempext->ext) == 0)
            {
              gftp_get_pixmap (wdata->listbox, tempext->filename, &pix, 
                               &bitmap);
              break;
            }
          templist = templist->next;
        }
    }

  if (pix == NULL && bitmap == NULL)
    gftp_get_pixmap (wdata->listbox, "doc.xpm", &pix, &bitmap);
   
  gtk_clist_set_pixmap (GTK_CLIST (wdata->listbox), clist_num, 0, pix, bitmap);

  if (fle->file != NULL && fle->filename_utf8_encoded)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 1, fle->file);

  if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode))
    tempstr = g_strdup_printf ("%d, %d", major (fle->size),
                               minor (fle->size));
  else
    tempstr = insert_commas (fle->size, NULL, 0);

  gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 2, tempstr);
  g_free (tempstr);

  if (localtime_r( &fle->datetime, &timeinfo )) {
    strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S ", &timeinfo);
    zeroseconds = strstr(time_str, ":00 ");
    if (zeroseconds) *zeroseconds = 0;
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 3, time_str);
  }

  if (fle->user)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 4, fle->user);
  if (fle->group)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 5, fle->group);

  attribs = gftp_convert_attributes_from_mode_t (fle->st_mode);
  gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 6, attribs);
  g_free (attribs);

}


void
destroy_dialog (gftp_dialog_data * ddata)
{
  if (ddata->dialog != NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
    }
}


static void
dialog_response (GtkWidget * widget, gint response, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }

  switch (response)
    {
      case GTK_RESPONSE_YES:
        if (ddata->yesfunc != NULL)
          ddata->yesfunc (ddata->yespointer, ddata);
        break;
      default:
        if (ddata->nofunc != NULL)
          ddata->nofunc (ddata->nopointer, ddata);
        break;
    }

  if (ddata->edit != NULL &&
      ddata->dialog != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}


static gint
dialog_keypress (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (event->type != GDK_KEY_PRESS)
    return (FALSE);

  if (event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)
    {
      dialog_response (widget, GTK_RESPONSE_YES, data);
      return (TRUE);
    }
  else if (event->keyval == GDK_Escape)
    {
      dialog_response (widget, GTK_RESPONSE_NO, data);
      return (TRUE);
    }

  return (FALSE);
}


void
MakeEditDialog (char *diagtxt, char *infotxt, char *deftext, int passwd_item,
		char *checktext, 
                gftp_dialog_button okbutton, void (*okfunc) (), void *okptr,
		void (*cancelfunc) (), void *cancelptr)
{
  GtkWidget * tempwid, * dialog;
  gftp_dialog_data * ddata;
  const gchar * yes_text;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = okfunc;
  ddata->yespointer = okptr;
  ddata->nofunc = cancelfunc;
  ddata->nopointer = cancelptr;

  switch (okbutton)
    {
      case gftp_dialog_button_ok:
        yes_text = GTK_STOCK_OK;
        break;
      case gftp_dialog_button_create:
        yes_text = GTK_STOCK_ADD;
        break;
      case gftp_dialog_button_change:
        yes_text = _("Change");
        break;
      case gftp_dialog_button_connect:
        yes_text = _("Connect");
        break;
      case gftp_dialog_button_rename:
        yes_text = _("Rename");
        break;
      default:
        yes_text = GTK_STOCK_MISSING_IMAGE;
        break;
    }

  dialog = gtk_dialog_new_with_buttons (_(diagtxt), NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_NO,
                                        yes_text,
                                        GTK_RESPONSE_YES,
                                        NULL);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "edit", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_grab_add (dialog);
  gtk_widget_realize (dialog);

  set_window_icon(GTK_WINDOW(dialog), NULL);

  ddata->dialog = dialog;

  tempwid = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, TRUE,
		      TRUE, 0);
  gtk_widget_show (tempwid);

  ddata->edit = gtk_entry_new ();
  g_signal_connect (G_OBJECT (ddata->edit), "key_press_event",
                      G_CALLBACK (dialog_keypress), (gpointer) ddata);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), ddata->edit, TRUE,
		      TRUE, 0);
  gtk_widget_grab_focus (ddata->edit);
  gtk_entry_set_visibility (GTK_ENTRY (ddata->edit), passwd_item);

  if (deftext != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (ddata->edit), deftext);
      gtk_entry_select_region (GTK_ENTRY (ddata->edit), 0, strlen (deftext));
    }
  gtk_widget_show (ddata->edit);

  if (checktext != NULL)
    {
      ddata->checkbox = gtk_check_button_new_with_label (checktext);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
                          ddata->checkbox, TRUE, TRUE, 0);
      gtk_widget_show (ddata->checkbox);
    }
      
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);

  gtk_widget_show (dialog);
}


void
MakeYesNoDialog (char *diagtxt, char *infotxt, 
                 void (*yesfunc) (), gpointer yespointer, 
                 void (*nofunc) (), gpointer nopointer)
{
  GtkWidget * text, * dialog;
  gftp_dialog_data * ddata;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = yesfunc;
  ddata->yespointer = yespointer;
  ddata->nofunc = nofunc;
  ddata->nopointer = nopointer;

  dialog = gtk_dialog_new_with_buttons (_(diagtxt), NULL, 0,
                                        GTK_STOCK_NO,
                                        GTK_RESPONSE_NO,
                                        GTK_STOCK_YES,
                                        GTK_RESPONSE_YES,
                                        NULL);

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "yndiag", "gFTP");
  gtk_grab_add (dialog);
  gtk_widget_realize (dialog);

  set_window_icon(GTK_WINDOW(dialog), NULL);

  ddata->dialog = dialog;

  text = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), text, TRUE, TRUE, 0);
  gtk_widget_show (text);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);

  gtk_widget_show (dialog);
}


static gint
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  return (TRUE);
}


static void
trans_stop_button (GtkWidget * widget, gpointer data)
{
  gftp_transfer * transfer;

  transfer = data;
  pthread_kill (((gftp_window_data *) transfer->fromwdata)->tid, SIGINT);
}


void
update_directory_download_progress (gftp_transfer * transfer)
{
  static GtkWidget * dialog = NULL, * textwid, * stopwid;
  char tempstr[255];
  GtkWidget * vbox;

  if (transfer->numfiles < 0 || transfer->numdirs < 0)
    {
      if (dialog != NULL)
        gtk_widget_destroy (dialog);
      dialog = NULL;
      return;
    }

  if (dialog == NULL)
    {
      dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
      gtk_window_set_decorated (GTK_WINDOW (dialog), 0);
      gtk_grab_add (dialog);
      g_signal_connect (G_OBJECT (dialog), "delete_event",
                          G_CALLBACK (delete_event), NULL);
      gtk_window_set_title (GTK_WINDOW (dialog),
			    _("Getting directory listings"));
      gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
      gtk_window_set_wmclass (GTK_WINDOW(dialog), "dirlist", "gFTP");

      vbox = gtk_vbox_new (FALSE, 5);
      gtk_container_border_width (GTK_CONTAINER (vbox), 10);
      gtk_container_add (GTK_CONTAINER (dialog), vbox);
      gtk_widget_show (vbox);

      textwid = gtk_label_new (NULL);
      gtk_box_pack_start (GTK_BOX (vbox), textwid, TRUE, TRUE, 0);
      gtk_widget_show (textwid);

      statuswid = gtk_progress_bar_new ();
      gtk_progress_set_activity_mode (GTK_PROGRESS (statuswid), 1);
      gtk_progress_bar_set_activity_step (GTK_PROGRESS_BAR (statuswid), 3);
      gtk_progress_bar_set_activity_blocks (GTK_PROGRESS_BAR (statuswid), 5);
      gtk_box_pack_start (GTK_BOX (vbox), statuswid, TRUE, TRUE, 0);
      gtk_widget_show (statuswid);

      stopwid = gtk_button_new_with_label (_("  Stop  "));
      g_signal_connect (G_OBJECT (stopwid), "clicked",
                          G_CALLBACK (trans_stop_button), transfer);
      gtk_box_pack_start (GTK_BOX (vbox), stopwid, TRUE, TRUE, 0);
      gtk_widget_show (stopwid); 

      gtk_widget_show (dialog);
    }

  g_snprintf (tempstr, sizeof (tempstr),
              _("Received %ld directories\nand %ld files"), 
              transfer->numdirs, transfer->numfiles);
  gtk_label_set_text (GTK_LABEL (textwid), tempstr);
}


int
progress_timeout (gpointer data)
{
  gftp_transfer * tdata;
  double val;

  tdata = data;

  update_directory_download_progress (tdata);

  val = gtk_progress_get_value (GTK_PROGRESS (statuswid));
  if (val >= 1.0)
    val = 0.0;
  else
    val += 0.10;
  gtk_progress_bar_update (GTK_PROGRESS_BAR (statuswid), val);

  return (1);
}


void
display_cached_logs (void)
{
  gftp_log * templog;
  GList * templist; 

  pthread_mutex_lock (&log_mutex);
  templist = gftp_file_transfer_logs;
  while (templist != NULL)
    { 
      templog = (gftp_log *) templist->data;
      ftp_log (templog->type, NULL, "%s", templog->msg);
      g_free (templog->msg);
      g_free (templog); 
      templist->data = NULL;
      templist = templist->next;
    }
  g_list_free (gftp_file_transfer_logs);
  gftp_file_transfer_logs = NULL;
  pthread_mutex_unlock (&log_mutex);
}

char *
get_image_path (char *filename) {
	char *path1 = NULL, *path2 = NULL, *found = NULL;

	// see lib/misc.c -> gftp_get_share_dir ()
	path1 = g_strconcat (gftp_get_share_dir (), "/", filename, NULL);
	if (access (path1, F_OK) == 0) {
		found = path1;
	}

	if (!found) {
		path2 = g_strconcat ("/usr/share/gftp/", filename, NULL);
		if (access (path2, F_OK) == 0) {
			found = path2;
		}
	}
	if (!found) {
		if (path1) fprintf(stderr, "* %s: %s not found\n", PACKAGE_NAME, path1);
		if (path2 && strcmp(path1,path2) != 0)
		           fprintf(stderr, "* %s: %s not found\n", PACKAGE_NAME, path2);
	}

	if (path1 && path1 != found) g_free (path1);
	if (path2 && path2 != found) g_free (path2);

	return (found);
}

void
set_window_icon(GtkWindow *window, char *icon_name)
{
  GdkPixbuf *pixbuf;
  char *img_path;
  if (icon_name)
    img_path = get_image_path (icon_name);
  else
    img_path = get_image_path ("gftp.xpm");
  if (img_path) {
    pixbuf = gdk_pixbuf_new_from_file(img_path, NULL);
    g_free (img_path);
    if (pixbuf) {
      gtk_window_set_icon (window, pixbuf);
      //gtk_window_set_icon_name (window, gftp_version);
      g_object_unref(pixbuf);
    }
  }
}
