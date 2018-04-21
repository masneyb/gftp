/*****************************************************************************/
/*  transfer.c - functions to handle transfering files                       */
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

static int num_transfers_in_progress = 0;

int
ftp_list_files (gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  gtk_label_set (GTK_LABEL (wdata->hoststxt), _("Receiving file names..."));

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_ls;
  cdata->dont_refresh = 1;

  gftpui_common_run_callback_function (cdata);

  wdata->files = cdata->files;
  g_free (cdata);
  
  if (wdata->files == NULL || !GFTP_IS_CONNECTED (wdata->request))
    {
      gftpui_disconnect (wdata);
      return (0);
    }

  wdata->sorted = 0;
  sortrows (GTK_CLIST (wdata->listbox), -1, (gpointer) wdata);

  if (IS_NONE_SELECTED (wdata))
    gtk_clist_select_row (GTK_CLIST (wdata->listbox), 0, 0);

  return (1);
}


int
ftp_connect (gftp_window_data * wdata, gftp_request * request)
{
  if (wdata->request == request)
    gtk_label_set (GTK_LABEL (wdata->hoststxt), _("Connecting..."));

  return (gftpui_common_cmd_open (wdata, request, NULL, NULL, NULL));
}


void 
get_files (gpointer data)
{
  transfer_window_files (&window2, &window1);
}


void
put_files (gpointer data)
{
  transfer_window_files (&window1, &window2);
}


void
transfer_window_files (gftp_window_data * fromwdata, gftp_window_data * towdata)
{
  gftp_file * tempfle, * newfle;
  GList * templist, * filelist;
  gftp_transfer * transfer;
  int num, ret, disconnect;

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

  num = 0;
  templist = gftp_gtk_get_list_selection (fromwdata);
  filelist = fromwdata->files;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      if (strcmp (tempfle->file, "..") != 0)
        {
          newfle = copy_fdata (tempfle);
          transfer->files = g_list_append (transfer->files, newfle);
        }
    }

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
  return (gftp_get_all_subdirs (cdata->user_data, NULL));
}


int
gftp_gtk_get_subdirs (gftp_transfer * transfer)
{
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

  timeout_num = gtk_timeout_add (100, progress_timeout, transfer);
  ret = gftpui_common_run_callback_function (cdata);
  gtk_timeout_remove (timeout_num);

  numfiles = transfer->numfiles;
  numdirs = transfer->numdirs;
  transfer->numfiles = transfer->numdirs = -1; 
  update_directory_download_progress (transfer);
  transfer->numfiles = numfiles;
  transfer->numdirs = numdirs;

  g_free (cdata);

  return (ret);
}


static void
remove_file (gftp_viewedit_data * ve_proc)
{
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


static void
free_edit_data (gftp_viewedit_data * ve_proc)
{
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
  remove_file (ve_proc);
  free_edit_data (ve_proc);
}


static void
do_upload (gftp_viewedit_data * ve_proc, gftp_dialog_data * ddata)
{
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

      MakeYesNoDialog (_("Edit File"), str, do_upload, ve_proc, dont_upload,
                       ve_proc);
      g_free (str);
      return (1);
    }
}


static void
do_check_done_process (pid_t pid, int ret)
{
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
  pid_t pid;
  int ret;

  gftpui_common_child_process_done = 0;
  while ((pid = waitpid (-1, &ret, WNOHANG)) > 0)
    {
      do_check_done_process (pid, ret);
    }
}


static void
on_next_transfer (gftp_transfer * tdata)
{
  intptr_t refresh_files;
  gftp_file * tempfle;

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
        }
      else if (tempfle->done_rm)
	tdata->fromreq->rmfile (tdata->fromreq, tempfle->file);
      
      if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
        gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tempfle->user_data, 1,
  			         _("Skipped"));
      else
        gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tempfle->user_data, 1,
  			         _("Finished"));
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
  gftp_set_password (request, gtk_entry_get_text (GTK_ENTRY (ddata->edit)));
  request->stopable = 0;
}


static void
cancel_get_trans_password (gftp_transfer * tdata, gftp_dialog_data * ddata)
{
  if (tdata->fromreq->stopable == 0)
    return;

  gftpui_common_cancel_file_transfer (tdata);
}


