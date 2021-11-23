/***********************************************************************************/
/*  gftpui.h - UI related functions for gFTP                                       */
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
  void *user_data;
  int retries;
  int (*run_function) (gftpui_callback_data * cdata);
  int (*connect_function) (gftpui_callback_data * cdata);
  void (*disconnect_function) (gftpui_callback_data * cdata);
  unsigned int dont_check_connection : 1,
               dont_refresh : 1,
               dont_clear_cache : 1,
               toggled : 1;
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
  size_t minlen;
  int (*func)(void *uidata, gftp_request * request,
              void *other_uidata, gftp_request * other_request,
              const char *command);
  gftpui_common_request_type reqtype;
  char *cmd_description;
  int (*subhelp_func) (const char *topic);
} gftpui_common_methods;


typedef struct _gftpui_common_curtrans_data
{
  gftp_transfer * transfer;
  GList * curfle;
} gftpui_common_curtrans_data;


#define gftpui_common_use_threads(request)	(gftp_protocols[(request)->protonum].use_threads)

extern sigjmp_buf gftpui_common_jmp_environment;
extern volatile int gftpui_common_use_jmp_environment;
extern gftpui_common_methods gftpui_common_commands[];
extern WGMutex gftpui_common_transfer_mutex;
extern volatile sig_atomic_t gftpui_common_child_process_done;

/* -------- */
/* gftpui.c */
/* -------- */
int  gftpui_run_callback_function (gftpui_callback_data * cdata);
int  gftpui_common_run_callback_function (gftpui_callback_data * cdata);
void gftpui_common_init	 (int *argc, char ***argv, gftp_logging_func logfunc);
void gftpui_common_about (gftp_logging_func logging_function, gpointer logdata);

int gftpui_common_process_command (void *locui,
                                   gftp_request * locreq,
                                   void *remui,
                                   gftp_request * remreq,
                                   const char *command);

int gftpui_common_cmd_open (void *uidata,
                            gftp_request * request,
                            void *other_uidata,
                            gftp_request * other_request,
                            const char *command);

int gftpui_common_cmd_mget_file (void *uidata,
                                 gftp_request * request, 
                                 void *other_uidata,
                                 gftp_request * other_request,
                                 const char *command);

int gftpui_common_cmd_mput_file (void *uidata,
                                 gftp_request * request, 
                                 void *other_uidata,
                                 gftp_request * other_request,
                                 const char *command);

gftp_transfer * gftpui_common_add_file_transfer (gftp_request * fromreq,
                                                 gftp_request * toreq,
                                                 void *fromuidata,
                                                 void *touidata,
                                                 GList * files);

void gftpui_cancel_file_transfer (gftp_transfer * tdata);

void gftpui_common_skip_file_transfer (gftp_transfer * tdata,
                                       gftp_file * curfle);

void gftpui_common_cancel_file_transfer	(gftp_transfer * tdata);

int gftpui_common_transfer_files (gftp_transfer * tdata);


/* ---------------- */
/* gftpuicallback.c */
/* ---------------- */
int gftpui_common_run_mkdir   (gftpui_callback_data * cdata);
int gftpui_common_run_rename  (gftpui_callback_data * cdata);
int gftpui_common_run_site    (gftpui_callback_data * cdata);
int gftpui_common_run_chdir   (gftpui_callback_data * cdata);
int gftpui_common_run_chmod   (gftpui_callback_data * cdata);
int gftpui_common_run_ls      (gftpui_callback_data * cdata);
int gftpui_common_run_delete  (gftpui_callback_data * cdata);
int gftpui_common_run_rmdir   (gftpui_callback_data * cdata);
int gftpui_common_run_connect (gftpui_callback_data * cdata);


/* --------------------------------------------------------- */
/* UI Functions that must be implemented by each distinct UI */
/* --------------------------------------------------------- */
void gftpui_lookup_file_colors (gftp_file * fle,
                                char ** start_color,
                                char ** end_color);

int gftpui_check_reconnect (gftpui_callback_data * cdata);

void gftpui_refresh (void *uidata, int clear_cache_entry);

void *gftpui_generic_thread (void *(*run_function)(void *data), void *data);

void gftpui_show_busy (gboolean busy);

void gftpui_prompt_username (void *uidata, gftp_request * request);
void gftpui_prompt_password (void *uidata, gftp_request * request);

void gftpui_add_file_to_transfer (gftp_transfer * tdata, GList * curfle);
void gftpui_ask_transfer         (gftp_transfer * tdata);

void gftpui_start_current_file_in_transfer  (gftp_transfer * tdata);
void gftpui_update_current_file_in_transfer (gftp_transfer * tdata);
void gftpui_finish_current_file_in_transfer (gftp_transfer * tdata);

void gftpui_start_transfer (gftp_transfer * tdata);
void gftpui_disconnect (void *uidata);


#endif
