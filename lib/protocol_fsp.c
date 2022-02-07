/***********************************************************************************/
/*  protocol_fsp.c - functions interfacing with  FSP v2 protocol library           */
/*  Copyright (C) 2004 Radim Kolar <hsn@netmag.cz>                                 */
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

#include "fsplib.h"

typedef struct fsp_protocol_data_tag
{
  FSP_SESSION *session;
  FSP_DIR *dir;
  FSP_FILE *file;
  int data_connection;
  // is_copied:
  //   Want to use a single main control connection to the server
  //    so all requests use the FSP session from the main request
  //    (any other approach fails or segfaults, also not using swap_socks)
  unsigned int is_copied : 1;
} fsp_protocol_data;

static void fsp_destroy (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_FSP);
}


static int fsp_connect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;
  intptr_t network_timeout;
  char * server_version;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);

  if (request->datafd > 0) {
      return 0;
  }

  if(! request->port) {
      request->port = gftp_protocol_default_port (request);
  }
  fspdat = (fsp_protocol_data*) request->protocol_data;

  ret = gftp_connect_server (request, "fsp", NULL, 0);
  if (ret < 0) {
      return GFTP_ERETRYABLE;
  }

  fspdat->session = fsp_open_session (request->datafd, request->remote_addr,
                                      request->hostname, request->port, request->password);
  if (fspdat->session == NULL) {
      return (GFTP_ERETRYABLE);
  }
  /* set up network timeout */
  gftp_lookup_request_option (request, "network_timeout", &network_timeout);
  fspdat->session->timeout = 1000*network_timeout;

  if (request->directory && !*request->directory) {
     // empty string, this causes trouble, reset
     g_free (request->directory);
     request->directory = NULL;
  }
  if (!request->directory)
    request->directory = g_strdup ("/");

  request->datafd = fspdat->session->fd;

  // print server version
  server_version = fsp_get_server_version (fspdat->session);
  if (server_version) {
     request->logging_function (gftp_logging_misc, request, "* %s\n", server_version);
     free (server_version);
  }

  return (0);
}


static void fsp_disconnect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_FSP);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_if_fail (fspdat != NULL);

  if (fspdat->file) {
      fsp_fclose (fspdat->file);
      fspdat->file = NULL;
  }
  if (fspdat->dir) {
      fsp_closedir (fspdat->dir);
      fspdat->dir = NULL;
  }

  if (!fspdat->session) {
     DEBUG_MSG("Invalid session\n")
     request->datafd = -1;
     return;
  }
  if (request->datafd < 0) {
     return;
  }
  if (fspdat->is_copied) {
     return; // the session must remain open 
  }

  request->datafd = -1;
  fsp_close_session (fspdat->session);
  fspdat->session = NULL;
  DEBUG_MSG("Session was closed\n")
}


static off_t
fsp_get_file (gftp_request * request, const char *filename,
                off_t startsize)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;
  struct stat sb;

  g_return_val_if_fail (request != NULL,GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP,GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL,GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL,GFTP_EFATAL);

  /* CHECK: close prev. opened file, is this needed? */
  if (fspdat->file != NULL)
  {
       fsp_fclose (fspdat->file);
       fspdat->file = NULL;
  }

  if (fsp_stat (fspdat->session,filename,&sb))
      return (GFTP_ERETRYABLE);
  if(!S_ISREG(sb.st_mode))
      return (GFTP_ERETRYABLE);

  fspdat->file = fsp_fopen (fspdat->session,filename,"rb");

  if (fsp_fseek (fspdat->file, startsize, SEEK_SET) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot seek on file %s: %s\n"),
                                 filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (sb.st_size);
}

static ssize_t fsp_read_function(gftp_request *request, void *buf, size_t size,int fd)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, -1);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, -1);
  g_return_val_if_fail (buf != NULL, -1);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, -1);
  g_return_val_if_fail (fspdat->file != NULL, -1);

  return fsp_fread (buf,1,size,fspdat->file);
}


static ssize_t fsp_write_function(gftp_request *request, const char *buf, size_t size,int fd)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, -1);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, -1);
  g_return_val_if_fail (buf != NULL, -1);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, -1);
  g_return_val_if_fail (fspdat->file != NULL, -1);

  return fsp_fwrite (buf,1,size,fspdat->file);
}

static int
fsp_put_file (gftp_request * request, const char *filename,
                off_t startsize, off_t totalsize)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL,GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL,GFTP_EFATAL);

  if (fspdat->file != NULL)
  {
       fsp_fclose (fspdat->file);
       fspdat->file = NULL;
  }

  if (fsp_canupload (fspdat->session,filename))
  {
        request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot upload file %s\n"),
                                 filename );
        return (GFTP_ERETRYABLE);
  }
      
  fspdat->file = fsp_fopen (fspdat->session,filename, "wb");
  if (fspdat->file == NULL)
  {
        request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot write to file %s: %s\n"),
                                 filename, g_strerror (errno));
        return (GFTP_ERETRYABLE);
  }

  if (fsp_fseek (fspdat->file, startsize, SEEK_SET) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot seek on file %s: %s\n"),
                                 filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }
  return (0);
}


