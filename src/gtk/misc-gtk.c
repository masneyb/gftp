/***********************************************************************************/
/*  misc-gtk.c - misc stuff for the gtk+ port of gftp                              */
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

void remove_files_window (gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
  wdata->show_selected = 0;
  listbox_clear(wdata);
  free_file_list (wdata->files);
  wdata->files = NULL;
}


void ftp_log (gftp_logging_level level, gftp_request * request, 
              const char *string, ...)
{
  //DEBUG_PRINT_FUNC
  uintptr_t max_log_window_size;
  int upd, free_logstr;
  gftp_log * newlog;
  gint delsize;
  char *logstr;
  va_list argp;
  size_t len;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  const char *descr = NULL;
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

  if (gftp_gtk_colored_msgs)
  {
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

  DEBUG_MSG(logstr)
  if (free_logstr)
    g_free (logstr);
}


void update_window_info (void)
{
  DEBUG_PRINT_FUNC
  char *tempstr, empty[] = "";
  unsigned int port, i, j;
  GtkWidget *dir_combo_entry;

  if (current_wdata->request != NULL)
    {
      if (GFTP_IS_CONNECTED (current_wdata->request))
        {
          if ((tempstr = current_wdata->request->hostname) == NULL)
            tempstr = empty;
          dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(hostedit)));
          gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);

          if ((tempstr = current_wdata->request->username) == NULL)
            tempstr = empty;
          dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(useredit)));
          gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);
    
          if ((tempstr = current_wdata->request->password) == NULL)
            tempstr = empty;
          gtk_entry_set_text (GTK_ENTRY (passedit), tempstr);
    
          port = gftp_protocol_default_port (current_wdata->request);
          if (current_wdata->request->port != 0 &&
              port != current_wdata->request->port)
            {
              tempstr = g_strdup_printf ("%d", current_wdata->request->port);
              dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
              gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);
              g_free (tempstr);
            }
          else {
            dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
            gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), "");
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

static void set_menu_sensitive (gftp_window_data * wdata, char *path, int sensitive)
{
  //DEBUG_PRINT_FUNC
  GtkAction *tempwid = NULL;
  if (menus) {
    tempwid = gtk_action_group_get_action(menus, path);
  }
  if (tempwid) {
    gtk_action_set_sensitive (tempwid, sensitive);
  }
}

