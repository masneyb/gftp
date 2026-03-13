/***********************************************************************************/
/*  gftp.h - include file for the whole ftp program                                */
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

#ifndef __GFTP_H
#define __GFTP_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if !defined (_LARGEFILE_SOURCE)
#define _LARGEFILE_SOURCE 1
#endif
#if !defined (_LARGEFILE64_SOURCE)
#define _LARGEFILE64_SOURCE 1
#endif
/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
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

#include "glib-compat.h"

#ifndef __GNU__
#include <limits.h>
#endif
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
#include <inttypes.h>
#include <string.h>
#include <strings.h>

//#define GFTP_DEBUG
#ifdef GFTP_DEBUG
#define DEBUG_PRINT_FUNC printf("%s: %s  (%d)\n", __FILE__, __func__, __LINE__);
#define DEBUG_MSG(x) printf("-- %s", (x));
#define DEBUG_TRACE(format, ...) printf(format, __VA_ARGS__);
#define DEBUG_PUTS(x) puts(x);
#else
#define DEBUG_PRINT_FUNC
#define DEBUG_MSG(x)
#define DEBUG_TRACE(x, ...)
#define DEBUG_PUTS(x)
#endif

/* Macro to mark unused parameters and suppress warnings */
#ifdef __GNUC__
#define UNUSED(x) x __attribute__((unused))
#else
#define UNUSED(x) x
#endif

#ifdef USE_SSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
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

/* Solaris needs this included for major()/minor() */
#ifdef HAVE_SYS_MKDEV_H
#include <sys/mkdev.h>
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

#ifdef WITH_DMALLOC
#undef g_malloc
#define g_malloc(size) _malloc_leap(__FILE__, __LINE__, size)
#undef g_malloc0
#define g_malloc0(size) _calloc_leap(__FILE__, __LINE__, 1, size)
#undef g_realloc
#define g_realloc(ptr, size) _realloc_leap(__FILE__, __LINE__, ptr, size)
#undef g_strdup
#define g_strdup(str) _strdup_leap(__FILE__, __LINE__, str)
#undef g_free
#define g_free(ptr) _free_leap(__FILE__, __LINE__, ptr)
#include <dmalloc.h>
#endif

#ifdef __GNUC__
# define GFTP_LOG_FUNCTION_ATTRIBUTES __attribute__((format(printf, 3, 4)))
#else
# define GFTP_LOG_FUNCTION_ATTRIBUTES
#endif

#if defined (_LARGEFILE_SOURCE) && !defined (__hppa__) && !defined (__hppa)
#define gftp_parse_file_size(str) strtoll(str, NULL, 10)
#else
#define gftp_parse_file_size(str) strtol(str, NULL, 10)
#endif

#ifdef __APPLE__
/* macOS: off_t is long long, not intmax_t */
#define GFTP_OFF_T_HEX_PRINTF_MOD  "%llx"
#define GFTP_OFF_T_INTL_PRINTF_MOD "%'lld"
#define GFTP_OFF_T_PRINTF_MOD      "%lld"
#define GFTP_OFF_T_11PRINTF_MOD    "%11lld"
#else
#define GFTP_OFF_T_HEX_PRINTF_MOD  "%jx"
#define GFTP_OFF_T_INTL_PRINTF_MOD "%'jd"
#define GFTP_OFF_T_PRINTF_MOD      "%jd"
#define GFTP_OFF_T_11PRINTF_MOD    "%11jd"
#endif

/* Error types */
#define GFTP_ERETRYABLE -1 /* Temporary failure. The GUI should wait briefly */
#define GFTP_EFATAL     -2 /* Fatal error */
#define GFTP_ERETRYABLE_NO_WAIT -3 /* Temporary failure. The GUI
                                    should not wait and should reconnect */
#define GFTP_ENOTRANS   -4 /* Custom error. This is returned when a FXP transfer is requested */
#define GFTP_ETIMEDOUT  -5 /* Connected timed out */
#define GFTP_ECANIGNORE -6 /* Error that can be ignored */

