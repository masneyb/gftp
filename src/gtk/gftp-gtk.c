/***********************************************************************************/
/*  gftp-gtk.c - GTK+ port of gftp                                                 */
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
#include "listbox.c"

static GtkWidget * local_frame, * remote_frame, * log_scroll, * transfer_scroll,
                 * gftpui_command_toolbar;

GtkWindow *main_window;
gftp_window_data window1, window2, *other_wdata, *current_wdata;
GtkWidget * stop_btn, * hostedit, * useredit, * passedit, * portedit, * logwdw = NULL,
          * dlwdw, * toolbar_combo_protocol, * gftpui_command_widget, * download_left_arrow,
          * upload_right_arrow, * openurl_btn;
GtkAdjustment * logwdw_vadj;
GtkTextMark * logwdw_textmark;
int local_start, remote_start, trans_start;
GHashTable * graphic_hash_table = NULL;
GHashTable * pixbuf_hash_table = NULL;

GtkActionGroup * menus = NULL;
GtkUIManager * factory = NULL;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t main_thread_id;
GList * viewedit_processes = NULL;
intptr_t gftp_gtk_colored_msgs = 0;

static gboolean on_key_press_combo_toolbar(GtkWidget *widget, GdkEventKey *event, gpointer data);
static void on_combo_protocol_change_cb (GtkComboBox *cb, gpointer data);
static int combo_key_pressed = 0;

static int
get_column (GtkCListColumn * col)
{
  if (col->auto_resize)
    return (0);
  else if (!col->visible)
    return (-1);
  else
    return (col->width);
}


static void
_gftp_exit (GtkWidget * widget, gpointer data)
{
  intptr_t remember_last_directory, ret;
  const char *tempstr;

  ret = gtk_widget_get_allocated_width (GTK_WIDGET (local_frame));
  gftp_set_global_option ("listbox_local_width", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_width (GTK_WIDGET (remote_frame));
  gftp_set_global_option ("listbox_remote_width", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height (GTK_WIDGET (remote_frame));
  gftp_set_global_option ("listbox_file_height", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height (GTK_WIDGET (log_scroll));
  gftp_set_global_option ("log_height", GINT_TO_POINTER (ret));
  ret = gtk_widget_get_allocated_height (GTK_WIDGET (transfer_scroll));
  gftp_set_global_option ("transfer_height", GINT_TO_POINTER (ret));

  listbox_save_column_width (&window1, &window2);

  ret = get_column (&GTK_CLIST (dlwdw)->column[0]);
  gftp_set_global_option ("file_trans_column", GINT_TO_POINTER (ret));

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostedit));
  gftp_set_global_option ("host_value", tempstr);

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(portedit));
  gftp_set_global_option ("port_value", tempstr);

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(useredit));
  gftp_set_global_option ("user_value", tempstr);

  gftp_lookup_global_option ("remember_last_directory",
                             &remember_last_directory);
  if (remember_last_directory)
    {
      tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(window1.dir_combo));
      gftp_set_global_option ("local_startup_directory", tempstr);

      tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(window2.dir_combo));
      gftp_set_global_option ("remote_startup_directory", tempstr);
    }
  else
    {
      // problem: last_directory is always set, we must forget it
      // until remember_last_directory=1 again
      gftp_set_global_option ("local_startup_directory", "");
      gftp_set_global_option ("remote_startup_directory", "");
    }

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(toolbar_combo_protocol));
  gftp_set_global_option ("default_protocol", tempstr);

  if (pixbuf_hash_table) {
     GList *i, *gl = g_hash_table_get_values (pixbuf_hash_table);
     for ( i = gl; i != NULL; i = g_list_next(i) ) {
        g_object_unref (G_OBJECT (i->data)); // (GdkPixbuf *)
     }
     g_list_free (gl);
     g_hash_table_destroy (pixbuf_hash_table);
  }

  gftp_shutdown ();

  exit (0);
}


static gint
_gftp_try_close (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  if (gftp_file_transfers == NULL)
    {
      _gftp_exit (NULL, NULL);
      return (0);
    }
  else
    {
      YesNoDialog (main_window, _("Exit"), _("There are file transfers in progress.\nAre you sure you want to exit?"),
                   _gftp_exit, NULL, NULL, NULL);
      return (1);
    }
}


static void
_gftp_force_close (GtkWidget * widget, gpointer data)
{
  exit (0);
}


static void
_gftp_menu_exit (GtkWidget * widget, gpointer data)
{
  if (!_gftp_try_close (widget, NULL, data))
    _gftp_exit (widget, data);
}


static void on_activate_colored_msgs_cb (GtkToggleAction * checkb)
{
   gboolean state = gtk_toggle_action_get_active (checkb);
   gftp_set_global_option ("colored_msgs_gtk", GINT_TO_POINTER(state));
   gftp_gtk_colored_msgs = state;
   if (logwdw) {
      ftp_log (gftp_logging_misc, NULL,
               state ? _("-- Colors enabled\n") : _("-- Colors disabled\n"));
   }
}

static void change_setting (GtkAction *action, GtkRadioAction *current)
{
  gint menuitem = gtk_radio_action_get_current_value(current);
  switch (menuitem)
    {
    case GFTP_MENU_ITEM_ASCII:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(1));
      break;
    case GFTP_MENU_ITEM_BINARY:
      gftp_set_global_option ("ascii_transfers", GINT_TO_POINTER(0));
      break;
    case GFTP_MENU_ITEM_WIN1:
      current_wdata = &window1;
      other_wdata = &window2;

      if (current_wdata->request != NULL)
        update_window_info ();

      break;
    case GFTP_MENU_ITEM_WIN2:
      current_wdata = &window2;
      other_wdata = &window1;

      if (current_wdata->request != NULL)
        update_window_info ();

      break;
    }
}


static void
_gftpui_gtk_do_openurl (gftp_window_data * wdata, gftp_dialog_data * ddata)
{
  if (ddata->entry_text[0] != '\0') {
     gftpui_common_cmd_open (wdata, wdata->request, NULL, NULL, ddata->entry_text);
  }
}


static void
openurl_dialog (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  TextEntryDialog (NULL, _("Open Location"), _("Enter a URL to connect to"),
                   NULL, 1, NULL, gftp_dialog_button_connect,
                   _gftpui_gtk_do_openurl, wdata,
                   NULL, NULL);
}


static void
tb_openurl_dialog (gpointer data)
{
  const char *edttxt;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Open Location"));
      return;
    }

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostedit));

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftpui_disconnect (current_wdata);
  else if (edttxt != NULL && *edttxt != '\0')
    toolbar_hostedit ();
  else
    openurl_dialog (current_wdata);
}


static void
gftp_gtk_refresh (gftp_window_data * wdata)
{
  gftpui_refresh (wdata, 1);
}


