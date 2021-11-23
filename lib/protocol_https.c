/***********************************************************************************/
/*  protocol_https.c - General purpose routines for the HTTPS protocol             */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                        */
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

