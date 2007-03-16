/*****************************************************************************/
/*  local.c - functions that will use the local system                       */
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
static const char cvsid[] = "$Id$";

typedef struct local_protocol_data_tag
{
  DIR *dir;
  GHashTable *userhash, *grouphash;
} local_protocol_data;


static void
local_remove_key (gpointer key, gpointer value, gpointer user_data)
{
  g_free (value);
}


static void
local_destroy (gftp_request * request)
{
  local_protocol_data * lpd;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_LOCAL_NUM);

  lpd = request->protocol_data;
  g_hash_table_foreach (lpd->userhash, local_remove_key, NULL);
  g_hash_table_destroy (lpd->userhash);
  g_hash_table_foreach (lpd->grouphash, local_remove_key, NULL);
  g_hash_table_destroy (lpd->grouphash);
  lpd->userhash = lpd->grouphash = NULL;
}


static int
local_getcwd (gftp_request * request)
{
  char tempstr[PATH_MAX], *utf8;
  size_t destlen;
  
  if (request->directory != NULL)
    g_free (request->directory);

  if (getcwd (tempstr, sizeof (tempstr)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Could not get current working directory: %s\n"),
                                 g_strerror (errno));
      request->directory = NULL;
      return (GFTP_ERETRYABLE);
    }

  utf8 = gftp_filename_to_utf8 (request, tempstr, &destlen);
  if (utf8 != NULL)
    request->directory = utf8;
  else
    request->directory = g_strdup (tempstr);

  return (0);
}


static int
local_chdir (gftp_request * request, const char *directory)
{
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  utf8 = gftp_filename_from_utf8 (request, directory, &destlen);
  if (utf8 != NULL)
    {
      ret = chdir (utf8);
      g_free (utf8);
    }
  else
    ret = chdir (directory);

  if (ret == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                          _("Successfully changed local directory to %s\n"),
                          directory);
      ret = local_getcwd (request);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Could not change local directory to %s: %s\n"),
                                 directory, g_strerror (errno));
      ret = GFTP_ERETRYABLE;
    }

  return (ret);
}


static int
local_connect (gftp_request * request)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);

  if (request->directory != NULL)
    return (local_chdir (request, request->directory));
  else
    return (local_getcwd (request));
}


static void
local_disconnect (gftp_request * request)
{
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_LOCAL_NUM);

  if (request->datafd != -1)
    {
      if (close (request->datafd) == -1)
        request->logging_function (gftp_logging_error, request,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));
      request->datafd = -1;
    }
}


