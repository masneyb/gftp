/*****************************************************************************/
/*  gftp-gtk.h - include file for the gftp gtk+ port                         */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

/* $Id$ */

#ifndef __GFTP_GTK_H
#define __GFTP_GTK_H

#include "../../lib/gftp.h"
#include "../uicommon/gftpui.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pthread.h>

#define GFTP_MENU_ITEM_ASCII	1
#define GFTP_MENU_ITEM_BINARY	2
#define GFTP_MENU_ITEM_WIN1	3
#define GFTP_MENU_ITEM_WIN2	4

#define IS_ONE_SELECTED(wdata)		(GTK_CLIST ((wdata)->listbox)->selection && GTK_CLIST ((wdata)->listbox)->selection->next == NULL)
#define IS_NONE_SELECTED(wdata)		(GTK_CLIST ((wdata)->listbox)->selection == NULL)
#define gftp_gtk_get_list_selection(wdata)  (GTK_CLIST ((wdata)->listbox)->selection)

#define GFTP_IS_SAME_HOST_START_TRANS(wdata,trequest) \
  ((wdata) != NULL && (wdata)->request != NULL && \
  (wdata)->request->datafd > 0 && !(wdata)->request->always_connected && \
  !(wdata)->request->stopable && \
  compare_request (trequest, (wdata)->request, 0))

#define GFTP_IS_SAME_HOST_STOP_TRANS(wdata,trequest) \
  ((wdata) != NULL && (wdata)->request != NULL && \
  (wdata)->request->datafd < 0 && !(wdata)->request->always_connected && \
  (wdata)->request->cached && !(wdata)->request->stopable && \
  trequest->datafd > 0 && !trequest->always_connected && \
  compare_request (trequest, (wdata)->request, 0))

/* These 2 defines are for creating menu items with stock icons in GTK+ 2.0. */
#define MS_(a) "<StockItem>",a
#define MN_(a) a,NULL

/* These are used for the MakeEditDialog function. I have these types to make
   it easier for creating dialogs with GTK+ 2.0 */

typedef enum gftp_dialog_button_tag
{
  gftp_dialog_button_create,
  gftp_dialog_button_change,
  gftp_dialog_button_connect,
  gftp_dialog_button_rename,
  gftp_dialog_button_ok
} gftp_dialog_button;

typedef struct gftp_window_data_tag
{
  GtkWidget *combo, 		/* Entry widget/history for the user to enter 
   				   a directory */
            *hoststxt, 		/* Show which directory we're in */
            *listbox; 		/* Our listbox showing the files */
  unsigned int sorted : 1,	/* Is the output sorted? */
               show_selected : 1, /* Show only selected files */
               *histlen;	/* Pointer to length of history */
  char *filespec;		/* Filespec for the listbox */
  gftp_request * request;	/* The host that we are connected to */
  GList * files,		/* Files in the listbox */
        ** history;		/* History of the directories */
  GtkItemFactory *ifactory; 	/* This is for the menus that will
                                   come up when you right click */
  pthread_t tid;		/* Thread for the stop button */
  char *prefix_col_str;
} gftp_window_data;


typedef struct _gftpui_gtk_thread_data
{
  void * (*func) (void *);
  gftpui_callback_data * cdata;
} gftpui_gtk_thread_data;


typedef struct gftp_graphic_tag
{
  char * filename;
  GdkPixmap * pixmap;
  GdkBitmap * bitmap;
} gftp_graphic;


typedef struct gftp_dialog_data_tag
{
  GtkWidget * dialog,
            * checkbox,
            * edit;

  void (*yesfunc) ();
  gpointer yespointer;

  void (*nofunc) ();
  gpointer nopointer;
} gftp_dialog_data;


typedef struct gftp_viewedit_data_tag 
{
   char *filename,              /* File we are viewing/editing currently */
        *remote_filename;       /* The filename on the remote computer */
   struct stat st;              /* Vital file statistics */
   pid_t pid;                   /* Our process id */
   char **argv;                 /* Our arguments we passed to execvp. We will 
                                   free it when the process terminates. This 
                                   is the safest place to free this */
   unsigned int view : 1,       /* View or edit this file */
                rm : 1,         /* Delete this file after we're done with it */
                dontupload : 1; /* Don't upload this file after we're done
				   editing it */
   gftp_window_data * fromwdata, /* The window we are viewing this file in */
                    * towdata;
   gftp_request * torequest;
} gftp_viewedit_data;


