/***********************************************************************************/
/*  chmod_dialog.c - the chmod dialog box                                          */
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

static GtkWidget *suid, *sgid, *sticky, *ur, *uw, *ux, *gr, *gw, *gx, *or, *ow, *ox;
static mode_t mode; 


static int
do_chmod_thread (gftpui_callback_data * cdata)
{
  gftp_window_data * wdata;
  GList * templist, *igl;
  gftp_file * tempfle;
  int error;

  wdata = cdata->uidata;
  error = 0;

  templist = (GList *) listbox_get_selected_files (wdata, 0);
  for (igl = templist; igl != NULL; igl = igl->next)
  {
     tempfle = (gftp_file *) igl->data;
     if (gftp_chmod (wdata->request, tempfle->file, mode) != 0) {
        error = 1;
     }
     if (!GFTP_IS_CONNECTED (wdata->request)) {
        break;
     }
  }
  g_list_free (templist);

  return (error);
}

#define tb_get_active(w) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
#define tb_set_active(w,active) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), active)

static void
dochmod (gftp_window_data * wdata)
{
  gftpui_callback_data * cdata;

  mode = 0;
  if (tb_get_active (suid))   mode |= S_ISUID;
  if (tb_get_active (sgid))   mode |= S_ISGID;
  if (tb_get_active (sticky)) mode |= S_ISVTX;

  if (tb_get_active (ur))  mode |= S_IRUSR;
  if (tb_get_active (uw))  mode |= S_IWUSR;
  if (tb_get_active (ux))  mode |= S_IXUSR;

  if (tb_get_active (gr))  mode |= S_IRGRP;
  if (tb_get_active (gw))  mode |= S_IWGRP;
  if (tb_get_active (gx))  mode |= S_IXGRP;

  if (tb_get_active (or))  mode |= S_IROTH;
  if (tb_get_active (ow))  mode |= S_IWOTH;
  if (tb_get_active (ox))  mode |= S_IXOTH;

  if (check_reconnect (wdata) < 0)
    return;

  cdata = g_malloc0 (sizeof (*cdata));
  cdata->request = wdata->request;
  cdata->uidata = wdata;
  cdata->run_function = do_chmod_thread;

  gftpui_common_run_callback_function (cdata);

  g_free (cdata);
}

static void
on_gtk_dialog_response_chmod (GtkDialog *dialog, gint response, gpointer wdata)
{ /* chmod action */
   switch (response) {
      case GTK_RESPONSE_OK: dochmod (wdata); break;
   }
   gtk_widget_destroy (GTK_WIDGET (dialog));
}


void
chmod_dialog (gpointer data)
{
  GtkWidget *label, *dialog, *hbox, *main_vbox;
  GtkWidget * ButtonOK, * ButtonCancel, * IconOK, * IconCancel;
  GtkWidget * FrameItem[4][3]; // 4 frames with 3 items (CheckBoxes) each
  GtkWidget * frameX, * FrameVbox;
  int i, j;
  gftp_window_data * wdata;
  gftp_file * tempfle;

  wdata = data;
  if (!check_status (_("Chmod"), wdata, gftpui_common_use_threads (wdata->request), 0, 1, wdata->request->chmod != NULL))
    return;

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Chmod"));
  gtk_window_set_role (GTK_WINDOW (dialog), "Chmod");
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (main_window));
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
  set_window_icon (GTK_WINDOW (dialog), NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_widget_realize (dialog);

  // buttons
  ButtonCancel = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
  IconCancel   = gtk_image_new_from_icon_name ("gtk-cancel", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (ButtonCancel), IconCancel);
  ButtonOK     = gtk_dialog_add_button (GTK_DIALOG (dialog), _("_OK"),     GTK_RESPONSE_OK);
  IconOK       = gtk_image_new_from_icon_name ("gtk-ok", GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (ButtonOK), IconOK);

  // vbox
  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_set_spacing (GTK_BOX (main_vbox), 5);

  label = gtk_label_new (_("You can now adjust the attributes of your file(s)\nNote: Not all ftp servers support the chmod feature"));
  gtk_box_pack_start (GTK_BOX (main_vbox), label, TRUE, FALSE, 0);

  // vbox -> hbox
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

  // vbox -> hbox -> frames
  const char * FrameLabel[4] = { _("Special"), _("User"), _("Group"), _("Other"), };
  const char *  ItemLabel[4][3] = {
     { _("SUID"), _("SGID"),  _("Sticky"),  }, /* frame 0: special */
     { _("Read"), _("Write"), _("Execute"), }, /* frame 1: user    */
     { _("Read"), _("Write"), _("Execute"), }, /* frame 2: group   */
     { _("Read"), _("Write"), _("Execute"), }, /* frame 3: other   */
  };

  for (i = 0; i < 4; i++)
  {
     frameX = gtk_frame_new (FrameLabel[i]);
     gtk_box_pack_start (GTK_BOX (hbox), frameX, FALSE, FALSE, 0);

     FrameVbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
     gtk_box_set_homogeneous (GTK_BOX (FrameVbox), TRUE);
     gtk_container_add (GTK_CONTAINER (frameX), FrameVbox);
     gtk_container_set_border_width (GTK_CONTAINER (FrameVbox), 7); /* padding */

     for (j = 0; j < 3; j++)
     {
        FrameItem[i][j] = gtk_check_button_new_with_label (ItemLabel[i][j]);
        gtk_box_pack_start (GTK_BOX (FrameVbox), FrameItem[i][j], FALSE, FALSE, 0);
     }
  }

  /* frame 0: special */
  suid   = FrameItem[0][0];
  sgid   = FrameItem[0][1];
  sticky = FrameItem[0][2];
  /* frame 1: user    */
  ur     = FrameItem[1][0];
  uw     = FrameItem[1][1];
  ux     = FrameItem[1][2];
  /* frame 2: group   */
  gr     = FrameItem[2][0];
  gw     = FrameItem[2][1];
  gx     = FrameItem[2][2];
  /* frame 3: other   */
  or     = FrameItem[3][0];
  ow     = FrameItem[3][1];
  ox     = FrameItem[3][2];

  // --
  g_signal_connect (G_OBJECT (dialog), // GtkDialog
                    "response",        // signal
                    G_CALLBACK (on_gtk_dialog_response_chmod),
                    wdata);

  if (listbox_num_selected (wdata) == 1)
    {
      tempfle = (gftp_file *) listbox_get_selected_files (wdata, 1);

      tb_set_active (suid,   tempfle->st_mode & S_ISUID);
      tb_set_active (sgid,   tempfle->st_mode & S_ISGID);
      tb_set_active (sticky, tempfle->st_mode & S_ISVTX);

      tb_set_active (ur,     tempfle->st_mode & S_IRUSR);
      tb_set_active (uw,     tempfle->st_mode & S_IWUSR);
      tb_set_active (ux,     tempfle->st_mode & S_IXUSR);

      tb_set_active (gr,     tempfle->st_mode & S_IRGRP);
      tb_set_active (gw,     tempfle->st_mode & S_IWGRP);
      tb_set_active (gx,     tempfle->st_mode & S_IXGRP);

      tb_set_active (or,     tempfle->st_mode & S_IROTH);
      tb_set_active (ow,     tempfle->st_mode & S_IWOTH);
      tb_set_active (ox,     tempfle->st_mode & S_IXOTH);
    }

  gtk_widget_show_all (dialog);
}

