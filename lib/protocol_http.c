/***********************************************************************************/
/*  protocol_http.c - General purpose routines for the HTTP protocol               */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                        */
/*  Copyright (C) 2021 wdlkmpx <wdlkmpx@gmail.com>                                 */
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

// currently implemented: rfc2068

#include "gftp.h"
#include "protocol_http.h"

#define HTTP11 " HTTP/1.1\r\n"

/*
static gftp_config_vars config_vars[] =
{
   {"", "HTTP", gftp_option_type_notebook, NULL, NULL, 
    GFTP_CVARS_FLAGS_SHOW_BOOKMARK, NULL, GFTP_PORT_GTK, NULL},

   {"http_proxy_host", N_("Proxy hostname:"), 
    gftp_option_type_text, "", NULL, 0, 
    N_("Firewall hostname"), GFTP_PORT_ALL, 0},
   {"http_proxy_port", N_("Proxy port:"), 
    gftp_option_type_int, GINT_TO_POINTER(80), NULL, 0,
    N_("Port to connect to on the firewall"), GFTP_PORT_ALL, NULL},
   {"http_proxy_username", N_("Proxy username:"), 
    gftp_option_type_text, "", NULL, 0,
    N_("Your firewall username"), GFTP_PORT_ALL, NULL},
   {"http_proxy_password", N_("Proxy password:"), 
    gftp_option_type_hidetext, "", NULL, 0,
    N_("Your firewall password"), GFTP_PORT_ALL, NULL},

   {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};
*/

static char * base64_encode (char *str)
{
   /* The standard to Base64 encoding can be found in RFC2045 */
   char *newstr, *newpos, *fillpos, *pos;
   unsigned char table[64], encode[3];
   size_t slen, num;
   int i;

   for (i = 0; i < 26; i++) {
      table[i] = 'A' + i;
      table[i + 26] = 'a' + i;
   }
   for (i = 0; i < 10; i++) {
      table[i + 52] = '0' + i;
   }
   table[62] = '+';
   table[63] = '/';

   slen = strlen (str);
   num = slen / 3;
   if (slen % 3 > 0) {
      num++;
   }
   newstr = (char *) calloc (num * 4 + 1, sizeof(char));
   newstr[num * 4] = '\0';
   newpos = newstr;

   pos = str;
   while (*pos != '\0')
   {
      memset (encode, 0, sizeof (encode));
      for (i = 0; i < 3 && *pos != '\0'; i++)
          encode[i] = *pos++;

      fillpos = newpos;
      *newpos++ = table[encode[0] >> 2];
      *newpos++ = table[(encode[0] & 3) << 4 | encode[1] >> 4];
      *newpos++ = table[(encode[1] & 0xF) << 2 | encode[2] >> 6];
      *newpos++ = table[encode[2] & 0x3F];
      while (i < 3)
          fillpos[++i] = '=';
   }
   return (newstr);
}


static int http_connect (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   char *proxy_hostname = NULL;
   char *proxy_config   = NULL;
   intptr_t proxy_port  = 0;
   int ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (request->hostname != NULL, GFTP_EFATAL);

   if (request->datafd > 0)
      return (0);

   ///gftp_lookup_request_option (request, "proxy_config", &proxy_config);
   ///gftp_lookup_request_option (request, "http_proxy_host", &proxy_hostname);
   ///gftp_lookup_request_option (request, "http_proxy_port", &proxy_port);

   if (proxy_config != NULL && strcmp (proxy_config, "ftp") == 0)
   {
      g_free (request->url_prefix);
      request->url_prefix = g_strdup ("ftp");
   }

   if ((ret = gftp_connect_server (request, request->url_prefix, proxy_hostname, 
                                  proxy_port)) < 0)
      return (ret);

   if (request->directory && *request->directory == '\0')
   {
      g_free (request->directory);
      request->directory = NULL;
   }

   if (!request->directory)
      request->directory = g_strdup ("/");

   return (0);
}


