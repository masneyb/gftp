/*****************************************************************************/
/*  gftp.h - include file for the whole ftp program                          */
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

#ifndef __GFTP_H
#define __GFTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifndef TIOCGWINSZ
#include <sys/ioctl.h>
#endif
#include <sys/wait.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <utime.h>
#include <signal.h>
#include <termios.h>
#include <pwd.h>
#include <setjmp.h>
#include <dirent.h>
#include <grp.h>
#include <math.h>

#ifdef HAVE_GRANTPT
#include <stropts.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#else 
#define _(String) String
#endif
#define N_(String) String

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

/* We need the major() and minor() macros in the user interface. If they aren't
   defined by the system, we'll just define them here. */
#ifndef major
#warning major macro was not defined by the system. Defining one that is probably wrong for your system
#define major(dev) (((dev) >> 8) & 0xff)
#endif

#ifndef minor
#warning minor macro was not defined by the system. Defining one that is probably wrong for your system
#define minor(dev) ((dev) & 0xff)
#endif

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/* Server types (used by FTP protocol from SYST command) */
#define GFTP_DIRTYPE_UNIX	1
#define GFTP_DIRTYPE_EPLF	2
#define GFTP_DIRTYPE_CRAY	3	
#define GFTP_DIRTYPE_NOVELL	4	
#define GFTP_DIRTYPE_DOS	5	
#define GFTP_DIRTYPE_VMS	6	
#define GFTP_DIRTYPE_OTHER 	7

/* Error types */
#define GFTP_ERETRYABLE		-1
#define GFTP_EFATAL		-2

/* Some general settings */
#define BASE_CONF_DIR		"~/.gftp"
#define CONFIG_FILE		BASE_CONF_DIR "/gftprc"
#define BOOKMARKS_FILE		BASE_CONF_DIR "/bookmarks"
#define LOG_FILE		BASE_CONF_DIR "/gftp.log"
#define MAX_HIST_LEN		10

typedef enum gftp_transfer_type_tag 
{
  gftp_transfer_passive,
  gftp_transfer_active
} gftp_transfer_type;


typedef enum gftp_logging_level_tag 
{
  gftp_logging_send,
  gftp_logging_recv,
  gftp_logging_error,
  gftp_logging_misc
} gftp_logging_level;

typedef void (*gftp_logging_func)		( gftp_logging_level level, 
						  void *ptr, 
						  const char *string, ... );


typedef struct gftp_file_tag gftp_file;

#define GFTP_TRANS_ACTION_OVERWRITE		1
#define GFTP_TRANS_ACTION_RESUME		2
#define GFTP_TRANS_ACTION_SKIP			3

#define GFTP_DIRECTION_DOWNLOAD			0
#define GFTP_DIRECTION_UPLOAD			1

#define GFTP_SORT_COL_FILE			1
#define GFTP_SORT_COL_SIZE			2
#define GFTP_SORT_COL_USER			3
#define GFTP_SORT_COL_GROUP			4
#define GFTP_SORT_COL_DATETIME			5
#define GFTP_SORT_COL_ATTRIBS			6

struct gftp_file_tag 
{
  char *file,			/* Our filename */
       *user,			/* User that owns it */
       *group,			/* Group that owns it */
       *attribs,		/* Attribs (-rwxr-x-rx) */
       *destfile;		/* Full pathname to the destination for the 
                                   file transfer */

  int fd;			/* Already open fd for this file */
  /* FIXME - add fd_open function */

  time_t datetime;		/* File date and time */
  off_t size,			/* Size of the file */
        startsize;		/* Size to start the transfer at */
  unsigned int isdir : 1,	/* File type */
               isexe : 1,
               islink : 1,
               selected : 1,	/* Is this file selected? */
               was_sel : 1,	/* Was this file selected before  */
               shown : 1,	/* Is this file shown? */
               done_view : 1,	/* View the file when done transfering? */
               done_edit : 1,	/* Edit the file when done transfering? */
               done_rm : 1,	/* Remove the file when done */
               transfer_done : 1, /* Is current file transfer done? */
               is_fd : 1;	/* Is this a file descriptor? */
  char transfer_action;		/* See the GFTP_TRANS_ACTION_* vars above */
  void *user_data;
};


typedef struct gftp_proxy_hosts_tag 
{
  /* FIXME - add IPV4 stuff here */

  gint32 ipv4_network_address, 
         ipv4_netmask;
  char *domain;
} gftp_proxy_hosts;

typedef struct gftp_request_tag gftp_request;

