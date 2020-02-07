/*****************************************************************************/
/*  gftp-gtk.c - GTK+ port of gftp                                           */
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


static GtkWidget * local_frame, * remote_frame, * log_table, * transfer_scroll,
                 * gftpui_command_toolbar;

GtkWindow *main_window;
gftp_window_data window1, window2, *other_wdata, *current_wdata;
GtkWidget * stop_btn, * hostedit, * useredit, * passedit, * portedit, * logwdw,
          * dlwdw, * toolbar_combo_protocol, * gftpui_command_widget, * download_left_arrow,
          * upload_right_arrow, * openurl_btn;
GtkAdjustment * logwdw_vadj;
GtkTextMark * logwdw_textmark;
int local_start, remote_start, trans_start;
GHashTable * graphic_hash_table = NULL;

GtkActionGroup * menus = NULL;
GtkUIManager * factory = NULL;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t main_thread_id;
GList * viewedit_processes = NULL;

gboolean on_key_press_combo_toolbar(GtkWidget *widget, GdkEventKey *event, gpointer data);
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
  intptr_t remember_last_directory;
  const char *tempstr;
  intptr_t ret;

  ret = GTK_WIDGET (local_frame)->allocation.width;
  gftp_set_global_option ("listbox_local_width", GINT_TO_POINTER (ret));
  ret = GTK_WIDGET (remote_frame)->allocation.width;
  gftp_set_global_option ("listbox_remote_width", GINT_TO_POINTER (ret));
  ret = GTK_WIDGET (remote_frame)->allocation.height;
  gftp_set_global_option ("listbox_file_height", GINT_TO_POINTER (ret));
  ret = GTK_WIDGET (log_table)->allocation.height;
  gftp_set_global_option ("log_height", GINT_TO_POINTER (ret));
  ret = GTK_WIDGET (transfer_scroll)->allocation.height;
  gftp_set_global_option ("transfer_height", GINT_TO_POINTER (ret));

  ret = get_column (&GTK_CLIST (dlwdw)->column[0]);
  gftp_set_global_option ("file_trans_column", GINT_TO_POINTER (ret));

  ret = get_column (&GTK_CLIST (window1.listbox)->column[1]);
  gftp_set_global_option ("local_file_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window1.listbox)->column[2]);
  gftp_set_global_option ("local_size_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window1.listbox)->column[3]);
  gftp_set_global_option ("local_date_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window1.listbox)->column[4]);
  gftp_set_global_option ("local_user_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window1.listbox)->column[5]);
  gftp_set_global_option ("local_group_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window1.listbox)->column[6]);
  gftp_set_global_option ("local_attribs_width", GINT_TO_POINTER (ret));

  ret = get_column (&GTK_CLIST (window2.listbox)->column[1]);
  gftp_set_global_option ("remote_file_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window2.listbox)->column[2]);
  gftp_set_global_option ("remote_size_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window2.listbox)->column[3]);
  gftp_set_global_option ("remote_date_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window2.listbox)->column[4]);
  gftp_set_global_option ("remote_user_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window2.listbox)->column[5]);
  gftp_set_global_option ("remote_group_width", GINT_TO_POINTER (ret));
  ret = get_column (&GTK_CLIST (window2.listbox)->column[6]);
  gftp_set_global_option ("remote_attribs_width", GINT_TO_POINTER (ret));

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
      tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(window1.combo));
      gftp_set_global_option ("local_startup_directory", tempstr);

      tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(window2.combo));
      gftp_set_global_option ("remote_startup_directory", tempstr);
    }

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(toolbar_combo_protocol));
  gftp_set_global_option ("default_protocol", tempstr);

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
      MakeYesNoDialog (_("Exit"), _("There are file transfers in progress.\nAre you sure you want to exit?"), _gftp_exit, NULL, NULL, NULL);
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

static void
change_setting (GtkAction *action, GtkRadioAction *current)
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
  const char *tempstr;
  char *buf;

  tempstr = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (tempstr != NULL && *tempstr != '\0')
    {
      buf = g_strdup (tempstr);
      destroy_dialog (ddata);
      gftpui_common_cmd_open (wdata, wdata->request, NULL, NULL, buf);
      g_free (buf);
    }
}


static void
openurl_dialog (gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  MakeEditDialog (_("Open Location"), _("Enter a URL to connect to"),
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


static void
navi_up_directory(gftp_window_data * wdata)
{
  char *directory;
  if(gtk_widget_is_focus(window1.listbox))
  {
      wdata=&window1;
  }
  else
  {
      if (!GFTP_IS_CONNECTED (window2.request))
        return;
      wdata=&window2;
  }
  directory = gftp_build_path (wdata->request, wdata->request->directory,
                               "..", NULL);
  gftpui_run_chdir (wdata, directory);
  g_free (directory);
}

// ===
static void
on_local_openurl_dialog(void) {
  openurl_dialog(&window1);
}
static void
on_remote_openurl_dialog(void) {
  openurl_dialog(&window2);
}

static void
on_local_gftpui_disconnect(void) {
  gftpui_disconnect(&window1);
}
static void
on_remote_gftpui_disconnect(void) {
  gftpui_disconnect(&window2);
}

static void
on_local_change_filespec(void) {
  change_filespec(&window1);
}
static void
on_remote_change_filespec(void) {
  change_filespec(&window2);
}

static void
on_local_show_selected(void) {
  show_selected(&window1);
}
static void
on_remote_show_selected(void) {
  show_selected(&window2);
}

static void
on_local_navi_up_directory(void) {
  navi_up_directory(&window1);
}
static void
on_remote_navi_up_directory(void) {
  navi_up_directory(&window2);
}

static void
on_local_selectall(void) {
  selectall(&window1);
}
static void
on_remote_selectall(void) {
  selectall(&window2);
}

static void
on_local_selectallfiles(void) {
  selectallfiles(&window1);
}
static void
on_remote_selectallfiles(void) {
  selectallfiles(&window2);
}

static void
on_local_deselectall(void) {
  deselectall(&window1);
}
static void
on_remote_deselectall(void) {
  deselectall(&window2);
}

static void
on_local_save_directory_listing(void) {
  save_directory_listing(&window1);
}
static void
on_remote_save_directory_listing(void) {
  save_directory_listing(&window2);
}

static void
on_local_gftpui_site_dialog(void) {
  gftpui_site_dialog(&window1);
}
static void
on_remote_gftpui_site_dialog(void) {
  gftpui_site_dialog(&window2);
}

static void
on_local_gftpui_chdir_dialog(void) {
  gftpui_chdir_dialog(&window1);
}
static void
on_remote_gftpui_chdir_dialog(void) {
  gftpui_chdir_dialog(&window2);
}

static void
on_local_chmod_dialog(void) {
  chmod_dialog(&window1);
}
static void
on_remote_chmod_dialog(void) {
  chmod_dialog(&window2);
}

static void
on_local_gftpui_mkdir_dialog(void) {
  gftpui_mkdir_dialog(&window1);
}
static void
on_remote_gftpui_mkdir_dialog(void) {
  gftpui_mkdir_dialog(&window2);
}

static void
on_local_gftpui_rename_dialog(void) {
  gftpui_rename_dialog(&window1);
}
static void
on_remote_gftpui_rename_dialog(void) {
  gftpui_rename_dialog(&window2);
}

static void
on_local_delete_dialog(void) {
  delete_dialog(&window1);
}
static void
on_remote_delete_dialog(void) {
  delete_dialog(&window2);
}

static void
on_local_edit_dialog(void) {
  edit_dialog(&window1);
}
static void
on_remote_edit_dialog(void) {
  edit_dialog(&window2);
}

static void
on_local_view_dialog(void) {
  view_dialog(&window1);
}
static void
on_remote_view_dialog(void) {
  view_dialog(&window2);
}

static void
on_local_gftp_gtk_refresh(void) {
  gftp_gtk_refresh(&window1);
}
static void
on_remote_gftp_gtk_refresh(void) {
  gftp_gtk_refresh(&window2);
}
// ===

static GtkWidget *
CreateMenus (GtkWidget * parent)
{
  GtkAccelGroup * accel_group;
  intptr_t ascii_transfers;
  int default_ascii;

  static const GtkActionEntry menu_items[] =
  {
    //  name                    stock_id               "label"                  accel             tooltip  callback
    { "FTPMenu",              NULL,                  N_("_FTP"),              NULL,                NULL, NULL                   },
    { "FTPPreferences",       GTK_STOCK_PREFERENCES, N_("_Preferences..."),   NULL,                NULL, G_CALLBACK(options_dialog) },
    { "FTPQuit",              GTK_STOCK_QUIT,        N_("_Quit"),             "<control>Q",        NULL, G_CALLBACK(_gftp_menu_exit)  },

    { "LocalMenu",            NULL,                  N_("_Local"),            NULL,                NULL, NULL },
    { "LocalOpenLocation",    GTK_STOCK_OPEN,        N_("_Open Location..."), "<control><shift>O", NULL, G_CALLBACK(on_local_openurl_dialog) },
    { "LocalDisconnect",      GTK_STOCK_CLOSE,       N_("D_isconnect"),       "<control><shift>I", NULL, G_CALLBACK(on_local_gftpui_disconnect) },
    { "LocalChangeFilespec",  NULL,                  N_("Change _Filespec"),  "<control><shift>F", NULL, G_CALLBACK(on_local_change_filespec) },
    { "LocalShowSelected",    NULL,                  N_("_Show selected"),    NULL,                NULL, G_CALLBACK(on_local_show_selected) },
    { "LocalNavigateUp",      NULL,                  N_("Navigate _Up"),      "<alt>Up",           NULL, G_CALLBACK(on_local_navi_up_directory) },
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
    { "LocalRefresh",         GTK_STOCK_REFRESH,     N_("_Refresh"),          "<control><shift>R", NULL, G_CALLBACK(on_local_gftp_gtk_refresh) },

    { "RemoteMenu",           NULL,                  N_("_Remote"),            NULL,               NULL, NULL },
    { "RemoteOpenLocation",   GTK_STOCK_OPEN,        N_("_Open Location..."), "<control>O",        NULL, G_CALLBACK(on_remote_openurl_dialog) },
    { "RemoteDisconnect",     GTK_STOCK_CLOSE,       N_("D_isconnect"),       "<control>I",        NULL, G_CALLBACK(on_remote_gftpui_disconnect) },
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
    { "RemoteRefresh",        GTK_STOCK_REFRESH,     N_("_Refresh"),          "<control>R",        NULL, G_CALLBACK(on_remote_gftp_gtk_refresh) },

    { "BookmarksMenu",        NULL,                  N_("_Bookmarks"),        NULL,                NULL, NULL },
    { "BookmarksAddBookmark", GTK_STOCK_ADD,         N_("Add _Bookmark"),     "<control>B",        NULL, G_CALLBACK(add_bookmark) },
    { "BookmarksEditBookmarks",NULL,                 N_("Edit Bookmarks"),    NULL,                NULL, G_CALLBACK(edit_bookmarks) },

    { "TransferMenu",         NULL,                  N_("_Transfer"),         NULL,                NULL, NULL },
    { "TransferStart",        NULL,                  N_("_Start"),            NULL,                NULL, G_CALLBACK(start_transfer) },
    { "TransferStop",         GTK_STOCK_STOP,        N_("St_op"),             NULL,                NULL, G_CALLBACK(stop_transfer) },
    { "TransferSkipCurrentFile",NULL,                N_("Skip _Current File"),NULL,                NULL, G_CALLBACK(skip_transfer) },
    { "TransferRemoveFile",   GTK_STOCK_DELETE,      N_("_Remove File"),      NULL,                NULL, G_CALLBACK(remove_file_transfer) },
    { "TransferMoveFileUp",   GTK_STOCK_GO_UP,       N_("Move File _Up"),     NULL,                NULL, G_CALLBACK(move_transfer_up) },
    { "TransferMoveFileDown", GTK_STOCK_GO_DOWN,     N_("Move File _Down"),   NULL,                NULL, G_CALLBACK(move_transfer_down) },
    { "TransferRetrieveFiles",NULL,                  N_("_Retrieve Files"),   "<control>R",        NULL, G_CALLBACK(get_files) },
    { "TransferPutFiles",    NULL,                   N_("_Put Files"),        "<control>U",        NULL, G_CALLBACK(put_files) },

    { "LogMenu",             NULL,                   N_("L_og"),              NULL,                NULL, NULL },
    { "LogClear",            GTK_STOCK_CLEAR,        N_("_Clear"),            NULL,                NULL, G_CALLBACK(clearlog) },
    { "LogView",             NULL,                   N_("_View"),             NULL,                NULL, G_CALLBACK(viewlog) },
    { "LogSave",             GTK_STOCK_SAVE,         N_("_Save..."),          NULL,                NULL, G_CALLBACK(savelog) },

    { "ToolsMenu",           NULL,                   N_("Tool_s"),            NULL,                NULL, NULL },
    { "ToolsCompareWindows", NULL,                   N_("C_ompare Windows"),  NULL,                NULL, G_CALLBACK(compare_windows) },
    { "ToolsClearCache",     GTK_STOCK_CLEAR,        N_("_Clear Cache"),      NULL,                NULL, G_CALLBACK(clear_cache) },

    { "HelpMenu",            NULL,                   N_("_Help"),             NULL,                NULL, NULL },
    { "HelpAbout",           GTK_STOCK_ABOUT,        N_("_About"),            NULL,                NULL, G_CALLBACK(about_dialog) },
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
  
  gtk_action_group_add_actions(actions, menu_items, nmenu_items, NULL);

  current_wdata = &window2;
  gtk_action_group_add_radio_actions(actions, menu_radio_window,
                               G_N_ELEMENTS (menu_radio_window),
                               GFTP_MENU_ITEM_WIN2,
                               G_CALLBACK(change_setting),
                               0);
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
  GList *glist;
  GtkWidget *combo_entry;
  char *default_protocol, *tempstr;
  int i, j, num;

  toolbar = gtk_handle_box_new ();

  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (toolbar), box);
  gtk_container_border_width (GTK_CONTAINER (box), 5);

  //tempwid = gtk_image_new_from_icon_name ("gtk-network", GTK_ICON_SIZE_SMALL_TOOLBAR);
  tempwid = gtk_image_new_from_stock (GTK_STOCK_NETWORK, GTK_ICON_SIZE_SMALL_TOOLBAR);

  openurl_btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (openurl_btn), tempwid);
  gtk_signal_connect_object (GTK_OBJECT (openurl_btn), "clicked",
			     GTK_SIGNAL_FUNC (tb_openurl_dialog), NULL);
  gtk_signal_connect (GTK_OBJECT (openurl_btn), "drag_data_received",
		      GTK_SIGNAL_FUNC (openurl_get_drag_data), NULL);
  gtk_drag_dest_set (openurl_btn, GTK_DEST_DEFAULT_ALL, possible_types, 2,
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_container_border_width (GTK_CONTAINER (openurl_btn), 1);
  gtk_box_pack_start (GTK_BOX (box), openurl_btn, FALSE, FALSE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_Host:"));

  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  hostedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (hostedit, 120, -1);

  gftp_lookup_global_option ("hosthistory", &tmplistvar);
  glist = tmplistvar->list;
  gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(hostedit) )) );
  while (glist) {
    if (glist->data) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hostedit), glist->data);
    }
    glist = glist->next;
  }

  gftp_lookup_global_option ("host_value", &tempstr);
  combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(hostedit)));
  g_signal_connect (GTK_WIDGET(combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (GTK_WIDGET(combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), hostedit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Port:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  portedit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (portedit, 80, -1);

  gftp_lookup_global_option ("porthistory", &tmplistvar);
  gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(portedit) )) );
  glist = tmplistvar->list;
  while (glist) {
    if (glist->data) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(portedit), glist->data);
    }
    glist = glist->next;
  }

  gftp_lookup_global_option ("port_value", &tempstr);

  combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(portedit)));
  g_signal_connect (GTK_WIDGET(combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (GTK_WIDGET(combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), portedit, FALSE, FALSE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_User:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  useredit = gtk_combo_box_text_new_with_entry ();
  gtk_widget_set_size_request (useredit, 75, -1);

  gftp_lookup_global_option ("userhistory", &tmplistvar);
  gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(useredit) )) );
  glist = tmplistvar->list;
  while (glist) {
    if (glist->data) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(useredit), glist->data);
    }
    glist = glist->next;
  }

  gftp_lookup_global_option ("user_value", &tempstr);

  combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(useredit)));
  g_signal_connect (G_OBJECT(combo_entry), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (G_OBJECT(combo_entry), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);

  gtk_entry_set_text (GTK_ENTRY (combo_entry), tempstr);
  gtk_box_pack_start (GTK_BOX (box), useredit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Pass:"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  passedit = gtk_entry_new ();
  gtk_widget_set_size_request (passedit, 120, -1);

  gtk_entry_set_visibility (GTK_ENTRY (passedit), FALSE);
  g_signal_connect (GTK_WIDGET (passedit), "key_press_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  g_signal_connect (GTK_WIDGET (passedit), "key_release_event",
                   G_CALLBACK (on_key_press_combo_toolbar), NULL);
  gtk_box_pack_start (GTK_BOX (box), passedit, FALSE, FALSE, 0);

  tempwid = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  toolbar_combo_protocol = gtk_combo_box_text_new ();
  gtk_box_pack_start (GTK_BOX (tempwid), toolbar_combo_protocol, TRUE, FALSE, 0);

  num = 0;
  j = 0;
  gftp_lookup_global_option ("default_protocol", &default_protocol);
  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (!gftp_protocols[i].shown)
        continue;
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (toolbar_combo_protocol),
                                 gftp_protocols[i].name);
      if (default_protocol && strcmp (gftp_protocols[i].name, default_protocol) == 0)
        num = j;
      j++;
    }
  gtk_combo_box_set_active(GTK_COMBO_BOX(toolbar_combo_protocol), num);

  //tempwid = gtk_image_new_from_icon_name ("gtk-stop", GTK_ICON_SIZE_SMALL_TOOLBAR);
  tempwid = gtk_image_new_from_stock (GTK_STOCK_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR);

  stop_btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (stop_btn), tempwid);
  gtk_widget_set_sensitive (stop_btn, 0);
  gtk_signal_connect_object (GTK_OBJECT (stop_btn), "clicked",
			     GTK_SIGNAL_FUNC (stop_button), NULL);
  gtk_container_border_width (GTK_CONTAINER (stop_btn), 1);
  gtk_box_pack_start (GTK_BOX (box), stop_btn, FALSE, FALSE, 0);

  //gtk_widget_grab_focus (GTK_WIDGET (hostedit));

  return (toolbar);
}


