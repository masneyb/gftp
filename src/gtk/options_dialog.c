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

static GtkWidget * proxy_list, * new_proxy_domain, * network1,
                 * network2, * network3, * network4, * netmask1, * netmask2, 
                 * netmask3, * netmask4, * domain_active;
static GList * new_proxy_hosts;

/* FIXME - use cancel function */

static void
_setup_option (gftp_option_type_enum otype,
               gftp_options_dialog_data * option_data, 
               void * (*ui_print_function) (gftp_config_vars * cv,
                                                 void *user_data),
               void (*ui_save_function) (gftp_config_vars * cv,
                                         void *user_data),
               void (*ui_cancel_function) (gftp_config_vars * cv,
                                           void *user_data))

{
  gftp_option_types[otype].user_data = option_data;
  gftp_option_types[otype].ui_print_function = ui_print_function;
  gftp_option_types[otype].ui_save_function = ui_save_function;
  gftp_option_types[otype].ui_cancel_function = ui_cancel_function;
}


static void *
_gen_input_widget (gftp_options_dialog_data * option_data, char *label, char *tiptxt)
{
  GtkTooltips * tooltip;
  GtkWidget * tempwid;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                    option_data->tbl_row_num, 2);

  tempwid = gtk_label_new (_(label));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), tempwid, 0, 1,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (tempwid);

  tempwid = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), tempwid, 1, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (tempwid);

  if (tiptxt != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), tempwid, tiptxt, NULL);
    }

  return (tempwid);
}


static void *
_print_option_type_newtable (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;

  option_data = user_data;

  option_data->table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (option_data->table), 5);
  gtk_table_set_col_spacings (GTK_TABLE (option_data->table), 5);
  gtk_box_pack_start (GTK_BOX (option_data->box), option_data->table, FALSE, 
                      FALSE, 0);
  gtk_widget_show (option_data->table);
  option_data->tbl_row_num = 0;
  option_data->tbl_col_num = 0;

  return (NULL);
}


static void *
_print_option_type_text (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  gtk_entry_set_text (GTK_ENTRY (tempwid), (char *) cv->value);
  return (tempwid);
}


static void
_save_option_type_text (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM)
    g_free (cv->value);

  cv->value = g_strdup (tempstr);
  cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;
}


static GtkWidget *
_gen_combo_widget (gftp_options_dialog_data * option_data, char *label)
{
  GtkWidget * tempwid, * combo;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                               option_data->tbl_row_num, 2);

  tempwid = gtk_label_new (_(label));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 1, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), tempwid, 0, 1,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (tempwid);

  combo = gtk_combo_new ();
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), combo, 1, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  return (combo);
}


static void *
_print_option_type_textcombo (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid, * combo;
  GList * widget_list;
  GtkTooltips * tooltip;
  int selitem, i;
  char **clist;

  option_data = user_data;
  combo = _gen_combo_widget (option_data, cv->description);

  if (cv->listdata != NULL)
    {
      selitem = 0;
      widget_list = NULL;

      clist = cv->listdata;
      for (i=0; clist[i] != NULL; i++)
        {
          if (cv->value != NULL &&
              strcasecmp ((char *) cv->value, clist[i]) == 0)
            selitem = i;

          tempwid = gtk_list_item_new_with_label (clist[i]);
          gtk_widget_show (tempwid);
          widget_list = g_list_append (widget_list, tempwid);
        }

      gtk_list_prepend_items (GTK_LIST (GTK_COMBO (combo)->list), widget_list); 
      gtk_list_select_item (GTK_LIST (GTK_COMBO (combo)->list), selitem);
    }

  gtk_widget_show (combo);

  if (cv->comment != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), combo, cv->comment, NULL);
    }

  return (combo);
}


static void
_save_option_type_textcombo (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;
  char **clist;
  int i;

  option_data = user_data;

  tempstr = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->user_data)->entry));

  if (cv->listdata != NULL && tempstr != NULL)
    {
      clist = cv->listdata;
      for (i=0; clist[i] != NULL; i++)
        {
          if (strcasecmp (tempstr, clist[i]) == 0)
            {
              cv->value = clist[i];
              break;
            }
        }
    }
}


