/*
 * Z GTK COMPAT
 */

#ifndef __GTKCOMPAT_H
#define __GTKCOMPAT_H

#include <gtk/gtk.h>
#include <stdio.h>

#if !GTK_CHECK_VERSION (3, 0, 0)
GtkWidget *gtk_box_new (GtkOrientation orientation, gint spacing) ;
GtkWidget *gtk_button_box_new (GtkOrientation orientation);
GtkWidget *gtk_scale_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_scale_new_with_range (GtkOrientation orientation, gdouble min, gdouble max, gdouble step);
GtkWidget *gtk_separator_new (GtkOrientation orientation);
GtkWidget *gtk_scrollbar_new (GtkOrientation orientation, GtkAdjustment *adjustment);
GtkWidget *gtk_paned_new (GtkOrientation orientation);
#endif

#endif
