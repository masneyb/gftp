/*****************************************************************************/
/*  sslcommon.c - interface to OpenSSL                                       */
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
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftp.h"

static const char cvsid[] = "$Id$";

/* Some of the functions in here was taken either entirely or partially from
 * the O'Reilly book Network Security with OpenSSL */

#ifdef USE_SSL

static gftp_config_vars config_vars[] =
{
  {"", N_("SSL Engine"), gftp_option_type_notebook, NULL, NULL, 0, NULL,
   GFTP_PORT_GTK, NULL},

  {"entropy_source", N_("SSL Entropy File:"), 
   gftp_option_type_text, "/dev/urandom", NULL, 0, 
   N_("SSL entropy file"), GFTP_PORT_ALL, 0},
  {"entropy_len", N_("Entropy Seed Length:"), 
   gftp_option_type_int, GINT_TO_POINTER(1024), NULL, 0, 
   N_("The maximum number of bytes to seed the SSL engine with"), 
   GFTP_PORT_ALL, 0},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};  

static SSL_CTX * ctx = NULL;

static volatile int gftp_ssl_initialized = 0;

void
ssl_register_module (void)
{
  static volatile int module_registered = 0;

  if (!module_registered)
    {
      gftp_register_config_vars (config_vars);
      module_registered = 1;
    }
}


static int
gftp_ssl_get_index (void)
{
  static volatile int index = -1;

  if (index < 0)
    index = SSL_get_ex_new_index (0, gftp_version, NULL, NULL, NULL);

  return index;
}



static int 
gftp_ssl_verify_callback (int ok, X509_STORE_CTX *store)
{
  char issuer[256], subject[256];
  gftp_request * request;
  SSL * ssl;

  ssl = X509_STORE_CTX_get_ex_data (store, SSL_get_ex_data_X509_STORE_CTX_idx ());
  request = SSL_get_ex_data (ssl, gftp_ssl_get_index ());

  if (!ok)
    {
      X509 *cert = X509_STORE_CTX_get_current_cert (store);
      int depth = X509_STORE_CTX_get_error_depth (store);
      int err = X509_STORE_CTX_get_error (store);

      X509_NAME_oneline (X509_get_issuer_name (cert), issuer, sizeof (issuer));
      X509_NAME_oneline (X509_get_subject_name (cert), subject, sizeof (subject));
      request->logging_function (gftp_logging_error, request->user_data,
                                 "Error with certificate at depth: %i\nIssuer = %s\nSubject = %s\nError %i:%s\n", 
                                 depth, issuer, subject, err, 
                                 X509_verify_cert_error_string (err));
    }

  return ok;
}


static int
gftp_ssl_post_connection_check (gftp_request * request)
{
  char data[256], *extstr;
  int extcount, ok, i, j;
  X509_EXTENSION *ext;
  X509_NAME *subj;
  X509 *cert;
 
  ok = 0;
  if (!(cert = SSL_get_peer_certificate (request->ssl)))
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot get peer certificate\n"));
      return (X509_V_ERR_APPLICATION_VERIFICATION);
    }
         
  if ((extcount = X509_get_ext_count (cert)) > 0)
    {
      for (i = 0; i < extcount; i++)
        {
          ext = X509_get_ext (cert, i);
          extstr = (char *) OBJ_nid2sn (OBJ_obj2nid (X509_EXTENSION_get_object (ext)));
 
          if (strcmp (extstr, "subjectAltName") == 0)
            {
    unsigned char  *data;
    STACK_OF(CONF_VALUE) *val;
    CONF_VALUE   *nval;
    X509V3_EXT_METHOD *meth;
    void     *ext_str = NULL;
 
    if (!(meth = X509V3_EXT_get(ext)))
     break;
    data = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
    if (meth->it)
     ext_str = ASN1_item_d2i(NULL, &data, ext->value->length,
           ASN1_ITEM_ptr(meth->it));
    else
     ext_str = meth->d2i(NULL, &data, ext->value->length);
#else
    ext_str = meth->d2i(NULL, &data, ext->value->length);
#endif
    val = meth->i2v(meth, ext_str, NULL);
    for (j = 0; j < sk_CONF_VALUE_num(val); j++)
    {
     nval = sk_CONF_VALUE_value(val, j);
     if (strcmp(nval->name, "DNS") == 0 && strcmp(nval->value, request->hostname) == 0)
     {
      ok = 1;
      break;
     }
    }
   }
   if (ok)
    break;
  }
 }
 
