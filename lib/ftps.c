/*****************************************************************************/
/*  ftps.c - General purpose routines for the FTPS protocol                  */
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
#include "ftpcommon.h"

static const char cvsid[] = "$Id$";

#ifdef USE_SSL
static int
ftps_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc959_parms * params;
  int ret, resetptr;

  params = request->protocol_data;
  if (request->cached)
    {
      request->read_function = gftp_fd_read;
      request->write_function = gftp_fd_write;
      resetptr = 1;
    }
  else
    resetptr = 0;

  ret = rfc959_get_next_file (request, fle, fd);

  if (resetptr)
    {
      request->read_function = gftp_ssl_read;
      request->write_function = gftp_ssl_write;
    }

  return (ret);
}


static int 
ftps_auth_tls_start (gftp_request * request)
{
  rfc959_parms * params;
  int ret;

  params = request->protocol_data;

  ret = rfc959_send_command (request, "AUTH TLS\r\n", 1);
  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (0);

  if ((ret = gftp_ssl_session_setup (request)) < 0)
    return (ret);

  request->read_function = gftp_ssl_read;
  request->write_function = gftp_ssl_write;

  ret = rfc959_send_command (request, "PBSZ 0\r\n", 1);
  if (ret < 0)
    return (ret);

  ret = '5'; /* FIXME */
  /* ret = rfc959_send_command (request, "PROT P\r\n", 1); */
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    {
      params->data_conn_read = gftp_ssl_read;
      params->data_conn_write = gftp_ssl_write;
      params->encrypted_connection = 1;
    }
  else
    {
      ret = rfc959_send_command (request, "PROT C\r\n", 1);
      if (ret < 0)
        return (ret);

      params->data_conn_read = gftp_fd_read;
      params->data_conn_write = gftp_fd_write;
      params->encrypted_connection = 0;
    }

  return (0);
}
#endif


void
ftps_register_module (void)
{
#ifdef USE_SSL
  ssl_register_module ();
#endif
}


int
ftps_init (gftp_request * request)
{
#ifdef USE_SSL
  rfc959_parms * params;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  if ((ret = gftp_protocols[GFTP_FTP_NUM].init (request)) < 0)
    return (ret);

  params = request->protocol_data;
  request->protonum = GFTP_FTPS_NUM;
  params->auth_tls_start = ftps_auth_tls_start;
  request->get_next_file = ftps_get_next_file;
  request->init = ftps_init;
  request->post_connect = NULL;
  request->url_prefix = g_strdup ("ftps");

  if ((ret = gftp_ssl_startup (NULL)) < 0)
    return (ret);

  return (0);
#else
  request->logging_function (gftp_logging_error, request,
                             _("FTPS Support unavailable since SSL support was not compiled in. Aborting connection.\n"));

  return (GFTP_EFATAL);
#endif
}

