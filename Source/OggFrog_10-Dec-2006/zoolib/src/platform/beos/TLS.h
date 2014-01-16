#if __BEOS__
/*	================================================================================
	Project		: TLS
	File		: TLS.h
	Copyright	: 1997 - Laurent POINTAL
	Distribution: Freeware, source available
	================================================================================	*/

#ifndef _TLS_H
#define _TLS_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

// MDC added this for backwards compatibility
#ifndef _EXPORT
#define _EXPORT
#endif

//	Version used to fail tls_init if client use too old interface. 
//
#define TLS_VERSION		0x0100000b

//	Initialise and terminate TLS management.
extern _EXPORT status_t tls_init (int clientVersion) ;
extern _EXPORT status_t tls_term () ;

//	Basic TLS data type. May be changed to an union in the future...
//	So prefer use of typed set and get functions.
typedef int tls_data_t ;
#define TLS_INVALID_DATA	0xFFFFF0F0

//	Allocating/freeing TLS entry indexs.
extern _EXPORT status_t tls_alloc (int* pIndex) ;
extern _EXPORT status_t tls_free (int index) ;
//	Setting/Getting TLS data.
extern _EXPORT status_t tls_set (int index, tls_data_t data) ;
extern _EXPORT status_t tls_get (int index, tls_data_t* pData) ;


//	Hook calling functions to be called at first use of TLS in a thread,
//	or at detection of a thread ended. To replace the Win32 ability of
//	DLLs main code to be called at each thread entry and exit.
//	Allow to maintain automatic TLS allocation/deallocation for datas
//	used transparently by functions which are called by threads.
//	As this code can be called AFTER the thread ending (ie. in another
//	thread), the thread id is given as a parameter.
typedef void (*tls_proc_t) (thread_id id, int param, void* threadEnv) ;

//	Installing/Removing tls entry/exit procs.
//	Entry procs are called in the order they are insalled.
//	Exit procs are calle in the reverse order they are installed.
extern _EXPORT status_t tls_install_entry_proc (tls_proc_t proc, int param) ;
extern _EXPORT status_t tls_remove_entry_proc (tls_proc_t proc, int param) ;
extern _EXPORT status_t tls_install_exit_proc (tls_proc_t proc, int param) ;
extern _EXPORT status_t tls_remove_exit_proc (tls_proc_t proc, int param) ;

//	Signaling entering/exiting a thread (to be sure that entry/exit procs
//	are called at the right time/in the right thread, and not when these 
//	events are detected...).
extern _EXPORT status_t tls_entering_procs () ;
extern _EXPORT status_t tls_exiting_procs () ;

//	Setting/Getting TLS data ***into a tls proc***.
//	The 
extern _EXPORT status_t tls_set_inproc (int index, tls_data_t data, void* threadEnv) ;
extern _EXPORT status_t tls_get_inproc (int index, tls_data_t* pData, void* threadEnv) ;

#ifdef __cplusplus
}
#endif

#endif
#endif // __BEOS__