static int fsp_end_transfer (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if (fspdat->dir)
    {
      fsp_closedir (fspdat->dir);
      fspdat->dir = NULL;
    }
  if (fspdat->file)
  {
      if (fsp_fclose (fspdat->file))
      {
         fspdat -> file = NULL;
         request->logging_function (gftp_logging_error, request,
                                 _("Error: Error closing file: %s\n"),
                                 g_strerror (errno));
         return GFTP_EFATAL;
      }
      fspdat->file = NULL;
  }

  return (0);
}


static int fsp_abort_transfer (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if (fspdat->dir)
    {
      fsp_closedir (fspdat->dir);
      fspdat->dir = NULL;
    }
  if (fspdat ->file)
  {
      if (fspdat->file->writing && fspdat->file->pos>0)
      {
         /* need to cancel upload in progress */
         fspdat->file->writing = 0;
         fsp_install (fspdat->session,"",0);
      }
      /* we can safely ignore file close error on abort */
      fsp_fclose (fspdat->file);
      fspdat->file = NULL;
  }

  return (0);
}


static int
fsp_stat_filename (gftp_request * request, const char *filename,
                   mode_t * mode, off_t * filesize)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data * fspdat;
  struct stat st;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if (fsp_stat (fspdat->session,filename, &st) != 0) {
    return (GFTP_ERETRYABLE);
  }
  *mode = st.st_mode;
  *filesize = st.st_size;

  return (0);
}


static int
fsp_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  //DEBUG_PRINT_FUNC
  fsp_protocol_data *fspdat;
  struct FSP_RDENTRY dirent;
  struct FSP_RDENTRY *result;
  char *symlink;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);

  memset (fle, 0, sizeof (*fle));

  result=&dirent;

  if ( fsp_readdir_native (fspdat->dir,&dirent,&result))
    {
      fsp_closedir (fspdat->dir);
      fspdat->dir = NULL;
      request->logging_function (gftp_logging_error, request,
                           _("Corrupted file listing from FSP server %s\n"),
                           request->directory );
      return (GFTP_EFATAL);
    }

  if ( result == NULL)
  {
      fsp_closedir (fspdat->dir);
      fspdat->dir = NULL;
      return 0;
  }
  
  fle->user  = strdup ("-"); /* unknown */
  fle->group = strdup ("-"); /* unknown */
  
  /* turn FSP symlink into normal file */
  symlink=strchr(dirent.name,'\n');
  if (symlink) 
      *symlink='\0';
  fle->file = g_strdup (dirent.name);
  if (dirent.type==FSP_RDTYPE_DIR)
      fle->st_mode = S_IFDIR | 0755;
  else
      fle->st_mode = S_IFREG | 0644;
  fle->datetime = dirent.lastmod;
  fle->size = dirent.size;
  return (1);
}


static int fsp_list_files (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data *fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if ((fspdat->dir = fsp_opendir (fspdat->session,request->directory)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                           _("Could not get FSP directory listing %s: %s\n"),
                           request->directory, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }

  return (0);
}


static off_t 
fsp_get_file_size (gftp_request * request, const char *filename)
{
  DEBUG_PRINT_FUNC
  struct stat st;
  fsp_protocol_data * fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if (fsp_stat (fspdat->session,filename, &st) != 0)
    return (GFTP_ERETRYABLE);

  return (st.st_size);
}


static int
fsp_chdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data *fspdat;
  char *tempstr, *olddir;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);
  
  olddir=NULL;
  /* build new directory string */
  olddir = request->directory;

  if (*directory != '/' && request->directory != NULL)
    {
      tempstr = g_strconcat (request->directory, "/", directory, NULL);
      request->directory = gftp_expand_path (request, tempstr);
      g_free (tempstr);
    }
  else
    request->directory = gftp_expand_path (request, directory);

  if (fsp_getpro (fspdat->session,request->directory,NULL) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                          _("Successfully changed directory to %s\n"),
                          directory);
      if (olddir != NULL)
        g_free (olddir);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                              _("Could not change directory to %s\n"),
                              directory);
      g_free (request->directory);
      request->directory = olddir;
      return (GFTP_ERETRYABLE);
    }
}


