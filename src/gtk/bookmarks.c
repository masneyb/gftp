/*****************************************************************************/
/*  bookmarks.c - routines for the bookmarks                                 */
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

static GtkWidget * bm_dialog = NULL, * edit_bookmarks_dialog = NULL;
static GtkWidget * bm_hostedit, * bm_portedit, * bm_localdiredit,
                 * bm_remotediredit, * bm_useredit, * bm_passedit, * tree,
                 * bm_acctedit, * anon_chk, * bm_pathedit, * bm_protocol;
static GHashTable * new_bookmarks_htable = NULL;
static gftp_bookmarks_var * new_bookmarks = NULL;
static GtkUIManager *b_uimanager;

static guint bm_menu_merge_id = 0; // factory
static GtkActionGroup *dynamic_bm_menu = NULL;

void
on_menu_run_bookmark (GtkAction *action, gpointer path_str)
{
  int refresh_local;
  char *path = (char *) path_str;

  if (window1.request->stopable || window2.request->stopable)
    {
      ftp_log (gftp_logging_misc, NULL,
               _("%s: Please hit the stop button first to do anything else\n"),
               _("Run Bookmark"));
      return;
    }

  if (GFTP_IS_CONNECTED (current_wdata->request))
    gftpui_disconnect (current_wdata);

  if (gftp_parse_bookmark (current_wdata->request, other_wdata->request,
                           path, &refresh_local) < 0)
    return;

  if (refresh_local)
    gftpui_refresh (other_wdata, 0);

  ftp_connect (current_wdata, current_wdata->request);
}


static void
doadd_bookmark (gpointer * data, gftp_dialog_data * ddata)
{
  const char *edttxt, *spos;
  gftp_bookmarks_var * tempentry;
  char *dpos, *proto;

  edttxt = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*edttxt == '\0')
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Add Bookmark: You must enter a name for the bookmark\n"));
      return;
    }

  if (g_hash_table_lookup (gftp_bookmarks_htable, edttxt) != NULL)
    {
      ftp_log (gftp_logging_error, NULL,
	       _("Add Bookmark: Cannot add bookmark %s because that name already exists\n"), edttxt);
      return;
    }

  tempentry = g_malloc0 (sizeof (*tempentry));

  dpos = tempentry->path = g_malloc0 ((gulong) strlen (edttxt) + 1);
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

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostedit));
  tempentry->hostname = g_strdup (edttxt);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(portedit));
  tempentry->port = strtol (edttxt, NULL, 10);

  proto = gftp_protocols[current_wdata->request->protonum].name;
  tempentry->protocol = g_strdup (proto);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(other_wdata->combo));
  tempentry->local_dir = g_strdup (edttxt);

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(current_wdata->combo));
  tempentry->remote_dir = g_strdup (edttxt);

  if ( (edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(useredit))) != NULL)
    {
      tempentry->user = g_strdup (edttxt);

      edttxt = gtk_entry_get_text (GTK_ENTRY (passedit));
      tempentry->pass = g_strdup (edttxt);
      tempentry->save_password = GTK_TOGGLE_BUTTON (ddata->checkbox)->active;
    }

  gftp_add_bookmark (tempentry);

  gftp_write_bookmarks_file ();
  build_bookmarks_menu();
}