struct gftp_request_tag 
{
  int protonum;			/* Current number of the protocol this is 
                                   set to */
  char *hostname,		/* Hostname we will connect to */
       *username,		/* Username for host*/
       *password,		/* Password for host */
       *account,		/* Account for host (FTP only) */
       *directory,		/* Current working directory */
       *url_prefix,		/* URL Prefix (ex: ftp) */
       *last_ftp_response,	/* Last response from server */
       *last_dir_entry;		/* Last dir entry from server */
  size_t last_dir_entry_len;	/* Length of last_dir_entry */

  unsigned int port;		/* Port of remote site */

  int sockfd,			/* Control connection (read) */
      datafd,			/* Data connection */
      cachefd;			/* For the directory cache */
  int wakeup_main_thread[2];	/* FD that gets written to by the threads
                                   to wakeup the parent */

  /* One of these are used to lookup the IP address of the host we are
     connecting to */
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo *hostp;
#else
  struct hostent host, *hostp;
#endif

  int server_type;		/* The type of server we are connected to.
                                   See GFTP_DIRTYPE_* above */
  unsigned int use_proxy : 1,
               always_connected : 1,
               need_hostport : 1,
               need_userpass : 1,
               use_cache : 1,           /* Enable or disable the cache */
               use_threads : 1,         /* Whether we need to spawn a thread
                                           for this protocol */
               cached : 1,              /* Is this directory listing cached? */
               cancel : 1,		/* If a signal is received, should
					   we cancel this operation */
               stopable : 1;

  off_t gotbytes;
 
  void *protocol_data;
   
  gftp_logging_func logging_function;
  void *user_data;

  void (*init)				( gftp_request * request );
  void (*destroy)			( gftp_request * request );
  int (*connect)			( gftp_request * request );
  void (*disconnect) 			( gftp_request * request );
  off_t (*get_file) 			( gftp_request * request, 
					  const char *filename, 
					  int fd,
					  off_t startsize );
  int (*put_file) 			( gftp_request * request, 
					  const char *filename, 
					  int fd,
					  off_t startsize,
					  off_t totalsize );
  long (*transfer_file) 		( gftp_request * fromreq, 
					  const char *fromfile, 
					  off_t fromsize, 
					  gftp_request * toreq, 
					  const char *tofile, 
					  off_t tosize );
  ssize_t (*get_next_file_chunk) 	( gftp_request * request, 
					  char *buf, 
					  size_t size );
  ssize_t (*put_next_file_chunk) 	( gftp_request * request, 
					  char *buf, 
					  size_t size );
  int (*end_transfer) 			( gftp_request * request );
  int (*abort_transfer) 		( gftp_request * request );
  int (*list_files) 			( gftp_request * request );
  int (*get_next_file)			( gftp_request * request, 
					  gftp_file *fle, 
					  int fd );
  off_t (*get_file_size) 		( gftp_request * request, 
					  const char *filename );
  int (*chdir)				( gftp_request * request, 
					  const char *directory );
  int (*rmdir)				( gftp_request * request, 
					  const char *directory );
  int (*rmfile)				( gftp_request * request, 
					  const char *filename );
  int (*mkdir)				( gftp_request * request, 
					  const char *directory );
  int (*rename)				( gftp_request * request, 
					  const char *oldname, 
					  const char *newname );
  int (*chmod)				( gftp_request * request, 
					  const char *filename, 
					  int mode );
  int (*set_file_time)			( gftp_request * request, 
					  const char *filename, 
					  time_t datettime );
  int (*site)				( gftp_request * request, 
					  const char *filename );
  int (*parse_url)			( gftp_request * request,
					  const char *url );
  void (*set_config_options)		( gftp_request * request );
  void (*swap_socks)			( gftp_request * dest,
					  gftp_request * source );

  GHashTable * local_options;
};


typedef struct gftp_transfer_tag gftp_transfer;

struct gftp_transfer_tag
{
  gftp_request * fromreq,
               * toreq;

  unsigned int transfer_direction : 1,
               cancel : 1,
               ready : 1,
               started : 1,
               done : 1,
               show : 1,
               stalled : 1,
               next_file : 1,
               skip_file : 1;

  struct timeval starttime,
                 lasttime;

  double kbs;
  
  GList * files,
        * curfle,
        * updfle;

  long numfiles,
       numdirs,
       current_file_number,
       current_file_retries;

