/*****************************************************************************/
/*  bookmarks.c - routines for the bookmarks                                 */
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

static GtkWidget * bm_hostedit, * bm_portedit, * bm_localdiredit,
  * bm_remotediredit, * bm_useredit, * bm_passedit, * bm_acctedit, * anon_chk,
  * bm_pathedit, * bm_protocol, * tree, *bm_sftppath;
static GHashTable * new_bookmarks_htable;
static gftp_bookmarks * new_bookmarks;
static GtkItemFactory * edit_factory;


void
run_bookmark (gpointer data)
{
  gftp_bookmarks * tempentry;
  int i;

  if (window1.request->stopable || window2.request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Run Bookmark"));
      return;
    }

  if ((tempentry = g_hash_table_lookup (bookmarks_htable, (char *) data)) == NULL)
    {
      ftp_log (gftp_logging_misc, NULL,
	       _("Internal gFTP Error: Could not look up bookmark entry. This is definately a bug. Please email masneyb@gftp.org about it. Please be sure to include the version number and how you can reproduce it\n"));
      return;
    }
  else if (tempentry->hostname == NULL || *tempentry->hostname == '\0' ||
	   tempentry->user == NULL || *tempentry->user == '\0')
    {
      ftp_log (gftp_logging_misc, NULL, _("Bookmarks Error: There are some missing entries in this bookmark. Make sure you have a hostname and username\n"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    disconnect (current_wdata);

  if (tempentry->local_dir && *tempentry->local_dir != '\0')
    {
      gftp_set_directory (other_wdata->request, tempentry->local_dir);
      gtk_clist_freeze (GTK_CLIST (other_wdata->listbox));
      remove_files_window (other_wdata);
      ftp_list_files (other_wdata, 1);
      gtk_clist_thaw (GTK_CLIST (other_wdata->listbox));
    }

  gftp_set_username (current_wdata->request, tempentry->user);
  if (strncmp (tempentry->pass, "@EMAIL@", 7) == 0)
    gftp_set_password (current_wdata->request, emailaddr);
  else
    gftp_set_password (current_wdata->request, tempentry->pass);
  if (tempentry->acct != NULL)
    gftp_set_account (current_wdata->request, tempentry->acct);
  gftp_set_hostname (current_wdata->request, tempentry->hostname);
  gftp_set_directory (current_wdata->request, tempentry->remote_dir);
  gftp_set_port (current_wdata->request, tempentry->port);
  gftp_set_sftpserv_path (current_wdata->request, tempentry->sftpserv_path);

  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (strcmp (gftp_protocols[i].name, tempentry->protocol) == 0)
	{
	  gftp_protocols[i].init (current_wdata->request);
	  break;
	}
    }

  if (!gftp_protocols[i].name)
    gftp_protocols[0].init (current_wdata->request);

  /* If we're using one of the SSH protocols, override the value of 
     ssh_need_userpass in the config file */
  if (strncmp (tempentry->protocol, "SSH", 3) == 0)
    current_wdata->request->need_userpass = 1;

  ftp_connect (current_wdata, current_wdata->request, 1);
}


static void
doadd_bookmark (gpointer * data, gftp_dialog_data * ddata)
{
  GtkItemFactoryEntry test = { NULL, NULL, run_bookmark, 0 };
  const char *edttxt, *spos;
  gftp_bookmarks * tempentry;
  char *dpos;

  edttxt = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Add Bookmark: You must enter a name for the bookmark\n"));
      return;
    }

  if (g_hash_table_lookup (bookmarks_htable, edttxt) != NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Add Bookmark: Cannot add bookmark %s because that name already exists\n"), edttxt);
      return;
    }

  tempentry = g_malloc0 (sizeof (*tempentry));

  dpos = tempentry->path = g_malloc (strlen (edttxt) + 1);
  for (spos = edttxt; *spos != '\0';)
    {
      *dpos++ = *spos++;
      if (*spos == '/')
        {
          *dpos++ = '/';
          for (; *spos == '/'; spos++);
        }
    }
  *dpos = '\0';

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry));
  tempentry->hostname = g_malloc (strlen (edttxt) + 1);
  strcpy (tempentry->hostname, edttxt);

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (portedit)->entry));
  tempentry->port = strtol (edttxt, NULL, 10);

  tempentry->protocol =
    g_malloc (strlen (GFTP_GET_PROTOCOL_NAME (current_wdata->request)) + 1);
  strcpy (tempentry->protocol, GFTP_GET_PROTOCOL_NAME (current_wdata->request));

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (other_wdata->combo)->entry));
  tempentry->local_dir = g_malloc (strlen (edttxt) + 1);
  strcpy (tempentry->local_dir, edttxt);

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (current_wdata->combo)->entry));
  tempentry->remote_dir = g_malloc (strlen (edttxt) + 1);
  strcpy (tempentry->remote_dir, edttxt);

  if ((edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (useredit)->entry))) != NULL)
    {
      tempentry->user = g_malloc (strlen (edttxt) + 1);
      strcpy (tempentry->user, edttxt);

      edttxt = gtk_entry_get_text (GTK_ENTRY (passedit));
      tempentry->pass = g_malloc (strlen (edttxt) + 1);
      strcpy (tempentry->pass, edttxt);
      tempentry->save_password = GTK_TOGGLE_BUTTON (ddata->checkbox)->active;
    }

  add_to_bookmark (tempentry);

  test.path = g_strconcat ("/Bookmarks/", tempentry->path, NULL);
  gtk_item_factory_create_item (factory, &test, (gpointer) tempentry->path,
				1);
  g_free (test.path);
  gftp_write_bookmarks_file ();
}


