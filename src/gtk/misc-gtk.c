/*****************************************************************************/
/*  misc-gtk.c - misc stuff for the gtk+ 1.2 port of gftp                    */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

#include <gftp-gtk.h>
static const char cvsid[] = "$Id$";

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
  char *logstr;
  gint delsize;
  va_list argp;
  guint len;
#if GTK_MAJOR_VERSION == 1
  gftp_color * color;
  GdkColor fore;
#else
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  const char *descr;
  char *utf8_str;
#endif

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

#if GTK_MAJOR_VERSION > 1
  if ((utf8_str = gftp_string_to_utf8 (request, logstr)) != NULL)
    {
      if (free_logstr)
        g_free (logstr);
      else
        free_logstr = 1;

      logstr = utf8_str;
    }
#endif

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

#if GTK_MAJOR_VERSION == 1
  switch (level)
    {
      case gftp_logging_send:
        gftp_lookup_global_option ("send_color", &color);
        break;
      case gftp_logging_recv:
        gftp_lookup_global_option ("recv_color", &color);
        break;
      case gftp_logging_error:
        gftp_lookup_global_option ("error_color", &color);
        break;
      default:
        gftp_lookup_global_option ("misc_color", &color);
        break;
    }

  memset (&fore, 0, sizeof (fore));
  fore.red = color->red;
  fore.green = color->green;
  fore.blue = color->blue;

  gtk_text_freeze (GTK_TEXT (logwdw));
  gtk_text_insert (GTK_TEXT (logwdw), NULL, &fore, NULL, logstr, -1);

  len = gtk_text_get_length (GTK_TEXT (logwdw));
  if (max_log_window_size > 0 && len > max_log_window_size)
    {
      delsize = len - max_log_window_size;
      gtk_text_set_point (GTK_TEXT (logwdw), delsize);
      gtk_text_backward_delete (GTK_TEXT (logwdw), delsize);
      len = max_log_window_size;
    }
  gtk_text_set_point (GTK_TEXT (logwdw), len);

  gtk_text_thaw (GTK_TEXT (logwdw));

  if (upd)
    gtk_adjustment_set_value (logwdw_vadj, logwdw_vadj->upper);
#else
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
#endif

  if (free_logstr)
    g_free (logstr);
}


void
update_window_info (void)
{
  char *tempstr, empty[] = "";
  GtkWidget * tempwid;
  int i;

  if (current_wdata->request != NULL)
    {
      if ((tempstr = current_wdata->request->hostname) == NULL)
        tempstr = empty;
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry), tempstr);

      if ((tempstr = current_wdata->request->username) == NULL)
        tempstr = empty;
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (useredit)->entry), tempstr);

      if ((tempstr = current_wdata->request->password) == NULL)
        tempstr = empty;
      gtk_entry_set_text (GTK_ENTRY (passedit), tempstr);

      if (current_wdata->request->port != 0)
        {
          tempstr = g_strdup_printf ("%d", current_wdata->request->port);
          gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (portedit)->entry), tempstr);
          g_free (tempstr);
        }

     for (i=0; gftp_protocols[i].init != NULL; i++)
       {
         if (current_wdata->request->init == gftp_protocols[i].init)
           {
             gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), i);
             break;
           }
       }
    }

  update_window (&window1);
  update_window (&window2);

  tempwid = gtk_item_factory_get_widget (factory, menus[tools_start+2].path);
  gtk_widget_set_sensitive (tempwid, GFTP_IS_CONNECTED (window1.request) 
			    && GFTP_IS_CONNECTED (window2.request));
}


static void
set_menu_sensitive (gftp_window_data * wdata, char *path, int sensitive)
{
  GtkWidget * tempwid;
  char * pos;

  tempwid = NULL;

  if (factory != NULL)
    tempwid = gtk_item_factory_get_widget (factory, path);
  if (tempwid)
    gtk_widget_set_sensitive (tempwid, sensitive);

  if ((pos = strchr (path + 1, '/')) == NULL)
    pos = path;

  if (wdata->ifactory)
    tempwid = gtk_item_factory_get_widget (wdata->ifactory, pos);
  if (tempwid)
    gtk_widget_set_sensitive (tempwid, sensitive);
}


