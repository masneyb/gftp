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

static void
update_window_listbox (gftp_window_data * wdata)
{
  GList * templist, * filelist;
  gftp_file * tempfle;
  int num;

  filelist = wdata->files;
  templist = gftp_gtk_get_list_selection (wdata);
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
  update_window_listbox (wdata);
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
      ftp_log (gftp_logging_error, NULL, 
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

      tempstr = gftp_gen_ls_string (NULL, tempfle, NULL, NULL);
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

  str = g_malloc0 (sizeof (*str));
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
  gftp_window_data * wdata;

  wdata = data;
  wdata->show_selected = 1;
  update_window_listbox (wdata);
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
	  if (S_ISDIR (tempfle->st_mode))
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


int
chdir_edit (GtkWidget * widget, gpointer data)
{
  gftp_window_data * wdata;
  const char *edttxt; 
  char *tempstr;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0,
                     wdata->request->chdir != NULL))
    return (0);

  if (check_reconnect (wdata) < 0)
    return (0);

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry));

  if (!GFTP_IS_CONNECTED (wdata->request) && *edttxt != '\0')
    {
      toolbar_hostedit (NULL, NULL);
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


static void
dosavelog (GtkWidget * widget, GtkFileSelection * fs)
{
  const char *filename;
  char *txt, *pos;
  gint textlen;
  ssize_t len;
  FILE *fd;
  int ok;
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));
  if ((fd = fopen (filename, "w")) == NULL)
    {
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
      if ((len = write (fileno (fd), pos, textlen)) == -1)
        {
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
    const gchar * authors[] =
    {
        "Brian Masney <masneyb@gftp.org>",
        NULL
    };
    /* TRANSLATORS: Replace this string with your names, one name per line. */
    gchar * translators = _("Translated by");
    char * logopath = get_image_path ("gftp-logo.xpm", 0); /* misc-gtk.c */

    /* Create and initialize the dialog. */
    GtkWidget * about_dlg = gtk_about_dialog_new();
    gtk_container_set_border_width(GTK_CONTAINER(about_dlg), 2);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dlg), VERSION);
    gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(about_dlg), "gFTP");
    if (logopath) {
       gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dlg), gdk_pixbuf_new_from_file(logopath, NULL));
       g_free (logopath);
    }
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dlg), "Copyright (C) 1998-2020");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dlg), _("A multithreaded ftp client"));
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dlg), "This program is free software; you can redistribute it and/or\nmodify it under the terms of the GNU General Public License\nas published by the Free Software Foundation; either version 2\nof the License, or (at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\nYou should have received a copy of the GNU General Public License\nalong with this program; if not, write to the Free Software\nFoundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dlg), "http://www.gftp.org");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dlg), authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about_dlg), translators);

    /* Display the dialog, wait for the user to click OK, and dismiss the dialog. */
    gtk_dialog_run(GTK_DIALOG(about_dlg));
    gtk_widget_destroy(about_dlg);
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

      if (otherlist == NULL)
	gtk_clist_select_row (GTK_CLIST (win1->listbox), row, 0);
      row++;
      curlist = curlist->next;
    }
}


void 
compare_windows (gpointer data)
{
  if (!check_status (_("Compare Windows"), &window2, 1, 0, 0, 1))
    return;

  deselectall (&window1);
  deselectall (&window2);

  _do_compare_windows (&window1, &window2);
  _do_compare_windows (&window2, &window1);
}