static off_t  http_read_response (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;
   unsigned int chunked;
   char tempstr[8192];
   int ret;

   httpdat = request->protocol_data;
   *tempstr = '\0';
   chunked = 0;
   httpdat->chunk_size = 0;
   httpdat->content_length = 0;
   httpdat->eof = 0;

   if (request->last_ftp_response)
   {
      g_free (request->last_ftp_response); 
      request->last_ftp_response = NULL;
   }

   if (httpdat->extra_read_buffer != NULL)
   {
      g_free (httpdat->extra_read_buffer);
      httpdat->extra_read_buffer = NULL;
      httpdat->extra_read_buffer_len = 0;
   }

   do
   {
      if ((ret = gftp_get_line (request, &httpdat->rbuf, tempstr, 
                                sizeof (tempstr), request->datafd)) < 0)
        return (ret);

      if (request->last_ftp_response == NULL)
         request->last_ftp_response = g_strdup (tempstr);

      if (*tempstr != '\0')
      {
         request->logging_function (gftp_logging_recv, request, "%s\n", tempstr);
         if (strncmp (tempstr, "Content-Length:", 15) == 0)
            httpdat->content_length = gftp_parse_file_size (tempstr + 16);
         else if (strcmp (tempstr, "Transfer-Encoding: chunked") == 0)
            chunked = 1;
      }
   } while (*tempstr != '\0');

   if (chunked)
   {
      if ((ret = gftp_get_line (request, &httpdat->rbuf, tempstr, 
                              sizeof (tempstr), request->datafd)) < 0)
         return (ret);
      if (sscanf (tempstr, GFTP_OFF_T_HEX_PRINTF_MOD, &httpdat->chunk_size) != 1)
      {
         request->logging_function (gftp_logging_recv, request,
                                    _("Received wrong response from server, disconnecting\nInvalid chunk size '%s' returned by the remote server\n"), 
                                    tempstr);
         gftp_disconnect (request);
         return (GFTP_EFATAL);
      }
      if (httpdat->chunk_size == 0) {
         return (0);
      }
      httpdat->chunk_size -= httpdat->rbuf->cur_bufsize;
      if (httpdat->chunk_size < 0)
      {
         httpdat->extra_read_buffer_len = httpdat->chunk_size * -1;
         httpdat->chunk_size = 0;
         httpdat->extra_read_buffer = g_malloc0 ((gulong) httpdat->extra_read_buffer_len + 1);
         memcpy (httpdat->extra_read_buffer, httpdat->rbuf->curpos + (httpdat->rbuf->cur_bufsize - httpdat->extra_read_buffer_len), httpdat->extra_read_buffer_len);
         httpdat->extra_read_buffer[httpdat->extra_read_buffer_len] = '\0';
         httpdat->rbuf->cur_bufsize -= httpdat->extra_read_buffer_len;
         httpdat->rbuf->curpos[httpdat->rbuf->cur_bufsize] = '\0';
      }
   }

   httpdat->chunked_transfer = chunked;
   return (httpdat->content_length > 0 ? httpdat->content_length : httpdat->chunk_size);
}


