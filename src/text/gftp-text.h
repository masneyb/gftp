/*****************************************************************************/
/*  gftp-text.h - include file for the gftp text port                        */
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

#ifndef __GFTP_TEXT_H
#define __GFTP_TEXT_H

#include "../../lib/gftp.h"

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_GREY      "\033[38m"
#define COLOR_DEFAULT   "\033[39m"

struct _gftp_text_methods
{
  char *command;
  int minlen;
  int (*func)(gftp_request * request, char *command, gpointer *data);
  gftp_request ** request;
  char *cmd_description;
};

/* gftp-text.h */
void gftp_text_log				( gftp_logging_level level, 
						  void *ptr, 
						  const char *string, ... );
int gftp_text_open				( gftp_request * request, 
						  char *command, 
						  gpointer *data );
int gftp_text_close				( gftp_request * request, 
						  char *command, 
						  gpointer *data );
int gftp_text_about				( gftp_request * request, 
						  char *command, 
						  gpointer *data );
int gftp_text_quit 				( gftp_request * request, 
						  char *command, 
						  gpointer *data );
int gftp_text_pwd 				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_cd				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_mkdir				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_rmdir				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_delete				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_rename				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_chmod				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_ls				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_binary				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_ascii				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_mget_file				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_mput_file				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_transfer_files 			( gftp_transfer * transfer );
int gftp_text_help				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
int gftp_text_set				( gftp_request * request, 
						  char *command, 
						  gpointer *data);
char *gftp_text_ask_question 			( const char *question, 
						  int echo,
						  char *buf,
						  size_t size );
int gftp_text_get_win_size 			( void );
void gftp_text_calc_kbs 			( gftp_transfer * tdata, 
						  ssize_t num_read );
void sig_child 					( int signo );

#endif

