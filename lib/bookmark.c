/*****************************************************************************/
/*  local.c - functions that will use the local system                       */
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

static int bookmark_parse_url 			( gftp_request * request, 
						  const char * url );

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

static int
bookmark_parse_url (gftp_request * request, const char * url)
{
  gftp_bookmarks * tempentry;
  const char * pos;
  int i;

  g_return_val_if_fail (request != NULL, -2);
  g_return_val_if_fail (url != NULL, -2);
  
  if ((pos = strstr (url, "://")) != NULL)
    pos += 3;
  else
    pos = url;

  if ((tempentry = g_hash_table_lookup (bookmarks_htable, pos)) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: Could not find bookmark %s\n"), pos);
      return (-2);
    }
  else if (tempentry->hostname == NULL || *tempentry->hostname == '\0' ||
           tempentry->user == NULL || *tempentry->user == '\0')
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Bookmarks Error: There are some missing entries in this bookmark. Make sure you have a hostname and username\n"));
      return (-2);
    }

  gftp_set_username (request, tempentry->user);
  if (strncmp (tempentry->pass, "@EMAIL@", 7) == 0)
    gftp_set_password (request, emailaddr);
  else
    gftp_set_password (request, tempentry->pass);
  if (tempentry->acct != NULL)
    gftp_set_account (request, tempentry->acct);
  gftp_set_hostname (request, tempentry->hostname);
  gftp_set_directory (request, tempentry->remote_dir);
  gftp_set_port (request, tempentry->port);
  gftp_set_sftpserv_path (request, tempentry->sftpserv_path);

  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (strcmp (gftp_protocols[i].name, tempentry->protocol) == 0)
        {
          gftp_protocols[i].init (request);
          break;
        }
    }

  if (!gftp_protocols[i].name)
    gftp_protocols[0].init (request);

  return (0);
}

