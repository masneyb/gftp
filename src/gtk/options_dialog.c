/*****************************************************************************/
/*  menu-items.c - menu callbacks                                            */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

#include <gftp-gtk.h>
static const char cvsid[] = "$Id$";

static void make_proxy_hosts_tab 		( GtkWidget * notebook );
static void add_host_to_listbox 		( GList * templist );
static void add_proxy_host 			( GtkWidget * widget, 
						  gpointer data );
static void add_toggle 				( GtkWidget * widget, 
						  gpointer data );
static void add_ok 				( GtkWidget * widget, 
						  gpointer data );
static void delete_proxy_host 			( GtkWidget * widget, 
						  gpointer data );
static void proxy_toggle 			( GtkList * list, 
						  GtkWidget * child, 
						  gpointer data );
static void apply_changes 			( GtkWidget * widget, 
						  gpointer data );
static void clean_old_changes 			( GtkWidget * widget, 
						  gpointer data );
static char *get_proxy_config 			( void );

static GtkWidget * proxy_text, * proxy_list, * new_proxy_domain, * network1,
                 * network2, * network3, * network4, * netmask1, * netmask2, 
                 * netmask3, * netmask4, * domain_active, * proxy_combo,
                 * def_proto_combo;
static GList * new_proxy_hosts;
static char *custom_proxy;

#if GTK_MAJOR_VERSION > 1
static void
options_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_APPLY:
        apply_changes (widget, NULL);
        break;
      case GTK_RESPONSE_OK:
        apply_changes (widget, NULL);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}
#endif


void
options_dialog (gpointer data)
{
  GtkWidget * dialog, * tempwid, * notebook, * table, * box;
  char tempstr[20], *pos, *endpos, *oldstr;
  int i, tbl_col, tbl_num, combo_num;
  GList * combo_list;

#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Options"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area),
                              5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
#else
  dialog = gtk_dialog_new_with_buttons (_("Options"), NULL, 0,
                                        GTK_STOCK_SAVE,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_APPLY,
                                        GTK_RESPONSE_APPLY,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "options", "gFTP");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
  gtk_widget_realize (dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (dialog->window, _("gFTP Icon"));
    }

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook, TRUE,
                      TRUE, 0);
  gtk_widget_show (notebook);

  tbl_num = tbl_col = 0;
  table = box = NULL;
  for (i=0; config_file_vars[i].key != NULL; i++)
    {
      if (!(config_file_vars[i].ports_shown & GFTP_PORT_GTK))
        continue;

      switch (config_file_vars[i].type)
        {
          case CONFIG_NOTEBOOK:
            box = gtk_vbox_new (FALSE, 0);
            gtk_container_border_width (GTK_CONTAINER (box), 10);
            gtk_widget_show (box);

            tempwid = gtk_label_new (_(config_file_vars[i].description));
            gtk_widget_show (tempwid);
            gtk_notebook_append_page (GTK_NOTEBOOK (notebook), box, tempwid);
            break;
          case CONFIG_TABLE:
            table = gtk_table_new (1, 2, FALSE);
            gtk_table_set_row_spacings (GTK_TABLE (table), 5);
            gtk_table_set_col_spacings (GTK_TABLE (table), 5);
            gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
            gtk_widget_show (table);
            tbl_num = 1;
            tbl_col = 0;
            break;
          case CONFIG_COMBO:
            gtk_table_resize (GTK_TABLE (table), tbl_num, 2);

            tempwid = gtk_label_new (_(config_file_vars[i].description));
            gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1,
                                       tbl_num - 1, tbl_num);
            gtk_widget_show (tempwid);

            tempwid = gtk_combo_new ();
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 1, 2,
                                       tbl_num - 1, tbl_num);
            gtk_widget_show (tempwid);
            config_file_vars[i].widget = tempwid;

            /* We only have Default Protocol and the Proxy type as the two
               combo types. If I add more later on, I'll work on a better
               interface for all this stuff */
            if (strcmp (config_file_vars[i].comment, "DP") == 0)
              def_proto_combo = tempwid;
            else
              proxy_combo = tempwid;

            tbl_num++;
            break;
          case CONFIG_TEXT:
#if GTK_MAJOR_VERSION == 1
            proxy_text = gtk_text_new (NULL, NULL);
            gtk_text_set_editable (GTK_TEXT (proxy_text), TRUE);
