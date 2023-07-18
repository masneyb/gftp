/***********************************************************************************/
/*  platform_specific.c - platform specific functionality                          */
/*  Copyright (C) 2016 Steven Edwards <winehacker@gmail.com>                       */
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

#ifdef __APPLE__
#include <dlfcn.h>

struct ProcessSerialNumber {
   unsigned long highLongOfPSN;
   unsigned long lowLongOfPSN;
};
typedef struct ProcessSerialNumber ProcessSerialNumber;
typedef ProcessSerialNumber * ProcessSerialNumberPtr;

typedef void (*_pGetCurrentProcess)(struct ProcessSerialNumber *psn);
typedef void (*_pCPSEnableForegroundOperation)(struct ProcessSerialNumber *psn, int val2, int val3, int val4, int val5);
typedef void (*_pSetFrontProcess)(struct ProcessSerialNumber *psn);

void gftp_gtk_platform_specific_init(void)
{
  /* There are some cases where GTK apps don't start in the foreground
   * this calls the legacy Carbon Process Number forces the WM to bring it in to focus.
   *
   * I don't actually know all of the arguments these functions take as they are undocumented
   * but thankfully C lets us get away without having the proper prototype for these.
   *
   * ProcessSerialNumber psn;
   * GetCurrentProcess( &psn );
   * CPSEnableForegroundOperation( &psn );
   * SetFrontProcess( &psn );
   */
    int val2 = 0, val3 = 0, val4 = 0, val5 = 0;

    void *carbon_framework = dlopen("/System/Library/Frameworks/Carbon.framework/Carbon", RTLD_LAZY|RTLD_NOW|RTLD_GLOBAL);

    _pGetCurrentProcess pGetCurrentProcess;
    _pCPSEnableForegroundOperation pCPSEnableForegroundOperation;
    _pSetFrontProcess pSetFrontProcess;

    *(void**)(&pGetCurrentProcess)  = dlsym(carbon_framework, "GetCurrentProcess");
    *(void**)(&pCPSEnableForegroundOperation) = dlsym(carbon_framework, "CPSEnableForegroundOperation");
    *(void**)(&pSetFrontProcess) = dlsym(carbon_framework, "SetFrontProcess");

    ProcessSerialNumber psn;
    (void)pGetCurrentProcess(&psn);
    (void)pCPSEnableForegroundOperation(&psn, val2, val3, val4, val5);
    (void)pSetFrontProcess(&psn);

    dlclose(carbon_framework);
}
#else
/* No Dirty Hacks for you */
void gftp_gtk_platform_specific_init(void){};
#endif

// Replace GtkCList with GtkTreeView
//gtk_ctree_node_get_row_data
//
//
//
//
// warning: ‘gtk_image_menu_item_new_with_mnemonic’ is deprecated: Use 'gtk_menu_item_new_with_mnemonic' instead [-Wdeprecated-declarations]
#if 0
908 |         item = GTK_MENU_ITEM (gtk_image_menu_item_new_with_mnemonic (label));
/usr/include/gtk-3.0/gtk/deprecated/gtkimagemenuitem.h:82:12: note: declared here
   82 | GtkWidget* gtk_image_menu_item_new_with_mnemonic (const gchar      *label);
misc-gtk.c:909:9: warning: ‘gtk_image_menu_item_set_image’ is deprecated [-Wdeprecated-declarations]
  909 |         gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
/usr/include/gtk-3.0/gtk/deprecated/gtkimagemenuitem.h:92:12: note: declared here
misc-gtk.c:909:9: warning: ‘gtk_image_menu_item_get_type’ is deprecated: Use 'gtk_menu_item_get_type' instead [-Wdeprecated-declarations]
  909 |         gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
/usr/include/gtk-3.0/gtk/deprecated/gtkimagemenuitem.h:76:12: note: declared here
misc-gtk.c:913:9: warning: ‘gtk_image_menu_item_new_from_stock’ is deprecated: Use 'gtk_menu_item_new' instead [-Wdeprecated-declarations]
#endif
