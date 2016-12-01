/*****************************************************************************/
/*  gtkui.c - GTK+ UI related functions for gFTP                             */
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
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftp-gtk.h"
static const char cvsid[] = "$Id$";


void
gftpui_lookup_file_colors (gftp_file * fle, char **start_color,
                           char ** end_color)
{
  *start_color = GFTPUI_COMMON_COLOR_NONE;
  *end_color = GFTPUI_COMMON_COLOR_NONE;
}


void
gftpui_run_command (GtkWidget * widget, gpointer data)
{
  const char *txt;

  txt = gtk_entry_get_text (GTK_ENTRY (gftpui_command_widget));
  gftpui_common_process_command (&window1, window1.request,
                                 &window2, window2.request, txt);
  gtk_entry_set_text (GTK_ENTRY (gftpui_command_widget), "");
}


void
gftpui_refresh (void *uidata, int clear_cache_entry)
{
  gftp_window_data * wdata;

  wdata = uidata;
  wdata->request->refreshing = 1;

  if (!check_status (_("Refresh"), wdata, 0, 0, 0, 1))
    {
      wdata->request->refreshing = 0;
      return;
    }

  if (clear_cache_entry)
    gftp_delete_cache_entry (wdata->request, NULL, 0);

  if (check_reconnect (wdata) < 0)
    {
      wdata->request->refreshing = 0;
      return;
    }

  gtk_clist_freeze (GTK_CLIST (wdata->listbox));
  remove_files_window (wdata);

  ftp_list_files (wdata);
  gtk_clist_thaw (GTK_CLIST (wdata->listbox));

  wdata->request->refreshing = 0;
}


#define _GFTPUI_GTK_USER_PW_SIZE	256

static void 
_gftpui_gtk_set_username (gftp_request * request, gftp_dialog_data * ddata)
{
  gftp_set_username (request, gtk_entry_get_text (GTK_ENTRY (ddata->edit)));
  request->stopable = 0;
}


static void 
_gftpui_gtk_set_password (gftp_request * request, gftp_dialog_data * ddata)
{
  gftp_set_password (request, gtk_entry_get_text (GTK_ENTRY (ddata->edit)));
  request->stopable = 0;
}


static void
_gftpui_gtk_abort (gftp_request * request, gftp_dialog_data * ddata)
{
  request->stopable = 0;
}

void
gftpui_show_busy (gboolean busy)
{
  GtkWidget * toplevel = gtk_widget_get_toplevel (openurl_btn);
  GdkDisplay * display = gtk_widget_get_display (toplevel);

  GdkCursor * busyCursor = 
    (busy) ? (gdk_cursor_new_for_display (display, GDK_WATCH)) : NULL;

  gdk_window_set_cursor (toplevel->window, busyCursor);

  if (busy)
    gdk_cursor_unref (busyCursor);
}

void
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  MakeEditDialog (_("Enter Username"),
                  _("Please enter your username for this site"), NULL,
                  1, NULL, gftp_dialog_button_connect,
                  _gftpui_gtk_set_username, request,
                  _gftpui_gtk_abort, request);

  request->stopable = 1;
  while (request->stopable)
    {
      GDK_THREADS_LEAVE ();
      g_main_context_iteration (NULL, TRUE);
    }
}


void
gftpui_prompt_password (void *uidata, gftp_request * request)
{
  MakeEditDialog (_("Enter Password"),
                  _("Please enter your password for this site"), NULL,
                  0, NULL, gftp_dialog_button_connect,
                  _gftpui_gtk_set_password, request,
                  _gftpui_gtk_abort, request);

  request->stopable = 1;
  while (request->stopable)
    {
      GDK_THREADS_LEAVE ();
      g_main_context_iteration (NULL, TRUE);
    }
}


/* The wakeup main thread functions are so that after the thread terminates
   there won't be a delay in updating the GUI */
static void
_gftpui_wakeup_main_thread (gpointer data, gint source,
                            GdkInputCondition condition)
{
  gftp_request * request;
  char c;

  request = data;
  if (request->wakeup_main_thread[0] > 0)
    read (request->wakeup_main_thread[0], &c, 1);
}


static gint
_gftpui_setup_wakeup_main_thread (gftp_request * request)
{
  gint handler;

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, request->wakeup_main_thread) == 0)
    {
      handler = gdk_input_add (request->wakeup_main_thread[0],
                               GDK_INPUT_READ, _gftpui_wakeup_main_thread,
                               request);
    }
  else
    {
      request->wakeup_main_thread[0] = 0;
      request->wakeup_main_thread[1] = 0;
      handler = 0;
    }

  return (handler);
}


