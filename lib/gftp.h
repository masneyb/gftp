/*****************************************************************************/
/*  gftp.h - include file for the whole ftp program                          */
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

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

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
  time_t datetime;		/* File date and time */
  off_t size,			/* Size of the file */
        startsize;		/* Size to start the transfer at */
  unsigned int isdir : 1,	/* File type */
               isexe : 1,
               islink : 1,
               isblock : 1,
               ischar : 1,
               issocket : 1,
               isfifo : 1,
               ascii : 1, 	/* Transfer in ASCII mode */
               selected : 1,	/* Is this file selected? */
               was_sel : 1,	/* Was this file selected before  */
               shown : 1,	/* Is this file shown? */
               done_view : 1,	/* View the file when done transfering? */
               done_edit : 1,	/* Edit the file when done transfering? */
               done_rm : 1,	/* Remove the file when done */
               transfer_done : 1, /* Is current file transfer done? */
               is_fd : 1;	/* Is this a file descriptor? */
  char transfer_action;		/* See the GFTP_TRANS_ACTION_* vars above */
  void *node;			/* Pointer to the node for the gui */
  FILE * fd;
};


typedef struct gftp_proxy_hosts_tag 
{
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
       *proxy_config,		/* Proxy configuration */
       *proxy_hostname,		/* Proxy hostname */
       *proxy_username,		/* Proxy username */
       *proxy_password,		/* Proxy password */
       *proxy_account,		/* Proxy account (FTP only) */
       *url_prefix,		/* URL Prefix (ex: ftp) */
       *protocol_name,		/* Protocol description */
       *last_ftp_response,	/* Last response from server */
       *last_dir_entry;		/* Last dir entry from server */
  size_t last_dir_entry_len;	/* Length of last_dir_entry */

  unsigned int port,		/* Port of remote site */
               proxy_port;	/* Port of the proxy server */

  FILE *sockfd,			/* Control connection (read) */
       *sockfd_write,		/* Control connection (write) */
       *datafd,			/* Data connection */
       *cachefd;		/* For the directory cache */
  int wakeup_main_thread[2];	/* FD that gets written to by the threads
                                   to wakeup the parent */
        
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo *hostp;	/* Remote host we are connected to */
#else
  struct hostent host, *hostp;  /* Remote host we are connected to */
#endif

  int data_type;		/* ASCII or BINARY (FTP only) */
  unsigned int use_proxy : 1,           /* Go out of proxy server */
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
  long (*get_file) 			( gftp_request * request, 
					  const char *filename, 
					  FILE * fd,
					  off_t startsize );
  int (*put_file) 			( gftp_request * request, 
					  const char *filename, 
					  FILE * fd,
					  off_t startsize,
					  off_t totalsize );
  long (*transfer_file) 		( gftp_request * fromreq, 
					  const char *fromfile, 
					  off_t fromsize, 
					  gftp_request * toreq, 
					  const char *tofile, 
					  off_t tosize );
  size_t (*get_next_file_chunk) 	( gftp_request * request, 
					  char *buf, 
					  size_t size );
  size_t (*put_next_file_chunk) 	( gftp_request * request, 
					  char *buf, 
					  size_t size );
  int (*end_transfer) 			( gftp_request * request );
  int (*abort_transfer) 		( gftp_request * request );
  int (*list_files) 			( gftp_request * request );
  int (*get_next_file)			( gftp_request * request, 
					  gftp_file *fle, 
					  FILE *fd );
  int (*set_data_type)			( gftp_request * request, 
					  int data_type );
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

  /* Options */
  gftp_transfer_type transfer_type;	/* Passive or non-passive (FTP only) */
  int network_timeout,
      retries,
      sleep_time,
      passive_transfer;
  float maxkbs;
  char *sftpserv_path;
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

  void *statmutex,
       *structmutex;
  void *node;
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
  char *url_prefix;				/* URL Prefix */
  int shown;					/* Whether this protocol is shown or not to the user
                                                   in the protocol dropdown box */
} supported_gftp_protocols;


