/*****************************************************************************/
/*  gftp-gtk.c - GTK+ 1.2 port of gftp                                       */
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

static gint delete_event 			( GtkWidget * widget, 
						  GdkEvent * event, 
						  gpointer data );
static void destroy 				( GtkWidget * widget, 
						  gpointer data );
static RETSIGTYPE sig_child 			( int signo );
static GtkWidget * CreateFTPWindows 		( GtkWidget * ui );
static GtkWidget * CreateMenus			( GtkWidget * parent );
static GtkWidget * CreateToolbar 		( GtkWidget * parent );
static void doexit 				( GtkWidget * widget, 
						  gpointer data );
static int get_column 				( GtkCListColumn * col );
static void init_gftp 				( int argc, 
						  char *argv[], 
						  GtkWidget * parent );
static void menu_exit 				( GtkWidget * widget, 
						  gpointer data );
static GtkWidget * CreateFTPWindow 		( gftp_window_data * wdata,
						  int width,
						  int columns[6] );
static void setup_column 			( GtkWidget * listbox, 
						  int column, 
						  int width );
static gint menu_mouse_click 			( GtkWidget * widget, 
						  GdkEventButton * event, 
						  gpointer data );
static gint list_dblclick 			( GtkWidget * widget, 
						  GdkEventButton * event, 
						  gpointer data );
static void list_doaction 			( gftp_window_data * wdata );
static gint list_enter 				( GtkWidget * widget, 
						  GdkEventKey * event, 
						  gpointer data );
static void chfunc				( gpointer data );

static GtkItemFactory *log_factory, *dl_factory;
static GtkWidget * local_frame, * remote_frame, * log_table, * transfer_scroll,
                 * openurl_btn;

gftp_window_data window1, window2, *other_wdata, *current_wdata;
GtkWidget * stop_btn, * hostedit, * useredit, * passedit, * portedit, * logwdw,
          * dlwdw, * protocol_menu, * optionmenu;
GtkAdjustment * logwdw_vadj;
#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0
GtkTextMark * logwdw_textmark;
#endif
int local_start, remote_start, trans_start, log_start, tools_start;
GHashTable * graphic_hash_table = NULL;
GtkItemFactoryEntry * menus = NULL;
GtkItemFactory * factory = NULL;
pthread_mutex_t transfer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
gftp_graphic * gftp_icon;

int
main (int argc, char **argv)
{
  GtkWidget *window, *ui;

#ifdef HAVE_GETTEXT
  setlocale (LC_ALL, "");
  bindtextdomain ("gftp", LOCALE_DIR);
#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION == 0
  bind_textdomain_codeset ("gftp", "UTF-8");
#endif
  textdomain ("gftp");
#endif

  g_thread_init (NULL);
  gtk_set_locale ();
  gtk_init (&argc, &argv);
  signal (SIGCHLD, sig_child);
  signal (SIGPIPE, SIG_IGN);
  signal (SIGALRM, SIG_IGN);
  graphic_hash_table = g_hash_table_new (string_hash_function, string_hash_compare);
  gftp_read_config_file (argv, 1);
  if (gftp_parse_command_line (&argc, &argv) != 0)
    exit (0);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (delete_event), NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (destroy), NULL);
  gtk_window_set_title (GTK_WINDOW (window), version);
  gtk_window_set_wmclass (GTK_WINDOW(window), "main", "gFTP");
  gtk_widget_set_name (window, version);
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, FALSE);
  gtk_widget_realize (window);

  gftp_icon = open_xpm (window, "gftp.xpm");
  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (window->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (window->window, _("gFTP Icon"));
    }

  other_wdata = &window1;
  current_wdata = &window2;
  ui = CreateFTPWindows (window);
  gtk_container_add (GTK_CONTAINER (window), ui);
  gtk_widget_show (window);

  ftp_log (gftp_logging_misc, NULL,
	   "%s, Copyright (C) 1998-2002 Brian Masney <", version);
  ftp_log (gftp_logging_recv, NULL, "masneyb@gftp.org");
  ftp_log (gftp_logging_misc, NULL,
	   _(">. If you have any questions, comments, or suggestions about this program, please feel free to email them to me. You can always find out the latest news about gFTP from my website at http://www.gftp.org/\n"));
  ftp_log (gftp_logging_misc, NULL,
	   _("gFTP comes with ABSOLUTELY NO WARRANTY; for details, see the COPYING file. This is free software, and you are welcome to redistribute it under certain conditions; for details, see the COPYING file\n"));

  init_gftp (argc, argv, window);
  gtk_timeout_add (1000, update_downloads, NULL);
  gftp_protocols[GFTP_LOCAL_NUM].init (window1.request);
  if (startup_directory != NULL && *startup_directory != '\0')
    gftp_set_directory (window1.request, startup_directory);
  gftp_connect (window1.request);
  ftp_list_files (&window1, 0);

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();
  return (0);
}


