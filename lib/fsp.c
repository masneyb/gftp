/*****************************************************************************/
/*  fsp.c - functions interfacing with  FSP v2 protocol library              */
/*  Copyright (C) 1998-2003 Brian Masney <masneyb@gftp.org>                  */
/*  Copyright (C) 2004 Radim Kolar <hsn@netmag.cz>                           */
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

#define FSP_USE_SHAREMEM_AND_SEMOP 1
#include "fsplib/fsplib.h"
#include "fsplib/lock.h"

typedef struct fsp_protocol_data_tag
{
  FSP_SESSION *fsp;
  FSP_DIR *dir;
  FSP_FILE *file;
} fsp_protocol_data;

static void
fsp_destroy (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_FSP_NUM);
}

static int
fsp_connect (gftp_request * request)
{
  fsp_protocol_data * lpd;
  intptr_t network_timeout;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);

  if(! request->port)
      request->port = gftp_protocol_default_port (request);

  lpd = request->protocol_data;
  lpd->fsp=fsp_open_session(request->hostname, request->port, request->password);

  if(lpd->fsp == NULL)
    return (GFTP_ERETRYABLE);
  /* set up network timeout */
  gftp_lookup_request_option (request, "network_timeout", &network_timeout);
  lpd->fsp->timeout=1000*network_timeout;

  if (!request->directory)
    request->directory = g_strdup ("/");

  request->datafd = lpd->fsp->fd;
  return (0);
}

static void
fsp_disconnect (gftp_request * request)
{
  fsp_protocol_data * lpd;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_FSP_NUM);

  lpd = request->protocol_data;
  g_return_if_fail (lpd != NULL);
 
  if(lpd->file)
  {
      fsp_fclose(lpd->file);
      lpd->file=NULL;
  }
  fsp_close_session(lpd->fsp);
  lpd->fsp=NULL;
  request->datafd = -1;
  if(lpd->dir)
  {
      fsp_closedir(lpd->dir);
      lpd->dir=NULL;
  }
}

static off_t
fsp_get_file (gftp_request * request, const char *filename, int fd,
                off_t startsize)
{
  fsp_protocol_data * lpd;
  struct stat sb;

  g_return_val_if_fail (request != NULL,GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM,GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL,GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL,GFTP_EFATAL);

  /* CHECK: close prev. opened file, is this needed? */
  if(lpd->file != NULL)
  {
       fsp_fclose(lpd->file);
       lpd->file=NULL;
  }

  if(fsp_stat(lpd->fsp,filename,&sb))
      return (GFTP_ERETRYABLE);
  if(!S_ISREG(sb.st_mode))
      return (GFTP_ERETRYABLE);
  lpd->file=fsp_fopen(lpd->fsp,filename,"rb");

  if (fsp_fseek (lpd->file, startsize, SEEK_SET) == -1)
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
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, -1);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, -1);
  g_return_val_if_fail (buf != NULL, -1);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL, -1);
  g_return_val_if_fail (lpd->file != NULL, -1);

  return fsp_fread(buf,1,size,lpd->file);
}

static ssize_t fsp_write_function(gftp_request *request, const char *buf, size_t size,int fd)
{
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, -1);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, -1);
  g_return_val_if_fail (buf != NULL, -1);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL, -1);
  g_return_val_if_fail (lpd->file != NULL, -1);

  return fsp_fwrite(buf,1,size,lpd->file);
}

static int
fsp_put_file (gftp_request * request, const char *filename, int fd,
                off_t startsize, off_t totalsize)
{
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL,GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL,GFTP_EFATAL);

  if(lpd->file != NULL)
  {
       fsp_fclose(lpd->file);
       lpd->file=NULL;
  }

  if(fsp_canupload(lpd->fsp,filename))
  {
        request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot upload file %s\n"),
                                 filename );
        return (GFTP_ERETRYABLE);
  }

      
  lpd->file=fsp_fopen(lpd->fsp,filename, "wb");
  if(lpd->file == NULL)
  {
        request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot write to file %s: %s\n"),
                                 filename, g_strerror (errno));
        return (GFTP_ERETRYABLE);
  }

  if (fsp_fseek (lpd->file, startsize, SEEK_SET) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot seek on file %s: %s\n"),
                                 filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }
  return (0);
}

static int
fsp_end_transfer (gftp_request * request)
{
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (lpd->dir)
    {
      fsp_closedir (lpd->dir);
      lpd->dir = NULL;
    }
  if (lpd ->file)
  {
      if(fsp_fclose(lpd->file))
      {
	  lpd -> file = NULL;
          request->logging_function (gftp_logging_error, request,
                                 _("Error: Error closing file: %s\n"),
                                 g_strerror (errno));
	  return GFTP_EFATAL;
      }
      lpd->file=NULL;
  }

  return (0);
}