static void
_gftpui_teardown_wakeup_main_thread (gftp_request * request, gint handler)
{
  if (request->wakeup_main_thread[0] > 0 && request->wakeup_main_thread[1] > 0)
    {
      gdk_input_remove (handler);
      close (request->wakeup_main_thread[0]);
      close (request->wakeup_main_thread[1]);
      request->wakeup_main_thread[0] = 0;
      request->wakeup_main_thread[1] = 0;
    }
}

static void *
_gftpui_gtk_thread_func (void *data)
{
  gftpui_gtk_thread_data * thread_data;
  void *ret;

  thread_data = data;
  ret = thread_data->func (thread_data->cdata);

  if (thread_data->cdata->request->wakeup_main_thread[1] > 0)
    write (thread_data->cdata->request->wakeup_main_thread[1], " ", 1);

  return (ret);
}


void *
gftpui_generic_thread (void * (*func) (void *), void *data)
{
  gftpui_gtk_thread_data * thread_data;
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;
  gint handler;
  void * ret;

  cdata = data;
  wdata = cdata->uidata;

  wdata->request->stopable = 1;
  gtk_widget_set_sensitive (stop_btn, 1);

  thread_data = g_malloc0 (sizeof (*thread_data));
  thread_data->func = func;
  thread_data->cdata = cdata;

  handler = _gftpui_setup_wakeup_main_thread (cdata->request);
  pthread_create (&wdata->tid, NULL, _gftpui_gtk_thread_func, thread_data);

  while (wdata->request->stopable)
    {
      GDK_THREADS_LEAVE ();
      g_main_context_iteration (NULL, TRUE);
    }

  _gftpui_teardown_wakeup_main_thread (cdata->request, handler);
  g_free (thread_data);

  pthread_join (wdata->tid, &ret);
  gtk_widget_set_sensitive (stop_btn, 0);

  if (!GFTP_IS_CONNECTED (wdata->request))
    gftpui_disconnect (wdata);

  return (ret);
}


int
gftpui_check_reconnect (gftpui_callback_data * cdata)
{
  gftp_window_data * wdata;

  wdata = cdata->uidata;
  return (wdata->request->cached && wdata->request->datafd < 0 &&
          !wdata->request->always_connected &&
          !ftp_connect (wdata, wdata->request) ? -1 : 0);
}


void
gftpui_run_function_callback (gftp_window_data * wdata,
                              gftp_dialog_data * ddata)
{
  gftpui_callback_data * cdata;
  const char *edttext;

  cdata = ddata->yespointer;
  if (ddata->edit != NULL)
    {
      edttext = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
      if (*edttext == '\0')
        {
          ftp_log (gftp_logging_error, NULL,
                   _("Operation canceled...you must enter a string\n"));
          return;
        }

      cdata->input_string = g_strdup (edttext);
    }

  if (ddata->checkbox != NULL)
    cdata->toggled = GTK_TOGGLE_BUTTON (ddata->checkbox)->active;
  else
    cdata->toggled = 0;

  gtk_widget_destroy (ddata->dialog);
  ddata->dialog = NULL;

  gftpui_common_run_callback_function (cdata);
}


void
gftpui_run_function_cancel_callback (gftp_window_data * wdata,
                                     gftp_dialog_data * ddata)
{
  gftpui_callback_data * cdata;

  cdata = ddata->yespointer;
  if (cdata->input_string != NULL)
    g_free (cdata->input_string);
  if (cdata->source_string != NULL)
    g_free (cdata->source_string);
  g_free (cdata);
}