void
add_bookmark (gpointer data)
{
  const char *edttxt;

  if (!check_status (_("Add Bookmark"), current_wdata, 0, 0, 0, 1))
    return;

  edttxt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(hostedit));
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
  GtkAction *action;
  gftp_bookmarks_var * tempentry;
  char *slash;

  if (bm_menu_merge_id) {
    gtk_ui_manager_remove_ui(factory, bm_menu_merge_id);
  }
  bm_menu_merge_id = gtk_ui_manager_new_merge_id(factory);

  if (!dynamic_bm_menu) {
     dynamic_bm_menu = gtk_action_group_new("bm_menu");
     gtk_ui_manager_insert_action_group(factory, dynamic_bm_menu, 0);
     g_object_unref(dynamic_bm_menu);
  }

  tempentry = gftp_bookmarks->children;
  while (tempentry != NULL)
    {
      action = gtk_action_group_get_action (dynamic_bm_menu, tempentry->path);
      if (action) {
        gtk_action_group_remove_action (dynamic_bm_menu, action);
        action = NULL;
      }

      slash = strchr(tempentry->path, '/');
      if (tempentry->isfolder) {
        // BSD Sites
        action = gtk_action_new(tempentry->path, tempentry->path, NULL, NULL);
        gtk_action_group_add_action(dynamic_bm_menu, action);
        gtk_ui_manager_add_ui (factory, bm_menu_merge_id,
                          "/M/BookmarksMenu", tempentry->path, tempentry->path,
                          GTK_UI_MANAGER_MENU, FALSE);
      }
      else if (!slash) {
        action = gtk_action_new(tempentry->path, tempentry->path, NULL, NULL);
        gtk_action_group_add_action(dynamic_bm_menu, action);
        g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(on_menu_run_bookmark),
                        (gpointer) tempentry->path);
        gtk_ui_manager_add_ui (factory, bm_menu_merge_id,
                            "/M/BookmarksMenu", tempentry->path, tempentry->path,
                            GTK_UI_MANAGER_MENUITEM, FALSE);
      }
      else {
        // BSD Sites/FreeBSD
        char *tmp = g_strdup(tempentry->path);
        slash = strchr(tmp, '/');
        *slash = 0;
        char *path = g_strconcat("/M/BookmarksMenu/", tmp, NULL); /* /M/BookmarksMenu/BSD Sites */
        char *label = slash + 1;
        char *name = tempentry->path;
        action = gtk_action_new(name, label, NULL, NULL);
        gtk_action_group_add_action(dynamic_bm_menu, action);
        g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(on_menu_run_bookmark),
                        (gpointer) tempentry->path);
        gtk_ui_manager_add_ui (factory, bm_menu_merge_id,
                            path, name, name,
                            GTK_UI_MANAGER_MENUITEM, FALSE);
        g_free(tmp);
        g_free(path);
      }
      g_object_unref(action);

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