static GtkWidget *
CreateCommandToolbar (void)
{
  GtkWidget * box, * tempwid;

  gftpui_command_toolbar = gtk_handle_box_new ();

  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (gftpui_command_toolbar), box);
  gtk_container_border_width (GTK_CONTAINER (box), 5);

  tempwid = gtk_label_new (_("Command: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  gftpui_command_widget = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), gftpui_command_widget, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (gftpui_command_widget), "activate",
		      GTK_SIGNAL_FUNC (gftpui_run_command), NULL);

  return (gftpui_command_toolbar);
}


static void
setup_column (GtkWidget * listbox, int column, int width)
{
  if (width == 0)
    gtk_clist_set_column_auto_resize (GTK_CLIST (listbox), column, TRUE);
  else if (width == -1)
    gtk_clist_set_column_visibility (GTK_CLIST (listbox), column, FALSE);
  else
    gtk_clist_set_column_width (GTK_CLIST (listbox), column, width);
}


static void
list_doaction (gftp_window_data * wdata)
{
  intptr_t list_dblclk_action;
  GList *templist, *filelist;
  gftp_file *tempfle;
  int num, success;
  char *directory;

  gftp_lookup_request_option (wdata->request, "list_dblclk_action", 
                              &list_dblclk_action);

  filelist = wdata->files;
  templist = gftp_gtk_get_list_selection (wdata);
  num = 0;
  templist = get_next_selection (templist, &filelist, &num);
  tempfle = (gftp_file *) filelist->data;

  if (check_reconnect (wdata) < 0) 
    return;

  success = 0;
  if (S_ISLNK (tempfle->st_mode) || S_ISDIR (tempfle->st_mode))
    {
      if(tempfle->st_mode & S_IXUSR)
      {
          directory = gftp_build_path (wdata->request, wdata->request->directory,
                                   tempfle->file, NULL);
          success = gftpui_run_chdir (wdata, directory);
          g_free (directory);
      }else{
          ftp_log (gftp_logging_error, NULL,
               _("Directory %s is not listable\n"),
               tempfle->file);
      }
    }

  if (tempfle && !S_ISDIR (tempfle->st_mode) && !success)
    {
      switch (list_dblclk_action)
        {
          case 0:
            view_dialog (wdata);
            break;
          case 1:
            edit_dialog (wdata);
            break;
          case 2:
            if (wdata == &window2)
              get_files (wdata);
            else
              put_files (wdata);
            break;
        }
    }
}


