/***********************************************************************************/
/*  protocol_localfs.c - functions that will use the local system                  */
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

typedef struct localfs_protocol_data_tag
{
  DIR *dir;
  GHashTable *userhash, *grouphash;
} localfs_protocol_data;


static void localfs_remove_key (gpointer key, gpointer value, gpointer user_data)
{
  g_free (value);
}


static void localfs_destroy (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  localfs_protocol_data * lpd;

  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS);

  lpd = request->protocol_data;
  g_hash_table_foreach (lpd->userhash, localfs_remove_key, NULL);
  g_hash_table_destroy (lpd->userhash);
  g_hash_table_foreach (lpd->grouphash, localfs_remove_key, NULL);
  g_hash_table_destroy (lpd->grouphash);
  lpd->userhash = lpd->grouphash = NULL;
}


static int localfs_getcwd (gftp_request * request)
{
  DEBUG_PRINT_FUNC
#ifdef __GNU__
  char *tempstr;
#else
  char tempstr[PATH_MAX];
#endif
  char *utf8;
  size_t destlen;
  
  if (request->directory != NULL)
    g_free (request->directory);

#ifdef __GNU__
  tempstr = get_current_dir_name();
  if (tempstr == NULL)
#else
  if (getcwd (tempstr, sizeof (tempstr)) == NULL)
#endif
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

#ifdef __GNU__
  free(tempstr);
#endif
  return (0);
}


static int localfs_chdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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
      ret = localfs_getcwd (request);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Could not change local directory to %s: %s\n"),
                                 directory, g_strerror (errno));
      ret = GFTP_ECANIGNORE;
    }

  return (ret);
}


static int localfs_connect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);

  if (request->directory != NULL)
    return (localfs_chdir (request, request->directory));
  else
    return (localfs_getcwd (request));
}


static void localfs_disconnect (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  g_return_if_fail (request != NULL);
  g_return_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS);

  if (request->datafd != -1)
    {
      if (close (request->datafd) == -1)
        request->logging_function (gftp_logging_error, request,
                                   _("Error closing file descriptor: %s\n"),
                                   g_strerror (errno));
      request->datafd = -1;
    }
}


static off_t localfs_get_file (gftp_request * request, const char *filename,
                             off_t startsize)
{
  DEBUG_PRINT_FUNC
  size_t destlen;
  char *utf8;
  off_t size;
  int flags;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_put_file (gftp_request * request, const char *filename,
                           off_t startsize, off_t totalsize)
{
  DEBUG_PRINT_FUNC
  int flags, perms;
  size_t destlen;
  char *utf8;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_end_transfer (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  localfs_protocol_data * lpd;

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


static int localfs_stat_filename (gftp_request * request, const char *filename,
                                mode_t * mode, off_t * filesize)
{
  DEBUG_PRINT_FUNC
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


static int localfs_get_next_file (gftp_request * request, gftp_file * fle, int fd)
{
  //DEBUG_PRINT_FUNC
  localfs_protocol_data * lpd;
  struct stat st, fst;
  struct dirent *dirp;
  char *user, *group;
  struct passwd *pw;
  struct group *gr;

  /* the struct passwd and struct group are not thread safe. But,
     we're ok here because I have threading turned off for the local
     protocol (see use_threads in gftp_protocols in options.h) */
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_list_files (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  localfs_protocol_data *lpd;
  char *dir, *utf8;
  size_t destlen;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);

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
      return (GFTP_ECANIGNORE);
    }
  else
    return (0);
}


static off_t  localfs_get_file_size (gftp_request * request, const char *filename)
{
  DEBUG_PRINT_FUNC
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


static int localfs_rmdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_rmfile (gftp_request * request, const char *file)
{
  DEBUG_PRINT_FUNC
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_mkdir (gftp_request * request, const char *directory)
{
  DEBUG_PRINT_FUNC
  int ret, perms;
  size_t destlen;
  char *utf8;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_rename (gftp_request * request, const char *oldname,
                         const char *newname)
{
  DEBUG_PRINT_FUNC
  const char *conv_oldname, *conv_newname;
  char *old_utf8, *new_utf8;
  size_t destlen;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_chmod (gftp_request * request, const char *file, mode_t mode)
{
  DEBUG_PRINT_FUNC
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


static int localfs_set_file_time (gftp_request * request, const char *file,
                                time_t datetime)
{
  DEBUG_PRINT_FUNC
  struct utimbuf time_buf;
  size_t destlen;
  char *utf8;
  int ret;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_PROTOCOL_LOCALFS, GFTP_EFATAL);
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


void  localfs_register_module (void)
{
  DEBUG_PRINT_FUNC
}


int localfs_init (gftp_request * request)
{
  DEBUG_PRINT_FUNC
  localfs_protocol_data *lpd;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);

  request->protonum = GFTP_PROTOCOL_LOCALFS;
  request->url_prefix = gftp_protocols[GFTP_PROTOCOL_LOCALFS].url_prefix;

  request->init = localfs_init;
  request->copy_param_options = NULL;
  request->destroy = localfs_destroy;
  request->read_function = gftp_fd_read;
  request->write_function = gftp_fd_write;
  request->connect = localfs_connect;
  request->post_connect = NULL;
  request->disconnect = localfs_disconnect;
  request->get_file = localfs_get_file;
  request->put_file = localfs_put_file;
  request->transfer_file = NULL;
  request->get_next_file_chunk = NULL;
  request->put_next_file_chunk = NULL;
  request->end_transfer = localfs_end_transfer;
  request->abort_transfer = localfs_end_transfer; /* NOTE: uses end_transfer */
  request->stat_filename = localfs_stat_filename;
  request->list_files = localfs_list_files;
  request->get_next_file = localfs_get_next_file;
  request->get_file_size = localfs_get_file_size;
  request->chdir = localfs_chdir;
  request->rmdir = localfs_rmdir;
  request->rmfile = localfs_rmfile;
  request->mkdir = localfs_mkdir;
  request->rename = localfs_rename;
  request->chmod = localfs_chmod;
  request->set_file_time = localfs_set_file_time;
  request->site = NULL;
  request->parse_url = NULL;
  request->set_config_options = NULL;
  request->swap_socks = NULL;
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