#else
            proxy_text = gtk_text_view_new ();
            gtk_text_view_set_editable (GTK_TEXT_VIEW (proxy_text), TRUE);
#endif
            gtk_widget_set_size_request (proxy_text, -1, 75);
            gtk_table_attach_defaults (GTK_TABLE (table), proxy_text, 0, 2, 
                                       tbl_num - 1, tbl_num);
            gtk_widget_show (proxy_text);
            config_file_vars[i].widget = proxy_text;

            tbl_num++;
            break;
          case CONFIG_CHARTEXT:
          case CONFIG_CHARPASS:
          case CONFIG_INTTEXT:
          case CONFIG_UINTTEXT:
          case CONFIG_FLOATTEXT:
            gtk_table_resize (GTK_TABLE (table), tbl_num, 2);

            tempwid = gtk_label_new (_(config_file_vars[i].description));
            gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1,
                                       tbl_num - 1, tbl_num);
            gtk_widget_show (tempwid);

            tempwid = gtk_entry_new ();
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 1, 2,
                                       tbl_num - 1, tbl_num);

            switch (config_file_vars[i].type)
              {
                case CONFIG_INTTEXT:
                case CONFIG_UINTTEXT:
                  g_snprintf (tempstr, sizeof (tempstr), "%d",
                              *(int *) config_file_vars[i].var);
                  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
                  break;
                case CONFIG_FLOATTEXT:
                  g_snprintf (tempstr, sizeof (tempstr), "%.2f",
                              *(double *) config_file_vars[i].var);
                  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
                  break;
                case CONFIG_CHARTEXT:
                  gtk_entry_set_text (GTK_ENTRY (tempwid),
                                      *(char **) config_file_vars[i].var);
                  break;
                case CONFIG_CHARPASS:
                  gtk_entry_set_text (GTK_ENTRY (tempwid),
                                      *(char **) config_file_vars[i].var);
                  gtk_entry_set_visibility (GTK_ENTRY (tempwid), 0);
                  break;
              }
            gtk_widget_show (tempwid);
            config_file_vars[i].widget = tempwid;
            tbl_num++;
            break;
          case CONFIG_CHECKBOX:
            tempwid = gtk_check_button_new_with_label (
                                    _(config_file_vars[i].description));
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 
                                    tbl_col, tbl_col + 1, tbl_num, tbl_num + 1);
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tempwid),
                                    *(int *) config_file_vars[i].var);
            gtk_widget_show (tempwid);
            config_file_vars[i].widget = tempwid;
            tbl_col++;
            if (tbl_col == 2)
              {
                tbl_col = 0;
                tbl_num++;
                gtk_table_resize (GTK_TABLE (table), tbl_num + 1, 2);
              }
            break; 
          case CONFIG_LABEL:
            tempwid = gtk_label_new (_(config_file_vars[i].description));
            gtk_misc_set_alignment (GTK_MISC (tempwid), tbl_col, 0.5);
            gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 
                                    tbl_col, tbl_col + 1, tbl_num, tbl_num + 1);
            gtk_widget_show (tempwid);
            config_file_vars[i].widget = tempwid;
            tbl_col++;
            if (tbl_col == 2)
              {
                tbl_col = 0;
                tbl_num++;
                gtk_table_resize (GTK_TABLE (table), tbl_num + 1, 2);
              }
            break;
        }
    }

  combo_num = 0;
  combo_list = NULL;
  for (i = 0; gftp_protocols[i].name != NULL; i++)
    {
      if (strcmp (default_protocol, gftp_protocols[i].name) == 0)
        combo_num = i;
      tempwid = gtk_list_item_new_with_label (gftp_protocols[i].name);
      gtk_widget_show (tempwid);
      combo_list = g_list_append (combo_list, tempwid);
    }
  gtk_list_prepend_items (GTK_LIST (GTK_COMBO (def_proto_combo)->list), 
                          combo_list); 
  gtk_list_select_item (GTK_LIST (GTK_COMBO (def_proto_combo)->list), 
                        combo_num);

  combo_list = NULL;
  for (i = 0; proxy_type[i].key != NULL; i++)
    {
      tempwid = gtk_list_item_new_with_label (_(proxy_type[i].key));
      gtk_widget_show (tempwid);
      combo_list = g_list_append (combo_list, tempwid);
    }
  gtk_list_prepend_items (GTK_LIST (GTK_COMBO (proxy_combo)->list), combo_list);
  combo_list = NULL;

  custom_proxy = g_malloc0 (1);
  if (proxy_config == NULL || *proxy_config == '\0')
    combo_num = 0;
  else
    {
      pos = proxy_config;
      while ((endpos = strstr (pos, "%n")))
        {
          *endpos = '\0';
          oldstr = custom_proxy;
          custom_proxy = g_strconcat (custom_proxy, pos, "\n", NULL);
          g_free (oldstr);
          *endpos = '%';
          pos = endpos + 2;
        }

      for (combo_num = 1; combo_num < GFTP_CUSTOM_PROXY_NUM; combo_num++)
        {
          if (strcmp (proxy_type[combo_num].description, custom_proxy) == 0)
            break;
        }
    }

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (proxy_combo)->list),
                      "select_child", GTK_SIGNAL_FUNC (proxy_toggle), NULL);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (proxy_combo)->list), combo_num);

  make_proxy_hosts_tab (notebook);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
                      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
                      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (clean_old_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Apply"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
                      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (options_action), NULL);
