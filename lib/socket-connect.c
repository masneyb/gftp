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


static struct addrinfo *
lookup_host (gftp_request *request, char *service,
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
                                 _("Unknown host. Maybe you misspelled it?\n"));
      return (NULL);
    }

  return (hostp);
}


static int
gftp_connect_server_with_getaddrinfo (gftp_request * request, char *service,
                                      char *proxy_hostname,
                                      unsigned int proxy_port)
{
  struct addrinfo *res, *hostp, *current_hostp;
  unsigned int port;
  int sock = -1;

  hostp = lookup_host (request, service, proxy_hostname,
                                        proxy_port);
  if (hostp == NULL)
    return (GFTP_EFATAL);

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

// ========================================================================

static int
gftp_need_proxy (gftp_request * request, char *service, char *proxy_hostname, 
                 unsigned int proxy_port, void **connect_data)
{
  gftp_config_list_vars * proxy_hosts;
  gftp_proxy_hosts * hostname;
  size_t hostlen, domlen;
  unsigned char addy[4] = { 0, 0, 0, 0 };
  GList * templist;
  gint32 netaddr;
  char *pos;

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);

  if (proxy_hostname == NULL || *proxy_hostname == '\0')
    return (0);
  else if (proxy_hosts->list == NULL)
    return (proxy_hostname != NULL && 
            *proxy_hostname != '\0');

  *connect_data = lookup_host (request, service,
                                                proxy_hostname, proxy_port);
  if (*connect_data == NULL)
    return (GFTP_EFATAL);

  templist = proxy_hosts->list;
  while (templist != NULL)
    {
      hostname = templist->data;
      if (hostname->domain != NULL)
        {
           hostlen = strlen (request->hostname);
           domlen = strlen (hostname->domain);
           if (hostlen > domlen)
             {
                pos = request->hostname + hostlen - domlen;
                if (strcmp (hostname->domain, pos) == 0)
                  return (0);
             }
        }

      if (hostname->ipv4_network_address != 0)
        {
          memcpy (addy, request->remote_addr, sizeof (*addy));
          netaddr =
            (((addy[0] & 0xff) << 24) | ((addy[1] & 0xff) << 16) |
             ((addy[2] & 0xff) << 8) | (addy[3] & 0xff)) & 
             hostname->ipv4_netmask;
          if (netaddr == hostname->ipv4_network_address)
            return (0);
        }
      templist = templist->next;
    }

  return (proxy_hostname != NULL && *proxy_hostname != '\0');
}


int
gftp_connect_server (gftp_request * request, char *service,
                     char *proxy_hostname, unsigned int proxy_port)
{
  void *connect_data;
  int sock, ret;

  if ((ret = gftp_need_proxy (request, service, proxy_hostname,
                              proxy_port, &connect_data)) < 0)
    return (ret);
  request->use_proxy = ret;

  /* FIXME - pass connect_data to these functions. This is to bypass a
     second DNS lookup */
  sock = gftp_connect_server_with_getaddrinfo (request, service, proxy_hostname,
                                               proxy_port);
  if (sock < 0)
    return (sock);

  if (fcntl (sock, F_SETFD, 1) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot set close on exec flag: %s\n"),
                                 g_strerror (errno));
      close (sock);
      return (GFTP_ERETRYABLE);
    }

  if (gftp_fd_set_sockblocking (request, sock, 1) < 0)
    {
      close (sock);
      return (GFTP_ERETRYABLE);
    }

  request->datafd = sock;

  if (request->post_connect != NULL)
    return (request->post_connect (request));

  return (0);
}