  off_t curtrans,		/* Current transfered bytes for this file */
        curresumed,		/* Resumed bytes for this file */
        trans_bytes,		/* Amount of data transfered for entire 
				   transfer */
        total_bytes, 		/* Grand total bytes for whole transfer */
        resumed_bytes;		/* Grand total of resumed bytes for whole 
                                   transfer */

  void * fromwdata,
       * towdata;

  GStaticMutex statmutex,
               structmutex;

  void *user_data;
  void *clist;
};


typedef struct gftp_log_tag
{
  char *msg;
  gftp_logging_level type;
} gftp_log;


typedef struct supported_gftp_protocols_tag 
{
  char *name;					/* Description of protocol */
  void (*init) (gftp_request * request);	/* Init function */
  void (*register_options) (void);		/* Protocol options */
  char *url_prefix;				/* URL Prefix */
  int shown;					/* Whether this protocol is 
                                                   shown or not to the user in 
                                                   the protocol dropdown box */
} supported_gftp_protocols;


typedef struct gftp_bookmarks_tag gftp_bookmarks_var;

struct gftp_bookmarks_tag 
{
  char *path;                  /* Path */
  char *hostname,              /* Our actual internet hostname */
       *protocol,              /* Protocol we will connect through */
       *remote_dir,            /* Initial directory */
       *local_dir,             /* Init local directory */
       *user,                  /* Username to log in as */
       *pass,                  /* Our password */
       *acct;                  /* Account */

  unsigned int port,           /* The port we will connect to */
               isfolder : 1,   /* If this is set, then the children field can
                                  be non-NULL */
               save_password : 1; /* Save this password */
  gftp_bookmarks_var *children, /* The children of this node. */
                     *prev, 	/* The parent of this node */
                     *next; 	/* The next sibling of this node */
  gpointer cnode; 

  /* Site options */
  char *sftpserv_path;		/* Path to the sftp server */
};


typedef struct gftp_file_extensions_tag
{
   char *ext,                   /* The file extension to register */
        *filename,              /* The xpm file to display */
        *view_program,          /* The program used to view this file */
        *ascii_binary;          /* Is this a ASCII transfer or a BINARY transfer */
   int stlen;                   /* How long is the file extension. */
} gftp_file_extensions;


typedef struct gftp_color_tag
{
  gushort red,
          green,
          blue;
} gftp_color;


/* Note, these numbers must match up to the index number in config_file.c
   in the declaration of gftp_option_types */
typedef enum 
{
  gftp_option_type_text		= 0,
  gftp_option_type_textcombo	= 1,
  gftp_option_type_textcomboedt = 2,
  gftp_option_type_hidetext	= 3,
  gftp_option_type_int		= 4,
  gftp_option_type_checkbox	= 5,
  gftp_option_type_intcombo	= 6,
  gftp_option_type_float	= 7,
  gftp_option_type_color	= 8,
  gftp_option_type_notebook	= 9,
  gftp_option_type_newtable	= 10,
  gftp_option_type_label	= 11,
  gftp_option_type_subtree	= 12,
  gftp_option_type_table	= 13
} gftp_option_type_enum;


#define GFTP_PORT_GTK			(1 << 1)
#define GFTP_PORT_TEXT			(1 << 2)
#define GFTP_PORT_ALL			(GFTP_PORT_GTK | GFTP_PORT_TEXT)


typedef struct gftp_config_list_vars_tag
{
  char *key;
  void * (*read_func) (char *buf, int line);
  void (*write_func) (FILE *fd, void *data);
  GList * list;
  unsigned int num_items;
  char *header;
} gftp_config_list_vars;


#define GFTP_CVARS_FLAGS_DYNMEM		(1 << 1)


typedef struct gftp_config_vars_tag
{
  char *key,			/* variable name */
       *description;		/* How this field will show up in the dialog */
  int otype;			/* Type of option this is */
  void *value;
  void *listdata;		/* For options that have several different 
				   options, this is a list of all the options.
				   Each option_type that uses this will use this
				   field differently */
  int flags;			/* See GFTP_CVARS_FLAGS_* above */
  char *comment;                /* Comment to write out to the config file */
  int ports_shown;		/* What ports of gFTP is this option shown in */
  void *user_data;		/* Data that the GUI can store here (Widget in gtk+) */
} gftp_config_vars;


typedef struct gftp_option_type_tag
{
  int (*read_function) (char *str, gftp_config_vars * cv, int line);
  int (*write_function) (gftp_config_vars * cv, FILE * fd, int to_config_file);
  int (*ui_print_function) (char *label, void *ptr, void *user_data);
  void *user_data;
} gftp_option_type_var;


