/***********************************************************************************/
/*  view_dialog.c - view dialog box and ftp routines                               */
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

static gftp_file * curfle;

static void do_view_or_edit_file (gftp_window_data * fromwdata, int is_view)
{
  DEBUG_PRINT_FUNC
  GList * newfile;
  gftp_window_data * towdata;
  gftp_file * new_fle;
  char *suffix;

  if (!check_status (is_view ? _("View") : _("Edit"), fromwdata, 0, 1, 1, 1))
    return;

  towdata = fromwdata == &window1 ? &window2 : &window1;

  // retrieve selected file
  gftp_file * tempfle = (gftp_file *) listbox_get_selected_files (fromwdata, 1);

  curfle = tempfle;

  if (S_ISDIR (curfle->st_mode))
    {
      if (is_view)
      {
        if (fromwdata->request->protonum == GFTP_PROTOCOL_LOCALFS)
        {
          gchar* argv[] = { (gchar*) "xdg-open", curfle->file, NULL };
          ftp_log (gftp_logging_misc, NULL, _("View directory: %s %s\n"), argv[0], argv[1]);
          if (!g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL))
            ftp_log (gftp_logging_error, NULL,
                   _("Couldn't execute command: %s\n"), argv[0]);
        }
        else
        {
          ftp_log (gftp_logging_error, NULL,
                 _("View: %s is a remote directory. Cannot view it.\n"), curfle->file);
        }
      }
      else
        ftp_log (gftp_logging_error, NULL,
                 _("Edit: %s is a directory. Cannot edit it.\n"), curfle->file);

      return;
    }

  if (fromwdata->request->protonum == GFTP_PROTOCOL_LOCALFS)
    view_file (curfle->file, 0, is_view, 0, 1, 1, NULL, fromwdata);
  else
    {
      new_fle = copy_fdata (curfle);
      if (new_fle->destfile)
        g_free (new_fle->destfile);

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ < 7
      suffix = NULL; // glibc < 2.7: use mkstemp
#else
      suffix = strrchr (curfle->file, '.');
#endif
      if (suffix)
        {
          new_fle->destfile = g_strconcat (g_get_tmp_dir (),
                                           "/gftp-view.XXXXXX", suffix, NULL);
          new_fle->fd = mkstemps (new_fle->destfile, strlen (suffix));
	}
      else
        {
	  new_fle->destfile = g_strconcat (g_get_tmp_dir (),
                                           "/gftp-view.XXXXXX", NULL);		
          new_fle->fd = mkstemp (new_fle->destfile);
	}
		
      if (new_fle->fd < 0)
        {
          ftp_log (gftp_logging_error, NULL, 
                   _("Error: Cannot open %s for writing: %s\n"),  
                   new_fle->destfile, g_strerror (errno));
          gftp_file_destroy (new_fle, 1);
          return;
        }

      fchmod (new_fle->fd, S_IRUSR | S_IWUSR);

      if (is_view)
        {
          new_fle->done_view = 1;
          new_fle->done_rm = 1;
        }
      else
        new_fle->done_edit = 1;

      newfile = g_list_append (NULL, new_fle); 
      gftpui_common_add_file_transfer (fromwdata->request, towdata->request,
                                       fromwdata, towdata, newfile);
    }
}


void view_dialog (gpointer data)
{
  DEBUG_PRINT_FUNC
  do_view_or_edit_file (data, 1);
}


void edit_dialog (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftp_window_data * fromwdata = data;
  char *edit_program;

  gftp_lookup_request_option (fromwdata->request, "edit_program", 
                              &edit_program);

  if (*edit_program == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Edit: You must specify an editor in the options dialog\n"));
      return;
    }

  do_view_or_edit_file (data, 0);
}


static gftp_viewedit_data *
fork_process (char *proc, char *filename, int fd, char *remote_filename, 
              unsigned int viewedit, unsigned int del_file,
              unsigned int dontupload, gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
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
      argv = g_realloc (argv, (gulong) n * sizeof (char *));
      argv[n - 1] = g_strdup (pos);
      *endpos = ' ';
      pos = endpos + 1;
    }
  argv = g_realloc (argv, (gulong) (n + 3) * sizeof (char *));
  argv[n] = g_strdup (pos);

  if (wdata != NULL)
    {
      if ((argv[n + 1] = gftp_filename_from_utf8 (wdata->request, filename, NULL)) == NULL)
        argv[n + 1] = g_strdup (filename);
    }
  else
    argv[n + 1] = g_strdup (filename);

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
      newproc->torequest = gftp_copy_request (newproc->towdata->request);
      newproc->filename = g_strdup (filename);
      if (remote_filename != NULL)
	newproc->remote_filename = g_strdup (remote_filename);
      newproc->view = viewedit;
      newproc->rm = del_file;
      newproc->dontupload = dontupload;
      viewedit_processes = g_list_append (viewedit_processes, newproc);
    }
  return (newproc);
}


