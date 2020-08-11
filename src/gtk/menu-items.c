/*****************************************************************************/
/*  menu-items.c - menu callbacks                                            */
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

static int combo_key2_pressed = 0;

void
show_selected (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 1;

  listbox_update_filelist (wdata);
  update_window (wdata);
}


static void
dochange_filespec (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  const char *edttext;

  wdata->show_selected = 0;

  edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttext == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
               _("Change Filespec: Operation canceled...you must enter a string\n"));
      return;
    }

  if (wdata->filespec)
    g_free (wdata->filespec);

  wdata->filespec = g_strdup (edttext);

  listbox_update_filelist (wdata);
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


void 
save_directory_listing (gpointer data)
{
  gftp_window_data * wdata = (gftp_window_data *)data;
  GtkWidget *filew;
  char current_dir[256];
  getcwd(current_dir, sizeof(current_dir));
  const char *filename;

  filew = gtk_file_chooser_dialog_new (_("Save Directory Listing"),
            main_window, //GTK_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET(xxx)))
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "gtk-save",   GTK_RESPONSE_ACCEPT,
            "gtk-cancel", GTK_RESPONSE_CANCEL,
            NULL );

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(filew), TRUE);
  gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(filew), current_dir);
  gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(filew), "Directory_Listing.txt");

  if (gtk_dialog_run (GTK_DIALOG(filew)) == GTK_RESPONSE_ACCEPT) {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filew));
    gftp_file * tempfle;
    GList * templist = wdata->files;
    char *tempstr;
    FILE * fd = fopen (filename, "w");
    if (fd)
    {
      for ( ;  templist; templist = templist->next)
      {
        tempfle = templist->data;
        if (!tempfle->shown)
          continue;
        tempstr = gftp_gen_ls_string (NULL, tempfle, NULL, NULL);
        fprintf (fd, "%s\n", tempstr);
        g_free (tempstr);
      }
      fclose (fd);
    }
    else {
      ftp_log (gftp_logging_error, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), filename,  g_strerror (errno));
    }
  }

  gtk_widget_destroy (filew);
}

gboolean
chdir_edit (GtkWidget * widget, GdkEventKey *event, gpointer data )
{
  if (event->type == GDK_KEY_PRESS) {
    if (event->keyval == GDK_KEY_Return)
       combo_key2_pressed = 1;
    return FALSE;
  }
  else if (event->type == GDK_KEY_RELEASE) {
    if (combo_key2_pressed == 0)
      return FALSE;
  }
  else {
    return FALSE;
  }

  if (event->keyval != GDK_KEY_Return) {
    return FALSE;
  }

  gftp_window_data * wdata;
  const char *edttxt; 
  char *tempstr;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0,
                     wdata->request->chdir != NULL))
    return (0);

  if (check_reconnect (wdata) < 0)
    return (0);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(wdata->combo));

  if (!GFTP_IS_CONNECTED (wdata->request) && *edttxt != '\0')
    {
      toolbar_hostedit ();
      return (0);
    }

  if ((tempstr = gftp_expand_path (wdata->request, edttxt)) == NULL)
    return (FALSE);

  if (gftpui_run_chdir (wdata, tempstr))
    add_history (wdata->combo, wdata->history, wdata->histlen, edttxt);

  g_free (tempstr);
  return (0);
}


void 
clearlog (gpointer data)
{
  gint len;

  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);
}


void 
viewlog (gpointer data)
{
  char *tempstr, *txt, *pos;
  gint textlen;
  ssize_t len;
  int fd;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  tempstr = g_strconcat (g_get_tmp_dir (), "/gftp-view.XXXXXXXXXX", NULL);
  if ((fd = mkstemp (tempstr)) < 0)
    {
      ftp_log (gftp_logging_error, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), tempstr, 
               g_strerror (errno));
      g_free (tempstr); 
      return;
    }
  chmod (tempstr, S_IRUSR | S_IWUSR);
  
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);

  /* gtk_text_buffer_get_char_count() returns the number of characters,
     not bytes. So get the number of bytes that need to be written out */
  textlen = strlen (txt);

  pos = txt;

  while (textlen > 0)
    {
      if ((len = write (fd, pos, textlen)) == -1)
        { 
          ftp_log (gftp_logging_error, NULL, 
                   _("Error: Error writing to %s: %s\n"), 
                   tempstr, g_strerror (errno));
          break;
        }
      textlen -= len;
      pos += len;
    }

  fsync (fd);
  lseek (fd, 0, SEEK_SET);
  view_file (tempstr, fd, 1, 1, 0, 1, NULL, NULL);
  close (fd);

  g_free (tempstr);
  g_free (txt);
}


