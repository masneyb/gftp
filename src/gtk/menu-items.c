/*****************************************************************************/
/*  menu-items.c - menu callbacks                                            */
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

void
change_setting (gftp_window_data * wdata, int menuitem, GtkWidget * checkmenu)
{
  switch (menuitem)
    {
    case 1:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(1));
      break;
    case 2:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(0));
      break;
    case 3:
      current_wdata = &window1;
      other_wdata = &window2;
      if (wdata->request)
        update_window_info ();
      break;
    case 4:
      current_wdata = &window2;
      other_wdata = &window1;
      if (wdata->request)
        update_window_info ();
      break;
    }
}


void
tb_openurl_dialog (gpointer data)
{
  const char *edttxt;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("OpenURL"));
      return;
    }

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
               _("OpenURL: Operation canceled...you must enter a string\n"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    disconnect (current_wdata);
  else if (edttxt != NULL && *edttxt != '\0')
    toolbar_hostedit (NULL, NULL);
  else
    openurl_dialog (current_wdata);
}


static void
do_openurl (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  const char *tempstr;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("OpenURL"));
      return;
    }

  tempstr = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*tempstr == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
               _("OpenURL: Operation canceled...you must enter a string\n"));
      return;
    }

  if (GFTP_IS_CONNECTED (wdata->request))
    disconnect (wdata);

  if (gftp_parse_url (wdata->request, tempstr) == 0)
    {
      gtk_widget_destroy (ddata->dialog);
      ftp_connect (wdata, wdata->request, 1);
    }
  else
    gtk_widget_destroy (ddata->dialog);

  ddata->dialog = NULL; 
}


void
openurl_dialog (gpointer data)
{
  MakeEditDialog (_("Connect via URL"), _("Enter ftp url to connect to"),
                  NULL, 1, NULL, gftp_dialog_button_connect, do_openurl, data,
                  NULL, NULL);
}


void 
disconnect (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  gftp_delete_cache_entry (wdata->request, NULL, 1);
  gftp_disconnect (wdata->request);
  remove_files_window (wdata);
  update_window (wdata);
}


static void
dochange_filespec (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  GList * templist, * filelist;
  gftp_file * tempfle;
  const char *edttext;
  int num;

  wdata->show_selected = 0;

  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
               _("Change Filespec: Operation canceled...you must enter a string\n"));
      return;
    }
  if (wdata->filespec)
    g_free (wdata->filespec);
  wdata->filespec = g_strdup (edttext);

  filelist = wdata->files;
  templist = GTK_CLIST (wdata->listbox)->selection;
  num = 0;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      tempfle->was_sel = 1;
    }

  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  gtk_clist_clear (GTK_CLIST (wdata->listbox));
  templist = wdata->files;
  while (templist != NULL)
    {
      tempfle = templist->data;
      add_file_listbox (wdata, tempfle);
      templist = templist->next;
    }
  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
  update_window (wdata);
}


void 
change_filespec (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!check_status (_("Change Filespec"), wdata, 0, 0, 0, 1))
    return;

  MakeEditDialog (_("Change Filespec"), _("Enter the new file specification"),
                  wdata->filespec, 1, NULL, gftp_dialog_button_change, 
                  dochange_filespec, wdata, NULL, NULL);
}


static void
destroy_save_directory_listing (GtkWidget * widget, gftp_save_dir_struct * str)
{
  gtk_widget_destroy (str->filew);
  g_free (str);
}


static void
dosave_directory_listing (GtkWidget * widget, gftp_save_dir_struct * str)
{
  const char *filename;
  gftp_file * tempfle;
  GList * templist;
  char *tempstr;
  FILE * fd;
 

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (str->filew));
  if ((fd = fopen (filename, "w")) == NULL)
    {
      ftp_log (gftp_logging_misc, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), filename, 
               g_strerror (errno));
      return;
    }

  for (templist = str->wdata->files; 
       templist != NULL;
       templist = templist->next)
    {
      tempfle = templist->data;

      if (!tempfle->shown)
        continue;

      tempstr = gftp_gen_ls_string (tempfle, NULL, NULL);
      fprintf (fd, "%s\n", tempstr);
      g_free (tempstr);
    }

  fclose (fd);
}