#endif

  gtk_widget_show (dialog);
}


static void
make_proxy_hosts_tab (GtkWidget * notebook)
{
  GtkWidget *tempwid, *box, *hbox, *scroll;
  gftp_proxy_hosts *hosts, *newhosts;
  char *add_data[2];
  GList *templist;

  add_data[0] = _("Network");
  add_data[1] = _("Netmask");

  box = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box), 10);
  gtk_widget_show (box);

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
  gtk_widget_show (proxy_list);
  gtk_widget_show (scroll);

  hbox = gtk_hbox_new (TRUE, 15);
  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  tempwid = gtk_button_new_with_label (_("Add"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (add_proxy_host), NULL);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Edit"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (add_proxy_host), (gpointer) 1);
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Delete"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (delete_proxy_host), NULL);
  gtk_widget_show (tempwid);

  new_proxy_hosts = NULL;
  for (templist = proxy_hosts; templist != NULL;
       templist = templist->next)
    {
      hosts = templist->data;
      newhosts = g_malloc (sizeof (*newhosts));
      memcpy (newhosts, hosts, sizeof (*newhosts));
      if (newhosts->domain)
	{
	  newhosts->domain = g_malloc (strlen (hosts->domain) + 1);
	  strcpy (newhosts->domain, hosts->domain);
	}
      new_proxy_hosts = g_list_prepend (new_proxy_hosts, newhosts);
      add_host_to_listbox (new_proxy_hosts);
    }
}


static void
add_host_to_listbox (GList * templist)
{
  gftp_proxy_hosts *hosts;
  char *add_data[2];
  int num;

  hosts = templist->data;
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


#if GTK_MAJOR_VERSION > 1
static void
proxyhosts_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        add_ok (widget, user_data);
        /* no break */
      default:
        gtk_widget_destroy (widget);
    }
}
#endif


