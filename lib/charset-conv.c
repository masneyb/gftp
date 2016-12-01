/*****************************************************************************/
/*  charset-conv.c - contains functions for performing conversions between   */
/*     character sets.                                                       */
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

#if GLIB_MAJOR_VERSION > 1

static /*@null@*/ char *
_gftp_get_next_charset (char **curpos)
{
  char *ret, *endpos;
  size_t len, retlen;

  if (**curpos == '\0')
    return (NULL);

  for (; **curpos == ' ' || **curpos == '\t'; (*curpos)++);

  if ((endpos = strchr (*curpos, ',')) == NULL)
    len = strlen (*curpos) - 1; /* the trailing ',' should be omitted */
  else
    len = endpos - *curpos;

  for (retlen = len - 1;
       (*curpos)[retlen - 1] == ' ' || (*curpos)[retlen - 1] == '\t';
       retlen--);

  retlen++; /* Needed due to the len - 1 above... */
  ret = g_malloc0 (retlen + 1);
  memcpy (ret, *curpos, retlen);

  for (*curpos += len; **curpos == ','; (*curpos)++);

  return (ret);
}


static void
_do_show_iconv_error (const char *str, char *charset, int from_utf8,
                      GError * error)
{
  const char *fromset, *toset;

  if (from_utf8)
    {
      fromset = "UTF-8";
      toset = charset;
    }
  else
    {
      fromset = charset;
      toset = "UTF-8";
    }

  printf (_("Error converting string '%s' from character set %s to character set %s: %s\n"),
          str, fromset, toset, error->message);
}


/*@null@*/ static char *
_do_convert_string (gftp_request * request, int is_filename, int force_local,
                    const char *str, size_t *dest_len, int from_utf8)
{
  char *remote_charsets, *ret, *fromset, *toset, *stpos, *cur_charset;
  GError * error;
  gsize bread;

  if (request == NULL)
    return (NULL);

  if (g_utf8_validate (str, -1, NULL) != from_utf8)
    return (NULL);

  error = NULL;
  gftp_lookup_request_option (request, "remote_charsets", &remote_charsets);
  if (*remote_charsets == '\0' || request->use_local_encoding ||
      force_local == 1)
    {
      if (from_utf8)
        {
          if (is_filename)
            ret = g_filename_from_utf8 (str, -1, &bread, dest_len, &error);
          else
            ret = g_locale_from_utf8 (str, -1, &bread, dest_len, &error);
        }
      else
        {
          if (is_filename)
            ret = g_filename_to_utf8 (str, -1, &bread, dest_len, &error);
          else
            ret = g_locale_to_utf8 (str, -1, &bread, dest_len, &error);
        }

      if (ret == NULL)
        _do_show_iconv_error (str, request->iconv_charset, from_utf8, error);

      return (ret);
    }

  if (from_utf8)
    {
      if (request->iconv_from_initialized)
        {
          ret = g_convert_with_iconv (str, -1, request->iconv_from, &bread, dest_len,
                                      &error);
          if (ret == NULL)
            _do_show_iconv_error (str, request->iconv_charset, from_utf8, error);

          return (ret);
        }
    }
  else
    {
      if (request->iconv_to_initialized)
        {
          ret = g_convert_with_iconv (str, -1, request->iconv_to, &bread, dest_len,
                                      &error);
          if (ret == NULL)
            _do_show_iconv_error (str, request->iconv_charset, from_utf8, error);

          return (ret);
        }
    }

  stpos = remote_charsets;
  while ((cur_charset = _gftp_get_next_charset (&stpos)) != NULL)
    {
      if (from_utf8)
        {
          fromset = "UTF-8";
          toset = cur_charset;
          if ((request->iconv_from = g_iconv_open (toset, fromset)) == (GIConv) -1)
            {
              g_free (cur_charset);
              continue;
            }

          error = NULL;
          if ((ret = g_convert_with_iconv (str, -1, request->iconv_from, &bread,
                                           dest_len, &error)) == NULL)
            {
              g_iconv_close (request->iconv_from);
              request->iconv_from = NULL;
              _do_show_iconv_error (str, cur_charset, from_utf8, error);
              g_free (cur_charset);
              continue;
            }

          request->iconv_from_initialized = 1;
        }
      else
        {
          fromset = cur_charset;
          toset = "UTF-8";
          if ((request->iconv_to = g_iconv_open (toset, fromset)) == (GIConv) -1)
            {
              g_free (cur_charset);
              continue;
            }

          error = NULL;
          if ((ret = g_convert_with_iconv (str, -1, request->iconv_to, &bread,
                                           dest_len, &error)) == NULL)
            {
              g_iconv_close (request->iconv_to);
              request->iconv_to = NULL;
              _do_show_iconv_error (str, cur_charset, from_utf8, error);
              g_free (cur_charset);
              continue;
            }

          request->iconv_to_initialized = 1;
        }

      request->iconv_charset = cur_charset;
      return (ret);
    }

  return (NULL);
}

char *
gftp_string_to_utf8 (gftp_request * request, const char *str, size_t *dest_len)
{
  return (_do_convert_string (request, 0, 0, str, dest_len, 0));
}


char *
gftp_string_from_utf8 (gftp_request * request, int force_local, const char *str,
                       size_t *dest_len)
{
  return (_do_convert_string (request, 0, force_local, str, dest_len, 1));
}


char *
gftp_filename_to_utf8 (gftp_request * request, const char *str,
                       size_t *dest_len)
{
  return (_do_convert_string (request, 1, 0, str, dest_len, 0));
}


char *
gftp_filename_from_utf8 (gftp_request * request, const char *str,
                         size_t *dest_len)
{
  return (_do_convert_string (request, 1, 0, str, dest_len, 1));
}

#else

char *
gftp_string_to_utf8 (gftp_request * request, const char *str, size_t dest_len)
{
  return (NULL);
}


char *
gftp_string_from_utf8 (gftp_request * request, int force_local, const char *str,
                       size_t dest_len)
{
  return (NULL);
}


char *
gftp_filename_to_utf8 (gftp_request * request, const char *str, size_t dest_len)
{
  return (NULL);
}


char *
gftp_filename_from_utf8 (gftp_request * request, int force_local,
                         const char *str, size_t dest_len)
{
  return (NULL);
}

#endif


