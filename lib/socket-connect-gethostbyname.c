/*****************************************************************************/
/*  socket-connect.c - contains functions for connecting to a server         */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                  */
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
static const char cvsid[] = "$Id: protocols.c 952 2008-01-24 23:31:26Z masneyb $";

#if !defined (HAVE_GETADDRINFO) || !defined (HAVE_GAI_STRERROR)

static struct hostent *
r_gethostbyname (const char *name, struct hostent *result_buf, int *h_errnop)
{
  static GStaticMutex hostfunclock = G_STATIC_MUTEX_INIT; 
  struct hostent *hent;

  if (g_thread_supported ())
    g_static_mutex_lock (&hostfunclock);

  if ((hent = gethostbyname (name)) == NULL)
    {
      if (h_errnop)
        *h_errnop = h_errno;
    }
  else
    {
      *result_buf = *hent;
      hent = result_buf;
    }

  if (g_thread_supported ())
    g_static_mutex_unlock (&hostfunclock);

  return (hent);
}


int
lookup_host_with_gethostbyname (gftp_request *request, char *proxy_hostname,
                                struct hostent *hostp)
{
  char *connect_host;

  if (request->use_proxy)
    connect_host = proxy_hostname;
  else
    connect_host = request->hostname;

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), connect_host);

  if ((r_gethostbyname (connect_host, hostp, NULL)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 connect_host, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  return (0);
}


int
gftp_connect_server_legacy (gftp_request * request, char *service,
                            char *proxy_hostname, unsigned int proxy_port)
{
  int sock, curhost, ret, connect_port;
  struct sockaddr_in remote_address;
  struct servent serv_struct;
  struct hostent host;

  ret = lookup_host_with_gethostbyname (request, proxy_hostname, &host);
  if (ret != 0)
    return (ret);

  if (request->use_proxy)
    connect_port = proxy_port;
  else
    connect_port = request->port;

  if (connect_port == 0)
    {
      if (!r_getservbyname (service, "tcp", &serv_struct, NULL))
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot look up service name %s/tcp. Please check your services file\n"),
                                     service);
          return (GFTP_ERETRYABLE);
        }

      connect_port = ntohs (serv_struct.s_port);

      if (!request->use_proxy)
        request->port = connect_port;
    }

  sock = GFTP_ERETRYABLE;
  request->ai_family = AF_INET;

  for (curhost = 0; host.h_addr_list[curhost] != NULL; curhost++)
    {
      if ((sock = socket (request->ai_family, SOCK_STREAM, IPPROTO_TCP)) < 0)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Failed to create a IPv4 socket: %s\n"),
                                     g_strerror (errno));
          return (GFTP_ERETRYABLE);
        }

      memset (&remote_address, 0, sizeof (remote_address));
      remote_address.sin_family = AF_INET;
      remote_address.sin_port = htons (connect_port);

      memcpy (&remote_address.sin_addr,
              host.h_addr_list[curhost],
              host.h_length);

      request->logging_function (gftp_logging_misc, request,
                                 _("Trying %s:%d\n"),
                                 host.h_name, ntohs (remote_address.sin_port));

      if (connect (sock, (struct sockaddr *) &remote_address,
                   sizeof (remote_address)) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     host.h_name, g_strerror (errno));
          close (sock);
          continue;
        }

      break;
    }

  if (host.h_addr_list[curhost] == NULL)
    return (GFTP_ERETRYABLE);

  request->remote_addr_len = host.h_length;
  request->remote_addr = g_malloc0 (request->remote_addr_len);
  memcpy (request->remote_addr, &host.h_addr_list[curhost],
          request->remote_addr_len);

  request->logging_function (gftp_logging_misc, request,
                             _("Connected to %s:%d\n"), host.h_name,
                             ntohs (remote_address.sin_port));

  return (sock);
}

#endif
