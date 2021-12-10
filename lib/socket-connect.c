/***********************************************************************************/
/*  socket-connect.c - contains functions for connecting to a server               */
/*  Copyright (C) 1998-2008 Brian Masney <masneyb@gftp.org>                        */
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


int sockaddr_get_port (struct sockaddr * saddr)
{
  unsigned short sin_port;
  if (saddr->sa_family == AF_INET) {
     sin_port = (((struct sockaddr_in*)saddr)->sin_port);
  } else { /* ipv6 */
     sin_port = (((struct sockaddr_in6*)saddr)->sin6_port);
  }
  return (ntohs (sin_port));
}

void sockaddr_get_ip_str (struct sockaddr * saddr, char * outbuf, int size)
{
  void * sin_addr;
  if (saddr->sa_family == AF_INET) {
     sin_addr = &(((struct sockaddr_in*)saddr)->sin_addr);
  } else { /* ipv6 */
     sin_addr = &(((struct sockaddr_in6*)saddr)->sin6_addr);
  }
  inet_ntop (saddr->sa_family, sin_addr, outbuf, size);
}


void * sockaddr_get_addr (struct sockaddr * saddr)
{
  void * sin_addr;
  if (saddr->sa_family == AF_INET) {
     sin_addr = &(((struct sockaddr_in*)saddr)->sin_addr);
  } else { /* ipv6 */
     sin_addr = &(((struct sockaddr_in6*)saddr)->sin6_addr);
  }
  return sin_addr;
}

socklen_t sockaddr_get_size (struct sockaddr * saddr)
{
  if (saddr->sa_family == AF_INET) {
     return sizeof(struct sockaddr_in);
  } else { /* ipv6 */
     return sizeof(struct sockaddr_in6);
  }
}

void sockaddr_reset (void * addr)
{
  struct sockaddr * saddr = (struct sockaddr *) addr;
  if (saddr->sa_family == AF_INET) {
     memset (addr, 0, sizeof(struct sockaddr_in));
     saddr->sa_family = AF_INET;
  } else { /* ipv6 */
     memset (addr, 0, sizeof(struct sockaddr_in6));
     saddr->sa_family = AF_INET6;
  }
}

void sockaddr_set_port (struct sockaddr * saddr, int port)
{
  if (saddr->sa_family == AF_INET) {
     ((struct sockaddr_in*)saddr)->sin_port = htons (port);
  } else { /* ipv6 */
     ((struct sockaddr_in6*)saddr)->sin6_port = htons (port);
  }
}

// ====================================================================

static struct addrinfo * lookup_host (gftp_request *request,
                                      char *service,
                                      char *proxy_hostname,
                                      int proxy_port)
{
  DEBUG_PRINT_FUNC
  struct addrinfo hints, *hostp;
  intptr_t enable_ipv6;
  char serv[8], *connect_host;
  int ret, connect_port;

  if (request->use_proxy) {
      connect_host = proxy_hostname;
      connect_port = proxy_port;
  } else {
      connect_host = request->hostname;
      connect_port = request->port;
  }

  gftp_lookup_request_option (request, "enable_ipv6", &enable_ipv6);

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_CANONNAME;

  hints.ai_family = enable_ipv6 ? PF_UNSPEC : AF_INET;
  if (request->use_udp) {
      hints.ai_socktype = SOCK_DGRAM;  /* UDP */
  } else {
      hints.ai_socktype = SOCK_STREAM; /* TCP */
  }
  if (connect_port == 0) {
      strcpy (serv, service); 
  } else {
      snprintf (serv, sizeof (serv), "%d", connect_port);
  }
  request->logging_function (gftp_logging_misc, request, _("Looking up %s\n"), connect_host);

  ret = getaddrinfo (connect_host, serv, &hints, &hostp);
  if (ret != 0)
  {
      request->logging_function (gftp_logging_error, request, _("Unknown host. Maybe you misspelled it?\n"));
      return (NULL);
  }

  return (hostp);
}


