/***********************************************************************************/
/*  menu-items.c - menu callbacks                                                  */
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

#include "gftp-gtk.h"

static gftp_options_dialog_data * gftp_option_data;

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
  GtkWidget * tempwid;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                    option_data->tbl_row_num, 2);

  tempwid = gtk_label_new (_(label));
  gtkcompat_widget_set_halign_left (tempwid);
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
      gtk_widget_set_tooltip_text(tempwid,  _(tiptxt));
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
  gtkcompat_widget_set_halign_left (tempwid);
  gtk_table_attach (GTK_TABLE (option_data->table), tempwid, 0, 1,
                    option_data->tbl_row_num - 1, 
                    option_data->tbl_row_num,
		    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_show (tempwid);

  //combo = gtk_combo_box_text_new_with_entry ();
  combo = gtk_combo_box_text_new ();
  gtk_table_attach_defaults (GTK_TABLE (option_data->table), combo, 1, 2,
                             option_data->tbl_row_num - 1, 
                             option_data->tbl_row_num);
  return (combo);
}


static void *
_print_option_type_textcombo (gftp_config_vars * cv, void *user_data, void *value)
{
  gftp_options_dialog_data * option_data;
  GtkWidget * combo;
  int selitem, i;
  char **clist;

  option_data = user_data;
  combo = _gen_combo_widget (option_data, cv->description);

  if (cv->listdata != NULL)
    {
      selitem = 0;
      clist = cv->listdata;
      for (i=0; clist[i] != NULL; i++)
        {
          if (value != NULL && strcasecmp ((char *) value, clist[i]) == 0)
            selitem = i;

          gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), clist[i]);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), selitem);
    }

  gtk_widget_show (combo);

  if (cv->comment != NULL)
    {
      gtk_widget_set_tooltip_text (combo, _(cv->comment));
    }

  return (combo);
}


static void
_save_option_type_textcombo (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  const char *tempstr;

  option_data = user_data;

  tempstr = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(cv->user_data));

  if (option_data->bm == NULL)
    gftp_set_global_option (cv->key, tempstr);
  else
    gftp_set_bookmark_option (option_data->bm, cv->key, tempstr);
}


static void
on_textcomboedt_change (GtkComboBox* cb, gpointer data)
{
  gftp_textcomboedt_widget_data * widdata;
  gftp_textcomboedt_data * tedata;
  char *insert_text;
  int num, isedit;
  GtkTextIter iter, iter2;
  GtkTextBuffer * textbuf;
  gint len;

  widdata = data;
  tedata = widdata->cv->listdata;

  num = gtk_combo_box_get_active( cb );
  isedit = tedata[num].flags & GFTP_TEXTCOMBOEDT_EDITABLE;
  gtk_text_view_set_editable (GTK_TEXT_VIEW (widdata->text), isedit);

  if (isedit)
    insert_text = widdata->custom_edit_value;
  else
    insert_text = tedata[num].text;

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widdata->text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  gtk_text_buffer_delete (textbuf, &iter, &iter2);

  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, len);
  gtk_text_buffer_insert (textbuf, &iter, insert_text, -1);
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
      tedata = cv->listdata;
      for (i=0; tedata[i].description != NULL; i++)
        {
          if (tedata[i].flags & GFTP_TEXTCOMBOEDT_EDITABLE)
            edititem = i;

          if (selitem == -1 &&
              strcasecmp (tempstr, tedata[i].text) == 0)
            selitem = i;

          gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), tedata[i].description);
        }

      if (selitem == -1 && edititem != -1)
        selitem = edititem;
    }

  if (selitem == -1)
    selitem = 0;

  option_data->tbl_row_num++;
  gtk_table_resize (GTK_TABLE (option_data->table), 
                               option_data->tbl_row_num, 2);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
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

  widdata = g_malloc0 (sizeof (*widdata));
  widdata->combo = combo;
  widdata->text = textwid;
  widdata->cv = cv;
  widdata->custom_edit_value = tempstr;

  g_signal_connect (combo, "changed", G_CALLBACK(on_textcomboedt_change), widdata);
  gtk_combo_box_set_active (GTK_COMBO_BOX(combo), selitem);
  gtk_widget_show (combo);

  if (cv->comment != NULL)
    {
       gtk_widget_set_tooltip_text (textwid, _(cv->comment));
       gtk_widget_set_tooltip_text (textwid, _(cv->comment)); 
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
  GtkTextBuffer * textbuf;
  GtkTextIter iter, iter2;
  size_t len;

  option_data = user_data;
  widdata = cv->user_data;

  textbuf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widdata->text));
  len = gtk_text_buffer_get_char_count (textbuf);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &iter2, len);
  newstr = gtk_text_buffer_get_text (textbuf, &iter, &iter2, 0);
  freeit = 1;

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
      gtk_widget_set_tooltip_text (tempwid, _(cv->comment));
    }

  return (tempwid);
}


static void
_save_option_type_checkbox (gftp_config_vars * cv, void *user_data)
{
  gftp_options_dialog_data * option_data;
  intptr_t val;

  option_data = user_data;

  val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cv->user_data));

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

  option_data->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (option_data->box), 10);
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
  gftpui_show_or_hide_command ();
}


static void
options_action (GtkWidget * widget, gint response, gpointer user_data)
{
  switch (response) {
     case GTK_RESPONSE_OK: apply_changes (widget, NULL); break;
  }
  clean_old_changes (widget, user_data);
  gtk_widget_destroy (widget);
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
  GtkWidget *main_vbox;
  gftp_config_vars * cv;
  GList * templist;
  void *value;
  int i;

  gftp_option_data = _init_option_data ();

  gftp_option_data->dialog = gtk_dialog_new_with_buttons (_("Options"), NULL, 0,
                                        "gtk-cancel",
                                        GTK_RESPONSE_CANCEL,
                                        "gtk-ok",
                                        GTK_RESPONSE_OK,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (gftp_option_data->dialog), 2);
  gtk_window_set_resizable (GTK_WINDOW (gftp_option_data->dialog), FALSE);
  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (gftp_option_data->dialog));

  gtk_window_set_role (GTK_WINDOW(gftp_option_data->dialog), "options");
  gtk_window_set_position (GTK_WINDOW (gftp_option_data->dialog),
                           GTK_WIN_POS_MOUSE);

  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);
  gtk_widget_realize (gftp_option_data->dialog);

  set_window_icon(GTK_WINDOW(gftp_option_data->dialog), NULL);

  gftp_option_data->notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), gftp_option_data->notebook, TRUE, TRUE, 0);
  gtk_widget_show (gftp_option_data->notebook);
  gtk_container_set_border_width (GTK_CONTAINER (gftp_option_data->notebook), 2);

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

  g_signal_connect (G_OBJECT (gftp_option_data->dialog), "response",
                    G_CALLBACK (options_action), NULL);

  gtk_widget_show_all (gftp_option_data->dialog);
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

