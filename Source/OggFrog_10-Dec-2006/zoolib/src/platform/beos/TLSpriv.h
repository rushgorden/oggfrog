#if __BEOS__
/*	================================================================================
	Project		: TLS
	File		: TLSpriv.h
	Copyright	: 1997 - Laurent POINTAL
	Distribution: Freeware, source available
	================================================================================	*/

#ifndef _TLSPRIV_H
#define _TLSPRIV_H

#ifdef __cplusplus
extern "C" {
#endif

//	===== MODULE STRUCTURES, TYPES AND CONSTANTS ===================================
//	--- ENTRIES
//	Number of tls entries to add at each overflow.
#define THREAD_ENTRIES_ADD_COUNT				50
//	Table of most recently used thread entries.
#define THREAD_ENTRIES_RECENT_COUNT			4
//	Structure associated to each thread requesting TLS service.
typedef struct {
	int sign; // Indicator/signature of the structure.
	thread_id ownerThread; // Thread which own this entry.
	int datasCount; // TLS data count for that thread
	tls_data_t *datas; // TLS data for that thread.
	} _tls_thread_entry_t;
//	Signature identifying structure.
#define SIGN_OK			'=dat'
#define SIGN_FREE		'!dat'

//	--- INDEXS
//	Number of indexs to add at each overflow.
#define INDEXS_ADD_COUNT						10
//	Number of index allocated.
extern int32 gm_indexs_allocated_count ;

//	--- ENTRY/EXIT
//	Structure associated to each entry/exit proc.
typedef struct _tls_entry_exit_t
	{
	int useCount ;
	tls_proc_t proc ;
	int param ;
	struct _tls_entry_exit_t* next ;
	} _tls_entry_exit_t ;
//	Must a proc be inserted/removed into/from entry or exit procs list.
#define PROC_ENTRY		true
#define PROC_EXIT		false


//	===== ENTRIES TOOL FUNCTIONS ===================================================
//	Note: these functions assume mutual exclusion is done by caller.
//	Acquire/release the entries mutex.
extern status_t tls_entries_mutex_p () ;
extern void tls_entries_mutex_v () ;
//	Get the entry for the current thread.
extern status_t tls_get_entry (thread_id id, _tls_thread_entry_t** ppEntry) ;
//	Create a new entry for a thread.
extern status_t tls_create_entry (thread_id id, _tls_thread_entry_t** ppEntry) ;
//	Delete an existing entry (by entry pointer or by thread id).
extern status_t tls_delete_entry (_tls_thread_entry_t* pEntry) ;
extern status_t tls_delete_entry_thread_id (thread_id id) ;
//	Expand the threads entries table.
extern status_t tls_expand_entries (int numAlloc) ;

//	===== INDEXS TOOL FUNCTIONS ====================================================
//	Note: these functions assume mutual exclusion is done by caller.
//	Acquire/release the indexs mutex.
extern status_t tls_index_mutex_p () ;
extern void tls_index_mutex_v () ;
//	Check if an index has been allocated.
extern bool tls_check_index (int index) ;
//	Allocate a new index.
extern status_t tls_create_index (int* index) ;
//	Delete an existing index.
extern status_t tls_delete_index (int index) ;
//	Expand the indexs table.
extern status_t tls_expand_indexs (int numAlloc) ;

//	===== VALUES-TABLE TOOL FUNCTIONS ==============================================
extern status_t tls_expand_values (_tls_thread_entry_t* e, int numberOfIndexs) ;

//	===== ENTRY/EXIT TOOL FUNCTIONS ================================================
//	Note: these functions does themselves synchronization. You MUST use them to go
//		from one _tls_proc_t struture to another.
//	Acquire/release the indexs mutex.
extern status_t tls_entry_exit_mutex_p () ;
extern void tls_entry_exit_mutex_v () ;
//	Allocate a new entry/exit structure, and insert it.
extern status_t tls_create_entry_exit (bool bEntry, tls_proc_t proc, int param) ;
//	Find an existing entry/exit structure, and remove it.
extern status_t tls_remove_entry_exit (bool bEntry, tls_proc_t proc, int param) ;
//	Get access to the first entry/exit structure.
extern _tls_entry_exit_t* tls_access_first (bool bEntry) ;
//	Get access to the next entry/exit structure.
extern _tls_entry_exit_t* tls_access_next (_tls_entry_exit_t* pCurrent) ;
//	Call entry/exit procs.
extern void tls_call_entry_exit (thread_id id, bool bEntry, 
										_tls_thread_entry_t* threadEnv) ;

#ifdef DEBUG_TLS
	#define TlsDebug(x)				printf (x)
	#define TlsDebug0(x)			printf (x)
	#define TlsDebug1(x,y)			printf (x,y)
	#define TlsDebug2(x,y,z)		printf (x,y,z)
	#define TlsDebug3(x,y,z,t)		printf (x,y,z,t)
#else
	#define TlsDebug(x)				(0)
	#define TlsDebug0(x)			(0)
	#define TlsDebug1(x,y)			(0)
	#define TlsDebug2(x,y,z)		(0)
	#define TlsDebug3(x,y,z,t)		(0)
#endif


#ifdef __cplusplus
}
#endif

#endif
#endif // __BEOS__