static off_t http_send_command (gftp_request * request, const char *command)
{
   DEBUG_PRINT_FUNC
   char *tempstr, *str, *proxy_hostname, *proxy_username, *proxy_password;
   tempstr = str = proxy_hostname = proxy_username = proxy_password = NULL;
   ///intptr_t proxy_port = 0;
   int conn_ret;
   ssize_t ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (command != NULL, GFTP_EFATAL);

   if (request->datafd < 0 && (conn_ret = http_connect (request)) != 0)
      return (conn_ret);

   tempstr = g_strdup_printf ("%sUser-Agent: %s\r\nHost: %s\r\nAccept: */*\r\nAccept-Encoding: identity\r\n\r\n",
                             (char *) command, gftp_version, request->hostname);

   request->logging_function (gftp_logging_send, request,
                             "%s", tempstr);

   ret = request->write_function (request, tempstr, strlen (tempstr), 
                                 request->datafd);
   g_free (tempstr);

   if (ret < 0) {
      return (ret);
   }

   ///gftp_lookup_request_option (request, "http_proxy_host", &proxy_hostname);
   ///gftp_lookup_request_option (request, "http_proxy_port", &proxy_port);
   ///gftp_lookup_request_option (request, "http_proxy_username", &proxy_username);
   ///gftp_lookup_request_option (request, "http_proxy_password", &proxy_password);

   if (request->use_proxy && proxy_username != NULL && *proxy_username != '\0')
   {
      tempstr = g_strconcat (proxy_username, ":", proxy_password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request,
                                 "Proxy-authorization: Basic xxxx:xxxx\n");
      ret = gftp_writefmt (request, request->datafd, 
                           "Proxy-authorization: Basic %s\n", str);
      if (str) free (str);
      if (ret < 0)
        return (ret);
   }

   if (request->username != NULL && *request->username != '\0')
   {
      tempstr = g_strconcat (request->username, ":", request->password, NULL);
      str = base64_encode (tempstr);
      g_free (tempstr);

      request->logging_function (gftp_logging_send, request,
                                 "Authorization: Basic xxxx\n");
      ret = gftp_writefmt (request, request->datafd, 
                           "Authorization: Basic %s\n", str);
      if (str) free (str);
      if (ret < 0)
        return (ret);
   }

   if ((ret = request->write_function (request, "\n", 1, request->datafd)) < 0) {
      return (ret);
   }
   return (http_read_response (request));
}


static void http_disconnect (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   g_return_if_fail (request != NULL);

   if (request->datafd > 0)
   {
      request->logging_function (gftp_logging_misc, request,
                                 _("Disconnecting from site %s\n"),
                                 request->hostname);
      if (close (request->datafd) < 0)
        request->logging_function (gftp_logging_error, request,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));

      request->datafd = -1;
   }
}


static off_t http_get_file (gftp_request * request, const char *filename,
                               off_t startsize)
{
   DEBUG_PRINT_FUNC
   char *tempstr, *oldstr, *hf;
   http_protocol_data * httpdat;
   ///intptr_t use_http11 = 1;
   int restarted;
   size_t len;
   off_t size;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

   httpdat = request->protocol_data;
   ///gftp_lookup_request_option (request, "use_http11", &use_http11);

   hf = g_strconcat ("/", filename, NULL);

   if (!request->username || !*request->username) {
      tempstr = g_strconcat ("GET ", hf, HTTP11, NULL);
   } else {
      tempstr = g_strconcat ("GET ", request->username, "@", hf, HTTP11, NULL);
   }
   g_free (hf);

   if (startsize > 0)
   {
      request->logging_function (gftp_logging_misc, request,
                                 _("Starting the file transfer at offset " GFTP_OFF_T_PRINTF_MOD "\n"),
                                 startsize);

      oldstr = tempstr;
      tempstr = g_strdup_printf ("%sRange: bytes=" GFTP_OFF_T_PRINTF_MOD "-\n",
                                 tempstr, startsize);
      g_free (oldstr);
   }

   size = http_send_command (request, tempstr);
   g_free (tempstr);
   if (size < 0) {
      return (size);
   }
   restarted = 0;
   len = strlen (request->last_ftp_response);
   if (len  > 9 && strncmp (request->last_ftp_response + 9, "206", 3) == 0)
      restarted = 1;
   else if (len < 9 || strncmp (request->last_ftp_response + 9, "200", 3) != 0)
   {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot retrieve file %s\n"), filename);
      return (GFTP_ERETRYABLE);
   }

   httpdat->read_bytes = 0;

   return (restarted ? size + startsize : size);
}