static gint
list_enter (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (!GFTP_IS_CONNECTED (wdata->request))
    return (TRUE);

  if (event->type == GDK_KEY_PRESS && 
           (event->keyval == GDK_KP_Delete || event->keyval == GDK_Delete))
    {
      delete_dialog (wdata);
      return (FALSE);
    }
  else if (IS_ONE_SELECTED (wdata) && event->type == GDK_KEY_PRESS && 
      event->keyval == GDK_Return)
    {
      list_doaction (wdata);
      return (FALSE);
    }
  return (TRUE);
}


static gint
list_dblclick (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  gftp_window_data * wdata = data;

  if (event->button == 3) {
    if (strcmp (gftp_protocols[wdata->request->protonum].name, "Local") == 0) {
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
select_row_callback (GtkWidget *widget, gint row, gint column,
                     GdkEventButton *event, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;

  if (event != NULL && event->type == GDK_2BUTTON_PRESS && event->button == 1 &&
      GFTP_IS_CONNECTED (wdata->request) && IS_ONE_SELECTED (wdata))
    list_doaction (wdata);
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
  char *titles[7], tempstr[50], *startup_directory;
  GtkWidget *box, *scroll_list, *parent;
  intptr_t listbox_file_height, colwidth;
  GtkWidget *combo_entry;
  GList *glist;

  titles[0] = "";
  titles[1] = _("Filename");
  titles[2] = _("Size");
  titles[3] = _("Date");
  titles[4] = _("User");
  titles[5] = _("Group");
  titles[6] = _("Attribs");

  wdata->request = gftp_request_new ();
  gftp_gtk_init_request (wdata);

  parent = gtk_frame_new (NULL);
  
  gftp_lookup_global_option ("listbox_file_height", &listbox_file_height);
  g_snprintf (tempstr, sizeof (tempstr), "listbox_%s_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  gtk_widget_set_size_request (parent, colwidth, listbox_file_height);

  gtk_container_border_width (GTK_CONTAINER (parent), 5);

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (parent), box);

  wdata->combo = gtk_combo_box_text_new_with_entry ();
  gtk_box_pack_start (GTK_BOX (box), wdata->combo, FALSE, FALSE, 0);

  combo_entry = GTK_WIDGET(gtk_bin_get_child(GTK_BIN(wdata->combo)));
  g_signal_connect (GTK_WIDGET(combo_entry), "key_press_event",
                   G_CALLBACK (chdir_edit), NULL);
  g_signal_connect (GTK_WIDGET(combo_entry), "key_release_event",
                   G_CALLBACK (chdir_edit), wdata);

  gtk_list_store_clear( GTK_LIST_STORE(gtk_combo_box_get_model( GTK_COMBO_BOX(wdata->combo) )) );
  glist = *wdata->history;
  while (glist) {
    if (glist->data) {
      gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(wdata->combo), glist->data);
    }
    glist = glist->next;
  }

  g_snprintf (tempstr, sizeof (tempstr), "%s_startup_directory",
              wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &startup_directory);
  gtk_entry_set_text (GTK_ENTRY (combo_entry), startup_directory);

  wdata->hoststxt = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (wdata->hoststxt), 0, 0);
  gtk_box_pack_start (GTK_BOX (box), wdata->hoststxt, FALSE, FALSE, 0);

  scroll_list = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll_list),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  wdata->listbox = gtk_clist_new_with_titles (7, titles);
  gtk_container_add (GTK_CONTAINER (scroll_list), wdata->listbox);
  gtk_drag_source_set (wdata->listbox, GDK_BUTTON1_MASK, possible_types, 3,
		       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drag_dest_set (wdata->listbox, GTK_DEST_DEFAULT_ALL, possible_types, 2,
		     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gtk_clist_set_selection_mode (GTK_CLIST (wdata->listbox),
				GTK_SELECTION_EXTENDED);

  gtk_clist_set_column_width (GTK_CLIST (wdata->listbox), 0, 16);
  gtk_clist_set_column_justification (GTK_CLIST (wdata->listbox), 0,
				      GTK_JUSTIFY_CENTER);

  g_snprintf (tempstr, sizeof (tempstr), "%s_file_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 1, colwidth);

  gtk_clist_set_column_justification (GTK_CLIST (wdata->listbox), 2,
				      GTK_JUSTIFY_RIGHT);

  g_snprintf (tempstr, sizeof (tempstr), "%s_size_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 2, colwidth);

  g_snprintf (tempstr, sizeof (tempstr), "%s_date_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 3, colwidth);

  g_snprintf (tempstr, sizeof (tempstr), "%s_user_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 4, colwidth);

  g_snprintf (tempstr, sizeof (tempstr), "%s_group_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 5, colwidth);

  g_snprintf (tempstr, sizeof (tempstr), "%s_attribs_width", wdata->prefix_col_str);
  gftp_lookup_global_option (tempstr, &colwidth);
  setup_column (wdata->listbox, 6, colwidth);

  gtk_box_pack_start (GTK_BOX (box), scroll_list, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (wdata->listbox), "click_column",
	 	      GTK_SIGNAL_FUNC (sortrows), (gpointer) wdata);
  gtk_signal_connect (GTK_OBJECT (wdata->listbox), "drag_data_get",
		      GTK_SIGNAL_FUNC (listbox_drag), (gpointer) wdata);
  gtk_signal_connect (GTK_OBJECT (wdata->listbox), "drag_data_received",
		      GTK_SIGNAL_FUNC (listbox_get_drag_data),
		      (gpointer) wdata);
  gtk_signal_connect_after (GTK_OBJECT (wdata->listbox), "key_press_event",
                            GTK_SIGNAL_FUNC (list_enter), (gpointer) wdata);
  gtk_signal_connect (GTK_OBJECT (wdata->listbox), "select_row",
                      GTK_SIGNAL_FUNC(select_row_callback),
                      (gpointer) wdata);
  gtk_signal_connect_after (GTK_OBJECT (wdata->listbox), "button_press_event",
                            GTK_SIGNAL_FUNC (list_dblclick), (gpointer) wdata);
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
 
  mainvbox = gtk_vbox_new (FALSE, 0);

  tempwid = CreateMenus (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateConnectToolbar (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateCommandToolbar ();
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  winpane = gtk_hpaned_new ();

  box = gtk_hbox_new (FALSE, 0);

  window1.prefix_col_str = "local";
  local_frame = CreateFTPWindow (&window1);
  gtk_box_pack_start (GTK_BOX (box), local_frame, TRUE, TRUE, 0);

  dlbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (dlbox), 5);
  gtk_box_pack_start (GTK_BOX (box), dlbox, FALSE, FALSE, 0);

  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);

  upload_right_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), upload_right_arrow, TRUE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (upload_right_arrow), "clicked",
			     GTK_SIGNAL_FUNC (put_files), NULL);
  gtk_container_add (GTK_CONTAINER (upload_right_arrow), tempwid);

  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);

  download_left_arrow = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), download_left_arrow, TRUE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (download_left_arrow), "clicked",
			     GTK_SIGNAL_FUNC (get_files), NULL);
  gtk_container_add (GTK_CONTAINER (download_left_arrow), tempwid);

  gtk_paned_pack1 (GTK_PANED (winpane), box, 1, 1);

  window2.prefix_col_str = "remote";
  remote_frame = CreateFTPWindow (&window2);

  gtk_paned_pack2 (GTK_PANED (winpane), remote_frame, 1, 1);

  dlpane = gtk_vpaned_new ();
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
  g_signal_connect (GTK_WIDGET (dlwdw), "button_press_event",
        G_CALLBACK (on_key_press_transfer), NULL);
  gtk_paned_pack2 (GTK_PANED (dlpane), transfer_scroll, 1, 1);

  logpane = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (logpane), dlpane, 1, 1);

  log_table = gtk_table_new (1, 2, FALSE);
  gftp_lookup_global_option ("log_height", &tmplookup);
  gtk_widget_set_size_request (log_table, -1, tmplookup);

  logwdw = gtk_text_view_new ();
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

  tempwid = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tempwid),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (tempwid), logwdw);
  gtk_table_attach (GTK_TABLE (log_table), tempwid, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
		    0, 0);
  logwdw_vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (tempwid));
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  logwdw_textmark = gtk_text_buffer_create_mark (textbuf, "end", &iter, 1);

  gtk_paned_pack2 (GTK_PANED (logpane), log_table, 1, 1);
  gtk_box_pack_start (GTK_BOX (mainvbox), logpane, TRUE, TRUE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (transfer_scroll), 5);
  gtk_container_set_border_width (GTK_CONTAINER (tempwid), 5);

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

