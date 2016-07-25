#ifndef _PORT_H
#define _PORT_H

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

static inline void mac_gtk_foreground_hack(void)
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
static inline void mac_gtk_foreground_hack(void){};
#endif

#endif /* _PORT_H */