static ssize_t http_get_next_file_chunk (gftp_request * request, char *buf, size_t size)
{
   //DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;
   size_t len;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   httpdat = request->protocol_data;
   if (httpdat->rbuf != NULL && httpdat->rbuf->curpos != NULL)
   {
      len = httpdat->rbuf->cur_bufsize > size ? size : httpdat->rbuf->cur_bufsize;
      memcpy (buf, httpdat->rbuf->curpos, len);

      if (len == httpdat->rbuf->cur_bufsize)
         gftp_free_getline_buffer (&httpdat->rbuf);
      else
      {
         httpdat->rbuf->curpos += len;
         httpdat->rbuf->cur_bufsize -= len;
      }

      return (len);
   }

   return (request->read_function (request, buf, size, request->datafd));
}


static int http_end_transfer (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   if (request->datafd < 0) {
      return (GFTP_EFATAL);
   }
   gftp_disconnect (request);

   httpdat = request->protocol_data;
   httpdat->content_length = 0;
   httpdat->chunked_transfer = 0;
   httpdat->chunk_size = 0;
   httpdat->eof = 0;

   return (0);
}


static int http_list_files (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   http_protocol_data *httpdat;
   char *tempstr, *hd;
   ///intptr_t use_http11 = 1;
   off_t ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   httpdat = request->protocol_data;
   ///gftp_lookup_request_option (request, "use_http11", &use_http11);

   if (strncmp (request->directory, "/", strlen (request->directory)) == 0) {
      hd = g_strdup ("/");
   } else {
      hd = g_strconcat ("/", request->directory, "/", NULL);
   }
   if (!request->username || !*request->username) {
      tempstr = g_strconcat ("GET ", hd, HTTP11, NULL);
   } else {
      tempstr = g_strconcat ("GET ", request->username, "@", hd, HTTP11, NULL);
   }
   g_free (hd);

   ret = http_send_command (request, tempstr);
   g_free (tempstr);
   if (ret < 0) {
      return ((int) ret);
   }
   httpdat->read_bytes = 0;
   if (strlen (request->last_ftp_response) > 9 &&
      strncmp (request->last_ftp_response + 9, "200", 3) == 0)
   {
      request->logging_function (gftp_logging_misc, request,
                                 _("Retrieving directory listing...\n"));
      return (0);
   }

   gftp_end_transfer (request);

   return (GFTP_ERETRYABLE);
}


static off_t http_get_file_size (gftp_request * request, const char *filename)
{
   DEBUG_PRINT_FUNC
   char *tempstr, *hf;
   ///intptr_t use_http11 = 1;
   off_t size;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
   ///gftp_lookup_request_option (request, "use_http11", &use_http11);

   hf = g_strconcat ("/", filename, NULL);

   if (!request->username || !*request->username) {
      tempstr = g_strconcat ("HEAD ", hf, HTTP11, NULL);
   } else {
      tempstr = g_strconcat ("HEAD ", request->username, "@", hf, HTTP11, NULL);
   }
   g_free (hf);

   size = http_send_command (request, tempstr);
   g_free (tempstr);
   return (size);
}