static void
_textcomboedt_toggle (GtkList * list, GtkWidget * child, gpointer data)
{
  gftp_textcomboedt_widget_data * widdata;
  gftp_textcomboedt_data * tedata;
  char *insert_text;
  int num, isedit;
#if GTK_MAJOR_VERSION > 1
  GtkTextIter iter, iter2;
  GtkTextBuffer * textbuf;
  guint len;
#endif

  widdata = data;
  tedata = widdata->cv->listdata;

  num = gtk_list_child_position (list, child);
  isedit = tedata[num].flags & GFTP_TEXTCOMBOEDT_EDITABLE;
#if GTK_MAJOR_VERSION == 1
  gtk_text_set_editable (GTK_TEXT (widdata->text), isedit);
#else
  gtk_text_view_set_editable (GTK_TEXT_VIEW (widdata->text), isedit);
#endif

  if (isedit)
    insert_text = widdata->custom_edit_value;
  else
    insert_text = tedata[num].text;

#if GTK_MAJOR_VERSION == 1
  gtk_text_set_point (GTK_TEXT (widdata->text), 0);
  gtk_text_forward_delete (GTK_TEXT (widdata->text),
			   gtk_text_get_length (GTK_TEXT (widdata->text)));

  gtk_text_insert (GTK_TEXT (widdata->text), NULL, NULL, NULL, 
                   insert_text, -1);
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widdata->text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len - 1);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);

  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len - 1);
  gtk_text_buffer_insert (textbuf, &iter, insert_text, -1);
#endif
}


static char *
_gftp_convert_to_newlines (char *str)
{
  char *stpos, *endpos, *ret, savechar;
  size_t len;

  ret = g_strdup ("");
  len = 0;
  for (stpos = str; 
       (endpos = strstr (stpos, "%n")) != NULL;
       stpos = endpos + 2)
    { 
      savechar = *endpos;
      *endpos = '\0';

      len += strlen (stpos) + 1;
      ret = g_realloc (ret, len + 1);
      strcat (ret, stpos);
      strcat (ret, "\n");

      *endpos = savechar;
    }

  return (ret);
}


static char *
_gftp_convert_from_newlines (char *str)
{
  char *stpos, *endpos, *ret, savechar;
  size_t len;

  ret = g_strdup ("");
  len = 0;
  for (stpos = str; 
       (endpos = strchr (stpos, '\n')) != NULL;
       stpos = endpos + 1)
    { 
      savechar = *endpos;
      *endpos = '\0';

      len += strlen (stpos) + 2;
      ret = g_realloc (ret, len + 1);
      strcat (ret, stpos);
      strcat (ret, "%n");

      *endpos = savechar;
    }

  return (ret);
}


static void *
_print_option_type_textcomboedt (gftp_config_vars * cv, void *user_data)
{
  gftp_textcomboedt_widget_data * widdata;
  GtkWidget * combo, * textwid, * tempwid;
  gftp_options_dialog_data * option_data;
  gftp_textcomboedt_data * tedata;
  int i, selitem, edititem;
  GtkTooltips * tooltip;
  GList * widget_list;
  char *tempstr;

  option_data = user_data;
  combo = _gen_combo_widget (option_data, cv->description);
 
  tempstr = NULL;
  if (cv->value != NULL)
    tempstr = _gftp_convert_to_newlines (cv->value);
  if (tempstr == NULL)
    tempstr = g_strdup ("");

  edititem = selitem = -1;
  if (cv->listdata != NULL)
    {
      widget_list = NULL;

      tedata = cv->listdata;
      for (i=0; tedata[i].description != NULL; i++)
        {
          if (tedata[i].flags & GFTP_TEXTCOMBOEDT_EDITABLE)
            edititem = i;

          if (selitem == -1 &&
              strcasecmp (tempstr, tedata[i].text) == 0)
            selitem = i;

          tempwid = gtk_list_item_new_with_label (tedata[i].description);
          gtk_widget_show (tempwid);
          widget_list = g_list_append (widget_list, tempwid);
        }

      gtk_list_prepend_items (GTK_LIST (GTK_COMBO (combo)->list), widget_list); 

      if (selitem == -1 && edititem != -1)
        selitem = edititem;
    }

  if (selitem == -1)
    selitem = 0;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                               option_data->tbl_row_num, 2);

#if GTK_MAJOR_VERSION == 1
  textwid = gtk_text_new (NULL, NULL);
#else
  textwid = gtk_text_view_new ();
#endif
  gtk_widget_set_size_request (textwid, -1, 75);
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), textwid, 0, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (textwid);

  widdata = g_malloc0 (sizeof (*widdata));
  widdata->combo = combo;
  widdata->text = textwid;
  widdata->cv = cv;
  widdata->custom_edit_value = tempstr;

  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (combo)->list),
                      "select_child", 
                      GTK_SIGNAL_FUNC (_textcomboedt_toggle), widdata);
  gtk_list_select_item (GTK_LIST (GTK_COMBO (combo)->list), selitem);
  gtk_widget_show (combo);

  if (cv->comment != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), combo, cv->comment, NULL);

      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), textwid, cv->comment, NULL);
    }

  return (widdata);
}


