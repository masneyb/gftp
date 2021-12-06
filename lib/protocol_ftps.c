/***********************************************************************************/
/*  protocol_ftps.c - General purpose routines for the FTPS protocol               */
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
#include "protocol_ftp.h"

//=======================================================================
//                          EXPLICIT TLS
//=======================================================================

#ifdef USE_SSL

static int
ftps_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  ftp_protocol_data * ftpdat;
  int resetptr = 0, resetdata = 0;
  size_t ret;

  ftpdat = request->protocol_data;
  if (request->cached && request->read_function == gftp_ssl_read)
    {
      request->read_function = gftp_fd_read;
      request->write_function = gftp_fd_write;
      resetptr = 1;
      if (ftpdat->data_conn_read == gftp_ssl_read)
      {
         ftpdat->data_conn_read  = gftp_fd_read;
         ftpdat->data_conn_write = gftp_fd_write;
         resetdata = 1;
      }
    }

  ret = ftp_get_next_file (request, fle, fd);

  if (resetptr)
    {
      request->read_function  = gftp_ssl_read;
      request->write_function = gftp_ssl_write;
      if (resetdata)
      {
         ftpdat->data_conn_read  = gftp_ssl_read;
         ftpdat->data_conn_write = gftp_ssl_write;
      }
    }

  return (ret);
}


static int  ftps_data_conn_tls_start (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat = request->protocol_data;
  return gftp_ssl_session_setup_ex (request, ftpdat->data_connection);
}


static void ftps_data_conn_tls_close (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat = request->protocol_data;
  gftp_ssl_session_close_ex (request, ftpdat->data_connection);
}


static int ftps_auth_tls_start (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  ftp_protocol_data * ftpdat;
  int ret;

  ftpdat = request->protocol_data;

  ret = ftp_send_command (request, "AUTH TLS\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret != '2')
    return (GFTP_EFATAL);

  if ((ret = gftp_ssl_session_setup (request)) < 0)
    return (ret);

  request->read_function  = gftp_ssl_read;
  request->write_function = gftp_ssl_write;

  ret = ftp_send_command (request, "PBSZ 0\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);

  ret = ftp_send_command (request, "PROT P\r\n", -1, 1, 0);
  if (ret < 0)
    return (ret);
  else if (ret == '2')
    {
      ftpdat->data_conn_tls_start = ftps_data_conn_tls_start;
      ftpdat->data_conn_read      = gftp_ssl_read;
      ftpdat->data_conn_write     = gftp_ssl_write;
      ftpdat->data_conn_tls_close = ftps_data_conn_tls_close;
    }
  else
    {
      ret = ftp_send_command (request, "PROT C\r\n", -1, 1, 0);
      if (ret < 0)
        return (ret);
      else if (ret != '2')
        {
          gftp_disconnect (request);
          return (GFTP_ERETRYABLE);
        }

      ftpdat->data_conn_read  = gftp_fd_read;
      ftpdat->data_conn_write = gftp_fd_write;
    }

  return (0);
}


static int ftps_connect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  if (request->datafd > 0) {
      return (0);
  }
  // need to send AUTH TLS in clear text
  request->read_function  = gftp_fd_read;
  request->write_function = gftp_fd_write;

  return (ftp_connect (request));
}

#endif


void ftps_register_module (void)
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
  ftp_protocol_data * ftpdat;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  /* init FTP first */
  ret = gftp_protocols[GFTP_PROTOCOL_FTP].init (request);
  if (ret < 0) {
     return (ret);
  }

  /* don't change protonum */
  //request->protonum = GFTP_PROTOCOL_FTPS;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_FTPS].url_prefix;

  ftpdat = (ftp_protocol_data *) request->protocol_data;
  request->init = ftps_init;
  request->connect = ftps_connect;
  ftpdat->auth_tls_start = ftps_auth_tls_start;
  request->get_next_file = ftps_get_next_file;
  request->post_connect = NULL;

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
//                          IMPLICIT TLS
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
   if (request->datafd > 0) {
      return (0);
   }
   // this is just to override default port
   // default port for FTPSi is 990..
   if (!request->port) {
      request->port = gftp_protocols[GFTP_PROTOCOL_FTPSi].default_port;
   }
   return (ftp_connect (request));
}


static int ftpsi_auth_tls_start (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   int ret;
   // buggy servers may require this (vsftpd)
   // this is not needed for servers with proper implicit SSL support
   //  (pure-FTPd, Rebex FTPS)
   ret = ftp_send_command (request, "PBSZ 0\r\n", -1, 1, 0);
   ret = ftp_send_command (request, "PROT P\r\n", -1, 1, 0);
   if (ret < 0) {
      return (ret);
   }
   return 0;
}


int ftpsi_init (gftp_request * request)
{
   DEBUG_PRINT_FUNC
#ifdef USE_SSL
   int ret;
   ftp_protocol_data * ftpdat;
   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   // init funcs (FTP->FTPS->FTPSi)
   ret = gftp_protocols[GFTP_PROTOCOL_FTPS].init (request);
   if (ret < 0) {
      return (ret);
   }

   /* don't change protonum */
   //request->protonum = GFTP_PROTOCOL_FTPSi;
   request->url_prefix = gftp_protocols[GFTP_PROTOCOL_FTPSi].url_prefix;

   ftpdat = (ftp_protocol_data *) request->protocol_data;
   ftpdat->data_conn_tls_start = ftps_data_conn_tls_start;
   ftpdat->data_conn_read      = gftp_ssl_read;
   ftpdat->data_conn_write     = gftp_ssl_write;
   ftpdat->data_conn_tls_close = ftps_data_conn_tls_close;
   request->read_function      = gftp_ssl_read;
   request->write_function     = gftp_ssl_write;

   request->init               = ftpsi_init;
   request->connect            = ftpsi_connect;
   ftpdat->auth_tls_start      = ftpsi_auth_tls_start;
   request->post_connect       = gftp_ssl_session_setup; /* socket-connect.c */

   return 0;
#else
   request->logging_function (gftp_logging_error, request,
                             _("FTPSi (Implicit SSL) Support unavailable since SSL support was not compiled in. Aborting connection.\n"));

   return (GFTP_EFATAL);
#endif
}

