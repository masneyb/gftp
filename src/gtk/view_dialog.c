/*****************************************************************************/
/*  view_dialog.c - view dialog box and ftp routines                         */
/*  Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>                  */
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

#include "gftp-gtk.h"

static gftp_viewedit_data * fork_process 	( char *proc, 
						  char *filename,
						  int fd,
						  char *remote_filename,
						  int viewedit, 
                                                  int del_file,
                                                  int dontupload,
						  gftp_window_data * wdata );

static gftp_file * curfle;

void
view_dialog (gpointer data)
{
  GList * templist, * filelist, * newfile;
  gftp_window_data * fromwdata, * towdata;
  gftp_file * new_fle;
  int num, fd;

  fromwdata = data;
  towdata = fromwdata == &window1 ? &window2 : &window1;
  if (!check_status (_("View"), fromwdata, 0, 1, 1, 1))
    return;

  templist = GTK_CLIST (fromwdata->listbox)->selection;
  num = 0;
  filelist = fromwdata->files;
  templist = get_next_selection (templist, &filelist, &num);
  curfle = filelist->data;

  if (curfle->isdir)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("View: %s is a directory. Cannot view it.\n"), curfle->file);
      return;
    }

  if (strcmp (GFTP_GET_PROTOCOL_NAME (fromwdata->request), "Local") == 0)
    view_file (curfle->file, 0, 1, 0, 1, 1, NULL, fromwdata);
  else
    {
      new_fle = copy_fdata (curfle);
      if (new_fle->destfile)
        g_free (new_fle->destfile);
      new_fle->destfile = g_strconcat (tmp_directory, "/gftp-view.XXXXXX", NULL);
      if ((fd = mkstemp (new_fle->destfile)) < 0)
        {
          ftp_log (gftp_logging_misc, NULL, _("Error: Cannot open %s: %s\n"),
                   new_fle->destfile, g_strerror (errno));
          free_fdata (new_fle);
          return;
        }

      chmod (new_fle->destfile, S_IRUSR | S_IWUSR);
      if ((new_fle->fd = fdopen (fd, "rw+")) == NULL)
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Cannot fdopen() socket: %s\n"),
                   g_strerror (errno));
          return;
        }

      new_fle->is_fd = 1;
      new_fle->done_view = 1;
      new_fle->done_rm = 1;
      newfile = g_list_append (NULL, new_fle); 
      add_file_transfer (fromwdata->request, towdata->request,
                         fromwdata, towdata, newfile, 1);
    }
}


void
edit_dialog (gpointer data)
{
  gftp_window_data * fromwdata, * towdata;
  GList * templist, * filelist, * newfile;
  gftp_file * new_fle;
  int num, fd;

  fromwdata = data;
  towdata = fromwdata == &window1 ? &window2 : &window1;
  if (!check_status (_("Edit"), fromwdata, 0, 1, 1, 1))
    return;

  if (*edit_program == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("Edit: You must specify an editor in the options dialog\n"));
      return;
    }

  templist = GTK_CLIST (fromwdata->listbox)->selection;
  num = 0;
  filelist = fromwdata->files;
  templist = get_next_selection (templist, &filelist, &num);
  curfle = filelist->data;

  if (curfle->isdir)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("Edit: %s is a directory. Cannot edit it.\n"), curfle->file);
      return;
    }

  if (strcmp (GFTP_GET_PROTOCOL_NAME (fromwdata->request), "Local") == 0)
    view_file (curfle->file, 0, 0, 0, 1, 1, NULL, fromwdata);
  else
    {
      new_fle = copy_fdata (curfle);
      if (new_fle->destfile)
        g_free (new_fle->destfile);
      new_fle->destfile = g_strconcat (tmp_directory, "/gftp-view.XXXXXX",
                                       NULL);
      if ((fd = mkstemp (new_fle->destfile)) < 0)
        {
          ftp_log (gftp_logging_misc, NULL, _("Error: Cannot open %s: %s\n"),
                   new_fle->destfile, g_strerror (errno));
          free_fdata (new_fle);
          return;
        }

      chmod (new_fle->destfile, S_IRUSR | S_IWUSR);
      if ((new_fle->fd = fdopen (fd, "rw+")) == NULL)
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Cannot fdopen() socket: %s\n"),
                   g_strerror (errno));
          return;
        }

      new_fle->is_fd = 1;
      new_fle->done_edit = 1;
      newfile = g_list_append (NULL, new_fle); 
      add_file_transfer (fromwdata->request, towdata->request,
                         fromwdata, towdata, newfile, 1);
    }
}