void
view_file (char *filename, int fd, unsigned int viewedit, unsigned int del_file,
           unsigned int start_pos, unsigned int dontupload,
           char *remote_filename, gftp_window_data * wdata)
{
  DEBUG_PRINT_FUNC
  GtkWidget * dialog, * view, * scrolledw, *main_vbox;
  char buf[8192], *view_program, *edit_program;
  gftp_config_list_vars * tmplistvar;
  gftp_file_extensions * tempext;
  gftp_viewedit_data * newproc;
  GtkAdjustment * vadj;
  GList * templist;
  size_t stlen;
  int doclose;
  ssize_t n;
  char * non_utf8;
  GtkTextBuffer * textbuf;
  GtkTextIter iter;

  doclose = 1;
  stlen = strlen (filename);
  gftp_lookup_global_option ("ext", &tmplistvar);
  for (templist = tmplistvar->list; templist != NULL; templist = templist->next)
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

  if (wdata != NULL)
    {
      gftp_lookup_request_option (wdata->request, "view_program", &view_program);
      gftp_lookup_request_option (wdata->request, "edit_program", &edit_program);
      if ((non_utf8 = gftp_filename_from_utf8 (wdata->request, filename, NULL)) == NULL) /* freeme later! */
        non_utf8 = filename;
    }
  else
    {
      gftp_lookup_global_option ("view_program", &view_program);
      gftp_lookup_global_option ("edit_program", &edit_program);
      non_utf8 = filename;
    }

  if (viewedit && *view_program != '\0')
    {
      /* Open the file with the default file viewer */
      fork_process (view_program, filename, fd, remote_filename, viewedit,
                    del_file, dontupload, wdata);
      if (non_utf8 != filename && non_utf8)
        g_free (non_utf8);
      return;
    }
  else if (!viewedit && *edit_program != '\0')
    {
      /* Open the file with the default file editor */
      newproc = fork_process (edit_program, filename, fd, remote_filename, 
                              viewedit, del_file, dontupload, wdata);
      stat (non_utf8, &newproc->st);
      if (non_utf8 != filename && non_utf8)
        g_free (non_utf8);
      return;
    }

  ftp_log (gftp_logging_misc, NULL, _("Viewing file %s\n"), filename);

  if (fd == 0)
    {
      if ((fd = open (non_utf8, O_RDONLY)) < 0)
        {
          ftp_log (gftp_logging_error, NULL, 
                   _("View: Cannot open file %s: %s\n"), non_utf8, 
                   g_strerror (errno));
          if (non_utf8 != filename && non_utf8)
            g_free (non_utf8);
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
      if (unlink (non_utf8) == 0)
        ftp_log (gftp_logging_misc, NULL, _("Successfully removed %s\n"), 
                 filename);
      else
        ftp_log (gftp_logging_error, NULL,
                 _("Error: Could not remove file %s: %s\n"), filename, 
                 g_strerror (errno));
    }

  if (non_utf8 != filename && non_utf8)
    g_free (non_utf8);

  dialog = gtk_dialog_new_with_buttons (filename, NULL, 0,
                                        "gtk-close",
                                        GTK_RESPONSE_CLOSE,
                                        NULL);

  gtk_window_set_role (GTK_WINDOW(dialog), "fileview");

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);
  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);
  gtk_widget_realize (dialog);

  set_window_icon(GTK_WINDOW(dialog), NULL);

  view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD);

  scrolledw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (scrolledw), view);

  gtk_box_pack_start (GTK_BOX (main_vbox), scrolledw, TRUE, TRUE, 0);

  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolledw));
  gtk_widget_set_size_request (scrolledw, 500, 400);

  g_signal_connect_swapped (G_OBJECT (dialog), "response",
                            G_CALLBACK (gtk_widget_destroy),
                            G_OBJECT (dialog));

  buf[sizeof (buf) - 1] = '\0';
  while ((n = read (fd, buf, sizeof (buf) - 1)) > 0)
    {
      buf[n] = '\0';
      textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
      gtk_text_buffer_get_iter_at_offset (textbuf, &iter, -1);
      gtk_text_buffer_insert (textbuf, &iter, buf, -1);
    }

  if (doclose)
    close (fd);

  gtk_widget_show_all (dialog);

  if (!start_pos)
    gtk_adjustment_set_value (vadj, vadj->upper);
}

