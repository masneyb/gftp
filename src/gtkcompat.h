/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

/** 2022-03-02 **/

/*
 * gtkcompat.h, GTK2+ compatibility layer
 * 
 * This lib makes it easier to support older GTK versions
 * while still avoiding deprecated functions as much as possible.
 * 
 * The older the GTK version, the more compatible functions are "defined"
 * so it's not wise to use the compiled binary in newer distros or something.
 * 
 * Apps should support gtk2 >= 2.14 / gtk3 >= 3.14
 * 
 */

/* 
special defines:
	gtkcompat_widget_set_halign_left   (w)
	gtkcompat_widget_set_halign_center (w)
	gtkcompat_widget_set_halign_right  (w)
*/

/*
GTKCOMPAT_DRAW_SIGNAL (gtk3="draw", gtk2="expose_event")
---------------------
 g_signal_connect (w, GTKCOMPAT_DRAW_SIGNAL, G_CALLBACK (w_draw_cb), NULL);
 gboolean w_draw_cb (GtkWidget *w, gpointer compat, gpointer user_data)
 {
 #if GTK_CHECK_VERSION (3, 0, 0)
    cairo_t * cr = (cairo_t *) compat;
 #else // gtk2
    //GdkEventExpose * event = (GdkEventExpose *) compat;
    cairo_t * cr = gdk_cairo_create (gtk_widget_get_window (w));
 #endif
 #if GTK_MAJOR_VERSION == 2
    cairo_destroy (cr);
 #endif
 }
*/

#ifndef __GTKCOMPAT_H
#define __GTKCOMPAT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <gtk/gtk.h>

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


// GLIB < 2.40
#if ! GLIB_CHECK_VERSION (2, 40, 0)
#define g_key_file_save_to_file(kfile,filename,error) { \
   char * xdatax = g_key_file_to_data (kfile, NULL, error); \
   g_file_set_contents (filename, xdatax, -1, error); \
   g_free (xdatax); \
}
#endif


// GLIB < 2.32
#if ! GLIB_CHECK_VERSION (2, 32, 0)
#define G_SOURCE_REMOVE   FALSE
#define G_SOURCE_CONTINUE TRUE
#define GRecMutex              GStaticRecMutex
#define g_rec_mutex_init(x)    g_static_rec_mutex_init(x)
#define g_rec_mutex_lock(x)    g_static_rec_mutex_lock(x)
#define g_rec_mutex_trylock(x) g_static_rec_mutex_trylock(x)
#define g_rec_mutex_unlock(x)  g_static_rec_mutex_unlock(x)
#define g_rec_mutex_clear(x)   g_static_rec_mutex_free(x)
#define g_thread_new(name,func,data) g_thread_create(func,data,TRUE,NULL)
#define g_thread_try_new(name,func,data,error) g_thread_create(func,data,TRUE,error)
#define g_hash_table_add(ht,key)      g_hash_table_replace(ht,key,key)
#define g_hash_table_contains(ht,key) g_hash_table_lookup_extended(ht,key,NULL,NULL)
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


// GLIB < 2.30
#if ! GLIB_CHECK_VERSION (2, 30, 0)
#define g_format_size g_format_size_for_display
#endif


// GLIB < 2.28
#if ! GLIB_CHECK_VERSION (2, 28, 0)
#define g_list_free_full(list,free_func) {\
     g_list_foreach (list, (GFunc) free_func, NULL);\
     g_list_free (list);\
}
#endif


// GLIB < 2.22
#if ! GLIB_CHECK_VERSION (2, 22, 0)
#define g_mapped_file_unref(x) g_mapped_file_free(x)
#endif

// GLIB < 2.20
#if ! GLIB_CHECK_VERSION (2, 20, 0)
#define g_app_info_get_commandline(app) g_app_info_get_executable(app)
#endif

/* glib 2.18+ tested */

#endif /* __GLIB_COMPAT_H */


/* ================================================== */
/*                     GDK KEYS                       */
/* ================================================== */

