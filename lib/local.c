/*****************************************************************************/
/*  local.c - functions that will use the local system                       */
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
local_connect (gftp_request * request)
{
  char tempstr[PATH_MAX];

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);

  if (request->directory)
    {
      if (chdir (request->directory) != 0)
        {
          request->logging_function (gftp_logging_error, request,
                             _("Could not change local directory to %s: %s\n"),
                             request->directory, g_strerror (errno));
        }
      g_free (request->directory);
      request->directory = NULL;
    }

  if (getcwd (tempstr, sizeof (tempstr)) != NULL)
    {
      tempstr[sizeof (tempstr) - 1] = '\0';
      request->directory = g_strdup (tempstr);
    }
  else
    request->logging_function (gftp_logging_error, request,
                             _("Could not get current working directory: %s\n"),
                             g_strerror (errno));

  return (0);
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
local_get_file (gftp_request * request, const char *filename, int fd,
                off_t startsize)
{
  off_t size;
  int flags;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  if (fd <= 0)
    {
      flags = O_RDONLY;
#if defined (_LARGEFILE_SOURCE) && defined (O_LARGEFILE)
      flags |= O_LARGEFILE;
#endif

      if ((request->datafd = gftp_fd_open (request, filename, flags, 0)) == -1)
        return (GFTP_ERETRYABLE); 
    }
  else
    request->datafd = fd;

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
local_put_file (gftp_request * request, const char *filename, int fd,
                off_t startsize, off_t totalsize)
{
  int flags;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (filename != NULL, GFTP_EFATAL);

  if (fd <= 0)
    {
      flags = O_WRONLY | O_CREAT;
      if (startsize > 0)
         flags |= O_APPEND;
#if defined (_LARGEFILE_SOURCE) && defined (O_LARGEFILE)
      flags |= O_LARGEFILE;
#endif

      if ((request->datafd = gftp_fd_open (request, filename, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1)
        return (GFTP_ERETRYABLE);
    }
  else
    request->datafd = fd;

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


static mode_t
local_stat_filename (gftp_request * request, const char *filename)
{
  struct stat st;

  if (stat (filename, &st) != 0)
    return (GFTP_ERETRYABLE);

  return (st.st_mode);
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
  char *tempstr;
  int freeit;

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->directory != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);

  lpd = request->protocol_data;

  g_return_val_if_fail (lpd != NULL, GFTP_EFATAL);

  if (request->directory[strlen (request->directory) - 1] != '/')
    {
      tempstr = g_strconcat (request->directory, "/", NULL);
      freeit = 1;
    }
  else
    {
      tempstr = request->directory;
      freeit = 0;
    }

  if ((lpd->dir = opendir (tempstr)) == NULL)
    {
      request->logging_function (gftp_logging_error, request,
                           _("Could not get local directory listing %s: %s\n"),
                           tempstr, g_strerror (errno));
      if (freeit)
        g_free (tempstr);
      return (GFTP_ERETRYABLE);
    }

  if (freeit)
    g_free (tempstr);

  return (0);
}


static off_t 
local_get_file_size (gftp_request * request, const char *filename)
{
  struct stat st;

  if (stat (filename, &st) == -1)
    return (GFTP_ERETRYABLE);
  return (st.st_size);
}


static int
local_chdir (gftp_request * request, const char *directory)
{
  char tempstr[255];

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (chdir (directory) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                          _("Successfully changed local directory to %s\n"),
                          directory);

      if (request->directory != directory)
        {
          if (getcwd (tempstr, sizeof (tempstr)) == NULL)
            {
              request->logging_function (gftp_logging_error, request,
                             _("Could not get current working directory: %s\n"),
                             g_strerror (errno));
	      return (GFTP_ERETRYABLE);
            }

          if (request->directory)
            g_free (request->directory);
          request->directory = g_strdup (tempstr);
        }

      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                              _("Could not change local directory to %s: %s\n"),
                              directory, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}


static int
local_rmdir (gftp_request * request, const char *directory)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (rmdir (directory) == 0)
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
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  if (unlink (file) == 0)
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
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (directory != NULL, GFTP_EFATAL);

  if (mkdir (directory, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0)
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
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (oldname != NULL, GFTP_EFATAL);
  g_return_val_if_fail (newname != NULL, GFTP_EFATAL);

  if (rename (oldname, newname) == 0)
    {
      request->logging_function (gftp_logging_misc, request,
                                 _("Successfully renamed %s to %s\n"),
                                 oldname, newname);
      return (0);
    }
  else
    {
      request->logging_function (gftp_logging_error, request,
                                 _("Error: Could not rename %s to %s: %s\n"),
                                 oldname, newname, g_strerror (errno));
      return (GFTP_ERETRYABLE);
    }
}


static int
local_chmod (gftp_request * request, const char *file, mode_t mode)
{
  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  if (chmod (file, mode) == 0) 
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

  g_return_val_if_fail (request != NULL, GFTP_EFATAL);
  g_return_val_if_fail (request->protonum == GFTP_LOCAL_NUM, GFTP_EFATAL);
  g_return_val_if_fail (file != NULL, GFTP_EFATAL);

  time_buf.modtime = time_buf.actime = datetime;
  return (utime (file, &time_buf) == 0 ? 0 : GFTP_ERETRYABLE);
}


static gint
hash_compare (gconstpointer path1, gconstpointer path2)
{
  return (GPOINTER_TO_UINT (path1) == GPOINTER_TO_UINT (path2));
}


static guint
hash_function (gconstpointer key)
{
  return (GPOINTER_TO_UINT (key));
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
  request->need_userpass = 0;
  request->use_cache = 0;
  request->always_connected = 1;

  lpd = g_malloc0 (sizeof (*lpd));
  request->protocol_data = lpd;
  lpd->userhash = g_hash_table_new (hash_function, hash_compare);
  lpd->grouphash = g_hash_table_new (hash_function, hash_compare);

  if (request->hostname != NULL)
    g_free (request->hostname);
  request->hostname = g_strdup (_("local filesystem"));

  return (gftp_set_config_options (request));
}