static gint
delete_event (GtkWidget * widget, GdkEvent * event, gpointer data)
{
  if (file_transfers == NULL)
    doexit (NULL, NULL);
  else
    {
      MakeYesNoDialog (_("Exit"), _("There are file transfers in progress.\nAre you sure you want to exit?"), doexit, NULL, NULL, NULL);
      return (TRUE);
    }
  return (FALSE);
}


static void
destroy (GtkWidget * widget, gpointer data)
{
  exit (0);
}


static RETSIGTYPE
sig_child (int signo)
{
  viewedit_process_done = 1;
}


static GtkWidget *
CreateFTPWindows (GtkWidget * ui)
{
  GtkWidget *box, *dlbox, *winpane, *dlpane, *logpane, *mainvbox, *tempwid,
            *button;
  char *dltitles[2];
#if !(GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2)
  GtkTextBuffer * textbuf;
  GtkTextIter iter;
  GtkTextTag *tag;
  GdkColor fore;
#endif

  memset (&window1, 0, sizeof (window1));
  memset (&window2, 0, sizeof (window2));
  window1.history = &localhistory;
  window1.histlen = &localhistlen;
  window2.history = &remotehistory;
  window2.histlen = &remotehistlen;
 
  mainvbox = gtk_vbox_new (FALSE, 0);

  tempwid = CreateMenus (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  tempwid = CreateToolbar (ui);
  gtk_box_pack_start (GTK_BOX (mainvbox), tempwid, FALSE, FALSE, 0);

  winpane = gtk_hpaned_new ();

  box = gtk_hbox_new (FALSE, 0);

  local_frame = CreateFTPWindow (&window1, listbox_local_width, local_columns);
  gtk_box_pack_start (GTK_BOX (box), local_frame, TRUE, TRUE, 0);

  dlbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (dlbox), 5);
  gtk_box_pack_start (GTK_BOX (box), dlbox, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  tempwid = toolbar_pixmap (ui, "right.xpm");
#else
  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), button, TRUE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (put_files), NULL);
  gtk_container_add (GTK_CONTAINER (button), tempwid);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  tempwid = toolbar_pixmap (ui, "left.xpm");