static void navi_up_directory (gftp_window_data * wdata)
{
  char * dir;
  if (!GFTP_IS_CONNECTED (wdata->request)) {
     return;
  }
  dir = gftp_build_path (wdata->request, wdata->request->directory, "..", NULL);
  gftpui_run_chdir (wdata, dir);
  g_free (dir);
}

// ===
static void on_local_openurl_dialog (void) {
  openurl_dialog(&window1);
}
static void on_remote_openurl_dialog (void) {
  openurl_dialog(&window2);
}

static void on_local_gftpui_disconnect (void) {
  gftpui_disconnect(&window1);
}
static void on_remote_gftpui_disconnect (void) {
  gftpui_disconnect(&window2);
}

static void on_local_change_filespec (void) {
  change_filespec(&window1);
}
static void on_remote_change_filespec (void) {
  change_filespec(&window2);
}

static void on_local_show_selected (void) {
  show_selected(&window1);
}
static void on_remote_show_selected (void) {
  show_selected(&window2);
}

static void on_local_navi_up_directory (void) {
  navi_up_directory(&window1);
}
static void on_remote_navi_up_directory (void) {
  navi_up_directory(&window2);
}

static void on_local_selectall (void) {
  window1.show_selected = 0;
  listbox_select_all(&window1);
}
static void on_remote_selectall (void) {
  window2.show_selected = 0;
  listbox_select_all(&window2);
}

static void on_local_selectallfiles (void) {
  window1.show_selected = 0;
  listbox_select_all_files(&window1);
}
static void on_remote_selectallfiles (void) {
  window2.show_selected = 0;
  listbox_select_all_files(&window2);
}

static void on_local_deselectall (void) {
  window1.show_selected = 0;
  listbox_deselect_all(&window1);
}
static void on_remote_deselectall (void) {
  window2.show_selected = 0;
  listbox_deselect_all(&window2);
}

static void on_local_save_directory_listing (void) {
  save_directory_listing(&window1);
}
static void on_remote_save_directory_listing (void) {
  save_directory_listing(&window2);
}

static void on_local_gftpui_site_dialog (void) {
  gftpui_site_dialog(&window1);
}
static void on_remote_gftpui_site_dialog (void) {
  gftpui_site_dialog(&window2);
}

static void on_local_gftpui_chdir_dialog (void) {
  gftpui_chdir_dialog(&window1);
}
static void on_remote_gftpui_chdir_dialog (void) {
  gftpui_chdir_dialog(&window2);
}

static void on_local_chmod_dialog (void) {
  chmod_dialog(&window1);
}
static void on_remote_chmod_dialog (void) {
  chmod_dialog(&window2);
}

static void on_local_gftpui_mkdir_dialog (void) {
  gftpui_mkdir_dialog(&window1);
}
static void on_remote_gftpui_mkdir_dialog (void) {
  gftpui_mkdir_dialog(&window2);
}

static void on_local_gftpui_rename_dialog (void) {
  gftpui_rename_dialog(&window1);
}
static void on_remote_gftpui_rename_dialog (void) {
  gftpui_rename_dialog(&window2);
}

static void on_local_delete_dialog (void) {
  delete_dialog(&window1);
}
static void on_remote_delete_dialog (void) {
  delete_dialog(&window2);
}

static void on_local_edit_dialog(void) {
  edit_dialog(&window1);
}
static void on_remote_edit_dialog(void) {
  edit_dialog(&window2);
}

static void on_local_view_dialog (void) {
  view_dialog(&window1);
}
static void on_remote_view_dialog (void) {
  view_dialog(&window2);
}

static void on_local_gftp_gtk_refresh (void) {
  gftp_gtk_refresh(&window1);
}
static void on_remote_gftp_gtk_refresh (void) {
  gftp_gtk_refresh(&window2);
}
// ===