static gftp_bookmarks_var *
copy_bookmarks (gftp_bookmarks_var * bookmarks)
{
  gftp_bookmarks_var * new_bm, * preventry, * tempentry, * sibling, * newentry,
                 * tentry;

  new_bm = g_malloc0 (sizeof (*new_bm));
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
	newentry->path = g_strdup (tempentry->path);

      if (tempentry->hostname)
	newentry->hostname = g_strdup (tempentry->hostname);

      if (tempentry->protocol)
	newentry->protocol = g_strdup (tempentry->protocol);

      if (tempentry->local_dir)
	newentry->local_dir = g_strdup (tempentry->local_dir);

      if (tempentry->remote_dir)
	newentry->remote_dir = g_strdup (tempentry->remote_dir);

      if (tempentry->user)
	newentry->user = g_strdup (tempentry->user);

      if (tempentry->pass)
	newentry->pass = g_strdup (tempentry->pass);

      if (tempentry->acct)
	newentry->acct = g_strdup (tempentry->acct);

      newentry->port = tempentry->port;

      gftp_copy_local_options (&newentry->local_options_vars,
                               &newentry->local_options_hash,
                               &newentry->num_local_options_vars,
                               tempentry->local_options_vars,
                               tempentry->num_local_options_vars);
      newentry->num_local_options_vars = tempentry->num_local_options_vars;

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
_free_menu_entry (gftp_bookmarks_var * entry)
{
  GtkAction *action;
  if (dynamic_bm_menu) {
    if (entry->oldpath != NULL) {
        action = gtk_action_group_get_action (dynamic_bm_menu, entry->oldpath);
    } else {
        action = gtk_action_group_get_action (dynamic_bm_menu, entry->path);
    }
    if (action) {
        gtk_action_group_remove_action (dynamic_bm_menu, action);
    }
  }
}


static void
bm_apply_changes (GtkWidget * widget, gpointer backup_data)
{
  gftp_bookmarks_var * tempentry, * delentry;

  if (bm_dialog != NULL)
    {
      gtk_widget_grab_focus (bm_dialog);
      return;
    }

  if (gftp_bookmarks != NULL)
    {
      tempentry = gftp_bookmarks->children;
      while (tempentry != NULL)
        {
          if (tempentry->children != NULL)
            tempentry = tempentry->children;
          else
            {
              while (tempentry->next == NULL && tempentry->prev != NULL)
                {
                  delentry = tempentry;
                  tempentry = tempentry->prev;
                  _free_menu_entry (delentry);
                  gftp_free_bookmark (delentry, 1);
                }

              delentry = tempentry;
              tempentry = tempentry->next;
              if (tempentry != NULL)
                _free_menu_entry (delentry);

              gftp_free_bookmark (delentry, 1);
            }
        }

      g_hash_table_destroy (gftp_bookmarks_htable);
    }

  if (backup_data)
    {
      gftp_bookmarks = copy_bookmarks (new_bookmarks);
      gftp_bookmarks_htable = build_bookmarks_hash_table (gftp_bookmarks);
    }
  else
    {
      gftp_bookmarks = new_bookmarks;
      gftp_bookmarks_htable = new_bookmarks_htable;

      new_bookmarks = NULL;
      new_bookmarks_htable = NULL;
    }

  build_bookmarks_menu ();
  gftp_write_bookmarks_file ();
}


static void
bm_close_dialog (GtkWidget * widget, GtkWidget * dialog)
{
  if (bm_dialog != NULL)
    return;

  if (new_bookmarks_htable != NULL)
    {
      g_hash_table_destroy (new_bookmarks_htable);
      new_bookmarks_htable = NULL;
    }

  if (new_bookmarks != NULL)
    {
      gftp_bookmarks_destroy (new_bookmarks);
      new_bookmarks = NULL;
    }

  if (edit_bookmarks_dialog != NULL)
    {
      gtk_widget_destroy (edit_bookmarks_dialog);
      edit_bookmarks_dialog = NULL;
    }
}


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


static void
do_make_new (gpointer data, gftp_dialog_data * ddata)
{
  GdkPixmap * closedir_pixmap, * opendir_pixmap;
  GdkBitmap * closedir_bitmap, * opendir_bitmap;  
  gftp_bookmarks_var * tempentry, * newentry;
  char *pos, *text[2];
  const char *str;

  gftp_get_pixmap (tree, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
  gftp_get_pixmap (tree, "dir.xpm", &closedir_pixmap, &closedir_bitmap);

  str = gtk_entry_get_text (GTK_ENTRY (ddata->edit));
  if (*str == '\0')
    {
      ftp_log (gftp_logging_misc, NULL,
               _("You must specify a name for the bookmark."));
      return;
    }

  newentry = g_malloc0 (sizeof (*newentry));
  newentry->path = g_strdup (str);
  while ((pos = g_utf8_strchr (newentry->path, -1, '/')) != NULL)
    *pos++ = ' ';

  newentry->prev = new_bookmarks;
  if (data)
    newentry->isfolder = 1;

  if (new_bookmarks->children == NULL)
    new_bookmarks->children = newentry;
  else
    {
      tempentry = new_bookmarks->children;
      while (tempentry->next != NULL)
	tempentry = tempentry->next;
      tempentry->next = newentry;
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
                                             TRUE, FALSE);

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
  MakeEditDialog (_("New Item"),
		  _("Enter the name of the new item to create"), NULL, 1,
		  NULL, gftp_dialog_button_create,
                  do_make_new, NULL, NULL, NULL);
}


static void
do_delete_entry (gftp_bookmarks_var * entry, gftp_dialog_data * ddata)
{
  gftp_bookmarks_var * tempentry, * delentry;

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
      gftp_free_bookmark (tempentry, 0);

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
  gftp_bookmarks_var * entry;
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

      tempstr = g_strdup_printf (_("Are you sure you want to erase the bookmark\n%s and all its children?"), pos);
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
_add_tree_node (gftp_bookmarks_var * entry, char *path, int isleaf,
                int expanded)
{
  static GdkPixmap * closedir_pixmap = NULL,
                   * opendir_pixmap = NULL;
  static GdkBitmap * closedir_bitmap = NULL,
                   * opendir_bitmap = NULL;
  static int initialized = 0;
  GtkCTreeNode * parent;
  char *text[2], *pos;

  if (!initialized)
    {
      gftp_get_pixmap (tree, "open_dir.xpm", &opendir_pixmap, &opendir_bitmap);
      gftp_get_pixmap (tree, "dir.xpm", &closedir_pixmap, &closedir_bitmap);
      initialized = 1;
    }

  if (path == NULL)
    {
      if ((pos = strrchr (entry->path, '/')) == NULL)
        pos = entry->path;
      else
        pos++;
    }
  else
    pos = path;

  text[0] = text[1] = pos;

  if (entry->prev == NULL)
    parent = NULL;
  else
    parent = entry->prev->cnode;

  if (isleaf) 
    entry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree), parent, NULL, text,
                                          5, NULL, NULL, NULL, NULL, TRUE,
                                          expanded);
  else
    entry->cnode = gtk_ctree_insert_node (GTK_CTREE (tree), parent, NULL,
                                          text, 5, closedir_pixmap,
                                          closedir_bitmap, opendir_pixmap,
                                          opendir_bitmap, FALSE, expanded);

  gtk_ctree_node_set_row_data (GTK_CTREE (tree), entry->cnode, entry);
}


static void
build_bookmarks_tree (void)
{
  gftp_bookmarks_var * tempentry, * preventry;
  char *pos, *tempstr;

  _add_tree_node (new_bookmarks, _("Bookmarks"), 0, 1);

  tempentry = new_bookmarks->children;
  while (tempentry != NULL)
    {
      tempentry->cnode = NULL;

      if (tempentry->children != NULL)
	{
	  tempentry = tempentry->children;
	  continue;
	}
      else if (strchr (tempentry->path, '/') == NULL && tempentry->isfolder)
        _add_tree_node (tempentry, NULL, 0, 0);
      else
        {
          pos = tempentry->path;
          while ((pos = strchr (pos, '/')) != NULL)
            {
              *pos = '\0';
              tempstr = g_strdup (tempentry->path);
              *pos++ = '/';

              preventry = g_hash_table_lookup (new_bookmarks_htable, tempstr);
              g_free (tempstr);

              if (preventry->cnode == NULL)
                _add_tree_node (preventry, NULL, 0, 0);
            }

          _add_tree_node (tempentry, NULL, TRUE, 0);
	}

      while (tempentry->next == NULL && tempentry->prev != NULL)
        tempentry = tempentry->prev;
      tempentry = tempentry->next;
    }
}


static void
entry_apply_changes (GtkWidget * widget, gftp_bookmarks_var * entry)
{
  gftp_bookmarks_var * tempentry, * nextentry, * bmentry;
  char *pos, *newpath, tempchar, *tempstr;
  GtkWidget * tempwid;
  size_t oldpathlen;
  const char *str;

  tempstr = g_strdup (gtk_entry_get_text (GTK_ENTRY (bm_pathedit)));
  while ((pos = strchr (tempstr, '/')) != NULL)
    *pos = ' ';

  oldpathlen = strlen (entry->path);
  if ((pos = strrchr (entry->path, '/')) != NULL)
    {
      tempchar = *pos;
      *pos = '\0';
      newpath = gftp_build_path (NULL, entry->path, tempstr, NULL);
      *pos = tempchar;
      g_free (tempstr);
    }
  else
    newpath = tempstr;

  str = gtk_entry_get_text (GTK_ENTRY (bm_hostedit));
  if (entry->hostname != NULL)
    g_free (entry->hostname);
  entry->hostname = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_portedit));
  entry->port = strtol (str, NULL, 10);

  tempwid = gtk_menu_get_active (GTK_MENU (bm_protocol));
  str = gtk_object_get_user_data (GTK_OBJECT (tempwid));
  if (entry->protocol != NULL)
    g_free (entry->protocol);
  entry->protocol = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_remotediredit));
  if (entry->remote_dir != NULL)
    g_free (entry->remote_dir);
  entry->remote_dir = g_strdup (str);

  str = gtk_entry_get_text (GTK_ENTRY (bm_localdiredit));
  if (entry->local_dir != NULL)
    g_free (entry->local_dir);
  entry->local_dir = g_strdup (str);

  if (GTK_TOGGLE_BUTTON (anon_chk)->active)
    str = GFTP_ANONYMOUS_USER;
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_useredit));

  if (entry->user != NULL)
    g_free (entry->user);
  entry->user = g_strdup (str);

  if (strcasecmp (entry->user, GFTP_ANONYMOUS_USER) == 0)
    str = "@EMAIL@";
  else
    str = gtk_entry_get_text (GTK_ENTRY (bm_passedit));
  if (entry->pass != NULL)
    g_free (entry->pass);
  entry->pass = g_strdup (str);
  entry->save_password = *entry->pass != '\0';

  str = gtk_entry_get_text (GTK_ENTRY (bm_acctedit));
  if (entry->acct != NULL)
    g_free (entry->acct);
  entry->acct = g_strdup (str);

  gftp_gtk_save_bookmark_options ();

  if (strcmp (entry->path, newpath) != 0)
    {
      tempentry = entry;
      nextentry = entry->next;
      entry->next = NULL;

      while (tempentry != NULL)
	{
          bmentry = g_hash_table_lookup (gftp_bookmarks_htable,
                                         tempentry->path);
	  if (bmentry == NULL)
            bmentry = g_hash_table_lookup (new_bookmarks_htable,
                                           tempentry->path);

          g_hash_table_remove (new_bookmarks_htable, tempentry->path);

          if (bmentry->oldpath == NULL)
            bmentry->oldpath = tempentry->path;
          else
            g_free (tempentry->path);

          if (*(tempentry->path + oldpathlen) == '\0')
	    tempentry->path = g_strdup (newpath);
          else
	    tempentry->path = gftp_build_path (NULL, newpath,
                                               tempentry->path + oldpathlen,
                                               NULL);

	  g_hash_table_insert (new_bookmarks_htable, tempentry->path,
                               tempentry);

          gtk_ctree_node_set_text (GTK_CTREE (tree), tempentry->cnode, 0,
                                   tempentry->path);

	  if (tempentry->children != NULL)
	    tempentry = tempentry->children;
          else
            {
	      while (tempentry->next == NULL && tempentry != entry &&
                     tempentry->prev != NULL)
                tempentry = tempentry->prev;

	      tempentry = tempentry->next;
            }
	}

      entry->next = nextentry;
    }

  g_free (newpath);
}