static int gftp_do_connect_server (gftp_request * request,
                                   char *service,
                                   char *proxy_hostname,
                                   unsigned int proxy_port)
{
  DEBUG_PRINT_FUNC
  struct addrinfo *res, *hostp, *current_hostp;
  struct sockaddr * saddr;
  char ipstr[128], * hostname;
  int last_errno = 0;
  unsigned int port;
  int sock = -1;
  struct timeval timeout;
  timeout.tv_sec = 30; /* connection timeout in seconds */
  timeout.tv_usec = 0;
  intptr_t use_proxy;

  gftp_lookup_global_option ("ftp_use_proxy", &use_proxy);
  request->use_proxy = use_proxy;

  if (use_proxy) {
     if (!proxy_hostname || !*proxy_hostname) {
        request->logging_function (gftp_logging_error, request,
                                   _("FTP Proxy is enabled but no proxy server has been specified\n"));
        return GFTP_EFATAL;
     }
  }

  hostp = lookup_host (request, service, proxy_hostname, proxy_port);

  if (hostp == NULL) {
      return (GFTP_EFATAL);
  }
  for (res = hostp; res != NULL; res = res->ai_next)
  {
      saddr = res->ai_addr;
      port = sockaddr_get_port (saddr);
      if (!request->use_proxy)
        request->port = port;

      *ipstr = 0;
      hostname = res[0].ai_canonname;
      sockaddr_get_ip_str (saddr, ipstr, sizeof(ipstr));
      if (hostname && (strcmp(hostname, ipstr) == 0)) {
         hostname = ipstr;
      }
      if (!hostname || (hostname == ipstr)) {
         hostname = hostname ? hostname : ipstr;
         request->logging_function (gftp_logging_misc, request,
                                    _("Trying %s:%d\n"), hostname, port);
      } else {
         request->logging_function (gftp_logging_misc, request,
                                    _("Trying %s:%d (%s)\n"), hostname, port, ipstr);
      }

      sock = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
      if (sock < 0)
      {
          request->logging_function (gftp_logging_error, request,
                                     _("Failed to create a socket: %s\n"),
                                     g_strerror (errno));
          continue;
      }

      setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
      if (connect (sock, res->ai_addr, res->ai_addrlen) == -1)
      {
          request->logging_function (gftp_logging_error, request,
                                     _("Cannot connect to %s: %s\n"),
                                     hostname, g_strerror (errno));
          close (sock);
          last_errno = errno;
          continue;
      }

      current_hostp = res;
      break;
  }

  if (res == NULL)
  {
      freeaddrinfo (hostp);
      switch (last_errno)
      {
         case EINVAL:       /* Invalid argument */
         case EHOSTUNREACH: /* No route to host */
            return GFTP_EFATAL;
            break;
         default:
            return (GFTP_ERETRYABLE);
      }
  }

  request->ai_family   = current_hostp->ai_family;
  request->ai_socktype = current_hostp->ai_socktype;

  saddr = current_hostp->ai_addr;
  request->remote_addr_len = sockaddr_get_size (saddr);
  request->remote_addr     = g_malloc0 (request->remote_addr_len);

  memcpy (request->remote_addr, (void*)saddr, request->remote_addr_len);

  request->logging_function (gftp_logging_misc, request,
                             _("Connected to %s:%d\n"), res[0].ai_canonname,
                             port);
  freeaddrinfo (hostp);

  return (sock);
}

// ========================================================================

int gftp_connect_server (gftp_request * request, char *service,
                         char *proxy_hostname, unsigned int proxy_port)
{
  DEBUG_PRINT_FUNC
  int sock;

  sock = gftp_do_connect_server (request, service, proxy_hostname, proxy_port);
  if (sock < 0) {
      return (sock);
  }
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

  if (request->post_connect != NULL) {
     DEBUG_MSG("$ calling request->post_connect()\n")
     return (request->post_connect (request));
  }
  return (0);
}

