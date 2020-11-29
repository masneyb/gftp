/*****************************************************************************/
/*  Copyright (C) Brian Masney <masneyb@gftp.org>                      */
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

/* included by options_dialog.c
 * - make_proxy_hosts_tab()
 * - new_proxy_hosts
 */

static GtkWidget * proxy_list, * new_proxy_domain,
                 * entry_netipv4[4], * entry_netmask[4],
                 * network_label, * netmask_label,
                 * radio_domain, * domain_label,
                 * edit_button, * delete_button,
                 * _opt_dlg;

static GList * new_proxy_hosts = NULL;

static void make_proxy_hosts_tab (GtkWidget * notebook, GtkWidget * dialog);
static void add_proxy_host (GtkWidget * widget, gpointer data);
static void add_host_dlg_toggled_cb (GtkToggleButton *togglebutton, gpointer data);
static void delete_proxy_host (GtkWidget * widget, gpointer data);
static void buttons_toggle (GtkWidget * widget, gint row, gint col, GdkEventButton * event, gpointer data);
static void add_host_dlg_response_cb (GtkWidget * widget, gint response, gpointer user_data);
static void add_host_dlg_ok (GtkWidget * widget, gpointer data);
static void add_host_to_listbox (GList * templist);

// =====================================================================

static void
add_host_to_listbox (GList * templist)
{
  gftp_proxy_hosts * hosts = (gftp_proxy_hosts *) templist->data;
  char *add_data[2];
  int num;

  if (hosts->domain)
    {
      add_data[0] = hosts->domain;
      add_data[1] = NULL;
      num = gtk_clist_append (GTK_CLIST (proxy_list), add_data);
    }
  else
    {
      add_data[0] = g_strdup_printf ("%d.%d.%d.%d",
                                     hosts->ipv4_network_address >> 24 & 0xff,
                                     hosts->ipv4_network_address >> 16 & 0xff,
                                     hosts->ipv4_network_address >> 8 & 0xff,
                                     hosts->ipv4_network_address & 0xff);
      add_data[1] = g_strdup_printf ("%d.%d.%d.%d",
                                     hosts->ipv4_netmask >> 24 & 0xff,
                                     hosts->ipv4_netmask >> 16 & 0xff,
                                     hosts->ipv4_netmask >> 8 & 0xff,
                                     hosts->ipv4_netmask & 0xff);
      num = gtk_clist_append (GTK_CLIST (proxy_list), add_data);
      g_free (add_data[0]);
      g_free (add_data[1]);
    }

  gtk_clist_set_row_data (GTK_CLIST (proxy_list), num, (gpointer) templist);
}


static void
add_host_dlg_ok (GtkWidget * widget, gpointer data)
{
  gftp_proxy_hosts * hosts;
  const char * edttxt;
  GList * templist = (GList *) data;
  int num;

  if (templist == NULL)
    {
      hosts = g_malloc0 (sizeof (*hosts));
      new_proxy_hosts = g_list_append (new_proxy_hosts, hosts);
      for (templist = new_proxy_hosts; templist->next != NULL;
           templist = templist->next);
    }
  else
    {
      num = gtk_clist_find_row_from_data (GTK_CLIST (proxy_list), templist);
      if (num != -1)
        gtk_clist_remove (GTK_CLIST (proxy_list), num);
      hosts = templist->data;
    }

  if (hosts->domain)
    {
      g_free (hosts->domain);
      hosts->domain = NULL;
    }

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_domain)))
    {
      edttxt = gtk_entry_get_text (GTK_ENTRY (new_proxy_domain));
      hosts->domain = g_strdup (edttxt);
      hosts->ipv4_netmask = hosts->ipv4_network_address = 0;
    }
  else
    {
      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netipv4[0]));
      hosts->ipv4_network_address = (strtol (edttxt, NULL, 10) & 0xff) << 24;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netipv4[1]));
      hosts->ipv4_network_address |= (strtol (edttxt, NULL, 10) & 0xff) << 16;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netipv4[2]));
      hosts->ipv4_network_address |= (strtol (edttxt, NULL, 10) & 0xff) << 8;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netipv4[3]));
      hosts->ipv4_network_address |= strtol (edttxt, NULL, 10) & 0xff;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netmask[0]));
      hosts->ipv4_netmask = (strtol (edttxt, NULL, 10) & 0xff) << 24;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netmask[1]));
      hosts->ipv4_netmask |= (strtol (edttxt, NULL, 10) & 0xff) << 16;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netmask[2]));
      hosts->ipv4_netmask |= (strtol (edttxt, NULL, 10) & 0xff) << 8;

      edttxt = gtk_entry_get_text (GTK_ENTRY (entry_netmask[3]));
      hosts->ipv4_netmask |= strtol (edttxt, NULL, 10) & 0xff;
    }

  add_host_to_listbox (templist);
}


