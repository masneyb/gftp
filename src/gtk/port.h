#ifndef _PORT_H
#define _PORT_H

#ifdef __APPLE__
#include <dlfcn.h>
#include <Carbon/Carbon.h>

typedef void (*_pGetCurrentProcess)(const char *psn);
typedef void (*_pCPSEnableForegroundOperation)(const char *psn, unsigned int *arg2, unsigned int *arg3, unsigned int *arg4, unsigned int *arg5);
typedef void (*_pSetFrontProcess)(const char *psn);

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

    void *carbon_framework = dlopen("/System/Library/Frameworks/Carbon.framework/Carbon", RTLD_LAZY|RTLD_NOW|RTLD_GLOBAL);

    int arg2, arg3, arg4, arg5;

    _pGetCurrentProcess pGetCurrentProcess;
    _pCPSEnableForegroundOperation pCPSEnableForegroundOperation;
    _pSetFrontProcess pSetFrontProcess;

    *(void**)(&pGetCurrentProcess)  = dlsym(carbon_framework, "GetCurrentProcess");
    *(void**)(&pCPSEnableForegroundOperation) = dlsym(carbon_framework, "CPSEnableForegroundOperation");
    *(void**)(&pSetFrontProcess) = dlsym(carbon_framework, "SetFrontProcess");

    ProcessSerialNumber psn;
    (void)pGetCurrentProcess(&psn);
    (void)pCPSEnableForegroundOperation(&psn, arg2, arg3, arg4, arg5);
    (void)pSetFrontProcess(&psn);

    dlclose(carbon_framework);
}
#else
static inline void mac_gtk_foreground_hack(void){};
#endif

#endif /* _PORT_H */

