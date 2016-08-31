/*****************************************************************************/
/*  menu-items.c - menu callbacks                                            */
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
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/*****************************************************************************/

#include "gftp-gtk.h"
static const char cvsid[] = "$Id$";

static GtkWidget * proxy_list, * new_proxy_domain, * network1,
                 * network2, * network3, * network4, * netmask1, * netmask2, 
                 * netmask3, * netmask4, * domain_active,
                 * domain_label, * network_label, * netmask_label, 
                 * edit_button, * delete_button;
static gftp_options_dialog_data * gftp_option_data;
static GList * new_proxy_hosts = NULL;

static void
_setup_option (gftp_option_type_enum otype,
               gftp_options_dialog_data * option_data, 
               void * (*ui_print_function) (gftp_config_vars * cv,
                                            void *user_data,
                                            void *value),
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
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0.5);
  gtk_table_attach (GTK_TABLE (option_data->table), tempwid, 0, 1,
                    option_data->tbl_row_num - 1, 
                    option_data->tbl_row_num,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_show (tempwid);

  tempwid = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), tempwid, 1, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (tempwid);

  if (tiptxt != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), tempwid, _(tiptxt), NULL);
    }

  return (tempwid);
}


static void *
_print_option_type_newtable (void *user_data)
{
  gftp_options_dialog_data * option_data;

  option_data = user_data;

  option_data->table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (option_data->table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (option_data->table), 12);
  gtk_box_pack_start (GTK_BOX (option_data->box), option_data->table, FALSE, 
                      FALSE, 0);
  gtk_widget_show (option_data->table);
  option_data->tbl_row_num = 0;
  option_data->tbl_col_num = 0;

  return (NULL);
}


static void *
_print_option_type_text (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  if (value != NULL)
    gtk_entry_set_text (GTK_ENTRY (tempwid), (char *) value);

  return (tempwid);
}


static void
_save_option_type_text (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, tempstr);
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, tempstr);
}


static GtkWidget *
_gen_combo_widget (gftp_options_dialog_data * option_data, char *label)
{
  GtkWidget * tempwid, * combo;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                               option_data->tbl_row_num, 2);

  tempwid = gtk_label_new (_(label));
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0.5);
  gtk_table_attach (GTK_TABLE (option_data->table), tempwid, 0, 1,
                    option_data->tbl_row_num - 1, 
                    option_data->tbl_row_num,
		    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_show (tempwid);

  combo = gtk_combo_new ();
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), combo, 1, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  return (combo);
}


static void *
_print_option_type_textcombo (gftp_config_vars * cv, void *user_data, void *value)
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
          if (value != NULL && strcasecmp ((char *) value, clist[i]) == 0)
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
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), combo, _(cv->comment), NULL);
    }

  return (combo);
}


static void
_save_option_type_textcombo (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;

  option_data = user_data;

  tempstr = gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (cv->user_data)->entry));

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, tempstr);
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, tempstr);
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
  gint len;
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
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);

  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len);
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
      ret = g_realloc (ret, (gulong) len + 1);
      strcat (ret, stpos);
      strcat (ret, "\n");

      *endpos = savechar;
    }

  if (stpos != NULL && *stpos != '\0')
    {
      len += strlen (stpos);
      ret = g_realloc (ret, (gulong) len + 1);
      strcat (ret, stpos);
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
      ret = g_realloc (ret, (gulong) len + 1);
      strcat (ret, stpos);
      strcat (ret, "%n");

      *endpos = savechar;
    }

  if (stpos != NULL && *stpos != '\0')
    {
      len += strlen (stpos);
      ret = g_realloc (ret, (gulong) len + 1);
      strcat (ret, stpos);
    }

  return (ret);
}


