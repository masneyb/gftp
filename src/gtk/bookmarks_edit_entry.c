/***********************************************************************************/
/*  bookmarks_edit_entry.c - dialog to edit a bookmark entry                       */
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

static GtkWidget * bm_hostedit;
static GtkWidget * bm_portedit;
static GtkWidget * bm_localdiredit;
static GtkWidget * bm_remotediredit;
static GtkWidget * bm_useredit;
static GtkWidget * bm_passedit;
static GtkWidget * bm_acctedit;
static GtkWidget * bm_anon_chk;
static GtkWidget * bm_pathedit;
static GtkWidget * bm_combo_protocol;


static void entrydlg_set_userpass_visible (GtkWidget * checkbutton, GtkWidget * entry)
{
   gboolean active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bm_anon_chk));
   gtk_widget_set_sensitive (bm_useredit, !active);
   gtk_widget_set_sensitive (bm_passedit, !active);
   gtk_widget_set_sensitive (bm_acctedit, !active);
}


static void entry_apply_changes (GtkWidget * widget, gftp_bookmarks_var * entry)
{
   char *pos, *newpath, tempchar, *tempstr;
   const char *str;

   tempstr = g_strdup (gtk_entry_get_text (GTK_ENTRY (bm_pathedit)));
   while ((pos = strchr (tempstr, '/')) != NULL)
      *pos = ' ';

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

   if (entry->hostname)   g_free (entry->hostname);
   if (entry->protocol)   g_free (entry->protocol);
   if (entry->remote_dir) g_free (entry->remote_dir);
   if (entry->local_dir)  g_free (entry->local_dir);
   if (entry->user)       g_free (entry->user);
   if (entry->pass)       g_free (entry->pass);
   if (entry->acct)       g_free (entry->acct);

   str = gtk_entry_get_text (GTK_ENTRY (bm_hostedit));
   entry->hostname = g_strdup (str);

   str = gtk_entry_get_text (GTK_ENTRY (bm_portedit));
   entry->port = strtol (str, NULL, 10);

   str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(bm_combo_protocol));
   entry->protocol = g_strdup (str);

   str = gtk_entry_get_text (GTK_ENTRY (bm_remotediredit));
   entry->remote_dir = g_strdup (str);

   str = gtk_entry_get_text (GTK_ENTRY (bm_localdiredit));
   entry->local_dir = g_strdup (str);

   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bm_anon_chk))) {
      str = GFTP_ANONYMOUS_USER;
   } else {
      str = gtk_entry_get_text (GTK_ENTRY (bm_useredit));
   }
   entry->user = g_strdup (str);

   if (strcasecmp (entry->user, GFTP_ANONYMOUS_USER) == 0) {
      str = "@EMAIL@";
   } else {
      str = gtk_entry_get_text (GTK_ENTRY (bm_passedit));
   }
   entry->pass = g_strdup (str);
   entry->save_password = *entry->pass != '\0';

   str = gtk_entry_get_text (GTK_ENTRY (bm_acctedit));
   entry->acct = g_strdup (str);

   gftp_gtk_save_bookmark_options ();

   if (strcmp (entry->path, newpath) != 0)
   {
      // get label/description (doesn't include parent directory)
      pos = strrchr (newpath, '/');
      if (pos) pos++;
      else pos = newpath;

      // set new entry->path
      g_free (entry->path);
      entry->path = g_strdup (newpath);

      // set GtkTreeView node text/bookmark
      GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (btree));
      GtkTreeSelection *select = gtk_tree_view_get_selection (btree);
      GtkTreeIter iter;
      if (gtk_tree_selection_get_selected (select, NULL, &iter))
      {
         gtk_tree_store_set (store, &iter,  BTREEVIEW_COL_TEXT, pos, -1);
      }
   }

   g_free (newpath);
}


static void on_gtk_dialog_response_EditEntryDlg (GtkDialog *dialog,
                                                 gint response,
                                                 gpointer user_data)
{
   if (response == GTK_RESPONSE_OK)
   {
      gftp_bookmarks_var *entry = (gftp_bookmarks_var *) user_data;
      GtkWidget *m;
      struct btree_bookmark found_bm;

      const char *gtkentry_text = gtk_entry_get_text (GTK_ENTRY (bm_pathedit));
      char *p, tempchar;
      char *new_path;

      p = strrchr (entry->path, '/');
      if (p) {
         // directory, build dir/xxx
         tempchar = *p;
         *p = 0;
         new_path = g_strdup_printf ("%s/%s", entry->path, gtkentry_text);
         *p = tempchar;
      } else {
         new_path = g_strdup (gtkentry_text);
      }

      gtktreemodel_find_bookmark (new_path, &found_bm);
      if (new_path) g_free (new_path);

      /* don't allow bookmark path collision */
      if (found_bm.entry && found_bm.entry != entry)
      {
         g_signal_stop_emission_by_name (dialog, "response");
         m = gtk_message_dialog_new (GTK_WINDOW (edit_bm_entry_dlg),
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_OK,
                                     _("A bookmark with that description already exists."));
         g_signal_connect_swapped (m, "response", G_CALLBACK (gtk_widget_destroy), m);
         gtk_widget_show (m);
         return;
      }

      entry_apply_changes (GTK_WIDGET (dialog), user_data);
   }

   gtk_widget_destroy (GTK_WIDGET(dialog));
   edit_bm_entry_dlg = NULL;
}

// =============================================================================