typedef struct gftp_save_dir_struct_tag
{
  GtkWidget * filew;
  gftp_window_data * wdata;
} gftp_save_dir_struct;


typedef struct gftp_textcomboedt_widget_data_tag
{
  GtkWidget * combo,
            * text;
  gftp_config_vars * cv;
  char * custom_edit_value;
} gftp_textcomboedt_widget_data;


typedef struct gftp_options_dialog_data_tag
{
  GtkWidget * dialog,
            * notebook,
            * box,
            * table;
  unsigned int tbl_col_num,
               tbl_row_num;
  gftp_option_type_enum last_option;
  gftp_bookmarks_var * bm;
} gftp_options_dialog_data;


extern gftp_window_data window1, window2, * other_wdata, * current_wdata;
extern GtkWidget * stop_btn, * hostedit, * useredit, * passedit,
                 * portedit, * logwdw, * dlwdw, * optionmenu,
                 * gftpui_command_widget, * download_left_arrow,
                 * upload_right_arrow, * openurl_btn;
extern GtkAdjustment * logwdw_vadj;
extern GtkTextMark * logwdw_textmark;
extern int local_start, remote_start, trans_start;
extern GHashTable * graphic_hash_table;
extern GtkItemFactoryEntry * menus;
extern GtkItemFactory * factory;
extern pthread_mutex_t log_mutex;
extern pthread_t main_thread_id;
extern GList * viewedit_processes;


/* bookmarks.c */
void run_bookmark 				( gpointer data );

void add_bookmark 				( gpointer data );

void edit_bookmarks 				( gpointer data );

void build_bookmarks_menu			( void );

/* chmod_dialog.c */ 
void chmod_dialog 				( gpointer data );

/* delete_dialog.c */ 
void delete_dialog 				( gpointer data );

/* dnd.c */
void openurl_get_drag_data 			( GtkWidget * widget, 
						  GdkDragContext * context, 
						  gint x,
						  gint y, 
						  GtkSelectionData * selection_data, 
						  guint info, 
						  guint32 clk_time, 
						  gpointer data );

void listbox_drag 				( GtkWidget * widget, 
						  GdkDragContext * context,
						  GtkSelectionData * selection_data, 
						  guint info, 
						  guint32 clk_time, 
						  gpointer data );

void listbox_get_drag_data 			( GtkWidget * widget, 
						  GdkDragContext * context, 
						  gint x,
						  gint y, 
						  GtkSelectionData * selection_data, 
						  guint info, 
						  guint32 clk_time, 
						  gpointer data );

/* gftp-gtk.c */
void gftp_gtk_init_request 			( gftp_window_data * wdata );

void toolbar_hostedit 				( GtkWidget * widget, 
						  gpointer data );

void sortrows 					( GtkCList * clist, 
						  gint column, 
						  gpointer data );

void stop_button				( GtkWidget * widget,
						  gpointer data );

void gftpui_show_or_hide_command 		( void );

/* gtkui.c */
void gftpui_run_command 			( GtkWidget * widget,
						  gpointer data );

void gftpui_run_function_callback 		( gftp_window_data * wdata,
						  gftp_dialog_data * ddata );

void gftpui_run_function_cancel_callback 	( gftp_window_data * wdata,
						  gftp_dialog_data * ddata );

void gftpui_mkdir_dialog 			( gpointer data );

void gftpui_rename_dialog			( gpointer data );

void gftpui_site_dialog 			( gpointer data );

int gftpui_run_chdir 				( gpointer uidata,
						  char *directory );

void gftpui_chdir_dialog 			( gpointer data );

char * gftpui_gtk_get_utf8_file_pos 		( gftp_file * fle );

/* menu_items.c */
void change_filespec 				( gpointer data );

void save_directory_listing 			( gpointer data );

void show_selected				( gpointer data );

void selectall 					( gpointer data );

void selectallfiles 				( gpointer data );

void deselectall 				( gpointer data );

int chdir_edit					( GtkWidget * widget,
						  gpointer data );

