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

#include "gftp.h"

enum
{
   FTP_FEAT_MLSD,  /* rfc3659 - use MLSD/MLST insted of LIST */
   FTP_FEAT_PRET,  /* accepts PRET, distibuted FTP server (DrFTP) */
   FTP_FEAT_TOTAL,
};

struct ftp_supported_feature
{
   int index;
   char * str;
};
extern struct ftp_supported_feature ftp_supported_features[];


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
  int list_type;    /* LIST. See FTP_DIRTYPE_* above */
  int last_cmd;     /* 4hackz */
  unsigned int feat[FTP_FEAT_TOTAL];
};

typedef struct ftp_protocol_data_tag ftp_protocol_data;


// =========
// functions
// =========

/* parse-dir-listing.c */
int ftp_parse_ls (gftp_request * request,
                   const char *lsoutput, 
                   gftp_file *fle,
                   int fd);

void parse_syst_response (char * response_str, ftp_protocol_data * ftpdat);


// protocol_ftp.c
int ftp_send_command (gftp_request * request,
                      const char *command,
                      ssize_t command_len,
                      int read_response,
                      int dont_try_to_reconnect );

