/*****************************************************************************/
/*  gftpui.h - UI related functions for gFTP                                 */
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

#ifndef __GFTPUI_H
#define __GFTPUI_H

#include "../../lib/gftp.h"

typedef struct _gftpui_callback_data gftpui_callback_data;

struct _gftpui_callback_data
{
  gftp_request * request;
  void *uidata;
  char *input_string,
       *source_string;
  GList * files;
  int retries;
  int (*run_function) (gftpui_callback_data * cdata);
};


typedef enum _gftpui_common_request_type
{
  gftpui_common_request_none,
  gftpui_common_request_local,
  gftpui_common_request_remote
} gftpui_common_request_type;


typedef struct _gftpui_common_methods
{
  char *command;
  int minlen;
  int (*func)(void *uidata, gftp_request * request, char *command);
  gftpui_common_request_type reqtype;
  char *cmd_description;
  int (*subhelp_func) (char *topic);
} gftpui_common_methods;

typedef struct _gftpui_common_curtrans_data
{
  gftp_transfer * transfer;
  GList * curfle;
} gftpui_common_curtrans_data;

#define gftpui_common_use_threads(request)	(gftp_protocols[(request)->protonum].use_threads)

#define GFTPUI_COMMON_COLOR_BLACK     "\033[30m"
#define GFTPUI_COMMON_COLOR_RED       "\033[31m"
#define GFTPUI_COMMON_COLOR_GREEN     "\033[32m"
#define GFTPUI_COMMON_COLOR_YELLOW    "\033[33m"
#define GFTPUI_COMMON_COLOR_BLUE      "\033[34m"
#define GFTPUI_COMMON_COLOR_MAGENTA   "\033[35m"
#define GFTPUI_COMMON_COLOR_CYAN      "\033[36m"
#define GFTPUI_COMMON_COLOR_WHITE     "\033[37m"
#define GFTPUI_COMMON_COLOR_GREY      "\033[38m"
#define GFTPUI_COMMON_COLOR_DEFAULT   "\033[39m"
#define GFTPUI_COMMON_COLOR_NONE	""

extern sigjmp_buf gftpui_common_jmp_environment;
extern volatile int gftpui_common_use_jmp_environment;
extern gftpui_common_methods gftpui_common_commands[];
extern GStaticMutex gftpui_common_transfer_mutex;

/* gftpui.c */
int gftpui_run_callback_function	( gftpui_callback_data * cdata );

int gftpui_common_run_callback_function ( gftpui_callback_data * cdata );

RETSIGTYPE gftpui_common_signal_handler ( int signo );

void gftpui_common_about 		( gftp_logging_func logging_function,
					  gpointer logdata );

int gftpui_common_init			( void *locui,
					  gftp_request * locreq,
					  void *remui,
					  gftp_request * remreq );

int gftpui_common_process_command 	( const char *command );

int gftpui_common_cmd_open 		( void *uidata,
					  gftp_request * request,
					  char *command );

gftp_transfer * gftpui_common_add_file_transfer ( gftp_request * fromreq,
						  gftp_request * toreq,
						  void *fromuidata,
						  void *touidata,
						  GList * files );

int gftpui_common_transfer_files 	( gftp_transfer * tdata );

/* gftpuicallback.c */
int gftpui_common_run_mkdir 		( gftpui_callback_data * cdata );

int gftpui_common_run_rename 		( gftpui_callback_data * cdata );

int gftpui_common_run_site 		( gftpui_callback_data * cdata );

int gftpui_common_run_chdir 		( gftpui_callback_data * cdata );

int gftpui_common_run_chmod		( gftpui_callback_data * cdata );

int gftpui_common_run_ls 		( gftpui_callback_data * cdata );

int gftpui_common_run_delete		( gftpui_callback_data * cdata );

int gftpui_common_run_rmdir 		( gftpui_callback_data * cdata );

int gftpui_common_run_connect 		( gftpui_callback_data * cdata );

/* UI Functions that must be implemented by each distinct UI */
void gftpui_lookup_file_colors 		( gftp_file * fle,
					  char **start_color,
					  char ** end_color );

int gftpui_check_reconnect		( gftpui_callback_data * cdata );

void gftpui_refresh 			( void *uidata );

void *gftpui_generic_thread 		( void *(*run_function)(void *data),
					  void *data);

char *gftpui_prompt_username		( void *uidata,
					  gftp_request * request );

char *gftpui_prompt_password 		( void *uidata,
					  gftp_request * request );

void gftpui_add_file_to_transfer 	( gftp_transfer * tdata,
					  GList * curfle,
					  char *filepos );

void gftpui_ask_transfer		( gftp_transfer * tdata );

#endif
