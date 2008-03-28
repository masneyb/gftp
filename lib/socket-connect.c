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

static int
gftp_need_proxy (gftp_request * request, char *service, char *proxy_hostname, 
                 unsigned int proxy_port, void **connect_data)
{
  gftp_config_list_vars * proxy_hosts;
  gftp_proxy_hosts * hostname;
  size_t hostlen, domlen;
  unsigned char addy[4];
  GList * templist;
  gint32 netaddr;
  char *pos;

#if !defined (HAVE_GETADDRINFO) || !defined (HAVE_GAI_STRERROR)
  struct hostent host;
  int ret;
#endif

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);

  if (proxy_hostname == NULL || *proxy_hostname == '\0')
    return (0);
  else if (proxy_hosts->list == NULL)
    return (proxy_hostname != NULL && 
            *proxy_hostname != '\0');

#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)

  *connect_data = lookup_host_with_getaddrinfo (request, service,
                                                proxy_hostname, proxy_port);
  if (*connect_data == NULL)
    return (GFTP_ERETRYABLE);

#else /* !HAVE_GETADDRINFO */

  ret = lookup_host_with_gethostbyname (request, proxy_hostname, &host);
  if (ret != 0)
    return (ret);

  connect_data = NULL; /* FIXME */

#endif

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
#if defined (HAVE_GETADDRINFO) && defined (HAVE_GAI_STRERROR)
  sock = gftp_connect_server_with_getaddrinfo (request, service, proxy_hostname,
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