static void
_cancel_option_type_textcomboedt (gftp_config_vars * cv, void *user_data)
{
  gftp_textcomboedt_widget_data * widdata;

  widdata = cv->user_data;
  if (widdata != NULL)
    {
      if (widdata->custom_edit_value != NULL)
        g_free (widdata->custom_edit_value);
      g_free (widdata);
    }
}


static void
_save_option_type_textcomboedt (gftp_config_vars * cv, void *user_data)
{
  gftp_textcomboedt_widget_data * widdata;
  char *newstr;
  int freeit;
#if GTK_MAJOR_VERSION == 1
  char tmp[128];
#else
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  size_t len;
#endif

  widdata = cv->user_data;

#if GTK_MAJOR_VERSION == 1
  /*
     GTK_TEXT uses wchar_t instead of char in environment of multibyte encoding
     locale (ex Japanese),  so we must convert from wide character 
     to multibyte charator....   Yasuyuki Furukawa (yasu@on.cs.keio.ac.jp)
   */
  if (GTK_TEXT (widdata->text)->use_wchar)
    {
      wcstombs (tmp, (wchar_t *) GTK_TEXT (widdata->text)->text.wc, sizeof (tmp));
      newstr = tmp;
      freeit = 0;
    }
  else
    {
      newstr = (char *) GTK_TEXT (widdata->text)->text.ch; 
      freeit = 0;
    }
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widdata->text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len - 1);
  newstr = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
  freeit = 1;
#endif

  if (cv->flags & GFTP_CVARS_FLAGS_DYNMEM)
    g_free (cv->value);

  cv->value = _gftp_convert_from_newlines (newstr);
  cv->flags |= GFTP_CVARS_FLAGS_DYNMEM;

  if (freeit)
    g_free (newstr);

  _cancel_option_type_textcomboedt (cv, user_data);
}


static void *
_print_option_type_hidetext (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  gtk_entry_set_visibility (GTK_ENTRY (tempwid), 0);
  gtk_entry_set_text (GTK_ENTRY (tempwid), (char *) cv->value);
  return (tempwid);
}


static void *
_print_option_type_int (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;
  char tempstr[20];

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  g_snprintf (tempstr, sizeof (tempstr), "%d", GPOINTER_TO_INT(cv->value));
  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
  return (tempwid);
}


static void
_save_option_type_int (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));
  cv->value = GINT_TO_POINTER(strtol (tempstr, NULL, 10));
}


static void *
_print_option_type_checkbox (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkTooltips * tooltip;
  GtkWidget * tempwid;

  option_data = user_data;

  if (option_data->last_option != gftp_option_type_checkbox)
    _print_option_type_newtable (NULL, user_data);

  if (option_data->tbl_col_num == 0)
    {
      option_data->tbl_row_num++;
      gtk_table_resize (GTK_TABLE (option_data->table), 
                        option_data->tbl_row_num + 1, 2);
    }

  tempwid = gtk_check_button_new_with_label (_(cv->description));
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), tempwid, 
                             option_data->tbl_col_num, 
                             option_data->tbl_col_num + 1, 
                             option_data->tbl_row_num, 
                             option_data->tbl_row_num + 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tempwid),
                                GPOINTER_TO_INT(cv->value));
  gtk_widget_show (tempwid);

  option_data->tbl_col_num = (option_data->tbl_col_num + 1) % 2;

  if (cv->comment != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), tempwid, cv->comment, NULL);
    }

  return (tempwid);
}


static void
_save_option_type_checkbox (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;

  option_data = user_data;
  cv->value = GINT_TO_POINTER (GTK_TOGGLE_BUTTON (cv->user_data)->active);
}


static void *
_print_option_type_float (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;
  char tempstr[20];
  float f;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  memcpy (&f, &cv->value, sizeof (f));
  g_snprintf (tempstr, sizeof (tempstr), "%.2f", f);
  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
  return (tempwid);
}


static void
_save_option_type_float (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;
  float f;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));
  f = strtod (tempstr, NULL);
  memcpy (&cv->value, &f, sizeof (cv->value));
}


static void *
_print_option_type_notebook (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  option_data->box = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (option_data->box), 10);
  gtk_widget_show (option_data->box);

  tempwid = gtk_label_new (_(cv->description));
  gtk_widget_show (tempwid);
  gtk_notebook_append_page (GTK_NOTEBOOK (option_data->notebook), 
                            option_data->box, tempwid);

  _print_option_type_newtable (NULL, user_data);
  
  return (NULL);
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
}