void
add_bookmark (gpointer data)
{
  const char *edttxt;

  if (!check_status (_("Add Bookmark"), current_wdata, 0, 0, 0, 1))
    return;

  edttxt = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (hostedit)->entry));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Add Bookmark: You must enter a hostname\n"));
      return;
    }

  MakeEditDialog (_("Add Bookmark"), _("Enter the name of the bookmark you want to add\nYou can separate items by a / to put it into a submenu\n(ex: Linux Sites/Debian)"), NULL, 1, _("Remember password"), gftp_dialog_button_create, doadd_bookmark, data, NULL, NULL);
}


void
build_bookmarks_menu (void)
{
  GtkItemFactoryEntry test = { NULL, NULL, NULL, 0 };
  gftp_bookmarks * tempentry;

  tempentry = bookmarks->children;
  while (tempentry != NULL)
    {
      test.path = g_strconcat ("/Bookmarks/", tempentry->path, NULL);
      if (tempentry->isfolder)
        {
          test.item_type = "<Branch>";
          test.callback = NULL;
        }
      else
        {
          test.item_type = "";
          test.callback = run_bookmark;
        }
      gtk_item_factory_create_item (factory, &test,
                                    (gpointer) tempentry->path, 1);
      g_free (test.path);
      if (tempentry->children != NULL)
        {
          tempentry = tempentry->children;
          continue;
        }
      while (tempentry->next == NULL && tempentry->prev != NULL)
        tempentry = tempentry->prev;

      tempentry = tempentry->next;
    }
}


static void
free_bookmark_entry_items (gftp_bookmarks * entry)
{
  if (entry->hostname)
    g_free (entry->hostname);
  if (entry->remote_dir)
    g_free (entry->remote_dir);
  if (entry->local_dir)
    g_free (entry->local_dir);
  if (entry->user)
    g_free (entry->user);
  if (entry->pass)
    g_free (entry->pass);
  if (entry->acct)
    g_free (entry->acct);
  if (entry->protocol)
    g_free (entry->protocol);
}


static gftp_bookmarks *
copy_bookmarks (gftp_bookmarks * bookmarks)
{
  gftp_bookmarks * new_bm, * preventry, * tempentry, * sibling, * newentry,
                 * tentry;

  new_bm = g_malloc0 (sizeof (*new_bm));
  /* FIXME - memory leak */
  new_bm->path = g_malloc0 (1);
  new_bm->isfolder = bookmarks->isfolder;
  preventry = new_bm;
  tempentry = bookmarks->children;
  sibling = NULL;
  while (tempentry != NULL)
    {
      newentry = g_malloc0 (sizeof (*newentry));
      newentry->isfolder = tempentry->isfolder;
      newentry->save_password = tempentry->save_password;
      newentry->cnode = tempentry->cnode;
      if (tempentry->path)
	{
	  newentry->path = g_malloc (strlen (tempentry->path) + 1);
	  strcpy (newentry->path, tempentry->path);
	}
      if (tempentry->hostname)
	{
	  newentry->hostname = g_malloc (strlen (tempentry->hostname) + 1);
	  strcpy (newentry->hostname, tempentry->hostname);
	}
      if (tempentry->protocol)
	{
	  newentry->protocol = g_malloc (strlen (tempentry->protocol) + 1);
	  strcpy (newentry->protocol, tempentry->protocol);
	}
      if (tempentry->local_dir)
	{
	  newentry->local_dir = g_malloc (strlen (tempentry->local_dir) + 1);
	  strcpy (newentry->local_dir, tempentry->local_dir);
	}
      if (tempentry->remote_dir)
	{
	  newentry->remote_dir =
	    g_malloc (strlen (tempentry->remote_dir) + 1);
	  strcpy (newentry->remote_dir, tempentry->remote_dir);
	}
      if (tempentry->user)
	{
	  newentry->user = g_malloc (strlen (tempentry->user) + 1);
	  strcpy (newentry->user, tempentry->user);
	}
      if (tempentry->pass)
	{
	  newentry->pass = g_malloc (strlen (tempentry->pass) + 1);
	  strcpy (newentry->pass, tempentry->pass);
	}
      if (tempentry->acct)
	{
	  newentry->acct = g_malloc (strlen (tempentry->acct) + 1);
	  strcpy (newentry->acct, tempentry->acct);
	}
      if (tempentry->sftpserv_path)
        {
          newentry->sftpserv_path = g_malloc (strlen (tempentry->sftpserv_path) + 1);
          strcpy (newentry->sftpserv_path, tempentry->sftpserv_path);
        }
      newentry->port = tempentry->port;

      if (sibling == NULL)
	{
	  if (preventry->children == NULL)
	    preventry->children = newentry;
	  else
	    {
	      tentry = preventry->children;
	      while (tentry->next != NULL)
		tentry = tentry->next;
	      tentry->next = newentry;
	    }
	}
      else
	sibling->next = newentry;
      newentry->prev = preventry;

      if (tempentry->children != NULL)
	{
	  preventry = newentry;
	  sibling = NULL;
	  tempentry = tempentry->children;
	}
      else
	{
	  if (tempentry->next == NULL)
	    {
	      sibling = NULL;
	      while (tempentry->next == NULL && tempentry->prev != NULL)
		{
		  tempentry = tempentry->prev;
		  preventry = preventry->prev;
		}
	      tempentry = tempentry->next;
	    }
	  else
	    {
	      sibling = newentry;
	      tempentry = tempentry->next;
	    }
	}
    }
  return (new_bm);
}