#include <gdk/gdkkeysyms.h>
#if !defined(GDK_KEY_a) // GTK_MAJOR_VERSION >= 3
#define GDK_KEY(symbol) GDK_##symbol
#else
#define GDK_KEY(symbol) GDK_KEY_##symbol
#endif


/* ================================================== */
/*                       GTK 3                        */
/* ================================================== */

// GTK >= 3.0 -- applies to GTK3, GTK4...
#if GTK_MAJOR_VERSION >= 3
#define GTKCOMPAT_DRAW_SIGNAL "draw"
#define gtkcompat_widget_set_halign_left(w)   gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START)
#define gtkcompat_widget_set_halign_center(w) gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_CENTER)
#define gtkcompat_widget_set_halign_right(w)  gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_END)
// using GtkTable format, translate to to GtkGrid
#define gtkcompat_grid_attach(table,child,left,right,top,bottom) \
    gtk_grid_attach((table),(child), (left), (top), (right)-(left), (bottom)-(top))
#define gtkcompat_grid_new(rows,cols) (gtk_grid_new())
#endif

/* GTK < 3.12
#if ! GTK_CHECK_VERSION (3, 12, 0)
#define gtk_application_set_accels_for_action(app,name,accels) \
          gtk_application_add_accelerator(app,accels[0],name,NULL)
#define gtk_widget_set_margin_start(widget,margin) gtk_widget_set_margin_left(widget,margin)
#define gtk_widget_set_margin_end(widget,margin)   gtk_widget_set_margin_right(widget,margin)
#endif */


/* ================================================== */
/*                       GTK 2                        */
/* ================================================== */

#if GTK_MAJOR_VERSION == 2

// define some GTK3.14+ functions
// GTK < 3.10
// gdk_window_create_similar_image_surface() was removed in gtk4
//                  only use gdk_window_create_similar_surface()
// GTK < 3.8
#define gtk_widget_set_opacity(w,o) gtk_window_set_opacity(GTK_WINDOW(w),o)
#define gtk_widget_get_opacity(w)  (gtk_window_get_opacity(GTK_WINDOW(w))
// GTK < 3.4
#define gtk_application_window_new(app) gtk_window_new(GTK_WINDOW_TOPLEVEL)

/*** GTK 3.0 ***/
#define GTKCOMPAT_DRAW_SIGNAL "expose_event"
#define gtk_box_new(ori,spacing) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hbox_new(FALSE,spacing) \
                                       : gtk_vbox_new(FALSE,spacing))
#define gtk_button_box_new(ori) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hbutton_box_new() \
                                       : gtk_vbutton_box_new())
#define gtk_scale_new(ori,adjustment) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hscale_new(adjustment) \
                                       : gtk_vscale_new(adjustment))
#define gtk_scale_new_with_range(ori,min,max,step) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hscale_new_with_range(min,max,step) \
                                       : gtk_vscale_new_with_range(min,max,step))
#define gtk_separator_new(ori) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hseparator_new() \
                                       : gtk_vseparator_new())
#define gtk_scrollbar_new(ori,adjustment) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hscrollbar_new(adjustment) \
                                       : gtk_vscrollbar_new(adjustment))
#define gtk_paned_new(ori) \
  ((ori == GTK_ORIENTATION_HORIZONTAL) ? gtk_hpaned_new() \
                                       : gtk_vpaned_new())
#define gtk_widget_get_allocated_height(widget) (GTK_WIDGET(widget)->allocation.height )
#define gtk_widget_get_allocated_width(widget)  (GTK_WIDGET(widget)->allocation.width  )
#define gtk_combo_box_text_remove_all(cmb) { \
   gtk_list_store_clear (GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (cmb)))); \
}
#define gtk_tree_model_iter_previous(model,iter) ({ \
   GtkTreePath * xpathx = gtk_tree_model_get_path (model, iter); \
   gboolean xvalidx = gtk_tree_path_prev (xpathx); \
   if (xvalidx) gtk_tree_model_get_iter (model, iter, xpathx); \
   gtk_tree_path_free (xpathx); \
   xvalidx; \
})
#define gtk_progress_bar_set_show_text(pb,show)
#define gtk_widget_override_font(w,f) gtk_widget_modify_font(w,f)
#define gtkcompat_widget_set_halign_left(w)   gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.5)
#define gtkcompat_widget_set_halign_center(w) gtk_misc_set_alignment(GTK_MISC(w), 0.5, 0.5)
#define gtkcompat_widget_set_halign_right(w)  gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5)
//-
#define GTK_GRID GTK_TABLE
#define GtkGrid  GtkTable
#define gtk_grid_attach(grid,child,left,top,width,height) \
    gtk_table_attach_defaults((grid),(child), (left), (left)+(width), (top), (top)+(height))