static void
add_proxy_host (GtkWidget * widget, gpointer data)
{
  GtkWidget *tempwid, *dialog, *frame, *box, *table;
  gftp_proxy_hosts *hosts;
  char *tempstr, *title;
  GList *templist;

  if (data)
    {
      if ((templist = GTK_CLIST (proxy_list)->selection) == NULL)
	return;
      templist = gtk_clist_get_row_data (GTK_CLIST (proxy_list), 
                                         (int) templist->data);
      hosts = templist->data;
    }
  else
    {
      hosts = NULL;
      templist = NULL;
    }

  title = hosts ? _("Edit Host") : _("Add Host");
#if GTK_MAJOR_VERSION == 1
  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_container_border_width (GTK_CONTAINER
			      (GTK_DIALOG (dialog)->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (dialog)->action_area), TRUE);
  gtk_grab_add (dialog);
#else
  dialog = gtk_dialog_new_with_buttons (title, NULL, 0,
                                        GTK_STOCK_SAVE,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
#endif
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "hostinfo", "Gftp");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, TRUE, TRUE,
		      0);
  gtk_widget_show (frame);

  box = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (box), 5);
  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_widget_show (box);

  tempwid = gtk_label_new (_("Domain"));
  gtk_box_pack_start (GTK_BOX (box), tempwid, TRUE, TRUE, 0);
  gtk_widget_show (tempwid);

  new_proxy_domain = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), new_proxy_domain, TRUE, TRUE, 0);
  gtk_widget_show (new_proxy_domain);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), frame, TRUE, TRUE,
		      0);
  gtk_widget_show (frame);

  table = gtk_table_new (4, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 5);
  gtk_table_set_row_spacings (GTK_TABLE (table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  tempwid = gtk_label_new (_("Network Address"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 0, 1);
  gtk_widget_show (tempwid);

  box = gtk_hbox_new (FALSE, 5);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 0, 1);
  gtk_widget_show (box);

  network1 = gtk_entry_new ();
  gtk_widget_set_size_request (network1, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), network1, TRUE, TRUE, 0);
  gtk_widget_show (network1);

  network2 = gtk_entry_new ();
  gtk_widget_set_size_request (network2, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), network2, TRUE, TRUE, 0);
  gtk_widget_show (network2);

  network3 = gtk_entry_new ();
  gtk_widget_set_size_request (network3, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), network3, TRUE, TRUE, 0);
  gtk_widget_show (network3);

  network4 = gtk_entry_new ();
  gtk_widget_set_size_request (network4, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), network4, TRUE, TRUE, 0);
  gtk_widget_show (network4);

  tempwid = gtk_label_new (_("Netmask"));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 1, 2);
  gtk_widget_show (tempwid);

  box = gtk_hbox_new (FALSE, 5);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 1, 2);
  gtk_widget_show (box);

  netmask1 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask1, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask1, TRUE, TRUE, 0);
  gtk_widget_show (netmask1);

  netmask2 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask2, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask2, TRUE, TRUE, 0);
  gtk_widget_show (netmask2);

  netmask3 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask3, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask3, TRUE, TRUE, 0);
  gtk_widget_show (netmask3);

  netmask4 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask4, 28, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask4, TRUE, TRUE, 0);
  gtk_widget_show (netmask4);

  box = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), box, TRUE, TRUE,
		      0);
  gtk_widget_show (box);

  domain_active = gtk_radio_button_new_with_label (NULL, _("Domain"));
  gtk_signal_connect (GTK_OBJECT (domain_active), "toggled",
		      GTK_SIGNAL_FUNC (add_toggle), (gpointer) 1);
  gtk_box_pack_start (GTK_BOX (box), domain_active, TRUE, TRUE, 0);
  gtk_widget_show (domain_active);

  tempwid = gtk_radio_button_new_with_label (gtk_radio_button_group
				     (GTK_RADIO_BUTTON (domain_active)),
				     _("Network"));
  gtk_signal_connect (GTK_OBJECT (tempwid), "toggled",
		      GTK_SIGNAL_FUNC (add_toggle), NULL);
  gtk_box_pack_start (GTK_BOX (box), tempwid, TRUE, TRUE, 0);
  gtk_widget_show (tempwid);

  if (!hosts || !hosts->domain)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tempwid), TRUE);
      add_toggle (NULL, NULL);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (domain_active), TRUE);
      add_toggle (NULL, (gpointer) 1);
    }

  if (hosts)
    {
      if (hosts->domain)
        gtk_entry_set_text (GTK_ENTRY (new_proxy_domain), hosts->domain);
      else
	{
	  tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 24 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (network1), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 16 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (network2), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address >> 8 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (network3), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_network_address & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (network4), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 24 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (netmask1), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 16 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (netmask2), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask >> 8 & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (netmask3), tempstr);
	  g_free (tempstr);

	  tempstr = g_strdup_printf ("%d", hosts->ipv4_netmask & 0xff);
	  gtk_entry_set_text (GTK_ENTRY (netmask4), tempstr);
	  g_free (tempstr);
	}
    }

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (add_ok), (gpointer) templist);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), tempwid,
		      TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy),
			     GTK_OBJECT (dialog));
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (dialog), "response",
                    G_CALLBACK (proxyhosts_action), NULL);
#endif

  gtk_widget_show (dialog);
}


static void
add_toggle (GtkWidget * widget, gpointer data)
{
  gtk_widget_set_sensitive (new_proxy_domain, data != NULL);
  gtk_widget_set_sensitive (network1, data == NULL);
  gtk_widget_set_sensitive (network2, data == NULL);
  gtk_widget_set_sensitive (network3, data == NULL);
  gtk_widget_set_sensitive (network4, data == NULL);
  gtk_widget_set_sensitive (netmask1, data == NULL);
  gtk_widget_set_sensitive (netmask2, data == NULL);
  gtk_widget_set_sensitive (netmask3, data == NULL);
  gtk_widget_set_sensitive (netmask4, data == NULL);
}