void 
savelog (gpointer data)
{
  GtkWidget *filew;
  const char *filename = NULL;
  char current_dir[256];
  getcwd(current_dir, sizeof(current_dir));

  filew = gtk_file_chooser_dialog_new (_("Save Log"),
            main_window, //GTK_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET(xxx)))
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "gtk-save",   GTK_RESPONSE_ACCEPT,
            "gtk-cancel", GTK_RESPONSE_CANCEL,
            NULL );

  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(filew), TRUE);
  gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(filew), current_dir);
  gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(filew), "gftp.log");

  if (gtk_dialog_run (GTK_DIALOG(filew)) != GTK_RESPONSE_ACCEPT) {
     gtk_widget_destroy (filew);
     return;
  }

  filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filew));

  // do save log
  char *txt, *pos;
  gint textlen;
  ssize_t len;
  FILE *fd;
  int ok;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  if ((fd = fopen (filename, "w")) == NULL) {
      ftp_log (gftp_logging_error, NULL, 
               _("Error: Cannot open %s for writing: %s\n"), filename, 
               g_strerror (errno));
      return;
  }
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));
  textlen = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, textlen);
  txt = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);

  /* gtk_text_buffer_get_char_count() returns the number of characters,
     not bytes. So get the number of bytes that need to be written out */
  textlen = strlen (txt);

  ok = 1;
  pos = txt;
  do
  {
      if ((len = write (fileno (fd), pos, textlen)) == -1) {
          ok = 0;
          ftp_log (gftp_logging_error, NULL, 
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

  gtk_widget_destroy (filew);
}


void 
clear_cache (gpointer data)
{
  gftp_clear_cache_files ();
}


void 
about_dialog (gpointer data)
{
    GtkWidget *w;
    const gchar * authors[] =
    {
        "Brian Masney <masneyb@gftp.org>",
        NULL
    };
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    gchar * translators = _("Translated by");

    GdkPixbuf * logo = NULL;
    char * logopath = get_image_path ("gftp-logo.xpm"); /* misc-gtk.c */
    if (logopath) {
       logo = gdk_pixbuf_new_from_file (logopath, NULL);
       g_free (logopath);
    }

    /* Create and initialize the dialog. */
    w = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                      "version",      VERSION,
                      "program-name", "gFTP",
                      "copyright",    "Copyright (C) 1998-2020",
                      "comments",     _("A multithreaded ftp client"),
                      "license",      "This program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.",
                      "website",      "http://www.gftp.org",
                      "authors",      authors,
                      "translator-credits", translators,
                      "logo",         logo,
                      NULL);
    gtk_container_set_border_width (GTK_CONTAINER (w), 2);
    set_window_icon (GTK_WINDOW (w), NULL);
    gtk_window_set_transient_for (GTK_WINDOW (w), main_window);
    gtk_window_set_modal (GTK_WINDOW (w), TRUE);
    gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER_ON_PARENT);

    /* Display the dialog, wait for the user to close it, then destroy it. */
    gtk_dialog_run (GTK_DIALOG (w));
    gtk_widget_destroy (w);
}


static void
_do_compare_windows (gftp_window_data * win1, gftp_window_data * win2)
{
  gftp_file * curfle, * otherfle;
  GList * curlist, * otherlist;
  int row, curdir, othdir;

  row = 0;
  curlist = win1->files;
  while (curlist != NULL)
    {
      curfle = curlist->data;
      if (!curfle->shown)
        {
          curlist = curlist->next;
          continue;
        }

      otherlist = win2->files;
      while (otherlist != NULL)
	{
          otherfle = otherlist->data;
          if (!otherfle->shown)
            {
              otherlist = otherlist->next;
              continue;
            }

          curdir = S_ISDIR (curfle->st_mode);
          othdir = S_ISDIR (otherfle->st_mode);

          if (strcmp (otherfle->file, curfle->file) == 0 &&
              curdir == othdir &&
              (curdir || otherfle->size == curfle->size))
	    break;

          otherlist = otherlist->next;
	}

      if (!otherlist) {
         listbox_select_row (win1, row);
      }

      row++;
      curlist = curlist->next;
    }
}


void 
compare_windows (gpointer data)
{
  if (!check_status (_("Compare Windows"), &window2, 1, 0, 0, 1))
    return;

  listbox_deselect_all(&window1);
  listbox_deselect_all(&window2);

  _do_compare_windows (&window1, &window2);
  _do_compare_windows (&window2, &window1);
}

// ---------------------------------------------------------------

static void
do_delete_dialog (gpointer data)
{
  gftp_file * tempfle, * newfle;
  GList * templist, *igl;
  gftp_transfer * transfer;
  gftp_window_data * wdata = (gftp_window_data *) data;
  int ret;

  if (!check_status (_("Delete"), wdata,
      gftpui_common_use_threads (wdata->request), 0, 1, 1))
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = gftp_copy_request (wdata->request);
  transfer->fromwdata = wdata;

  templist = listbox_get_selected_files(wdata);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
     tempfle = (gftp_file *) igl->data;
     if (strcmp (tempfle->file, "..") == 0 ||
         strcmp (tempfle->file, ".") == 0)
            continue;
     newfle = copy_fdata (tempfle);
     transfer->files = g_list_append (transfer->files, newfle);
  }
  g_list_free (templist);

  if (transfer->files == NULL) {
      free_tdata (transfer);
      return;
  }

  gftp_swap_socks (transfer->fromreq, wdata->request);
  ret = gftp_gtk_get_subdirs (transfer);
  gftp_swap_socks (wdata->request, transfer->fromreq);

  if (!GFTP_IS_CONNECTED (wdata->request)) {
      gftpui_disconnect (wdata);
      return;
  }

  if (!ret)
    return;

  // yesCB
  gftpui_callback_data * cdata;

  g_return_if_fail (transfer != NULL);
  g_return_if_fail (transfer->files != NULL);

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->files = transfer->files;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_delete;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
  free_tdata (transfer); //_gftp_gtk_free_del_data (transfer);
}

void
delete_dialog (gpointer data)
{
  gftp_window_data * wdata = (gftp_window_data *) data;
  if (!wdata) {
     return;
  }
  long int num = listbox_num_selected (wdata);
  if (num > 0) {
      char * tempstr = g_strdup_printf (_("Are you sure you want to delete the %ld selected item(s)"), num);
      MakeYesNoDialog (_("Delete Files/Directories"), tempstr,
                   do_delete_dialog, data, NULL, NULL);
      g_free (tempstr);  
  }
}