static void
bmedit_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        entry_apply_changes (widget, user_data);
        /* no break */
      default:
        gtk_widget_destroy (widget);
        bm_dialog = NULL;
    }
}   


static void
edit_entry (gpointer data)
{
  GtkWidget * table, * tempwid, * menu, * notebook;
  gftp_bookmarks_var * entry;
  unsigned int num, i;
  char *pos;

  if (bm_dialog != NULL)
    {
      gtk_widget_grab_focus (bm_dialog);
      return;
    }

  if (GTK_CLIST (tree)->selection == NULL)
    return;

  entry = gtk_ctree_node_get_row_data (GTK_CTREE (tree),
				 GTK_CLIST (tree)->selection->data);

  if (entry == NULL || entry == new_bookmarks)
    return;

  bm_dialog = gtk_dialog_new_with_buttons (_("Edit Entry"), NULL, 0,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_CANCEL,
                                           GTK_STOCK_SAVE,
                                           GTK_RESPONSE_OK,
                                           NULL);

  gtk_window_set_wmclass (GTK_WINDOW (bm_dialog), "Edit Bookmark Entry",
                          "gFTP");
  gtk_window_set_position (GTK_WINDOW (bm_dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (bm_dialog)->vbox), 10);
  gtk_widget_realize (bm_dialog);

  set_window_icon(GTK_WINDOW(bm_dialog), NULL);

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (bm_dialog)->vbox), notebook, TRUE,
		      TRUE, 0);
  gtk_widget_show (notebook);

  table = gtk_table_new (11, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_widget_show (table);

  tempwid = gtk_label_new (_("Bookmark"));
  gtk_widget_show (tempwid);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, tempwid);

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

  g_signal_connect (GTK_OBJECT (bm_dialog), "response",
                    G_CALLBACK (bmedit_action), (gpointer) entry);

  gftp_gtk_setup_bookmark_options (notebook, entry);

  gtk_widget_show (bm_dialog);
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