#else
  tempwid = gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (dlbox), button, TRUE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC (get_files), NULL);
  gtk_container_add (GTK_CONTAINER (button), tempwid);

  gtk_paned_pack1 (GTK_PANED (winpane), box, 1, 1);

  remote_frame = CreateFTPWindow (&window2, listbox_remote_width, 
                                  remote_columns);
  gtk_paned_pack2 (GTK_PANED (winpane), remote_frame, 1, 1);

  dlpane = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (dlpane), winpane, 1, 1);

  transfer_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (transfer_scroll, -1, transfer_height);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (transfer_scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  dltitles[0] = _("Filename");
  dltitles[1] = _("Progress");
  dlwdw = gtk_ctree_new_with_titles (2, 0, dltitles);
  gtk_clist_set_selection_mode (GTK_CLIST (dlwdw), GTK_SELECTION_SINGLE);
  gtk_clist_set_reorderable (GTK_CLIST (dlwdw), 0);

  if (file_trans_column == 0)
    gtk_clist_set_column_auto_resize (GTK_CLIST (dlwdw), 0, TRUE);
  else
    gtk_clist_set_column_width (GTK_CLIST (dlwdw), 0, file_trans_column);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (transfer_scroll),                                          dlwdw);
  gtk_signal_connect (GTK_OBJECT (dlwdw), "button_press_event",
		      GTK_SIGNAL_FUNC (menu_mouse_click), (gpointer) dl_factory);
  gtk_paned_pack2 (GTK_PANED (dlpane), transfer_scroll, 1, 1);

  logpane = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (logpane), dlpane, 1, 1);

  log_table = gtk_table_new (1, 2, FALSE);
  gtk_widget_set_size_request (log_table, -1, log_height);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  logwdw = gtk_text_new (NULL, NULL);

  gtk_text_set_editable (GTK_TEXT (logwdw), FALSE);
  gtk_text_set_word_wrap (GTK_TEXT (logwdw), TRUE);

  gtk_table_attach (GTK_TABLE (log_table), logwdw, 0, 1, 0, 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND | GTK_SHRINK,
		    0, 0);
  gtk_signal_connect (GTK_OBJECT (logwdw), "button_press_event",
		      GTK_SIGNAL_FUNC (menu_mouse_click), 
                      (gpointer) log_factory);

  tempwid = gtk_vscrollbar_new (GTK_TEXT (logwdw)->vadj);
  gtk_table_attach (GTK_TABLE (log_table), tempwid, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
  logwdw_vadj = GTK_TEXT (logwdw)->vadj;
#else
  logwdw = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (logwdw), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (logwdw), GTK_WRAP_WORD);

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (logwdw));

  tag = gtk_text_buffer_create_tag (textbuf, "send", NULL);
  fore.red = send_color.red;
  fore.green = send_color.green;
  fore.blue = send_color.blue;
  g_object_set (G_OBJECT (tag), "foreground_gdk", &fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "recv", NULL);
  fore.red = recv_color.red;
  fore.green = recv_color.green;
  fore.blue = recv_color.blue;
  g_object_set (G_OBJECT (tag), "foreground_gdk", &fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "error", NULL);
  fore.red = error_color.red;
  fore.green = error_color.green;
  fore.blue = error_color.blue;
  g_object_set (G_OBJECT (tag), "foreground_gdk", &fore, NULL);

  tag = gtk_text_buffer_create_tag (textbuf, "misc", NULL);
  fore.red = misc_color.red;
  fore.green = misc_color.green;
  fore.blue = misc_color.blue;
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
#endif
  gtk_paned_pack2 (GTK_PANED (logpane), log_table, 1, 1);
  gtk_box_pack_start (GTK_BOX (mainvbox), logpane, TRUE, TRUE, 0);

  gtk_widget_show_all (mainvbox);
  return (mainvbox);
}


