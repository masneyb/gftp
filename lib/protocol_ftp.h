/*****************************************************************************/
/*  protocol_ftp.h - common data structures for RFC959 and FTPS                 */
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

#include "gftp.h"

/* Server types (used by FTP protocol from SYST command) */
#define FTP_DIRTYPE_UNIX    1
#define FTP_DIRTYPE_EPLF    2
#define FTP_DIRTYPE_CRAY    3
#define FTP_DIRTYPE_NOVELL  4
#define FTP_DIRTYPE_DOS     5
#define FTP_DIRTYPE_VMS     6
#define FTP_DIRTYPE_OTHER   7
#define FTP_DIRTYPE_MVS     8
#define FTP_DIRTYPE_MLSD    9 /* MLSD/MLST replaces LIST */


struct ftp_protocol_data_tag
{  
  gftp_getline_buffer * datafd_rbuf;
  gftp_getline_buffer * dataconn_rbuf;
  int data_connection;
  unsigned int is_ascii_transfer : 1;
  unsigned int is_fxp_transfer : 1;
  int (*auth_tls_start) (gftp_request * request);
  int (*data_conn_tls_start) (gftp_request * request);
  ssize_t (*data_conn_read) (gftp_request * request, void *ptr, size_t size,
                             int fd);
  ssize_t (*data_conn_write) (gftp_request * request, const char *ptr,
                              size_t size, int fd);
  void (*data_conn_tls_close) (gftp_request * request);
  unsigned int implicit_ssl : 1;
  unsigned int use_mlsd_cmd : 1; /* use MLSD/MLST insted of LIST */
  unsigned int use_pret_cmd : 1; /* for distibuted FTP servers (DrFTP) */
  int list_type;  /* LIST. See FTP_DIRTYPE_* above */
  int last_cmd;     /* 4hackz */
  int flags;        /* currently unused */
};

typedef struct ftp_protocol_data_tag ftp_protocol_data;

int ftp_send_command (gftp_request * request,
                      const char *command,
                      ssize_t command_len,
                      int read_response,
                      int dont_try_to_reconnect );