static GtkWidget *
CreateMenus (GtkWidget * parent)
{
  GtkAccelGroup * accel_group;
  intptr_t ascii_transfers;
  int default_ascii;
  GtkWidget * colormsg_chkbox;

  gftp_lookup_global_option ("colored_msgs_gtk", &gftp_gtk_colored_msgs);

  static const GtkActionEntry menu_items[] =
  {
    //  name                    stock_id               "label"                  accel             tooltip  callback
    { "FTPMenu",              NULL,                  N_("g_FTP"),             NULL,                NULL, NULL                   },
    { "FTPPreferences",       "gtk-preferences",     N_("_Preferences..."),   NULL,                NULL, G_CALLBACK(options_dialog) },
    { "FTPQuit",              "gtk-quit",            N_("_Quit"),             "<control>Q",        NULL, G_CALLBACK(_gftp_menu_exit)  },

    { "LocalMenu",            NULL,                  N_("_Local"),            NULL,                NULL, NULL },
    { "LocalOpenLocation",    "gtk-open",            N_("_Open Location..."), "<control><shift>O", NULL, G_CALLBACK(on_local_openurl_dialog) },
    { "LocalDisconnect",      "gtk-close",           N_("D_isconnect"),       "<control><shift>I", NULL, G_CALLBACK(on_local_gftpui_disconnect) },
    { "LocalChangeFilespec",  NULL,                  N_("Change _Filespec"),  "<control><shift>F", NULL, G_CALLBACK(on_local_change_filespec) },
    { "LocalShowSelected",    NULL,                  N_("_Show selected"),    NULL,                NULL, G_CALLBACK(on_local_show_selected) },
    { "LocalNavigateUp",      NULL,                  N_("Navigate _Up"),      "<ctrl><alt>Up",     NULL, G_CALLBACK(on_local_navi_up_directory) },
    { "LocalSelectAll",       NULL,                  N_("Select _All"),       "<control><shift>A", NULL, G_CALLBACK(on_local_selectall) },
    { "LocalSelectAllFiles",  NULL,                  N_("Select All Files"),  NULL,                NULL, G_CALLBACK(on_local_selectallfiles) },
    { "LocalDeselectAll",     NULL,                  N_("Deselect All"),      NULL,                NULL, G_CALLBACK(on_local_deselectall) },
    { "LocalSaveDirectoryListing", NULL,             N_("Save Directory Listing..."), NULL,        NULL, G_CALLBACK(on_local_save_directory_listing) },
    { "LocalSendSITECommand", NULL,                  N_("Send SITE Command.."),NULL,               NULL, G_CALLBACK(on_local_gftpui_site_dialog) },
    { "LocalChangeDirectory", NULL,                  N_("_Change Directory"), NULL,                NULL, G_CALLBACK(on_local_gftpui_chdir_dialog) },
    { "LocalPermissions",     NULL,                  N_("_Permissions..."),   "<control><shift>P", NULL, G_CALLBACK(on_local_chmod_dialog) },
    { "LocalNewFolder",       NULL,                  N_("_New Folder..."),    "<control><shift>N", NULL, G_CALLBACK(on_local_gftpui_mkdir_dialog) },
    { "LocalRename",          NULL,                  N_("Rena_me..."),        "<control><shift>M", NULL, G_CALLBACK(on_local_gftpui_rename_dialog) },
    { "LocalDelete",          NULL,                  N_("_Delete..."),        "<control><shift>D", NULL, G_CALLBACK(on_local_delete_dialog) },
    { "LocalEdit",            NULL,                  N_("_Edit..."),          "<control><shift>E", NULL, G_CALLBACK(on_local_edit_dialog) },
    { "LocalView",            NULL,                  N_("_View..."),          "<control><shift>L", NULL, G_CALLBACK(on_local_view_dialog) },
    { "LocalRefresh",         "gtk-refresh",         N_("_Refresh"),          "<control><shift>R", NULL, G_CALLBACK(on_local_gftp_gtk_refresh) },

    { "RemoteMenu",           NULL,                  N_("_Remote"),            NULL,               NULL, NULL },
    { "RemoteOpenLocation",   "gtk-open",            N_("_Open Location..."), "<control>O",        NULL, G_CALLBACK(on_remote_openurl_dialog) },
    { "RemoteDisconnect",     "gtk-close",           N_("D_isconnect"),       "<control>I",        NULL, G_CALLBACK(on_remote_gftpui_disconnect) },
    { "RemoteChangeFilespec", NULL,                  N_("Change _Filespec"),  "<control>F",        NULL, G_CALLBACK(on_remote_change_filespec) },
    { "RemoteShowSelected",   NULL,                  N_("_Show selected"),    NULL,                NULL, G_CALLBACK(on_remote_show_selected) },
    { "RemoteNavigateUp",     NULL,                  N_("Navigate _Up"),      "<alt>Up",           NULL, G_CALLBACK(on_remote_navi_up_directory) },
    { "RemoteSelectAll",      NULL,                  N_("Select _All"),       "<control>A",        NULL, G_CALLBACK(on_remote_selectall) },
    { "RemoteSelectAllFiles", NULL,                  N_("Select All Files"),  NULL,                NULL, G_CALLBACK(on_remote_selectallfiles) },
    { "RemoteDeselectAll",    NULL,                  N_("Deselect All"),      NULL,                NULL, G_CALLBACK(on_remote_deselectall) },
    { "RemoteSaveDirectoryListing", NULL,            N_("Save Directory Listing..."), NULL,        NULL, G_CALLBACK(on_remote_save_directory_listing) },
    { "RemoteSendSITECommand",NULL,                  N_("Send SITE Command.."),NULL,               NULL, G_CALLBACK(on_remote_gftpui_site_dialog) },
    { "RemoteChangeDirectory",NULL,                  N_("_Change Directory"), NULL,                NULL, G_CALLBACK(on_remote_gftpui_chdir_dialog) },
    { "RemotePermissions",    NULL,                  N_("_Permissions..."),   "<control>P",        NULL, G_CALLBACK(on_remote_chmod_dialog) },
    { "RemoteNewFolder",      NULL,                  N_("_New Folder..."),    "<control>N",        NULL, G_CALLBACK(on_remote_gftpui_mkdir_dialog) },
    { "RemoteRename",         NULL,                  N_("Rena_me..."),        "<control>M",        NULL, G_CALLBACK(on_remote_gftpui_rename_dialog) },
    { "RemoteDelete",         NULL,                  N_("_Delete..."),        "<control>D",        NULL, G_CALLBACK(on_remote_delete_dialog) },
    { "RemoteEdit",           NULL,                  N_("_Edit..."),          "<control>E",        NULL, G_CALLBACK(on_remote_edit_dialog) },
    { "RemoteView",           NULL,                  N_("_View..."),          "<control>L",        NULL, G_CALLBACK(on_remote_view_dialog) },
    { "RemoteRefresh",        "gtk-refresh",         N_("_Refresh"),          "<control>R",        NULL, G_CALLBACK(on_remote_gftp_gtk_refresh) },

    { "BookmarksMenu",        NULL,                  N_("_Bookmarks"),        NULL,                NULL, NULL },
    { "BookmarksAddBookmark", "gtk-add",             N_("Add _Bookmark"),     "<control>B",        NULL, G_CALLBACK(add_bookmark) },
    { "BookmarksEditBookmarks",NULL,                 N_("Edit Bookmarks"),    NULL,                NULL, G_CALLBACK(edit_bookmarks) },

    { "TransferMenu",         NULL,                  N_("_Transfer"),         NULL,                NULL, NULL },
    { "TransferStart",        NULL,                  N_("_Start"),            NULL,                NULL, G_CALLBACK(start_transfer) },
    { "TransferStop",         "gtk-stop",            N_("St_op"),             NULL,                NULL, G_CALLBACK(stop_transfer) },
    { "TransferSkipCurrentFile",NULL,                N_("Skip _Current File"),NULL,                NULL, G_CALLBACK(skip_transfer) },
    { "TransferRemoveFile",   "gtk-delete",          N_("_Remove File"),      NULL,                NULL, G_CALLBACK(remove_file_transfer) },
    { "TransferMoveFileUp",   "gtk-go-up",           N_("Move File _Up"),     NULL,                NULL, G_CALLBACK(move_transfer_up) },
    { "TransferMoveFileDown", "gtk-go-down",         N_("Move File _Down"),   NULL,                NULL, G_CALLBACK(move_transfer_down) },
    { "TransferRetrieveFiles",NULL,                  N_("_Retrieve Files"),   "<control>R",        NULL, G_CALLBACK(get_files) },
    { "TransferPutFiles",    NULL,                   N_("_Put Files"),        "<control>U",        NULL, G_CALLBACK(put_files) },

    { "LogMenu",             NULL,                   N_("L_og"),              NULL,                NULL, NULL },
    { "LogClear",            "gtk-clear",            N_("_Clear"),            NULL,                NULL, G_CALLBACK(clearlog) },
    { "LogView",             NULL,                   N_("_View"),             NULL,                NULL, G_CALLBACK(viewlog) },
    { "LogSave",             "gtk-save",             N_("_Save..."),          NULL,                NULL, G_CALLBACK(savelog) },

    { "ToolsMenu",           NULL,                   N_("Tool_s"),            NULL,                NULL, NULL },
    { "ToolsCompareWindows", NULL,                   N_("C_ompare Windows"),  NULL,                NULL, G_CALLBACK(compare_windows) },
    { "ToolsClearCache",     "gtk-clear",            N_("_Clear Cache"),      NULL,                NULL, G_CALLBACK(clear_cache) },

    { "HelpMenu",            NULL,                   N_("_Help"),             NULL,                NULL, NULL },
    { "HelpAbout",           "gtk-about",            N_("_About"),            NULL,                NULL, G_CALLBACK(about_dialog) },
  };

  static const GtkToggleActionEntry menu_check_colormsg[] = {
    { "FTPColorMsg", NULL, N_("Colored messages"), "<control><alt>C", NULL,
      G_CALLBACK (on_activate_colored_msgs_cb), FALSE }, //FALSE: need a constant element
  };

  static const GtkRadioActionEntry menu_radio_window[] = {
    { "FTPWindow1", NULL, N_("Window _1"), "<control>1", NULL, GFTP_MENU_ITEM_WIN1 },
    { "FTPWindow2", NULL, N_("Window _2"), "<control>2", NULL, GFTP_MENU_ITEM_WIN2 },
  };
  static const GtkRadioActionEntry menu_radio_mode[] = {
    { "FTPAscii",  NULL, N_("_Ascii"),  NULL, NULL, GFTP_MENU_ITEM_ASCII },
    { "FTPBinary", NULL, N_("_Binary"), NULL, NULL, GFTP_MENU_ITEM_BINARY },
  };

  guint nmenu_items = G_N_ELEMENTS (menu_items);

  static const gchar *ui_info = " \
  <ui> \
    <menubar name='M'> \
      <menu action='FTPMenu'> \
        <menuitem action='FTPColorMsg'/> \
        <separator/> \
        <menuitem action='FTPWindow1'/> \
        <menuitem action='FTPWindow2'/> \
        <separator/> \
        <menuitem action='FTPAscii'/> \
        <menuitem action='FTPBinary'/> \
        <separator/> \
        <menuitem action='FTPPreferences'/> \
        <separator/> \
        <menuitem action='FTPQuit'/> \
      </menu> \
      <menu action='LocalMenu'> \
        <menuitem action='LocalOpenLocation'/> \
        <menuitem action='LocalDisconnect'/> \
        <separator/> \
        <menuitem action='LocalChangeFilespec'/> \
        <menuitem action='LocalShowSelected'/> \
        <menuitem action='LocalNavigateUp'/> \
        <menuitem action='LocalSelectAll'/> \
        <menuitem action='LocalSelectAllFiles'/> \
        <menuitem action='LocalDeselectAll'/> \
        <separator/> \
        <menuitem action='LocalSaveDirectoryListing'/> \
        <menuitem action='LocalSendSITECommand'/> \
        <menuitem action='LocalChangeDirectory'/> \
        <menuitem action='LocalPermissions'/> \
        <menuitem action='LocalNewFolder'/> \
        <menuitem action='LocalRename'/> \
        <menuitem action='LocalDelete'/> \
        <menuitem action='LocalEdit'/> \
        <menuitem action='LocalView'/> \
        <menuitem action='LocalRefresh'/> \
      </menu> \
      <menu action='RemoteMenu'> \
        <menuitem action='RemoteOpenLocation'/> \
        <menuitem action='RemoteDisconnect'/> \
        <separator/> \
        <menuitem action='RemoteChangeFilespec'/> \
        <menuitem action='RemoteShowSelected'/> \
        <menuitem action='RemoteNavigateUp'/> \
        <menuitem action='RemoteSelectAll'/> \
        <menuitem action='RemoteSelectAllFiles'/> \
        <menuitem action='RemoteDeselectAll'/> \
        <separator/> \
        <menuitem action='RemoteSaveDirectoryListing'/> \
        <menuitem action='RemoteSendSITECommand'/> \
        <menuitem action='RemoteChangeDirectory'/> \
        <menuitem action='RemotePermissions'/> \
        <menuitem action='RemoteNewFolder'/> \
        <menuitem action='RemoteRename'/> \
        <menuitem action='RemoteDelete'/> \
        <menuitem action='RemoteEdit'/> \
        <menuitem action='RemoteView'/> \
        <menuitem action='RemoteRefresh'/> \
      </menu> \
      <menu action='BookmarksMenu'> \
        <menuitem action='BookmarksAddBookmark'/> \
        <menuitem action='BookmarksEditBookmarks'/> \
        <separator/> \
      </menu> \
      <menu action='TransferMenu'> \
        <menuitem action='TransferStart'/> \
        <menuitem action='TransferStop'/> \
        <separator/> \
        <menuitem action='TransferSkipCurrentFile'/> \
        <menuitem action='TransferRemoveFile'/> \
        <menuitem action='TransferMoveFileUp'/> \
        <menuitem action='TransferMoveFileDown'/> \
        <separator/> \
        <menuitem action='TransferRetrieveFiles'/> \
        <menuitem action='TransferPutFiles'/> \
      </menu> \
      <menu action='LogMenu'> \
        <menuitem action='LogClear'/> \
        <menuitem action='LogView'/> \
        <menuitem action='LogSave'/> \
      </menu> \
      <menu action='ToolsMenu'> \
        <menuitem action='ToolsCompareWindows'/> \
        <menuitem action='ToolsClearCache'/> \
      </menu> \
      <menu action='HelpMenu'> \
        <menuitem action='HelpAbout'/> \
      </menu> \
    </menubar> \
    \
    <popup action='LocalPopupMenu'> \
        <menuitem action='LocalOpenLocation'/> \
        <menuitem action='LocalDisconnect'/> \
        <separator/> \
        <menuitem action='LocalChangeFilespec'/> \
        <menuitem action='LocalShowSelected'/> \
        <menuitem action='LocalNavigateUp'/> \
        <menuitem action='LocalSelectAll'/> \
        <menuitem action='LocalSelectAllFiles'/> \
        <menuitem action='LocalDeselectAll'/> \
        <separator/> \
        <menuitem action='LocalSaveDirectoryListing'/> \
        <menuitem action='LocalSendSITECommand'/> \
        <menuitem action='LocalChangeDirectory'/> \
        <menuitem action='LocalPermissions'/> \
        <menuitem action='LocalNewFolder'/> \
        <menuitem action='LocalRename'/> \
        <menuitem action='LocalDelete'/> \
        <menuitem action='LocalEdit'/> \
        <menuitem action='LocalView'/> \
        <menuitem action='LocalRefresh'/> \
    </popup> \
      \
    <popup action='RemotePopupMenu'> \
        <menuitem action='RemoteOpenLocation'/> \
        <menuitem action='RemoteDisconnect'/> \
        <separator/> \
        <menuitem action='RemoteChangeFilespec'/> \
        <menuitem action='RemoteShowSelected'/> \
        <menuitem action='RemoteNavigateUp'/> \
        <menuitem action='RemoteSelectAll'/> \
        <menuitem action='RemoteSelectAllFiles'/> \
        <menuitem action='RemoteDeselectAll'/> \
        <separator/> \
        <menuitem action='RemoteSaveDirectoryListing'/> \
        <menuitem action='RemoteSendSITECommand'/> \
        <menuitem action='RemoteChangeDirectory'/> \
        <menuitem action='RemotePermissions'/> \
        <menuitem action='RemoteNewFolder'/> \
        <menuitem action='RemoteRename'/> \
        <menuitem action='RemoteDelete'/> \
        <menuitem action='RemoteEdit'/> \
        <menuitem action='RemoteView'/> \
        <menuitem action='RemoteRefresh'/> \
    </popup> \
    \
    <popup action='TransferPopupMenu'> \
        <menuitem action='TransferStart'/> \
        <menuitem action='TransferStop'/> \
        <separator/> \
        <menuitem action='TransferSkipCurrentFile'/> \
        <menuitem action='TransferRemoveFile'/> \
        <menuitem action='TransferMoveFileUp'/> \
        <menuitem action='TransferMoveFileDown'/> \
        <separator/> \
        <menuitem action='TransferRetrieveFiles'/> \
        <menuitem action='TransferPutFiles'/> \
    </popup> \
  </ui>";

  factory = gtk_ui_manager_new();
  
  GtkActionGroup *actions = gtk_action_group_new("Actions");

  // must set translate_func or domain before adding actions
  gtk_action_group_set_translation_domain (actions, "gftp");

  gtk_action_group_add_actions(actions, menu_items, nmenu_items, NULL);

  current_wdata = &window2;

  // colored msgs
  gtk_action_group_add_toggle_actions (actions, menu_check_colormsg,
                                       G_N_ELEMENTS (menu_check_colormsg), 0);
  // Windows 1/2
  gtk_action_group_add_radio_actions(actions, menu_radio_window,
                               G_N_ELEMENTS (menu_radio_window),
                               GFTP_MENU_ITEM_WIN2,
                               G_CALLBACK(change_setting),
                               0);
  // ascii binary
  gftp_lookup_global_option ("ascii_transfers", &ascii_transfers);
  if (ascii_transfers)
    default_ascii = GFTP_MENU_ITEM_ASCII;
  else
    default_ascii = GFTP_MENU_ITEM_BINARY;
  gtk_action_group_add_radio_actions(actions, menu_radio_mode,
                               G_N_ELEMENTS (menu_radio_mode),
                               default_ascii,
                               G_CALLBACK(change_setting),
                               0);
  gtk_ui_manager_insert_action_group(factory, actions, 0);
  g_object_unref(actions);
  if (!gtk_ui_manager_add_ui_from_string(GTK_UI_MANAGER(factory), ui_info, -1, NULL))
     ftp_log (gftp_logging_error, NULL, "error");
  accel_group = gtk_ui_manager_get_accel_group(factory);
  GtkWidget* menu = gtk_ui_manager_get_widget(factory, "/M");

  menus = actions;
  
  window1.ifactory = factory;
  window2.ifactory = factory;

  build_bookmarks_menu ();

  gtk_window_add_accel_group (GTK_WINDOW (parent), accel_group);

  // the menu is now ready. enable/disable colored msgs
  colormsg_chkbox = gtk_ui_manager_get_widget (factory, "/M/FTPMenu/FTPColorMsg");
  if (colormsg_chkbox) {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (colormsg_chkbox), gftp_gtk_colored_msgs);
  }

  return (menu);
}