static void *
_print_option_type_textcomboedt (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_textcomboedt_widget_data * widdata;
  GtkWidget * box, * combo, * textwid, * tempwid;
  gftp_options_dialog_data * option_data;
  gftp_textcomboedt_data * tedata;
  int i, selitem, edititem;
  GtkTooltips * tooltip;
  GList * widget_list;
  char *tempstr;

  option_data = user_data;
  combo = _gen_combo_widget (option_data, cv->description);
 
  tempstr = NULL;
  if (value != NULL)
    tempstr = _gftp_convert_to_newlines (value);
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
  gtk_widget_set_size_request (textwid, -1, 75);
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), textwid, 0, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  gtk_widget_show (textwid);
#else
  box = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), box, 0, 2,
                    	     option_data->tbl_row_num - 1, 
                    	     option_data->tbl_row_num);
  gtk_widget_show (box);
  
  tempwid = gtk_label_new ("    ");
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);
  
  tempwid = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (tempwid), 0);
  gtk_widget_set_size_request (tempwid, -1, 75);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tempwid), 
                                       GTK_SHADOW_IN); 
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tempwid),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (box), tempwid, TRUE, TRUE, 0);
  gtk_widget_show (tempwid);
  
  textwid = gtk_text_view_new ();
  gtk_container_add (GTK_CONTAINER (tempwid), GTK_WIDGET (textwid));
  gtk_widget_show (textwid);
#endif

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
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), combo, _(cv->comment), NULL);

      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), textwid, _(cv->comment), 
                            NULL);
    }

  return (widdata);
}


static void
_save_option_type_textcomboedt (gftp_config_vars * cv, void *user_data)
{
  gftp_textcomboedt_widget_data * widdata;
  gftp_options_dialog_data * option_data;
  char *newstr, *proxy_config;
  int freeit;
#if GTK_MAJOR_VERSION == 1
  char tmp[128];
#else
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  size_t len;
#endif

  option_data = user_data;
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
    }
  else
    newstr = (char *) GTK_TEXT (widdata->text)->text.ch; 

  freeit = 0;
#else
  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widdata->text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  newstr = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
  freeit = 1;
#endif

  proxy_config = _gftp_convert_from_newlines (newstr);

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, proxy_config);
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, proxy_config);

  g_free (proxy_config);

  if (freeit)
    g_free (newstr);
}


static void *
_print_option_type_hidetext (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  gtk_entry_set_visibility (GTK_ENTRY (tempwid), 0);
  gtk_entry_set_text (GTK_ENTRY (tempwid), (char *) value);
  return (tempwid);
}


static void *
_print_option_type_int (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;
  char tempstr[20];

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  g_snprintf (tempstr, sizeof (tempstr), "%d", GPOINTER_TO_INT(value));
  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
  return (tempwid);
}


static void
_save_option_type_int (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;
  intptr_t val;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));

  val = strtol (tempstr, NULL, 10);

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, GINT_TO_POINTER(val));
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, GINT_TO_POINTER(val));
}


static void *
_print_option_type_checkbox (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkTooltips * tooltip;
  GtkWidget * tempwid;

  option_data = user_data;

  if (option_data->last_option != gftp_option_type_checkbox)
    _print_option_type_newtable (user_data);

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
                                GPOINTER_TO_INT(value));
  gtk_widget_show (tempwid);

  option_data->tbl_col_num = (option_data->tbl_col_num + 1) % 2;

  if (cv->comment != NULL)
    {
      tooltip = gtk_tooltips_new ();
      gtk_tooltips_set_tip (GTK_TOOLTIPS(tooltip), tempwid, _(cv->comment),
                            NULL);
    }

  return (tempwid);
}


static void
_save_option_type_checkbox (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  intptr_t val;

  option_data = user_data;

  val = GTK_TOGGLE_BUTTON (cv->user_data)->active;

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, GINT_TO_POINTER (val));
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, GINT_TO_POINTER (val));
}


static void *
_print_option_type_float (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;
  char tempstr[20];
  float f;

  option_data = user_data;

  tempwid = _gen_input_widget (option_data, cv->description, cv->comment);
  memcpy (&f, &value, sizeof (f));
  g_snprintf (tempstr, sizeof (tempstr), "%.2f", f);
  gtk_entry_set_text (GTK_ENTRY (tempwid), tempstr);
  return (tempwid);
}