typedef struct gftp_bookmarks_tag gftp_bookmarks;

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
  gftp_bookmarks *children, 	/* The children of this node. */
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


typedef struct gftp_config_vars_tag
{
  char *key,			/* variable name */
       *description;		/* How this field will show up in the dialog */
  gpointer var;			/* Pointer to our variable */
  int type;			/* See defines below */
  char *comment;                /* Comment to write out to the config file */
  gpointer widget;
  int ports_shown;		/* What ports of gFTP is this option shown in */
} gftp_config_vars;

#define CONFIG_INTTEXT                  1
#define CONFIG_FLOATTEXT                2
#define CONFIG_CHECKBOX                 3       
#define CONFIG_LABEL                    4
#define CONFIG_NOTEBOOK                 5
#define CONFIG_HIDEINT                  6
#define CONFIG_TABLE			7
#define CONFIG_CHARTEXT                 8
#define CONFIG_COMBO			9
#define CONFIG_TEXT			10
#define CONFIG_COLOR			11
#define CONFIG_UINTTEXT                 12
#define CONFIG_CHARPASS			13

#define GFTP_PORT_GTK			(1 << 1)
#define GFTP_PORT_TEXT			(1 << 2)
#define GFTP_PORT_ALL			(GFTP_PORT_GTK | GFTP_PORT_TEXT)

typedef struct gftp_proxy_type_tag
{
  char *key,
       *description;
} gftp_proxy_type;

#define GFTP_CUSTOM_PROXY_NUM        8

/* Global config options */
extern supported_gftp_protocols gftp_protocols[];
extern char version[], *emailaddr, *edit_program, *view_program, 
            *firewall_host, *firewall_username, *firewall_password, 
            *firewall_account, *proxy_config, *http_proxy_host, 
            *http_proxy_username, *http_proxy_password, 
            *startup_directory, *ssh_prog_name, *ssh_extra_params, 
            **ssh_extra_params_list, *default_protocol, *ssh1_sftp_path, 
            *ssh2_sftp_path;
extern int num_ssh_extra_params;
extern FILE * logfd;
extern double maxkbs;
extern GList * proxy_hosts, * registered_exts, * viewedit_processes, 
             * file_transfers, * file_transfer_logs;
extern gftp_bookmarks * bookmarks;
extern int do_one_transfer_at_a_time, start_file_transfers, 
           transfer_in_progress, passive_transfer, sort_dirs_first, 
           use_default_dl_types, show_hidden_files, refresh_files, 
           listbox_local_width, listbox_remote_width, listbox_file_height, 
           transfer_height, log_height, retries, sleep_time, network_timeout, 
           use_http11, listbox_dblclick_action, file_trans_column, 
           local_columns[6], remote_columns[6], resolve_symlinks, 
           firewall_port, http_proxy_port, overwrite_by_default, 
           append_file_transfers, enable_old_ssh, ssh_need_userpass, 
           ssh_use_askpass, sshv2_use_sftp_subsys, local_sortcol, 
           local_sortasds, remote_sortcol, remote_sortasds;
extern guint max_log_window_size;
extern GHashTable * bookmarks_htable, * config_htable;
extern GList * localhistory, * remotehistory, * host_history, * port_history, 
             * user_history;
extern unsigned int host_len, port_len, user_len, localhistlen, remotehistlen;
extern volatile sig_atomic_t viewedit_process_done;
extern gftp_config_vars config_file_vars[];
extern gftp_proxy_type proxy_type[];
extern gftp_color send_color, recv_color, error_color, misc_color;

/* cache.c */
FILE * gftp_new_cache_entry 		( gftp_request * request );

FILE * gftp_find_cache_entry 		( gftp_request * request );

void gftp_clear_cache_files 		( void );

void gftp_delete_cache_entry 		( gftp_request * request,
					  int ignore_directory );

/* config_file.c */
void gftp_read_config_file 		( char **argv,
					  int get_xpms );

void gftp_read_bookmarks 		( void );

