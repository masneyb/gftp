/*****************************************************************************/
/*  ftpcommon.h - common data structures for RFC959 and FTPS                 */
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

struct rfc959_params_tag
{  
  gftp_getline_buffer * datafd_rbuf,
                      * dataconn_rbuf;
  int data_connection;
  unsigned int is_ascii_transfer : 1,
               is_fxp_transfer : 1,
               quote_filename : 1;
  int (*auth_tls_start) (gftp_request * request);
  ssize_t (*data_conn_read) (gftp_request * request, void *ptr, size_t size,
                             int fd);
  ssize_t (*data_conn_write) (gftp_request * request, const char *ptr,
                              size_t size, int fd);
};

typedef struct rfc959_params_tag rfc959_parms;

int rfc959_send_command 		( gftp_request * request,
					  const char *command,
					  int read_response );