static GtkWidget *
CreateConnectToolbar (GtkWidget * parent)
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  GtkWidget *toolbar, *box, *tempwid;
  gftp_config_list_vars * tmplistvar;
  GtkWidget *dir_combo_entry;
  char *default_protocol, *tempstr;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

#if GTK_MAJOR_VERSION == 2
  toolbar = gtk_handle_box_new ();
  gtk_container_add (GTK_CONTAINER (toolbar), box);
#else
  toolbar = box;
#endif

  //tempwid = gtk_image_new_from_icon_name ("gtk-network", GTK_ICON_SIZE_SMALL_TOOLBAR);
  tempwid = gtk_image_new_from_stock ("gtk-network", GTK_ICON_SIZE_SMALL_TOOLBAR);

  openurl_btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (openurl_btn), tempwid);
  g_signal_connect_swapped (G_OBJECT (openurl_btn), "clicked",
			     G_CALLBACK (tb_openurl_dialog), NULL);
  g_signal_connect (G_OBJECT (openurl_btn), "drag_data_received",
		      G_CALLBACK (openurl_get_drag_data), NULL);
  gtk_drag_dest_set (openurl_btn, GTK_DEST_DEFAULT_ALL, possible_types, 2,
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_container_set_border_width (GTK_CONTAINER (openurl_btn), 1);
  gtk_box_pack_start (GTK_BOX (box), openurl_btn, FALSE, FALSE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_Host:"));

  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  hostedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (hostedit, 120, -1);

  gftp_lookup_global_option ("hosthistory", &tmplistvar);
  glist_to_combobox (tmplistvar->list, hostedit);

  gftp_lookup_global_option ("host_value", &tempstr);
  dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(hostedit)));
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), hostedit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Port:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  portedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (portedit, 80, -1);

  gftp_lookup_global_option ("porthistory", &tmplistvar);
  glist_to_combobox (tmplistvar->list, portedit);

  gftp_lookup_global_option ("port_value", &tempstr);

  dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), portedit, FALSE, FALSE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_User:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  useredit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (useredit, 75, -1);

  gftp_lookup_global_option ("userhistory", &tmplistvar);
  glist_to_combobox (tmplistvar->list, useredit);

  gftp_lookup_global_option ("user_value", &tempstr);

  dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(useredit)));
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), useredit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Pass:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  passedit = gtk_entry_new ();
  gtk_widget_set_size_request (passedit, 120, -1);

  gtk_entry_set_visibility (GTK_ENTRY (passedit), FALSE);
  g_signal_connect (G_OBJECT (passedit), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (G_OBJECT (passedit), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  gtk_box_pack_start (GTK_BOX (box), passedit, FALSE, FALSE, 0);

  tempwid = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  toolbar_combo_protocol = gtk_combo_box_text_new ();
  gtk_box_pack_start (GTK_BOX (tempwid), toolbar_combo_protocol, TRUE, FALSE, 0);
  g_signal_connect (G_OBJECT (toolbar_combo_protocol),
                    "changed",
                    G_CALLBACK (on_combo_protocol_change_cb),
                    NULL);

  gftp_lookup_global_option ("default_protocol", &default_protocol);
  populate_combo_and_select_protocol (toolbar_combo_protocol, default_protocol);

  //tempwid = gtk_image_new_from_icon_name ("gtk-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
  tempwid = gtk_image_new_from_stock ("gtk-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);

  stop_btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (stop_btn), tempwid);
  gtk_widget_set_sensitive (stop_btn, 0);
  g_signal_connect_swapped (G_OBJECT (stop_btn), "clicked",
			     G_CALLBACK (stop_button), NULL);
  gtk_container_set_border_width (GTK_CONTAINER (stop_btn), 1);
  gtk_box_pack_start (GTK_BOX (box), stop_btn, FALSE, FALSE, 0);

  return (toolbar);
}


static GtkWidget *
CreateCommandToolbar (void)
{
  GtkWidget * box, * tempwid;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

#if GTK_MAJOR_VERSION == 2
  // GtkHandleBox is broken in GTK3 - macOS
  gftpui_command_toolbar = gtk_handle_box_new ();
  gtk_container_add (GTK_CONTAINER (gftpui_command_toolbar), box);
#else
  gftpui_command_toolbar = box;
#endif

  tempwid = gtk_label_new (_("Command: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  gftpui_command_widget = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), gftpui_command_widget, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (gftpui_command_widget), "activate",
		      G_CALLBACK (gftpui_run_command), NULL);

  return (gftpui_command_toolbar);
}


static gboolean
on_listbox_key_press_cb (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  gftp_window_data * wdata = data;
  if (!GFTP_IS_CONNECTED (wdata->request))
    return (TRUE);

  if (event->type == GDK_KEY_PRESS) {
     if (event->keyval == GDK_KEY(KP_Delete) || event->keyval == GDK_KEY(Delete)) {
        delete_dialog (wdata);
        return TRUE;
      //} else if (event->keyval == GDK_KEY(Return)) { .. see on_listbox_row_activated_cb
     }
  }
  return (FALSE);
}


static gboolean
on_listbox_right_click_cb (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  // display popup menu for listbox
  gftp_window_data * wdata = data;

  if (event->button == 3) {
    if (wdata->request->protonum == GFTP_PROTOCOL_LOCALFS) {
      GtkWidget* menu = gtk_ui_manager_get_widget(factory, "/LocalPopupMenu");
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
    }
    else {
      GtkWidget* menu = gtk_ui_manager_get_widget(factory, "/RemotePopupMenu");
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
    }
  }
  return (FALSE);
}


static void 
on_listbox_row_activated_cb (GtkTreeView *tree_view,    GtkTreePath *path,
                             GtkTreeViewColumn *column, gpointer data)
{
  // https://developer.gnome.org/gtk2/stable/GtkTreeView.html#GtkTreeView-row-activated
  gftp_window_data * wdata = data;
  if (listbox_num_selected(wdata) != 1) {
     return;
  }

  if (check_reconnect (wdata) < 0) 
     return;

  // only 1 row selected - double click / enter
  // list directory or download file

  gftp_file * tempfle = (gftp_file *) listbox_get_selected_files (wdata, 1);
  if (!tempfle) {
     fprintf (stderr, "* on_listbox_row_activated_cb(): ERROR\n"); // debug
     return;
  }

  char      *directory;
  int success = 0;

  if (S_ISLNK (tempfle->st_mode) || S_ISDIR (tempfle->st_mode))
  {
      directory = gftp_build_path (wdata->request, wdata->request->directory,
                                   tempfle->file, NULL);
      success = gftpui_run_chdir (wdata, directory);
      g_free (directory);
  }

  if (tempfle && !S_ISDIR (tempfle->st_mode) && !success)
  {
     if (wdata == &window2)
        get_files (wdata);
     else
        put_files (wdata);
  }
}


void
gftp_gtk_init_request (gftp_window_data * wdata)
{
  wdata->request->logging_function = ftp_log;
  wdata->filespec = g_malloc0 (2);
  *wdata->filespec = '*';
}


static GtkWidget *
CreateFTPWindow (gftp_window_data * wdata)
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  char tempstr[50], *startup_directory;
  GtkWidget *box, *scroll_list, *parent;
  intptr_t listbox_file_height, colwidth;
  GtkWidget *dir_combo_entry;

  wdata->request = gftp_request_new ();
  gftp_gtk_init_request (wdata);

  parent = gtk_frame_new (NULL);
  
  gftp_lookup_global_option ("listbox_file_height", &listbox_file_height);
  g_snprintf (tempstr, sizeof (tempstr), "listbox_%s_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  gtk_widget_set_size_request (parent, colwidth, listbox_file_height);

  gtk_container_set_border_width (GTK_CONTAINER (parent), 5);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (parent), box);

  wdata->dir_combo = gtk_combo_box_text_new_with_entry ();
  gtk_box_pack_start (GTK_BOX (box), wdata->dir_combo, FALSE, FALSE, 0);

  dir_combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(wdata->dir_combo)));
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_press_event",
                   G_CALLBACK (dir_combo_keycb), NULL);
  g_signal_connect (G_OBJECT(dir_combo_entry), "key_release_event",
                   G_CALLBACK (dir_combo_keycb), wdata);

  glist_to_combobox (*wdata->history, wdata->dir_combo);

  g_snprintf (tempstr, sizeof (tempstr), "%s_startup_directory",
              wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &startup_directory);
  gtk_entry_set_text (GTK_ENTRY (dir_combo_entry), startup_directory);

  wdata->dirinfo_label = gtk_label_new (NULL);
  gtkcompat_widget_set_halign_left (wdata->dirinfo_label);
  gtk_box_pack_start (GTK_BOX (box), wdata->dirinfo_label, FALSE, FALSE, 0);

  scroll_list = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_list),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /* create GtkTreeView */
  wdata->listbox = create_listbox(wdata);
  listbox_add_columns (wdata);             /* add columns to the tree view */
  listbox_set_default_column_width (wdata);

  gtk_container_add (GTK_CONTAINER (scroll_list), wdata->listbox);
  gtk_box_pack_start (GTK_BOX (box), scroll_list, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (wdata->listbox), "drag_data_get",
  /* dnd.c */       G_CALLBACK (listbox_drag), (gpointer) wdata);
  g_signal_connect (G_OBJECT (wdata->listbox), "drag_data_received",
  /* dnd.c */       G_CALLBACK (listbox_get_drag_data), (gpointer) wdata);
  gtk_drag_source_set (wdata->listbox, GDK_BUTTON1_MASK,
  /* dnd.c */          possible_types, 3,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drag_dest_set (wdata->listbox, GTK_DEST_DEFAULT_ALL,
  /* dnd.c */        possible_types, 2,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  g_signal_connect (G_OBJECT (wdata->listbox),  "row_activated",
                    G_CALLBACK(on_listbox_row_activated_cb), (gpointer) wdata);
  g_signal_connect (G_OBJECT (wdata->listbox),  "key_press_event",
                    G_CALLBACK (on_listbox_key_press_cb), (gpointer) wdata);
  g_signal_connect (G_OBJECT   (wdata->listbox), "button_press_event",
                    G_CALLBACK (on_listbox_right_click_cb),
                    (gpointer) wdata);

  return (parent);
}


static gint
on_key_press_transfer (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  if (event->button == 3) {
    GtkWidget* menu = gtk_ui_manager_get_widget(factory, "/TransferPopupMenu");
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
  }
  return (FALSE);
}


static GtkWidget *
CreateFTPWindows (GtkWidget * ui)
{
  GtkWidget *box, *dlbox, *winpane, *dlpane, *logpane, *mainvbox, *tempwid;
  gftp_config_list_vars * tmplistvar;
  char *dltitles[2];
  intptr_t tmplookup;
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  GtkTextTag *tag;
  GdkColor * fore;

  memset (&window1, 0, sizeof (window1));
  memset (&window2, 0, sizeof (window2));

  gftp_lookup_global_option ("localhistory", &tmplistvar);
  window1.history = &tmplistvar->list;
  window1.histlen = &tmplistvar->num_items;

  gftp_lookup_global_option ("remotehistory", &tmplistvar);
  window2.history = &tmplistvar->list;
  window2.histlen = &tmplistvar->num_items;
 
  mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  tempwid = CreateMenus (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateConnectToolbar (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateCommandToolbar ();
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  winpane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  window1.prefix_col_str = "local";
  local_frame = CreateFTPWindow (&window1);
  gtk_box_pack_start (GTK_BOX (box), local_frame, TRUE, TRUE, 0);

  dlbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dlbox), 5);
  gtk_box_pack_start (GTK_BOX (box), dlbox, FALSE, FALSE, 0);

  tempwid = gtk_image_new_from_stock ("gtk-go-forward",
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);

  upload_right_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), upload_right_arrow, TRUE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (upload_right_arrow), "clicked",
			     G_CALLBACK (put_files), NULL);
  gtk_container_add (GTK_CONTAINER (upload_right_arrow), tempwid);

  tempwid = gtk_image_new_from_stock ("gtk-go-back",
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);

  download_left_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), download_left_arrow, TRUE, FALSE, 0);
  g_signal_connect_swapped (G_OBJECT (download_left_arrow), "clicked",
			     G_CALLBACK (get_files), NULL);
  gtk_container_add (GTK_CONTAINER (download_left_arrow), tempwid);

  gtk_paned_pack1 (GTK_PANED (winpane), box, 1, 1);

  window2.prefix_col_str = "remote";
  remote_frame = CreateFTPWindow (&window2);

  gtk_paned_pack2 (GTK_PANED (winpane), remote_frame, 1, 1);

  dlpane = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_paned_pack1 (GTK_PANED (dlpane), winpane, 1, 1);

  transfer_scroll = gtk_scrolled_window_new (NULL, NULL);
  gftp_lookup_global_option ("transfer_height", &tmplookup);
  gtk_widget_set_size_request (transfer_scroll, -1, tmplookup);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (transfer_scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  dltitles[0] = _("Filename");
  dltitles[1] = _("Progress");
  dlwdw = gtk_ctree_new_with_titles (2, 0, dltitles);
  gtk_clist_set_selection_mode (GTK_CLIST (dlwdw), GTK_SELECTION_SINGLE);
  gtk_clist_set_reorderable (GTK_CLIST (dlwdw), 0);

  gftp_lookup_global_option ("file_trans_column", &tmplookup);
  if (tmplookup == 0)
    gtk_clist_set_column_auto_resize (GTK_CLIST (dlwdw), 0, TRUE);
  else
    gtk_clist_set_column_width (GTK_CLIST (dlwdw), 0, tmplookup);

  gtk_container_add (GTK_CONTAINER (transfer_scroll), dlwdw);
  g_signal_connect (G_OBJECT (dlwdw), "button_press_event",
        G_CALLBACK (on_key_press_transfer), NULL);
  gtk_paned_pack2 (GTK_PANED (dlpane), transfer_scroll, 1, 1);

  // log_scroll ("log window")
  logpane = gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (mainvbox), logpane, TRUE, TRUE, 0);

  log_scroll = gtk_scrolled_window_new (NULL, NULL);

  gtk_paned_pack1 (GTK_PANED (logpane), dlpane,     1, 1);
  gtk_paned_pack2 (GTK_PANED (logpane), log_scroll, 1, 1);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (log_scroll),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  logwdw_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (log_scroll));

  gftp_lookup_global_option ("log_height", &tmplookup);
  gtk_widget_set_size_request (log_scroll, -1, tmplookup);

  logwdw = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (log_scroll), logwdw);

  gtk_text_view_set_editable (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (logwdw), GTK_WRAP_WORD);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));

  tag = gtk_text_buffer_create_tag (textbuf, "send", NULL);
  gftp_lookup_global_option ("send_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "recv", NULL);
  gftp_lookup_global_option ("recv_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "error", NULL);
  gftp_lookup_global_option ("error_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "misc", NULL);
  gftp_lookup_global_option ("misc_color", &fore);
  g_object_set (G_OBJECT (tag), "foreground_gdk", &fore, NULL);

  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  logwdw_textmark = gtk_text_buffer_create_mark (textbuf, "end", &iter, 1);

  gtk_container_set_border_width (GTK_CONTAINER (transfer_scroll), 5);
  gtk_container_set_border_width (GTK_CONTAINER (log_scroll), 5);

  // show
  gtk_widget_show_all (mainvbox);
  gftpui_show_or_hide_command ();
  return (mainvbox);
}


static int
_get_selected_protocol ()
{
  int i;
  char *activetxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(toolbar_combo_protocol));
  for (i = 0; gftp_protocols[i].name; i++) {
    if (activetxt && strcmp (gftp_protocols[i].name, activetxt) == 0) {
      break;
    }
  }
  //fprintf(stderr,"combo: %s\nproto:%s %d\n", activetxt, gftp_protocols[i].name, i);
  return i;
}

static void
on_combo_protocol_change_cb (GtkComboBox *cb, gpointer data)
{
  GtkComboBoxText *combo = GTK_COMBO_BOX_TEXT(cb);
  char *txt = gtk_combo_box_text_get_active_text (combo);
  gftp_set_global_option ("default_protocol", txt);
}

static gboolean
on_key_press_combo_toolbar(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->type == GDK_KEY_PRESS) {
    if (event->keyval == GDK_KEY(Return))
       combo_key_pressed = 1;
    return FALSE;
  }
  else if (event->type == GDK_KEY_RELEASE) {
    if (combo_key_pressed == 0)
      return FALSE;
  }
  else {
    return FALSE;
  }

  if (event->keyval != GDK_KEY(Return)) {
    return FALSE;
  }
  toolbar_hostedit();
  combo_key_pressed = 0;
  return TRUE;
}