void add_to_bookmark                    ( gftp_bookmarks *newentry );

void gftp_write_config_file 		( void );

void gftp_write_bookmarks_file 		( void );

GHashTable * build_bookmarks_hash_table	( gftp_bookmarks * entry );

void print_bookmarks 			( gftp_bookmarks * bookmarks );

/* misc.c */
char *insert_commas 			( off_t number, 
					  char *dest_str, 
					  size_t dest_len );

long file_countlf 			( int filefd, 
					  long endpos );

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

void swap_socks 			( gftp_request * dest, 
					  gftp_request * source );

int compare_request 			( gftp_request * request1, 
					  gftp_request * request2, 
					  int compare_dirs );

void free_tdata 			( gftp_transfer * tdata );

gftp_request * copy_request 		( gftp_request * req );

int ptym_open 				( char *pts_name );

int ptys_open 				( int fdm, 
					  char *pts_name );

int tty_raw 				( int fd );

char **make_ssh_exec_args 		( gftp_request * request, 
					  char *execname,
					  int use_sftp_subsys,
					  char *portstring );

char * ssh_start_login_sequence 	( gftp_request * request, 
					  int fd );

#ifdef G_HAVE_GINT64
gint64 hton64				( gint64 val );
#endif

GList * gftp_sort_filelist 		( GList * filelist, 
					  int column, 
					  int asds );

/* protocols.c */
#define GFTP_CONNECTED(request)			(request->sockfd != NULL)
#define GFTP_GET_HOSTNAME(request)		(request->hostname)
#define GFTP_GET_USERNAME(request)		(request->username)
#define GFTP_GET_PASSWORD(request)		(request->password)
#define GFTP_GET_ACCOUNT(request)		(request->account)
#define GFTP_GET_DIRECTORY(request)		(request->directory)
#define GFTP_GET_PORT(request)			(request->port)
#define GFTP_GET_PROXY_CONFIG(request)		(request->proxy_config)
#define GFTP_GET_PROXY_HOSTNAME(request)	(request->proxy_hostname)
#define GFTP_GET_PROXY_USERNAME(request)	(request->proxy_username)
#define GFTP_GET_PROXY_PASSWORD(request)	(request->proxy_password)
#define GFTP_GET_PROXY_ACCOUNT(request)		(request->proxy_account)
#define GFTP_GET_PROXY_PORT(request)		(request->proxy_port)
#define GFTP_GET_URL_PREFIX(request)		(request->url_prefix)
#define GFTP_GET_PROTOCOL_NAME(request)		(request->protocol_name)
#define GFTP_GET_LAST_RESPONSE(request)		(request->last_ftp_response)
#define GFTP_GET_LAST_DIRENT(request)		(request->last_dir_entry)
#define GFTP_GET_LAST_DIRENT_LEN(request)	(request->last_dir_entry_len)
#define GFTP_GET_CONTROL_FD(request)		(request->sockfd)
#define GFTP_GET_DATA_FD(request)		(request->datafd)
#define GFTP_GET_DATA_TYPE(request)		(request->data_type)
#define GFTP_GET_TRANSFER_TYPE(request)		(request->transfer_type)
#define GFTP_SET_TRANSFER_TYPE(request,val) 	(request->transfer_type = (val))
#define GFTP_GET_LOGGING(request)		(request->logging)
#define GFTP_SET_LOGGING(request, val)		(request->logging = (val))
#define GFTP_UNSAFE_SYMLINKS(request)		(request->unsafe_symlinks)
#define GFTP_FTP_NUM				0
#define GFTP_HTTP_NUM				1
#define GFTP_LOCAL_NUM				2
#define GFTP_SSHV2_NUM				3
#define GFTP_BOOKMARK_NUM			4
#define GFTP_SSH_NUM				5
#define GFTP_TYPE_BINARY			1
#define GFTP_TYPE_ASCII				2   
#define GFTP_IS_CONNECTED(request)		((request) != NULL && \
                                                 ((request)->sockfd != NULL || \
                                                  (request)->cached))