static off_t
local_get_file (gftp_request * request, const char *filename,
                off_t startsize)
{
  size_t destlen;
  char *utf8;
  off_t size;
  int flags;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  flags = O_RDONLY;
#if defined (_LARGEFILE_SOURCE) && defined (O_LARGEFILE)
  flags |= O_LARGEFILE;
#endif

  utf8 = gftp_filename_from_utf8 (request, filename, &destlen);
  if (utf8 != NULL)
    {
      request->datafd = gftp_fd_open (request, utf8, flags, 0);
      g_free (utf8);
    }
  else
    request->datafd = gftp_fd_open (request, filename, flags, 0);

  if (request->datafd == -1)
    return (GFTP_ERETRYABLE); 

  if ((size = lseek (request->datafd, 0, SEEK_END)) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot seek on file %s: %s\n"),
                                 filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  if (lseek (request->datafd, startsize, SEEK_SET) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Cannot seek on file %s: %s\n"),
                                 filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }

  return (size);
}


static int
local_put_file (gftp_request * request, const char *filename,
                off_t startsize, off_t totalsize)
{
  int flags, perms;
  size_t destlen;
  char *utf8;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  flags = O_WRONLY | O_CREAT;
  if (startsize > 0)
     flags |= O_APPEND;
#if defined (_LARGEFILE_SOURCE) && defined (O_LARGEFILE)
  flags |= O_LARGEFILE;
#endif

  perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  utf8 = gftp_filename_from_utf8 (request, filename, &destlen);
  if (utf8 != NULL)
    {
      request->datafd = gftp_fd_open (request, utf8, flags, perms);
      g_free (utf8);
    }
  else
    request->datafd = gftp_fd_open (request, filename, flags, perms);

  if (request->datafd == -1)
    return (GFTP_ERETRYABLE);

  if (ftruncate (request->datafd, startsize) == -1)
    {
      request->logging_function (gftp_logging_error, request,
                               _("Error: Cannot truncate local file %s: %s\n"),
                               filename, g_strerror (errno));
      gftp_disconnect (request);
      return (GFTP_ERETRYABLE);
    }
    
  if (lseek (request->datafd, startsize, SEEK_SET) == -1)
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
local_end_transfer (gftp_request * request)
{
  local_protocol_data * lpd;

  lpd = request->protocol_data;
  if (lpd->dir)
    {
      closedir (lpd->dir);
      lpd->dir = NULL;
    }

  if (request->datafd > 0)
    {
      if (close (request->datafd) == -1)
        request->logging_function (gftp_logging_error, request,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));

      request->datafd = -1;
    }

  return (0);
}


static int
local_stat_filename (gftp_request * request, const char *filename,
                     mode_t * mode, off_t * filesize)
{
  struct stat st;
  size_t destlen;
  char *utf8;
  int ret;

  utf8 = gftp_filename_from_utf8 (request, filename, &destlen);
  if (utf8 != NULL)
    {
      ret = stat (utf8, &st);
      g_free (utf8);
    }
  else
    ret = stat (filename, &st);

  if (ret != 0)
    return (GFTP_ERETRYABLE);

  *mode = st.st_mode;
  *filesize = st.st_size;

  return (0);
}


static int
local_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  local_protocol_data * lpd;
  struct stat st, fst;
  struct dirent *dirp;
  char *user, *group;
  struct passwd *pw;
  struct group *gr;

  /* the struct passwd and struct group are not thread safe. But,
     we're ok here because I have threading turned off for the local
     protocol (see use_threads in gftp_protocols in options.h) */
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (fle != NULL, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);

  memset (fle, 0, sizeof (*fle));

  if ((dirp = readdir (lpd->dir)) == NULL)
    {
      closedir (lpd->dir);
      lpd->dir = NULL;
      return (GFTP_EFATAL);
    }

  fle->file = g_strdup (dirp->d_name);
  if (lstat (fle->file, &st) != 0)
    return (GFTP_ERETRYABLE);

  if (stat (fle->file, &fst) != 0)
    return (GFTP_ERETRYABLE);

  if ((user = g_hash_table_lookup (lpd->userhash, 
                                   GUINT_TO_POINTER(st.st_uid))) != NULL)
    fle->user = g_strdup (user);
  else
    {
      if ((pw = getpwuid (st.st_uid)) == NULL)
        fle->user = g_strdup_printf ("%u", st.st_uid); 
      else
        fle->user = g_strdup (pw->pw_name);

      user = g_strdup (fle->user);
      g_hash_table_insert (lpd->userhash, GUINT_TO_POINTER (st.st_uid), user);
    }

  if ((group = g_hash_table_lookup (lpd->grouphash, 
                                    GUINT_TO_POINTER(st.st_gid))) != NULL)
    fle->group = g_strdup (group);
  else
    {
      if ((gr = getgrgid (st.st_gid)) == NULL)
        fle->group = g_strdup_printf ("%u", st.st_gid); 
      else
        fle->group = g_strdup (gr->gr_name);

      group = g_strdup (fle->group);
      g_hash_table_insert (lpd->grouphash, GUINT_TO_POINTER (st.st_gid), group);
    }

  fle->st_dev = fst.st_dev;
  fle->st_ino = fst.st_ino;
  fle->st_mode = fst.st_mode;
  fle->datetime = st.st_mtime;

  if (GFTP_IS_SPECIAL_DEVICE (fle->st_mode))
    fle->size = (off_t) st.st_rdev;
  else
    fle->size = fst.st_size;

  return (1);
}


static int
local_list_files (gftp_request * request)
{
  local_protocol_data *lpd;
  char *dir, *utf8;
  size_t destlen;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);

  if (request->directory[strlen (request->directory) - 1] != '/')
    dir = g_strconcat (request->directory, "/", NULL);
  else
    dir = request->directory;

  utf8 = gftp_filename_from_utf8 (request, dir, &destlen);
  if (utf8 != NULL)
    {
      lpd->dir = opendir (utf8);
      g_free (utf8);
    }
  else
    lpd->dir = opendir (dir);

  if (dir != request->directory)
    g_free (dir);

  if (lpd->dir == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                           _("Could not get local directory listing %s: %s\n"),
                           request->directory, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
  else
    return (0);
}


static off_t 
local_get_file_size (gftp_request * request, const char *filename)
{
  struct stat st;
  size_t destlen;
  char *utf8;
  int ret;

  utf8 = gftp_filename_from_utf8 (request, filename, &destlen);
  if (utf8 != NULL)
    {
      ret = stat (utf8, &st);
      g_free (utf8);
    }
  else
    ret = stat (filename, &st);

  if (ret == -1)
    return (GFTP_ERETRYABLE);

  return (st.st_size);
}


