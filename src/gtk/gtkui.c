/***********************************************************************************/
/*  gtkui.c - GTK+ UI related functions for gFTP                                   */
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

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "gftp-gtk.h"

void
gftpui_lookup_file_colors (gftp_file * fle, char **start_color,
                           char ** end_color)
{
  DEBUG_PRINT_FUNC
  /* not used in the gtk ui */
  *start_color = NULL;
  *end_color = NULL;
}


void
gftpui_run_command (GtkWidget * widget, gpointer data)
{
  DEBUG_PRINT_FUNC
  const char *txt;

  txt = gtk_entry_get_text (GTK_ENTRY (gftpui_command_widget));
  gftpui_common_process_command (&window1, window1.request,
                                 &window2, window2.request, txt);
  gtk_entry_set_text (GTK_ENTRY (gftpui_command_widget), "");
}


void
gftpui_refresh (void *uidata, int clear_cache_entry)
{
  DEBUG_PRINT_FUNC
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

  remove_files_window (wdata);

   gftp_gtk_list_files (wdata);

  wdata->request->refreshing = 0;
}


#define _GFTPUI_GTK_USER_PW_SIZE	256

static void 
_gftpui_gtk_set_username (gftp_request * request, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftp_set_username (request, ddata->entry_text);
  request->stopable = 0;
}


static void 
_gftpui_gtk_set_password (gftp_request * request, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftp_set_password (request, ddata->entry_text);
  request->stopable = 0;
}


static void
_gftpui_gtk_abort (gftp_request * request, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  request->stopable = 0;
}

void
gftpui_show_busy (gboolean busy)
{
  DEBUG_PRINT_FUNC
  GtkWidget * toplevel = gtk_widget_get_toplevel (openurl_btn);
  GdkWindow * gwin     = gtk_widget_get_window (toplevel);
  GdkDisplay * display = gtk_widget_get_display (toplevel);

  GdkCursor * busyCursor = 
    (busy) ? (gdk_cursor_new_for_display (display, GDK_WATCH)) : NULL;

  gdk_window_set_cursor (gwin, busyCursor);

  if (busy)
    gdk_cursor_unref (busyCursor);
}