static int fsp_delete (gftp_request * request, const char *name, int isdir)
{
   DEBUG_PRINT_FUNC
   fsp_protocol_data *fspdat;
   char * fullpath;
   int ret;

   g_return_val_if_fail (request != NULL, GFTP_EFATAL);
   g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
   g_return_val_if_fail (name != NULL, GFTP_EFATAL);
   fspdat = (fsp_protocol_data*) request->protocol_data;
   g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);
   g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

   fullpath = g_strconcat ((strcmp(request->directory,"/") == 0) ? "" : request->directory,
                           "/", name, NULL);

   if (isdir) {
      ret = fsp_rmdir (fspdat->session, fullpath);
   } else {
      ret = fsp_unlink (fspdat->session, fullpath);
   }

   if (ret == 0) {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully removed %s\n"), fullpath);
      g_free (fullpath);
      return (0);
   } else {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not remove %s: %s\n"),
                                 fullpath, g_strerror (errno));
      g_free (fullpath);
      return (GFTP_ERETRYABLE);
   }
}

static int fsp_removedir (gftp_request * request, const char *directory)
{
   DEBUG_PRINT_FUNC
   return (fsp_delete (request, directory, 1));
}

static int fsp_rmfile (gftp_request * request, const char *file)
{
   DEBUG_PRINT_FUNC
   return (fsp_delete (request, file, 0));
}


static int fsp_makedir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data *fspdat;
  char * fulldir = NULL;
  const char * dir;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);

  if (request->directory && *request->directory) {
     // need to provide full path
     fulldir = g_strconcat (request->directory, "/", directory, NULL);
     dir = fulldir;
  } else {
     dir = directory;
  }

  if (fsp_mkdir (fspdat->session,dir) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully made directory %s\n"), dir);
      if (fulldir) g_free (fulldir);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not make directory %s: %s\n"),
                                 dir, g_strerror (errno));
      if (fulldir) g_free (fulldir);
      return (GFTP_ERETRYABLE);
    }
}


static int
fsp_ren (gftp_request * request, const char *oldname,
	      const char *newname)
{
  DEBUG_PRINT_FUNC
  char *newname1,*newname2;   
  fsp_protocol_data *fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_FSP, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);
  fspdat = (fsp_protocol_data*) request->protocol_data;
  g_return_val_if_fail (fspdat != NULL, GFTP_EFATAL);
  g_return_val_if_fail (fspdat->session != NULL, GFTP_EFATAL);
  
  newname1= g_strconcat (request->directory, "/", oldname, NULL);
  newname2= g_strconcat (request->directory, "/", newname, NULL);

  if (fsp_rename (fspdat->session,newname1, newname2) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully renamed %s to %s\n"),
                                 oldname, newname);
      g_free(newname1);
      g_free(newname2);
      return (0);
    }
  else
    {
      g_free(newname1);
      g_free(newname2);

      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not rename %s to %s: %s\n"),
                                 oldname, newname, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}



static void fsp_copy_param_options (gftp_request * dest, gftp_request * src)
{
   DEBUG_PRINT_FUNC
   fsp_protocol_data *destfsp  = dest->protocol_data;
   fsp_protocol_data *srcfsp = src->protocol_data;
   if (!destfsp->session) {
      DEBUG_PUTS("SESSION IS COPIED")
      destfsp->session = srcfsp->session; // only want the ->session
      destfsp->is_copied = 1;
      dest->datafd = src->datafd;
   } else {
      DEBUG_PUTS("SESSION IS ***NOT*** COPIED")
   }
}


/* TODO: FSP needs to know file last modification time while starting
upload. It can not change last mod time of existing files on server.
*/

void  fsp_register_module (void)
{
  DEBUG_PRINT_FUNC
}

int fsp_init (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  fsp_protocol_data *fspdat;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_PROTOCOL_FSP;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_FSP].url_prefix;

  request->init = fsp_init;
  request->copy_param_options = fsp_copy_param_options;
  request->destroy = fsp_destroy;
  request->read_function = fsp_read_function;
  request->write_function = fsp_write_function;
  request->connect = fsp_connect;
  request->post_connect = NULL;
  request->disconnect = fsp_disconnect;
  request->get_file = fsp_get_file;
  request->put_file = fsp_put_file;
  request->transfer_file = NULL;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = fsp_end_transfer;
  request->abort_transfer = fsp_abort_transfer;
  request->stat_filename = fsp_stat_filename;
  request->list_files = fsp_list_files;
  request->get_next_file = fsp_get_next_file;
  request->get_file_size = fsp_get_file_size;
  request->chdir = fsp_chdir;
  request->rmdir = fsp_removedir;
  request->rmfile = fsp_rmfile;
  request->mkdir = fsp_makedir;
  request->rename = fsp_ren;
  request->chmod = NULL;
  request->set_file_time = NULL;
  request->site = NULL;
  request->parse_url = NULL;
  request->set_config_options = NULL;
  request->swap_socks = NULL;
  request->need_hostport = 1;
  request->need_username = 0;
  request->need_password = 0;
  request->use_cache = 0;
  request->always_connected = 0;
  request->use_local_encoding = 0;
  request->use_udp = 1; /*** UDP ***/

  fspdat = g_malloc0 (sizeof (*fspdat));
  request->protocol_data = fspdat;

  return (gftp_set_config_options (request));
}