static int
local_rmdir (gftp_request * request, const char *directory)
{
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  utf8 = gftp_filename_from_utf8 (request, directory, &destlen);
  if (utf8 != NULL)
    {
      ret = rmdir (utf8);
      g_free (utf8);
    }
  else
    ret = rmdir (directory);

  if (ret == 0)
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
local_rmfile (gftp_request * request, const char *file)
{
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  utf8 = gftp_filename_from_utf8 (request, file, &destlen);
  if (utf8 != NULL)
    {
      ret = unlink (utf8);
      g_free (utf8);
    }
  else
    ret = unlink (file);

  if (ret == 0)
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
local_mkdir (gftp_request * request, const char *directory)
{
  int ret, perms;
  size_t destlen;
  char *utf8;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  perms = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

  utf8 = gftp_filename_from_utf8 (request, directory, &destlen);
  if (utf8 != NULL)
    {
      ret = mkdir (utf8, perms);
      g_free (utf8);
    }
  else
    ret = mkdir (directory, perms);

  if (ret == 0)
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
local_rename (gftp_request * request, const char *oldname,
	      const char *newname)
{
  const char *conv_oldname, *conv_newname;
  char *old_utf8, *new_utf8;
  size_t destlen;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  old_utf8 = gftp_filename_from_utf8 (request, oldname, &destlen);
  conv_oldname = old_utf8 != NULL ? old_utf8 : oldname;
  new_utf8 = gftp_filename_from_utf8 (request, newname, &destlen);
  conv_newname = new_utf8 != NULL ? new_utf8 : newname;

  if (rename (conv_oldname, conv_newname) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully renamed %s to %s\n"),
                                 oldname, newname);
      ret = 0;
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not rename %s to %s: %s\n"),
                                 oldname, newname, g_strerror (errno));
      ret = GFTP_ERETRYABLE;
    }

  if (old_utf8 != NULL)
    g_free (old_utf8);
  if (new_utf8 != NULL)
    g_free (new_utf8);

  return (ret);
}


static int
local_chmod (gftp_request * request, const char *file, mode_t mode)
{
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  utf8 = gftp_filename_from_utf8 (request, file, &destlen);
  if (utf8 != NULL)
    {
      ret = chmod (utf8, mode);
      g_free (utf8);
    }
  else
    ret = chmod (file, mode);

  if (ret == 0) 
    {
      request->logging_function (gftp_logging_misc, request, 
                                 _("Successfully changed mode of %s to %o\n"),
                                 file, mode);
      return (0);
    }
  else 
    {
      request->logging_function (gftp_logging_error, request, 
                          _("Error: Could not change mode of %s to %o: %s\n"),
                          file, mode, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}


static int
local_set_file_time (gftp_request * request, const char *file,
		     time_t datetime)
{
  struct utimbuf time_buf;
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  time_buf.modtime = datetime;
  time_buf.actime = datetime;

  utf8 = gftp_filename_from_utf8 (request, file, &destlen);
  if (utf8 != NULL)
    {
      ret = utime (utf8, &time_buf);
      g_free (utf8);
    }
  else
    ret = utime (file, &time_buf);

  if (ret == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully changed the time stamp of %s\n"),
                                 file);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not change the time stamp of %s: %s\n"),
                                 file, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}


void 
local_register_module (void)
{
}


int
local_init (gftp_request * request)
{
  local_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_LOCAL_NUM;
  request->init = local_init;
  request->copy_param_options = NULL;
  request->destroy = local_destroy;
  request->read_function = gftp_fd_read;
  request->write_function = gftp_fd_write;
  request->connect = local_connect;
  request->post_connect = NULL;
  request->disconnect = local_disconnect;
  request->get_file = local_get_file;
  request->put_file = local_put_file;
  request->transfer_file = NULL;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = local_end_transfer;
  request->abort_transfer = local_end_transfer; /* NOTE: uses end_transfer */
  request->stat_filename = local_stat_filename;
  request->list_files = local_list_files;
  request->get_next_file = local_get_next_file;
  request->get_next_dirlist_line = NULL;
  request->get_file_size = local_get_file_size;
  request->chdir = local_chdir;
  request->rmdir = local_rmdir;
  request->rmfile = local_rmfile;
  request->mkdir = local_mkdir;
  request->rename = local_rename;
  request->chmod = local_chmod;
  request->set_file_time = local_set_file_time;
  request->site = NULL;
  request->parse_url = NULL;
  request->set_config_options = NULL;
  request->swap_socks = NULL;
  request->url_prefix = "file";
  request->need_hostport = 0;
  request->need_username = 0;
  request->need_password = 0;
  request->use_cache = 0;
  request->always_connected = 1;
  request->use_local_encoding = 1;

  lpd = g_malloc0 (sizeof (*lpd));
  request->protocol_data = lpd;
  lpd->userhash = g_hash_table_new (uint_hash_function, uint_hash_compare);
  lpd->grouphash = g_hash_table_new (uint_hash_function, uint_hash_compare);

  if (request->hostname != NULL)
    g_free (request->hostname);
  request->hostname = g_strdup (_("local filesystem"));

  return (gftp_set_config_options (request));
}