static void
bm_apply_changes (GtkWidget * widget, gpointer backup_data)
{
  gftp_bookmarks * tempentry, * delentry;
  char *tempstr;

  tempentry = bookmarks->children;
  while (tempentry != NULL)
    {
      if (tempentry->path && !tempentry->isfolder)
	{
	  tempstr = g_strdup_printf ("/Bookmarks/%s", tempentry->path);
          gtk_widget_destroy (gtk_item_factory_get_item (factory, tempstr));
	  g_free (tempentry->path);
	  g_free (tempstr);
	}

      free_bookmark_entry_items (tempentry);

      if (tempentry->children != NULL)
	tempentry = tempentry->children;
      else
	{
	  if (tempentry->next == NULL)
	    {
	      while (tempentry->next == NULL && tempentry->prev != NULL)
		{
		  delentry = tempentry;
		  tempentry = tempentry->prev;
		  if (delentry->isfolder)
		    {
		      tempstr = g_strdup_printf ("/Bookmarks/%s", 
                                                 delentry->path);
                      gtk_widget_destroy (gtk_item_factory_get_item (factory, 
                                                                     tempstr));
		      g_free (tempstr);
		      g_free (delentry->path);
		    }
		  g_free (delentry);
		}
	      delentry = tempentry;
	      tempentry = tempentry->next;
	      if (delentry->isfolder && tempentry != NULL)
		{
		  tempstr = g_strdup_printf ("/Bookmarks/%s", 
                                             delentry->path);
                    gtk_widget_destroy (gtk_item_factory_get_item (factory, 
                                                                   tempstr));
		  g_free (delentry->path);
		  g_free (tempstr);
		  g_free (delentry);
		}
	    }
	  else
	    {
	      delentry = tempentry;
	      tempentry = tempentry->next;
	      g_free (delentry);
	    }
	}
    }
  g_free (bookmarks);
  g_hash_table_destroy (bookmarks_htable);

  bookmarks = new_bookmarks;
  bookmarks_htable = new_bookmarks_htable;
  if (backup_data)
    {
      new_bookmarks = copy_bookmarks (bookmarks);
      new_bookmarks_htable = build_bookmarks_hash_table (new_bookmarks);
    }
  else
    {
      new_bookmarks = NULL;
      new_bookmarks_htable = NULL;
    }
  build_bookmarks_menu ();
  gftp_write_bookmarks_file ();
}


static void
bm_close_dialog (GtkWidget * widget, GtkWidget * dialog)
{
  gftp_bookmarks * tempentry, * delentry;

  if (new_bookmarks_htable)
    g_hash_table_destroy (new_bookmarks_htable);

  tempentry = new_bookmarks;
  while (tempentry != NULL)
    {
      free_bookmark_entry_items (tempentry);
      g_free (tempentry->path);

      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}

      while (tempentry->next == NULL && tempentry->prev != NULL)
        {
          delentry = tempentry;
	  tempentry = tempentry->prev;
          g_free (delentry);
        }

      delentry = tempentry;
      tempentry = tempentry->next;
      g_free (delentry);
    }

  new_bookmarks = NULL;
  new_bookmarks_htable = NULL;
  gtk_widget_destroy (dialog);
}


#if GTK_MAJOR_VERSION > 1 
static void
editbm_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        bm_apply_changes (widget, NULL);
        /* no break */
      default:
        bm_close_dialog (NULL, widget);
    }
}
#endif