/* Some general settings */
extern char * BASE_CONF_DIR; // see misc.c: gftp_locale_init()
extern char * CONFIG_FILE;
extern char * BOOKMARKS_FILE;
extern char * LOG_FILE;
#define MAX_HIST_LEN    10
#define GFTP_URL_USAGE  "[[protocol://][user[:pass]@]site[:port][/directory]]"

typedef enum gftp_logging_level_tag 
{
  gftp_logging_send,
  gftp_logging_recv,
  gftp_logging_error,
  gftp_logging_misc,
  gftp_logging_misc_nolog /* Log to the screen, but don't log to disk */
} gftp_logging_level;

typedef struct gftp_file_tag gftp_file;

#define GFTP_TRANS_ACTION_OVERWRITE 1
#define GFTP_TRANS_ACTION_RESUME    2
#define GFTP_TRANS_ACTION_SKIP      3

#define GFTP_SORT_COL_FILE          1
#define GFTP_SORT_COL_SIZE          2
#define GFTP_SORT_COL_DATETIME      3
#define GFTP_SORT_COL_USER          4
#define GFTP_SORT_COL_GROUP         5
#define GFTP_SORT_COL_ATTRIBS       6

#define GFTP_IS_SPECIAL_DEVICE(mode) (S_ISBLK (mode) || S_ISCHR (mode))

struct gftp_file_tag 
{
  char *file;      /* @null@ Our filename */
  char *user;      /* User that owns it */
  char *group;     /* Group that owns it */
  char *destfile;  /* Full pathname to the destination for the  file transfer */
  int fd;  /* Already open fd for this file */
  /* FIXME - add fd_open function */

  time_t datetime; /* File date and time */
  off_t size;      /* Size of the file */
  off_t startsize; /* Size to start the transfer at */
  mode_t st_mode;  /* File attributes */

  dev_t st_dev; /* The device and associated inode. These */
  ino_t st_ino; /* two fields are used for detecting loops with symbolic links. */

  unsigned int selected : 1;  /* Is this file selected? */
  unsigned int was_sel : 1;   /* Was this file selected before  */
  unsigned int shown : 1;     /* Is this file shown? */
  unsigned int done_view : 1; /* View the file when done transferring? */
  unsigned int done_edit : 1; /* Edit the file when done transferring? */
  unsigned int done_rm : 1;   /* Remove the file when done */
  unsigned int transfer_done : 1;  /* Is current file transfer done? */
  unsigned int retry_transfer : 1; /* Is current file transfer done? */
  unsigned int exists_other_side : 1; /* The file exists on the other side
                                         during the file transfer */
  unsigned int filename_utf8_encoded : 1; /* Is the filename properly UTF8encoded? */

  char transfer_action; /* See the GFTP_TRANS_ACTION_* vars above */
  void *user_data;      /* @null@ */
  unsigned int free_user_data;
};


typedef enum 
{
  /* Note, these numbers must match up to the index number in config_file.c
     in the declaration of gftp_option_types */
  gftp_option_type_text         = 0,
  gftp_option_type_textcombo    = 1,
  gftp_option_type_textcomboedt = 2,
  gftp_option_type_hidetext     = 3,
  gftp_option_type_int          = 4,
  gftp_option_type_checkbox     = 5,
  gftp_option_type_intcombo     = 6,
  gftp_option_type_float        = 7,
  gftp_option_type_color        = 8,
  gftp_option_type_notebook     = 9
} gftp_option_type_enum;


#define GFTP_PORT_GTK  (1 << 1)
#define GFTP_PORT_TEXT (1 << 2)
#define GFTP_PORT_ALL  (GFTP_PORT_GTK | GFTP_PORT_TEXT)


typedef struct gftp_config_list_vars_tag
{
  char *key;
  void * (*read_func)  (char *buf, int line);
  void   (*write_func) (FILE *fd, void *data);
  GList * list;
  unsigned int num_items;
  char *header;
} gftp_config_list_vars;


