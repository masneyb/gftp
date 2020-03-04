/*****************************************************************************/
/*  dnd.c - drag and drop functions                                          */
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

#include "gftp-gtk.h"


static int
dnd_remote_file (gftp_window_data * wdata, GList ** trans_list, char *url)
{
  gftp_request * current_ftpdata;
  gftp_window_data * fromwdata;
  gftp_transfer * tdata;
  gftp_file * newfle;
  GList * templist;
  char *pos;

  if (wdata == &window1)
    fromwdata = &window2;
  else if (wdata == &window2)
    fromwdata = &window1;
  else 
    fromwdata = NULL;
    
  newfle = g_malloc0 (sizeof (*newfle));
  newfle->shown = 1;
  if (url[strlen (url) - 1] == '/') 
    {
      newfle->st_mode |= S_IFDIR;
      url[strlen (url) - 1] = '\0';
    }

  current_ftpdata = gftp_request_new ();
  current_ftpdata->logging_function = ftp_log;

  if (gftp_parse_url (current_ftpdata, url) != 0 ||
      current_ftpdata->directory == NULL ||
      (pos = strrchr (current_ftpdata->directory, '/')) == NULL) 
    {
      gftp_request_destroy (current_ftpdata, 1);
      gftp_file_destroy (newfle, 1);
      return (0);
    }

  *pos++ = '\0';
  if (compare_request (current_ftpdata, wdata->request, 1))
    {
      gftp_request_destroy (current_ftpdata, 1);
      gftp_file_destroy (newfle, 1);
      return (0);
    }

  if (fromwdata != NULL && 
      compare_request (current_ftpdata, fromwdata->request, 0))
    {
      if (fromwdata->request->password != NULL)
        gftp_set_password (current_ftpdata, fromwdata->request->password);
    }
  else
    fromwdata = NULL;

  *(pos - 1) = '/';
  newfle->file = g_strdup (current_ftpdata->directory);
  *(pos - 1) = '\0';
  
  newfle->destfile = gftp_build_path (wdata->request,
                                      wdata->request->directory, pos, NULL);

  tdata = NULL;
  for (templist = *trans_list; templist != NULL; templist = templist->next)
    {
      tdata = templist->data;
      if (compare_request (current_ftpdata, tdata->fromreq, 0))
        break;
    }

  if (templist == NULL || tdata == NULL)
    {
      tdata = gftp_tdata_new ();

      tdata->fromreq = current_ftpdata;
      tdata->fromwdata = fromwdata;

      tdata->toreq = wdata->request;
      tdata->towdata = wdata;

      *trans_list = g_list_append (*trans_list, tdata);
    }
  else
    gftp_request_destroy (current_ftpdata, 1);

  tdata->files = g_list_append (tdata->files, newfle);

  return (1);
}


