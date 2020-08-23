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

#include "gtkcompat.h"

/* ================================================== */
/*                   GTK < 3.0                        */
/* ================================================== */

#if ! GTK_CHECK_VERSION (3, 0, 0)

GtkWidget *
gtk_box_new (GtkOrientation orientation, gint spacing)
{
	// the HOMOGENEOUS property defaults to FALSE
	// add this to make it TRUE
	//   gtk_box_set_homogeneous (GTK_BOX(hbox), TRUE);
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hbox_new (FALSE, spacing);
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vbox_new (FALSE, spacing);
	}
	return NULL;
}

GtkWidget *
gtk_button_box_new (GtkOrientation orientation)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hbutton_box_new ();
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vbutton_box_new ();
	}
	return NULL;
}

GtkWidget *
gtk_scale_new (GtkOrientation orientation, GtkAdjustment *adjustment)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hscale_new (adjustment);
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vscale_new (adjustment);
	}
	return NULL;
}

GtkWidget *
gtk_scale_new_with_range (GtkOrientation orientation,
                          gdouble min,
                          gdouble max,
                          gdouble step)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hscale_new_with_range (min, max, step);
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vscale_new_with_range (min, max, step);
	}
	return NULL;
}

GtkWidget *
gtk_separator_new (GtkOrientation orientation)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hseparator_new ();
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vseparator_new ();
	}
	return NULL;
}

GtkWidget *
gtk_scrollbar_new (GtkOrientation orientation, GtkAdjustment *adjustment)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hscrollbar_new (adjustment);
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vscrollbar_new (adjustment);
	}
	return NULL;
}

GtkWidget *
gtk_paned_new (GtkOrientation orientation)
{
	if (orientation == GTK_ORIENTATION_HORIZONTAL) {
		return gtk_hpaned_new ();
	}
	if (orientation == GTK_ORIENTATION_VERTICAL) {
		return gtk_vpaned_new ();
	}
	return NULL;
}

void
gtk_widget_set_halign (GtkWidget *widget, GtkAlign align)
{
	GtkMisc * misc = GTK_MISC (widget);
	gfloat xalign = 0.0;
	switch (align)
	{
		case GTK_ALIGN_FILL:
		case GTK_ALIGN_START:  xalign = 0.0; break;
		case GTK_ALIGN_CENTER: xalign = 0.5; break;
		case GTK_ALIGN_END:    xalign = 1.0; break;
	}
	if (xalign != misc->xalign)
	{
		g_object_freeze_notify (G_OBJECT (misc));
		g_object_notify (G_OBJECT (misc), "xalign");
		misc->xalign = xalign;
		if (gtk_widget_is_drawable (widget))
			gtk_widget_queue_draw (widget);
		g_object_thaw_notify (G_OBJECT (misc));
	}
}

void
gtk_widget_set_valign (GtkWidget *widget, GtkAlign align)
{
	GtkMisc * misc = GTK_MISC (widget);
	gfloat yalign = 0.0;
	switch (align)
	{
		case GTK_ALIGN_FILL:
		case GTK_ALIGN_START:  yalign = 0.0; break;
		case GTK_ALIGN_CENTER: yalign = 0.5; break;
		case GTK_ALIGN_END:    yalign = 1.0; break;
	}
	if (yalign != misc->yalign)
	{
		g_object_freeze_notify (G_OBJECT (misc));
		g_object_notify (G_OBJECT (misc), "yalign");
		misc->yalign = yalign;
		if (gtk_widget_is_drawable (widget))
			gtk_widget_queue_draw (widget);
		g_object_thaw_notify (G_OBJECT (misc));
	}
}

#endif


/* ================================================== */
/*                   GTK < 2.22                       */
/* ================================================== */

#if ! GTK_CHECK_VERSION (2, 22, 0)

gboolean gtk_window_has_group (GtkWindow *window)
{
	return window->group != NULL;
}

GtkWidget * gtk_window_group_get_current_grab (GtkWindowGroup *window_group)
{
	if (window_group->grabs) {
		return GTK_WIDGET (window_group->grabs->data);
	}
	return NULL;
}

#endif


/* ================================================== */
/*                   GTK < 2.20                       */
/* ================================================== */

#if ! GTK_CHECK_VERSION (2, 20, 0)

void
gtk_widget_get_requisition (GtkWidget *widget, GtkRequisition *requisition)
{
	*requisition = widget->requisition;
}

void
gtk_widget_set_mapped (GtkWidget *widget, gboolean mapped)
{
	if (mapped)
		GTK_OBJECT_FLAGS (widget) |= GTK_MAPPED;
	else
		GTK_OBJECT_FLAGS (widget) &= ~(GTK_MAPPED);
}

void
gtk_widget_set_realized (GtkWidget *widget, gboolean realized)
{
	if (realized)
		GTK_OBJECT_FLAGS (widget) |= GTK_REALIZED;
	else
		GTK_OBJECT_FLAGS (widget) &= ~(GTK_REALIZED);
}

#endif


/* ================================================== */
/*                   GTK < 2.18                       */
/* ================================================== */

#if ! GTK_CHECK_VERSION (2, 18, 0)

void gtk_widget_get_allocation (GtkWidget *widget, GtkAllocation *allocation)
{
	*allocation = widget->allocation;
}

void gtk_widget_set_allocation (GtkWidget *widget, const GtkAllocation *allocation)
{
	widget->allocation = *allocation;
}

void gtk_widget_set_can_default (GtkWidget *widget, gboolean can_default)
{
	if (can_default != gtk_widget_get_can_default (widget))
	{
		if (can_default)
			GTK_OBJECT_FLAGS (widget) |= GTK_CAN_DEFAULT;
		else
			GTK_OBJECT_FLAGS (widget) &= ~(GTK_CAN_DEFAULT);
	}
}

void gtk_widget_set_can_focus (GtkWidget *widget, gboolean can_focus)
{
	if (can_focus != gtk_widget_get_can_focus (widget))
	{
		if (can_focus)
			GTK_OBJECT_FLAGS (widget) |= GTK_CAN_FOCUS;
		else
			GTK_OBJECT_FLAGS (widget) &= ~(GTK_CAN_FOCUS);
	}
}

void gtk_widget_set_has_window (GtkWidget *widget, gboolean has_window)
{
	if (has_window)
		GTK_OBJECT_FLAGS (widget) &= ~(GTK_NO_WINDOW);
	else
		GTK_OBJECT_FLAGS (widget) |= GTK_NO_WINDOW;
}

void gtk_widget_set_visible (GtkWidget *widget, gboolean visible)
{
	if (visible)
		gtk_widget_show (widget);
	else
		gtk_widget_hide (widget);
}
#endif