void
gftpui_mkdir_dialog (gpointer data)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_mkdir;

  if (!check_status (_("Mkdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0, wdata->request->mkdir != NULL))
    return;

  MakeEditDialog (_("Make Directory"), _("Enter name of directory to create"),
                  NULL, 1, NULL, gftp_dialog_button_create,
                  gftpui_run_function_callback, cdata,
                  gftpui_run_function_cancel_callback, cdata);
}


void
gftpui_rename_dialog (gpointer data)
{
  gftpui_callback_data * cdata;
  GList *templist, *filelist;
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;
  int num;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_rename;

  if (!check_status (_("Rename"), wdata, gftpui_common_use_threads (wdata->request), 1, 1, wdata->request->rename != NULL))
    return;

  templist = gftp_gtk_get_list_selection (wdata);
  num = 0;
  filelist = wdata->files;
  templist = get_next_selection (templist, &filelist, &num);
  curfle = filelist->data;
  cdata->source_string = g_strdup (curfle->file);

  tempstr = g_strdup_printf (_("What would you like to rename %s to?"),
                             cdata->source_string);
  MakeEditDialog (_("Rename"), tempstr, cdata->source_string, 1, NULL,
                  gftp_dialog_button_rename,
                  gftpui_run_function_callback, cdata,
                  gftpui_run_function_cancel_callback, cdata);
  g_free (tempstr);
}


void
gftpui_site_dialog (gpointer data)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_site;

  if (!check_status (_("Site"), wdata, 0, 0, 0, wdata->request->site != NULL))
    return;

  MakeEditDialog (_("Site"), _("Enter site-specific command"), NULL, 1,
                  _("Prepend with SITE"), gftp_dialog_button_ok,
                  gftpui_run_function_callback, cdata,
                  gftpui_run_function_cancel_callback, cdata);
}


int
gftpui_run_chdir (gpointer uidata, char *directory)
{
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;
  char *tempstr;
  int ret;

  wdata = uidata;
  if ((tempstr = gftp_expand_path (wdata->request, directory)) == NULL)
    return (FALSE);	  
  
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_chdir;
  cdata->input_string = tempstr;
  cdata->dont_clear_cache = 1;

  ret = gftpui_common_run_callback_function (cdata);

  g_free(tempstr);
  g_free (cdata);
  return (ret);
}


void
gftpui_chdir_dialog (gpointer data)
{
  GList *templist, *filelist;
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;
  int num;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 1, 0,
                     wdata->request->chdir != NULL))
    return;

  templist = gftp_gtk_get_list_selection (wdata);
  num = 0;
  filelist = wdata->files;
  templist = get_next_selection (templist, &filelist, &num);
  curfle = filelist->data;

  tempstr = gftp_build_path (wdata->request, wdata->request->directory,
                             curfle->file, NULL);
  gftpui_run_chdir (wdata, tempstr);
  g_free (tempstr);
}


void 
gftpui_disconnect (void *uidata)
{
  gftp_window_data * wdata;

  wdata = uidata;
  gftp_delete_cache_entry (wdata->request, NULL, 1);
  gftp_disconnect (wdata->request);
  remove_files_window (wdata);

  /* Free the request structure so that all old settings are purged. */
  gftp_request_destroy (wdata->request, 0);
  gftp_gtk_init_request (wdata);

  update_window_info ();
}


char *
gftpui_gtk_get_utf8_file_pos (gftp_file * fle)
{
  char *pos;

  if ((pos = strrchr (fle->file, '/')) != NULL)
    pos++;
  else
    pos = fle->file;

  return (pos);
}


static void
_protocol_yes_answer (gpointer answer, gftp_dialog_data * ddata)
{
  *(int *) answer = 1;
}


static void
_protocol_no_answer (gpointer answer, gftp_dialog_data * ddata)
{
  *(int *) answer = 0;
}


int
gftpui_protocol_ask_yes_no (gftp_request * request, char *title,
                            char *question)
{
  int answer = -1;

  GDK_THREADS_ENTER ();

  MakeYesNoDialog (title, question, _protocol_yes_answer, &answer,
                   _protocol_no_answer, &answer);

  if (gftp_protocols[request->protonum].use_threads)
    {
      /* Let the main loop in the main thread run the events */
      GDK_THREADS_LEAVE ();

      while (answer == -1)
        {
          sleep (1);
        }
    }
  else
    {
      while (answer == -1)
        {
          GDK_THREADS_LEAVE ();
          g_main_context_iteration (NULL, TRUE);
        }
    }

  return (answer);
}


static void
_protocol_ok_answer (char *buf, gftp_dialog_data * ddata)
{
  buf[1] = ' '; /* In case this is an empty string entered */
  strncpy (buf, gtk_entry_get_text (GTK_ENTRY (ddata->edit)), BUFSIZ);
}


static void
_protocol_cancel_answer (char *buf, gftp_dialog_data * ddata)
{
  buf[0] = '\0';
  buf[1] = '\0';
}


char *
gftpui_protocol_ask_user_input (gftp_request * request, char *title,
                                char *question, int shown)
{
  char buf[BUFSIZ];

  GDK_THREADS_ENTER ();

  *buf = '\0';
  *(buf + 1) = ' ';
  MakeEditDialog (title, question, NULL, shown, NULL, gftp_dialog_button_ok,
                  _protocol_ok_answer, &buf, _protocol_cancel_answer, &buf);

  if (gftp_protocols[request->protonum].use_threads)
    {
      /* Let the main loop in the main thread run the events */
      GDK_THREADS_LEAVE ();

      while (*buf == '\0' && *(buf + 1) == ' ')
        {
          sleep (1);
        }
    }
  else
    {
      while (*buf == '\0' && *(buf + 1) == ' ')
        {
          GDK_THREADS_LEAVE ();
          g_main_context_iteration (NULL, TRUE);
        }
    }

  if (*buf != '\0')
    return (g_strdup (buf));
  else
    return (NULL);
}