void 
save_directory_listing (gpointer data)
{
  gftp_save_dir_struct * str;
  GtkWidget *filew;

  filew = gtk_file_selection_new (_("Save Directory Listing"));

  str = g_malloc (sizeof (*str));
  str->filew = filew;
  str->wdata = data;

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC (dosave_directory_listing), 
                      str);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
                      "clicked", 
                      GTK_SIGNAL_FUNC (destroy_save_directory_listing), str);
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), 
                      "clicked", 
                      GTK_SIGNAL_FUNC (destroy_save_directory_listing), str);

  gtk_window_set_wmclass (GTK_WINDOW(filew), "Save Directory Listing", "gFTP");
  gtk_widget_show (filew);
}


void
show_selected (gpointer data)
{
  GList * templist, * filelist;
  gftp_window_data * wdata;
  gftp_file * tempfle;
  int num;

  wdata = data;
  wdata->show_selected = 1;

  filelist = wdata->files;
  templist = GTK_CLIST (wdata->listbox)->selection;
  num = 0;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      tempfle->was_sel = 1;
    }

  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  gtk_clist_clear (GTK_CLIST (wdata->listbox));
  templist = wdata->files;
  while (templist != NULL)
    {
      tempfle = templist->data;
      add_file_listbox (wdata, tempfle);
      templist = templist->next;
    }
  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
  update_window (wdata);
}


void 
selectall (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 0;
  gtk_clist_select_all (GTK_CLIST (wdata->listbox));
}


void 
selectallfiles (gpointer data)
{
  gftp_window_data * wdata;
  gftp_file * tempfle;
  GList *templist;
  int i;

  wdata = data;
  wdata->show_selected = 0;
  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  i = 0;
  templist = wdata->files;
  while (templist != NULL)
    {
      tempfle = (gftp_file *) templist->data;
      if (tempfle->shown)
	{
	  if (tempfle->isdir)
	    gtk_clist_unselect_row (GTK_CLIST (wdata->listbox), i, 0);
	  else
	    gtk_clist_select_row (GTK_CLIST (wdata->listbox), i, 0);
          i++;
	}
      templist = templist->next;
    }
  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
}


void 
deselectall (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 0;
  gtk_clist_unselect_all (GTK_CLIST (wdata->listbox));
}


static void
dosite (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  const char *edttext;

  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
               _("SITE: Operation canceled...you must enter a string\n"));
      return;
    }

  if (check_reconnect (wdata) < 0)
    return;
  gftp_site_cmd (wdata->request, edttext);

  if (!GFTP_IS_CONNECTED (wdata->request))
    disconnect (wdata);
}


void
site_dialog (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!check_status (_("Site"), wdata, 0, 0, 0, wdata->request->site != NULL))
    return;

  MakeEditDialog (_("Site"), _("Enter site-specific command"), NULL, 1,
                  NULL, gftp_dialog_button_ok, dosite, wdata, NULL, NULL);
}


static void *
do_change_dir_thread (void * data)
{
  int success, sj;
  intptr_t network_timeout;
  gftp_window_data * wdata;

  wdata = data;

  if (wdata->request->use_threads)
    {
      sj = sigsetjmp (jmp_environment, 1);
      use_jmp_environment = 1;
    }
  else
    sj = 0;

  gftp_lookup_request_option (wdata->request, "network_timeout", 
                              &network_timeout);

  success = 0;
  if (sj == 0) 
    {
      if (network_timeout > 0)
        alarm (network_timeout);
      success = gftp_set_directory (wdata->request, wdata->request->directory);
      alarm (0);
    }
  else
    {
      gftp_disconnect (wdata->request);
      wdata->request->logging_function (gftp_logging_error,
                                        wdata->request,
                                        _("Operation canceled\n"));
    }

  if (wdata->request->use_threads)
    use_jmp_environment = 0;

  wdata->request->stopable = 0;
  return (GINT_TO_POINTER (success));
}


static int
do_change_dir (gftp_window_data * wdata, char *directory)
{
  char *olddir;
  int ret;

  if (directory != wdata->request->directory)
    {
      olddir = wdata->request->directory;
      wdata->request->directory = g_strdup (directory);
    }
  else
    olddir = NULL;

  ret = GPOINTER_TO_INT (generic_thread (do_change_dir_thread, wdata));

  if (!GFTP_IS_CONNECTED (wdata->request))
    {
      disconnect (wdata);
      if (olddir != NULL)
        g_free (olddir);
      return (-2);
    }

  if (ret != 0)
    {
      g_free (wdata->request->directory);
      wdata->request->directory = olddir;
    }
  else
    g_free (olddir);

  return (ret);
}