static void
add_ok (GtkWidget * widget, gpointer data)
{
  gftp_proxy_hosts *hosts;
  const char *edttxt;
  GList *templist;
  int num;

  if ((templist = data) == NULL)
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

  if (GTK_TOGGLE_BUTTON (domain_active)->active)
    {
      edttxt = gtk_entry_get_text (GTK_ENTRY (new_proxy_domain));
      hosts->domain = g_malloc (strlen (edttxt) + 1);
      strcpy (hosts->domain, edttxt);
      hosts->ipv4_netmask = hosts->ipv4_network_address = 0;
    }
  else
    {
      edttxt = gtk_entry_get_text (GTK_ENTRY (network1));
      hosts->ipv4_network_address = (strtol (edttxt, NULL, 10) & 0xff) << 24;

      edttxt = gtk_entry_get_text (GTK_ENTRY (network2));
      hosts->ipv4_network_address |= (strtol (edttxt, NULL, 10) & 0xff) << 16;

      edttxt = gtk_entry_get_text (GTK_ENTRY (network3));
      hosts->ipv4_network_address |= (strtol (edttxt, NULL, 10) & 0xff) << 8;

      edttxt = gtk_entry_get_text (GTK_ENTRY (network4));
      hosts->ipv4_network_address |= strtol (edttxt, NULL, 10) & 0xff;

      edttxt = gtk_entry_get_text (GTK_ENTRY (netmask1));
      hosts->ipv4_netmask = (strtol (edttxt, NULL, 10) & 0xff) << 24;

      edttxt = gtk_entry_get_text (GTK_ENTRY (netmask2));
      hosts->ipv4_netmask |= (strtol (edttxt, NULL, 10) & 0xff) << 16;

      edttxt = gtk_entry_get_text (GTK_ENTRY (netmask3));
      hosts->ipv4_netmask |= (strtol (edttxt, NULL, 10) & 0xff) << 8;

      edttxt = gtk_entry_get_text (GTK_ENTRY (netmask4));
      hosts->ipv4_netmask |= strtol (edttxt, NULL, 10) & 0xff;
    }
  add_host_to_listbox (templist);
}


static void
delete_proxy_host (GtkWidget * widget, gpointer data)
{
  GList *templist;
  int num;

  if ((templist = GTK_CLIST (proxy_list)->selection) == NULL)
    return;
  num = (int) templist->data;
  templist = gtk_clist_get_row_data (GTK_CLIST (proxy_list), num);
  new_proxy_hosts = g_list_remove_link (new_proxy_hosts, templist);
  gtk_clist_remove (GTK_CLIST (proxy_list), num);
}


static void
proxy_toggle (GtkList * list, GtkWidget * child, gpointer data)
{
  int proxy_num;
  char *str;

#if GTK_MAJOR_VERSION > 1
  GtkTextIter iter, iter2;
  GtkTextBuffer * textbuf;
  guint len;
#endif

  proxy_num = gtk_list_child_position (list, child);
  if (proxy_num == GFTP_CUSTOM_PROXY_NUM)
    str = custom_proxy;
  else
    str = proxy_type[proxy_num].description;

#if GTK_MAJOR_VERSION == 1
  gtk_text_set_point (GTK_TEXT (proxy_text), 0);
  gtk_text_forward_delete (GTK_TEXT (proxy_text),
			   gtk_text_get_length (GTK_TEXT (proxy_text)));

  gtk_text_insert (GTK_TEXT (proxy_text), NULL, NULL, NULL, str, -1);
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (proxy_text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len - 1);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);

  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len - 1);
  gtk_text_buffer_insert (textbuf, &iter, str, -1);
#endif
}