void
update_window (gftp_window_data * wdata)
{
  char *dir, *tempstr, *temp1str, *fspec, *utf8_directory;
  int connected, start;

  connected = GFTP_IS_CONNECTED (wdata->request);
  if (connected)
    {
      fspec = wdata->show_selected ? "Selected" : strcmp (wdata->filespec, "*") == 0 ?  _("All Files") : wdata->filespec;

      if ((temp1str = wdata->request->hostname) == NULL ||
          wdata->request->protonum == GFTP_LOCAL_NUM)
	temp1str = "";
      tempstr = g_strconcat (temp1str, *temp1str == '\0' ? "[" : " [",
		     gftp_protocols[wdata->request->protonum].name,
		     wdata->request->cached ? _("] (Cached) [") : "] [",
                     fspec, "]", current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);

      utf8_directory = NULL;
      if ((dir = wdata->request->directory) == NULL)
        temp1str = "";
      else
        {
          utf8_directory = gftp_string_to_utf8 (wdata->request, 
                                                wdata->request->directory);
          if (utf8_directory != NULL)
            temp1str = utf8_directory;
          else
            temp1str = dir;
        }

      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry),temp1str);

      if (utf8_directory != NULL)
        g_free (utf8_directory);
    }
  else if (wdata->hoststxt != NULL)
    {
      tempstr = g_strconcat (_("Not connected"), 
                             current_wdata == wdata ? "*" : "", NULL);
      gtk_label_set (GTK_LABEL (wdata->hoststxt), tempstr);
      g_free (tempstr);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry), "");
    }

  if (wdata == &window1)
    start = local_start;
  else
    start = remote_start;

  set_menu_sensitive (wdata, menus[start + 3].path, connected && 
                      strcmp (wdata->request->url_prefix, "file") != 0);
  set_menu_sensitive (wdata, menus[start + 5].path, connected);
  set_menu_sensitive (wdata, menus[start + 6].path, connected);
  set_menu_sensitive (wdata, menus[start + 7].path, connected);
  set_menu_sensitive (wdata, menus[start + 8].path, connected);
  set_menu_sensitive (wdata, menus[start + 9].path, connected);
  set_menu_sensitive (wdata, menus[start + 11].path, connected);
  set_menu_sensitive (wdata, menus[start + 12].path, connected &&
                      wdata->request->site != NULL);
  set_menu_sensitive (wdata, menus[start + 13].path, connected &&
                      wdata->request->chdir!= NULL);
  set_menu_sensitive (wdata, menus[start + 14].path, connected &&
                      wdata->request->chmod != NULL);
  set_menu_sensitive (wdata, menus[start + 15].path, connected &&
                      wdata->request->mkdir != NULL);
  set_menu_sensitive (wdata, menus[start + 16].path, connected &&
                      wdata->request->rename != NULL);
  set_menu_sensitive (wdata, menus[start + 17].path, connected &&
                      wdata->request->rmdir != NULL &&
                      wdata->request->rmfile != NULL);
  set_menu_sensitive (wdata, menus[start + 18].path, connected &&
                      wdata->request->get_file != NULL);
  set_menu_sensitive (wdata, menus[start + 19].path, connected &&
                      wdata->request->get_file != NULL);
  set_menu_sensitive (wdata, menus[start + 20].path, connected);
}  