int
chdir_edit (GtkWidget * widget, gpointer data)
{
  gftp_window_data * wdata;
  const char *edttxt; 
  char *tempstr;

  wdata = data;
  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry));
  if (!GFTP_IS_CONNECTED (wdata->request) && *edttxt != '\0')
    {
      toolbar_hostedit (NULL, NULL);
      return (0);
    }

  if (!check_status (_("Chdir"), wdata, wdata->request->use_threads, 0, 0, 
                     wdata->request->chdir != NULL))
    return (FALSE);

  if ((tempstr = expand_path (edttxt)) == NULL)
    return (FALSE);

  if (check_reconnect (wdata) < 0)
    return (FALSE);

  if (do_change_dir (wdata, tempstr) == 0)
    {
      gtk_clist_freeze (GTK_CLIST (wdata->listbox));
      remove_files_window (wdata);
      ftp_list_files (wdata, 1);
      gtk_clist_thaw (GTK_CLIST (wdata->listbox));
      add_history (wdata->combo, wdata->history, wdata->histlen, tempstr);
    }

  g_free (tempstr);
  return (FALSE);
}


int
chdir_dialog (gpointer data)
{
  GList * templist, * filelist;
  gftp_window_data * wdata;
  char *newdir, *tempstr;
  gftp_file *tempfle;
  int num, ret;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, wdata->request->use_threads, 1, 0, 
                     wdata->request->chdir != NULL))
    return (0);

  filelist = wdata->files;
  templist = GTK_CLIST (wdata->listbox)->selection;
  num = 0;
  templist = get_next_selection (templist, &filelist, &num);
  tempfle = filelist->data;

  newdir = gftp_build_path (wdata->request->directory, tempfle->file, NULL);

  if ((tempstr = expand_path (newdir)) == NULL)
    {
      g_free (newdir);
      return (0);
    }

  g_free (newdir);

  if (check_reconnect (wdata) < 0)
    return (0);

  ret = 0; 
  if (do_change_dir (wdata, tempstr) == 0)
    {
      gtk_clist_freeze (GTK_CLIST (wdata->listbox));
      remove_files_window (wdata);
      ftp_list_files (wdata, 1);
      gtk_clist_thaw (GTK_CLIST (wdata->listbox));
      ret = 1;
    }
  g_free (tempstr);
  return (ret);
}


void 
clearlog (gpointer data)
{
  guint len;
#if GTK_MAJOR_VERSION == 1
  len = gtk_text_get_length (GTK_TEXT (logwdw));
  gtk_text_set_point (GTK_TEXT (logwdw), len);
  gtk_text_backward_delete (GTK_TEXT (logwdw), len);
#else
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len - 1);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);
#endif
}


void 
viewlog (gpointer data)
{
  char *tempstr, *txt, *pos;
  guint textlen;
  ssize_t len;
  int fd;
#if GTK_MAJOR_VERSION > 1
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
#endif

  tempstr = g_strconcat (g_get_tmp_dir (), "/gftp-view.XXXXXXXXXX", NULL);
  if ((fd = mkstemp (tempstr)) < 0)
    {
      ftp_log (gftp_logging_misc, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), tempstr, 
               g_strerror (errno));
      g_free (tempstr); 
      return;
    }
  chmod (tempstr, S_IRUSR | S_IWUSR);
  unlink (tempstr);
  
#if GTK_MAJOR_VERSION == 1
  textlen = gtk_text_get_length (GTK_TEXT (logwdw));
  txt = gtk_editable_get_chars (GTK_EDITABLE (logwdw), 0, -1);
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen - 1);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
#endif
  pos = txt;
  do 
    {
      if ((len = write (fd, pos, textlen)) == -1)
        { 
          ftp_log (gftp_logging_misc, NULL, 
                   _("Error: Error writing to %s: %s\n"), 
                   tempstr, g_strerror (errno));
          break;
        }
      textlen -= len;
      pos += len;
    } while (textlen > 0);

  lseek (fd, 0, SEEK_SET);
  view_file (tempstr, fd, 1, 0, 0, 1, NULL, NULL);
  close (fd);
  g_free (tempstr);
  g_free (txt);
}