static void
apply_changes (GtkWidget * widget, gpointer data)
{
  const char *tempstr;
  int num, found, i;
  GList *templist;

  for (num = 0; config_file_vars[num].var != NULL; num++)
    {
      if (config_file_vars[num].widget != NULL)
        {
          switch (config_file_vars[num].type)
            {
              case CONFIG_CHECKBOX:
                *(int *) config_file_vars[num].var =
                      GTK_TOGGLE_BUTTON (config_file_vars[num].widget)->active;
                break;
              case CONFIG_INTTEXT:
              case CONFIG_UINTTEXT:
                tempstr = gtk_entry_get_text ( 
                               GTK_ENTRY (config_file_vars[num].widget));
                *(int *) config_file_vars[num].var = strtol (tempstr, NULL, 10);
                break;
              case CONFIG_FLOATTEXT:
                tempstr = gtk_entry_get_text ( 
                               GTK_ENTRY (config_file_vars[num].widget));
                *(double *) config_file_vars[num].var = strtod (tempstr, NULL);
                break;
              case CONFIG_CHARTEXT:
              case CONFIG_CHARPASS:
                tempstr = gtk_entry_get_text ( 
                               GTK_ENTRY (config_file_vars[num].widget));
                g_free (*(char **) config_file_vars[num].var);
                *(char **) config_file_vars[num].var = 
                                                g_malloc (strlen (tempstr) + 1);
                strcpy (*(char **) config_file_vars[num].var, tempstr);
                break;
            }
        }
    }

  tempstr = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (def_proto_combo)->entry));
  found = 0;
  for (i = 0; gftp_protocols[i].name; i++)
    {
      if (strcmp (gftp_protocols[i].name, tempstr) == 0)
        {
          found = 1;
          break;
        }
    }

  if (found)
    {
      g_free (default_protocol);
      default_protocol = g_strconcat (tempstr, NULL);
    }

  templist = proxy_hosts;
  proxy_hosts = new_proxy_hosts;
  new_proxy_hosts = templist;
  clean_old_changes (NULL, NULL);

  if (proxy_config != NULL)
    g_free (proxy_config);
  proxy_config = get_proxy_config ();

  gftp_write_config_file ();
}


static void
clean_old_changes (GtkWidget * widget, gpointer data)
{
  gftp_proxy_hosts *hosts;
  GList *templist;

  templist = new_proxy_hosts;
  while (templist != NULL)
    {
      hosts = templist->data;
      if (hosts->domain)
        g_free (hosts->domain);
      g_free (hosts);
      templist = templist->next;
    }
  g_list_free (new_proxy_hosts);
  new_proxy_hosts = NULL;

  if (custom_proxy != NULL)
    {
      g_free (custom_proxy);
      custom_proxy = NULL;
    }
}


static char *
get_proxy_config (void)
{
  char *newstr, *oldstr, *pos, *endpos, *textstr;
  guint len;
#if GTK_MAJOR_VERSION == 1
  char tmp[128];
#else
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
#endif

  textstr = NULL;
  newstr = g_malloc (1);
  *newstr = '\0';

#if GTK_MAJOR_VERSION == 1
  /*
     GTK_TEXT uses wchar_t instead of char in environment of multibyte encoding
     locale (ex Japanese),  so we must convert from wide character 
     to multibyte charator....   Yasuyuki Furukawa (yasu@on.cs.keio.ac.jp)
   */
  if (GTK_TEXT (proxy_text)->use_wchar)
    {
      wcstombs (tmp, (wchar_t *) GTK_TEXT (proxy_text)->text.wc,
                sizeof (tmp));
      pos = tmp;
    }
  else
    {
      oldstr = (char *) GTK_TEXT (proxy_text)->text.ch; 
      len = gtk_text_get_length (GTK_TEXT (proxy_text));
      textstr = g_malloc (len + 1);
      strncpy (textstr, oldstr, len);
      textstr[len] = '\0';
      pos = textstr;
    }
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (proxy_text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len - 1);
  pos = textstr = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
#endif

  do
    {
      if ((endpos = strchr (pos, '\n')) != NULL)
        *endpos = '\0';
      oldstr = newstr;
      if (endpos != NULL)
        newstr = g_strconcat (newstr, pos, "%n", NULL);
      else
        newstr = g_strconcat (newstr, pos, NULL);
      g_free (oldstr);
      if (endpos != NULL)
        {
          *endpos = '\n';
          pos = endpos + 1;
        }
    }
  while (endpos != NULL);

#if GTK_MAJOR_VERSION == 1
  if (!GTK_TEXT (proxy_text)->use_wchar)
    g_free (textstr);
#else
  g_free (textstr);
#endif

  return (newstr);
}