char * report_list_info(gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
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


void update_window (gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
  char *tempstr, *hostname, *fspec, *listinfo;
  int connected;
  GtkWidget *dir_combo_entry;
  char * real_protocol;
  int i;

  connected = GFTP_IS_CONNECTED (wdata->request);
  if (connected)
    {
      fspec = wdata->show_selected ? "Selected" : strcmp (wdata->filespec, "*") == 0 ?  _("All Files") : wdata->filespec;

      if (!wdata->request->hostname || wdata->request->protonum == GFTP_PROTOCOL_LOCALFS) {
        hostname = "";
      } else {
        hostname = wdata->request->hostname;
      }
      real_protocol = gftp_protocols[wdata->request->protonum].name;

      for (i = 0; gftp_protocols[i].name; i++) {
         // protocols that are implemented on top of other protocols
         // may not set the protonum to avoid causing trouble
         // only the request->url_prefix may provide the true protocol
         if (wdata->request->url_prefix == gftp_protocols[i].url_prefix) {
            real_protocol = gftp_protocols[i].name;
            break;
         }
      }

      listinfo = report_list_info(wdata);
      tempstr = g_strconcat (hostname, *hostname == '\0' ? "[" : " [",
                             real_protocol,
                             wdata->request->cached ? _("] (Cached) [") : "] [",
                             fspec, "]",
                             listinfo == NULL ? "" : listinfo,
                             current_wdata == wdata ? "*" : "",
                             NULL);
      gtk_label_set_text (GTK_LABEL (wdata->dirinfo_label), tempstr);
      g_free (listinfo);
      g_free (tempstr);

      if (wdata->request->directory != NULL) {
        dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(wdata->dir_combo)));
        gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), wdata->request->directory);
      }
    }
  else if (wdata->dirinfo_label != NULL)
    {
      tempstr = g_strconcat (_("Not connected"), 
                             current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set_text (GTK_LABEL (wdata->dirinfo_label), tempstr);
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


gftp_graphic * open_xpm (GtkWidget * widget, char *filename)
{
  //DEBUG_PRINT_FUNC
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


void gftp_free_pixmap (char *filename)
{
  //DEBUG_PRINT_FUNC
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
  //DEBUG_PRINT_FUNC
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

GdkPixbuf * gftp_get_pixbuf (char *filename)
{
   //DEBUG_PRINT_FUNC
   GdkPixbuf *pixbuf;
   char *img_path;

   if (!filename || !*filename) {
      return NULL;
   }

   pixbuf = g_hash_table_lookup (pixbuf_hash_table, filename);
   if (!pixbuf) {

      img_path = get_image_path (filename);
      if (!img_path) {
         return NULL;
      }

      pixbuf = gdk_pixbuf_new_from_file (img_path, NULL);
      if (pixbuf) {
         g_hash_table_insert (pixbuf_hash_table, filename, pixbuf);
      }
      g_free (img_path);
   }

   return (pixbuf);
}

int
check_status (char *name, gftp_window_data *wdata,
              unsigned int check_other_stop, unsigned int only_one,
              unsigned int at_least_one, unsigned int func)
{
  DEBUG_PRINT_FUNC
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

  if (only_one && listbox_num_selected(wdata) != 1)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: You must only have one item selected\n"), name);
      return (0);
    }

  if (at_least_one && !only_one && listbox_num_selected(wdata) == 0)
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
  DEBUG_PRINT_FUNC
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

  glist_to_combobox (*history, widget);
}


int check_reconnect (gftp_window_data *wdata)
{ // see gftpui_check_reconnect
  DEBUG_PRINT_FUNC
  return (wdata->request->cached && wdata->request->datafd < 0 && 
          !wdata->request->always_connected &&
          !gftp_gtk_connect (wdata, wdata->request) ? -1 : 0);
}


static void
dialog_response (GtkDialog * dlg, gint responseID, gpointer data)
{
  //DEBUG_PRINT_FUNC
  gftp_dialog_data * ddata = (gftp_dialog_data *) data;
  /* destroy dialog */
  if (ddata && ddata->dialog) {
     if (ddata->edit) {
        ddata->entry_text = g_strdup (gtk_entry_get_text (GTK_ENTRY (ddata->edit)));
     }
     if (ddata->checkbox) {
        ddata->checkbox_is_ticked = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (ddata->checkbox));
     }
     gtk_widget_destroy (ddata->dialog);
     ddata->dialog = NULL;
  }
  /* run yesfunc or nofunc */
  if (responseID == GTK_RESPONSE_YES && ddata->yesfunc) {
     ddata->yesfunc (ddata->yespointer, ddata);
  } else if (ddata->nofunc) {
     ddata->nofunc (ddata->nopointer, ddata);
  }
  /* free ddata */
  if (ddata->entry_text) g_free (ddata->entry_text);
  g_free (ddata);
}


static gint
dialog_keypress (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  DEBUG_PRINT_FUNC
  if (event->keyval == GDK_KEY(KP_Enter) || event->keyval == GDK_KEY(Return))
    {
      dialog_response (NULL, GTK_RESPONSE_YES, data);
      return (TRUE);
    }
  else if (event->keyval == GDK_KEY(Escape))
    {
      dialog_response (NULL, GTK_RESPONSE_NO, data);
      return (TRUE);
    }

  return (FALSE);
}


void
TextEntryDialog (GtkWindow * parent_window,       /* nullable */
                 char * title,   char * infotxt,
                 char * deftext, int passwd_item,
                 char * checktext, 
                 gftp_dialog_button okbutton, void (*okfunc) (), void *okptr,
                 void (*cancelfunc) (), void *cancelptr)
{
  DEBUG_PRINT_FUNC
  GtkWidget * tempwid, * dialog, *vbox;
  gftp_dialog_data * ddata;
  const gchar * yes_text;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = okfunc;
  ddata->yespointer = okptr;
  ddata->nofunc = cancelfunc;
  ddata->nopointer = cancelptr;

  dialog = gtk_dialog_new();
  ddata->dialog = dialog;

  switch (okbutton)
    {
      case gftp_dialog_button_ok:
        yes_text = "gtk-ok";
        break;
      case gftp_dialog_button_create:
        yes_text = "gtk-add";
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
        yes_text = "gtk-missing-image";
        break;
    }

  if (!parent_window) {
     // must create a parent window
     parent_window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
     gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
     gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
     gtk_grab_add (dialog);
  } else {
     gtk_window_set_transient_for (GTK_WINDOW (dialog), parent_window);
     gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
     gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
     gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
     gtk_window_set_skip_pager_hint (GTK_WINDOW (dialog), TRUE);
  }

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_widget_set_size_request (dialog, 380, -1);
  set_window_icon (GTK_WINDOW(dialog), NULL);

  gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-cancel", GTK_RESPONSE_NO);
  gtk_dialog_add_button (GTK_DIALOG (dialog), yes_text,     GTK_RESPONSE_YES);

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
  vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_box_set_spacing (GTK_BOX (vbox), 5);

  tempwid = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (vbox), tempwid, TRUE, TRUE, 0);
  gtkcompat_widget_set_halign_left (tempwid);

  ddata->edit = gtk_entry_new ();
  g_signal_connect (G_OBJECT (ddata->edit), "key_press_event",
                    G_CALLBACK (dialog_keypress), (gpointer) ddata);

  gtk_box_pack_start (GTK_BOX (vbox), ddata->edit, TRUE, TRUE, 0);
  gtk_widget_grab_focus (ddata->edit);
  gtk_entry_set_visibility (GTK_ENTRY (ddata->edit), passwd_item);

  if (deftext != NULL)
    {
      gtk_entry_set_text (GTK_ENTRY (ddata->edit), deftext);
      gtk_editable_select_region (GTK_EDITABLE (ddata->edit), 0, strlen (deftext));
    }

  if (checktext != NULL)
    {
      ddata->checkbox = gtk_check_button_new_with_label (checktext);
      gtk_box_pack_start (GTK_BOX (vbox), ddata->checkbox, TRUE, TRUE, 0);
    }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);

  gtk_widget_show_all (dialog);
}