static void
dosavelog (GtkWidget * widget, GtkFileSelection * fs)
{
  const char *filename;
  char *txt, *pos;
  guint textlen;
  ssize_t len;
  FILE *fd;
  int ok;
#if GTK_MAJOR_VERSION > 1
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
#endif

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));
  if ((fd = fopen (filename, "w")) == NULL)
    {
      ftp_log (gftp_logging_misc, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), filename, 
               g_strerror (errno));
      return;
    }

#if GTK_MAJOR_VERSION == 1
  textlen = gtk_text_get_length (GTK_TEXT (logwdw));
  txt = gtk_editable_get_chars (GTK_EDITABLE (logwdw), 0, -1);
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen - 1);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
#endif

  ok = 1;
  pos = txt;
  do
    {
      if ((len = write (fileno (fd), pos, textlen)) == -1)
        {
          ok = 0;
          ftp_log (gftp_logging_misc, NULL, 
                   _("Error: Error writing to %s: %s\n"), 
                   filename, g_strerror (errno));
          break;
        }

      textlen -= len;
      pos += len;
    } while (textlen > 0);

  if (ok)
    ftp_log (gftp_logging_misc, NULL,
             _("Successfully wrote the log file to %s\n"), filename);

  fclose (fd);
  g_free (txt);
}


void 
savelog (gpointer data)
{
  GtkWidget *filew;

  filew = gtk_file_selection_new (_("Save Log"));

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
                      "clicked", GTK_SIGNAL_FUNC (dosavelog), filew);
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filew)->ok_button),
                             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (filew));
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filew)->cancel_button), "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (filew));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filew), "gftp.log");
  gtk_window_set_wmclass (GTK_WINDOW(filew), "Save Log", "gFTP");
  gtk_widget_show (filew);
}


void 
clear_cache (gpointer data)
{
  gftp_clear_cache_files ();
}


void 
about_dialog (gpointer data)
{
  GtkWidget * tempwid, * notebook, * box, * label, * view, * vscroll,
            * dialog;
  char *tempstr, *no_license_agreement, *str, buf[255];
  size_t len;
  FILE * fd;
#if GTK_MAJOR_VERSION > 1
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  guint textlen;
#endif

  no_license_agreement = g_strdup_printf (_("Cannot find the license agreement file COPYING. Please make sure it is in either %s or in %s"), BASE_CONF_DIR, SHARE_DIR);

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("About gFTP"));
  gtk_container_border_width (GTK_CONTAINER
			      (GTK_DIALOG (dialog)->action_area), 5);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
#else
  dialog = gtk_dialog_new_with_buttons (_("About gFTP"), NULL, 0,
                                        GTK_STOCK_CLOSE,
                                        GTK_RESPONSE_CLOSE,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "about", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, gftp_version);
    }

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook, TRUE,
		      TRUE, 0);
  gtk_widget_show (notebook);

  box = gtk_vbox_new (TRUE, 5);
  gtk_container_border_width (GTK_CONTAINER (box), 10);
  gtk_widget_show (box);

  tempwid = toolbar_pixmap (dialog, "gftp-logo.xpm");
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  tempstr = g_strdup_printf (_("%s\nCopyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>\nOfficial Homepage: http://www.gftp.org/\nLogo by: Aaron Worley <planet_hoth@yahoo.com>\n"), gftp_version);
  str = _("Translated by");
  if (strcmp (str, "Translated by") != 0)
    {
      tempstr = g_realloc (tempstr, strlen (tempstr) + strlen (str) + 1);
      strcat (tempstr, str);
    }
  tempwid = gtk_label_new (tempstr);
  g_free (tempstr);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  label = gtk_label_new (_("About"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

  box = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box), 10);
  gtk_widget_show (box);

  tempwid = gtk_table_new (1, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (box), tempwid, TRUE, TRUE, 0);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  view = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (view), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (view), TRUE);

  gtk_table_attach (GTK_TABLE (tempwid), view, 0, 1, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                    0, 0);
  gtk_widget_show (view);

  vscroll = gtk_vscrollbar_new (GTK_TEXT (view)->vadj);
  gtk_table_attach (GTK_TABLE (tempwid), vscroll, 1, 2, 0, 1,
                    GTK_FILL, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
  gtk_widget_show (vscroll);
#else
  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);

  vscroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (vscroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (vscroll), view);
  gtk_widget_show (view);

  gtk_table_attach (GTK_TABLE (tempwid), vscroll, 0, 1, 0, 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
                    0, 0);
  gtk_widget_show (vscroll);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