void
toolbar_hostedit(void)
{
  int (*init) (gftp_request * request);
  gftp_config_list_vars * tmplistvar;
  const char *txt;
  int num;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Connect"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftp_disconnect (current_wdata->request);

  num = _get_selected_protocol ();
  init = gftp_protocols[num].init;
  if (init (current_wdata->request) < 0)
    return;
 
  txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostedit));
  if (strchr (txt, '/') != NULL) 
    {
      /* The user entered a URL in the host box... */
      gftpui_common_cmd_open (current_wdata, current_wdata->request, NULL, NULL, txt);
      return;
    }

  gftp_set_hostname (current_wdata->request, txt);
  if (current_wdata->request->hostname == NULL)
    return;
  alltrim (current_wdata->request->hostname);

  if (current_wdata->request->need_hostport && 
      *current_wdata->request->hostname == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Error: You must type in a host to connect to\n"));
      return;
    }

  gftp_lookup_global_option ("hosthistory", &tmplistvar);
  add_history (hostedit, &tmplistvar->list, &tmplistvar->num_items, 
               current_wdata->request->hostname);

  if (strchr (current_wdata->request->hostname, '/') != NULL &&
      gftp_parse_url (current_wdata->request, 
                      current_wdata->request->hostname) == 0)
    {
      gftp_gtk_connect (current_wdata, current_wdata->request);
      return;
    }
 
  txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(portedit));
  gftp_set_port (current_wdata->request, strtol (txt, NULL, 10));

  gftp_lookup_global_option ("porthistory", &tmplistvar);
  add_history (portedit, &tmplistvar->list, &tmplistvar->num_items, txt);

  gftp_set_username (current_wdata->request, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(useredit)));
  if (current_wdata->request->username != NULL)
    alltrim (current_wdata->request->username);


  gftp_lookup_global_option ("userhistory", &tmplistvar);
  add_history (useredit, &tmplistvar->list, &tmplistvar->num_items, 
               current_wdata->request->username);

  gftp_set_password (current_wdata->request,
		     gtk_entry_get_text (GTK_ENTRY (passedit)));

  gftp_set_directory (current_wdata->request,
                      gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(current_wdata->dir_combo)));
  if (current_wdata->request->directory != NULL)
    alltrim (current_wdata->request->directory);

  add_history (current_wdata->dir_combo, current_wdata->history, 
               current_wdata->histlen, current_wdata->request->directory);

  gftp_gtk_connect (current_wdata, current_wdata->request);
}

