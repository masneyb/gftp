/*****************************************************************************/
/*  gftp-text.h - include file for the gftp text port                        */
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

#ifndef __GFTP_TEXT_H
#define __GFTP_TEXT_H

#include "../../lib/gftp.h"
#include "../uicommon/gftpui.h"

#if HAVE_LIBREADLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

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


unsigned int gftp_text_get_win_size 		( void );

char * gftp_text_ask_question 			( gftp_request * request,
						  const char *question,
						  int echo,
						  char *buf,
						  size_t size );

#endif