static void
_save_option_type_float (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  union { void *ptr; float f; } fv;
  const char *tempstr;

  option_data = user_data;
  tempstr = gtk_entry_get_text (GTK_ENTRY (cv->user_data));
  fv.f = strtod (tempstr, NULL);

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, fv.ptr);
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, fv.ptr);
}


static void *
_print_option_type_notebook (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * tempwid;

  option_data = user_data;

  option_data->box = gtk_vbox_new (FALSE, 6);
  gtk_container_border_width (GTK_CONTAINER (option_data->box), 12);
  gtk_widget_show (option_data->box);

  tempwid = gtk_label_new (_(cv->description));
  gtk_widget_show (tempwid);
  gtk_notebook_append_page (GTK_NOTEBOOK (option_data->notebook), 
                            option_data->box, tempwid);

  _print_option_type_newtable (user_data);
  
  return (NULL);
}


static void
clean_old_changes (GtkWidget * widget, gpointer data)
{
  gftp_textcomboedt_widget_data * widdata;
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
          widdata = cv->user_data;
          if (widdata != NULL)
            {
              if (widdata->custom_edit_value != NULL)
                g_free (widdata->custom_edit_value);
              g_free (widdata);
              cv->user_data = NULL;
            }
        }
    }

  if (new_proxy_hosts != NULL)
    {
      gftp_free_proxy_hosts (new_proxy_hosts);
      new_proxy_hosts = NULL;
    }
}


static void
apply_changes (GtkWidget * widget, gpointer data)
{
  gftp_config_list_vars * proxy_hosts;
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

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);

  if (proxy_hosts->list != NULL)
    gftp_free_proxy_hosts (proxy_hosts->list);

  proxy_hosts->list = new_proxy_hosts;
  new_proxy_hosts = NULL;
  gftpui_show_or_hide_command ();
}


#if GTK_MAJOR_VERSION > 1
static void
options_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response)
    {
      case GTK_RESPONSE_OK:
        apply_changes (widget, NULL);
        /* no break */
      default:
        clean_old_changes (widget, user_data);
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

  templist = data;
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

  if (GTK_TOGGLE_BUTTON (domain_active)->active)
    {
      edttxt = gtk_entry_get_text (GTK_ENTRY (new_proxy_domain));
      hosts->domain = g_strdup (edttxt);
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
  gtk_widget_set_sensitive (domain_label, data != NULL);
  gtk_widget_set_sensitive (network_label, data == NULL);
  gtk_widget_set_sensitive (netmask_label, data == NULL);
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
add_proxy_host (GtkWidget * widget, gpointer data)
{
  GtkWidget *tempwid, *dialog, *box, *rbox, *vbox, *nradio, *table;
  gftp_proxy_hosts *hosts;
  char *tempstr, *title;
  GList *templist;

  gftp_configuration_changed = 1; /* FIXME */

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
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), 10);
#else
  dialog = gtk_dialog_new_with_buttons (title, NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE,
                                        GTK_RESPONSE_OK,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
#endif
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
  gtk_window_set_wmclass (GTK_WINDOW(dialog), "hostinfo", "Gftp");
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);

#if GTK_MAJOR_VERSION > 1
  if (gftp_icon != NULL)
    {
      if ((tempstr = get_xpm_path (gftp_icon->filename, 0)) != NULL)
        {
         gtk_window_set_default_icon_from_file (tempstr, NULL);
	 g_free (tempstr);
        }
    }
#endif

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_border_width (GTK_CONTAINER (vbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);
    
  box = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);
  
#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_label_new (_("Type:"));
#else
  tempwid = gtk_label_new_with_mnemonic (_("_Type:"));
#endif
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);
  
  rbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (box), rbox, TRUE, TRUE, 0);
  gtk_widget_show (rbox);
  
  domain_active = gtk_radio_button_new_with_label (NULL, _("Domain"));
  gtk_signal_connect (GTK_OBJECT (domain_active), "toggled",
		      GTK_SIGNAL_FUNC (add_toggle), (gpointer) 1);
  
  nradio = gtk_radio_button_new_with_label (gtk_radio_button_group
                                            (GTK_RADIO_BUTTON (domain_active)),
                                           _("Network"));
  gtk_signal_connect (GTK_OBJECT (nradio), "toggled",
		      GTK_SIGNAL_FUNC (add_toggle), NULL);