void
stop_button (GtkWidget * widget, gpointer data)
{
  pthread_t comptid;

  memset (&comptid, 0, sizeof (comptid));
  if (!pthread_equal (comptid, window1.tid))
    {
      window1.request->cancel = 1;
      pthread_kill (window1.tid, SIGINT);
    }
  else if (!pthread_equal (comptid, window2.tid))
    {
      window2.request->cancel = 1;
      pthread_kill (window2.tid, SIGINT);
    }
}


static int
gftp_gtk_config_file_read_color (char *str, gftp_config_vars * cv, int line)
{
  char *red, *green, *blue;
  GdkColor * color;

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM && cv->value != NULL)
    g_free (cv->value);

  gftp_config_parse_args (str, 3, line, &red, &green, &blue);

  color = g_malloc0 (sizeof (*color));
  color->red = strtol (red, NULL, 16);
  color->green = strtol (green, NULL, 16);
  color->blue = strtol (blue, NULL, 16);

  g_free (red);
  g_free (green);
  g_free (blue);

  cv->value = color;
  cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;

  return (0);
}


static int
gftp_gtk_config_file_write_color (gftp_config_vars * cv, char *buf,
                                  size_t buflen, int to_config_file)
{
  GdkColor * color;

  color = cv->value;
  g_snprintf (buf, buflen, "%x:%x:%x", color->red, color->green, color->blue);
  return (0);
}