static GtkWidget *
CreateMenus (GtkWidget * parent)
{
  int local_len, remote_len, len, i, trans_len, log_len, tools_len;
  GtkAccelGroup *accel_group;
  GtkWidget * tempwid;
  static GtkItemFactoryEntry menu_items[] = {
    {N_("/_FTP"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/FTP/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/FTP/Window 1"), NULL, change_setting, 3, MN_("<RadioItem>")},
    {N_("/FTP/Window 2"), NULL, change_setting, 4, MN_("/FTP/Window 1")},
    {N_("/FTP/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/FTP/Ascii"), NULL, change_setting, 1, MN_("<RadioItem>")},
    {N_("/FTP/Binary"), NULL, change_setting, 2, MN_("/FTP/Ascii")},
    {N_("/FTP/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/FTP/_Options..."), "<control>O", options_dialog, 0,
	MS_(GTK_STOCK_PREFERENCES)},
    {N_("/FTP/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/FTP/_Quit"), "<control>Q", menu_exit, 0, MS_(GTK_STOCK_QUIT)},
    {N_("/_Local"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Local/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Local/Open _URL..."), NULL, openurl_dialog, 0, MS_(GTK_STOCK_OPEN)},
    {N_("/Local/Disconnect"), NULL, disconnect, 0, MS_(GTK_STOCK_CLOSE)},
    {N_("/Local/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Local/Change Filespec..."), NULL, change_filespec, 0, MN_(NULL)},
    {N_("/Local/Show selected"), NULL, show_selected, 0, MN_(NULL)},
    {N_("/Local/Select All"), NULL, selectall, 0, MN_(NULL)},
    {N_("/Local/Select All Files"), NULL, selectallfiles, 0, MN_(NULL)},
    {N_("/Local/Deselect All"), NULL, deselectall, 0, MN_(NULL)},
    {N_("/Local/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Local/Save Directory Listing..."), NULL, save_directory_listing, 0, MN_(NULL)},
    {N_("/Local/Send SITE Command..."), NULL, site_dialog, 0, MN_(NULL)},
    {N_("/Local/Change Directory"), NULL, chfunc, 0, MN_(NULL)},
    {N_("/Local/Chmod..."), NULL, chmod_dialog, 0, MN_(NULL)},
    {N_("/Local/Make Directory..."), NULL, mkdir_dialog, 0, MN_(NULL)},
    {N_("/Local/Rename..."), NULL, rename_dialog, 0, MN_(NULL)},
    {N_("/Local/Delete..."), NULL, delete_dialog, 0, MN_(NULL)},
    {N_("/Local/Edit..."), NULL, edit_dialog, 0, MN_(NULL)},
    {N_("/Local/View..."), NULL, view_dialog, 0, MN_(NULL)},
    {N_("/Local/Refresh"), NULL, refresh, 0, MS_(GTK_STOCK_REFRESH)},
    {N_("/_Remote"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Remote/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Remote/Open _URL..."), "<control>U", openurl_dialog, 0,
	MS_(GTK_STOCK_OPEN)},
    {N_("/Remote/Disconnect"), "<control>D", disconnect, 0,
	MS_(GTK_STOCK_CLOSE)},
    {N_("/Remote/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Remote/Change Filespec..."), NULL, change_filespec, 0, MN_(NULL)},
    {N_("/Remote/Show selected"), NULL, show_selected, 0, MN_(NULL)},
    {N_("/Remote/Select All"), NULL, selectall, 0, MN_(NULL)},
    {N_("/Remote/Select All Files"), NULL, selectallfiles, 0, MN_(NULL)},
    {N_("/Remote/Deselect All"), NULL, deselectall, 0, MN_(NULL)},
    {N_("/Remote/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Remote/Save Directory Listing..."), NULL, save_directory_listing, 0, MN_(NULL)},
    {N_("/Remote/Send SITE Command..."), NULL, site_dialog, 0, MN_(NULL)},
    {N_("/Remote/Change Directory"), NULL, chfunc, 0, MN_(NULL)},
    {N_("/Remote/Chmod..."), NULL, chmod_dialog, 0, MN_(NULL)},
    {N_("/Remote/Make Directory..."), NULL, mkdir_dialog, 0, MN_(NULL)},
    {N_("/Remote/Rename..."), NULL, rename_dialog, 0, MN_(NULL)},
    {N_("/Remote/Delete..."), NULL, delete_dialog, 0, MN_(NULL)},
    {N_("/Remote/Edit..."), NULL, edit_dialog, 0, MN_(NULL)},
    {N_("/Remote/View..."), NULL, view_dialog, 0, MN_(NULL)},
    {N_("/Remote/Refresh"), NULL, refresh, 0, MS_(GTK_STOCK_REFRESH)},
    {N_("/_Bookmarks"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Bookmarks/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Bookmarks/Add bookmark"), "<control>A", add_bookmark, 0,
	MS_(GTK_STOCK_ADD)},
    {N_("/Bookmarks/Edit bookmarks"), NULL, edit_bookmarks, 0, MN_(NULL)},
    {N_("/Bookmarks/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/_Transfers"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Transfers/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Transfers/Start Transfer"), NULL, start_transfer, 0, MN_(NULL)},
    {N_("/Transfers/Stop Transfer"), NULL, stop_transfer, 0,
	MS_(GTK_STOCK_STOP)},
    {N_("/Transfers/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Transfers/Skip Current File"), NULL, skip_transfer, 0, MN_(NULL)},
    {N_("/Transfers/Remove File"), NULL, remove_file_transfer, 0,
	MS_(GTK_STOCK_DELETE)},
    {N_("/Transfers/Move File _Up"), NULL, move_transfer_up, 0,
	MS_(GTK_STOCK_GO_UP)},
    {N_("/Transfers/Move File _Down"), NULL, move_transfer_down, 0,
	MS_(GTK_STOCK_GO_DOWN)},
    {N_("/Transfers/sep"), NULL, 0, 0, MN_("<Separator>")},
    {N_("/Transfers/Retrieve Files"), "<control>R", get_files, 0, MN_(NULL)},
    {N_("/Transfers/Put Files"), "<control>P", put_files, 0, MN_(NULL)},
    {N_("/L_ogging"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Logging/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Logging/Clear"), NULL, clearlog, 0, MS_(GTK_STOCK_CLEAR)},
    {N_("/Logging/View log..."), NULL, viewlog, 0, MN_(NULL)},
    {N_("/Logging/Save log..."), NULL, savelog, 0, MS_(GTK_STOCK_SAVE)},
    {N_("/Tool_s"), NULL, 0, 0, MN_("<Branch>")},
    {N_("/Tools/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Tools/Compare Windows"), NULL, compare_windows, 0, MN_(NULL)},
    {N_("/Tools/Clear Cache"), NULL, clear_cache, 0, MS_(GTK_STOCK_CLEAR)},
    {N_("/_Help"), NULL, 0, 0, MN_("<LastBranch>")},
    {N_("/Help/tearoff"), NULL, 0, 0, MN_("<Tearoff>")},
    {N_("/Help/About..."), NULL, about_dialog, 0, MS_(GTK_STOCK_HELP)}
  };

  menus = menu_items;

  accel_group = gtk_accel_group_new ();
  factory = item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group, NULL);

  i = 0;
  len = 11;
  /* FTP Menu */
  create_item_factory (factory, len, menu_items, &window2);

  i += len;
  /* Local Menu */
  local_start = i;
  local_len = 21;
  create_item_factory (factory, local_len, menu_items + i, &window1);

  i += local_len;
  /* Remote Menu */
  remote_start = i;
  remote_len = 21;
  create_item_factory (factory, remote_len, menu_items + i, &window2);

  i += remote_len;
  len = 5;
  /* Bookmarks Menu */
  create_item_factory (factory, len, menu_items + i, &window2);

  i += len;
  /* Transfers Menu */
  trans_start = i;
  trans_len = 12;
  create_item_factory (factory, trans_len, menu_items + i, NULL);

  i += trans_len;
  /* Logging Menu */
  log_start = i;
  log_len = 5;
  create_item_factory (factory, log_len, menu_items + i, NULL);

  i += log_len;
  /* Tools Menu */
  tools_start = i;
  tools_len = 4;
  create_item_factory (factory, tools_len, menu_items + i, NULL);

  i += tools_len;
  /* Help Menu */
  create_item_factory (factory, 3, menu_items + i, NULL);

  build_bookmarks_menu ();

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  gtk_accel_group_attach (accel_group, GTK_OBJECT (parent));
#else
  _gtk_accel_group_attach (accel_group, G_OBJECT (parent));
#endif

  tempwid = gtk_item_factory_get_widget (factory, menu_items[6].path);
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (tempwid), TRUE);

  tempwid = gtk_item_factory_get_widget (factory, menu_items[3].path);
  gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (tempwid), TRUE);

  window1.ifactory = item_factory_new (GTK_TYPE_MENU, "<local>", NULL, "/Local");
  create_item_factory (window1.ifactory, local_len - 2, menu_items + local_start + 2, &window1);

  window2.ifactory = item_factory_new (GTK_TYPE_MENU, "<remote>", NULL, "/Remote");
  create_item_factory (window2.ifactory, remote_len - 2, menu_items + remote_start + 2, &window2);

  log_factory = item_factory_new (GTK_TYPE_MENU, "<log>", NULL, "/Logging");
  create_item_factory (log_factory, log_len - 2, menu_items + log_start + 2, NULL);

  dl_factory = item_factory_new (GTK_TYPE_MENU, "<download>", NULL, "/Transfers");
  create_item_factory (dl_factory, trans_len - 2, menu_items + trans_start + 2, NULL);

  return (factory->widget);
}


static GtkWidget *
CreateToolbar (GtkWidget * parent)
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  GtkWidget *toolbar, *box, *tempwid;
  int i, num;

  toolbar = gtk_handle_box_new ();

  box = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (toolbar), box);
  gtk_container_border_width (GTK_CONTAINER (box), 5);

  tempwid = toolbar_pixmap (parent, "connect.xpm");
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

  tempwid = gtk_label_new (_("Host: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  hostedit = gtk_combo_new ();
  gtk_combo_set_case_sensitive (GTK_COMBO (hostedit), 1);
  gtk_widget_set_size_request (hostedit, 130, -1);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (hostedit)->entry), "activate",
		      GTK_SIGNAL_FUNC (toolbar_hostedit), NULL);
  if (host_history)
    gtk_combo_set_popdown_strings (GTK_COMBO (hostedit), host_history);
  gtk_combo_disable_activate (GTK_COMBO (hostedit));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry), "");
  gtk_box_pack_start (GTK_BOX (box), hostedit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Port: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  portedit = gtk_combo_new ();
  gtk_combo_set_case_sensitive (GTK_COMBO (portedit), 1);
  gtk_widget_set_size_request (portedit, 40, -1);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (portedit)->entry), "activate",
		      GTK_SIGNAL_FUNC (toolbar_hostedit), NULL);
  if (port_history)
    gtk_combo_set_popdown_strings (GTK_COMBO (portedit), port_history);
  gtk_combo_disable_activate (GTK_COMBO (portedit));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (portedit)->entry), "");
  gtk_box_pack_start (GTK_BOX (box), portedit, FALSE, FALSE, 0);

  tempwid = gtk_label_new (_("User: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  useredit = gtk_combo_new ();
  gtk_combo_set_case_sensitive (GTK_COMBO (useredit), 1);
  gtk_widget_set_size_request (useredit, 75, -1);

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (useredit)->entry), "activate",
		      GTK_SIGNAL_FUNC (toolbar_hostedit), NULL);
  if (user_history)
    gtk_combo_set_popdown_strings (GTK_COMBO (useredit), user_history);
  gtk_combo_disable_activate (GTK_COMBO (useredit));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (useredit)->entry), "");
  gtk_box_pack_start (GTK_BOX (box), useredit, TRUE, TRUE, 0);

  tempwid = gtk_label_new (_("Pass: "));
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  passedit = gtk_entry_new ();
  gtk_widget_set_size_request (passedit, 55, -1);

  gtk_entry_set_visibility (GTK_ENTRY (passedit), FALSE);
  gtk_signal_connect (GTK_OBJECT (passedit), "activate",
		      GTK_SIGNAL_FUNC (toolbar_hostedit), NULL);
  gtk_box_pack_start (GTK_BOX (box), passedit, FALSE, FALSE, 0);

  tempwid = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  optionmenu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (tempwid), optionmenu, TRUE, FALSE, 0);

  num = 0;
  protocol_menu = gtk_menu_new ();
  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (!gftp_protocols[i].shown)
        continue;

      if (strcmp (gftp_protocols[i].name, default_protocol) == 0)
        num = i;

      tempwid = gtk_menu_item_new_with_label (gftp_protocols[i].name);
      gtk_object_set_user_data (GTK_OBJECT (tempwid), (gpointer) i);
      gtk_menu_append (GTK_MENU (protocol_menu), tempwid);
      gtk_widget_show (tempwid);
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), protocol_menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), num);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  tempwid = toolbar_pixmap (parent, "stop.xpm");
#else
  tempwid = gtk_image_new_from_stock (GTK_STOCK_STOP,
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif

  stop_btn = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (stop_btn), tempwid);
  gtk_widget_set_sensitive (stop_btn, 0);
  gtk_signal_connect_object (GTK_OBJECT (stop_btn), "clicked",
			     GTK_SIGNAL_FUNC (stop_button), NULL);
  gtk_container_border_width (GTK_CONTAINER (stop_btn), 1);
  gtk_box_pack_start (GTK_BOX (box), stop_btn, FALSE, FALSE, 0);

  return (toolbar);
}