static int parse_html_line (char *tempstr, gftp_file * fle)
{
   DEBUG_PRINT_FUNC
   char *stpos, *kpos, *mpos, *pos;
   long units;

   memset (fle, 0, sizeof (*fle));

   if ((pos = strstr (tempstr, "<A HREF=")) == NULL && 
       (pos = strstr (tempstr, "<a href=")) == NULL) {
      return (0);
   }
   /* Find the filename */
   while (*pos != '"' && *pos != '\0') {
      pos++;
   }
   if (*pos == '\0') {
     return (0);
   }
   pos++;

   for (stpos = pos; *pos != '"' && *pos != '\0'; pos++);
   if (*pos == '\0') {
      return (0);
   }
   *pos = '\0';

   /* Copy file attributes. Just about the only thing we can get is whether it
      is a directory or not */
   if (*(pos - 1) == '/')
   {
      fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
      *(pos - 1) = '\0';
   }
   else
      fle->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   /* Copy filename */
   if (strchr (stpos, '/') != NULL || strncmp (stpos, "mailto:", 7) == 0 ||
       *stpos == '\0' || *stpos == '?') {
      return (0);
   }
   fle->file = g_strdup (stpos);

   if (*(pos - 1) == '\0')
      *(pos - 1) = '/';
   *pos = '"';
   pos++;

   /* Skip whitespace and html tags after file and before date */
   stpos = pos;
   if ((pos = strstr (stpos, "</A>")) == NULL &&
       (pos = strstr (stpos, "</a>")) == NULL) {
     return (0);
   }
   pos += 4;

   while (*pos == ' ' || *pos == '\t' || *pos == '.' || *pos == '<')
   {
      if (*pos == '<')
      {
         if (strncmp (pos, "<A ", 3) == 0 || strncmp (pos, "<a ", 3) == 0)
         {
            stpos = pos;
            if ((pos = strstr (stpos, "</A>")) == NULL 
                 && (pos = strstr (stpos, "</a>")) == NULL)
            return (0);
            pos += 4;
         }
         else
         {
            while (*pos != '>' && *pos != '\0')
               pos++;
            if (*pos == '\0')
               return (0);
         }
      }
      pos++;
   }

   if (*pos == '[') {
      pos++;
   }
   fle->datetime = parse_time (pos, &pos);

   fle->user  = g_strdup (_("-")); /* unknown */
   fle->group = g_strdup (_("-")); /* unknown */

   if (pos == NULL) {
      return (1);
   }
   while (*pos == ' ' || *pos == ']') {
      pos++;
   }
   /* Get the size */

   kpos = strchr (pos, 'k');
   mpos = strchr (pos, 'M');

   if (kpos != NULL && (mpos == NULL || (kpos < mpos)))
      stpos = kpos;
   else
      stpos = mpos;

   if (stpos == NULL || !isdigit (*(stpos - 1))) {
      return (1);  /* Return successfully since we got the file */
   }
   if (*stpos == 'k')
      units = 1024;
   else
      units = 1048576;

   fle->size = 0;

   while (*(stpos - 1) != ' ' && *(stpos - 1) != '\t' && stpos > tempstr) 
   {
      stpos--;
      if ((*stpos == '.') && isdigit (*(stpos + 1))) 
      {  /* found decimal point */
         fle->size = units * gftp_parse_file_size (stpos + 1) / 10;;
      }
   }

   fle->size += units * gftp_parse_file_size (stpos);

   return (1);
}


int http_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;
   char tempstr[8192];
   size_t len;
   int ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

   httpdat = request->protocol_data;
   if (request->last_dir_entry)
   {
      g_free (request->last_dir_entry);
      request->last_dir_entry = NULL;
   }

   if (fd < 0)
      fd = request->datafd;

   while (1)
   {
      if ((ret = gftp_get_line (request, &httpdat->rbuf, tempstr, sizeof (tempstr), fd)) <= 0)
         return (ret);

      if (parse_html_line (tempstr, fle) == 0 || fle->file == NULL)
          gftp_file_destroy (fle, 0);
      else
          break;
   }

   if (fle->file == NULL)
   {
      gftp_file_destroy (fle, 0);
      return (0);
   }

   len = strlen (tempstr);
   if (!request->cached)
   {
      request->last_dir_entry = g_strdup_printf ("%s\n", tempstr);
      request->last_dir_entry_len = len + 1;
   }

   return (len);
}


static int http_chdir (gftp_request * request, const char *directory)
{
   DEBUG_PRINT_FUNC
   char *tempstr;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

   if (*directory != '/' && request->directory != NULL)
   {
      tempstr = g_strconcat (request->directory, "/", directory, NULL);
      g_free (request->directory);
      request->directory = gftp_expand_path (request, tempstr);
      g_free (tempstr);
   }
   else
   {
      if (request->directory != NULL) {
         g_free (request->directory);
      }
      request->directory = gftp_expand_path (request, directory);
   }
   return (0);
}