static void
add_host_dlg_response_cb (GtkWidget * widget, gint response, gpointer user_data)
{
  if (response == GTK_RESPONSE_OK) {
     add_host_dlg_ok (widget, user_data);
  }
  gtk_widget_destroy (widget);
}

static void
buttons_toggle (GtkWidget * widget, gint row, gint col, GdkEventButton * event, gpointer data)
{
  gtk_widget_set_sensitive (edit_button, data != NULL);
  gtk_widget_set_sensitive (delete_button, data != NULL);
}

static void
delete_proxy_host (GtkWidget * widget, gpointer data)
{
  GList *templist;
  int num;

  gftp_configuration_changed = 1; /* FIXME */

  if ((templist = GTK_CLIST (proxy_list)->selection) == NULL)
    return;
  num = GPOINTER_TO_INT (templist->data);
  templist = gtk_clist_get_row_data (GTK_CLIST (proxy_list), num);
  new_proxy_hosts = g_list_remove_link (new_proxy_hosts, templist);
  gtk_clist_remove (GTK_CLIST (proxy_list), num);
  buttons_toggle (NULL, 0, 0, 0, NULL);
}

static void
add_host_dlg_toggled_cb (GtkToggleButton *togglebutton, gpointer data)
{
  gtk_widget_set_sensitive (new_proxy_domain, data != NULL);
  gtk_widget_set_sensitive (domain_label, data != NULL);
  gtk_widget_set_sensitive (network_label, data == NULL);
  gtk_widget_set_sensitive (netmask_label, data == NULL);
  int i;
  for (i = 0; i < 4; i++) {
    gtk_widget_set_sensitive (entry_netipv4[i], data == NULL);
    gtk_widget_set_sensitive (entry_netmask[i], data == NULL);
  }
}