static void
doexit (GtkWidget * widget, gpointer data)
{
  listbox_local_width = GTK_WIDGET (local_frame)->allocation.width;
  listbox_remote_width = GTK_WIDGET (remote_frame)->allocation.width;
  listbox_file_height = GTK_WIDGET (remote_frame)->allocation.height;
  log_height = GTK_WIDGET (log_table)->allocation.height;
  transfer_height = GTK_WIDGET (transfer_scroll)->allocation.height;

  local_columns[0] = get_column (&GTK_CLIST (window1.listbox)->column[1]);
  local_columns[1] = get_column (&GTK_CLIST (window1.listbox)->column[2]);
  local_columns[2] = get_column (&GTK_CLIST (window1.listbox)->column[3]);
  local_columns[3] = get_column (&GTK_CLIST (window1.listbox)->column[4]);
  local_columns[4] = get_column (&GTK_CLIST (window1.listbox)->column[5]);
  local_columns[5] = get_column (&GTK_CLIST (window1.listbox)->column[6]);

  remote_columns[0] = get_column (&GTK_CLIST (window2.listbox)->column[1]);
  remote_columns[1] = get_column (&GTK_CLIST (window2.listbox)->column[2]);
  remote_columns[2] = get_column (&GTK_CLIST (window2.listbox)->column[3]);
  remote_columns[3] = get_column (&GTK_CLIST (window2.listbox)->column[4]);
  remote_columns[4] = get_column (&GTK_CLIST (window2.listbox)->column[5]);
  remote_columns[5] = get_column (&GTK_CLIST (window2.listbox)->column[6]);

  file_trans_column = get_column (&GTK_CLIST (dlwdw)->column[0]);

  gftp_write_config_file ();
  gftp_clear_cache_files ();
  exit (0);
}


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