#define gtkcompat_grid_new(rows,cols) (gtk_table_new((rows),(cols),FALSE))
#define gtkcompat_grid_attach       gtk_table_attach_defaults
#define gtk_grid_set_column_spacing gtk_table_set_col_spacings
#define gtk_grid_set_row_spacing    gtk_table_set_row_spacings
#define gtk_grid_get_column_spacing gtk_table_get_default_col_spacing
#define gtk_grid_get_row_spacing    gtk_table_get_default_row_spacing
#define gtk_grid_set_row_homogeneous    gtk_table_set_homogeneous
#define gtk_grid_set_column_homogeneous gtk_table_set_homogeneous
#define gtk_grid_get_row_homogeneous    gtk_table_get_homogeneous
#define gtk_grid_get_column_homogeneous gtk_table_gset_homogeneous
//-
typedef enum /* GtkAlign */
{
  GTK_ALIGN_FILL,
  GTK_ALIGN_START,
  GTK_ALIGN_END,
  GTK_ALIGN_CENTER
} GtkAlign;
/* GtkApplication */
#define GtkApplication void
#define g_application_quit(app) gtk_main_quit()
#undef G_APPLICATION
#define G_APPLICATION(app) ((void *) (app))
//-
#define gdk_error_trap_pop_ignored gdk_error_trap_pop


// GTK < 2.24
#if GTK_MINOR_VERSION < 24
typedef struct _GtkComboBox        GtkComboBoxText;
typedef struct _GtkComboBoxClass   GtkComboBoxTextClass;
typedef struct _GtkComboBoxPrivate GtkComboBoxTextPrivate;
#define GTK_TYPE_COMBO_BOX_TEXT            (gtk_combo_box_get_type ())
#define GTK_COMBO_BOX_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxText))
#define GTK_COMBO_BOX_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_COMBO_BOX, GtkComboBoxTextClass))
#define GTK_IS_COMBO_BOX_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_COMBO_BOX))
#define GTK_IS_COMBO_BOX_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_COMBO_BOX))
#define GTK_COMBO_BOX_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_COMBO_BOX, GtkComboBoxTextClass))
#define gtk_combo_box_text_new()            gtk_combo_box_new_text()
#define gtk_combo_box_text_new_with_entry()	gtk_combo_box_entry_new_text()
#define gtk_combo_box_text_append_text(combo,text)     gtk_combo_box_append_text(combo,text)
#define gtk_combo_box_text_insert_text(combo,pos,text) gtk_combo_box_insert_text(combo,pos,text)
#define gtk_combo_box_text_prepend_text(combo,text)    gtk_combo_box_prepend_text(combo,text)
#define gtk_combo_box_text_remove(combo,pos)           gtk_combo_box_remove_text(combo,pos)
#define gtk_combo_box_text_get_active_text(combo)      (gtk_combo_box_get_active_text(combo))
#define gtk_combo_box_get_has_entry(combo) (0)
#define gtk_combo_box_set_entry_text_column(combo,cl)
#define gtk_combo_box_get_entry_text_column(combo) (0)
#define gtk_range_get_round_digits(range) (GTK_RANGE(range)->round_digits)
//#define gdk_window_get_visual(w)  (gdk_drawable_get_visual(GDK_DRAWABLE(w)))
#define gdk_window_get_screen(w)  (gdk_drawable_get_screen(GDK_DRAWABLE(w)))
#define gdk_window_get_display(w) (gdk_drawable_get_display(GDK_DRAWABLE(w)))
#define gtk_notebook_get_group_name gtk_notebook_get_group
#define gtk_notebook_set_group_name gtk_notebook_set_group
#endif