static void
_setup_window1 ()
{
  if (gftp_protocols[GFTP_PROTOCOL_LOCALFS].init (window1.request) == 0)
    {
      gftp_setup_startup_directory (window1.request,
                                    "local_startup_directory");
      gftp_connect (window1.request);
       gftp_gtk_list_files (&window1);
    }
}


static void _setup_window2 (int argc, char **argv)
{
  intptr_t connect_to_remote_on_startup;
  char * rhost;
  gftp_lookup_request_option (window2.request, "connect_to_remote_on_startup",
                              &connect_to_remote_on_startup);

  if (argc == 2 && strncmp (argv[1], "--", 2) != 0)
    {
      if (gftp_parse_url (window2.request, argv[1]) == 0)
        gftp_gtk_connect (&window2, window2.request);
      else
        gftp_usage ();
    }
  else if (connect_to_remote_on_startup)
    {
       rhost = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (hostedit));
       if (rhost && *rhost) {
          ftp_log (gftp_logging_misc, NULL, _("** Connect to remote server on startup is enabled\n"));
          gftp_setup_startup_directory (window2.request, "remote_startup_directory");
          tb_openurl_dialog (&window2);
       }
    }
  else
    {
      /* On the remote window, even though we aren't connected, draw the sort
         icon on that side */
      listbox_sort_rows ( (gpointer) &window2, -1);
    }
}


