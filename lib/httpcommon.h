/*****************************************************************************/
/*  httpcommon.h - common data structures for RFC2068 and HTTPS              */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                  */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

/* $Id$ */

typedef struct rfc2068_params_tag
{
  gftp_getline_buffer * rbuf;
  unsigned long read_bytes,
                content_length,
                chunk_size;
  unsigned int chunked_transfer : 1;
  ssize_t (*real_read_function) ( gftp_request * request,
                                  void *ptr,
                                  size_t size,
                                  int fd );

} rfc2068_params;

int rfc2068_get_next_file 			( gftp_request * request,
						  gftp_file * fle,
						  int fd );