typedef struct gftp_textcomboedt_data_tag
{
  char *key,
       *description;
} gftp_textcomboedt_data;


typedef struct gftp_getline_buffer_tag
{
  char *buffer,
       *curpos;
  size_t max_bufsize,
         cur_bufsize;
} gftp_getline_buffer;


/* Global config options. These are defined in options.h */
extern GList * gftp_file_transfers, * gftp_file_transfer_logs,
             * gftp_options_list;
extern GHashTable * gftp_global_options_htable, * gftp_bookmarks_htable, 
                  * gftp_config_list_htable;
extern gftp_config_vars gftp_global_config_vars[];
extern supported_gftp_protocols gftp_protocols[];
extern gftp_bookmarks_var * gftp_bookmarks;
extern char gftp_version[];
extern FILE * gftp_logfd;

/* This is defined in config_file.c */

extern gftp_option_type_var gftp_option_types[];

/* cache.c */
int gftp_new_cache_entry 		( gftp_request * request );

int gftp_find_cache_entry 		( gftp_request * request );

void gftp_clear_cache_files 		( void );

void gftp_delete_cache_entry 		( gftp_request * request,
					  int ignore_directory );

/* config_file.c */
void gftp_add_bookmark 			( gftp_bookmarks_var * newentry );

void gftp_read_config_file 		( char *global_data_path );

void gftp_write_bookmarks_file 		( void );

void gftp_write_config_file 		( void );

GHashTable * build_bookmarks_hash_table	( gftp_bookmarks_var * entry );

void print_bookmarks 			( gftp_bookmarks_var * bookmarks );

void gftp_lookup_global_option 		( char * key, 
					  void *value );

void gftp_lookup_request_option 	( gftp_request * request, 
					  char * key, 
					  void *value );

void gftp_set_global_option 		( char * key, 
					  void *value );

void gftp_set_request_option 		( gftp_request * request, 
					  char * key, 
					  void *value );

void gftp_register_config_vars 		( gftp_config_vars *config_vars );

/* misc.c */
char *insert_commas 			( off_t number, 
					  char *dest_str, 
					  size_t dest_len );

char *alltrim 				( char *str );

char *expand_path 			( const char *src );

void remove_double_slashes 		( char *string );

void make_nonnull 			( char **str );

int copyfile				( char *source,
					  char *dest );

char *get_xpm_path 			( char *filename, 
					  int quit_on_err );

int gftp_match_filespec 		( char *filename, 
					  char *filespec );

int gftp_parse_command_line 		( int *argc,
					  char ***argv );

void gftp_usage 			( void );

gint string_hash_compare 		( gconstpointer path1,
					  gconstpointer path2 );

guint string_hash_function		( gconstpointer key );

void free_file_list			( GList * filelist );

void free_fdata				( gftp_file * fle );

gftp_file * copy_fdata 			( gftp_file * fle );

int compare_request 			( gftp_request * request1, 
					  gftp_request * request2, 
					  int compare_dirs );

gftp_transfer * gftp_tdata_new 		( void );

void free_tdata 			( gftp_transfer * tdata );

gftp_request * copy_request 		( gftp_request * req );

int ptym_open 				( char *pts_name,
					  size_t len );

int ptys_open 				( int fdm, 
					  char *pts_name );

int tty_raw 				( int fd );

GList * gftp_sort_filelist 		( GList * filelist, 
					  int column, 
					  int asds );

mode_t gftp_parse_attribs 		( char *attribs );

/* protocols.c */
#define GFTP_FTP_NUM				0
#define GFTP_HTTP_NUM				1
#define GFTP_LOCAL_NUM				2
#define GFTP_SSHV2_NUM				3
#define GFTP_BOOKMARK_NUM			4

#define GFTP_IS_CONNECTED(request)		((request) != NULL && \
                                                 ((request)->sockfd > 0 || \
                                                  (request)->cached || \
                                                  (request)->always_connected))


void rfc959_init 			( gftp_request * request );

void rfc959_register_module		( void );

int rfc959_get_next_file 		( gftp_request * request, 
					  gftp_file *fle, 
					  int fd );

void rfc2068_init 			( gftp_request * request );

void rfc2068_register_module		( void );

void local_init 			( gftp_request * request );

void local_register_module		( void );

void sshv2_init 			( gftp_request * request );

void sshv2_register_module		( void );

void bookmark_init 			( gftp_request * request );

void bookmark_register_module		( void );

gftp_request *gftp_request_new 		( void );

void gftp_request_destroy 		( gftp_request * request,
					  int free_request );