static void
do_make_new (gpointer data, gftp_dialog_data * ddata)
{
  GdkPixmap * closedir_pixmap, * opendir_pixmap;
  GdkBitmap * closedir_bitmap, * opendir_bitmap;  
  gftp_bookmarks * tempentry, * newentry;
  GtkCTreeNode * sibling;
  char *pos, *text[2];
  const char *str;
#if GTK_MAJOR_VERSION > 1
  gsize bread, bwrite;
#endif

  gftp_get_pixmap (tree, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
  gftp_get_pixmap (tree, "dir.xpm", &closedir_pixmap, &closedir_bitmap);

  str = gtk_entry_get_text (GTK_ENTRY (ddata->edit));

  newentry = g_malloc0 (sizeof (*newentry));
#if GTK_MAJOR_VERSION == 1
  newentry->path = g_strdup (str);

  while ((pos = strchr (str, '/')) != NULL)
    *pos++ = ' ';
#else
  if (g_utf8_validate (str, -1, NULL))
    newentry->path = g_strdup (str);
  else
    newentry->path = g_locale_to_utf8 (str, -1, &bread, &bwrite, NULL);

  while ((pos = g_utf8_strchr (str, -1, '/')) != NULL)
    *pos++ = ' ';
#endif

  newentry->prev = new_bookmarks;
  if (data)
    newentry->isfolder = 1;

  if (new_bookmarks->children == NULL)
    {
      new_bookmarks->children = newentry;
      sibling = NULL;
    }
  else
    {
      tempentry = new_bookmarks->children;
      while (tempentry->next != NULL)
	tempentry = tempentry->next;
      tempentry->next = newentry;
      sibling = tempentry->cnode;
    }

  text[0] = text[1] = newentry->path;

  if (newentry->isfolder)
    newentry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree), 
                                             new_bookmarks->cnode, NULL,
			                     text, 5, closedir_pixmap, 
                                             closedir_bitmap, opendir_pixmap,
			                     opendir_bitmap, FALSE, FALSE);
  else
    newentry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree), 
                                             new_bookmarks->cnode, NULL,
                                             text, 5, NULL, NULL, NULL, NULL, 
                                             FALSE, FALSE);

  gtk_ctree_node_set_row_data (GTK_CTREE (tree), newentry->cnode, newentry);
  g_hash_table_insert (new_bookmarks_htable, newentry->path, newentry);
}


static void
new_folder_entry (gpointer data)
{
  MakeEditDialog (_("New Folder"),
		  _("Enter the name of the new folder to create"), NULL, 1,
		  NULL, gftp_dialog_button_create, 
                  do_make_new, (gpointer) 0x1, NULL, NULL);
}


static void
new_item_entry (gpointer data)
{
  MakeEditDialog (_("New Folder"),
		  _("Enter the name of the new item to create"), NULL, 1,
		  NULL, gftp_dialog_button_create,
                  do_make_new, NULL, NULL, NULL);
}


static void
do_delete_entry (gftp_bookmarks * entry, gftp_dialog_data * ddata)
{
  gftp_bookmarks * tempentry, * delentry;

  g_hash_table_remove (new_bookmarks_htable, entry->path);
  gtk_ctree_remove_node (GTK_CTREE (tree), entry->cnode);
  if (entry->prev->children == entry)
    entry->prev->children = entry->prev->children->next;
  else
    {
      tempentry = entry->prev->children;
      while (tempentry->next != entry)
	tempentry = tempentry->next;
      tempentry->next = entry->next;
    }

  entry->prev = NULL;
  entry->next = NULL;
  tempentry = entry;
  while (tempentry != NULL)
    {
      if (tempentry->path)
	g_free (tempentry->path);
      if (tempentry->hostname)
	g_free (tempentry->hostname);
      if (tempentry->remote_dir)
	g_free (tempentry->remote_dir);
      if (tempentry->user)
	g_free (tempentry->user);
      if (tempentry->pass)
	g_free (tempentry->pass);
      if (tempentry->sftpserv_path)
        g_free (tempentry->sftpserv_path);

      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      else if (tempentry->next == NULL && tempentry->prev != NULL)
	{
	  delentry = tempentry->prev;
	  g_free (tempentry);
	  tempentry = delentry->next;
	  if (delentry != entry)
	    g_free (delentry);
	}
      else
	tempentry = tempentry->next;
    }
  g_free (entry);
}


static void
delete_entry (gpointer data)
{
  gftp_bookmarks * entry;
  char *tempstr, *pos;

  if (GTK_CLIST (tree)->selection == NULL)
    return;
  entry =
    gtk_ctree_node_get_row_data (GTK_CTREE (tree),
				 GTK_CLIST (tree)->selection->data);
  if (entry == NULL || entry->prev == NULL)
    return;

  if (!entry->isfolder)
    do_delete_entry (entry, NULL);
  else
    {
      if ((pos = strrchr (entry->path, '/')) == NULL)
	pos = entry->path;
      else
	pos++;

      tempstr = g_strdup_printf (_("Are you sure you want to erase the bookmark\n%s and all it's children?"), pos);
      MakeYesNoDialog (_("Delete Bookmark"), tempstr, do_delete_entry, entry, 
                       NULL, NULL);
      g_free (tempstr);
    }
}


static void
set_userpass_visible (GtkWidget * checkbutton, GtkWidget * entry)
{
  gtk_widget_set_sensitive (bm_useredit, !GTK_TOGGLE_BUTTON (anon_chk)->active);
  gtk_widget_set_sensitive (bm_passedit, !GTK_TOGGLE_BUTTON (anon_chk)->active);
  gtk_widget_set_sensitive (bm_acctedit, !GTK_TOGGLE_BUTTON (anon_chk)->active);
}


