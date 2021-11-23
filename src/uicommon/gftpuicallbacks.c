/***********************************************************************************/
/*  UI related functions for gFTP. All of these functions must be reentrant        */
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

#include "gftpui.h"

int gftpui_common_run_mkdir (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  return (gftp_make_directory (cdata->request, cdata->input_string));
}


int gftpui_common_run_rename (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  return (gftp_rename_file (cdata->request, cdata->source_string,
                            cdata->input_string));
}


int gftpui_common_run_site (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC

  return (gftp_site_cmd (cdata->request, cdata->toggled, cdata->input_string));
}


int gftpui_common_run_chdir (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  return (gftp_set_directory (cdata->request, cdata->input_string));
}


int gftpui_common_run_chmod (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  return (gftp_chmod (cdata->request, cdata->source_string,
                      strtol (cdata->input_string, NULL, 10)));
}


int gftpui_common_run_ls (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  int got, matched_filespec, have_dotdot, ret;
  char *sortcol_var, *sortasds_var;
  intptr_t sortcol, sortasds;
  gftp_file * fle;

  ret = gftp_list_files (cdata->request);
  if (ret < 0)
    return (ret);

  have_dotdot = 0;
  cdata->request->gotbytes = 0;
  cdata->files = NULL;
  fle = g_malloc0 (sizeof (*fle));
  while ((got = gftp_get_next_file (cdata->request, NULL, fle)) > 0 ||
         got == GFTP_ERETRYABLE)
    {
      if (cdata->source_string == NULL)
        matched_filespec = 1;
      else
        matched_filespec = gftp_match_filespec (cdata->request, fle->file,
                                                cdata->source_string);

      if (got < 0 || strcmp (fle->file, ".") == 0 || !matched_filespec)
        {
          gftp_file_destroy (fle, 0);
          continue;
        }
      else if (strcmp (fle->file, "..") == 0)
        have_dotdot = 1;

      cdata->request->gotbytes += got;
      cdata->files = g_list_prepend (cdata->files, fle);
      fle = g_malloc0 (sizeof (*fle));
    }
  g_free (fle);

  gftp_end_transfer (cdata->request);
  cdata->request->gotbytes = -1;

  if (!have_dotdot)
    {
      fle = g_malloc0 (sizeof (*fle));
      fle->file = g_strdup ("..");
      fle->user = g_malloc0 (1);
      fle->group = g_malloc0 (1);
      fle->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR;
      cdata->files = g_list_prepend (cdata->files, fle);
    }

  if (cdata->files != NULL)
    {
      if (cdata->request->protonum == GFTP_PROTOCOL_LOCALFS)
        {
          sortasds_var = "local_sortasds";
          sortcol_var = "local_sortcol";
        }
      else
        {
          sortasds_var = "remote_sortasds";
          sortcol_var = "remote_sortcol";
        }

      gftp_lookup_global_option (sortcol_var, &sortcol);
      gftp_lookup_global_option (sortasds_var, &sortasds);
    
      cdata->files = gftp_sort_filelist (cdata->files, sortcol, sortasds);
    }

  return (1);
}


static void
_gftpui_common_del_purge_cache (gpointer key, gpointer value,
                                gpointer user_data)
{
  DEBUG_PRINT_FUNC
  gftp_delete_cache_entry (NULL, key, 0);
  g_free (key);
}


static int
_gftpui_common_rm_list (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  char *tempstr, description[BUFSIZ];
  gftp_file * tempfle;
  GHashTable * rmhash;
  GList * templist;
  int success, ret;

  for (templist = cdata->files;
       templist->next != NULL;
       templist = templist->next); 

  if (cdata->request->use_cache)
    rmhash = g_hash_table_new (string_hash_function, string_hash_compare);
  else
    rmhash = NULL;

  ret = 0;
  for (; templist != NULL; templist = templist->prev)
    { 
      tempfle = templist->data;

      if (S_ISDIR (tempfle->st_mode))
        success = gftp_remove_directory (cdata->request, tempfle->file);
      else
        success = gftp_remove_file (cdata->request, tempfle->file);

      if (success < 0)
        ret = success;
      else if (rmhash != NULL)
        {
          gftp_generate_cache_description (cdata->request, description,
                                           sizeof (description), 0);
          if (g_hash_table_lookup (rmhash, description) == NULL)
            {
              tempstr = g_strdup (description);
              g_hash_table_insert (rmhash, tempstr, NULL);
            }
        }

      if (!GFTP_IS_CONNECTED (cdata->request))
        break;
    }

  if (rmhash != NULL)
    {
      g_hash_table_foreach (rmhash, _gftpui_common_del_purge_cache, NULL);
      g_hash_table_destroy (rmhash);
    }

  return (ret);
}


int gftpui_common_run_delete (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  int ret;

  if (cdata->files != NULL)
    ret = _gftpui_common_rm_list (cdata);
  else
    ret = gftp_remove_file (cdata->request, cdata->input_string);

  return (ret);
}


int gftpui_common_run_rmdir (gftpui_callback_data * cdata)
{
  DEBUG_PRINT_FUNC
  int ret;

  if (cdata->files != NULL)
    ret = _gftpui_common_rm_list (cdata);
  else
    ret = gftp_remove_directory (cdata->request, cdata->input_string);

  return (ret);
}


int gftpui_common_run_connect (gftpui_callback_data * cdata)
{ 
  DEBUG_PRINT_FUNC
  return (gftp_connect (cdata->request));
}