void gftp_file_destroy 			( gftp_file *file );

int gftp_connect 			( gftp_request * request );

void gftp_disconnect 			( gftp_request * request );

off_t gftp_get_file 			( gftp_request * request, 
					  const char *filename, 
					  int fd,
					  size_t startsize );

int gftp_put_file 			( gftp_request * request, 
					  const char *filename, 
					  int fd,
					  size_t startsize,
					  size_t totalsize );

long gftp_transfer_file 		( gftp_request *fromreq, 
					  const char *fromfile, 
					  int fromfd,
					  size_t fromsize, 
					  gftp_request *toreq, 
					  const char *tofile, 
					  int tofd,
					  size_t tosize );

ssize_t gftp_get_next_file_chunk 	( gftp_request * request, 
					  char *buf, 
					  size_t size );

ssize_t gftp_put_next_file_chunk 	( gftp_request * request, 
					  char *buf, 
					  size_t size );

int gftp_list_files 			( gftp_request * request );

int gftp_parse_bookmark 		( gftp_request * request, 
					  const char * bookmark );

int gftp_parse_url 			( gftp_request * request, 
					  const char *url );

int gftp_get_next_file 			( gftp_request * request, 
					  char *filespec,
					  gftp_file *fle );

int gftp_end_transfer 			( gftp_request * request );

int gftp_abort_transfer 		( gftp_request * request );

void gftp_set_hostname 			( gftp_request * request, 
					  const char *hostname );

void gftp_set_username 			( gftp_request * request, 
					  const char *username );

void gftp_set_password 			( gftp_request * request, 
					  const char *password );

void gftp_set_account 			( gftp_request * request, 
					  const char *account );

int gftp_set_directory 			( gftp_request * request, 
					  const char *directory );

void gftp_set_port 			( gftp_request * request, 
					  unsigned int port );

int gftp_remove_directory 		( gftp_request * request, 
					  const char *directory );

int gftp_remove_file 			( gftp_request * request, 
					  const char *file );

int gftp_make_directory 		( gftp_request * request, 
					  const char *directory );

int gftp_rename_file 			( gftp_request * request, 
					  const char *oldname, 
					  const char *newname );

int gftp_chmod 				( gftp_request * request, 
					  const char *file, 
					  int mode );

int gftp_set_file_time 			( gftp_request * request, 
					  const char *file, 
					  time_t datetime );

char gftp_site_cmd 			( gftp_request * request, 
					  const char *command );

off_t gftp_get_file_size 		( gftp_request * request, 
					  const char *filename );

void gftp_calc_kbs 			( gftp_transfer * tdata, 
				 	  ssize_t num_read );

time_t parse_time 			( char *str,
					  char **endpos );

int gftp_parse_ls 			( gftp_request * request,
					  const char *lsoutput, 
					  gftp_file *fle );

int gftp_get_all_subdirs 		( gftp_transfer * transfer, 
					  void (*update_func) 
						( gftp_transfer * transfer ));

int gftp_connect_server 		( gftp_request * request, 
					  char *service,
					  char *proxy_hostname,
					  int proxy_port );

#if !defined (HAVE_GETADDRINFO) || !defined (HAVE_GAI_STRERROR)

struct hostent *r_gethostbyname 	( const char *name, 
					  struct hostent *result_buf, 
					  int *h_errnop );

#endif

struct servent *r_getservbyname 	( const char *name, 
					  const char *proto,
					  struct servent *result_buf, 
					  int *h_errnop );

void gftp_set_config_options 		( gftp_request * request );

void print_file_list 			( GList * list );

ssize_t gftp_get_line 			( gftp_request * request, 
					  gftp_getline_buffer ** rbuf,
					  char * str, 
					  size_t len, 
					  int fd );

ssize_t gftp_read 			( gftp_request * request, 
					  void *ptr, 
					  size_t size, 
					  int fd );

ssize_t gftp_write 			( gftp_request * request, 
					  const char *ptr, 
					  size_t size, 
					  int fd );

ssize_t gftp_writefmt 			( gftp_request * request, 
					  int fd, 
					  const char *fmt, 
					  ... );

int gftp_set_sockblocking 		( gftp_request * request, 
					  int fd, 
					  int non_blocking );

void gftp_swap_socks 			( gftp_request * dest, 
					  gftp_request * source );

void gftp_calc_kbs 			( gftp_transfer * tdata, 
					  ssize_t num_read );

int gftp_get_transfer_status 		( gftp_transfer * tdata, 
					  ssize_t num_read );

#endif