static void
build_bookmarks_tree (void)
{
  GdkPixmap * closedir_pixmap, * opendir_pixmap;
  GdkBitmap * closedir_bitmap, * opendir_bitmap;  
  gftp_bookmarks * tempentry, * preventry;
  char *pos, *prevpos, *text[2], *str;
  GtkCTreeNode * parent;

  gftp_get_pixmap (tree, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
  gftp_get_pixmap (tree, "dir.xpm", &closedir_pixmap, &closedir_bitmap);
  text[0] = text[1] = _("Bookmarks");
  parent = gtk_ctree_insert_node (GTK_CTREE (tree), NULL, NULL, text, 5, 
                                  closedir_pixmap, closedir_bitmap, 
                                  opendir_pixmap, opendir_bitmap, FALSE, TRUE);
  gtk_ctree_node_set_row_data (GTK_CTREE (tree), parent, new_bookmarks);
  new_bookmarks->cnode = parent;

  tempentry = new_bookmarks->children;
  while (tempentry != NULL)
    {
      tempentry->cnode = NULL;
      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      else
	{
	  pos = tempentry->path;
	  while ((pos = strchr (pos, '/')) != NULL)
	    {
	      *pos = '\0';
	      str = g_malloc (strlen (tempentry->path) + 1);
	      strcpy (str, tempentry->path);
	      *pos = '/';
	      preventry = g_hash_table_lookup (new_bookmarks_htable, str);
	      if (preventry->cnode == NULL)
		{
		  if ((prevpos = strrchr (str, '/')) == NULL)
		    prevpos = str;
		  else
		    prevpos++;
		  text[0] = text[1] = prevpos;
		  preventry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree),
					   preventry->prev->cnode, NULL, text,
					   5, closedir_pixmap, closedir_bitmap,
					   opendir_pixmap, opendir_bitmap,
					   FALSE, FALSE);
		  gtk_ctree_node_set_row_data (GTK_CTREE (tree),
					       preventry->cnode, preventry);
		}

	      g_free (str);
	      pos++;
	    }
	}

      if ((pos = strrchr (tempentry->path, '/')) == NULL)
	pos = tempentry->path;
      else
	pos++;

      text[0] = text[1] = pos;
      tempentry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree), 
                               tempentry->prev->cnode, NULL,
			       text, 5, NULL, NULL, NULL, NULL, FALSE, FALSE);
      gtk_ctree_node_set_row_data (GTK_CTREE (tree), tempentry->cnode,
				   tempentry);

      while (tempentry->next == NULL && tempentry->prev != NULL)
	tempentry = tempentry->prev;
      tempentry = tempentry->next;
    }
}


static void
clear_bookmarks_tree (void)
{
  gftp_bookmarks * tempentry;

  tempentry = new_bookmarks->children;
  while (tempentry != NULL)
    {
      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      while (tempentry->next == NULL && tempentry->prev != NULL)
	{
	  gtk_ctree_remove_node (GTK_CTREE (tree), tempentry->cnode);
	  tempentry->cnode = NULL;
	  tempentry = tempentry->prev;
	}
      gtk_ctree_remove_node (GTK_CTREE (tree), tempentry->cnode);
      tempentry->cnode = NULL;
      tempentry = tempentry->next;
    }
}


static void
entry_apply_changes (GtkWidget * widget, gftp_bookmarks * entry)
{
  char *pos, *newpath, tempchar, *tempstr, *origpath;
  gftp_bookmarks * tempentry, * nextentry;
  GtkWidget * tempwid;
  const char *str;
  int oldpathlen;

  oldpathlen = strlen (entry->path);
  if ((pos = strrchr (entry->path, '/')) == NULL)
    {
      pos = entry->path;
      tempchar = *entry->path;
      *entry->path = '\0';
    }
  else
    {
      tempchar = *pos;
      *pos = '\0';
    }
  origpath = newpath = g_strconcat (entry->path, "/",
                         gtk_entry_get_text (GTK_ENTRY (bm_pathedit)), NULL);
  remove_double_slashes (newpath);
  for (; *newpath == '/'; newpath++);
  *pos = tempchar;

  str = gtk_entry_get_text (GTK_ENTRY (bm_hostedit));
  if (entry->hostname)
    g_free (entry->hostname);
  entry->hostname = g_malloc (strlen (str) + 1);
  strcpy (entry->hostname, str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_portedit));
  entry->port = strtol (str, NULL, 10);

  tempwid = gtk_menu_get_active (GTK_MENU (bm_protocol));
  str = gtk_object_get_user_data (GTK_OBJECT (tempwid));
  if (entry->protocol)
    g_free (entry->protocol);
  entry->protocol = g_malloc (strlen (str) + 1);
  strcpy (entry->protocol, str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_remotediredit));
  if (entry->remote_dir)
    g_free (entry->remote_dir);
  entry->remote_dir = g_malloc (strlen (str) + 1);
  strcpy (entry->remote_dir, str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_localdiredit));
  if (entry->local_dir)
    g_free (entry->local_dir);
  entry->local_dir = g_malloc (strlen (str) + 1);
  strcpy (entry->local_dir, str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_sftppath));
  if (entry->sftpserv_path)
    g_free (entry->sftpserv_path);
  if (strlen (str) > 0)
    {
      entry->sftpserv_path = g_malloc (strlen (str) + 1);
      strcpy (entry->sftpserv_path, str);
    }
  else
    entry->sftpserv_path = NULL;

  if (GTK_TOGGLE_BUTTON (anon_chk)->active)
    str = "anonymous";
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_useredit));
  if (entry->user)
    g_free (entry->user);
  entry->user = g_malloc (strlen (str) + 1);
  strcpy (entry->user, str);

  if (GTK_TOGGLE_BUTTON (anon_chk)->active)
    str = "@EMAIL@";
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_passedit));
  if (entry->pass)
    g_free (entry->pass);
  entry->pass = g_malloc (strlen (str) + 1);
  strcpy (entry->pass, str);
  entry->save_password = *entry->pass != '\0';

  if (GTK_TOGGLE_BUTTON (anon_chk)->active)
    str = "";
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_acctedit));
  if (entry->acct)
    g_free (entry->acct);
  entry->acct = g_malloc (strlen (str) + 1);
  strcpy (entry->acct, str);

  if (strcmp (entry->path, newpath) != 0)
    {
      tempentry = entry;
      nextentry = entry->next;
      entry->next = NULL;
      while (tempentry != NULL)
	{
	  g_hash_table_remove (new_bookmarks_htable, tempentry->path);
	  tempstr = g_strconcat (newpath, "/", tempentry->path + oldpathlen, 
                                 NULL);
	  remove_double_slashes (tempstr);
	  g_free (tempentry->path);
	  tempentry->path = tempstr;
	  g_hash_table_insert (new_bookmarks_htable, tempentry->path,
			       tempentry);
	  if (tempentry->children != NULL)
	    {
	      tempentry = tempentry->children;
	      continue;
	    }
	  while (tempentry->next == NULL && tempentry != entry &&
		 tempentry->prev != NULL)
	    tempentry = tempentry->prev;

	  tempentry = tempentry->next;
	}
      entry->next = nextentry;
      clear_bookmarks_tree ();
      build_bookmarks_tree ();
    }

  g_free (origpath);
}