#define GFTP_CVARS_FLAGS_DYNMEM         (1 << 1)
#define GFTP_CVARS_FLAGS_DYNLISTMEM     (1 << 2)
#define GFTP_CVARS_FLAGS_SHOW_BOOKMARK  (1 << 3)

typedef struct gftp_config_vars_tag
{
  char *key;         /* @null@ variable name */
  char *description; /* How this field will show up in the dialog */
  int otype;         /* Type of option this is */
  void *value;       /* @null@ */ 
  void *listdata; /* @null@ For options that have several different 
                     options, this is a list of all the options.
                     Each option_type that uses this will use this field differently */
  int flags;       /* See GFTP_CVARS_FLAGS_* above */
  char *comment;   /* @null@ Comment to write out to the config file */
  int ports_shown; /* What ports of gFTP is this option shown in */
  void *user_data; /* @null@ Data that the GUI can store here (Widget in gtk+) */
} gftp_config_vars;


typedef struct gftp_option_type_tag
{
  int  (*read_function)  (char *str, gftp_config_vars * cv, int line);
  int  (*write_function) (gftp_config_vars * cv, /*@out@*/ char *buf,
                          size_t buflen, int to_config_file);
  void (*copy_function)  (gftp_config_vars * cv, gftp_config_vars * dest_cv);
  int  (*compare_function)   (gftp_config_vars * cv1, gftp_config_vars * cv2);
  void *(*ui_print_function) (gftp_config_vars * cv, void *user_data, void *value);
  void (*ui_save_function)   (gftp_config_vars * cv, void *user_data);
  void (*ui_cancel_function) (gftp_config_vars * cv, void *user_data);
  void *user_data;
} gftp_option_type_var;


#define GFTP_TEXTCOMBOEDT_EDITABLE  (1 << 0)

typedef struct gftp_textcomboedt_data_tag
{
  char *description;  /*@null@*/
  char *text;         /*@null@*/
  int flags;
} gftp_textcomboedt_data;


typedef struct gftp_request_tag gftp_request;

typedef void (*gftp_logging_func) (gftp_logging_level level, 
                                   gftp_request * request, /*@null@*/
                                   const char *string, ...);

#define GFTP_ANONYMOUS_USER  "anonymous"
#define gftp_need_username(request) ((request)->need_username \
                                      && ((request)->username == NULL || *(request)->username == '\0'))
#define gftp_need_password(request) ((request)->need_password && (request)->username != NULL \
                                      && *(request)->username != '\0' && strcasecmp ((request)->username, GFTP_ANONYMOUS_USER) != 0 \
                                      && ((request)->password == NULL || *(request)->password == '\0'))


struct gftp_request_tag 
{
  int protonum;     /* Current number of the protocol this is  set to */
  char *hostname;   /* Hostname we will connect to */
  char *username;   /* Username for host*/
  char *password;   /* Password for host */
  char *account;    /* Account for host (FTP only) */
  char *directory;  /* Current working directory */
  char *url_prefix; /* URL Prefix (ex: ftp) */
  char *last_ftp_response;   /* Last response from server */
  char *last_dir_entry;      /* Last dir entry from server */
  size_t last_dir_entry_len; /* Length of last_dir_entry */

  unsigned int port; /* Port of remote site */

  int datafd;   /* Data connection */
  int cachefd;  /* For the directory cache */

  GIOChannel * chan;
  int wakeup_main_thread[2]; /* FD that gets written to by the threads to wakeup the parent */

  void *remote_addr;
  size_t remote_addr_len;
  int ai_family;
  int ai_socktype;
  unsigned int use_udp : 1; /* does this use UDP protocol? */

  unsigned int use_proxy : 1;
  unsigned int always_connected : 1;
  unsigned int need_hostport : 1;
  unsigned int need_username : 1;
  unsigned int need_password : 1;
  unsigned int use_cache : 1; /* Enable or disable the cache */
  unsigned int cached : 1;    /* Is this directory listing cached? */
  unsigned int cancel : 1;    /* If a signal is received, should we cancel this operation */
  unsigned int stopable : 1;
  unsigned int refreshing : 1;
  unsigned int use_local_encoding : 1;

