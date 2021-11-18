/*****************************************************************************/
/*  ftps.c - General purpose routines for the FTPS protocol                  */
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
#include "ftpcommon.h"

#ifdef USE_SSL

static int
ftps_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  rfc959_parms * params;
  int resetptr = 0, resetdata = 0;
  size_t ret;

  params = request->protocol_data;
  if (request->cached &&
     request->read_function == gftp_ssl_read)
    {
      request->read_function = gftp_fd_read;
      request->write_function = gftp_fd_write;
      resetptr = 1;
      if (params->data_conn_read == gftp_ssl_read)
	{
	  params->data_conn_read = gftp_fd_read;
	  params->data_conn_write = gftp_fd_write;
	  resetdata = 1;
	}
    }

  ret = rfc959_get_next_file (request, fle, fd);

  if (resetptr)
    {
      request->read_function = gftp_ssl_read;
      request->write_function = gftp_ssl_write;
      if (resetdata)
	{
	  params->data_conn_read = gftp_ssl_read;
	  params->data_conn_write = gftp_ssl_write;
	}
    }

  return (ret);
}

static int 
ftps_data_conn_tls_start (gftp_request * request)
{
  rfc959_parms * params = request->protocol_data;
  return gftp_ssl_session_setup_ex (request, params->data_connection);
}

static void
ftps_data_conn_tls_close (gftp_request * request)
{
  rfc959_parms * params = request->protocol_data;
  gftp_ssl_session_close_ex (request, params->data_connection);
}


static int ftps_auth_tls_start (gftp_request * request)
{
  rfc959_parms * params;
  int ret;

  params = request->protocol_data;

  ret = rfc959_send_command (request, "AUTH TLS\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_EFATAL);

  if ((ret = gftp_ssl_session_setup (request)) < 0)
    return (ret);

  request->read_function = gftp_ssl_read;
  request->write_function = gftp_ssl_write;

  ret = rfc959_send_command (request, "PBSZ 0\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);

  ret = rfc959_send_command (request, "PROT P\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    {
      params->data_conn_tls_start = ftps_data_conn_tls_start;
      params->data_conn_read = gftp_ssl_read;
      params->data_conn_write = gftp_ssl_write;
      params->data_conn_tls_close = ftps_data_conn_tls_close;
    }
  else
    {
      ret = rfc959_send_command (request, "PROT C\r\n", -1, 1, 0);
      if (ret < 0)
        return (ret);
      else if (ret != '2')
        {
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      params->data_conn_read = gftp_fd_read;
      params->data_conn_write = gftp_fd_write;
    }

  return (0);
}


static int ftps_connect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  rfc959_parms * parms = (rfc959_parms *) request->protocol_data;
  if (request->datafd > 0) {
    return (0);
  }
  if (!parms->implicit_ssl) {
     request->read_function = gftp_fd_read;
     request->write_function = gftp_fd_write;
  }
  return (rfc959_connect (request));
}

#endif


void
ftps_register_module (void)
{
  DEBUG_PRINT_FUNC
#ifdef USE_SSL
  ssl_register_module ();
#endif
}


int ftps_init (gftp_request * request)
{
  DEBUG_PRINT_FUNC
#ifdef USE_SSL
  rfc959_parms * params;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  /* init FTP first */
  ret = gftp_protocols[GFTP_FTP_NUM].init (request);
  if (ret < 0) {
     return (ret);
  }

  params = (rfc959_parms *) request->protocol_data;
  request->protonum = GFTP_FTPS_NUM;
  request->init = ftps_init;
  request->connect = ftps_connect;
  params->auth_tls_start = ftps_auth_tls_start;
  request->get_next_file = ftps_get_next_file;
  request->post_connect = NULL;
  request->url_prefix = "ftps";

  if ((ret = gftp_ssl_startup (NULL)) < 0)
    return (ret);

  return (0);
#else
  request->logging_function (gftp_logging_error, request,
                             _("FTPS Support unavailable since SSL support was not compiled in. Aborting connection.\n"));

  return (GFTP_EFATAL);
#endif
}



//=======================================================================
//                          IMPLICIT SSL/TSL
//=======================================================================
// Use the FTPS stuff and just override default values and functions where needed
// Establish encrypted connection before sending or receiving any message

void ftpsi_register_module (void)
{
   DEBUG_PRINT_FUNC
}


static int ftpsi_connect (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   // this is just to override default port
   // default port for FTPSi is 990..
   if (!request->port) {
      request->port = gftp_protocols[GFTP_FTPSi_NUM].default_port;
   }
   return (ftps_connect (request));
}


int ftpsi_init (gftp_request * request)
{
   DEBUG_PRINT_FUNC
#ifdef USE_SSL
   int ret;
   rfc959_parms * params;
   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   // init funcs (FTP->FTPS->FTPSi)
   ret = gftp_protocols[GFTP_FTPS_NUM].init (request);
   if (ret < 0) {
      return (ret);
   }

   request->protonum = GFTP_FTPSi_NUM;
   params = (rfc959_parms *) request->protocol_data;
   params->implicit_ssl        = 1;
   params->data_conn_tls_start = ftps_data_conn_tls_start;
   params->data_conn_read      = gftp_ssl_read;
   params->data_conn_write     = gftp_ssl_write;
   params->data_conn_tls_close = ftps_data_conn_tls_close;
   request->read_function      = gftp_ssl_read;
   request->write_function     = gftp_ssl_write;
   request->url_prefix = "ftpsi";
   request->connect = ftpsi_connect;
   return 0;
#else
   request->logging_function (gftp_logging_error, request,
                             _("FTPSi (Implicit SSL) Support unavailable since SSL support was not compiled in. Aborting connection.\n"));

   return (GFTP_EFATAL);
#endif
}