static int http_set_config_options (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   return (0);
}


static void http_destroy (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;

   httpdat = request->protocol_data;

   if (request->url_prefix)
   {
      g_free (request->url_prefix);
      request->url_prefix = NULL;
   }

   if (httpdat->rbuf != NULL) {
      gftp_free_getline_buffer (&httpdat->rbuf);
   }
   if (httpdat->extra_read_buffer != NULL)
   {
      g_free (httpdat->extra_read_buffer);
      httpdat->extra_read_buffer = NULL;
      httpdat->extra_read_buffer_len = 0;
   }
}


static ssize_t http_chunked_read (gftp_request * request, void *ptr, size_t size, int fd)
{
   DEBUG_PRINT_FUNC
   size_t read_size, begin_ptr_len, current_size, crlfsize;
   http_protocol_data * httpdat;
   char *stpos, *crlfpos;
   void *read_ptr_pos;
   ssize_t retval;
   size_t sret;

   httpdat = request->protocol_data;
   httpdat->read_ref_cnt++;

   if (httpdat->extra_read_buffer != NULL)
   {
      g_return_val_if_fail (httpdat->extra_read_buffer_len <= size, GFTP_EFATAL);

      memcpy (ptr, httpdat->extra_read_buffer, httpdat->extra_read_buffer_len);

      begin_ptr_len = httpdat->extra_read_buffer_len;
      read_ptr_pos = (char *) ptr + begin_ptr_len;

      /* Check for end of chunk */
      if (begin_ptr_len > 5 && strncmp ("\r\n0\r\n", (char *) ptr, 5) == 0)
         read_size = 0;
      else
         read_size = size - begin_ptr_len;

      g_free (httpdat->extra_read_buffer);
      httpdat->extra_read_buffer = NULL;
      httpdat->extra_read_buffer_len = 0;
   }
   else
   {
      begin_ptr_len = 0;
      read_ptr_pos = ptr;

      read_size = size;
      if (httpdat->content_length > 0)
      {
         if (httpdat->content_length == httpdat->read_bytes)
         {
             httpdat->read_ref_cnt--;
             return (0);
          }

          if (read_size + httpdat->read_bytes > httpdat->content_length)
             read_size = httpdat->content_length - httpdat->read_bytes;
      }
      else if (httpdat->chunked_transfer && httpdat->chunk_size > 0 &&
               httpdat->chunk_size < read_size)
         read_size = httpdat->chunk_size;
   }

   if (read_size > 0 && !httpdat->eof)
   {
      if (size == read_size)
          read_size--; /* decrement by one so that we can put the NUL character
                          in the buffer */
      retval = httpdat->real_read_function (request, read_ptr_pos, read_size, fd);
      if (retval <= 0)
      {
         httpdat->read_ref_cnt--;
         return (retval);
      }

      httpdat->read_bytes += retval;
      if (httpdat->chunk_size > 0)
      {
         httpdat->chunk_size -= retval;
         httpdat->read_ref_cnt--;
         return (retval);
      }

      sret = retval + begin_ptr_len;
   }
   else
      sret = begin_ptr_len;

   ((char *) ptr)[sret] = '\0';

   if (!httpdat->chunked_transfer)
   {
      httpdat->read_ref_cnt--;
      return (sret);
   }

   stpos = (char *) ptr;
   while (httpdat->chunk_size == 0)
   {
      current_size = sret - (stpos - (char *) ptr);
      if (current_size == 0)
         break;

      if (*stpos == '\r' && *(stpos + 1) == '\n')
         crlfpos = strstr (stpos + 2, "\r\n");
      else
         crlfpos = NULL;

      if (crlfpos == NULL)
      {
         /* The current chunk size is split between multiple packets.
            Save this chunk and read the next */
         httpdat->extra_read_buffer = g_malloc0 ((gulong) current_size + 1);
         memcpy (httpdat->extra_read_buffer, stpos, current_size);
         httpdat->extra_read_buffer[current_size] = '\0';
         httpdat->extra_read_buffer_len = current_size;
         sret -= current_size;

         if (sret == 0)
         {
            /* Don't let a hostile web server send us in an infinite recursive loop */
            if (httpdat->read_ref_cnt > 2)
            {
               request->logging_function (gftp_logging_error, request, _("Received wrong response from server, disconnecting\n"));
               gftp_disconnect (request);
               httpdat->read_ref_cnt--;
               return (GFTP_EFATAL);
            }

            retval = http_chunked_read (request, ptr, size, fd);
            if (retval < 0) {
               return (retval);
             }
             sret = retval;
         }

         httpdat->read_ref_cnt--;
         return (sret);
      }

      *crlfpos = '\0';
      crlfpos++; /* advance to line feed */

      if (sscanf (stpos + 2, GFTP_OFF_T_HEX_PRINTF_MOD,
                  &httpdat->chunk_size) != 1)
      {
         request->logging_function (gftp_logging_recv, request,
                                     _("Received wrong response from server, disconnecting\nInvalid chunk size '%s' returned by the remote server\n"), 
                                     stpos + 2);
         gftp_disconnect (request);
         httpdat->read_ref_cnt--;
         return (GFTP_EFATAL);
      }

      crlfsize = crlfpos - stpos + 1;
      sret -= crlfsize;
      current_size -= crlfsize;

      if (httpdat->chunk_size == 0)
      {
         if (httpdat->eof)
         {
            httpdat->read_ref_cnt--;
            return (0);
         }
         httpdat->eof = 1;
         httpdat->read_ref_cnt--;
         return (sret);
      }

      memmove (stpos, crlfpos + 1, current_size + 1);

      if (httpdat->chunk_size < current_size)
      {
         stpos += httpdat->chunk_size;
         httpdat->chunk_size = 0;
      }
      else
         httpdat->chunk_size -= current_size;
   }

   httpdat->read_ref_cnt--;
   return (sret);
}


