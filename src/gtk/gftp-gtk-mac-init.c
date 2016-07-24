/*****************************************************************************/
/*  gftp-gtk-mac-init.c - macOS GTK init corner case                         */
/*  Copyright (C) 1998-2007 Brian Masney <masneyb@gftp.org>                  */
/*  Copyright (C) 2016 Steven Edwards <winehacker@gmail.com>                 */
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
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA      */
/*****************************************************************************/

#include "gftp-gtk.h"

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

void gftp_gtk_mac_foreground_init( void )
{
  /* There are some cases where GTK apps don't start foregrounded so
   * this calls the legacy Carbon Process Number and forces the WM to 
   * bring the requested process in to focus.
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
void gftp_gtk_mac_foreground_init(void){};
#endif