static void
add_proxy_host (GtkWidget * widget, gpointer data)
{
  GtkWidget *tempwid, *dialog, *box, *rbox, *vbox, *radio_network, *table, *main_vbox;
  gftp_proxy_hosts *hosts;
  char *tempstr, *title;
  GList *templist;
  int i;

  gftp_configuration_changed = 1; /* FIXME */

  if (data)
    {
      if ((templist = GTK_CLIST (proxy_list)->selection) == NULL) {
        return;
      }
      templist = gtk_clist_get_row_data (GTK_CLIST (proxy_list), 
                                         GPOINTER_TO_INT (templist->data));
      hosts = templist->data;
    }
  else
    {
      hosts = NULL;
      templist = NULL;
    }

  title = hosts ? _("Edit Host") : _("Add Host");

  dialog = gtk_dialog_new ();

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (_opt_dlg));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_role (GTK_WINDOW(dialog), "hostinfo");

  gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-cancel", GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (dialog), "gtk-save",   GTK_RESPONSE_OK);

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 5);
  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);

  GtkWidget * frame = gtk_frame_new ("");
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_Type:"));
  gtkcompat_widget_set_halign_left (tempwid);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  rbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box), rbox, TRUE, TRUE, 0);

  radio_domain = gtk_radio_button_new_with_label (NULL, _("Domain"));
  g_signal_connect (G_OBJECT (radio_domain), "toggled",
                    G_CALLBACK (add_host_dlg_toggled_cb), (gpointer) 1);
  
  radio_network = gtk_radio_button_new_with_label (gtk_radio_button_get_group
                                            (GTK_RADIO_BUTTON (radio_domain)),
                                           _("Network"));
  g_signal_connect (G_OBJECT (radio_network), "toggled",
                    G_CALLBACK (add_host_dlg_toggled_cb), NULL);

  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), radio_network);

  gtk_box_pack_start (GTK_BOX (rbox), radio_network, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (rbox), radio_domain,  FALSE, FALSE, 0);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);

  network_label = gtk_label_new_with_mnemonic (_("_Network address:"));
  gtkcompat_widget_set_halign_left (network_label);
  gtk_table_attach_defaults (GTK_TABLE (table), network_label, 0, 1, 0, 1);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 0, 1);

  for (i = 0; i < 4; i++) {
     entry_netipv4[i] = gtk_entry_new ();
     gtk_widget_set_size_request (entry_netipv4[i], 38, -1);
     gtk_box_pack_start (GTK_BOX (box), entry_netipv4[i], TRUE, TRUE, 0);
  }
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), entry_netipv4[0]);

  netmask_label = gtk_label_new_with_mnemonic (_("N_etmask:"));
  gtkcompat_widget_set_halign_left (netmask_label);
  gtk_table_attach_defaults (GTK_TABLE (table), netmask_label, 0, 1, 1, 2);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 1, 2);

  for (i = 0; i < 4; i++) {
     entry_netmask[i] = gtk_entry_new ();
     gtk_widget_set_size_request (entry_netmask[i], 38, -1);
     gtk_box_pack_start (GTK_BOX (box), entry_netmask[i], TRUE, TRUE, 0);
  }
  gtk_label_set_mnemonic_widget (GTK_LABEL (netmask_label), entry_netmask[0]);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);

  tempwid = gtk_label_new_with_mnemonic (_("_Domain:"));
  domain_label = tempwid;
  gtkcompat_widget_set_halign_left (tempwid);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);

  new_proxy_domain = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), new_proxy_domain, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), new_proxy_domain);

  if (!hosts || !hosts->domain)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_network), TRUE);
      add_host_dlg_toggled_cb (NULL, NULL);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_domain), TRUE);
      add_host_dlg_toggled_cb (NULL, (gpointer) 1);
    }

  if (hosts && hosts->domain) {
     gtk_entry_set_text (GTK_ENTRY (new_proxy_domain), hosts->domain);
  } else if (hosts) {
     tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 24 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netipv4[0]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 16 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netipv4[1]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 8 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netipv4[2]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netipv4[3]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 24 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netmask[0]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 16 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netmask[1]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 8 & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netmask[2]), tempstr);
     g_free (tempstr);

     tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask & 0xff);
     gtk_entry_set_text (GTK_ENTRY (entry_netmask[3]), tempstr);
     g_free (tempstr);
  }

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (add_host_dlg_response_cb), NULL);

  gtk_widget_show_all (dialog);
}

static void
make_proxy_hosts_tab (GtkWidget * notebook, GtkWidget * dialog)
{
  GtkWidget *tempwid, *box, *hbox, *scroll;
  gftp_config_list_vars * proxy_hosts;
  char *add_data[2];
  GList * templist;

  _opt_dlg = dialog;
  add_data[0] = _("Network");
  add_data[1] = _("Netmask");

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 10);

  tempwid = gtk_label_new (_("Local Hosts"));
  gtk_widget_show (tempwid);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, tempwid);

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), scroll, TRUE, TRUE, 0);

  proxy_list = gtk_clist_new_with_titles (2, add_data);
  gtk_container_add (GTK_CONTAINER (scroll), proxy_list);
  gtk_clist_set_column_auto_resize (GTK_CLIST (proxy_list), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (proxy_list), 1, TRUE);

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);
  new_proxy_hosts = gftp_copy_proxy_hosts (proxy_hosts->list);

  for (templist = new_proxy_hosts;
       templist != NULL;
       templist = templist->next)
    add_host_to_listbox (templist);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);

  tempwid = gtk_button_new_from_stock ("gtk-add");
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
                    G_CALLBACK (add_proxy_host), NULL);

  tempwid = gtk_button_new_from_stock ("gtk-edit");
  edit_button = tempwid;
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
                    G_CALLBACK (add_proxy_host), (gpointer) 1);

  tempwid = gtk_button_new_from_stock ("gtk-delete");
  delete_button = tempwid;
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (tempwid), "clicked",
                    G_CALLBACK (delete_proxy_host), NULL);

  g_signal_connect (G_OBJECT (proxy_list), "select_row", 
                    G_CALLBACK (buttons_toggle), (gpointer) 1);
  g_signal_connect (G_OBJECT (proxy_list), "unselect_row", 
                    G_CALLBACK (buttons_toggle), NULL);
  buttons_toggle (NULL, 0, 0, 0, NULL);
}

