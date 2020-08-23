/*
 * Copyright (C) 2020
 * 
 * gtkcompat, GTK2+ compatibility layer
 * 
 * 2020-08-23
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

#ifndef __GTKCOMPAT_H
#define __GTKCOMPAT_H

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#if GTK_MAJOR_VERSION == 3
#include <gdk/gdkkeysyms-compat.h>
#endif


/* ================================================== */
/*                      GLIB                          */
/* ================================================== */

#ifndef __GLIB_COMPAT_H
#define __GLIB_COMPAT_H

#include <glib.h>

// GLIB < 2.58
#if ! GLIB_CHECK_VERSION (2, 58, 0)
#define G_SOURCE_FUNC(f) ((GSourceFunc) (void (*)(void)) (f))
#endif

// GLIB < 2.32
#if ! GLIB_CHECK_VERSION (2, 32, 0)
#define G_SOURCE_REMOVE   FALSE
#define G_SOURCE_CONTINUE TRUE
#define g_hash_table_add(ht, key) g_hash_table_replace(ht, key, key)
#define g_hash_table_contains(ht, key) g_hash_table_lookup_extended(ht, key, NULL, NULL)
#define GRecMutex              GStaticRecMutex
#define g_rec_mutex_init(x)    g_static_rec_mutex_init(x)
#define g_rec_mutex_lock(x)    g_static_rec_mutex_lock(x)
#define g_rec_mutex_trylock(x) g_static_rec_mutex_trylock(x)
#define g_rec_mutex_unlock(x)  g_static_rec_mutex_unlock(x)
#define g_rec_mutex_clear(x)   g_static_rec_mutex_free(x)
#define g_thread_new(name,func,data) g_thread_create(func,data,TRUE,NULL)
#define g_thread_try_new(name,func,data,error) g_thread_create(func,data,TRUE,error)
#endif

// GMutex vs GStaticMutex
#if GLIB_CHECK_VERSION (2, 32, 0)
// since version 2.32.0 GMutex can be statically allocated
// don't use WGMutex to replace GMutex * ... issues, errors.
#	define WGMutex GMutex
#	define Wg_mutex_init    g_mutex_init
#	define Wg_mutex_lock    g_mutex_lock
#	define Wg_mutex_trylock g_mutex_trylock
#	define Wg_mutex_unlock  g_mutex_unlock
#	define Wg_mutex_clear   g_mutex_clear
#else
#	define WGMutex GStaticMutex
#	define Wg_mutex_init    g_static_mutex_init
#	define Wg_mutex_lock    g_static_mutex_lock
#	define Wg_mutex_trylock g_static_mutex_trylock
#	define Wg_mutex_unlock  g_static_mutex_unlock
#	define Wg_mutex_clear   g_static_mutex_free
// sed -i 's%g_mutex_%Wg_mutex_%g' $(grep "g_mutex_" *.c *.h | cut -f 1 -d ":" | grep -v -E 'gtkcompat|glib-compat' | sort -u)
#endif

// GLIB < 2.28
#if ! GLIB_CHECK_VERSION (2, 28, 0)
#define g_list_free_full(list, free_func) {\
     g_list_foreach (list, (GFunc) free_func, NULL);\
     g_list_free (list);\
}
#endif

// GLIB < 2.22
#if ! GLIB_CHECK_VERSION (2, 22, 0)
#define g_mapped_file_unref(x) g_mapped_file_free(x)
#endif

#endif /* __GLIB_COMPAT_H */



/* ================================================== */
/*                       GTK                          */
/* ================================================== */