void
gftpui_prompt_username (void *uidata, gftp_request * request)
{
  DEBUG_PRINT_FUNC
  TextEntryDialog (NULL, _("Enter Username"),
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
  DEBUG_PRINT_FUNC
  TextEntryDialog (NULL, _("Enter Password"),
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
static gboolean
_gftpui_wakeup_main_thread (GIOChannel *source, GIOCondition condition,
                            gpointer data)
{
  DEBUG_PRINT_FUNC
  gftp_request * request = (gftp_request *) data;
  char c;
  if (request->wakeup_main_thread[0] > 0) {
    if (!read (request->wakeup_main_thread[0], &c, 1))
      return -1;
  }
  return TRUE;
}


static gint
_gftpui_setup_wakeup_main_thread (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  gint handler;

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, request->wakeup_main_thread) == 0)
    {
      request->chan = g_io_channel_unix_new (request->wakeup_main_thread[0]);
      handler = g_io_add_watch (request->chan,
                                G_IO_IN,
                                _gftpui_wakeup_main_thread,
                                request); /* user data */
    }
  else
    {
      request->wakeup_main_thread[0] = 0;
      request->wakeup_main_thread[1] = 0;
      request->chan = NULL;
      handler = 0;
    }

  return (handler);
}


static void
_gftpui_teardown_wakeup_main_thread (gftp_request * request, gint handler)
{
  DEBUG_PRINT_FUNC
  if (request->wakeup_main_thread[0] > 0 && request->wakeup_main_thread[1] > 0)
    {
      g_source_remove (handler);
      g_io_channel_unref (request->chan);
      close (request->wakeup_main_thread[0]);
      close (request->wakeup_main_thread[1]);
      request->wakeup_main_thread[0] = 0;
      request->wakeup_main_thread[1] = 0;
      request->chan = NULL;
    }
}

static void *
_gftpui_gtk_thread_func (void *data)
{
  DEBUG_PRINT_FUNC
  gftpui_gtk_thread_data * thread_data;
  void *ret;

  thread_data = data;
  ret = thread_data->func (thread_data->cdata);

  if (thread_data->cdata->request->wakeup_main_thread[1] > 0)
    if (!write (thread_data->cdata->request->wakeup_main_thread[1], " ", 1))
      return (ret);

  return (ret);
}


void *
gftpui_generic_thread (void * (*func) (void *), void *data)
{
  DEBUG_PRINT_FUNC
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
  if (pthread_create (&wdata->tid, NULL, _gftpui_gtk_thread_func, thread_data) != 0)
    perror("pthread_create failed");

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
  DEBUG_PRINT_FUNC
  gftp_window_data * wdata;

  wdata = cdata->uidata;
  return (wdata->request->cached && wdata->request->datafd < 0 &&
          !wdata->request->always_connected &&
          !gftp_gtk_connect (wdata, wdata->request) ? -1 : 0);
}


void
gftpui_run_function_callback (gftp_window_data * wdata,
                              gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata = (gftpui_callback_data *) ddata->yespointer;

  if (ddata->entry_text[0] == '\0') {
     ftp_log (gftp_logging_error, NULL,
              _("Operation canceled...you must enter a string\n"));
     return;
  }

  cdata->input_string = g_strdup (ddata->entry_text);
  cdata->toggled = ddata->checkbox_is_ticked;

  gftpui_common_run_callback_function (cdata);
  /* free cdata */
  gftpui_run_function_cancel_callback (wdata, ddata);
}


void
gftpui_run_function_cancel_callback (gftp_window_data * wdata,
                                     gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata = (gftpui_callback_data *) ddata->yespointer;

  if (cdata->input_string != NULL)
    g_free (cdata->input_string);
  if (cdata->source_string != NULL)
    g_free (cdata->source_string);
  g_free (cdata);
}


void
gftpui_mkdir_dialog (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_mkdir;

  if (!check_status (_("Mkdir"), wdata, gftpui_common_use_threads (wdata->request), 0, 0, wdata->request->mkdir != NULL))
    return;

  TextEntryDialog (NULL, _("Make Directory"), _("Enter name of directory to create"),
                   NULL, 1, NULL, gftp_dialog_button_create,
                   gftpui_run_function_callback, cdata,
                   gftpui_run_function_cancel_callback, cdata);
}


int
gftpui_common_run_rename_check(gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  if(access(cdata->input_string, F_OK))
    return (gftpui_common_run_rename(cdata));
  else
    ftp_log (gftp_logging_error, NULL,
         _("Operation canceled...a file with the new name already exists\n"));
  return -1;
}


void
gftpui_rename_dialog (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_rename_check;

  if (!check_status (_("Rename"), wdata, gftpui_common_use_threads (wdata->request), 1, 1, wdata->request->rename != NULL))
    return; // only 1 item selected allowed

  curfle = (gftp_file *) listbox_get_selected_files (wdata, 1);

  cdata->source_string = g_strdup (curfle->file);

  tempstr = g_strdup_printf (_("What would you like to rename %s to?"),
                             cdata->source_string);
  TextEntryDialog (NULL, _("Rename"), tempstr, cdata->source_string, 1, NULL,
                   gftp_dialog_button_rename,
                   gftpui_run_function_callback, cdata,
                   gftpui_run_function_cancel_callback, cdata);
  g_free (tempstr);
}


void
gftpui_site_dialog (gpointer data)
{
  DEBUG_PRINT_FUNC
  gftpui_callback_data * cdata;
  gftp_window_data * wdata;

  wdata = data;
  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = gftpui_common_run_site;

  if (!check_status (_("Site"), wdata, 0, 0, 0, wdata->request->site != NULL))
    return;

  TextEntryDialog (NULL, _("Site"), _("Enter site-specific command"), NULL, 1,
                   _("Prepend with SITE"), gftp_dialog_button_ok,
                   gftpui_run_function_callback, cdata,
                   gftpui_run_function_cancel_callback, cdata);
}


int
gftpui_run_chdir (gpointer uidata, char *directory)
{
  DEBUG_PRINT_FUNC
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
  DEBUG_PRINT_FUNC
  gftp_window_data * wdata;
  gftp_file * curfle;
  char *tempstr;

  wdata = data;
  if (!check_status (_("Chdir"), wdata, gftpui_common_use_threads (wdata->request), 1, 0,
                     wdata->request->chdir != NULL))
    return;

  curfle = (gftp_file *) listbox_get_selected_files (wdata, 1);

  tempstr = gftp_build_path (wdata->request, wdata->request->directory,
                             curfle->file, NULL);
  gftpui_run_chdir (wdata, tempstr);
  g_free (tempstr);
}


void 
gftpui_disconnect (void *uidata)
{
  DEBUG_PRINT_FUNC
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
  DEBUG_PRINT_FUNC
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
  DEBUG_PRINT_FUNC
  *(int *) answer = 1;
}


static void
_protocol_no_answer (gpointer answer, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  *(int *) answer = 0;
}


/* Structure to pass data to the main thread for dialog creation */
typedef struct {
  char *title;
  char *question;
  int *answer;
} YesNoDialogData;

/* Callback function that runs on the main thread to create the dialog */
static gboolean
_create_yes_no_dialog_on_main_thread (gpointer user_data)
{
  YesNoDialogData *data = (YesNoDialogData *) user_data;

  YesNoDialog (main_window, data->title, data->question,
               _protocol_yes_answer, data->answer,
               _protocol_no_answer, data->answer);

  /* Free the allocated strings */
  g_free (data->title);
  g_free (data->question);
  g_free (data);

  return FALSE; /* Remove the idle handler */
}

int
gftpui_protocol_ask_yes_no (gftp_request * request, char *title,
                            char *question)
{
  DEBUG_PRINT_FUNC
  int answer = -1;
  YesNoDialogData *data;

  /* On macOS (and GTK3 in general), GUI operations must happen on the main thread.
   * Use g_idle_add to schedule the dialog creation on the main thread. */
  data = g_malloc0 (sizeof (*data));
  data->title = g_strdup (title);
  data->question = g_strdup (question);
  data->answer = &answer;

  /* Schedule the dialog creation on the main thread */
  g_idle_add (_create_yes_no_dialog_on_main_thread, data);

  if (gftp_protocols[request->protonum].use_threads)
    {
      /* Wait for the user to answer */
      while (answer == -1)
        {
          sleep (1);
        }
    }
  else
    {
      /* If not using threads, process events until we get an answer */
      while (answer == -1)
        {
          g_main_context_iteration (NULL, TRUE);
        }
    }

  return (answer);
}


static void
_protocol_ok_answer (char *buf, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  buf[1] = ' '; /* In case this is an empty string entered */
  strncpy (buf, ddata->entry_text, BUFSIZ);
}


static void
_protocol_cancel_answer (char *buf, gftp_dialog_data * ddata)
{
  DEBUG_PRINT_FUNC
  buf[0] = '\0';
  buf[1] = '\0';
}


char *
gftpui_protocol_ask_user_input (gftp_request * request, char *title,
                                char *question, int shown)
{
  DEBUG_PRINT_FUNC
  char buf[BUFSIZ];
  GDK_THREADS_ENTER ();

  *buf = '\0';
  *(buf + 1) = ' ';
  TextEntryDialog (NULL, title, question, NULL, shown, NULL, gftp_dialog_button_ok,
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