void clearlog 					( gpointer data );

void viewlog 					( gpointer data );

void savelog 					( gpointer data );

void clear_cache				( gpointer data );

void compare_windows 				( gpointer data );

void about_dialog 				( gpointer data );

/* misc-gtk.c */
void fix_display				( void );

void remove_files_window			( gftp_window_data * wdata );

void ftp_log					( gftp_logging_level level,
						  gftp_request * request,
						  const char *string,
						  ... ) GFTP_LOG_FUNCTION_ATTRIBUTES;

void update_window_info				( void );

void update_window				( gftp_window_data * wdata );

GtkWidget * toolbar_image			( GtkWidget * widget,
						  char *filename );

gftp_graphic * open_xpm				( GtkWidget * widget,
						  char *filename );

void gftp_free_pixmap 				( char *filename ); 

void gftp_get_pixmap 				( GtkWidget * widget, 
						  char *filename, 
						  GdkPixmap ** pix,
						  GdkBitmap ** bitmap );

int check_status				( char *name,
						  gftp_window_data * wdata,
						  unsigned int check_other_stop,
						  unsigned int only_one,
						  unsigned int at_least_one,
						  unsigned int func );

GtkItemFactory *item_factory_new                ( GtkType	       container_type,
						  const char	      *path,
						  GtkAccelGroup       *accel_group,
						  const char          *strip_prefix );

void create_item_factory 			( GtkItemFactory * ifactory, 
						  gint n_entries, 
						  GtkItemFactoryEntry * entries,
						  gpointer callback_data );

void add_history 				( GtkWidget * widget, 
						  GList ** history, 
						  unsigned int *histlen, 
						  const char *str );

int check_reconnect 				( gftp_window_data * wdata );

void add_file_listbox 				( gftp_window_data * wdata, 
						  gftp_file * fle );

void destroy_dialog 				( gftp_dialog_data * ddata );

void MakeEditDialog 				( char *diagtxt, 
						  char *infotxt, 
						  char *deftext, 
						  int passwd_item,
						  char *checktext, 
						  gftp_dialog_button okbutton, 
						  void (*okfunc) (), 
						  void *okptr, 
						  void (*cancelfunc) (), 
						  void *cancelptr );

void MakeYesNoDialog 				( char *diagtxt, 
						  char *infotxt, 
						  void (*yesfunc) (), 
						  gpointer yespointer,
						  void (*nofunc) (), 
						  gpointer nopointer );

void update_directory_download_progress 	( gftp_transfer * transfer );

int progress_timeout 				( gpointer data );

void display_cached_logs			( void );

char * get_image_path 				( char *filename, 
						  int quit_on_err );

void set_window_icon(GtkWindow *window, char *icon_name);

/* options_dialog.c */
void options_dialog 				( gpointer data );

void gftp_gtk_setup_bookmark_options 		( GtkWidget * notebook,
						  gftp_bookmarks_var * bm );

void gftp_gtk_save_bookmark_options 		( void );

/* platform_specific.c */
void gftp_gtk_platform_specific_init		( void );

/* transfer.c */
int ftp_list_files				( gftp_window_data * wdata );

int ftp_connect					( gftp_window_data * wdata,
						  gftp_request * request );

gint update_downloads 				( gpointer data );

void get_files 					( gpointer data );

void put_files 					( gpointer data );

void transfer_window_files 			( gftp_window_data * fromwdata,
						  gftp_window_data * towdata );

int gftp_gtk_get_subdirs 			( gftp_transfer * transfer );

void *do_getdir_thread 				( void * data );

void start_transfer				( gpointer data );

void stop_transfer				( gpointer data );

void skip_transfer				( gpointer data );

void remove_file_transfer 			( gpointer data );

void move_transfer_up				( gpointer data );

void move_transfer_down				( gpointer data );

/* view_dialog.c */
void edit_dialog 				( gpointer data );

void view_dialog 				( gpointer data );

void view_file 					( char *filename, 
						  int fd, 
						  unsigned int viewedit, 
						  unsigned int del_file, 
						  unsigned int start_pos, 
						  unsigned int dontupload,
						  char *remote_filename, 
						  gftp_window_data * wdata );

#endif

