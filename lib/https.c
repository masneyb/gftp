/*****************************************************************************/
/*  https.c - General purpose routines for the HTTPS protocol                */
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

#include "gftp.h"
#include "httpcommon.h"

static const char cvsid[] = "$Id$";

#ifdef USE_SSL
static int
https_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc2068_params * params;
  int ret, resetptr;

  params = request->protocol_data;
  if (request->cached)
    {
      params->real_read_function = gftp_fd_read;
      request->write_function = gftp_fd_write;
      resetptr = 1;
    }
  else
    resetptr = 0;

  ret = rfc2068_get_next_file (request, fle, fd);

  if (resetptr)
    {
      params->real_read_function = gftp_ssl_read;
      request->write_function = gftp_ssl_write;
    }

  return (ret);
}
#endif

void
https_register_module (void)
{
#ifdef USE_SSL
  gftp_ssl_startup (NULL); /* FIXME - take out of here */
#endif
}


void
https_init (gftp_request * request)
{
#ifdef USE_SSL
  rfc2068_params * params;

  g_return_if_fail (request != NULL);

  gftp_protocols[GFTP_HTTP_NUM].init (request);
  params = request->protocol_data;
  request->init = https_init;
  request->post_connect = gftp_ssl_session_setup;
  params->real_read_function = gftp_ssl_read;
  request->write_function = gftp_ssl_write;
  request->get_next_file = https_get_next_file;
  request->url_prefix = g_strdup ("https");
#else
  gftp_protocols[GFTP_HTTP_NUM].init (request);

  request->logging_function (gftp_logging_error, request->user_data,
                             _("HTTPS Support unavailable since SSL support was not compiled in. Reverting back to plaintext\n"));
#endif
}

