/*****************************************************************************/
/*  protocol_https.c - General purpose routines for the HTTPS protocol       */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
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

#include "gftp.h"
#include "protocol_http.h"

#ifdef USE_SSL
static int https_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;
   int resetptr;
   size_t ret;

   httpdat = request->protocol_data;
   if (request->cached) {
      httpdat->real_read_function = gftp_fd_read;
      request->write_function = gftp_fd_write;
      resetptr = 1;
   } else {
      resetptr = 0;
   }
   ret = http_get_next_file (request, fle, fd);

   if (resetptr) {
      httpdat->real_read_function = gftp_ssl_read;
      request->write_function = gftp_ssl_write;
   }
   return (ret);
}
#endif

void https_register_module (void)
{
   DEBUG_PRINT_FUNC
#ifdef USE_SSL
   ssl_register_module ();
#endif
}


int https_init (gftp_request * request)
{
   DEBUG_PRINT_FUNC
#ifdef USE_SSL
   http_protocol_data * httpdat;
   int ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   if ((ret = gftp_protocols[GFTP_PROTOCOL_HTTP].init (request)) < 0) {
      return (ret);
   }

   /* don't change protonum */
   //request->protonum = GFTP_PROTOCOL_HTTPS;
   request->url_prefix = gftp_protocols[GFTP_PROTOCOL_HTTPS].url_prefix;

   httpdat = request->protocol_data;

   httpdat->real_read_function = gftp_ssl_read;

   request->init           = https_init;
   request->post_connect   = gftp_ssl_session_setup; /* socket-connect.c */
   request->write_function = gftp_ssl_write;
   request->get_next_file  = https_get_next_file;

   if ((ret = gftp_ssl_startup (NULL)) < 0) {
      return (ret);
   }
   return (0);
#else
   request->logging_function (gftp_logging_error, request,
                             _("HTTPS Support unavailable since SSL support was not compiled in. Aborting connection.\n"));
   return (GFTP_EFATAL);
#endif
}