void
init_gftp (int argc, char *argv[], GtkWidget * parent)
{
  if (argc == 2 && strncmp (argv[1], "--", 2) != 0)
    {
      if (gftp_parse_url (window2.request, argv[1]) == 0)
	ftp_connect (&window2, window2.request, 1);
      else
	gftp_usage ();
    }
}


static void
menu_exit (GtkWidget * widget, gpointer data)
{
  if (!delete_event (widget, NULL, data))
    doexit (widget, data);
}


static GtkWidget *
CreateFTPWindow (gftp_window_data * wdata, int width, int columns[6])
{
  const GtkTargetEntry possible_types[] = {
    {"STRING", 0, 0},
    {"text/plain", 0, 0},
    {"application/x-rootwin-drop", 0, 1}
  };
  GtkWidget *box, *scroll_list, *parent;
  char *titles[7];

  titles[0] = "";
  titles[1] = _("Filename");
  titles[2] = _("Size");
  titles[3] = _("User");
  titles[4] = _("Group");
  titles[5] = _("Date");
  titles[6] = _("Attribs");

  wdata->request = gftp_request_new ();
  wdata->request->logging_function = ftp_log;
  wdata->filespec = g_malloc0 (2);
  *wdata->filespec = '*';
  wdata->sortcol = 1; 
  wdata->sortasds = 1;

  parent = gtk_frame_new (NULL);
  gtk_widget_set_size_request (parent, width, listbox_file_height);

  gtk_container_border_width (GTK_CONTAINER (parent), 5);

  box = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (parent), box);

  wdata->combo = gtk_combo_new ();
  gtk_combo_set_case_sensitive (GTK_COMBO (wdata->combo), 1);
  gtk_box_pack_start (GTK_BOX (box), wdata->combo, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (wdata->combo)->entry),
		      "activate", GTK_SIGNAL_FUNC (chdir_edit),
		      (gpointer) wdata);
  if (*wdata->history)
    gtk_combo_set_popdown_strings (GTK_COMBO (wdata->combo), *wdata->history);
  gtk_combo_disable_activate (GTK_COMBO (wdata->combo));
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (wdata->combo)->entry), "");

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
  setup_column (wdata->listbox, 1, columns[0]);
  gtk_clist_set_column_justification (GTK_CLIST (wdata->listbox), 2,
				      GTK_JUSTIFY_RIGHT);
  setup_column (wdata->listbox, 2, columns[1]);
  setup_column (wdata->listbox, 3, columns[2]);
  setup_column (wdata->listbox, 4, columns[3]);
  setup_column (wdata->listbox, 5, columns[4]);
  setup_column (wdata->listbox, 6, columns[5]);
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
#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  gtk_signal_connect_after (GTK_OBJECT (wdata->listbox), "button_press_event",
                            GTK_SIGNAL_FUNC (list_dblclick), (gpointer) wdata);
