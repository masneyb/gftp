/*****************************************************************************/
/*  bookmark.c - functions for connecting to a site via a bookmark           */
/*  Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>                  */
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
static const char cvsid[] = "$Id$";


static int
bookmark_parse_url (gftp_request * request, const char * url)
{
  const char * pos;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (url != NULL, GFTP_EFATAL);
  
  if ((pos = strstr (url, "://")) != NULL)
    {
      pos += 3;
      if (strncmp (url, "bookmark://", 11) != 0)
        {
          request->logging_function (gftp_logging_error, request->user_data,
                                 _("Invalid URL %s\n"), url);
          return (GFTP_EFATAL);
        }
    }
  else
    pos = url;

  return (gftp_parse_bookmark (request, pos));
}


void
bookmark_init (gftp_request * request)
{
  g_return_if_fail (request != NULL);

  request->protonum = GFTP_BOOKMARK_NUM;
  request->init = bookmark_init;
  request->destroy = NULL;
  request->connect = NULL;
  request->disconnect = NULL;
  request->get_file = NULL;
  request->put_file = NULL;
  request->transfer_file = NULL;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = NULL;
  request->list_files = NULL;
  request->get_next_file = NULL;
  request->set_data_type = NULL;
  request->get_file_size = NULL;
  request->chdir = NULL;
  request->rmdir = NULL;
  request->rmfile = NULL;
  request->mkdir = NULL;
  request->rename = NULL;
  request->chmod = NULL;
  request->set_file_time = NULL;
  request->site = NULL;
  request->parse_url = bookmark_parse_url;
  request->url_prefix = "bookmark";
  request->protocol_name = "Bookmark";
  request->need_hostport = 0;
  request->need_userpass = 0;
  request->use_threads = 0;
  request->use_cache = 0;
  request->always_connected = 0;
  gftp_set_config_options (request);
}