// GTK < 2.22
#if GTK_MINOR_VERSION < 22
#define gtk_accessible_get_widget(a) ((a)->widget)
#define gtk_window_has_group(w) (GTK_WINDOW(w)->group != NULL)
#define gtk_window_group_get_current_grab(wg) \
  ((GTK_WINDOW_GROUP(wg)->grabs) ? GTK_WIDGET(GTK_WINDOW_GROUP(wg)->grabs->data) : NULL)
#define gtk_font_selection_dialog_get_font_selection(fsd)(GTK_FONT_SELECTION_DIALOG(fsd)->fontsel)
#define gtk_notebook_get_tab_hborder(n) (GTK_NOTEBOOK(n)->tab_hborder)
#define gtk_notebook_get_tab_vborder(n) (GTK_NOTEBOOK(n)->tab_vborder)
#define gtk_button_get_event_window(button) (GTK_BUTTON(button)->event_window)
#define gdk_visual_get_visual_type(visual) (GDK_VISUAL(visual)->type)
#define gdk_visual_get_depth(visual) (GDK_VISUAL(visual)->depth)
#define gdk_visual_get_byte_order(visual) (GDK_VISUAL(visual)->byte_order)
#define gdk_visual_get_colormap_size(visual) (GDK_VISUAL(visual)->colormap_size)
#define gdk_visual_get_bits_per_rgb(visual) (GDK_VISUAL(visual)->bits_per_rgb)
#define gtk_icon_view_get_item_orientation gtk_icon_view_get_orientation
#define gtk_icon_view_set_item_orientation gtk_icon_view_set_orientation
#define gdk_window_create_similar_surface(gdksurf,content,width,height) ({ \
   cairo_t * wcr = gdk_cairo_create (gdksurf); \
   cairo_surface_t * window_surface = cairo_get_target (wcr); \
   cairo_surface_t * out_s = cairo_surface_create_similar (window_surface, content, width, height); \
   cairo_destroy (wcr); \
   out_s; \
})
#endif


// GTK < 2.20
#if GTK_MINOR_VERSION < 20
#define gtk_widget_get_mapped(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_MAPPED) != 0)
#define gtk_widget_get_realized(wid) ((GTK_WIDGET_FLAGS (wid) & GTK_REALIZED) != 0)
#define gtk_window_get_window_type(window) (GTK_WINDOW(window)->type)
#define gtk_widget_get_requisition(w,r) (*(r) = GTK_WIDGET(w)->requisition)
#define gtk_widget_set_mapped(w,yes) { \
   if (yes) GTK_WIDGET_SET_FLAGS(w,GTK_MAPPED); \
   else GTK_WIDGET_UNSET_FLAGS(w,GTK_MAPPED); \
}
#define gtk_widget_set_realized(w,yes) { \
   if (yes) GTK_WIDGET_SET_FLAGS(w,GTK_REALIZED); \
   else GTK_WIDGET_UNSET_FLAGS(w,GTK_REALIZED); \
}
#define gtk_range_get_slider_size_fixed(range) (GTK_RANGE(range)->slider_size_fixed)
#define gtk_range_get_min_slider_size(range) (GTK_RANGE(range)->min_slider_size)
#define gtk_entry_get_text_window(entry) (GTK_ENTRY(entry)->text_area)
#endif


