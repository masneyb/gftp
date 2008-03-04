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

/* FIXME - clean up this function */
static int
gftp_need_proxy (gftp_request * request, char *service, char *proxy_hostname, 
                 unsigned int proxy_port)
{
  gftp_config_list_vars * proxy_hosts;
  gftp_proxy_hosts * hostname;
  size_t hostlen, domlen;
  unsigned char addy[4];
  struct sockaddr *addr;
  GList * templist;
  gint32 netaddr;
  char *pos;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  struct addrinfo hints, *hostp;
  unsigned int port;
  int errnum;
  char serv[8];
#else
  struct hostent host, *hostp;
#endif

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);

  if (proxy_hostname == NULL || *proxy_hostname == '\0')
    return (0);
  else if (proxy_hosts->list == NULL)
    return (proxy_hostname != NULL && 
            *proxy_hostname != '\0');

  hostp = NULL;
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  port = request->use_proxy ? proxy_port : request->port;
  if (port == 0)
    strcpy (serv, service);
  else
    snprintf (serv, sizeof (serv), "%d", port);

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), request->hostname);

  if ((errnum = getaddrinfo (request->hostname, serv, &hints, 
                             &hostp)) != 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, gai_strerror (errnum));
      return (GFTP_ERETRYABLE);
    }

  addr = hostp->ai_addr;

#else /* !HAVE_GETADDRINFO */
  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), request->hostname);

  if (!(hostp = r_gethostbyname (request->hostname, &host, NULL)))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 request->hostname, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  addr = (struct sockaddr *) host.h_addr_list[0];

#endif /* HAVE_GETADDRINFO */

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
          memcpy (addy, addr, sizeof (*addy));
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


static int
gftp_connect_server_with_getaddr (gftp_request * request, char *service,
                                  char *proxy_hostname, unsigned int proxy_port)
{
  struct addrinfo *hostp, *current_hostp;
  char *connect_host, *disphost;
  struct addrinfo hints, *res;
  intptr_t enable_ipv6;
  unsigned int port;
  int ret, sock = -1;
  char serv[8];

  if ((ret = gftp_need_proxy (request, service, proxy_hostname,
                                 proxy_port)) < 0)
    return (ret);
  else
    request->use_proxy = ret;

  gftp_lookup_request_option (request, "enable_ipv6", &enable_ipv6);

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;

  hints.ai_family = enable_ipv6 ? PF_UNSPEC : AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (request->use_proxy)
    {
      connect_host = proxy_hostname;
      port = proxy_port;
    }
  else
    {
      connect_host = request->hostname;
      port = request->port;
    }

  if (port == 0)
    strcpy (serv, service); 
  else
    snprintf (serv, sizeof (serv), "%d", port);

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), connect_host);
  if ((ret = getaddrinfo (connect_host, serv, &hints, 
                             &hostp)) != 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 connect_host, gai_strerror (ret));
      return (GFTP_ERETRYABLE);
    }

  disphost = connect_host;
  for (res = hostp; res != NULL; res = res->ai_next)
    {
      disphost = res->ai_canonname ? res->ai_canonname : connect_host;
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
                                 _("Trying %s:%d\n"), disphost, port);

      if (connect (sock, res->ai_addr, res->ai_addrlen) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     disphost, g_strerror (errno));
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
                             _("Connected to %s:%d\n"), connect_host, port);

  return (sock);
}
#endif


static int
gftp_connect_server_legacy (gftp_request * request, char *service,
                            char *proxy_hostname, unsigned int proxy_port)
{
  struct sockaddr_in remote_address;
  char *connect_host, *disphost;
  struct hostent host, *hostp;
  struct servent serv_struct;
  int ret, sock, curhost;
  unsigned int port;

  if ((ret = gftp_need_proxy (request, service, proxy_hostname,
                              proxy_port)) < 0)
    return (ret);

  request->use_proxy = ret;
  if (request->use_proxy == 1)
    hostp = NULL;

  request->ai_family = AF_INET;
  if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Failed to create a IPv4 socket: %s\n"),
                                 g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  memset (&remote_address, 0, sizeof (remote_address));
  remote_address.sin_family = AF_INET;

  if (request->use_proxy)
    {
      connect_host = proxy_hostname;
      port = proxy_port;
    }
  else
    {
      connect_host = request->hostname;
      port = request->port;
    }

  if (port == 0)
    {
      if (!r_getservbyname (service, "tcp", &serv_struct, NULL))
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot look up service name %s/tcp. Please check your services file\n"),
                                     service);
          close (sock);
          return (GFTP_EFATAL);
        }

      port = ntohs (serv_struct.s_port);

      if (!request->use_proxy)
        request->port = port;
    }

  remote_address.sin_port = htons (port);

  request->logging_function (gftp_logging_misc, request,
                             _("Looking up %s\n"), connect_host);

  if (!(hostp = r_gethostbyname (connect_host, &host, NULL)))
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot look up hostname %s: %s\n"),
                                 connect_host, g_strerror (errno));
      close (sock);
      return (GFTP_ERETRYABLE);
    }

  disphost = NULL;
  for (curhost = 0;
       host.h_addr_list[curhost] != NULL;
       curhost++)
    {
      disphost = host.h_name;
      memcpy (&remote_address.sin_addr,
              host.h_addr_list[curhost],
              host.h_length);
      request->logging_function (gftp_logging_misc, request,
                                 _("Trying %s:%d\n"),
                                 host.h_name, port);

      if (connect (sock, (struct sockaddr *) &remote_address,
                   sizeof (remote_address)) == -1)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     connect_host, g_strerror (errno));
        }
      break;
    }

  if (host.h_addr_list[curhost] == NULL)
    {
      close (sock);
      return (GFTP_ERETRYABLE);
    }

  return (sock);
}


int
gftp_connect_server (gftp_request * request, char *service,
                     char *proxy_hostname, unsigned int proxy_port)
{
  int sock;

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  sock = gftp_connect_server_with_getaddr (request, service, proxy_hostname,
                                           proxy_port);
#else
  sock = gftp_connect_server_legacy (request, service, proxy_hostname,
                                     proxy_port);
#endif

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

