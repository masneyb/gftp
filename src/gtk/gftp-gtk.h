/***********************************************************************************/
/*  gftp-gtk.h - include file for the gftp gtk+ port                               */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                        */
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

/* $Id$ */

#ifndef __GFTP_GTK_H
#define __GFTP_GTK_H

#include "../../lib/gftp.h"
#include "../uicommon/gftpui.h"
#include "../gtkcompat.h"
#include <pthread.h>

#define GFTP_MENU_ITEM_ASCII	1
#define GFTP_MENU_ITEM_BINARY	2
#define GFTP_MENU_ITEM_WIN1	3
#define GFTP_MENU_ITEM_WIN2	4

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

/* These are used for the TextEntryDialog function. I have these types to make
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
  GtkWidget *dir_combo, /* Entry widget/history for the user to enter  a directory */
            *dirinfo_label,  /* protocol and dir info */
            *listbox; 		/* Our listbox showing the files */
  unsigned int sorted : 1,	/* Is the output sorted? */
               show_selected : 1, /* Show only selected files */
               *histlen;	/* Pointer to length of history */
  char *filespec;		/* Filespec for the listbox */
  gftp_request * request;	/* The host that we are connected to */
  GList * files,		/* Files in the listbox */
        ** history;		/* History of the directories */
  GtkUIManager *ifactory; 	/* This is for the menus that will
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
  /* the dialog is destroyed but these vars
   * are available to yesfunc() and nofunc() */
  char * entry_text;
  int checkbox_is_ticked;

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

extern GtkWindow *main_window;
extern gftp_window_data window1, window2, * other_wdata, * current_wdata;
extern GtkWidget * stop_btn, * hostedit, * useredit, * passedit,
                 * portedit, * logwdw, * dlwdw, * toolbar_combo_protocol,
                 * gftpui_command_widget, * download_left_arrow,
                 * upload_right_arrow, * openurl_btn;
extern GtkAdjustment * logwdw_vadj;
extern GtkTextMark * logwdw_textmark;
extern int local_start, remote_start, trans_start;
extern GHashTable * graphic_hash_table;
extern GHashTable * pixbuf_hash_table;

extern GtkActionGroup * menus;
extern GtkUIManager * factory;

extern pthread_mutex_t log_mutex;
extern pthread_t main_thread_id;
extern GList * viewedit_processes;

extern intptr_t gftp_gtk_colored_msgs;


/* bookmarks.c */
void run_bookmark 				( gpointer data );

void add_bookmark 				( gpointer data );

void edit_bookmarks 				( gpointer data );

void build_bookmarks_menu			( void );

/* chmod_dialog.c */ 
void chmod_dialog 				( gpointer data );

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

void toolbar_hostedit (void);

void stop_button				( GtkWidget * widget,
						  gpointer data );

void gftpui_show_or_hide_command 		( void );

/* listbox.c */
void listbox_sort_rows (gpointer data, gint column);

void listbox_select_row      (gftp_window_data *wdata, int n);
void listbox_update_filelist (gftp_window_data *wdata);
int  listbox_num_selected    (gftp_window_data *wdata);
void listbox_clear           (gftp_window_data *wdata);
void listbox_select_all      (gftp_window_data *wdata);
void listbox_deselect_all    (gftp_window_data *wdata);
void * listbox_get_selected_files (gftp_window_data *wdata, int only_one);

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

gboolean dir_combo_keycb (GtkWidget * widget, GdkEventKey *event, gpointer data);

void clearlog 					( gpointer data );

void viewlog 					( gpointer data );

void savelog 					( gpointer data );

void clear_cache				( gpointer data );

void compare_windows 				( gpointer data );

void about_dialog 				( gpointer data );
void delete_dialog 				( gpointer data );

/* misc-gtk.c */
void remove_files_window			( gftp_window_data * wdata );

void ftp_log					( gftp_logging_level level,
						  gftp_request * request,
						  const char *string,
						  ... ) GFTP_LOG_FUNCTION_ATTRIBUTES;

void update_window_info				( void );

void update_window				( gftp_window_data * wdata );

gftp_graphic * open_xpm				( GtkWidget * widget,
						  char *filename );

void gftp_free_pixmap 				( char *filename ); 

void gftp_get_pixmap 				( GtkWidget * widget, 
						  char *filename, 
						  GdkPixmap ** pix,
						  GdkBitmap ** bitmap );

GdkPixbuf *       gftp_get_pixbuf (char *filename);

int check_status				( char *name,
						  gftp_window_data * wdata,
						  unsigned int check_other_stop,
						  unsigned int only_one,
						  unsigned int at_least_one,
						  unsigned int func );

void add_history 				( GtkWidget * widget, 
						  GList ** history, 
						  unsigned int *histlen, 
						  const char *str );

int check_reconnect 				( gftp_window_data * wdata );

void TextEntryDialog (GtkWindow * parent_window,       /* nullable */
                      char * title,   char * infotxt,
                      char * deftext, int passwd_item,
                      char * checktext, 
                      gftp_dialog_button okbutton, void (*okfunc) (), void *okptr,
                      void (*cancelfunc) (), void *cancelptr);

void YesNoDialog (GtkWindow * parent_window,              /* nullable */
                  char * title,       char * infotxt, 
                  void (*yesfunc) (), gpointer yespointer, 
                  void (*nofunc) (),  gpointer nopointer);

void display_cached_logs			( void );

char * get_image_path 				( char *filename);

void set_window_icon (GtkWindow *window, char *icon_name);
void glist_to_combobox (GList *list, GtkWidget *combo);
void populate_combo_and_select_protocol (GtkWidget *combo, char * selected_protocol);
GtkMenuItem * new_menu_item (GtkMenu * menu, char * label, char * icon_name,
                             gpointer activate_callback, gpointer callback_data);

/* options_dialog.c */
void options_dialog 				( gpointer data );

void gftp_gtk_setup_bookmark_options 		( GtkWidget * notebook,
						  gftp_bookmarks_var * bm );

void gftp_gtk_save_bookmark_options 		( void );

/* platform_specific.c */
void gftp_gtk_platform_specific_init		( void );

/* transfer.c */
int  gftp_gtk_list_files				( gftp_window_data * wdata );

int gftp_gtk_connect					( gftp_window_data * wdata,
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

