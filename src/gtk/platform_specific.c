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
#include <gtk/gtk.h>

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

/* Extern reference to global UI manager */
extern GtkUIManager *factory;

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

#ifdef MAC_INTEGRATION
    /* Initialize gtk-mac-integration for native macOS menu bar */
    GtkosxApplication *theApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);

    /* Get the menu bar from the UI manager and move it to the macOS menu bar */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    GtkWidget *menubar = gtk_ui_manager_get_widget(factory, "/M");

    if (menubar && GTK_IS_MENU_BAR(menubar)) {
        /* Hide the menu bar from the window since it will be in the system menu bar */
        gtk_widget_hide(menubar);

        /* Set the menu bar in the macOS menu bar */
        gtkosx_application_set_menu_bar(theApp, GTK_MENU_BAR(menubar));

        /* Enable native macOS keyboard accelerators */
        gtkosx_application_set_use_quartz_accelerators(theApp, TRUE);

        /* Add Help -> About to the application menu */
        GtkWidget *about_item = gtk_ui_manager_get_widget(factory, "/M/HelpMenu/HelpAbout");
        if (about_item) {
            gtkosx_application_insert_app_menu_item(theApp, about_item, 0);
            gtkosx_application_insert_app_menu_item(theApp, gtk_separator_menu_item_new(), 1);
        }

        /* Signal that the application is ready */
        gtkosx_application_ready(theApp);
    }
#pragma GCC diagnostic pop
#endif
}
#else
/* No Dirty Hacks for you */
void gftp_gtk_platform_specific_init(void){};
#endif