GtkWidget *
toolbar_pixmap (GtkWidget * widget, char *filename)
{
  gftp_graphic * graphic;
  GtkWidget *pix;

  if (filename == NULL || *filename == '\0')
    return (NULL);

  graphic = open_xpm (widget, filename);

  if (graphic == NULL)
    return (NULL);
    
  if ((pix = gtk_pixmap_new (graphic->pixmap, graphic->bitmap)) == NULL)
    return (NULL);

  gtk_widget_show (pix);
  return (pix);
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

  if ((exfile = get_xpm_path (filename, 0)) == NULL)
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

#if GTK_MAJOR_VERSION == 1
  gdk_pixmap_unref (graphic->pixmap);
  gdk_bitmap_unref (graphic->bitmap);
#else
  g_object_unref (graphic->pixmap);
  g_object_unref (graphic->bitmap);
#endif

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
check_status (char *name, gftp_window_data *wdata, int check_other_stop,
              int only_one, int at_least_one, int func)
{
  gftp_window_data * owdata;

  owdata = wdata == &window1 ? &window2 : &window1;

  if (wdata->request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: Please hit the stop button first to do anything else\n"),
	       name);
      return (0);
    }

  if (check_other_stop && owdata->request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: Please hit the stop button first to do anything else\n"),
	       name);
      return (0);
    }

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: Not connected to a remote site\n"), name);
      return (0);
    }

  if (!func)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: This feature is not available using this protocol\n"),
	       name);
      return (0);
    }

  if (only_one && !IS_ONE_SELECTED (wdata))
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: You must only have one item selected\n"), name);
      return (0);
    }

  if (at_least_one && !only_one && IS_NONE_SELECTED (wdata))
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("%s: You must have at least one item selected\n"), name);
      return (0);
    }
  return (1);
}


static gchar *
gftp_item_factory_translate (const char *path, gpointer func_data)
{
  const gchar *strip_prefix = func_data;
  const char *result;
  
  if (strip_prefix)
    {
      char *tmp_path = g_strconcat (strip_prefix, path, NULL);
      result = gettext (tmp_path);
      if (result == tmp_path)
        result = path;
      g_free (tmp_path);
    }
  else
    result = gettext (path);

  return (char *)result;
}


GtkItemFactory *
item_factory_new (GtkType container_type, const char *path,
		  GtkAccelGroup *accel_group, const char *strip_prefix)
{
  GtkItemFactory *result = gtk_item_factory_new (container_type, path, accel_group);
  gchar *strip_prefix_dup = g_strdup (strip_prefix);
  
  gtk_item_factory_set_translate_func (result, gftp_item_factory_translate,
				       strip_prefix_dup, NULL);

  if (strip_prefix_dup)
    gtk_object_set_data_full (GTK_OBJECT (result), "gftp-strip-prefix",
			      strip_prefix_dup, (GDestroyNotify)g_free);

  return result;
}


void
create_item_factory (GtkItemFactory * ifactory, guint n_entries,
		     GtkItemFactoryEntry * entries, gpointer callback_data)
{
  int i;
  const char *strip_prefix = gtk_object_get_data (GTK_OBJECT (ifactory), "gftp-strip-prefix");
  int strip_prefix_len = 0;

  if (strip_prefix)
    strip_prefix_len = strlen (strip_prefix);

  for (i = 0; i < n_entries; i++)
    {
      GtkItemFactoryEntry dummy_item = entries[i];
      if (strip_prefix && strncmp (entries[i].path, strip_prefix, strip_prefix_len) == 0)
	dummy_item.path += strip_prefix_len;
      
      gtk_item_factory_create_item (ifactory, &dummy_item, callback_data, 1);
    }
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
  gtk_combo_set_popdown_strings (GTK_COMBO (widget), *history);
}


int
check_reconnect (gftp_window_data *wdata)
{
  return (wdata->request->cached && wdata->request->datafd < 0 && 
          !wdata->request->always_connected &&
	  !ftp_connect (wdata, wdata->request, 0) ? -1 : 0);
}