  off_t gotbytes;
 
  void *protocol_data;
   
  gftp_logging_func logging_function;
  void *user_data;

  int (*init)                (gftp_request * request);
  void (*copy_param_options) (gftp_request * dest_request, 
                              gftp_request * src_request);

  ssize_t (*read_function)  (gftp_request * request,
                             void *ptr,       size_t size, int fd);
  ssize_t (*write_function) (gftp_request * request, 
                             const char *ptr, size_t size, int fd);

  void (*destroy)     (gftp_request * request);
  int (*connect)      (gftp_request * request);
  int (*post_connect) (gftp_request * request);
  void (*disconnect)  (gftp_request * request);

  off_t (*get_file) (gftp_request * request, 
                     const char *filename, 
                     off_t startsize);

  int (*put_file) (gftp_request * request, 
                   const char *filename, 
                   off_t startsize,
                   off_t totalsize);

  off_t (*transfer_file) (gftp_request * fromreq, 
                          const char *fromfile, 
                          off_t fromsize, 
                          gftp_request * toreq, 
                          const char *tofile, 
                          off_t tosize);

  ssize_t (*get_next_file_chunk) (gftp_request * request, char *buf, size_t size);
  ssize_t (*put_next_file_chunk) (gftp_request * request, char *buf, size_t size);

  int (*end_transfer)   (gftp_request * request);
  int (*abort_transfer) (gftp_request * request);

  int (*stat_filename) (gftp_request * request,
                        const char *filename,
                        mode_t * mode,
                        off_t * filesize);

  int (*list_files)    (gftp_request * request);
  int (*get_next_file) (gftp_request * request, gftp_file *fle, int fd);

  off_t (*get_file_size) (gftp_request * request, const char *filename);
  int (*chdir)  (gftp_request * request, const char *directory);
  int (*rmdir)  (gftp_request * request, const char *directory);
  int (*rmfile) (gftp_request * request, const char *filename);
  int (*mkdir)  (gftp_request * request, const char *directory);
  int (*rename) (gftp_request * request, const char *oldname, const char *newname);
  int (*chmod)  (gftp_request * request, const char *filename, mode_t mode);
  int (*set_file_time) (gftp_request * request,  const char *filename,  time_t datettime);
  int (*site) (gftp_request * request,  int specify_site, const char *filename);
  int (*parse_url) (gftp_request * request, const char *url);
  int (*set_config_options) (gftp_request * request);
  void (*swap_socks) (gftp_request * dest, gftp_request * source);

  gftp_config_vars * local_options_vars;
  int num_local_options_vars;
  GHashTable * local_options_hash;

  GIConv iconv_to; 
  GIConv iconv_from; 
  unsigned int iconv_from_initialized : 1;
  unsigned int iconv_to_initialized : 1;
  char *iconv_charset;
};



typedef struct gftp_transfer_tag
{
  gftp_request * fromreq;
  gftp_request * toreq;

  unsigned int cancel : 1;
  unsigned int ready : 1;
  unsigned int started : 1;
  unsigned int done : 1;
  unsigned int show : 1;
  unsigned int stalled : 1;
  unsigned int conn_error_no_timeout : 1;
  unsigned int next_file : 1;
  unsigned int skip_file : 1;

  struct timeval starttime;
  struct timeval lasttime;

  double kbs;
  
  GList * files;
  GList * curfle;
  GList * updfle;

  long numfiles;
  long numdirs;
  long current_file_number;
  long current_file_retries;

  off_t curtrans;      /* Current transferred bytes for this file */
  off_t tot_file_trans;/* Total number of bytes in the file being transferred */
  off_t curresumed;    /* Resumed bytes for this file */
  off_t trans_bytes;   /* Amount of data transferred for entire  transfer */
  off_t total_bytes;   /* Grand total bytes for whole transfer */
  off_t resumed_bytes; /* Grand total of resumed bytes for whole  transfer */

  void * fromwdata;
  void * towdata;

  GMutex statmutex;
  GMutex structmutex;

  void *user_data;
  void *thread_id;
  void *clist;
} gftp_transfer;