static void show_edit_entry_dlg (gftp_bookmarks_var * xentry)
{
   GtkWidget * table, * tempwid, * notebook, *main_vbox;
   gftp_bookmarks_var * entry;
   char *pos;

   if (edit_bm_entry_dlg != NULL)
   {
      gtk_widget_grab_focus (edit_bm_entry_dlg);
      return;
   }

   if (xentry) {
      entry = xentry;
   } else {
      entry = btree_get_selected_bookmark (NULL);
   }

   if (!entry || !entry->path || !*entry->path)
      return;

   edit_bm_entry_dlg = gtk_dialog_new_with_buttons (
               _("Edit Entry"),
               GTK_WINDOW (edit_bookmarks_dialog),
               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
               "gtk-cancel", GTK_RESPONSE_CANCEL,
               "gtk-save",   GTK_RESPONSE_OK,
               NULL);

   gtk_window_set_role (GTK_WINDOW (edit_bm_entry_dlg), "Edit Bookmark Entry");
   gtk_window_set_position (GTK_WINDOW (edit_bm_entry_dlg), GTK_WIN_POS_MOUSE);
   gtk_widget_realize (edit_bm_entry_dlg);

   set_window_icon(GTK_WINDOW(edit_bm_entry_dlg), NULL);

   main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (edit_bm_entry_dlg));
   gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 10);

   notebook = gtk_notebook_new ();
   gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);

   table = gtk_table_new (11, 2, FALSE);
   gtk_container_set_border_width (GTK_CONTAINER (table), 5);
   gtk_table_set_row_spacings (GTK_TABLE (table), 5);
   gtk_table_set_col_spacings (GTK_TABLE (table), 5);

   tempwid = gtk_label_new (_("Bookmark"));

   gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, tempwid);

   tempwid = gtk_label_new (_("Description:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 0, 1);

   bm_pathedit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_pathedit, 1, 2, 0, 1);
   if ((pos = strrchr (entry->path, '/')) == NULL) {
      pos = entry->path;
   } else {
      pos++;
   }
   if (pos) {
      gtk_entry_set_text (GTK_ENTRY (bm_pathedit), pos);
   }

   tempwid = gtk_label_new (_("Hostname:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 1, 2);

   bm_hostedit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_hostedit, 1, 2, 1, 2);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_hostedit, 0);
   } else if (entry->hostname) {
      gtk_entry_set_text (GTK_ENTRY (bm_hostedit), entry->hostname);
   }

   tempwid = gtk_label_new (_("Port:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 2, 3);

   bm_portedit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_portedit, 1, 2, 2, 3);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_portedit, 0);
   } else if (entry->port) {
      pos = g_strdup_printf ("%d", entry->port);
      gtk_entry_set_text (GTK_ENTRY (bm_portedit), pos);
      g_free (pos);
   }

   tempwid = gtk_label_new (_("Protocol:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 3, 4);

   bm_combo_protocol = gtk_combo_box_text_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_combo_protocol, 1, 2, 3, 4);

   populate_combo_and_select_protocol (bm_combo_protocol, entry->protocol);

   tempwid = gtk_label_new (_("Remote Directory:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 4, 5);

   bm_remotediredit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_remotediredit, 1, 2, 4, 5);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_remotediredit, 0);
   } else if (entry->remote_dir) {
      gtk_entry_set_text (GTK_ENTRY (bm_remotediredit), entry->remote_dir);
   }
   tempwid = gtk_label_new (_("Local Directory:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 5, 6);

   bm_localdiredit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_localdiredit, 1, 2, 5, 6);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_localdiredit, 0);
   } else if (entry->local_dir) {
      gtk_entry_set_text (GTK_ENTRY (bm_localdiredit), entry->local_dir);
   }
   tempwid = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 2, 7, 8);

   tempwid = gtk_label_new (_("Username:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 8, 9);

   bm_useredit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_useredit, 1, 2, 8, 9);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_useredit, 0);
   } else if (entry->user) {
      gtk_entry_set_text (GTK_ENTRY (bm_useredit), entry->user);
   }
   tempwid = gtk_label_new (_("Password:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 9, 10);

   bm_passedit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_passedit, 1, 2, 9, 10);
   gtk_entry_set_visibility (GTK_ENTRY (bm_passedit), FALSE);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_passedit, 0);
   } else if (entry->pass) {
      gtk_entry_set_text (GTK_ENTRY (bm_passedit), entry->pass);
   }
   tempwid = gtk_label_new (_("Account:"));
   gtkcompat_widget_set_halign_right (tempwid);
   gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 10, 11);

   bm_acctedit = gtk_entry_new ();
   gtk_table_attach_defaults (GTK_TABLE (table), bm_acctedit, 1, 2, 10, 11);
   gtk_entry_set_visibility (GTK_ENTRY (bm_acctedit), FALSE);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_acctedit, 0);
   } else if (entry->acct) {
      gtk_entry_set_text (GTK_ENTRY (bm_acctedit), entry->acct);
   }

   bm_anon_chk = gtk_check_button_new_with_label (_("Log in as ANONYMOUS"));
   gtk_table_attach_defaults (GTK_TABLE (table), bm_anon_chk, 0, 2, 11, 12);
   if (entry->isfolder) {
      gtk_widget_set_sensitive (bm_anon_chk, 0);
   } else {
      g_signal_connect (G_OBJECT (bm_anon_chk), "toggled",
                        G_CALLBACK (entrydlg_set_userpass_visible), NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bm_anon_chk),
                                    entry->user && strcmp(entry->user,"anonymous") == 0);
   }

   g_signal_connect (G_OBJECT (edit_bm_entry_dlg),
                     "response",
                     G_CALLBACK (on_gtk_dialog_response_EditEntryDlg),
                     (gpointer) entry);

   gftp_gtk_setup_bookmark_options (notebook, entry);

   gtk_widget_show_all (edit_bm_entry_dlg);

   // switch to page 0 (widget must be visible)
   gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
}