void
view_file (char *filename, int fd, int viewedit, int del_file, int start_pos,
	   int dontupload, char *remote_filename, gftp_window_data * wdata)
{
  GtkWidget * dialog, * view, * table, * tempwid;
  gftp_file_extensions * tempext;
  gftp_viewedit_data * newproc;
  GtkAdjustment * vadj;
  int stlen, doclose;
  GList * templist;
  char buf[8192];
  ssize_t n;
#if !(GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2)
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  guint len;
#endif

  doclose = 1;
  stlen = strlen (filename);
  for (templist = registered_exts; templist != NULL; templist = templist->next)
    {
      tempext = templist->data;
      if (stlen >= tempext->stlen &&
          strcmp (&filename[stlen - tempext->stlen], tempext->ext) == 0)
        {
          if (*tempext->view_program == '\0')
            break;
          ftp_log (gftp_logging_misc, NULL, _("Opening %s with %s\n"),
                   filename, tempext->view_program);
          fork_process (tempext->view_program, filename, fd, remote_filename,
                        viewedit, del_file, dontupload, wdata);
          return;
        }
    }

  if (viewedit && *view_program != '\0')
    {
      /* Open the file with the default file viewer */
      fork_process (view_program, filename, fd, remote_filename, viewedit,
                    del_file, dontupload, wdata);
      return;
    }
  else if (!viewedit)
    {
      /* Open the file with the default file editor */
      newproc = fork_process (edit_program, filename, fd, remote_filename, 
                              viewedit, del_file, dontupload, wdata);
      stat (filename, &newproc->st);
      return;
    }

  ftp_log (gftp_logging_misc, NULL, _("Viewing file %s\n"), filename);

  if (fd == 0)
    {
      if ((fd = open (filename, O_RDONLY)) < 0)
        {
          ftp_log (gftp_logging_misc, NULL, 
                   _("View: Cannot open file %s: %s\n"), filename, 
                   g_strerror (errno));
          return;
        }
      doclose = 1;
    }
  else
    {
      lseek (fd, 0, SEEK_SET);
      doclose = 0;
    }

  if (del_file)
    {
      if (unlink (filename) == 0)
        ftp_log (gftp_logging_misc, NULL, _("Successfully removed %s\n"), 
                 filename);
      else
        ftp_log (gftp_logging_error, NULL,
                 _("Error: Could not remove file %s: %s\n"), filename, 
                 g_strerror (errno));
    }

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), filename);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "fileview", "gFTP");
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 5);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area),
                              5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 5);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (dialog), "delete_event",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));

  table = gtk_table_new (1, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  view = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (view), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (view), TRUE);

  gtk_table_attach (GTK_TABLE (table), view, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (view);

  tempwid = gtk_vscrollbar_new (GTK_TEXT (view)->vadj);
  gtk_table_attach (GTK_TABLE (table), tempwid, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
  gtk_widget_show (tempwid);

  vadj = GTK_TEXT (view)->vadj;
#else
  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);

  tempwid = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tempwid),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (tempwid), view);
  gtk_widget_show (view);

  gtk_table_attach (GTK_TABLE (table), tempwid, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
		    0, 0);
  gtk_widget_show (tempwid);

  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tempwid));
#endif
  gtk_widget_set_size_request (table, 500, 400);
  gtk_widget_show (table);

  tempwid = gtk_button_new_with_label (_("  Close  "));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      FALSE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);

  buf[sizeof (buf) - 1] = '\0';
  while ((n = read (fd, buf, sizeof (buf) - 1)))
    {
      buf[n] = '\0';
#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
      gtk_text_insert (GTK_TEXT (view), NULL, NULL, NULL, buf, -1);
#else
      textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
      len = gtk_text_buffer_get_char_count (textbuf);
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len - 1);
      gtk_text_buffer_insert (textbuf, &iter, buf, -1);
#endif
    }

  if (doclose)
    close (fd);

  gtk_widget_show (dialog);

  if (!start_pos)
    gtk_adjustment_set_value (vadj, vadj->upper);
}


static gftp_viewedit_data *
fork_process (char *proc, char *filename, int fd, char *remote_filename, 
              int viewedit, int del_file, int dontupload, 
              gftp_window_data * wdata)
{
  gftp_viewedit_data * newproc;
  char *pos, *endpos, **argv;
  pid_t ret;
  int n;

  argv = NULL;
  n = 0;
  pos = proc;
  while ((endpos = strchr (pos, ' ')) != NULL)
    {
      *endpos = '\0';
      n++;
      argv = g_realloc (argv, n * sizeof (char *));
      argv[n - 1] = g_malloc (strlen (pos) + 1);
      strcpy (argv[n - 1], pos);
      *endpos = ' ';
      pos = endpos + 1;
    }
  argv = g_realloc (argv, (n + 3) * sizeof (char *));
  argv[n] = g_malloc (strlen (pos) + 1);
  strcpy (argv[n], pos);
  argv[n + 1] = g_malloc (strlen (filename) + 1);
  strcpy (argv[n + 1], filename);
  argv[n + 2] = NULL;

  newproc = NULL;
  switch ((ret = fork ()))
    {
    case 0:
      close (fd);
      execvp (argv[0], argv);
      _exit (1);
    case -1:
      for (n = 0; argv[n] != NULL; n++)
	g_free (argv[n]);
      ftp_log (gftp_logging_error, NULL,
              _("View: Cannot fork another process: %s\n"), g_strerror (errno));
      break;
    default:
      ftp_log (gftp_logging_misc, NULL, _("Running program: %s %s\n"), proc,
	       filename);
      newproc = g_malloc0 (sizeof (*newproc));
      newproc->pid = ret;
      newproc->argv = argv;
      if (wdata == &window2)
        {
          newproc->fromwdata = &window2;
          newproc->towdata = &window1;
        }
      else
        {
          newproc->fromwdata = &window1;
          newproc->towdata = &window2;
        }
      newproc->filename = g_malloc (strlen (filename) + 1);
      strcpy (newproc->filename, filename);
      if (remote_filename != NULL)
	{
	  newproc->remote_filename = g_malloc (strlen (remote_filename) + 1);
	  strcpy (newproc->remote_filename, remote_filename);
	}
      newproc->view = viewedit;
      newproc->rm = del_file;
      newproc->dontupload = dontupload;
      viewedit_processes = g_list_append (viewedit_processes, newproc);
    }
  return (newproc);
}