// GTK < 3.0
#if ! GTK_CHECK_VERSION (3, 0, 0)
GtkWidget *gtk_box_new (GtkOrientation orientation, gint spacing) ;
GtkWidget *gtk_button_box_new (GtkOrientation orientation);
GtkWidget *gtk_scale_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_scale_new_with_range (GtkOrientation orientation, gdouble min, gdouble max, gdouble step);
GtkWidget *gtk_separator_new (GtkOrientation orientation);
GtkWidget *gtk_scrollbar_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_paned_new (GtkOrientation orientation);
#define gtk_widget_get_allocated_height(widget) (GTK_WIDGET(widget)->allocation.height )
#define gtk_widget_get_allocated_width(widget)  (GTK_WIDGET(widget)->allocation.width  )
typedef enum /* GtkAlign */
{
  GTK_ALIGN_FILL,
  GTK_ALIGN_START,
  GTK_ALIGN_END,
  GTK_ALIGN_CENTER
} GtkAlign;
void gtk_widget_set_halign (GtkWidget *widget, GtkAlign align);
void gtk_widget_set_valign (GtkWidget *widget, GtkAlign align);
#endif


// GTK < 2.24
#if ! GTK_CHECK_VERSION (2, 24, 0)
typedef struct _GtkComboBox        GtkComboBoxText;
typedef struct _GtkComboBoxClass   GtkComboBoxTextClass;
typedef struct _GtkComboBoxPrivate GtkComboBoxTextPrivate;
#define GTK_TYPE_COMBO_BOX_TEXT            (gtk_combo_box_get_type ())
#define GTK_COMBO_BOX_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxText))
#define GTK_COMBO_BOX_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_COMBO_BOX, GtkComboBoxTextClass))
#define GTK_IS_COMBO_BOX_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_COMBO_BOX))
#define GTK_IS_COMBO_BOX_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_COMBO_BOX))
#define GTK_COMBO_BOX_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxTextClass))
#define gtk_combo_box_text_new() gtk_combo_box_new_text()
#define gtk_combo_box_text_new_with_entry()	gtk_combo_box_entry_new_text()
#define gtk_combo_box_text_append_text(combo,text) gtk_combo_box_append_text(combo,text)
#define gtk_combo_box_text_insert_text(combox,pos,text) gtk_combo_box_insert_text(combox,pos,text)
#define gtk_combo_box_text_prepend_text(combo,text) gtk_combo_box_prepend_text(combo,text)
#define gtk_combo_box_text_remove(combo,pos) gtk_combo_box_remove_text(combo,pos)
#define gtk_combo_box_text_get_active_text(combo) gtk_combo_box_get_active_text(combo)
#endif


// GTK < 2.22
#if ! GTK_CHECK_VERSION (2, 22, 0)
gboolean gtk_window_has_group (GtkWindow *window);
GtkWidget * gtk_window_group_get_current_grab (GtkWindowGroup *window_group);
#define gtk_font_selection_dialog_get_font_selection(fsd)(GTK_FONT_SELECTION_DIALOG(fsd)->fontsel)
#endif


// GTK < 2.20
#if ! GTK_CHECK_VERSION (2, 20, 0)
#define gtk_widget_get_mapped(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_MAPPED) != 0)
#define gtk_widget_get_realized(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_REALIZED) != 0)
#define gtk_window_get_window_type(window) (GTK_WINDOW(window)->type)
void gtk_widget_get_requisition (GtkWidget *widget, GtkRequisition *requisition);
void gtk_widget_set_mapped (GtkWidget *widget, gboolean mapped);
void gtk_widget_set_realized (GtkWidget *widget, gboolean realized);
#endif