#endif

  label = gtk_label_new (_("License Agreement"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, label);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("  Close  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      FALSE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);
#else
  g_signal_connect_swapped (GTK_OBJECT (dialog), "response",
                            G_CALLBACK (gtk_widget_destroy),
                            GTK_OBJECT (dialog));
#endif

  tempstr = g_strconcat ("/usr/share/common-licenses/GPL", NULL);
  if (access (tempstr, F_OK) != 0)
    {
      g_free (tempstr);
      tempstr = expand_path (SHARE_DIR "/COPYING");
      if (access (tempstr, F_OK) != 0)
	{
	  g_free (tempstr);
          tempstr = expand_path (BASE_CONF_DIR "/COPYING");
	  if (access (tempstr, F_OK) != 0)
	    {
#if GTK_MAJOR_VERSION == 1
	      gtk_text_insert (GTK_TEXT (view), NULL, NULL, NULL,
			       no_license_agreement, -1);
#else
              textlen = gtk_text_buffer_get_char_count (textbuf);
              gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen - 1);
              gtk_text_buffer_insert (textbuf, &iter, no_license_agreement, -1);
#endif
	      gtk_widget_show (dialog);
	      return;
	    }
	}
    }

  if ((fd = fopen (tempstr, "r")) == NULL)
    {
#if GTK_MAJOR_VERSION == 1
      gtk_text_insert (GTK_TEXT (view), NULL, NULL, NULL,
		       no_license_agreement, -1);
#else
      textlen = gtk_text_buffer_get_char_count (textbuf);
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen - 1);
      gtk_text_buffer_insert (textbuf, &iter, no_license_agreement, -1);
#endif
      gtk_widget_show (dialog);
      g_free (tempstr);
      return;
    }
  g_free (tempstr);

  memset (buf, 0, sizeof (buf));
  while ((len = fread (buf, 1, sizeof (buf) - 1, fd)))
    {
      buf[len] = '\0';
#if GTK_MAJOR_VERSION == 1
      gtk_text_insert (GTK_TEXT (view), NULL, NULL, NULL, buf, -1);
#else
      textlen = gtk_text_buffer_get_char_count (textbuf);
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, textlen - 1);
      gtk_text_buffer_insert (textbuf, &iter, buf, -1);
#endif
    }
  fclose (fd);
  gtk_widget_show (dialog);
  g_free (no_license_agreement);
  gftp_free_pixmap ("gftp-logo.xpm");
}


void 
compare_windows (gpointer data)
{
  gftp_file * curfle, * otherfle;
  GList * curlist, * otherlist;
  int row;

  if (!check_status (_("Compare Windows"), &window2, 1, 0, 0, 1))
    return;

  deselectall (&window1);
  deselectall (&window2);

  row = 0;
  curlist = window1.files;
  while (curlist != NULL)
    {
      curfle = curlist->data;
      if (!curfle->shown)
        {
          curlist = curlist->next;
          continue;
        }

      otherlist = window2.files;
      while (otherlist != NULL)
	{
          otherfle = otherlist->data;
          if (!otherfle->shown)
            {
              otherlist = otherlist->next;
              continue;
            }

          if (strcmp (otherfle->file, curfle->file) == 0 &&
              otherfle->isdir == curfle->isdir &&
              (curfle->isdir || otherfle->size == curfle->size))
	    break;

          otherlist = otherlist->next;
	}

      if (otherlist == NULL)
	gtk_clist_select_row (GTK_CLIST (window1.listbox), row, 0);
      row++;
      curlist = curlist->next;
    }

  row = 0;
  curlist = window2.files;
  while (curlist != NULL)
    {
      curfle = curlist->data;
      if (!curfle->shown)
        {
          curlist = curlist->next;
          continue;
        }

      otherlist = window1.files;
      while (otherlist != NULL)
	{
          otherfle = otherlist->data;
          if (!otherfle->shown)
            {
              otherlist = otherlist->next;
              continue;
            }

          if (strcmp (otherfle->file, curfle->file) == 0 &&
              otherfle->isdir == curfle->isdir &&
              (curfle->isdir || otherfle->size == curfle->size))
	    break;

          otherlist = otherlist->next;
	}

      if (otherlist == NULL)
	gtk_clist_select_row (GTK_CLIST (window2.listbox), row, 0);
      row++;
      curlist = curlist->next;
    }
}