typedef struct gftp_log_tag
{
  char *msg;
  gftp_logging_level type;
} gftp_log;


typedef struct supported_gftp_protocols_tag 
{
  char *name;  /* Description of protocol @null@*/
  int (*init) (gftp_request * request); /* Init function @null@*/
  void (*register_options) (void);      /* Protocol options @null@*/
  char *url_prefix;          /* URL Prefix @null@*/
  unsigned int default_port; /* Default port */
  unsigned int shown : 1;    /* Whether this protocol is 
                                shown or not to the user in the protocol dropdown box */
  unsigned int use_threads : 1; /* Whether or not operations in this protocol should use threads */
} supported_gftp_protocols;


typedef struct gftp_bookmarks_tag gftp_bookmarks_var;

struct gftp_bookmarks_tag 
{
  char *path;       /* Path */
  char *hostname;   /* Our actual internet hostname */
  char *protocol;   /* Protocol we will connect through */
  char *remote_dir; /* Initial directory */
  char *local_dir;  /* Init local directory */
  char *user;       /* Username to log in as */
  char *pass;       /* Our password */
  char *acct;       /* Account */

  unsigned int port;         /* The port we will connect to */
  unsigned int isfolder : 1; /* If this is set, then the children field can be non-NULL */
  unsigned int save_password : 1; /* Save this password */
  gftp_bookmarks_var * children; /* The children of this node. */
  gftp_bookmarks_var * parent;   /* The parent of this node */
  gftp_bookmarks_var * next;     /* The next sibling of this node */

  gftp_config_vars * local_options_vars;
  int num_local_options_vars;
  GHashTable * local_options_hash;
};


typedef struct gftp_file_extensions_tag
{
   char *ext;            /* The file extension to register */
   char *filename;       /* The xpm file to display */
   char *view_program;   /* The program used to view this file */
   char *ascii_binary;   /* Is this a ASCII transfer or a BINARY transfer */
   unsigned int stlen;   /* How long is the file extension. */
} gftp_file_extensions;


typedef struct gftp_color_tag
{
  gushort red;
  gushort green;
  gushort blue;
} gftp_color;


typedef struct gftp_getline_buffer_tag
{
  char *buffer;
  char *curpos;
  size_t max_bufsize;
  size_t cur_bufsize;
  unsigned int eof : 1;
} gftp_getline_buffer;


/* Global config options. These are defined in options.h */
extern GList * gftp_file_transfers;     /*@null@*/
extern GList * gftp_file_transfer_logs; /*@null@*/
extern GList * gftp_options_list;       /*@null@*/
extern GHashTable * gftp_global_options_htable; /*@null@*/
extern GHashTable * gftp_bookmarks_htable;      /*@null@*/
extern GHashTable * gftp_config_list_htable;    /*@null@*/
extern gftp_bookmarks_var * gftp_bookmarks;     /*@null@*/
extern FILE * gftp_logfd;                       /*@null@*/
extern gftp_config_vars gftp_global_config_vars[];
extern supported_gftp_protocols gftp_protocols[];
extern char gftp_version[];
extern int gftp_configuration_changed;
/* This is defined in config_file.c */
extern gftp_option_type_var gftp_option_types[];


/* cache.c */
void gftp_generate_cache_description (gftp_request * request, 
                                      /*@out@*/ char *description,
                                      size_t len, 
                                      int ignore_directory);

int gftp_new_cache_entry    (gftp_request * request);
int gftp_find_cache_entry   (gftp_request * request);
void gftp_clear_cache_files (void);

void gftp_delete_cache_entry  (gftp_request * request,
                               char *descr,
                               int ignore_directory);

/* charset-conv.c */
char * gftp_string_to_utf8   (gftp_request * request, const char *str, size_t *dest_len);
char * gftp_string_from_utf8 (gftp_request * request, int force_local,
                              const char *str, size_t *dest_len);