void
add_file_listbox (gftp_window_data * wdata, gftp_file * fle)
{
  char *add_data[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL }, *pos;
  gftp_config_list_vars * tmplistvar;
  int clist_num;
  intptr_t show_hidden_files;
  gftp_file_extensions * tempext;
  char *tempstr, *str;
  GdkBitmap * bitmap;
  GList * templist;
  GdkPixmap * pix;
  size_t stlen;

  gftp_lookup_request_option (wdata->request, "show_hidden_files", 
                              &show_hidden_files);

  if (wdata->show_selected)
    {
      fle->shown = fle->was_sel;
      if (!fle->shown)
        return;
    }
  else if ((!show_hidden_files && *fle->file == '.' && 
            strcmp (fle->file, "..") != 0) ||
           !gftp_match_filespec (fle->file, wdata->filespec))
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
  else if (fle->islink && fle->isdir)
    gftp_get_pixmap (wdata->listbox, "linkdir.xpm", &pix, &bitmap);
  else if (fle->islink)
    gftp_get_pixmap (wdata->listbox, "linkfile.xpm", &pix, &bitmap);
  else if (fle->isdir)
    gftp_get_pixmap (wdata->listbox, "dir.xpm", &pix, &bitmap);
  else if (fle->isexe)
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

  if (fle->utf8_file)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 1, 
                        fle->utf8_file);
  else if (fle->file)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 1, fle->file);

  if (fle->attribs && (*fle->attribs == 'b' || *fle->attribs == 'c'))
    tempstr = g_strdup_printf ("%d, %d", major (fle->size),
                               minor (fle->size));
  else
    tempstr = insert_commas (fle->size, NULL, 0);

  gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 2, tempstr);
  g_free (tempstr);

  if (fle->user)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 3, fle->user);
  if (fle->group)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 4, fle->group);
  if ((str = ctime (&fle->datetime)))
    {
      if ((pos = strchr (str, '\n')) != NULL)
        *pos = '\0';
      gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 5, str);
    }
  if (fle->attribs)
    gtk_clist_set_text (GTK_CLIST (wdata->listbox), clist_num, 6, fle->attribs);

}


#if GTK_MAJOR_VERSION == 1
static void
ok_dialog_response (GtkWidget * widget, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }
 
  if (ddata->yesfunc != NULL)
    ddata->yesfunc (ddata->yespointer, ddata);

  if (ddata->edit != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}


static void
cancel_dialog_response (GtkWidget * widget, gftp_dialog_data * ddata)
{
  if (ddata->edit == NULL)
    {
      gtk_widget_destroy (ddata->dialog);
      ddata->dialog = NULL;
      ddata->checkbox = NULL;
    }
 
  if (ddata->nofunc != NULL)
    ddata->nofunc (ddata->nopointer, ddata);

  if (ddata->edit != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}
#else
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

  if (ddata->edit != NULL)
    gtk_widget_destroy (ddata->dialog);

  g_free (ddata);
}
#endif


static gint
dialog_keypress (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (event->type != GDK_KEY_PRESS)
    return (FALSE);

  if (event->keyval == GDK_KP_Enter || event->keyval == GDK_Return)
    {
#if GTK_MAJOR_VERSION == 1
      ok_dialog_response (widget, data);
#else
      dialog_response (widget, GTK_RESPONSE_YES, data);
#endif
      return (TRUE);
    }
  else if (event->keyval == GDK_Escape)
    {
#if GTK_MAJOR_VERSION == 1
      cancel_dialog_response (widget, data);
#else
      dialog_response (widget, GTK_RESPONSE_NO, data);
#endif
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

  ddata = g_malloc (sizeof (*ddata));
  ddata->yesfunc = okfunc;
  ddata->yespointer = okptr;
  ddata->nofunc = cancelfunc;
  ddata->nopointer = cancelptr;

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), diagtxt);
  gtk_grab_add (dialog);
  gtk_container_border_width (GTK_CONTAINER
			      (GTK_DIALOG (dialog)->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
#else
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
                                        yes_text,
                                        GTK_RESPONSE_YES,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_NO,
                                        NULL);
#endif
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "edit", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, gftp_version);
    }

  ddata->dialog = dialog;

  tempwid = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, TRUE,
		      TRUE, 0);
  gtk_widget_show (tempwid);

  ddata->edit = gtk_entry_new ();
  gtk_signal_connect (GTK_OBJECT (ddata->edit), "key_press_event",
                      GTK_SIGNAL_FUNC (dialog_keypress), (gpointer) ddata);

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
      
