/*****************************************************************************/
/*  dnd.c - drag and drop functions                                          */
/*  Copyright (C) 1998-2002 Brian Masney <masneyb@gftp.org>                  */
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
static const char cvsid[] = "$Id$";


static int
dnd_remote_file (char *url, gftp_window_data * wdata)
{
  gftp_request * current_ftpdata;
  gftp_file * newfle;
  char *str, *pos;
  GList templist;
  int i;

  printf ("FIXME - in dnd_remote file\n");
  newfle = g_malloc0 (sizeof (*newfle));
  newfle->shown = 1;
  if (url[strlen (url) - 1] == '/') 
    {
      newfle->isdir = 1;
      url[strlen (url) - 1] = '\0';
    }

  current_ftpdata = gftp_request_new ();
  current_ftpdata->logging_function = ftp_log;

  if (gftp_parse_url (current_ftpdata, url) != 0) 
    {
      ftp_log (gftp_logging_misc, NULL, 
               _("Drag-N-Drop: Ignoring url %s: Not a valid url\n"), url);
      gftp_request_destroy (current_ftpdata);
      free_fdata (newfle);
      return (0);
    }

  if ((str = GFTP_GET_DIRECTORY (current_ftpdata)) != NULL) 
    {
      if ((pos = strrchr (str, '/')) == NULL) 
        pos = str;
      else pos++;
      *(pos - 1) = '\0';
      i = 1;
    }
  else 
    {
      pos = str = GFTP_GET_DIRECTORY (current_ftpdata);
      i = 0;
    }

  if (compare_request (current_ftpdata, wdata->request, 1))
    {
      gftp_request_destroy (current_ftpdata);
      return (0);
    }

  if (i)
    {
      *(pos - 1) = '/';
      newfle->file = g_malloc (strlen (str) + 1);
      strcpy (newfle->file, str);
      *(pos - 1) = '\0';
    }
  else
    {
      newfle->file = g_malloc (strlen (str) + 1);
      strcpy (newfle->file, str);
    }
  
  newfle->destfile = g_strconcat (GFTP_GET_DIRECTORY (wdata->request),
                                     "/", pos, NULL);
  newfle->ascii = gftp_get_file_transfer_mode (newfle->file, 
                                wdata->request->data_type) == GFTP_TYPE_ASCII;

  templist.data = newfle;
  templist.next = NULL;
  add_file_transfer (current_ftpdata, wdata->request, NULL,
                     wdata, &templist, 1);
  gftp_request_destroy (current_ftpdata);

  if (newfle->isdir)
    {
/* FIXME - need to fix this
      add_entire_directory (tdata, newfle, 
		            GFTP_GET_DIRECTORY (tdata->fromhdata->ftpdata), 
			    GFTP_GET_DIRECTORY (tdata->tohdata->ftpdata), 
			    tdata->fromhdata->ftpdata);
*/
    }
  return (1);
}


void
openurl_get_drag_data (GtkWidget * widget, GdkDragContext * context, gint x,
		       gint y, GtkSelectionData * selection_data, guint info,
		       guint32 clk_time, gpointer data)
{
  if ((selection_data->length >= 0) && (selection_data->format == 8)) 
    {
      if (gftp_parse_url (current_wdata->request, 
                          (char *) selection_data->data) == 0)
        {
          if (GFTP_IS_CONNECTED (current_wdata->request))
            disconnect (current_wdata);

          ftp_connect (current_wdata, current_wdata->request, 1);
        }
      else
        {
          ftp_log (gftp_logging_misc, NULL, _("Could not parse URL %s\n"), 
                   selection_data->data);
        }
    }
}


void
listbox_drag (GtkWidget * widget, GdkDragContext * context,
	      GtkSelectionData * selection_data, guint info, guint32 clk_time,
	      gpointer data)
{
  GList * templist, * filelist;
  char *tempstr, *str, *pos;
  gftp_window_data * wdata;
  size_t totlen, oldlen;
  gftp_file * tempfle;
  int curpos;
   
  totlen = 0;
  str = NULL;
  wdata = data;
  if (!check_status (_("Drag-N-Drop"), wdata, 1, 0, 1, 1)) 
    return;

  filelist = wdata->files;
  templist = GTK_CLIST (wdata->listbox)->selection;
  curpos = 0;
  while (templist != NULL)
    {
      templist = get_next_selection (templist, &filelist, &curpos);
      tempfle = filelist->data;

      if (strcmp (tempfle->file, "..") == 0) 
        continue;

      oldlen = totlen;
      if (GFTP_GET_HOSTNAME (wdata->request) == NULL)
        {
          tempstr = g_strdup_printf ("%s://%s/%s%c", 
                                 GFTP_GET_URL_PREFIX (wdata->request),
                                 GFTP_GET_DIRECTORY (wdata->request), 
                                 tempfle->file, tempfle->isdir ? '/' : '\0');
        }
      else if (GFTP_GET_USERNAME (wdata->request) == NULL 
               || *GFTP_GET_USERNAME (wdata->request) == '\0')
        {
          tempstr = g_strdup_printf ("%s://%s:%d%s/%s%c", 
                                 GFTP_GET_URL_PREFIX (wdata->request),
                                 GFTP_GET_HOSTNAME (wdata->request),
                                 GFTP_GET_PORT (wdata->request),
                                 GFTP_GET_DIRECTORY (wdata->request), 
                                 tempfle->file, tempfle->isdir ? '/' : '\0');
        }
      else
        {
          tempstr = g_strdup_printf ("%s://%s@%s:%d%s/%s%c", 
                                 GFTP_GET_URL_PREFIX (wdata->request),
                                 GFTP_GET_USERNAME (wdata->request), 
                                 GFTP_GET_HOSTNAME (wdata->request),
                                 GFTP_GET_PORT (wdata->request),
                                 GFTP_GET_DIRECTORY (wdata->request), 
                                 tempfle->file, tempfle->isdir ? '/' : '\0');
        }
      if ((pos = strchr (tempstr, ':')) != NULL)
        pos += 2;
      else
        pos = tempstr;
      remove_double_slashes (pos);
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
          str = g_malloc (totlen + 1);
          strcpy (str, tempstr);
        }
      g_free (tempstr);
    }

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
		       guint32 clk_time, gpointer data)
{
  char *newpos, *oldpos, tempchar;
  gftp_window_data * wdata;
  int finish_drag;

  wdata = data;   
  if (!check_status (_("Drag-N-Drop"), wdata, 1, 0, 0, 1)) 
    return;

  finish_drag = 0;
  if ((selection_data->length >= 0) && (selection_data->format == 8)) 
    {
      oldpos = (char *) selection_data->data;
      printf ("FIXME - got data %s\n", oldpos);
      while ((newpos = strchr (oldpos, '\n')) || 
             (newpos = strchr (oldpos, '\0'))) 
        {
          tempchar = *newpos;
          *newpos = '\0';
          ftp_log (gftp_logging_misc, NULL, _("Received URL %s\n"), oldpos);
          if (dnd_remote_file (oldpos, wdata))
            finish_drag = 1;
         
          if (*newpos == '\0') 
            break;
          oldpos = newpos + 1;
        }
    }

  gtk_drag_finish (context, finish_drag, FALSE, clk_time);
}

