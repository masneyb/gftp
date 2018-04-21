/*****************************************************************************/
/*  sslcommon.c - interface to OpenSSL                                       */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
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
  {"verify_ssl_peer", N_("Verify SSL Peer"),
  gftp_option_type_checkbox, GINT_TO_POINTER(1), NULL, 0,
   N_("Verify SSL Peer"), GFTP_PORT_ALL, NULL},

  {NULL, NULL, 0, NULL, NULL, 0, NULL, 0, NULL}
};  

static GMutex * gftp_ssl_mutexes = NULL;
static volatile int gftp_ssl_initialized = 0;
static SSL_CTX * ctx = NULL;
static GHashTable * gftp_ssl_map = NULL;

struct CRYPTO_dynlock_value
{ 
  GMutex mutex;
};


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

static SSL*
gftp_get_ssl_for_fd (int fd)
{
  SSL* ssl= gftp_ssl_map?(SSL*)g_hash_table_lookup (gftp_ssl_map, &fd):NULL;
  return ssl;
}

static void
gftp_set_ssl_for_fd (int fd, SSL* ssl)
{
  if (ssl)
    {
      int *key = g_new (gint, 1);
      *key = fd;
      g_hash_table_insert (gftp_ssl_map, key, ssl);
    }
  else
    g_hash_table_remove (gftp_ssl_map, &fd);
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
  intptr_t verify_ssl_peer;
  gftp_request * request;
  SSL * ssl;

  ssl = X509_STORE_CTX_get_ex_data (store, SSL_get_ex_data_X509_STORE_CTX_idx ());
  request = SSL_get_ex_data (ssl, gftp_ssl_get_index ());
  gftp_lookup_request_option (request, "verify_ssl_peer", &verify_ssl_peer);

  if (!verify_ssl_peer)
    ok = 1;

  if (!ok)
    {
      X509 *cert = X509_STORE_CTX_get_current_cert (store);
      int depth = X509_STORE_CTX_get_error_depth (store);
      int err = X509_STORE_CTX_get_error (store);

      X509_NAME_oneline (X509_get_issuer_name (cert), issuer, sizeof (issuer));
      X509_NAME_oneline (X509_get_subject_name (cert), subject, sizeof (subject));
      request->logging_function (gftp_logging_error, request,
                                 _("Error with certificate at depth: %i\nIssuer = %s\nSubject = %s\nError %i:%s\n"),
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
  SSL* ssl = gftp_get_ssl_for_fd (request->datafd);
 
  ok = 0;
  if (!(cert = SSL_get_peer_certificate (ssl)))
    {
      request->logging_function (gftp_logging_error, request,
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
              STACK_OF(CONF_VALUE) *val;
              CONF_VALUE   *nval;
              const X509V3_EXT_METHOD *meth;
              void *ext_str = NULL;
 
              if (!(meth = X509V3_EXT_get (ext)))
                break;

	      /*
	       * The *_d2i functions side-effect the input pointer;
	       * thus, we must make a mutable copy of the pointer
	       * rather than passing in the one from the struct.
	       * If we do not do this, it won't fault right away,
	       * but it will fault later when we free the struct,
	       * as the heap would be corrupt.
	       */
	      const unsigned char* mutable_ptr = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
              if (meth->it)
                ext_str = ASN1_item_d2i (NULL, &mutable_ptr, ext->value->length,
                                        ASN1_ITEM_ptr (meth->it));
              else
                ext_str = meth->d2i (NULL, &mutable_ptr, ext->value->length);
#else
              ext_str = meth->d2i(NULL, &mutable_ptr, ext->value->length);
#endif
              val = meth->i2v(meth, ext_str, NULL);

              for (j = 0; j < sk_CONF_VALUE_num(val); j++)
                {
                  nval = sk_CONF_VALUE_value (val, j);
                  if (strcmp (nval->name, "DNS") == 0 && 
                      strcmp (nval->value, request->hostname) == 0)
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

  if (!ok && (subj = X509_get_subject_name (cert)) &&
      X509_NAME_get_text_by_NID (subj, NID_commonName, data, sizeof (data)) > 0)
    {
      data[sizeof (data) - 1] = '\0';
      /* Check for wildcard CN (must begin with *.) */
      if (strncmp (data, "*.", 2) == 0)
        {
          size_t hostname_len = strlen (data) - 1;
          if (strlen (request->hostname) > hostname_len &&
              strcasecmp (&(data[1]), &(request->hostname[strlen (request->hostname) - hostname_len])) == 0)
            ok = 1;
        }
      else if (strcasecmp (data, request->hostname) == 0)
        ok = 1;
      
      if (!ok)
        {
          request->logging_function (gftp_logging_error, request,
                                     _("ERROR: The host in the SSL certificate (%s) does not match the host that we connected to (%s). Aborting connection.\n"),
                                     data, request->hostname);
          X509_free (cert);
          return (X509_V_ERR_APPLICATION_VERIFICATION);
        }
    }
  X509_free (cert);

  return (SSL_get_verify_result (ssl));
}


static void
_gftp_ssl_locking_function (int mode, int n, const char * file, int line)
{
  if (mode & CRYPTO_LOCK)
    g_mutex_lock (&gftp_ssl_mutexes[n]);
  else
    g_mutex_unlock (&gftp_ssl_mutexes[n]);
}


static unsigned long
_gftp_ssl_id_function (void)
{ 
#if GLIB_MAJOR_VERSION > 1
  return ((unsigned long) g_thread_self ());
#else
  /* FIXME - call pthread version. */
  return (0);
#endif
} 


static struct CRYPTO_dynlock_value *
_gftp_ssl_create_dyn_mutex (const char *file, int line)
{ 
  struct CRYPTO_dynlock_value *value;

  value = g_malloc0 (sizeof (*value));
  g_mutex_init (&value->mutex);
  return (value);
}


static void
_gftp_ssl_dyn_mutex_lock (int mode, struct CRYPTO_dynlock_value *l,
                          const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    g_mutex_lock (&l->mutex);
  else
    g_mutex_unlock (&l->mutex);
}


static void
_gftp_ssl_destroy_dyn_mutex (struct CRYPTO_dynlock_value *l,
                             const char *file, int line)
{
  g_mutex_clear (&l->mutex);
  g_free (l);
}


static void
_gftp_ssl_thread_setup (void)
{
  int i;

#if G_MAJOR_VERSION == 1
  /* Thread setup isn't supported in glib 1.2 yet */
  return;
#endif

  gftp_ssl_mutexes = g_malloc0 (CRYPTO_num_locks( ) * sizeof (*gftp_ssl_mutexes));

  for (i = 0; i < CRYPTO_num_locks ( ); i++)
    g_mutex_init (&gftp_ssl_mutexes[i]);

  CRYPTO_set_id_callback (_gftp_ssl_id_function);
  CRYPTO_set_locking_callback (_gftp_ssl_locking_function);
  CRYPTO_set_dynlock_create_callback (_gftp_ssl_create_dyn_mutex);
  CRYPTO_set_dynlock_lock_callback (_gftp_ssl_dyn_mutex_lock);
  CRYPTO_set_dynlock_destroy_callback (_gftp_ssl_destroy_dyn_mutex);
} 


int
gftp_ssl_startup (gftp_request * request)
{
  intptr_t entropy_len;
  char *entropy_source;

  if (gftp_ssl_initialized)
    return (0);

  gftp_ssl_initialized = 1;

  if (g_thread_supported ())
    _gftp_ssl_thread_setup ();

  if (!SSL_library_init ())
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Cannot initialize the OpenSSL library\n"));
      return (GFTP_EFATAL);
    }

  SSL_load_error_strings (); 

  gftp_lookup_request_option (request, "entropy_source", &entropy_source);
  gftp_lookup_request_option (request, "entropy_len", &entropy_len);
  RAND_load_file (entropy_source, entropy_len);

  ctx = SSL_CTX_new (SSLv23_method ());
  if (ctx == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Could not initialize SSL Context\n"));
      return (GFTP_EFATAL);
    }

  if (SSL_CTX_set_default_verify_paths (ctx) != 1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error loading default SSL certificates\n"));
      return (GFTP_EFATAL);
    }

  SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, gftp_ssl_verify_callback);
  SSL_CTX_set_verify_depth (ctx, 9);

  SSL_CTX_set_options (ctx, SSL_OP_ALL|SSL_OP_NO_SSLv2);

  if (SSL_CTX_set_cipher_list (ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH") != 1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error setting cipher list (no valid ciphers)\n"));
      return (GFTP_EFATAL);
    }

  gftp_ssl_map = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);

  return (0);
}

static void
gftp_ssl_abort (gftp_request * request, int fd)
{
  /* disconnect only for errors on the main (control) channel */
  if(fd == request->datafd)
    gftp_disconnect (request);
}

int
gftp_ssl_session_setup_ex (gftp_request * request, int fd)
{
  intptr_t verify_ssl_peer;
  BIO * bio;
  long ret;
  SSL* ssl = gftp_get_ssl_for_fd (fd);

  /* ensure the data socket is open and tls is not yet started on it */
  g_return_val_if_fail (fd > 0, GFTP_EFATAL);
  g_return_val_if_fail (ssl == 0, GFTP_EFATAL);

  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: SSL engine was not initialized\n"));
      gftp_ssl_abort (request, fd);
      return (GFTP_EFATAL);
    }

  /* FIXME - take this out. I need to find out how to do timeouts with the SSL
   * functions (a select() or poll() like function)
   *
   * In the meantime, set the data socket to blocking for tls negotiation
   */
  int non_blocking = gftp_fd_get_sockblocking (request, fd);
  if (non_blocking < 0 || non_blocking == 1 &&
      gftp_fd_set_sockblocking (request, fd, 0) < 0)
    {
      gftp_ssl_abort (request, fd);
      return (GFTP_ERETRYABLE);
    }

  if ((bio = BIO_new (BIO_s_socket ())) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error setting up SSL connection (BIO object)\n"));
      gftp_ssl_abort (request, fd);
      return (GFTP_EFATAL);
    }

  BIO_set_fd (bio, fd, BIO_NOCLOSE);

  if ((ssl = SSL_new (ctx)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error setting up SSL connection (SSL object)\n"));
      gftp_ssl_abort (request, fd);
      return (GFTP_EFATAL);
    }

  SSL_set_bio (ssl, bio, bio);
  SSL_set_ex_data (ssl, gftp_ssl_get_index (), request);

  /*
   * for secondary connections, reuse the session ID from the
   * primary connection.  this is required for FTPS data connections,
   * which must reuse the control channel's session.
   */
  if (fd != request->datafd)
    SSL_set_session (ssl, SSL_get1_session (gftp_get_ssl_for_fd (request->datafd)));

  if (SSL_connect (ssl) <= 0)
    {
      gftp_ssl_abort (request, fd);
      return (GFTP_EFATAL);
    }

  gftp_set_ssl_for_fd (fd, ssl);

  /* perform cert check only on the main (control) channel */
  if (fd == request->datafd)
    {
      gftp_lookup_request_option (request, "verify_ssl_peer", &verify_ssl_peer);

      if (verify_ssl_peer && 
          (ret = gftp_ssl_post_connection_check (request)) != X509_V_OK)
        {
          if (ret != X509_V_ERR_APPLICATION_VERIFICATION)
            request->logging_function (gftp_logging_error, request,
                                       _("Error with peer certificate: %s\n"),
                                       X509_verify_cert_error_string (ret));
          gftp_ssl_abort (request, fd);
          return (GFTP_EFATAL);
        }
    }
  
  request->logging_function (gftp_logging_misc, request,
                             "SSL%s connection established using %s (%s)\n", 
                             fd == request->datafd?"":" data", 
                             SSL_get_cipher_version (ssl), 
                             SSL_get_cipher_name (ssl));

  /* restore the socket's previous blocking state */
  if (non_blocking &&
      (ret = gftp_fd_set_sockblocking (request, fd, 1)) < 0)
    {
      gftp_ssl_abort (request, fd);
      return ret;
    }

  return (0);
}

int
gftp_ssl_session_setup (gftp_request * request)
{
  return gftp_ssl_session_setup_ex (request, request->datafd);
}

void
gftp_ssl_session_close_ex (gftp_request * request, int fd)
{
  SSL* ssl = gftp_get_ssl_for_fd (fd);
  if(ssl)
    {
      SSL_shutdown (ssl);
      SSL_free (ssl);
      gftp_set_ssl_for_fd(fd, NULL);
    }
}

void
gftp_ssl_session_close (gftp_request * request)
{
  gftp_ssl_session_close_ex (request, request->datafd);
}

ssize_t 
gftp_ssl_read (gftp_request * request, void *ptr, size_t size, int fd)
{
  int ret;
  int err;
  SSL* ssl = gftp_get_ssl_for_fd (fd);

  g_return_val_if_fail (ssl != NULL, GFTP_EFATAL);

  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: SSL engine was not initialized\n"));
      return (GFTP_EFATAL);
    }

  errno = 0;
  ret = 0;
  do
    {
      if ((ret = SSL_read (ssl, ptr, size)) < 0)
        { 
          err = SSL_get_error (ssl, ret);
          if (errno == EINTR || errno == EAGAIN)
            {
              if (request->cancel)
                {
                  gftp_ssl_abort (request, fd);
                  return (GFTP_ERETRYABLE);
                }

              continue;
            }
 
          request->logging_function (gftp_logging_error, request,
                                   _("Error: Could not read from socket: %s\n"),
                                    g_strerror (errno));
          gftp_ssl_abort (request, fd);

          return (GFTP_ERETRYABLE);
        }

      break;
    }
  while (1);

  return (ret);
}