#if GTK_MAJOR_VERSION == 1
  switch (okbutton)
    {
      case gftp_dialog_button_ok:
        yes_text = GTK_STOCK_OK;
        break;
      case gftp_dialog_button_create:
        yes_text = _("Add");
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
        yes_text = "";
        break;
    }

  tempwid = gtk_button_new_with_label (yes_text);
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (ok_dialog_response),
                      ddata);
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (cancel_dialog_response),
                      ddata);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);
#endif

  gtk_widget_show (dialog);
}


void
MakeYesNoDialog (char *diagtxt, char *infotxt, 
                 void (*yesfunc) (), gpointer yespointer, 
                 void (*nofunc) (), gpointer nopointer)
{
  GtkWidget * text, * dialog;
  gftp_dialog_data * ddata;
#if GTK_MAJOR_VERSION == 1
  GtkWidget * tempwid;
#endif

  ddata = g_malloc (sizeof (*ddata));
  ddata->yesfunc = yesfunc;
  ddata->yespointer = yespointer;
  ddata->nofunc = nofunc;
  ddata->nopointer = nopointer;

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_grab_add (dialog);
  gtk_window_set_title (GTK_WINDOW (dialog), diagtxt);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 
                              5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
#else
  dialog = gtk_dialog_new_with_buttons (_(diagtxt), NULL, 0,
                                        GTK_STOCK_YES,
                                        GTK_RESPONSE_YES,
                                        GTK_STOCK_NO,
                                        GTK_RESPONSE_NO,
                                        NULL);
#endif
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "yndiag", "gFTP");
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, gftp_version);
    }

  ddata->dialog = dialog;

  text = gtk_label_new (infotxt);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), text, TRUE, TRUE, 0);
  gtk_widget_show (text);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("  Yes  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
                      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (ok_dialog_response), ddata);

  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  No  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
                      FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (cancel_dialog_response), ddata);
  gtk_widget_show (tempwid);

#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (dialog_response), ddata);
#endif

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
#if GTK_MAJOR_VERSION > 1
      gtk_window_set_decorated (GTK_WINDOW (dialog), 0);
#endif
      gtk_grab_add (dialog);
      gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                          GTK_SIGNAL_FUNC (delete_event), NULL);
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
      gtk_signal_connect (GTK_OBJECT (stopwid), "clicked",
                          GTK_SIGNAL_FUNC (trans_stop_button), transfer);
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
    val = 0;
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
get_xpm_path (char *filename, int quit_on_err)
{
  char *tempstr, *exfile;

  tempstr = g_strconcat (BASE_CONF_DIR, "/", filename, NULL);
  exfile = expand_path (tempstr);
  g_free (tempstr);
  if (access (exfile, F_OK) != 0)
    {
      g_free (exfile);
      tempstr = g_strconcat (SHARE_DIR, "/", filename, NULL);
      exfile = expand_path (tempstr);
      g_free (tempstr);
      if (access (exfile, F_OK) != 0)
	{
	  g_free (exfile);
	  exfile = g_strconcat ("/usr/share/icons/", filename, NULL);
	  if (access (exfile, F_OK) != 0)
	    {
	      g_free (exfile);
	      if (!quit_on_err)
		return (NULL);

	      printf (_("gFTP Error: Cannot find file %s in %s or %s\n"),
		      filename, SHARE_DIR, BASE_CONF_DIR);
	      exit (1);
	    }
	}
    }
  return (exfile);
}