#if GTK_MAJOR_VERSION > 1
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), nradio);
#endif

  gtk_box_pack_start (GTK_BOX (rbox), nradio, TRUE, TRUE, 0);
  gtk_widget_show (nradio);
  gtk_box_pack_start (GTK_BOX (rbox), domain_active, TRUE, TRUE, 0);
  gtk_widget_show (domain_active);

  box = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  tempwid = gtk_label_new ("    ");
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 12);
  gtk_box_pack_start (GTK_BOX (box), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_label_new (_("Network Address"));
#else
  tempwid = gtk_label_new_with_mnemonic (_("_Network address:"));
#endif
  network_label = tempwid;
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 0, 1);
  gtk_widget_show (tempwid);

  box = gtk_hbox_new (FALSE, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 0, 1);
  gtk_widget_show (box);

  network1 = gtk_entry_new ();
  gtk_widget_set_size_request (network1, 36, -1);
#if GTK_MAJOR_VERSION > 1
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), network1);
#endif

  gtk_box_pack_start (GTK_BOX (box), network1, TRUE, TRUE, 0);
  gtk_widget_show (network1);

  network2 = gtk_entry_new ();
  gtk_widget_set_size_request (network2, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), network2, TRUE, TRUE, 0);
  gtk_widget_show (network2);

  network3 = gtk_entry_new ();
  gtk_widget_set_size_request (network3, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), network3, TRUE, TRUE, 0);
  gtk_widget_show (network3);

  network4 = gtk_entry_new ();
  gtk_widget_set_size_request (network4, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), network4, TRUE, TRUE, 0);
  gtk_widget_show (network4);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_label_new (_("Netmask"));
#else
  tempwid = gtk_label_new_with_mnemonic (_("N_etmask:"));
#endif
  netmask_label = tempwid;
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), tempwid, 0, 1, 1, 2);
  gtk_widget_show (tempwid);

  box = gtk_hbox_new (FALSE, 6);
  gtk_table_attach_defaults (GTK_TABLE (table), box, 1, 2, 1, 2);
  gtk_widget_show (box);

  netmask1 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask1, 36, -1);
#if GTK_MAJOR_VERSION > 1
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), netmask1);
#endif

  gtk_box_pack_start (GTK_BOX (box), netmask1, TRUE, TRUE, 0);
  gtk_widget_show (netmask1);

  netmask2 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask2, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask2, TRUE, TRUE, 0);
  gtk_widget_show (netmask2);

  netmask3 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask3, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask3, TRUE, TRUE, 0);
  gtk_widget_show (netmask3);

  netmask4 = gtk_entry_new ();
  gtk_widget_set_size_request (netmask4, 36, -1);

  gtk_box_pack_start (GTK_BOX (box), netmask4, TRUE, TRUE, 0);
  gtk_widget_show (netmask4);

  box = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  tempwid = gtk_label_new ("    ");
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_label_new (_("Domain"));
#else
  tempwid = gtk_label_new_with_mnemonic (_("_Domain:"));
#endif
  domain_label = tempwid;
  gtk_misc_set_alignment (GTK_MISC (tempwid), 0, 0.5);
  gtk_box_pack_start (GTK_BOX (box), tempwid, FALSE, FALSE, 0);
  gtk_widget_show (tempwid);

  new_proxy_domain = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (box), new_proxy_domain, TRUE, TRUE, 0);
  gtk_widget_show (new_proxy_domain);
#if GTK_MAJOR_VERSION > 1
  gtk_label_set_mnemonic_widget (GTK_LABEL (tempwid), new_proxy_domain);