void
YesNoDialog (GtkWindow * parent_window,              /* nullable */
             char * title,       char * infotxt, 
             void (*yesfunc) (), gpointer yespointer, 
             void (*nofunc) (),  gpointer nopointer)
{
  DEBUG_PRINT_FUNC
  GtkWidget * dialog;
  gftp_dialog_data * ddata;

  ddata = g_malloc0 (sizeof (*ddata));
  ddata->yesfunc = yesfunc;
  ddata->yespointer = yespointer;
  ddata->nofunc = nofunc;
  ddata->nopointer = nopointer;

  dialog = gtk_message_dialog_new (parent_window,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   "%s", infotxt);
  ddata->dialog = dialog;

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  set_window_icon (GTK_WINDOW(dialog), NULL);
  if (!parent_window) {
     gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
     gtk_grab_add (dialog);
  }
  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);
  gtk_widget_show (dialog);
}


void display_cached_logs (void)
{
  DEBUG_PRINT_FUNC
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

char * get_image_path (char *filename)
{
   //DEBUG_PRINT_FUNC
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

void set_window_icon(GtkWindow *window, char *icon_name)
{
  //DEBUG_PRINT_FUNC
  GdkPixbuf *pixbuf;
  char *img_path;
  if (icon_name)
    img_path = get_image_path (icon_name);
  else
    img_path = get_image_path ("gftp.png");
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

void glist_to_combobox (GList *list, GtkWidget *combo)
{
   //DEBUG_PRINT_FUNC
   GtkTreeIter iter;
   GtkTreeModel *model = GTK_TREE_MODEL (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));
   GtkListStore *store = GTK_LIST_STORE (model);

   gtk_list_store_clear (store);
   while (list) {
      if (list->data) {
         // gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(combo), list->data);
         gtk_list_store_append (store, &iter);
         gtk_list_store_set (store, &iter, 0, list->data, -1);
      }
      list = list->next;
   }
}

void populate_combo_and_select_protocol (GtkWidget *combo, char * selected_protocol)
{
   DEBUG_PRINT_FUNC
   int i = 0;
   int combo_item_count = 0;
   int combo_item_selected = 0;
   for (i = 0; gftp_protocols[i].name; i++)
   {
      if (!gftp_protocols[i].shown) {
         continue; /* ignore hidden protocols */
      }
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo),
                                      gftp_protocols[i].name);
      if (selected_protocol) {
         if (strcmp (gftp_protocols[i].name, selected_protocol) == 0) {
            combo_item_selected = combo_item_count;
         }
      }
      combo_item_count++;
   }
   gtk_combo_box_set_active (GTK_COMBO_BOX(combo), combo_item_selected);
}


GtkMenuItem *
new_menu_item (GtkMenu * menu, char * label, char * icon_name,
               gpointer activate_callback, gpointer callback_data)
{
   //DEBUG_PRINT_FUNC
   GtkMenuItem *item = NULL;

   /* 0=normal 1=image 2=stock 3=check 4=separator */
   int type = 0;

   if (icon_name) {
      type = 1;
      if (strncmp (icon_name, "gtk-", 4) == 0)
      {
         type = 2; /*  GTK_STOCK_*   */
         GtkIconTheme *icon_theme = gtk_icon_theme_get_default();
         icon_theme = gtk_icon_theme_get_default();
         if (gtk_icon_theme_has_icon(icon_theme, icon_name)) {
            /* icon name is in icon theme.. don't use GTK_STOCK.. */
            type = 1;
         }
      } else if (strcmp (icon_name, "-check-") == 0) {
         type = 3;
      }
   }
   if (!label && !icon_name) {
      type = 4;
   }

   switch (type)
   {
     case 0: /* normal */
        item = GTK_MENU_ITEM (gtk_menu_item_new_with_mnemonic (label));
        break;
     case 1: /* image */
        item = GTK_MENU_ITEM (gtk_image_menu_item_new_with_mnemonic (label));
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
               gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU));
        break;
     case 2: /* stock */
        item = GTK_MENU_ITEM (gtk_image_menu_item_new_from_stock (label, NULL));
        break;
     case 3: /* check */
        item = GTK_MENU_ITEM(gtk_check_menu_item_new_with_mnemonic (label));
        break;
     case 4: /* separator */
        item = GTK_MENU_ITEM (gtk_separator_menu_item_new ());
        break;
   }

   if (menu) {
      gtk_container_add (GTK_CONTAINER (menu), GTK_WIDGET (item));
   }

   if (activate_callback) {
      g_signal_connect (item,
                        "activate",
                        G_CALLBACK (activate_callback),
                        callback_data);
   }

   if (item)  gtk_widget_show (GTK_WIDGET (item));

   return (item);
}