// GTK < 2.18
#if GTK_MINOR_VERSION < 18
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
#define gtk_widget_set_allocation(w,alloc) (GTK_WIDGET(w)->allocation = *(alloc))
#define gtk_widget_get_allocation(w,alloc) (*(alloc) = GTK_WIDGET(w)->allocation)
#define gtk_widget_set_can_default(w,yes) { \
   if (yes) GTK_WIDGET_SET_FLAGS(w,GTK_CAN_DEFAULT); \
   else GTK_WIDGET_UNSET_FLAGS(w,GTK_CAN_DEFAULT); \
}
#define gtk_widget_set_can_focus(w,yes) { \
   if (yes) GTK_WIDGET_SET_FLAGS(w,GTK_CAN_FOCUS); \
   else GTK_WIDGET_UNSET_FLAGS(w,GTK_CAN_FOCUS); \
}
#define gtk_widget_set_has_window(w,yes) { \
   if (yes) GTK_WIDGET_UNSET_FLAGS(w,GTK_NO_WINDOW); \
   else GTK_WIDGET_SET_FLAGS(w,GTK_NO_WINDOW); \
}
#define gtk_widget_set_visible(w,yes) { \
   if (yes) gtk_widget_show(w); \
   else gtk_widget_hide(w); \
}
#define gtk_range_get_flippable(range) (GTK_RANGE(range)->flippable)
#define gdk_window_is_destroyed(w) (GDK_WINDOW_DESTROYED (GDK_WINDOW(w)))
#endif


// GTK < 2.16
#if GTK_MINOR_VERSION < 16
#define gtk_menu_item_get_label(i) (gtk_label_get_label (GTK_LABEL (GTK_BIN (i)->child)))
#define gtk_menu_item_set_label(i,label) gtk_label_set_label(GTK_LABEL(GTK_BIN(i)->child), (label) ? label : "")
#define gtk_menu_item_get_use_underline(i) (gtk_label_get_use_underline (GTK_LABEL (GTK_BIN (i)->child)))
#define gtk_status_icon_set_tooltip_text gtk_status_icon_set_tooltip
#endif


// GTK < 2.14
#if GTK_MINOR_VERSION < 14
#define gtk_dialog_get_action_area(dialog)    (GTK_DIALOG(dialog)->action_area)
#define gtk_dialog_get_content_area(dialog)   (GTK_DIALOG(dialog)->vbox)
#define gtk_widget_get_window(widget)         (GTK_WIDGET(widget)->window)
#define gtk_window_get_default_widget(window) (GTK_WINDOW(window)->default_widget)
#define gtk_menu_item_get_accel_path(i)       (GTK_MENU_ITEM(i)->accel_path)
#define gtk_menu_get_accel_path(menu)         (GTK_MENU(menu)->accel_path)
#define gtk_message_dialog_get_image(m)       (GTK_MESSAGE_DIALOG(m)->image)
#define gtk_entry_get_overwrite_mode(e)       (GTK_ENTRY(e)->overwrite_mode)
// ---
#define gtk_adjustment_set_page_increment(a,val) ((a)->page_increment = (val))
#define gtk_adjustment_get_page_size(a)       ((a)->page_size)
#define gtk_adjustment_get_lower(a)           ((a)->lower)
#define gtk_adjustment_get_upper(a)           ((a)->upper) // GTK_ADJUSTMENT
#define gtk_selection_data_get_length(data)   ((data)->length)
#endif


#endif /* ------- GTK2 ------- */

// ===================================================

// CAIRO < 1.10
#if CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 10, 0)
#define cairo_region_t         GdkRegion
#define cairo_rectangle_int_t  GdkRectangle
#define cairo_region_create    gdk_region_new
#define cairo_region_copy      gdk_region_copy
#define cairo_region_destroy   gdk_region_destroy
#define cairo_region_create_rectangle gdk_region_rectangle
#define cairo_region_get_extents gdk_region_get_clipbox
#define cairo_region_is_empty  gdk_region_empty
#define cairo_region_equal     gdk_region_empty
#define cairo_region_contains_point gdk_region_point_in
#define cairo_region_contains_rectangle gdk_region_rect_in
#define cairo_region_translate gdk_region_rect_in
#define cairo_region_union_rectangle gdk_region_union_with_rect
#define cairo_region_intersect gdk_region_intersect
#define cairo_region_union     gdk_region_union
#define cairo_region_subtract  gdk_region_subtract
#define cairo_region_xor       gdk_region_xor
//#define cairo_region_num_rectangles / cairo_region_get_rectangle  gdk_region_get_rectangles
#endif


// PANGO
#ifndef PANGO_WEIGHT_MEDIUM
#define PANGO_WEIGHT_MEDIUM 500
#endif


#ifdef __cplusplus
}
#endif

#endif /* __GTKCOMPAT_H */
