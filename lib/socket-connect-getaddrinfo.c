/*****************************************************************************/
/*  socket-connect-getaddrinfo.c - uses getaddrinfo for connections          */
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

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)

static int
get_port (struct addrinfo *addr)
{
  struct sockaddr_in * saddr;
  int port;

  if (addr->ai_family == AF_INET)
    {
      saddr = (struct sockaddr_in *) addr->ai_addr;
      port = ntohs (saddr->sin_port);
    }
  else
    port = 0;

  return (port);
}


struct addrinfo *
lookup_host_with_getaddrinfo (gftp_request *request, char *service,
                              char *proxy_hostname, int proxy_port)
{
  struct addrinfo hints, *hostp;
  intptr_t enable_ipv6;
  char serv[8], *connect_host;
  int ret, connect_port;

  if (request->use_proxy)
    {
      connect_host = proxy_hostname;
      connect_port = proxy_port;
    }
  else
    {
      connect_host = request->hostname;
      connect_port = request->port;
    }

  gftp_lookup_request_option (request, "enable_ipv6", &enable_ipv6);

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;

  hints.ai_family = enable_ipv6 ? PF_UNSPEC : AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (connect_port == 0)
    strcpy (serv, service); 
  else
    snprintf (serv, sizeof (serv), "%d", connect_port);

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), connect_host);
  if ((ret = getaddrinfo (connect_host, serv, &hints, 
                             &hostp)) != 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 connect_host, gai_strerror (ret));
      return (NULL);
    }

  return (hostp);
}


int
gftp_connect_server_with_getaddrinfo (gftp_request * request, char *service,
                                      char *proxy_hostname,
                                      unsigned int proxy_port)
{
  struct addrinfo *res, *hostp, *current_hostp;
  unsigned int port;
  int sock = -1;

  hostp = lookup_host_with_getaddrinfo (request, service, proxy_hostname,
                                        proxy_port);
  if (hostp == NULL)
    return (GFTP_ERETRYABLE);

  for (res = hostp; res != NULL; res = res->ai_next)
    {
      port = get_port (res);
      if (!request->use_proxy)
        request->port = port;

      if ((sock = socket (res->ai_family, res->ai_socktype, 
                          res->ai_protocol)) < 0)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Failed to create a socket: %s\n"),
                                     g_strerror (errno));
          continue; 
        } 

      request->logging_function (gftp_logging_misc, request,
                                 _("Trying %s:%d\n"), res[0].ai_canonname,
                                 port);

      if (connect (sock, res->ai_addr, res->ai_addrlen) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     res[0].ai_canonname, g_strerror (errno));
          close (sock);
          continue;
        }

      current_hostp = res;
      request->ai_family = res->ai_family;
      break;
    }

  if (res == NULL)
    {
      if (hostp != NULL)
        freeaddrinfo (hostp);
      
      return (GFTP_ERETRYABLE);
    }

  request->remote_addr_len = current_hostp->ai_addrlen;
  request->remote_addr = g_malloc0 (request->remote_addr_len);
  memcpy (request->remote_addr, &((struct sockaddr_in *) current_hostp->ai_addr)->sin_addr,
          request->remote_addr_len);

  request->logging_function (gftp_logging_misc, request,
                             _("Connected to %s:%d\n"), res[0].ai_canonname,
                             port);

  return (sock);
}

#endif