#if GTK_MAJOR_VERSION > 1
static void
bmedit_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_APPLY:
        entry_apply_changes (widget, user_data);
        break;
      case GTK_RESPONSE_OK:
        entry_apply_changes (widget, user_data);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}   
#endif


static void
edit_entry (gpointer data)
{
  GtkWidget * table, * tempwid, * dialog, * menu;
  gftp_bookmarks * entry;
  int i, num;
  char *pos;

  if (GTK_CLIST (tree)->selection == NULL)
    return;

  entry = gtk_ctree_node_get_row_data (GTK_CTREE (tree),
				 GTK_CLIST (tree)->selection->data);

  if (entry == NULL || entry == new_bookmarks)
    return;

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Entry"));
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
#else
  dialog = gtk_dialog_new_with_buttons (_("Edit Entry"), NULL, 0,
                                        GTK_STOCK_SAVE,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_APPLY,
                                        GTK_RESPONSE_APPLY,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "Edit Bookmark Entry", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, _("gFTP Icon"));
    }

  tempwid = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tempwid, TRUE,
		      TRUE, 0);
  gtk_widget_show (tempwid);

  table = gtk_table_new (11, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_container_add (GTK_CONTAINER (tempwid), table);
  gtk_widget_show (table);

  tempwid = gtk_label_new (_("Description:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 0, 1);
  gtk_widget_show (tempwid);

  bm_pathedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_pathedit, 1, 2, 0, 1);
  if ((pos = strrchr (entry->path, '/')) == NULL)
    pos = entry->path;
  else
    pos++;
  if (pos)
    gtk_entry_set_text (GTK_ENTRY (bm_pathedit), pos);
  gtk_widget_show (bm_pathedit);

  tempwid = gtk_label_new (_("Hostname:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 1, 2);
  gtk_widget_show (tempwid);

  bm_hostedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_hostedit, 1, 2, 1, 2);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_hostedit, 0);
  else if (entry->hostname)
    gtk_entry_set_text (GTK_ENTRY (bm_hostedit), entry->hostname);
  gtk_widget_show (bm_hostedit);

  tempwid = gtk_label_new (_("Port:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 2, 3);
  gtk_widget_show (tempwid);

  bm_portedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_portedit, 1, 2, 2, 3);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_portedit, 0);
  else if (entry->port)
    {
      pos = g_strdup_printf ("%d", entry->port);
      gtk_entry_set_text (GTK_ENTRY (bm_portedit), pos);
      g_free (pos);
    }
  gtk_widget_show (bm_portedit);

  tempwid = gtk_label_new (_("Protocol:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 3, 4);
  gtk_widget_show (tempwid);

  menu = gtk_option_menu_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), menu, 1, 2, 3, 4);
  gtk_widget_show (menu);

  num = 0;
  bm_protocol = gtk_menu_new ();
  for (i = 0; gftp_protocols[i].name; i++)
    {
      tempwid = gtk_menu_item_new_with_label (gftp_protocols[i].name);
      gtk_object_set_user_data (GTK_OBJECT (tempwid), gftp_protocols[i].name);
      gtk_menu_append (GTK_MENU (bm_protocol), tempwid);
      gtk_widget_show (tempwid);
      if (entry->protocol &&
          strcmp (gftp_protocols[i].name, entry->protocol) == 0)
	num = i;
    }
  gtk_option_menu_set_menu (GTK_OPTION_MENU (menu), bm_protocol);
  gtk_option_menu_set_history (GTK_OPTION_MENU (menu), num);

  tempwid = gtk_label_new (_("Remote Directory:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 4, 5);
  gtk_widget_show (tempwid);

  bm_remotediredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_remotediredit, 1, 2, 4, 5);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_remotediredit, 0);
  else if (entry->remote_dir)
    gtk_entry_set_text (GTK_ENTRY (bm_remotediredit), entry->remote_dir);
  gtk_widget_show (bm_remotediredit);

  tempwid = gtk_label_new (_("Local Directory:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 5, 6);
  gtk_widget_show (tempwid);

  bm_localdiredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_localdiredit, 1, 2, 5, 6);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_localdiredit, 0);
  else if (entry->local_dir)
    gtk_entry_set_text (GTK_ENTRY (bm_localdiredit), entry->local_dir);
  gtk_widget_show (bm_localdiredit);

  tempwid = gtk_label_new (_("Remote SSH sftp path:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 6, 7);
  gtk_widget_show (tempwid);

  bm_sftppath = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_sftppath, 1, 2, 6, 7);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_sftppath, 0);
  else if (entry->sftpserv_path)
    gtk_entry_set_text (GTK_ENTRY (bm_sftppath), entry->sftpserv_path);
  gtk_widget_show (bm_sftppath);

  tempwid = gtk_hseparator_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 2, 7, 8);
  gtk_widget_show (tempwid);

  tempwid = gtk_label_new (_("Username:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 8, 9);
  gtk_widget_show (tempwid);

  bm_useredit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_useredit, 1, 2, 8, 9);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_useredit, 0);
  else if (entry->user)
    gtk_entry_set_text (GTK_ENTRY (bm_useredit), entry->user);
  gtk_widget_show (bm_useredit);

  tempwid = gtk_label_new (_("Password:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 9, 10);
  gtk_widget_show (tempwid);

  bm_passedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_passedit, 1, 2, 9, 10);
  gtk_entry_set_visibility (GTK_ENTRY (bm_passedit), FALSE);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_passedit, 0);
  else if (entry->pass)
    gtk_entry_set_text (GTK_ENTRY (bm_passedit), entry->pass);
  gtk_widget_show (bm_passedit);

  tempwid = gtk_label_new (_("Account:"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 10, 11);
  gtk_widget_show (tempwid);

  bm_acctedit = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), bm_acctedit, 1, 2, 10, 11);
  gtk_entry_set_visibility (GTK_ENTRY (bm_acctedit), FALSE);
  if (entry->isfolder)
    gtk_widget_set_sensitive (bm_acctedit, 0);
  else if (entry->acct)
    gtk_entry_set_text (GTK_ENTRY (bm_acctedit), entry->acct);
  gtk_widget_show (bm_acctedit);

  anon_chk = gtk_check_button_new_with_label (_("Log in as ANONYMOUS"));
  gtk_table_attach_defaults (GTK_TABLE (table), anon_chk, 0, 2, 11, 12);
  if (entry->isfolder)
    gtk_widget_set_sensitive (anon_chk, 0);
  else
    {
      gtk_signal_connect (GTK_OBJECT (anon_chk), "toggled",
			  GTK_SIGNAL_FUNC (set_userpass_visible), NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (anon_chk), entry->user
				    && strcmp (entry->user, "anonymous") == 0);
    }
  gtk_widget_show (anon_chk);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (entry_apply_changes),
		      (gpointer) entry);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (tempwid);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Apply"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (entry_apply_changes),
		      (gpointer) entry);
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (bmedit_action), (gpointer) entry);
#endif

  gtk_widget_show (dialog);
}