void  http_register_module (void)
{
   DEBUG_PRINT_FUNC
   ///gftp_register_config_vars (config_vars);
}


int http_init (gftp_request * request)
{
   DEBUG_PRINT_FUNC
   http_protocol_data * httpdat;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);

   request->protonum = GFTP_PROTOCOL_HTTP;
   request->url_prefix = gftp_protocols[GFTP_PROTOCOL_HTTP].url_prefix;

   request->init = http_init;
   request->copy_param_options = NULL;
   request->read_function = http_chunked_read;
   request->write_function = gftp_fd_write;
   request->destroy = http_destroy;
   request->connect = http_connect;
   request->post_connect = NULL;
   request->disconnect = http_disconnect;
   request->get_file = http_get_file;
   request->put_file = NULL;
   request->transfer_file = NULL;
   request->get_next_file_chunk = http_get_next_file_chunk;
   request->put_next_file_chunk = NULL;
   request->end_transfer = http_end_transfer;
   request->abort_transfer = http_end_transfer; /* NOTE: uses end_transfer */
   request->stat_filename = NULL;
   request->list_files = http_list_files;
   request->get_next_file = http_get_next_file;
   request->get_file_size = http_get_file_size;
   request->chdir = http_chdir;
   request->rmdir = NULL;
   request->rmfile = NULL;
   request->mkdir = NULL;
   request->rename = NULL;
   request->chmod = NULL;
   request->site = NULL;
   request->parse_url = NULL;
   request->swap_socks = NULL;
   request->set_config_options = http_set_config_options;
   request->need_hostport = 1;
   request->need_username = 0;
   request->need_password = 0;
   request->use_cache = 1;
   request->always_connected = 1;
   request->use_local_encoding = 0;

   request->protocol_data = g_malloc0 (sizeof (http_protocol_data));
   httpdat = request->protocol_data;
   httpdat->real_read_function = gftp_fd_read;

   return (gftp_set_config_options (request));
}