char * gftp_filename_to_utf8 (gftp_request * request, const char *str, size_t *dest_len);
char * gftp_filename_from_utf8 (gftp_request * request,  const char *str, size_t *dest_len);

/* config_file.c */
int gftp_config_parse_args (char *str, int numargs, int lineno, 
                            /*@out@*/ char **first, ...);
void gftp_add_bookmark         (gftp_bookmarks_var * newentry);
void gftp_read_config_file     (char *global_data_path);
void gftp_write_bookmarks_file (void);
void gftp_write_config_file    (void);

GHashTable * build_bookmarks_hash_table (gftp_bookmarks_var * entry);

void print_bookmarks (gftp_bookmarks_var * bookmarks);
void gftp_lookup_global_option (const char * key, /*@out@*/ void *value);

void gftp_lookup_request_option (gftp_request * request, 
                                 const char * key, 
                                 /*@out@*/ void *value);

void gftp_lookup_bookmark_option (gftp_bookmarks_var * bm, 
                                  const char * key,
                                  void *value);

void gftp_set_global_option (const char * key, const void *value);

void gftp_set_request_option (gftp_request * request, 
                              const char * key, 
                              const void *value);

void gftp_set_bookmark_option (gftp_bookmarks_var * bm,
                               const char * key,
                               const void *value);

void gftp_register_config_vars (gftp_config_vars *config_vars);

void gftp_copy_local_options (gftp_config_vars ** new_options_vars, 
                              GHashTable ** new_options_hash,
                              int *new_num_local_options_vars,
                              gftp_config_vars * orig_options,
                              int num_local_options_vars);

void gftp_config_free_options (gftp_config_vars * options_vars,
                               GHashTable * options_hash,
                               int num_options_vars);

void gftp_bookmarks_destroy (gftp_bookmarks_var * bookmarks);

/* misc.c */
char *insert_commas (off_t number, char *dest_str, size_t dest_len);
char *gftp_expand_path (gftp_request * request, const char *src);
int gftp_match_filespec (gftp_request * request, const char *filename, const char *filespec);
int gftp_parse_command_line (int *argc, char ***argv);
void gftp_usage (void);

gint string_hash_compare   (gconstpointer path1, gconstpointer path2);
guint string_hash_function (gconstpointer key);
gint uint_hash_compare     (gconstpointer path1, gconstpointer path2);
guint uint_hash_function   (gconstpointer key);

void free_file_list (GList * filelist);
gftp_file * copy_fdata (gftp_file * fle);

int compare_request (gftp_request * request1, 
                     gftp_request * request2, 
                     int compare_dirs);

gftp_transfer * gftp_tdata_new (void);
void free_tdata (gftp_transfer * tdata);
gftp_request * gftp_copy_request (gftp_request * req);

GList * gftp_sort_filelist (GList * filelist, int column, int asds);

char * gftp_gen_ls_string (gftp_request * request,
                           gftp_file * fle,
                           char *file_prefixstr,
                           char *file_suffixstr);

void gftp_free_bookmark (gftp_bookmarks_var * entry, int free_node);
void gftp_shutdown (void);
char * gftp_build_path (gftp_request * request, const char *first_element, ...);
void gftp_locale_init (void);

char * gftp_scramble_password   (const char *password);
char * gftp_descramble_password (const char *password);

int gftp_get_transfer_action (gftp_request * request, gftp_file * fle);
char * gftp_get_share_dir (void);
char * gftp_get_doc_dir (void);
void gftp_format_file_size (off_t bytes, char *out_buffer, size_t buffer_size);
char * str_get_next_line_pointer (char * buf, char **pos, int *line_ending);

/* parse-dir-listing.c */
time_t parse_time (char *str, char **endpos);