void rfc959_init 			( gftp_request * request );

int rfc959_get_next_file 		( gftp_request * request, 
					  gftp_file *fle, 
					  FILE *fd );

void rfc2068_init 			( gftp_request * request );

void local_init 			( gftp_request * request );

void ssh_init 				( gftp_request * request );

void sshv2_init 			( gftp_request * request );

void bookmark_init 			( gftp_request * request );

gftp_request *gftp_request_new 		( void );

void gftp_request_destroy 		( gftp_request * request );

void gftp_file_destroy 			( gftp_file *file );

int gftp_connect 			( gftp_request * request );

void gftp_disconnect 			( gftp_request * request );

size_t gftp_get_file 			( gftp_request * request, 
					  const char *filename, 
					  FILE * fd,
					  size_t startsize );

int gftp_put_file 			( gftp_request * request, 
					  const char *filename, 
					  FILE * fd,
					  size_t startsize,
					  size_t totalsize );

long gftp_transfer_file 		( gftp_request *fromreq, 
					  const char *fromfile, 
					  FILE * fromfd,
					  size_t fromsize, 
					  gftp_request *toreq, 
					  const char *tofile, 
					  FILE * tofd,
					  size_t tosize );

size_t gftp_get_next_file_chunk 	( gftp_request * request, 
					  char *buf, 
					  size_t size );

size_t gftp_put_next_file_chunk 	( gftp_request * request, 
					  char *buf, 
					  size_t size );

int gftp_list_files 			( gftp_request * request );

int gftp_parse_url 			( gftp_request * request, 
					  const char *url );

int gftp_get_next_file 			( gftp_request * request, 
					  char *filespec,
					  gftp_file *fle );

int gftp_end_transfer 			( gftp_request * request );

int gftp_abort_transfer 		( gftp_request * request );

int gftp_read_response 			( gftp_request * request );

int gftp_set_data_type 			( gftp_request * request, 
					  int data_type );

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

void gftp_set_proxy_hostname 		( gftp_request * request, 
					  const char *hostname );

void gftp_set_proxy_username 		( gftp_request * request, 
					  const char *username );

void gftp_set_proxy_password 		( gftp_request * request, 
					  const char *password );

void gftp_set_proxy_account 		( gftp_request * request, 
					  const char *account );

void gftp_set_proxy_port 		( gftp_request * request, 
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

void gftp_set_proxy_config 		( gftp_request * request, 
					  const char *proxy_config );

long gftp_get_file_size 		( gftp_request * request, 
					  const char *filename );

int gftp_need_proxy 			( gftp_request * request,
					  char *service );

char *gftp_convert_ascii 		( char *buf, 
					  ssize_t *len, 
					  int direction );

void gftp_calc_kbs 			( gftp_transfer * tdata, 
				 	  ssize_t num_read );

int gftp_parse_ls 			( const char *lsoutput, 
					  gftp_file *fle );

int gftp_get_all_subdirs 		( gftp_transfer * transfer, 
					  void (*update_func) 
						( gftp_transfer * transfer ));

int gftp_get_file_transfer_mode 	( char *filename,
					  int def );

int gftp_connect_server 		( gftp_request * request, 
					  char *service );

void gftp_set_sftpserv_path		( gftp_request * request,
					  char *path );

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)

int get_port 				( struct addrinfo *addr );

#else

struct hostent *r_gethostbyname 	( const char *name, 
					  struct hostent *result_buf, 
					  int *h_errnop );

struct servent *r_getservbyname 	( const char *name, 
					  const char *proto,
					  struct servent *result_buf, 
					  int *h_errnop );
#endif

void gftp_set_config_options 		( gftp_request * request );

void print_file_list 			( GList * list );

char *gftp_fgets 			( gftp_request * request, 
					  char * str, 
					  size_t len, 
					  FILE * fd );

size_t gftp_fwrite 			( gftp_request * request, 
					  const void *ptr, 
					  size_t size, 
					  FILE * fd );

#endif