int
main (int argc, char **argv)
{
  GtkWidget *ui;

  /* We override the read color functions because we are using a GdkColor 
     structures to store the color. If I put this in lib/config_file.c, then 
     the core library would be dependant on Gtk+ being present */
  gftp_option_types[gftp_option_type_color].read_function = gftp_gtk_config_file_read_color;
  gftp_option_types[gftp_option_type_color].write_function = gftp_gtk_config_file_write_color;

  gftpui_common_init (&argc, &argv, ftp_log);
  gftpui_common_child_process_done = 0;

#if ! GLIB_CHECK_VERSION (2,32,0)
  g_thread_init (NULL);
#endif

  gdk_threads_init();
  GDK_THREADS_ENTER ();
  main_thread_id = pthread_self ();
  gtk_init (&argc, &argv);

  graphic_hash_table = g_hash_table_new (string_hash_function,
                                         string_hash_compare);

  pixbuf_hash_table = g_hash_table_new (string_hash_function,  /* lib/misc.c */
                                         string_hash_compare); /* lib/misc.c */

  main_window = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
  g_signal_connect (G_OBJECT (main_window), "delete_event",
		      G_CALLBACK (_gftp_try_close), NULL);
  g_signal_connect (G_OBJECT (main_window), "destroy",
		      G_CALLBACK (_gftp_force_close), NULL);
  gtk_window_set_title (main_window, gftp_version);
  gtk_window_set_role (main_window, "main");
  gtk_widget_set_name (GTK_WIDGET(main_window), gftp_version);
#if GTK_MAJOR_VERSION==2
  gtk_window_set_policy (main_window, TRUE, TRUE, FALSE);
#endif
  gtk_widget_realize (GTK_WIDGET(main_window));

  set_window_icon(main_window, NULL);

  other_wdata = &window1;
  current_wdata = &window2;
  ui = CreateFTPWindows (GTK_WIDGET(main_window));
  gtk_container_add (GTK_CONTAINER (main_window), ui);
  gtk_widget_show (GTK_WIDGET(main_window));

  gftpui_common_about (ftp_log, NULL);

  g_timeout_add (1000, update_downloads, NULL);

  _setup_window1 ();
  _setup_window2 (argc, argv);

  gftp_gtk_platform_specific_init();

  gtk_main ();
  GDK_THREADS_LEAVE ();

  return (0);
}


void
gftpui_show_or_hide_command (void)
{
  intptr_t cmd_in_gui;

  gftp_lookup_global_option ("cmd_in_gui", &cmd_in_gui);
  if (cmd_in_gui)
    gtk_widget_show (gftpui_command_toolbar);
  else
    gtk_widget_hide (gftpui_command_toolbar);
}

