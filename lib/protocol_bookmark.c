/***********************************************************************************/
/*  protocol_bookmark.c - functions for connecting to a site via a bookmark        */
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
          request->logging_function (gftp_logging_error, request,
                                 _("Invalid URL %s\n"), url);
          return (GFTP_EFATAL);
        }
    }
  else
    pos = url;

  return (gftp_parse_bookmark (request, NULL, pos, NULL));
}


void 
bookmark_register_module (void)
{
}


int
bookmark_init (gftp_request * request)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_PROTOCOL_BOOKMARK;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_BOOKMARK].url_prefix;

  request->init = bookmark_init;
  request->read_function = NULL;
  request->write_function = NULL;
  request->destroy = NULL;
  request->connect = NULL;
  request->post_connect = NULL;
  request->disconnect = NULL;
  request->get_file = NULL;
  request->put_file = NULL;
  request->transfer_file = NULL;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = NULL;
  request->list_files = NULL;
  request->get_next_file = NULL;
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
  request->need_hostport = 0;
  request->need_username = 0;
  request->need_password = 0;
  request->use_cache = 0;
  request->always_connected = 0;

  return (gftp_set_config_options (request));
}