static void
show_transfer (gftp_transfer * tdata)
{
  GdkPixmap * closedir_pixmap, * opendir_pixmap;
  GdkBitmap * closedir_bitmap, * opendir_bitmap;
  gftpui_common_curtrans_data * transdata;
  gftp_file * tempfle;
  GList * templist;
  char *text[2];

  gftp_get_pixmap (dlwdw, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
  gftp_get_pixmap (dlwdw, "dir.xpm", &closedir_pixmap, &closedir_bitmap);

  text[0] = tdata->fromreq->hostname;
  text[1] = _("Waiting...");
  tdata->user_data = gtk_ctree_insert_node (GTK_CTREE (dlwdw), NULL, NULL, 
                                       text, 5,
                                       closedir_pixmap, closedir_bitmap, 
                                       opendir_pixmap, opendir_bitmap, 
                                       FALSE, 
                                       tdata->numdirs + tdata->numfiles < 50);
  transdata = g_malloc0 (sizeof (*transdata));
  transdata->transfer = tdata;
  transdata->curfle = NULL;
  gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), tdata->user_data, transdata);
  tdata->show = 0;
  tdata->curfle = tdata->updfle = tdata->files;

  tdata->total_bytes = 0;
  for (templist = tdata->files; templist != NULL; templist = templist->next)
    {
      tempfle = templist->data;

      text[0] = gftpui_gtk_get_utf8_file_pos (tempfle);
      if (tempfle->transfer_action == GFTP_TRANS_ACTION_SKIP)
        text[1] = _("Skipped");
      else
        {
          tdata->total_bytes += tempfle->size;
          text[1] = _("Waiting...");
        }

      tempfle->user_data = gtk_ctree_insert_node (GTK_CTREE (dlwdw), 
                                             tdata->user_data, 
                                             NULL, text, 5, NULL, NULL, NULL, 
                                             NULL, FALSE, FALSE);
      transdata = g_malloc0 (sizeof (*transdata));
      transdata->transfer = tdata;
      transdata->curfle = templist;
      gtk_ctree_node_set_row_data (GTK_CTREE (dlwdw), tempfle->user_data, 
                                   transdata);
    }

  if (!tdata->toreq->stopable && gftp_need_password (tdata->toreq))
    {
      tdata->toreq->stopable = 1;
      MakeEditDialog (_("Enter Password"),
		      _("Please enter your password for this site"), NULL, 0,
		      NULL, gftp_dialog_button_connect, 
                      get_trans_password, tdata->toreq,
		      cancel_get_trans_password, tdata);
    }

  if (!tdata->fromreq->stopable && gftp_need_password (tdata->fromreq))
    {
      tdata->fromreq->stopable = 1;
      MakeEditDialog (_("Enter Password"),
		      _("Please enter your password for this site"), NULL, 0,
		      NULL, gftp_dialog_button_connect, 
                      get_trans_password, tdata->fromreq,
		      cancel_get_trans_password, tdata);
    }
}


static void
transfer_done (GList * node)
{
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
      transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), 
                                               tdata->user_data);
      if (transdata != NULL)
        g_free (transdata);

      for (templist = tdata->files; templist != NULL; templist = templist->next)
        {
          tempfle = templist->data;
          transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), 
                                                   tempfle->user_data);
          if (transdata != NULL)
            g_free (transdata);
        }
          
      gtk_ctree_remove_node (GTK_CTREE (dlwdw), tdata->user_data);
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
  int ret;

  pthread_detach (pthread_self ());
  ret = gftpui_common_transfer_files (data);
  return (GINT_TO_POINTER(ret));
}


static void
create_transfer (gftp_transfer * tdata)
{
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
  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tdata->user_data, 1,
                           _("Connecting..."));

  if (tdata->thread_id == NULL)
    tdata->thread_id = g_malloc0 (sizeof (pthread_t));

  if (pthread_create (tdata->thread_id, NULL, _gftpui_transfer_files, tdata) != 0)
    perror("pthread_create failed");
}