/* protocols */
// see options.h: supported_gftp_protocols gftp_protocols[]
//                any discrepancy = segfault
#define GFTP_PROTOCOL_FTP         0
#define GFTP_PROTOCOL_FTPS        1
#define GFTP_PROTOCOL_FTPSi       2
#define GFTP_PROTOCOL_LOCALFS     3
#define GFTP_PROTOCOL_SSH2        4
#define GFTP_PROTOCOL_BOOKMARK    5
#define GFTP_PROTOCOL_HTTP        6
#define GFTP_PROTOCOL_HTTPS       7

#define GFTP_IS_CONNECTED(request) ((request) != NULL && \
                                             ((request)->datafd > 0 || \
                                             (request)->cached || \
                                             (request)->always_connected))
/* protocol_ftp.c */
int ftp_init (gftp_request * request);
void ftp_register_module (void);
int ftp_get_next_file (gftp_request * request, gftp_file *fle, int fd);
int ftp_connect       (gftp_request * request);

/* protocol_ftps.c */
int ftps_init (gftp_request * request);
void ftps_register_module  (void);
int ftpsi_init (gftp_request * request);
void ftpsi_register_module (void);

/* protocol_localfs.c */
int localfs_init (gftp_request * request);
void localfs_register_module (void);

/* protocol_http.c */
int http_init              (gftp_request * request);
void http_register_module  (void);
int https_init             (gftp_request * request);
void https_register_module (void);

/* sshv2.c */
int sshv2_init (gftp_request * request);
void sshv2_register_module (void);

/* sslcommon.c */
void ssl_register_module (void);

/* protocols_bookmark.c */
int bookmark_init (gftp_request * request);
void bookmark_register_module (void);

/* protocols.c */
gftp_request *gftp_request_new (void);
void gftp_request_destroy    (gftp_request * request, int free_request);
void gftp_copy_param_options (gftp_request * dest_request, gftp_request * src_request);
void gftp_file_destroy       (gftp_file *file, int free_it);
int gftp_connect             (gftp_request * request);
void gftp_disconnect         (gftp_request * request);

off_t gftp_get_file (gftp_request * request, 
                     const char *filename, 
                     off_t startsize);

int gftp_put_file (gftp_request * request, 
                   const char *filename, 
                   off_t startsize,
                   off_t totalsize);

off_t gftp_transfer_file (gftp_request *fromreq, 
                          const char *fromfile, 
                          off_t fromsize, 
                          gftp_request *toreq, 
                          const char *tofile, 
                          off_t tosize);

ssize_t gftp_get_next_file_chunk (gftp_request * request, char *buf, size_t size);
ssize_t gftp_put_next_file_chunk (gftp_request * request, char *buf, size_t size);

int gftp_list_files (gftp_request * request);

int gftp_parse_bookmark (gftp_request * request, 
                         gftp_request * local_request,
                         const char * bookmark,
                         int *refresh_local);

int gftp_parse_url (gftp_request * request, const char *url);

int gftp_get_next_file (gftp_request * request, 
                        const char *filespec,
                        gftp_file *fle);

int gftp_end_transfer   (gftp_request * request);
int gftp_abort_transfer (gftp_request * request);

int gftp_stat_filename (gftp_request * request,
                        const char *filename,
                        mode_t * mode,
                        off_t * filesize);

void gftp_set_hostname (gftp_request * request, const char *hostname);
void gftp_set_username (gftp_request * request, const char *username);
void gftp_set_password (gftp_request * request, const char *password);
void gftp_set_account  (gftp_request * request, const char *account);
void gftp_set_port     (gftp_request * request, unsigned int port);
int gftp_set_directory (gftp_request * request, const char *directory);
int gftp_remove_directory (gftp_request * request, const char *directory);
int gftp_remove_file    (gftp_request * request, const char *file);
int gftp_make_directory (gftp_request * request, const char *directory);
int gftp_rename_file    (gftp_request * request, const char *oldname, const char *newname);
int gftp_chmod (gftp_request * request, const char *file,  mode_t mode);

int gftp_set_file_time  (gftp_request * request, 
                         const char *file,
                         time_t datetime);