static void
apply_changes (GtkWidget * widget, gpointer data)
{
  gftp_config_vars * cv;
  GList * templist;
  int i;

  for (templist = gftp_options_list; 
       templist != NULL; 
       templist = templist->next)
    {
      cv = templist->data;

      for (i=0; cv[i].key != NULL; i++)
        {
          if (!(cv[i].ports_shown & GFTP_PORT_GTK))
            continue;

          if (gftp_option_types[cv[i].otype].ui_save_function == NULL)
            continue;

          gftp_option_types[cv[i].otype].ui_save_function (&cv[i], gftp_option_types[cv[i].otype].user_data);
        }
    }
}


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
delete_proxy_host (GtkWidget * widget, gpointer data)
{
  GList *templist;
  int num;

  if ((templist = GTK_CLIST (proxy_list)->selection) == NULL)
    return;
  num = GPOINTER_TO_INT (templist->data);
  templist = gtk_clist_get_row_data (GTK_CLIST (proxy_list), num);
  new_proxy_hosts = g_list_remove_link (new_proxy_hosts, templist);
  gtk_clist_remove (GTK_CLIST (proxy_list), num);
}


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
                                         GPOINTER_TO_INT (templist->data));
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
make_proxy_hosts_tab (GtkWidget * notebook)
{
  GtkWidget *tempwid, *box, *hbox, *scroll;
  char *add_data[2];

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
}


void
options_dialog (gpointer data)
{
  gftp_options_dialog_data option_data;
  gftp_config_vars * cv;
  GList * templist;
  int i;
#if GTK_MAJOR_VERSION == 1
  GtkWidget * tempwid;
#endif

  memset (&option_data, 0, sizeof (option_data));
  _setup_option (gftp_option_type_text, &option_data, 
                 _print_option_type_text, _save_option_type_text, NULL);
  _setup_option (gftp_option_type_textcombo, &option_data, 
                 _print_option_type_textcombo, _save_option_type_textcombo, 
                 NULL);
  _setup_option (gftp_option_type_textcomboedt, &option_data, 
                 _print_option_type_textcomboedt, 
                 _save_option_type_textcomboedt,
                 _cancel_option_type_textcomboedt);
  _setup_option (gftp_option_type_hidetext, &option_data, 
                 _print_option_type_hidetext, _save_option_type_text, NULL);
  _setup_option (gftp_option_type_int, &option_data, 
                 _print_option_type_int, _save_option_type_int, NULL);
  _setup_option (gftp_option_type_checkbox, &option_data, 
                 _print_option_type_checkbox, _save_option_type_checkbox, NULL);
  _setup_option (gftp_option_type_float, &option_data, 
                 _print_option_type_float, _save_option_type_float, NULL);
  _setup_option (gftp_option_type_notebook, &option_data, 
                 _print_option_type_notebook, NULL, NULL);

#if GTK_MAJOR_VERSION == 1
  option_data.dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (option_data.dialog), _("Options"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (option_data.dialog)->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (option_data.dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (option_data.dialog)->action_area), TRUE);
#else
  option_data.dialog = gtk_dialog_new_with_buttons (_("Options"), NULL, 0,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_APPLY,
                                        GTK_RESPONSE_APPLY,
                                        NULL);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(option_data.dialog), "options", "gFTP");
  gtk_window_set_position (GTK_WINDOW (option_data.dialog), GTK_WIN_POS_MOUSE);
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (option_data.dialog)->vbox), 10);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (option_data.dialog)->vbox), 2);
  gtk_widget_realize (option_data.dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (option_data.dialog->window, NULL, gftp_icon->pixmap,
                           gftp_icon->bitmap);
      gdk_window_set_icon_name (option_data.dialog->window, gftp_version);
    }

  option_data.notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (option_data.dialog)->vbox), 
                      option_data.notebook, TRUE, TRUE, 0);
  gtk_widget_show (option_data.notebook);

  cv = gftp_options_list->data;
  option_data.last_option = cv[0].otype;
  for (templist = gftp_options_list; 
       templist != NULL; 
       templist = templist->next)
    {
      cv = templist->data;

      for (i=0; cv[i].key != NULL; i++)
        {
          if (!(cv[i].ports_shown & GFTP_PORT_GTK))
            continue;

          if (gftp_option_types[cv[i].otype].ui_print_function == NULL)
            continue;

          cv[i].user_data = gftp_option_types[cv[i].otype].ui_print_function (&cv[i], gftp_option_types[cv[i].otype].user_data);

          option_data.last_option = cv[i].otype;
        }
    }

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (option_data.dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (option_data.dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (option_data.dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (clean_old_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (option_data.dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Apply"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (option_data.dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (option_data.dialog), "response",
                    G_CALLBACK (options_action), NULL);
#endif

  gtk_widget_show (option_data.dialog);
}