/* FIXME
 if (!ok && (subj = X509_get_subject_name (cert)) &&
     X509_NAME_get_text_by_NID (subj, NID_commonName, data, 256) > 0)
   {
     data[sizeof (data) - 1] = '\0';
     if (strcasecmp (data, request->hostname) != 0)
       {
         request->logging_function (gftp_logging_error, request->user_data,
                                    _("The SSL certificate's host %s does not match the host %s that we connected to\n"),
                                    data, request->hostname);
         X509_free (cert);
         return (X509_V_ERR_APPLICATION_VERIFICATION);
       }
   }
*/
 
  X509_free (cert);
  return (SSL_get_verify_result(request->ssl));
}


int
gftp_ssl_startup (gftp_request * request)
{
  char *entropy_source;
  int entropy_len;

  if (gftp_ssl_initialized)
    return (0);

  gftp_ssl_initialized = 1;

  /* FIXME _ thread setup */
  if (!SSL_library_init ())
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Cannot initialized the OpenSSL library\n"));
      return (GFTP_EFATAL);
    }

  SSL_load_error_strings (); 

  gftp_lookup_request_option (request, "entropy_source", &entropy_source);
  gftp_lookup_request_option (request, "entropy_len", &entropy_len);
  RAND_load_file (entropy_source, entropy_len);

  ctx = SSL_CTX_new (SSLv23_method ());

  if (SSL_CTX_set_default_verify_paths (ctx) != 1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error loading default SSL certificates\n"));
      return (GFTP_EFATAL);
    }

  SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, gftp_ssl_verify_callback);
  SSL_CTX_set_verify_depth (ctx, 9);
  SSL_CTX_set_options (ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2);

  if (SSL_CTX_set_cipher_list (ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH") != 1)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error setting cipher list (no valid ciphers)\n"));
      return (GFTP_EFATAL);
    }

  return (0);
}


int
gftp_ssl_session_setup (gftp_request * request)
{
  BIO * bio;
  long ret;

  g_return_val_if_fail (request->datafd > 0, GFTP_EFATAL);

  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: SSL engine was not initialized\n"));
      return (GFTP_EFATAL);
    }

  if (gftp_fd_set_sockblocking (request, request->datafd, 0) < 0) /* FIXME */
    {
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if ((bio = BIO_new (BIO_s_socket ())) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error setting up SSL connection (BIO object)\n"));
      return (GFTP_EFATAL);
    }

  BIO_set_fd (bio, request->datafd, BIO_NOCLOSE);

  if ((request->ssl = SSL_new (ctx)) == NULL)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error setting up SSL connection (SSL object)\n"));
      return (GFTP_EFATAL);
    }

  SSL_set_bio (request->ssl, bio, bio);
  SSL_set_ex_data (request->ssl, gftp_ssl_get_index (), request);

  if (SSL_connect (request->ssl) <= 0)
    return (GFTP_EFATAL);

  if ((ret = gftp_ssl_post_connection_check (request)) != X509_V_OK)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error with peer certificate: %s\n"),
                                 X509_verify_cert_error_string (ret));
      return (GFTP_EFATAL);
    }

  request->logging_function (gftp_logging_misc, request->user_data,
                             "SSL connection established using %s (%s)\n", 
                             SSL_get_cipher_version (request->ssl), 
                             SSL_get_cipher_name (request->ssl));

  return (0);
}


ssize_t 
gftp_ssl_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  ssize_t ret;
  int err;

  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: SSL engine was not initialized\n"));
      return (GFTP_EFATAL);
    }

  errno = 0;
  ret = 0;
  do
    {
      if ((ret = SSL_read (request->ssl, ptr, size)) < 0)
        { 
          err = SSL_get_error (request->ssl, ret);
           printf ("error is %d\n", err);
          if (errno == EINTR)
            {
              if (request != NULL && request->cancel)
                break;
              else
                continue;
             }
 
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                                   _("Error: Could not read from socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }
          return (GFTP_ERETRYABLE);
        }
    }
  while (errno == EINTR && !(request != NULL && request->cancel));

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (ret);
}


ssize_t 
gftp_ssl_write (gftp_request * request, const char *ptr, size_t size, int fd)
{
  size_t ret, w_ret;
 
  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request->user_data,
                                 _("Error: SSL engine was not initialized\n"));
      return (GFTP_EFATAL);
    }

  ret = 0;
  do
    {
      w_ret = SSL_write (request->ssl, ptr, size);
      if (w_ret <= 0)
        {
          if (errno == EINTR)
            {
              if (request != NULL && request->cancel)
                break;
              else
                continue;
             }
 
          if (request != NULL)
            {
              request->logging_function (gftp_logging_error, request->user_data,
                                    _("Error: Could not write to socket: %s\n"),
                                    g_strerror (errno));
              gftp_disconnect (request);
            }
          return (GFTP_ERETRYABLE);
        }
      ptr += w_ret;
      size -= w_ret;
      ret += w_ret;
    }
  while (size > 0);

  if (errno == EINTR && request != NULL && request->cancel)
    {
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (ret);
}

#endif