int gftp_site_cmd (gftp_request * request,
                   int specify_site,
                   const char *command);

off_t gftp_get_file_size (gftp_request * request, const char *filename);
void gftp_calc_kbs (gftp_transfer * tdata, ssize_t num_read);

int gftp_get_all_subdirs (gftp_transfer * transfer,
                          void (*update_func) (gftp_transfer * transfer));

int gftp_set_config_options (gftp_request * request);

void print_file_list (GList * list);
void gftp_swap_socks (gftp_request * dest, gftp_request * source);
void gftp_calc_kbs   (gftp_transfer * tdata, ssize_t num_read);
int gftp_get_transfer_status (gftp_transfer * tdata, ssize_t num_read);

int gftp_fd_open (gftp_request * request, 
                  const char *pathname, 
                  int flags,
                  mode_t perms);

void gftp_setup_startup_directory (gftp_request * request, const char *option_name);
unsigned int gftp_protocol_default_port (gftp_request * request);

/* pty.c */
char * gftp_get_pty_impl (void);
pid_t gftp_exec (gftp_request * request, int *fdm, int *ptymfd, char **args);
char * gftp_convert_attributes_from_mode_t (mode_t mode);
mode_t gftp_convert_attributes_to_mode_t   (char *attribs);


#ifdef USE_SSL
/* sslcommon.c */
int gftp_ssl_startup          (gftp_request * request);
int gftp_ssl_session_setup    (gftp_request * request);
int gftp_ssl_session_setup_ex (gftp_request * request, int fd);
void gftp_ssl_free            (gftp_request * request);
ssize_t gftp_ssl_read  (gftp_request * request, void       *ptr, size_t size, int fd);
ssize_t gftp_ssl_write (gftp_request * request, const char *ptr, size_t size, int fd);
void gftp_ssl_session_close_ex (gftp_request * request, int fd);
void gftp_ssl_session_close (gftp_request * request);
#endif /* USE_SSL */

/* UI dependent functions that must be implemented */
int gftpui_protocol_ask_yes_no (gftp_request * request,
                                char *title,
                                char *question);

char *gftpui_protocol_ask_user_input (gftp_request * request,
                                      char *title,
                                      char *question,
                                      int shown);

void gftpui_protocol_update_timeout (gftp_request * request);

/* socket-connect.c */
int  w_sockaddr_get_port   (struct sockaddr * saddr);
void w_sockaddr_get_ip_str (struct sockaddr * saddr, char * outbuf, int size);
void * w_sockaddr_get_addr (struct sockaddr * saddr);
socklen_t w_sockaddr_get_size (struct sockaddr * saddr);
void w_sockaddr_reset (struct sockaddr * saddr);
void w_sockaddr_set_port (struct sockaddr * saddr, int port);

int gftp_connect_server (gftp_request * request, 
                         char *service,
                         char *proxy_hostname,
                         unsigned int proxy_port);

int gftp_data_connection_new (gftp_request * request);
int gftp_data_connection_new_listen (gftp_request * request);
int gftp_accept_connection (gftp_request * request, int * fd);

/* sockutils.c */
ssize_t gftp_get_line (gftp_request * request, 
                       /*@out@*/ gftp_getline_buffer ** rbuf,
                       /*@out@*/ char * str, 
                       size_t len, 
                       int fd);

void gftp_free_getline_buffer (gftp_getline_buffer ** rbuf);

ssize_t gftp_fd_read  (gftp_request * request, void *ptr, size_t size, int fd);
ssize_t gftp_fd_write (gftp_request * request, const char *ptr, size_t size, int fd);
ssize_t gftp_writefmt (gftp_request * request, int fd, const char *fmt, ...);

int gftp_fd_get_sockblocking (gftp_request * request, int fd);
int gftp_fd_set_sockblocking (gftp_request * request, int fd, int non_blocking);

struct servent * r_getservbyname (const char *name,
                                  const char *proto,
                                  struct servent *result_buf,
                                  int *h_errnop);

#endif /* __GFTP_H */