// GTK < 2.18
#if ! GTK_CHECK_VERSION (2, 18, 0)
#define gtk_widget_get_state(wid) (GTK_WIDGET (wid)->state)
#define gtk_widget_is_toplevel(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_TOPLEVEL) != 0)
#define gtk_widget_get_has_window(wid) !((GTK_WIDGET_FLAGS (wid) & GTK_NO_WINDOW) != 0)
#define gtk_widget_get_visible(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_VISIBLE) != 0)
#define gtk_widget_is_drawable(wid)  (GTK_WIDGET_VISIBLE (wid) && GTK_WIDGET_MAPPED (wid))
#define gtk_widget_get_sensitive(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_SENSITIVE) != 0)
#define gtk_widget_get_can_focus(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_CAN_FOCUS) != 0)
#define gtk_widget_has_focus(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_FOCUS) != 0)
#define gtk_widget_get_can_default(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_CAN_DEFAULT) != 0)
#define gtk_widget_get_receives_default(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_RECEIVES_DEFAULT) != 0)
#define gtk_widget_has_default(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_DEFAULT) != 0)
#define gtk_widget_has_grab(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_HAS_GRAB) != 0)
#define gtk_widget_get_app_paintable(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_APP_PAINTABLE) != 0)
#define gtk_widget_get_double_buffered(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_DOUBLE_BUFFERED) != 0)
void gtk_widget_get_allocation (GtkWidget *widget, GtkAllocation *allocation);
void gtk_widget_set_allocation (GtkWidget *widget, const GtkAllocation *allocation);
void gtk_widget_set_can_default (GtkWidget *widget, gboolean can_default);
void gtk_widget_set_can_focus (GtkWidget *widget, gboolean can_focus);
void gtk_widget_set_has_window (GtkWidget *widget, gboolean has_window);
void gtk_widget_set_receives_default (GtkWidget *widget, gboolean receives_default);
void gtk_widget_set_visible (GtkWidget *widget, gboolean visible);
#endif


// GTK < 2.16
#if ! GTK_CHECK_VERSION (2, 16, 0)
#define gtk_status_icon_set_tooltip_text(icon,text) gtk_status_icon_set_tooltip(icon,text)
#define gtk_menu_item_get_label(i) (gtk_label_get_label (GTK_LABEL (GTK_BIN (i)->child)))
#define gtk_menu_item_get_use_underline(i) (gtk_label_get_use_underline (GTK_LABEL (GTK_BIN (i)->child)))
#endif


// GTK < 2.14
#if ! GTK_CHECK_VERSION (2, 14, 0)
#define gtk_dialog_get_action_area(dialog)    (GTK_DIALOG(dialog)->action_area)
#define gtk_dialog_get_content_area(dialog)   (GTK_DIALOG(dialog)->vbox)
#define gtk_widget_get_window(widget)         (GTK_WIDGET(widget)->window)
#define gtk_window_get_default_widget(window) (GTK_WINDOW(window)->default_widget)
#endif


// KEYS
#ifndef GDK_KEY_a
#	define GDK_KEY_Control_R GDK_Control_R
#	define GDK_KEY_Control_L GDK_Control_L
#	define GDK_KEY_Shift_R GDK_Shift_R
#	define GDK_KEY_Shift_L GDK_Shift_L
#	define GDK_KEY_Alt_R GDK_Alt_R
#	define GDK_KEY_Alt_L GDK_Alt_L
#	define GDK_KEY_Tab GDK_Tab
#	define GDK_KEY_Up GDK_Up
#	define GDK_KEY_space GDK_space
#	define GDK_KEY_Down GDK_Down
#	define GDK_KEY_Return GDK_Return
#	define GDK_KEY_exclam GDK_exclam
#	define GDK_KEY_BackSpace GDK_BackSpace
#	define GDK_KEY_Home GDK_Home
#	define GDK_KEY_End GDK_End
#	define GDK_KEY_Escape GDK_Escape
#	define GDK_KEY_G GDK_G
#	define GDK_KEY_g GDK_g
#	define GDK_KEY_E GDK_E
#	define GDK_KEY_e GDK_e
#	define GDK_KEY_n GDK_n
#	define GDK_KEY_w GDK_w
#	define GDK_KEY_R GDK_R
#	define GDK_KEY_r GDK_r
#	define GDK_KEY_S GDK_S
#	define GDK_KEY_s GDK_s
#endif


// PANGO
#ifndef PANGO_WEIGHT_MEDIUM
#define PANGO_WEIGHT_MEDIUM 500
#endif

#endif /* __GTKCOMPAT_H */
