/*
 * GTK COMPAT
 */

#include "gtkcompat.h"

#if !GTK_CHECK_VERSION (3, 0, 0)

// GTK 2.24 and lower

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

#endif