void
openurl_get_drag_data (GtkWidget * widget, GdkDragContext * context, gint x,
		       gint y, GtkSelectionData * selection_data, guint info,
		       guint clk_time, gpointer data)
{
  if (current_wdata->request->stopable)
    {
      ftp_log (gftp_logging_error, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Connect"));
      return;
    }

  if ((selection_data->length >= 0) && (selection_data->format == 8)) 
    {
      if (GFTP_IS_CONNECTED (current_wdata->request))
        gftpui_disconnect (current_wdata);

      if (gftp_parse_url (current_wdata->request, 
                          (char *) selection_data->data) == 0)
        {
          ftp_log (gftp_logging_misc, NULL,
                   _("Received URL %s\n"), (char *) selection_data->data);

          ftp_connect (current_wdata, current_wdata->request);
        }
    }
}


void
listbox_drag (GtkWidget * widget, GdkDragContext * context,
	      GtkSelectionData * selection_data, guint info, guint32 clk_time,
	      gpointer data)
{
  GList * templist, * igl;
  char *tempstr, *str, *df;
  gftp_window_data * wdata;
  size_t totlen, oldlen;
  gftp_file * tempfle;
   
  totlen = 0;
  str = NULL;
  wdata = data;
  if (!check_status (_("Drag-N-Drop"), wdata, 1, 0, 1, 1)) 
    return;

  templist = listbox_get_selected_files (wdata);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
      tempfle = (gftp_file *) igl->data;

      if (strcmp (tempfle->file, "..") == 0) 
        continue;

      oldlen = totlen;
      df = gftp_build_path (wdata->request, wdata->request->directory,
                            tempfle->file, NULL);

      if (wdata->request->hostname == NULL || 
          wdata->request->protonum == GFTP_LOCAL_NUM)
        {
          tempstr = g_strdup_printf ("%s://%s ", 
                                 wdata->request->url_prefix, df);
        }
      else if (wdata->request->username == NULL || 
               *wdata->request->username == '\0')
        {
          tempstr = g_strdup_printf ("%s://%s:%d%s ", 
                                 wdata->request->url_prefix,
                                 wdata->request->hostname,
                                 wdata->request->port, df);
        }
      else
        {
          tempstr = g_strdup_printf ("%s://%s@%s:%d%s ", 
                                 wdata->request->url_prefix,
                                 wdata->request->username, 
                                 wdata->request->hostname,
                                 wdata->request->port, df);
        }

      g_free (df);

      /* Note, I am allocating memory for this byte above. Note the extra space
         at the end of the g_strdup_printf() format argument */
      if (S_ISDIR (tempfle->st_mode))
        tempstr[strlen (tempstr) - 1] = '/';
      else
        tempstr[strlen (tempstr) - 1] = '\0';

      totlen += strlen (tempstr);
      if (str != NULL)
        {
          totlen++;
          str = g_realloc (str, totlen + 1);
          strcpy (str + oldlen, "\n");
          strcpy (str + oldlen + 1, tempstr);
        } 
      else
        {
          str = g_malloc0 ((gulong) totlen + 1);
          strcpy (str, tempstr);
        }
      g_free (tempstr);
  }
  g_list_free (templist);


  if (str != NULL)
    {
      gtk_selection_data_set (selection_data, selection_data->target, 8,
      	                      (unsigned char *) str, strlen (str));
      g_free (str);
    }
}


void
listbox_get_drag_data (GtkWidget * widget, GdkDragContext * context, gint x,
		       gint y, GtkSelectionData * selection_data, guint info,
		       guint clk_time, gpointer data)
{
  char *newpos, *oldpos, *tempstr;
  GList * trans_list, * templist;
  gftp_window_data * wdata;
  gftp_transfer * tdata;
  int finish_drag;
  size_t len;

  wdata = data;   
  if (!check_status (_("Drag-N-Drop"), wdata, 1, 0, 0, 1)) 
    return;

  trans_list = NULL;
  finish_drag = 0;
  if ((selection_data->length >= 0) && (selection_data->format == 8)) 
    {
      oldpos = (char *) selection_data->data;
      while ((newpos = strchr (oldpos, '\n')) || 
             (newpos = strchr (oldpos, '\0'))) 
        {
          len = newpos - oldpos;
          if (oldpos[len - 1] == '\r')
            len--;

          if (len == 0)
            break;

          tempstr = g_malloc0 ((gulong) len + 1);
          memcpy (tempstr, oldpos, len);
          tempstr[len] = '\0';

          ftp_log (gftp_logging_misc, NULL, _("Received URL %s\n"), tempstr);

          if (dnd_remote_file (wdata, &trans_list, tempstr))
            finish_drag = 1;

          g_free (tempstr);

          if (*newpos == '\0') 
            break;

          oldpos = newpos + 1;
        }
    }

  gtk_drag_finish (context, finish_drag, FALSE, clk_time);

  for (templist = trans_list; templist != NULL; templist = templist->next)
    {
      tdata = templist->data;

      if (tdata->files == NULL || gftp_get_all_subdirs (tdata, NULL) != 0)
        {
          g_free (tdata);
          continue;
        }

      if (tdata->files == NULL)
        {
          g_free (tdata);
          continue;
        }

      gftpui_common_add_file_transfer (tdata->fromreq, tdata->toreq,
                                       tdata->fromwdata, tdata->towdata,
                                       tdata->files);
      g_free (tdata);
    }

  g_list_free (trans_list);
}