ssize_t 
gftp_ssl_write (gftp_request * request, const char *ptr, size_t size, int fd)
{
  int ret, w_ret;
  SSL* ssl = gftp_get_ssl_for_fd (fd);
 
  g_return_val_if_fail (ssl != NULL, GFTP_EFATAL);

  if (!gftp_ssl_initialized)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: SSL engine was not initialized\n"));
      return (GFTP_EFATAL);
    }

  ret = 0;
  do
    {
      w_ret = SSL_write (ssl, ptr, size);
      if (w_ret <= 0)
        {
          int ssl_err = SSL_get_error (ssl, w_ret);
          if (ssl_err == SSL_ERROR_WANT_WRITE ||
              errno == EINTR || errno == EAGAIN)
            {
              if (request != NULL && request->cancel)
                {
                  gftp_ssl_abort (request, fd);
                  return (GFTP_ERETRYABLE);
                }

              continue;
             }
 
          request->logging_function (gftp_logging_error, request,
                                    _("Error: Could not write to socket: %s\n"),
                                    g_strerror (errno));
          gftp_ssl_abort (request, fd);

          return (GFTP_ERETRYABLE);
        }

      ptr += w_ret;
      size -= w_ret;
      ret += w_ret;
    }
  while (size > 0);

  return (ret);
}

#endif