static inline gboolean 
move_possible (GtkCTree *ctree, GtkCTreeNode *child, GtkCTreeNode *parent,
               GtkCTreeNode *sibling)
{
  return (parent != NULL && sibling != NULL);
}


static void
after_move (GtkCTree * ctree, GtkCTreeNode * child, GtkCTreeNode * parent,
	    GtkCTreeNode * sibling, gpointer data)
{

  gftp_bookmarks_var * childentry, * siblingentry, * parententry, * tempentry;
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
	  tempentry->path = g_strdup (stpos);
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

static void
close_bookmarks_dlg(GtkWidget *w, gpointer data) {
  gtk_widget_destroy(edit_bookmarks_dialog);
}

static gint
bm_dblclick (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  if (event->button == 3)
    {
      GtkWidget* menu = gtk_ui_manager_get_widget(b_uimanager, "/popup");
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
    }
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
  GtkAccelGroup * accel_group;
  GtkWidget * scroll;
  GtkActionEntry menu_items[] =
  {
    //  name         stock_id               "label"           accel tooltip  callback
    { "File",       NULL,                 N_("_File"),          NULL, NULL, NULL                            },
    { "NewFolder",  NULL,                 N_("New _Folder..."), NULL, NULL, G_CALLBACK(new_folder_entry)    },
    { "NewItem",    GTK_STOCK_NEW,        N_("New _Item..."),   NULL, NULL, G_CALLBACK(new_item_entry)      },
    { "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),        NULL, NULL, G_CALLBACK(delete_entry)        },
    { "Properties", GTK_STOCK_PROPERTIES, N_("_Properties..."), NULL, NULL, G_CALLBACK(edit_entry)          },
    { "Close",      GTK_STOCK_CLOSE,      N_("_Close"),         NULL, NULL, G_CALLBACK(close_bookmarks_dlg) }
  };

  guint nmenu_items = G_N_ELEMENTS (menu_items);

  static const gchar *ui_info = " \
  <ui> \
    <menubar name='M'> \
      <menu action='File'> \
        <menuitem action='NewFolder'/> \
        <menuitem action='NewItem'/> \
        <menuitem action='Delete'/> \
        <menuitem action='Properties'/> \
        <separator/> \
        <menuitem action='Close'/> \
      </menu> \
    </menubar> \
    <popup action='popup'> \
        <menuitem action='NewFolder'/> \
        <menuitem action='NewItem'/> \
        <menuitem action='Delete'/> \
        <menuitem action='Properties'/> \
        <separator/> \
        <menuitem action='Close'/> \
    </popup> \
  </ui>";

  if (edit_bookmarks_dialog != NULL)
    {
      gtk_widget_grab_focus (edit_bookmarks_dialog);
      return;
    }

  new_bookmarks = copy_bookmarks (gftp_bookmarks);
  new_bookmarks_htable = build_bookmarks_hash_table (new_bookmarks);

  edit_bookmarks_dialog = gtk_dialog_new_with_buttons (_("Edit Bookmarks"),
                                                       NULL, 0, 
                                                       GTK_STOCK_CANCEL,
                                                       GTK_RESPONSE_CANCEL,
						       GTK_STOCK_SAVE,
                                                       GTK_RESPONSE_OK,
                                                       NULL);
  gtk_window_set_position (GTK_WINDOW (edit_bookmarks_dialog),
                           GTK_WIN_POS_MOUSE);
  gtk_widget_realize (edit_bookmarks_dialog);

  set_window_icon(GTK_WINDOW(edit_bookmarks_dialog), NULL);

  b_uimanager = gtk_ui_manager_new();
  GtkActionGroup *b_actions = gtk_action_group_new("BActions");
  //gtk_action_group_set_translate_func(b_actions, menu_translate, NULL, NULL);
  gtk_action_group_add_actions(b_actions, menu_items, nmenu_items, NULL);
  //gtk_action_group_add_toggle_actions (b_actions, toggle_entries, n_toggle_entries, NULL);
  gtk_ui_manager_insert_action_group(b_uimanager, b_actions, 0);
  g_object_unref(b_actions);
  if (!gtk_ui_manager_add_ui_from_string(GTK_UI_MANAGER(b_uimanager), ui_info, -1, NULL))
     ftp_log (gftp_logging_error, NULL, "error");
  accel_group = gtk_ui_manager_get_accel_group(b_uimanager);
  gtk_window_add_accel_group(GTK_WINDOW(edit_bookmarks_dialog), accel_group);

  GtkWidget* menu = gtk_ui_manager_get_widget(b_uimanager, "/M");
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_bookmarks_dialog)->vbox),
                      menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scroll, 150, 200);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (edit_bookmarks_dialog)->vbox),
                      scroll, TRUE, TRUE, 0);
  gtk_container_border_width (GTK_CONTAINER (scroll), 3);
  gtk_widget_show (scroll);

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
  gtk_ctree_set_drag_compare_func (GTK_CTREE(tree), &move_possible);
  gtk_widget_show (tree);

  g_signal_connect (GTK_OBJECT (edit_bookmarks_dialog), "response",
                    G_CALLBACK (editbm_action), NULL);

  gtk_widget_show (edit_bookmarks_dialog);
  build_bookmarks_tree ();
}