gboolean
on_key_press_combo_toolbar(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->type == GDK_KEY_PRESS) {
    if (event->keyval == GDK_KEY_Return)
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

  if (event->keyval != GDK_KEY_Return) {
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
      ftp_connect (current_wdata, current_wdata->request);
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

  gftp_set_directory (current_wdata->request, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(current_wdata->combo)) );
  if (current_wdata->request->directory != NULL)
    alltrim (current_wdata->request->directory);

  add_history (current_wdata->combo, current_wdata->history, 
               current_wdata->histlen, current_wdata->request->directory);

  ftp_connect (current_wdata, current_wdata->request);
}


void
sortrows (GtkCList * clist, gint column, gpointer data)
{
  char sortcol_name[25], sortasds_name[25];
  intptr_t sortcol, sortasds;
  gftp_window_data * wdata;
  GtkWidget * sort_wid;
  GList * templist;
  int swap_col;

  wdata = data;
  g_snprintf (sortcol_name, sizeof (sortcol_name), "%s_sortcol",
              wdata->prefix_col_str);
  gftp_lookup_global_option (sortcol_name, &sortcol);
  g_snprintf (sortasds_name, sizeof (sortasds_name), "%s_sortasds",
              wdata->prefix_col_str);
  gftp_lookup_global_option (sortasds_name, &sortasds);

  if (column == -1)
    column = sortcol;

  if (column == 0 || (column == sortcol && wdata->sorted))
    {
      sortasds = !sortasds;
      gftp_set_global_option (sortasds_name, GINT_TO_POINTER (sortasds));
      swap_col = 1;
    }
  else
    swap_col = 0;

  if (swap_col || !wdata->sorted)
    {
      sort_wid = gtk_clist_get_column_widget (clist, 0);
      gtk_widget_destroy (sort_wid);

      if (sortasds)
        sort_wid = gtk_image_new_from_icon_name ("view-sort-ascending",
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);
      else
        sort_wid = gtk_image_new_from_icon_name ("view-sort-descending",
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);

      gtk_clist_set_column_widget (clist, 0, sort_wid);
    }
  else
    {
      sortcol = column;
      gftp_set_global_option (sortcol_name, GINT_TO_POINTER (sortcol));
    }

  if (!GFTP_IS_CONNECTED (wdata->request))
    return;

  gtk_clist_freeze (clist);
  gtk_clist_clear (clist);

  wdata->files = gftp_sort_filelist (wdata->files, sortcol, sortasds);

  templist = wdata->files; 
  while (templist != NULL)
    {
      add_file_listbox (wdata, templist->data);
      templist = templist->next;
    }

  wdata->sorted = 1;
  gtk_clist_thaw (clist);
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
  if (gftp_protocols[GFTP_LOCAL_NUM].init (window1.request) == 0)
    {
      gftp_setup_startup_directory (window1.request,
                                    "local_startup_directory");
      gftp_connect (window1.request);
      ftp_list_files (&window1);
    }
}


