/*
 * This is free and unencumbered software released into the public domain.
 *
 * For more information, please refer to <https://unlicense.org>
 */

/** 2020-09-14 **/

#ifndef __GLIB_COMPAT_H
#define __GLIB_COMPAT_H

#include <glib.h>

// GLIB < 2.58
#if ! GLIB_CHECK_VERSION (2, 58, 0)
#define G_SOURCE_FUNC(f) ((GSourceFunc) (void (*)(void)) (f))
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
#define g_list_free_full(list, free_func) {\
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