static int
fsp_abort_transfer (gftp_request * request)
{
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (lpd->dir)
    {
      fsp_closedir (lpd->dir);
      lpd->dir = NULL;
    }
  if (lpd ->file)
  {
      if(lpd->file->writing && lpd->file->pos>0)
      {
	  /* need to cancel upload in progress */
	  lpd->file->writing=0;
          fsp_install(lpd->fsp,"",0);
      }
      /* we can safely ignore file close error on abort */
      fsp_fclose(lpd->file);
      lpd->file=NULL;
  }

  return (0);
}

static int
fsp_stat_filename (gftp_request * request, const char *filename,
                   mode_t * mode, off_t * filesize)
{
  fsp_protocol_data * lpd;
  struct stat st;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (fsp_stat (lpd->fsp,filename, &st) != 0)
    return (GFTP_ERETRYABLE);

  *mode = st.st_mode;
  *filesize = st.st_size;

  return (0);
}

static int
fsp_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  fsp_protocol_data *lpd;
  struct FSP_RDENTRY dirent;
  struct FSP_RDENTRY *result;
  char *symlink;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);

  memset (fle, 0, sizeof (*fle));

  result=&dirent;

  if ( fsp_readdir_native (lpd->dir,&dirent,&result))
    {
      fsp_closedir (lpd->dir);
      lpd->dir = NULL;
      request->logging_function (gftp_logging_error, request,
                           _("Corrupted file listing from FSP server %s\n"),
                           request->directory );
      return (GFTP_EFATAL);
    }

  if ( result == NULL)
  {
      fsp_closedir (lpd->dir);
      lpd->dir = NULL;
      return 0;
  }
  
  fle->user = g_strdup (_("unknown"));
  fle->group = g_strdup (_("unknown"));
  
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

static int
fsp_list_files (gftp_request * request)
{
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);

  if (request->directory == NULL)
    {
      request->directory = g_strdup("/");
    }

  if ((lpd->dir = fsp_opendir (lpd->fsp,request->directory)) == NULL)
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
  struct stat st;
  fsp_protocol_data * lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (fsp_stat (lpd->fsp,filename, &st) != 0)
    return (GFTP_ERETRYABLE);

  return (st.st_size);
}

static int
fsp_chdir (gftp_request * request, const char *directory)
{
  fsp_protocol_data *lpd;
  char *tempstr, *olddir;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);
  
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

  if (fsp_getpro (lpd->fsp,request->directory,NULL) == 0)
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

static int
fsp_removedir (gftp_request * request, const char *directory)
{
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (fsp_rmdir (lpd->fsp,directory) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully removed %s\n"), directory);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                              _("Error: Could not remove directory %s: %s\n"),
                              directory, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}


static int
fsp_rmfile (gftp_request * request, const char *file)
{
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (fsp_unlink (lpd->fsp,file) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully removed %s\n"), file);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not remove file %s: %s\n"),
                                 file, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}

static int
fsp_makedir (gftp_request * request, const char *directory)
{
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);

  if (fsp_mkdir (lpd->fsp,directory) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully made directory %s\n"),
                                 directory);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not make directory %s: %s\n"),
                                 directory, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}

static int
fsp_ren (gftp_request * request, const char *oldname,
	      const char *newname)
{
  char *newname1,*newname2;   
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_FSP_NUM, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;
  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);
  g_return_val_if_fail (lpd->fsp != NULL, GFTP_EFATAL);
  
  newname1= g_strconcat (request->directory, "/", oldname, NULL);
  newname2= g_strconcat (request->directory, "/", newname, NULL);

  if (fsp_rename (lpd->fsp,newname1, newname2) == 0)
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

/* TODO: FSP needs to know file last modification time while starting
upload. It can not change last mod time of existing files on server.
*/

void 
fsp_register_module (void)
{
}

int
fsp_init (gftp_request * request)
{
  fsp_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_FSP_NUM;
  request->init = fsp_init;
  request->copy_param_options = NULL;
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
  request->get_next_dirlist_line = NULL;
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
  request->url_prefix = "fsp";
  request->need_hostport = 1;
  request->need_username = 0;
  request->need_password = 0;
  request->use_cache = 1;
  request->always_connected = 0;
  request->use_local_encoding = 0;

  lpd = g_malloc0 (sizeof (*lpd));
  request->protocol_data = lpd;

  return (gftp_set_config_options (request));
}
