/*****************************************************************************/
/*  options.h - the global variables for the program                         */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                  */
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

/* $Id$ */

#include "gftp.h"

static char *gftp_sort_columns[] = { N_("none"), N_("file"), N_("size"), 
                                     N_("user"), N_("group"), 
                                     N_("datetime"), N_("attribs"), NULL };

static char *gftp_sort_direction[] = { N_("descending"), N_("ascending"), NULL };

static float gftp_maxkbs = 0.0;

gftp_config_vars gftp_global_config_vars[] =
{
  {"", N_("General"), gftp_option_type_notebook, NULL, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},

  {"view_program", N_("View program:"), gftp_option_type_text, "", NULL, 0,
   N_("The default program used to view files. If this is blank, the internal file viewer will be used"), 
   GFTP_PORT_GTK, NULL},
  {"edit_program", N_("Edit program:"), gftp_option_type_text, "", NULL, 0,
   N_("The default program used to edit files."), GFTP_PORT_GTK, NULL},
  {"startup_directory", N_("Startup Directory:"), 
   gftp_option_type_text, "", NULL, 0,
   N_("The default directory gFTP will go to on startup"), GFTP_PORT_ALL, NULL},
  {"max_log_window_size", N_("Max Log Window Size:"), 
   gftp_option_type_int, 0, NULL, 0, 
   N_("The maximum size of the log window in bytes for the GTK+ port"), 
   GFTP_PORT_GTK, NULL},
  {"remote_charsets", N_("Remote Character Sets:"), 
   gftp_option_type_text, "", NULL, GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("This is a comma separated list of charsets to try to convert the remote messages to the current locale"), 
   GFTP_PORT_ALL, NULL},
  {"cache_ttl", N_("Cache TTL:"), 
   gftp_option_type_int, GINT_TO_POINTER(3600), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The number of seconds to keep cache entries before they expire."), 
   GFTP_PORT_ALL, NULL},

  {"append_transfers", N_("Append file transfers"), 
  gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 0,
   N_("Append new file transfers onto existing ones"), GFTP_PORT_GTK, NULL},
  {"one_transfer", N_("Do one transfer at a time"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 0,
   N_("Do only one transfer at a time?"), GFTP_PORT_GTK, NULL},
  {"overwrite_default", N_("Overwrite by Default"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Overwrite files by default or set to resume file transfers"), 
   GFTP_PORT_GTK, NULL},
  {"preserve_permissions", N_("Preserve file permissions"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Preserve file permissions of transfered files"), GFTP_PORT_ALL,
   NULL},
  {"refresh_files", N_("Refresh after each file transfer"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Refresh the listbox after each file is transfered"), GFTP_PORT_GTK,
   NULL},
  {"sort_dirs_first", N_("Sort directories first"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Put the directories first then the files"), GFTP_PORT_ALL, NULL},
  {"show_hidden_files", N_("Show hidden files"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Show hidden files in the listboxes"), GFTP_PORT_ALL, NULL},
  {"show_trans_in_title", N_("Show transfer status in title"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 0,
   N_("Show the file transfer status in the titlebar"), GFTP_PORT_GTK, NULL},
  {"cmd_in_gui", N_("Allow manual commands in GUI"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(0), NULL, 0,
   N_("Allow entering manual commands in the GUI (functions like the text port)"), GFTP_PORT_GTK, NULL},

  {"", N_("Network"), gftp_option_type_notebook, NULL, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},
  {"network_timeout", N_("Network timeout:"), 
   gftp_option_type_int, GINT_TO_POINTER(60), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The timeout waiting for network input/output. This is NOT an idle timeout."), 
   GFTP_PORT_ALL, NULL},
  {"retries", N_("Connect retries:"), 
   gftp_option_type_int, GINT_TO_POINTER(3), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The number of auto-retries to do. Set this to 0 to retry indefinately"), 
   GFTP_PORT_ALL, NULL},
  {"sleep_time", N_("Retry sleep time:"), 
   gftp_option_type_int, GINT_TO_POINTER(30), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The number of seconds to wait between retries"), GFTP_PORT_ALL, NULL},
  {"maxkbs", N_("Max KB/S:"), 
   gftp_option_type_float, &gftp_maxkbs, NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("The maximum KB/s a file transfer can get. (Set to 0 to disable)"),  
   GFTP_PORT_ALL, NULL},

  {"default_protocol", N_("Default Protocol:"),
   gftp_option_type_textcombo, "FTP", NULL, 0,
   N_("This specifies the default protocol to use"), GFTP_PORT_ALL, NULL},
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  {"enable_ipv6", N_("Enable IPv6 support"), 
   gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 
   GFTP_CVARS_FLAGS_SHOW_BOOKMARK,
   N_("Enable IPv6 support"), GFTP_PORT_ALL, NULL},
#endif

  {"list_dblclk_action", "", 
   gftp_option_type_int, GINT_TO_POINTER(0), NULL, 0,
   N_("This defines what will happen when you double click a file in the file listboxes. 0=View file 1=Edit file 2=Transfer file"), 0, NULL},
  {"listbox_local_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(302), NULL, 0,
   N_("The default width of the local files listbox"), 0, NULL},
  {"listbox_remote_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(302), NULL, 0,
   N_("The default width of the remote files listbox"), 0, NULL},
  {"listbox_file_height", "",
   gftp_option_type_int, GINT_TO_POINTER(265), NULL, 0,
   N_("The default height of the local/remote files listboxes"), 0, NULL},
  {"transfer_height", "", 
   gftp_option_type_int, GINT_TO_POINTER(80), NULL, 0,
   N_("The default height of the transfer listbox"), 0, NULL},
  {"log_height", "", 
   gftp_option_type_int, GINT_TO_POINTER(105), NULL, 0,
   N_("The default height of the logging window"), 0, NULL},
  {"file_trans_column", "", 
   gftp_option_type_int, GINT_TO_POINTER(100), NULL, 0,
   N_("The width of the filename column in the transfer window. Set this to 0 to have this column automagically resize."), 0, NULL},

  {"local_sortcol", "", 
   gftp_option_type_intcombo, GINT_TO_POINTER(1), gftp_sort_columns, 0,
   N_("The default column to sort by"), GFTP_PORT_TEXT, NULL},
  {"local_sortasds", "", 
   gftp_option_type_intcombo, GINT_TO_POINTER(1), gftp_sort_direction, 0,
   N_("Sort ascending or descending"), GFTP_PORT_TEXT, NULL},
  {"remote_sortcol", "", 
   gftp_option_type_intcombo, GINT_TO_POINTER(1), gftp_sort_columns, 0,
   N_("The default column to sort by"), GFTP_PORT_TEXT, NULL},
  {"remote_sortasds", "", 
   gftp_option_type_intcombo, GINT_TO_POINTER(1), gftp_sort_direction, 0,
   N_("Sort ascending or descending"), GFTP_PORT_TEXT, NULL},

  {"local_file_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(0), NULL, 0,
   N_("The width of the filename column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"local_size_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(85), NULL, 0,
   N_("The width of the size column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"local_user_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(75), NULL, 0,
   N_("The width of the user column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"local_group_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(76), NULL, 0,
   N_("The width of the group column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"local_date_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(120), NULL, 0,
   N_("The width of the date column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"local_attribs_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(75), NULL, 0,
   N_("The width of the attribs column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_file_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(0), NULL, 0,
   N_("The width of the filename column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_size_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(85), NULL, 0,
   N_("The width of the size column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_user_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(75), NULL, 0,
   N_("The width of the user column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_group_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(76), NULL, 0,
   N_("The width of the group column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_date_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(120), NULL, 0,
   N_("The width of the date column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"remote_attribs_width", "", 
   gftp_option_type_int, GINT_TO_POINTER(75), NULL, 0,
   N_("The width of the attribs column in the file listboxes. Set this to 0 to have this column automagically resize. Set this to -1 to disable this column"), 0, NULL},
  {"send_color", "", 
   gftp_option_type_color, "0:8600:0", NULL, 0,
   N_("The color of the commands that are sent to the server"), 0, NULL},
  {"recv_color", "", 
   gftp_option_type_color, "0:0:ffff", NULL, 0,
   N_("The color of the commands that are received from the server"), 0, NULL},
  {"error_color", "", 
   gftp_option_type_color, "ffff:0:0", NULL, 0,
   N_("The color of the error messages"), 0, NULL},
  {"misc_color", "", 
   gftp_option_type_color, "a000:8d00:4600", NULL, 0,
   N_("The color of the rest of the log messages"), 0, NULL},
  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};

supported_gftp_protocols gftp_protocols[] =
{
  {N_("FTP"), rfc959_init, rfc959_register_module, "ftp", 1, 1},

#ifdef USE_SSL
  {N_("FTPS"), ftps_init, ftps_register_module, "ftps", 1, 1},
#else
  {N_("FTPS"), ftps_init, ftps_register_module, "ftps", 0, 1},
#endif

  {N_("HTTP"), rfc2068_init, rfc2068_register_module, "http", 1, 1},

#ifdef USE_SSL
  {N_("HTTPS"), https_init, https_register_module, "https", 1, 1},
#else
  {N_("HTTPS"), https_init, https_register_module, "https", 0, 1},
#endif

  {N_("Local"), local_init, local_register_module, "file", 1, 0},

  {N_("SSH2"), sshv2_init, sshv2_register_module, "ssh2", 1, 1},

  {N_("Bookmark"), bookmark_init, bookmark_register_module, "bookmark", 0, 0},

  {NULL, NULL, NULL, NULL, 0}
};

GHashTable * gftp_global_options_htable = NULL, 	
           * gftp_config_list_htable = NULL,
           * gftp_bookmarks_htable = NULL;

char gftp_version[] = "gFTP " VERSION;

GList * gftp_file_transfers = NULL, 
      * gftp_file_transfer_logs = NULL,
      * gftp_options_list = NULL;
      
gftp_bookmarks_var * gftp_bookmarks = NULL;

FILE * gftp_logfd = NULL;

int gftp_configuration_changed = 0;

