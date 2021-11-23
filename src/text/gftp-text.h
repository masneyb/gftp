/***********************************************************************************/
/*  gftp-text.h - include file for the gftp text port                              */
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