static void
_setup_dlstr (gftp_transfer * tdata, gftp_file * fle, char *dlstr,
              size_t dlstr_len)
{
  int hours, mins, secs, stalled, usesentdescr;
  unsigned long remaining_secs, lkbs;
  char gotstr[50], ofstr[50];
  struct timeval tv;

  stalled = 1;
  gettimeofday (&tv, NULL);
  usesentdescr = (tdata->fromreq->protonum == GFTP_LOCAL_NUM);

  insert_commas (fle->size, ofstr, sizeof (ofstr));
  insert_commas (tdata->curtrans + tdata->curresumed, gotstr, sizeof (gotstr));

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


static void
update_file_status (gftp_transfer * tdata)
{
  char totstr[150], winstr[150], dlstr[150];
  unsigned long remaining_secs, lkbs;
  int hours, mins, secs, pcent;
  intptr_t show_trans_in_title;
  gftp_file * tempfle;
  
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

  *dlstr = '\0';
  if (!tdata->stalled)
    _setup_dlstr (tdata, tempfle, dlstr, sizeof (dlstr));

  g_mutex_unlock (&tdata->statmutex);

  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tdata->user_data, 1, totstr);
  
  gftp_lookup_global_option ("show_trans_in_title", &show_trans_in_title);
  if (gftp_file_transfers->data == tdata && show_trans_in_title)
    {
      g_snprintf (winstr, sizeof(winstr),  "%s: %s", gftp_version, totstr);
      gdk_window_set_title (gtk_widget_get_parent_window (GTK_WIDGET(dlwdw)),
                            winstr);
    }

  if (*dlstr != '\0')
    gtk_ctree_node_set_text (GTK_CTREE (dlwdw), tempfle->user_data, 1, dlstr);
}


static void
update_window_transfer_bytes (gftp_window_data * wdata)
{
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
      gtk_label_set (GTK_LABEL (wdata->hoststxt), temp1str);
      g_free (tempstr);
      g_free (temp1str);
    }
}


gint
update_downloads (gpointer data)
{
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

  gtk_timeout_add (500, update_downloads, NULL);
  return (0);
}


void
start_transfer (gpointer data)
{
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("There are no file transfers selected\n"));
      return;
    }
  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);

  g_mutex_lock (&transdata->transfer->structmutex);
  if (!transdata->transfer->started)
    create_transfer (transdata->transfer);
  g_mutex_unlock (&transdata->transfer->structmutex);
}


void
stop_transfer (gpointer data)
{
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	      _("There are no file transfers selected\n"));
      return;
    }

  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);
  gftpui_common_cancel_file_transfer (transdata->transfer);
}


void
skip_transfer (gpointer data)
{
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	      _("There are no file transfers selected\n"));
      return;
    }

  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);

  gftpui_common_skip_file_transfer (transdata->transfer,
                                    transdata->transfer->curfle->data);
}


void
remove_file_transfer (gpointer data)
{
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;
  gftp_file * curfle;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
              _("There are no file transfers selected\n"));
      return;
    }

  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);

  if (transdata->curfle == NULL || transdata->curfle->data == NULL)
    return;

  curfle = transdata->curfle->data;
  gftpui_common_skip_file_transfer (transdata->transfer, curfle);

  gtk_ctree_node_set_text (GTK_CTREE (dlwdw), curfle->user_data, 1,
                           _("Skipped"));
}


void
move_transfer_up (gpointer data)
{
  GList * firstentry, * secentry, * lastentry;
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	      _("There are no file transfers selected\n"));
      return;
    }
  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);

  if (transdata->curfle == NULL)
    return;

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

      gtk_ctree_move (GTK_CTREE (dlwdw), 
                      ((gftp_file *) transdata->curfle->data)->user_data,
                      transdata->transfer->user_data, 
                      transdata->curfle->next != NULL ?
                          ((gftp_file *) transdata->curfle->next->data)->user_data: NULL);
    }
  g_mutex_unlock (&transdata->transfer->structmutex);
}


void
move_transfer_down (gpointer data)
{
  GList * firstentry, * secentry, * lastentry;
  gftpui_common_curtrans_data * transdata;
  GtkCTreeNode * node;

  if (GTK_CLIST (dlwdw)->selection == NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	      _("There are no file transfers selected\n"));
      return;
    }
  node = GTK_CLIST (dlwdw)->selection->data;
  transdata = gtk_ctree_node_get_row_data (GTK_CTREE (dlwdw), node);

  if (transdata->curfle == NULL)
    return;

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

      gtk_ctree_move (GTK_CTREE (dlwdw), 
                      ((gftp_file *) transdata->curfle->data)->user_data,
                      transdata->transfer->user_data, 
                      transdata->curfle->next != NULL ?
                          ((gftp_file *) transdata->curfle->next->data)->user_data: NULL);
    }
  g_mutex_unlock (&transdata->transfer->structmutex);
}