static void
_setup_window2 (int argc, char **argv)
{
  intptr_t connect_to_remote_on_startup;

  gftp_lookup_request_option (window2.request, "connect_to_remote_on_startup",
                              &connect_to_remote_on_startup);

  if (argc == 2 && strncmp (argv[1], "--", 2) != 0)
    {
      if (gftp_parse_url (window2.request, argv[1]) == 0)
        ftp_connect (&window2, window2.request);
      else
        gftp_usage ();
    }
  else if (connect_to_remote_on_startup)
    {
      if (gftp_protocols[_get_selected_protocol ()].init (current_wdata->request) == 0)
        {
          gftp_setup_startup_directory (window2.request,
                                        "remote_startup_directory");
          gftp_connect (window2.request);
          ftp_list_files (&window2);
        }
    }
  else
    {
      /* On the remote window, even though we aren't connected, draw the sort
         icon on that side */
      sortrows (GTK_CLIST (window2.listbox), -1, &window2);
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

#if !GLIB_CHECK_VERSION(2,31,0)
  g_thread_init (NULL);
#endif

  gdk_threads_init();
  GDK_THREADS_ENTER ();
  main_thread_id = pthread_self ();
  gtk_set_locale ();
  gtk_init (&argc, &argv);

  graphic_hash_table = g_hash_table_new (string_hash_function,
                                         string_hash_compare);

  main_window = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
		      GTK_SIGNAL_FUNC (_gftp_try_close), NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (_gftp_force_close), NULL);
  gtk_window_set_title (main_window, gftp_version);
  gtk_window_set_wmclass (main_window, "main", "gFTP");
  gtk_widget_set_name (GTK_WIDGET(main_window), gftp_version);
  gtk_window_set_policy (main_window, TRUE, TRUE, FALSE);
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