#else
  g_signal_connect_after (G_OBJECT (wdata->listbox), "button_release_event",
                          G_CALLBACK (list_dblclick), (gpointer) wdata);
#endif
  return (parent);
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


static gint
menu_mouse_click (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  GtkItemFactory *factory;

  factory = (GtkItemFactory *) data;
  if (event->button == 3)
    gtk_item_factory_popup (factory, (guint) event->x_root,
			    (guint) event->y_root, 3, event->time);
  return (FALSE);
}


static gint
list_dblclick (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  gftp_window_data * wdata;

  wdata = data;
  if (event->button == 3)
    gtk_item_factory_popup (wdata->ifactory, (guint) event->x_root,
                            (guint) event->y_root, 3, event->time);
  else if (!GFTP_IS_CONNECTED (wdata->request) || !IS_ONE_SELECTED (wdata))
    return (TRUE);

#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
  if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
    {
      list_doaction (wdata);
      return (FALSE);
    }
  return (TRUE);
#else
  /* If we're using GTK 2.0, if I connect to the button_press_event signal,
     whenever I get the GDK_2BUTTON_PRESS event, nothing is selected inside
     the clist. But if I connect_after to the button_release_event, it seems
     to only get called when we double click */
  if (event->button == 1)
    {
      list_doaction (wdata);
      return (FALSE);
    }
  return (TRUE);
#endif
}