#endif

  if (!hosts || !hosts->domain)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nradio), TRUE);
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
  gftp_config_list_vars * proxy_hosts;
  char *add_data[2];
  GList * templist;

  add_data[0] = _("Network");
  add_data[1] = _("Netmask");

  box = gtk_vbox_new (FALSE, 6);
  gtk_container_border_width (GTK_CONTAINER (box), 12);
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

  gftp_lookup_global_option ("dont_use_proxy", &proxy_hosts);
  new_proxy_hosts = gftp_copy_proxy_hosts (proxy_hosts->list);

  for (templist = new_proxy_hosts;
       templist != NULL;
       templist = templist->next)
    add_host_to_listbox (templist);

  hbox = gtk_hbox_new (TRUE, 12);

  gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("Add"));
#else
  tempwid = gtk_button_new_from_stock (GTK_STOCK_ADD);
#endif
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (add_proxy_host), NULL);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("Edit"));
#elif GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 5
  tempwid = gtk_button_new_with_mnemonic (_("_Edit"));
#else
  tempwid = gtk_button_new_from_stock (GTK_STOCK_EDIT);
#endif
  edit_button = tempwid;
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (add_proxy_host), (gpointer) 1);
  gtk_widget_show (tempwid);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("Delete"));
#else
  tempwid = gtk_button_new_from_stock (GTK_STOCK_DELETE);
#endif
  delete_button = tempwid;
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
		      GTK_SIGNAL_FUNC (delete_proxy_host), NULL);
  gtk_widget_show (tempwid);

  gtk_signal_connect (GTK_OBJECT (proxy_list), "select_row", 
                      GTK_SIGNAL_FUNC (buttons_toggle), (gpointer) 1);
  gtk_signal_connect (GTK_OBJECT (proxy_list), "unselect_row", 
                      GTK_SIGNAL_FUNC (buttons_toggle), NULL);
  buttons_toggle (NULL, 0, 0, 0, NULL);
}


static gftp_options_dialog_data *
_init_option_data (void)
{
  gftp_options_dialog_data * option_data;

  option_data = g_malloc0 (sizeof (*option_data));

  _setup_option (gftp_option_type_text, option_data, 
                 _print_option_type_text, _save_option_type_text, NULL);
  _setup_option (gftp_option_type_textcombo, option_data, 
                 _print_option_type_textcombo, _save_option_type_textcombo, 
                 NULL);
  _setup_option (gftp_option_type_textcomboedt, option_data, 
                 _print_option_type_textcomboedt, 
                 _save_option_type_textcomboedt,
                 NULL);
  _setup_option (gftp_option_type_hidetext, option_data, 
                 _print_option_type_hidetext, _save_option_type_text, NULL);
  _setup_option (gftp_option_type_int, option_data, 
                 _print_option_type_int, _save_option_type_int, NULL);
  _setup_option (gftp_option_type_checkbox, option_data, 
                 _print_option_type_checkbox, _save_option_type_checkbox, NULL);
  _setup_option (gftp_option_type_float, option_data, 
                 _print_option_type_float, _save_option_type_float, NULL);
  _setup_option (gftp_option_type_notebook, option_data, 
                 _print_option_type_notebook, NULL, NULL);

  return (option_data);
}


void
options_dialog (gpointer data)
{
  gftp_config_vars * cv;
  GList * templist;
  void *value;
  int i;
#if GTK_MAJOR_VERSION == 1
  GtkWidget * tempwid;
#endif

  gftp_option_data = _init_option_data ();

#if GTK_MAJOR_VERSION == 1
  gftp_option_data->dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (gftp_option_data->dialog), _("Options"));
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (gftp_option_data->dialog)->action_area), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->action_area), 15);
  gtk_box_set_homogeneous (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->action_area), TRUE);
#else
  gftp_option_data->dialog = gtk_dialog_new_with_buttons (_("Options"), NULL, 0,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (gftp_option_data->dialog), 5);
  gtk_dialog_set_has_separator (GTK_DIALOG (gftp_option_data->dialog), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (gftp_option_data->dialog), FALSE);