static gint
bm_enter (GtkWidget * widget, GdkEventKey * event, gpointer data)
{
  if (event->type == GDK_KEY_PRESS)
    {
      if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
	  edit_entry (NULL);
	  return (FALSE);
	}
      else if (event->keyval == GDK_KP_Delete || event->keyval == GDK_Delete)
	{
	  delete_entry (NULL);
	  return (FALSE);
	}
    }
  return (TRUE);
}


static void
after_move (GtkCTree * ctree, GtkCTreeNode * child, GtkCTreeNode * parent,
	    GtkCTreeNode * sibling, gpointer data)
{

  gftp_bookmarks * childentry, * siblingentry, * parententry, * tempentry;
  char *tempstr, *pos, *stpos;

  childentry = gtk_ctree_node_get_row_data (ctree, child);
  parententry = gtk_ctree_node_get_row_data (ctree, parent);
  siblingentry = gtk_ctree_node_get_row_data (ctree, sibling);

  tempentry = childentry->prev->children;
  if (tempentry == childentry)
    childentry->prev->children = childentry->prev->children->next;
  else
    {
      while (tempentry->next != childentry)
	tempentry = tempentry->next;
      tempentry->next = childentry->next;
    }
  childentry->prev = parententry;
  if (!parententry->isfolder)
    {
      childentry->next = parententry->children;
      parententry->children = childentry;
      gtk_ctree_move (ctree, child, parententry->prev->cnode,
		      parententry->cnode);
    }
  else
    {
      if (siblingentry == NULL || parententry->children == siblingentry)
	{
	  childentry->next = parententry->children;
	  parententry->children = childentry;
	}
      else
	{
	  tempentry = parententry->children;
	  while (tempentry->next != siblingentry)
	    tempentry = tempentry->next;
	  childentry->next = tempentry->next;
	  tempentry->next = childentry;
	}

      tempentry = childentry;
      while (tempentry != NULL)
	{
	  g_hash_table_remove (new_bookmarks_htable, tempentry->path);
	  if ((pos = strrchr (tempentry->path, '/')) == NULL)
	    pos = tempentry->path;
	  else
	    pos++;
	  tempstr = g_strdup_printf ("%s/%s", tempentry->prev->path, pos);
	  for (stpos = tempstr; *stpos == '/'; stpos++);
	  g_free (tempentry->path);
	  tempentry->path = g_malloc (strlen (stpos) + 1);
	  strcpy (tempentry->path, stpos);
	  g_free (tempstr);
	  g_hash_table_insert (new_bookmarks_htable, tempentry->path,
			       tempentry);
	  if (tempentry->children != NULL)
	    tempentry = tempentry->children;
	  else
	    {
	      if (tempentry->next == NULL)
		{
		  if (tempentry->prev == childentry)
		    break;
		  else
		    {
		      while (tempentry->next == NULL
			     && tempentry->prev != NULL)
			tempentry = tempentry->prev;
		    }
		}
	      tempentry = tempentry->next;
	    }
	}
    }
}