static void
list_doaction (gftp_window_data * wdata)
{
  GList *templist, *filelist;
  int num, dir, success;
  gftp_file *tempfle;

  filelist = g_list_first (wdata->files);
  templist = GTK_CLIST (wdata->listbox)->selection;
  num = 0;
  templist = get_next_selection (templist, &filelist, &num);
  tempfle = (gftp_file *) filelist->data;

  dir = tempfle->isdir;
  success = 0;

  if (tempfle->islink || tempfle->isdir)
    success = chdir_dialog (wdata);

  if (!dir && !success)
    {
      switch (listbox_dblclick_action)
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


static void
chfunc (gpointer data)
{
  chdir_dialog (data);
}


void
toolbar_hostedit (GtkWidget * widget, gpointer data)
{
  void (*init) (gftp_request * request);
  GtkWidget *tempwid;
  const char *txt;
  int num;

  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Connect"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    disconnect (current_wdata);

  tempwid = gtk_menu_get_active (GTK_MENU (protocol_menu));
  num = (int) gtk_object_get_user_data (GTK_OBJECT (tempwid));
  init = gftp_protocols[num].init;
  init (current_wdata->request);
 
  gftp_set_hostname (current_wdata->request, gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry)));
  alltrim (current_wdata->request->hostname);

  if (current_wdata->request->need_hostport && 
      *current_wdata->request->hostname == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Error: You must type in a host to connect to\n"));
      return;
    }

  add_history (hostedit, &host_history, &host_len, 
               current_wdata->request->hostname);

  if (strchr (current_wdata->request->hostname, '/') != NULL &&
      gftp_parse_url (current_wdata->request, 
                      current_wdata->request->hostname) == 0)
    {
      ftp_connect (current_wdata, current_wdata->request, 1);
      return;
    }
 
  txt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (portedit)->entry));
  gftp_set_port (current_wdata->request, strtol (txt, NULL, 10));
  add_history (portedit, &port_history, &port_len, txt);

  gftp_set_username (current_wdata->request, gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (useredit)->entry)));
  alltrim (current_wdata->request->username);

  add_history (useredit, &user_history, &user_len, 
               current_wdata->request->username);

  gftp_set_password (current_wdata->request,
		     gtk_entry_get_text (GTK_ENTRY (passedit)));

  gftp_set_directory (current_wdata->request, gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (current_wdata->combo)->entry)));
  alltrim (current_wdata->request->directory);

  add_history (current_wdata->combo, current_wdata->history, 
               current_wdata->histlen, current_wdata->request->directory);

  ftp_connect (current_wdata, current_wdata->request, 1);
}


void
sortrows (GtkCList * clist, gint column, gpointer data)
{
  gftp_window_data * wdata;
  GtkWidget * sort_wid;
  GList * templist;
  int swap_col;

  wdata = data;
  if (column == 0 || (column == wdata->sortcol && wdata->sorted))
    {
      wdata->sortasds = !wdata->sortasds;
      swap_col = 1;
    }
  else
    swap_col = 0;

  if (swap_col || !wdata->sorted)
    {
      sort_wid = gtk_clist_get_column_widget (clist, 0);
      gtk_widget_destroy (sort_wid);
#if GTK_MAJOR_VERSION == 1 && GTK_MINOR_VERSION == 2
      if (wdata->sortasds)
	sort_wid = toolbar_pixmap (wdata->listbox, "down.xpm");
      else
	sort_wid = toolbar_pixmap (wdata->listbox, "up.xpm");
#else
      if (wdata->sortasds)
        sort_wid = gtk_image_new_from_stock (GTK_STOCK_SORT_ASCENDING, 
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);
      else
        sort_wid = gtk_image_new_from_stock (GTK_STOCK_SORT_DESCENDING, 
                                             GTK_ICON_SIZE_SMALL_TOOLBAR);
#endif

      gtk_clist_set_column_widget (clist, 0, sort_wid);
    }
  else
    wdata->sortcol = column;

  gtk_clist_freeze (clist);
  gtk_clist_clear (clist);

  wdata->files = gftp_sort_filelist (wdata->files, wdata->sortcol, 
                                     wdata->sortasds);

  templist = wdata->files; 
  while (templist != NULL)
    {
      add_file_listbox (wdata, templist->data);
      templist = templist->next;
    }

  wdata->sorted = 1;
  gtk_clist_thaw (clist);
  update_window_info ();
}


void
stop_button (GtkWidget * widget, gpointer data)
{
  pthread_t comptid;

  memset (&comptid, 0, sizeof (comptid));
  if (!pthread_equal (comptid, window1.tid))
    pthread_kill (window1.tid, SIGINT);
  else if (!pthread_equal (comptid, window2.tid))
    pthread_kill (window2.tid, SIGINT);
}