#endif
  gtk_window_set_wmclass (GTK_WINDOW(gftp_option_data->dialog),
                          "options", "gFTP");
  gtk_window_set_position (GTK_WINDOW (gftp_option_data->dialog),
                           GTK_WIN_POS_MOUSE);
#if GTK_MAJOR_VERSION == 1
  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG (gftp_option_data->dialog)->vbox), 10);
#endif
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->vbox), 2);
  gtk_widget_realize (gftp_option_data->dialog);

  if (gftp_icon != NULL)
    {
      gdk_window_set_icon (gftp_option_data->dialog->window, NULL,
                           gftp_icon->pixmap, gftp_icon->bitmap);
      gdk_window_set_icon_name (gftp_option_data->dialog->window, gftp_version);
    }

  gftp_option_data->notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->vbox), 
                      gftp_option_data->notebook, TRUE, TRUE, 0);
  gtk_widget_show (gftp_option_data->notebook);
#if GTK_MAJOR_VERSION > 1
  gtk_container_border_width (GTK_CONTAINER (gftp_option_data->notebook), 5);
#endif

  cv = gftp_options_list->data;
  gftp_option_data->last_option = cv[0].otype;
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

          if (*cv[i].key != '\0')
            gftp_lookup_global_option (cv[i].key, &value);
          else
            value = NULL;

          cv[i].user_data = gftp_option_types[cv[i].otype].ui_print_function (&cv[i], gftp_option_types[cv[i].otype].user_data, value);

          gftp_option_data->last_option = cv[i].otype;
        }
    }

  make_proxy_hosts_tab (gftp_option_data->notebook);

#if GTK_MAJOR_VERSION == 1
  tempwid = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (gftp_option_data->dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("  Cancel  "));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (clean_old_changes), NULL);
  gtk_signal_connect_object (GTK_OBJECT (tempwid), "clicked",
                             GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             GTK_OBJECT (gftp_option_data->dialog));
  gtk_widget_show (tempwid);

  tempwid = gtk_button_new_with_label (_("Apply"));
  GTK_WIDGET_SET_FLAGS (tempwid, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gftp_option_data->dialog)->action_area), 
                      tempwid, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (tempwid), "clicked",
                      GTK_SIGNAL_FUNC (apply_changes), NULL);
  gtk_widget_grab_default (tempwid);
  gtk_widget_show (tempwid);
#else
  g_signal_connect (GTK_OBJECT (gftp_option_data->dialog), "response",
                    G_CALLBACK (options_action), NULL);
#endif

  gtk_widget_show (gftp_option_data->dialog);
}


void
gftp_gtk_setup_bookmark_options (GtkWidget * notebook, gftp_bookmarks_var * bm)
{
  gftp_config_vars * cv;
  GList * templist;
  void *value;
  int i;

  gftp_option_data = _init_option_data ();
  gftp_option_data->bm = bm;
  gftp_option_data->notebook = notebook;

  cv = gftp_options_list->data;
  gftp_option_data->last_option = cv[0].otype;
  for (templist = gftp_options_list; 
       templist != NULL; 
       templist = templist->next)
    {
      cv = templist->data;

      for (i=0; cv[i].key != NULL; i++)
        {
          if (!(cv[i].flags & GFTP_CVARS_FLAGS_SHOW_BOOKMARK))
            continue;

          if (gftp_option_types[cv[i].otype].ui_print_function == NULL)
            continue;

          if (*cv[i].key != '\0')
            gftp_lookup_bookmark_option (bm, cv[i].key, &value);
          else
            value = NULL;

          cv[i].user_data = gftp_option_types[cv[i].otype].ui_print_function (&cv[i], gftp_option_data, value);

          gftp_option_data->last_option = cv[i].otype;
        }
    }
}


void
gftp_gtk_save_bookmark_options ()
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
          if (!(cv[i].flags & GFTP_CVARS_FLAGS_SHOW_BOOKMARK))
            continue;

          if (gftp_option_types[cv[i].otype].ui_save_function == NULL)
            continue;

          gftp_option_types[cv[i].otype].ui_save_function (&cv[i],
                                                           gftp_option_data);
        }
    }
}