static gint
bm_dblclick (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  if (event->button == 3)
    gtk_item_factory_popup (edit_factory, (guint) event->x_root,
			    (guint) event->y_root, 1, 0);
  else if (event->type == GDK_2BUTTON_PRESS)
    {
      edit_entry (NULL);
      return (FALSE);
    }
  return (TRUE);
}


void
edit_bookmarks (gpointer data)
{
  GtkWidget * dialog, * scroll;
  GtkItemFactory * ifactory;
  GtkItemFactoryEntry menu_items[] = {
    {N_("/_File"), NULL, 0, 0, "<Branch>"},
    {N_("/File/tearoff"), NULL, 0, 0, "<Tearoff>"},
    {N_("/File/New Folder..."), NULL, new_folder_entry, 0, MN_(NULL)},
    {N_("/File/New Item..."), NULL, new_item_entry, 0, MS_(GTK_STOCK_NEW)},
    {N_("/File/Delete"), NULL, delete_entry, 0, MS_(GTK_STOCK_DELETE)},
    {N_("/File/Properties..."), NULL, edit_entry, 0, MS_(GTK_STOCK_PROPERTIES)},
    {N_("/File/sep"), NULL, 0, 0, "<Separator>"},
    {N_("/File/Close"), NULL, gtk_widget_destroy, 0, MS_(GTK_STOCK_CLOSE)}
  };
#if GTK_MAJOR_VERSION == 1
  GtkWidget * tempwid;
#endif

  new_bookmarks = copy_bookmarks (bookmarks);
  new_bookmarks_htable = build_bookmarks_hash_table (new_bookmarks);

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Bookmarks"));
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
#else
  dialog = gtk_dialog_new_with_buttons (_("Edit Bookmarks"), NULL, 0,
                                        GTK_STOCK_SAVE,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "Edit Bookmarks", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, _("gFTP Icon"));
    }

  /* FIXME - memory leak */
  ifactory = item_factory_new (GTK_TYPE_MENU_BAR, "<bookmarks>", NULL, NULL);
  create_item_factory (ifactory, 7, menu_items, NULL);
  create_item_factory (ifactory, 1, menu_items + 7, dialog);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), ifactory->widget,
		      FALSE, FALSE, 0);
  gtk_widget_show (ifactory->widget);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 150, 200);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), scroll, TRUE, TRUE,
		      0);
  gtk_container_border_width (GTK_CONTAINER (scroll), 3);
  gtk_widget_show (scroll);

  /* FIXME - memory leak */
  edit_factory = item_factory_new (GTK_TYPE_MENU, "<edit_bookmark>", NULL, "/File");

  create_item_factory (edit_factory, 6, menu_items + 2, dialog);

  tree = gtk_ctree_new (1, 0);
  gtk_clist_set_selection_mode (GTK_CLIST (tree), GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable (GTK_CLIST (tree), 1);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), tree);
  gtk_signal_connect_after (GTK_OBJECT (tree), "key_press_event",
			    GTK_SIGNAL_FUNC (bm_enter), (gpointer) tree);
  gtk_signal_connect_after (GTK_OBJECT (tree), "tree_move",
			    GTK_SIGNAL_FUNC (after_move), NULL);
  gtk_signal_connect_after (GTK_OBJECT (tree), "button_press_event",
			    GTK_SIGNAL_FUNC (bm_dblclick), (gpointer) tree);
  gtk_widget_show (tree);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (bm_apply_changes), NULL);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (bm_close_dialog), (gpointer) dialog);
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (bm_close_dialog), (gpointer) dialog);
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_widget_grab_focus (tempwid);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (editbm_action), NULL);
#endif

  gtk_widget_show (dialog);

  build_bookmarks_tree ();
}

