/*****************************************************************************/
/*  delete_dialog.c - the delete dialog                                      */
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
static const char cvsid[] = "$Id$";

static void askdel 				( gftp_transfer * transfer );
static void yesCB 				( gftp_transfer * transfer,
						  gftp_dialog_data * ddata );
static void * do_delete_thread 			( void *data );
static void delete_purge_cache 			( gpointer key, 
						  gpointer value, 
						  gpointer user_data );

void
delete_dialog (gpointer data)
{
  gftp_file * tempfle, * newfle;
  GList * templist, * filelist;
  gftp_transfer * transfer;
  gftp_window_data * wdata;
  long numfiles, numdirs;
  int num, timeout_num;
  void *ret;

  wdata = data;
  if (!check_status (_("Delete"), wdata, wdata->request->use_threads, 0, 1, 1))
    return;

  transfer = g_malloc0 (sizeof (*transfer));
  transfer->fromreq = copy_request (wdata->request);
  transfer->fromwdata = wdata;
  transfer->transfer_direction = GFTP_DIRECTION_DOWNLOAD;

  num = 0;
  templist = GTK_CLIST (wdata->listbox)->selection;
  filelist = wdata->files;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &num);
      tempfle = filelist->data;
      if (strcmp (tempfle->file, "..") == 0 ||
          strcmp (tempfle->file, ".") == 0)
        continue;
      newfle = copy_fdata (tempfle);
      transfer->files = g_list_append (transfer->files, newfle);
    }

  if (transfer->files == NULL)
    {
      free_tdata (transfer);
      return;
    }

  swap_socks (transfer->fromreq, wdata->request);

  if (transfer->fromreq->use_threads)
    {
      wdata->request->stopable = 1;
      transfer->fromreq->stopable = 1;
      gtk_widget_set_sensitive (stop_btn, 1);
      pthread_create (&wdata->tid, NULL, do_getdir_thread, transfer);

      timeout_num = gtk_timeout_add (100, progress_timeout, transfer);
      while (transfer->fromreq->stopable)
        {
          GDK_THREADS_LEAVE ();
#if GTK_MAJOR_VERSION == 1
          g_main_iteration (TRUE);
#else
          g_main_context_iteration (NULL, TRUE);
#endif
        }

      gtk_widget_set_sensitive (stop_btn, 0);
      gtk_timeout_remove (timeout_num);
      numfiles = transfer->numfiles;
      numdirs = transfer->numdirs;
      transfer->numfiles = transfer->numdirs = -1;
      update_directory_download_progress (transfer);
      transfer->numfiles = numfiles;
      transfer->numdirs = numdirs;

      pthread_join (wdata->tid, &ret);
      wdata->request->stopable = 0;
    }
  else
    ret = do_getdir_thread (transfer);

  if (!GFTP_IS_CONNECTED (transfer->fromreq))
    {
      disconnect (wdata);
      return;
    }

  swap_socks (wdata->request, transfer->fromreq);

  askdel (transfer);
}


static void
askdel (gftp_transfer * transfer)
{
  char *tempstr;

  tempstr = g_strdup_printf (_("Are you sure you want to delete these %ld files and %ld directories"), transfer->numfiles, transfer->numdirs);

  MakeYesNoDialog (_("Delete Files/Directories"), tempstr, 
                   yesCB, transfer, NULL, NULL);
  g_free (tempstr);
}


static void
yesCB (gftp_transfer * transfer, gftp_dialog_data * ddata)
{
  gftp_window_data * wdata;
  void * ret;

  g_return_if_fail (transfer != NULL);
  g_return_if_fail (transfer->files != NULL);

  wdata = transfer->fromwdata;
  if (check_reconnect (wdata) < 0)
    return;

  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  swap_socks (transfer->fromreq, wdata->request);
  if (wdata->request->use_threads)
    {
      wdata->request->stopable = 1;
      transfer->fromreq->stopable = 1;
      gtk_widget_set_sensitive (stop_btn, 1);
      pthread_create (&wdata->tid, NULL, do_delete_thread, transfer);

      while (transfer->fromreq->stopable)
        {
          GDK_THREADS_LEAVE ();
#if GTK_MAJOR_VERSION == 1
          g_main_iteration (TRUE);
#else
          g_main_context_iteration (NULL, TRUE);
#endif
        }

      gtk_widget_set_sensitive (stop_btn, 0);
      pthread_join (wdata->tid, &ret);
      wdata->request->stopable = 0;
    }
  else
    ret = do_delete_thread (transfer);
  swap_socks (wdata->request, transfer->fromreq);
  free_tdata (transfer);

  if (!GFTP_IS_CONNECTED (wdata->request))
    disconnect (wdata);
  else
    refresh (wdata);

  gtk_clist_thaw (GTK_CLIST (wdata->listbox));
}


static void *
do_delete_thread (void *data)
{
  gftp_transfer * transfer;
  char *pos, *tempstr;
  gftp_file * tempfle;
  GHashTable * rmhash;
  GList * templist;
  int success, sj;

  transfer = data;
  transfer->fromreq->user_data = (void *) 0x01;

  if (transfer->fromreq->use_threads)
    {
      sj = sigsetjmp (jmp_environment, 1);
      use_jmp_environment = 1;
    }
  else
    sj = 0;

  if (sj == 0)
    {
      for (templist = transfer->files; templist->next != NULL; 
           templist = templist->next);

      rmhash = g_hash_table_new (string_hash_function, string_hash_compare);
      while (1)
        {
          tempfle = templist->data;
          if (tempfle->isdir)
            success = gftp_remove_directory (transfer->fromreq, tempfle->file);
          else
            success = gftp_remove_file (transfer->fromreq, tempfle->file);

          if (success == 0 && (pos = strrchr (tempfle->file, '/')))
            {
              *pos = '\0';
              if (g_hash_table_lookup (rmhash, tempfle->file) == NULL)
                {
                  tempstr = g_strconcat (tempfle->file, NULL);
                  g_hash_table_insert (rmhash, tempstr, NULL);
                }
              *pos = '/';
            }

          if (templist == transfer->files || 
              !GFTP_IS_CONNECTED (transfer->fromreq))
            break;
          templist = templist->prev;
        }

      g_hash_table_foreach (rmhash, delete_purge_cache, transfer->fromreq);
      g_hash_table_destroy (rmhash);
    }
  else
    {
      gftp_disconnect (transfer->fromreq);
      transfer->fromreq->logging_function (gftp_logging_error,
                                        transfer->fromreq->user_data,
                                        _("Operation canceled\n"));
    }

  transfer->fromreq->user_data = NULL;
  transfer->fromreq->stopable = 0;

  if (transfer->fromreq->use_threads)
    use_jmp_environment = 0;

  return (NULL);
}


static void
delete_purge_cache (gpointer key, gpointer value, gpointer user_data)
{
  gftp_request * request;
  char *olddir;

  request = user_data;
  olddir = request->directory;
  request->directory = key;
  gftp_delete_cache_entry (request);
  request->directory = olddir;
  g_free (key);
}

