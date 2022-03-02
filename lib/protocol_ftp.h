/***********************************************************************************/
/*  protocol_ftp.h - common data structures for RFC959 and FTPS                    */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                        */
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

enum
{
   FTP_FEAT_MLSD,  /* rfc3659 - use MLSD/MLST insted of LIST */
   FTP_FEAT_PRET,  /* accepts PRET, distibuted FTP server (DrFTP) */
   FTP_FEAT_EPSV,  /* rfc2428 - use EPSV instead of PASV */
   FTP_FEAT_EPRT,  /* rfc2428 - use EPRT instead of PORT */
   /* features enabled by default and disabled if needed */
   FTP_FEAT_SITE,
   FTP_FEAT_LIST_AL,
   FTP_FEAT_TOTAL,
};

struct ftp_supported_feature
{
   int index;
   char * str;
};
extern struct ftp_supported_feature ftp_supported_features[];


struct ftp_protocol_data_tag
{  
  gftp_getline_buffer * dataconn_rbuf;
  char * response_buffer;
  char * response_buffer_pos;
  int data_connection;
  unsigned int is_ascii_transfer : 1;
  unsigned int is_fxp_transfer : 1;
  int (*auth_tls_start) (gftp_request * request);
  int (*data_conn_tls_start) (gftp_request * request);
  ssize_t (*data_conn_read) (gftp_request * request, void *ptr, size_t size,
                             int fd);
  ssize_t (*data_conn_write) (gftp_request * request, const char *ptr,
                              size_t size, int fd);
  void (*data_conn_tls_close) (gftp_request * request);
  int list_dirtype;       /* See FTP_DIRTYPE_* in ftp-dir-listing.c */
  int list_dirtype_hint;  /* See FTP_DIRTYPE_* in ftp-dir-listing.c */
  int last_cmd;     /* 4hackz */
  int last_response_code;
  unsigned int feat[FTP_FEAT_TOTAL];
};

typedef struct ftp_protocol_data_tag ftp_protocol_data;


// =========
// functions
// =========

/* parse-dir-listing.c */
int ftp_parse_ls (gftp_request * request,
                   const char *lsoutput, 
                   gftp_file *fle,
                   int fd);

void parse_syst_response (char * response_str, ftp_protocol_data * ftpdat);


// protocol_ftp.c
int ftp_send_command (gftp_request * request,
                      const char *command,
                      ssize_t command_len,
                      int read_response,
                      int dont_try_to_reconnect );


ssize_t ftp_get_next_dirlist_line (gftp_request * request, int fd,
                                   char *buf, size_t buflen);
