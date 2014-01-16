#if __BEOS__
/*	================================================================================
	Project		: TLS
	File		: TLSinside.c
	Copyright	: 1997 - Laurent POINTAL
	Distribution: Freeware, source available
	--------------------------------------------------------------------------------
	Just a small shared library to allow support of Thread Local Storage (until Be
	insert this functionality in the kernel...).
	================================================================================	*/

#include <KernelKit.h>
#include <Errors.h>
#include <string.h>		// MDC for memset
#include <stdlib.h>
#include <assert.h>
#pragma export on
#include "TLS.h"
#pragma export reset
#include "TLSpriv.h"



//	===== TOOL FUNCTIONS ===========================================================
//	Function used to search in the sorted entries table.
static int bsearch_comp_proc (const void* keyval, const void* entry) ;
//	Function used to sort in the sorted entries table.
static int qsort_comp_proc (const void* entry1, const void* entry2) ;
//	Function used to create a new reference.
static _tls_entry_exit_t* new_reference (_tls_entry_exit_t* p) ;
//	Function used to delete a reference (and eventually delete structure).
static void release_reference (_tls_entry_exit_t* p) ;
//	Thread dying monitoring thread.
static int32 monitoring (void*) ;

//	===== MODULE DATA ==============================================================
static bool gm_initOk = false ;
//	--- MONITORING
//	Monitoring thread identifier.
static thread_id gm_monitor = 0 ;
//	Monitoring stop indicator.
static sem_id gm_signalExit = 0 ;
//	--- ENTRIES
//	Mutex to protect concurrent access to entries common global datas.
static sem_id gm_mutex_entries = 0 ;
//	Variable to implement benaphores. 
static int32 gm_mutex_entries_val = 1 ;
//	Recently used entries for threads ;
static _tls_thread_entry_t* gm_recent[THREAD_ENTRIES_RECENT_COUNT] ;
//	All entries for threads.
static _tls_thread_entry_t** gm_sorted = NULL ;
//	All entries for threads - count.
static int gm_entries_count = 0 ;
//	Entries for threads - allocation count.
static int gm_entries_allocated_count = 0 ;
//	--- INDEXS
//	Mutex to protect concurrent access to indexs common globals.
static sem_id gm_mutex_indexs = 0 ;
//	Variable to implement benaphores. 
static int32 gm_mutex_indexs_val = 1 ;
//	Array of indexs use.
static bool* gm_indexs = NULL ;
//	Indexs - allocation count.
int32 gm_indexs_allocated_count = 0 ;
//	--- ENTRY / EXIT
//	Mutex to protect concurrent access to entry/exit common globals.
static sem_id gm_mutex_entry_exit = 0 ;
//	Variable to implement benaphores. 
static int32 gm_mutex_entry_exit_val = 1 ;
//	Start of entry and exit procs lists.
static _tls_entry_exit_t* gm_entry_proc_list = NULL ;
static _tls_entry_exit_t* gm_exit_proc_list = NULL ;


/*	===== tls_init =================================================================
	PRIVATE. Initialise TLS management. Called at dynamic module loading.
	Note: as we use Benaphore like mutexs, the semaphores initial count are 0.
	================================================================================	*/
status_t tls_init (int clientVersion)
	{
	int i ;
	status_t status ;
	int semCreatedCount = 0 ;

	//	Currently, the test is based on the same primary version number.
	if ((clientVersion & 0xFF000000) < (TLS_VERSION &0xFF000000)) return B_ERROR ;

	//	Create entries mutex. If the mutex already exist, the init phase is already
	//	done (or is running). In this case, we wait for the mutex to be released
	//	by its current owner, and then return doing nothing. 
	if (gm_mutex_entries != 0)
		{
		if ((status=tls_entries_mutex_p ()) != B_NO_ERROR)	// Wait for the mutex.
			goto ERROR_LABEL ;
		tls_entries_mutex_v ();	// Release it.
		return B_NO_ERROR ;	// Consider init already done.
		}
	else
		{
		//	Note: When the semaphore is created, gm_mutex_entries_val is with
		//	a value 1 (not 0). So during init phase other threads will block on
		//	semaphore.
		status = gm_mutex_entries = create_sem (0, "tls-entries") ;
		if (gm_mutex_entries < B_NO_ERROR) goto ERROR_LABEL ;
		semCreatedCount = 1 ;
		}
		
	//	Create indexs mutex.
	status = gm_mutex_indexs = create_sem (0, "tls-indexs") ;
	if (gm_mutex_indexs < B_NO_ERROR) goto ERROR_LABEL ;
	semCreatedCount = 2 ;

	//	Create	entry/exit mutex.
	status = gm_mutex_entry_exit = create_sem (0, "tls-entry-exit") ;
	if (gm_mutex_indexs < B_NO_ERROR) goto ERROR_LABEL ;
	semCreatedCount = 3 ;

	//	Initialize array of last recently used entries.
	for (i=0 ; i<THREAD_ENTRIES_RECENT_COUNT ; i++)
		gm_recent[i] = NULL ;

	//	Create tls ended thread management.
	status = gm_signalExit = create_sem (0, "tls-monitor-exit") ;
	if (gm_signalExit < B_NO_ERROR) goto ERROR_LABEL ;
	semCreatedCount = 4 ;
	status = gm_monitor = spawn_thread (monitoring, "tls_monitor", B_LOW_PRIORITY, NULL) ;
	if (gm_monitor != B_NO_MORE_THREADS && gm_monitor != B_NO_MEMORY)
		resume_thread (gm_monitor) ;
	else
		goto ERROR_LABEL ;

	//	Finish.
	tls_entries_mutex_v () ;
	tls_index_mutex_v () ;
	tls_entry_exit_mutex_v () ;
	gm_initOk = true ;
	return B_NO_ERROR ;

	ERROR_LABEL:
	//	Error processing goes here. Release the resources that may have been allocated.
	gm_monitor = 0 ;
	if (semCreatedCount >= 4)
		{
		delete_sem (gm_signalExit) ; 
		gm_signalExit = 0 ;
		}
	if (semCreatedCount >= 3)
		{
		delete_sem (gm_mutex_entry_exit) ;
		gm_mutex_entry_exit = 0 ;
		gm_mutex_entry_exit_val = 1 ;
		}
	if (semCreatedCount >= 2)
		{
		delete_sem (gm_mutex_indexs) ;
		gm_mutex_indexs = 0 ;
		gm_mutex_indexs_val = 1 ;
		}
	if (semCreatedCount >= 1)
		{
		delete_sem (gm_mutex_entries) ; 
		gm_mutex_entries = 0 ;
		gm_mutex_entries_val = 1 ;
		}
	//	Return the status got when the error was detected.
	return status ;
	}


/*	===== tls_term =================================================================
	PRIVATE. Terminate TLS management.
	Note: When its called, you should be sure that there is no more thread to call
		TLS services.
	================================================================================	*/
status_t tls_term ()
	{
	status_t exit_value ;
	int i ;

	if (!gm_initOk) return B_NO_ERROR ;

	//	Signal to monitoring thread that it must end, and waits for it.
	release_sem (gm_signalExit) ;
	wait_for_thread (gm_monitor, &exit_value) ;
	//	Free monitor references.
	delete_sem (gm_signalExit) ; 
	gm_signalExit = 0 ;
	gm_monitor = 0 ;

	//	Remove entry and exit procs.
	while (gm_entry_proc_list && (B_NO_ERROR == tls_remove_entry_exit (PROC_ENTRY, 
						gm_entry_proc_list->proc, gm_entry_proc_list->param))) ;
	while (gm_exit_proc_list &&  (B_NO_ERROR == tls_remove_entry_exit (PROC_EXIT, 
						gm_exit_proc_list->proc, gm_exit_proc_list->param))) ;

	//	Acquire resources to be released. 
	if (gm_mutex_entries)		tls_index_mutex_p () ;
	if (gm_mutex_indexs)		tls_entries_mutex_p () ;
	if (gm_mutex_entry_exit)	tls_entry_exit_mutex_p () ;

	//	Free resources. 
	delete_sem (gm_mutex_entries) ; 
	gm_mutex_entries = 0 ;
	gm_mutex_entries_val = 1 ;
	for (i=0 ; i<THREAD_ENTRIES_RECENT_COUNT ; i++) gm_recent[i] = NULL ;

	delete_sem (gm_mutex_indexs) ;
	gm_mutex_indexs = 0 ;
	gm_mutex_indexs_val = 1 ;

	delete_sem (gm_mutex_entry_exit) ;
	gm_mutex_entry_exit = 0 ;
	gm_mutex_entry_exit_val = 1 ;

	//	Note: memory is not freed, and counters are not reseted (can be reused as is).
	if (gm_indexs) memset (gm_indexs, 0, sizeof(bool)*gm_indexs_allocated_count) ;

	gm_initOk = false ;

	return B_NO_ERROR ;
	}

/*	===== tls_entries_mutex_p ======================================================
	PRIVATE. Take the mutex.
	================================================================================	*/
status_t tls_entries_mutex_p () 
	{
	int32 previous = atomic_add (&gm_mutex_entries_val, 1); 
	if (previous >= 1) 
		return acquire_sem (gm_mutex_entries) ;
	else
		return B_NO_ERROR ;
	}


/*	===== tls_entries_mutex_v ======================================================
	PRIVATE. Release the mutex.
	================================================================================	*/
void tls_entries_mutex_v ()
	{
	int32 previous = atomic_add (&gm_mutex_entries_val, -1); 
	if (previous > 1) 
		release_sem_etc (gm_mutex_entries, 1, B_DO_NOT_RESCHEDULE); 
	}


/*	===== tls_get_entry ============================================================
	PRIVATE. Get the entry for the designed thread.
	================================================================================	*/
status_t tls_get_entry (thread_id id, _tls_thread_entry_t** ppEntry)
	{
	_tls_thread_entry_t* founded = NULL ;
	_tls_thread_entry_t** pFounded = NULL ;
	int i ;

	//	Internal check - must never occur.
	assert (ppEntry != NULL) ;

	//	Ensure never remain bad value in this return address.
	*ppEntry = NULL ;

	//	First, search in the most recently used entries.
	for (i=0 ; i<THREAD_ENTRIES_RECENT_COUNT ; i++)
		{
		if (gm_recent[i] && gm_recent[i]->sign==SIGN_OK && gm_recent[i]->ownerThread==id)
			{
			founded = gm_recent[i] ;
			//	Reorder most recently used entries. Make room at head
			if (i>0) 
				{
				memmove (gm_recent+1, gm_recent, i*sizeof (_tls_thread_entry_t*)) ;
				gm_recent[0] = founded ;
				}
			*ppEntry = founded ;
			return B_NO_ERROR ;
			}
		}

	//	Then search in the sorted array of thread entries.
	pFounded = (_tls_thread_entry_t**) bsearch ((const void*)id, gm_sorted, 
			gm_entries_count, sizeof (_tls_thread_entry_t*), bsearch_comp_proc) ;
	//	If an entry exists, insert it at head of recently used entries, and return it.
	if (pFounded) 
		{
		founded = *pFounded ;
		assert (founded != NULL) ;
		memmove (gm_recent+1, gm_recent, 
				(THREAD_ENTRIES_RECENT_COUNT-1)*sizeof (_tls_thread_entry_t*)) ;
		gm_recent[0] = founded ;
		*ppEntry = founded ;
		return B_NO_ERROR ;
		}

	//	Not founded (we use 'name', but its 'id').
	return B_NAME_NOT_FOUND ;
	}

/*	===== tls_create_entry =========================================================
	PRIVATE. Create an entry for a thread.
	================================================================================	*/
status_t tls_create_entry (thread_id id, _tls_thread_entry_t** ppEntry) 
	{
	_tls_thread_entry_t* pEntry = NULL ;
	status_t status ;
	int i ;

	//	Check parameters.
	if (id < 0) return B_BAD_VALUE ;
	if (ppEntry == NULL) return B_BAD_VALUE ;

	//	Set to NULL so that a use without error checking will resolve into error too.
	*ppEntry = NULL ;

	//	If no more room, reallocate more entries.
	if (gm_entries_count == gm_entries_allocated_count)
		{
		if ((status = tls_expand_entries (THREAD_ENTRIES_ADD_COUNT)) != B_NO_ERROR) 
			return status ;
		}

	//	Use next entry in the sorted array.
	//	If data remain associated with the entry, erase it.
	pEntry = gm_sorted[gm_entries_count] ;
	pEntry->sign = SIGN_OK ;
	pEntry->ownerThread = id ;
	if (pEntry->datasCount)
		{
		for (i=0 ; i<pEntry->datasCount ; i++)
			pEntry->datas[i] = 0 ;
		}
	gm_entries_count++ ;

	//	Sort the array with the new entry.
	assert (gm_sorted != NULL) ;
	assert (gm_entries_count >= 0) ;
	qsort (gm_sorted, gm_entries_count, sizeof (_tls_thread_entry_t*), qsort_comp_proc) ;

	//	As a new entry is created for this thread, call the TLS entry procs.
	tls_call_entry_exit (id, PROC_ENTRY, pEntry) ;

	//	Finish.
	*ppEntry = pEntry ;
	return B_NO_ERROR ;
	}

/*	===== tls_delete_entry =========================================================
	PRIVATE. Delete an entry by its entry address.
	================================================================================	*/
status_t tls_delete_entry (_tls_thread_entry_t* pEntry)
	{
	//	Internal check - must never occur.
	assert (pEntry != NULL) ;
	//	As the entry is deleted, call the TLS exit procs.
	tls_call_entry_exit (pEntry->ownerThread, PROC_EXIT, pEntry) ;
	//	Simply note it as free.
	pEntry->sign = SIGN_FREE ;
	//	Sort-it so it be moved at the end of the sorted array.
	assert (gm_sorted != NULL) ;
	assert (gm_entries_count >= 0) ;
	qsort (gm_sorted, gm_entries_count, sizeof (_tls_thread_entry_t*), qsort_comp_proc) ;
	//	And decrement the number of elements.
	gm_entries_count-- ;

	return B_NO_ERROR ;
	}


/*	===== tls_delete_entry_thread_id ===============================================
	PRIVATE. 
	================================================================================	*/
status_t tls_delete_entry_thread_id (thread_id id)
	{
	_tls_thread_entry_t* pEntry = NULL ;
	status_t status ;

	if ((status = tls_get_entry (id, &pEntry)) != B_NO_ERROR) return status ;
	return tls_delete_entry (pEntry) ;
	}


/*	===== tls_expand_entries =======================================================
	PRIVATE. Allocate more entries into the thread entries table.
	================================================================================	*/
status_t tls_expand_entries (int numAlloc)
	{
	_tls_thread_entry_t* newEntries = NULL ;
	_tls_thread_entry_t** newSorted = NULL ;
	int i ;

	//	Allocate and zero-initialize new slots.
	newEntries = (_tls_thread_entry_t*) malloc (numAlloc * sizeof(_tls_thread_entry_t)) ;
	if (!newEntries) return B_NO_MEMORY ;
	memset (newEntries, 0, numAlloc * sizeof (_tls_thread_entry_t)) ;

	//	Expand and initialize new pointers of sorted table.
	newSorted = (_tls_thread_entry_t**) realloc (gm_sorted, 
			(gm_entries_allocated_count + numAlloc) * sizeof (_tls_thread_entry_t*)) ;
	if (!newSorted)
		{
		free (newEntries) ;
		return B_ERROR ;
		}
	for (i=0 ; i<numAlloc ; i++)
		{
		newSorted[gm_entries_allocated_count+i] = newEntries + i ;
		newEntries[i].sign = SIGN_FREE ;
		}

	//	Finish.
	gm_sorted = newSorted ;
	gm_entries_allocated_count += numAlloc ;
	return B_NO_ERROR ;
	}


/*	===== bsearch_comp_proc ========================================================
	PRIVATE. Used with bsearch.
	================================================================================	*/
int bsearch_comp_proc (const void* keyval, const void* entry)
	{
	thread_id id = (thread_id) keyval ;
	_tls_thread_entry_t* pEntry = * (_tls_thread_entry_t**)entry ;
	assert (pEntry->sign == SIGN_OK) ;
	//	See K&R for sign of value returned.
	return (id - pEntry->ownerThread) ;
	}


/*	===== qsort_comp_proc ==========================================================
	PRIVATE. Used with qsort.
	================================================================================	*/
int qsort_comp_proc (const void* entry1, const void* entry2)
	{
	_tls_thread_entry_t* pEntry1 = * (_tls_thread_entry_t**)entry1 ;
	_tls_thread_entry_t* pEntry2 = * (_tls_thread_entry_t**)entry2 ;
	//	Free entries must be moved to the end of the array.
	if (pEntry1->sign == SIGN_FREE && pEntry2->sign == SIGN_FREE) return 0 ; 
	if (pEntry1->sign == SIGN_FREE) return +1 ;
	if (pEntry2->sign == SIGN_FREE) return -1 ;
	//	See K&R for sign of value returned.
	return (pEntry1->ownerThread - pEntry2->ownerThread) ;
	}

/*	===== tls_index_mutex_p ========================================================
	PRIVATE. Take the mutex.
	================================================================================	*/
status_t tls_index_mutex_p () 
	{
	int32 previous = atomic_add (&gm_mutex_indexs_val, 1); 
	if (previous >= 1) 
		return acquire_sem (gm_mutex_indexs) ;
	else
		return B_NO_ERROR ;
	}


/*	===== tls_index_mutex_v ========================================================
	PRIVATE. Release the mutex.
	================================================================================	*/
void tls_index_mutex_v () 
	{
	int32 previous = atomic_add (&gm_mutex_indexs_val, -1); 
	if (previous > 1) 
		release_sem_etc (gm_mutex_indexs, 1, B_DO_NOT_RESCHEDULE); 
	}


/*	===== tls_check_index ==========================================================
	PRIVATE. Check if an index is true (allocated) or false (free).
	================================================================================	*/
bool tls_check_index (int index)
	{
	if (index < 0 || index >= gm_indexs_allocated_count) return false ;
	return gm_indexs[index] ;
	}


/*	===== tls_create_index =========================================================
	PRIVATE. Allocate a new index. Expand indexs array if necessary.
	================================================================================	*/
status_t tls_create_index (int* index) 
	{
	int i ;
	status_t status ;

	//	First, search if there is not a free index in the allocated ones.
	//	Note: if not founded, i end with the gm_indexs_allocated_count value.
	for (i=0 ; i<gm_indexs_allocated_count ; i++)
		{
		if (!gm_indexs[i]) 
			{
			gm_indexs[i] = true ;
			*index = i ;
			return B_NO_ERROR ;
			}
		}
	
	//	Allocate more indexs, and return the next one allocated.
	if ((status = tls_expand_indexs (INDEXS_ADD_COUNT)) != B_NO_ERROR) return status ;
	gm_indexs[i] = true ;
	*index = i ;
	return B_NO_ERROR  ;
	}


/*	===== tls_delete_index =========================================================
	PRIVATE. Delete an index use.
	================================================================================	*/
status_t tls_delete_index (int index) 
	{
	//	Check range.
	if (index < 0 || index >= gm_indexs_allocated_count) return B_BAD_INDEX ;
	//	Check if used.
	if (gm_indexs[index] == false) return B_BAD_INDEX ;
	//	Free index.
	gm_indexs[index] = false ;
	return B_NO_ERROR ;
	}


/*	===== tls_expand_indexs ========================================================
	PRIVATE. Initialise TLS management. Called at dynamic module loading.
	================================================================================	*/
status_t tls_expand_indexs (int numAlloc)
	{
	bool* newIndexs = NULL ;
	int i ;

	//	Allocate and initialize new indexs of indexs table.
	newIndexs = (bool*) realloc (gm_indexs, gm_indexs_allocated_count + numAlloc) ;
	if (!newIndexs) return B_NO_MEMORY ;
	for (i=0 ; i<numAlloc ; i++)
		newIndexs[gm_indexs_allocated_count+i] = false ;

	//	Finish.
	gm_indexs = newIndexs ;
	atomic_add (&gm_indexs_allocated_count, numAlloc) ;
	return B_NO_ERROR ;
	}


/*	===== tls_expand_values ========================================================
	PUBLIC. Expand the number of values of en entry so that it correspond to the
	number of index used.
	If the number of values is ok, nothing is dones.
	================================================================================	*/
status_t tls_expand_values (_tls_thread_entry_t* e, int numberOfIndexs)
	{
	tls_data_t* p ;
	
	if (e->datasCount >= numberOfIndexs) return B_NO_ERROR ;

	//	Try reallocating.
	p = (tls_data_t*) realloc (e->datas, 
			numberOfIndexs * sizeof (tls_data_t)) ;
	if (!p) return B_NO_MEMORY ;

	//	Initialize newly allocated values with zeros.
	memset (p + e->datasCount, 0, (numberOfIndexs - e->datasCount) 
											* sizeof (tls_data_t)) ;

	//	Finish.
	e->datas = p ;
	e->datasCount = numberOfIndexs ;
	return B_NO_ERROR ;
	}


/*	===== tls_entry_exit_mutex_p ===================================================
	PRIVATE. Take the mutex.
	================================================================================	*/
status_t tls_entry_exit_mutex_p () 
	{
	int32 previous = atomic_add (&gm_mutex_entry_exit_val, 1); 
	if (previous >= 1) 
		return acquire_sem (gm_mutex_entry_exit) ;
	else
		return B_NO_ERROR ;
	}


/*	===== tls_entry_exit_mutex_v ===================================================
	PRIVATE. Release the mutex.
	================================================================================	*/
void tls_entry_exit_mutex_v () 
	{
	int32 previous = atomic_add (&gm_mutex_entry_exit_val, -1); 
	if (previous > 1) 
		release_sem_etc (gm_mutex_entry_exit, 1, B_DO_NOT_RESCHEDULE); 
	}


/*	===== tls_create_entry_exit ====================================================
	PRIVATE. Allocate a new entry/exit structure, and insert it.
	================================================================================	*/
status_t tls_create_entry_exit (bool bEntry, tls_proc_t proc, int param)
	{
	_tls_entry_exit_t* p ;

	//	Check parameters.
	if (proc == NULL) return B_BAD_VALUE ;

	//	Allocate structure, and initialize it.
	p = (_tls_entry_exit_t*) malloc (sizeof (_tls_entry_exit_t)) ;
	if (p == NULL) return B_NO_MEMORY ;
	p->useCount 	= 0 ;
	p->proc 		= proc ;
	p->param 		= param ;
	p->next 		= NULL ;

	//	Insert structure in the right list.
	//	Note: list are processed IN the mutex zone, so we dont care of
	//		managing useCount values.
	tls_entry_exit_mutex_p () ;
		{
		if (bEntry)	// bEntry == PROC_ENTRY
			{
			//	Structures are inserted at the end of the list.
			//	Use the well known algorithm which use list pointer links
			//	address in place of value, to reach the last link address and
			//	be able to update it.
			_tls_entry_exit_t** ppNext = &gm_entry_proc_list ;
			while (*ppNext != NULL)
				ppNext = &((*ppNext)->next) ;
			*ppNext = new_reference (p) ;
			}
		else		// bEntry == PROC_EXIT
			{
			//	Structures are inserted at the begining of the list.
			p->next = gm_exit_proc_list ;
			gm_exit_proc_list = new_reference (p) ;
			}
		}
	tls_entry_exit_mutex_v () ;

	//	Finish.
	return B_NO_ERROR ;
	}


/*	===== tls_remove_entry_exit ====================================================
	PRIVATE. Find an existing entry/exit structure, and remove it.
	The proc AND the param must match.
	================================================================================	*/
status_t tls_remove_entry_exit (bool bEntry, tls_proc_t proc, int param)
	{
	_tls_entry_exit_t* p = NULL ;
	_tls_entry_exit_t** ppNext = &gm_entry_proc_list ;

	//	Check parameters.
	if (proc == NULL) return B_BAD_VALUE ;

	//	Remove structure from the right list.
	//	Note: list are processed IN the mutex zone, so we dont care of
	//		managing useCount values.
	tls_entry_exit_mutex_p () ;
		{
		if (bEntry)	// bEntry == PROC_ENTRY
			ppNext = &gm_entry_proc_list ;
		else		// bEntry == PROC_EXIT
			ppNext = &gm_exit_proc_list ;
		//	Find the structure.
		//	Use the welle known algorithm which use list pointer links
		//	address in place of value, to reach the last link address and
		//	be able to update it.
		while (*ppNext != NULL)
			{
			//	Check proc AND param.
			if ((*ppNext)->proc == proc && (*ppNext)->param == param)
				{
				//	Founded. Remove it from the list.
				p = *ppNext ;
				*ppNext = (*ppNext)->next ;
				p->next = NULL ;
				//	And delete it (if possible).
				release_reference (p) ;
				break ;
				}
			ppNext = &((*ppNext)->next) ;
			}
		}
	tls_entry_exit_mutex_v () ;

	//	Finish.
	if (p != NULL)
		return B_NO_ERROR ;
	else
		return B_ERROR ;
	}


/*	===== tls_access_first =========================================================
	PRIVATE. Get access to the first entry/exit structure.
	================================================================================	*/
_tls_entry_exit_t* tls_access_first (bool bEntry)
	{
	_tls_entry_exit_t* p ;

	tls_entry_exit_mutex_p () ;
		{
		//	Get new reference on the first.
		if (bEntry)	// bEntry == PROC_ENTRY
			p = new_reference (gm_entry_proc_list) ;
		else 		// bEntry == PROC_EXIT
			p = new_reference (gm_exit_proc_list) ;
		}
	tls_entry_exit_mutex_v () ;

	return p ;
	}

/*	===== tls_access_next ==========================================================
	PRIVATE. Get access to the next entry/exit structure.
		Loose access to the current entry/exit structure.
	================================================================================	*/
_tls_entry_exit_t* tls_access_next (_tls_entry_exit_t* pCurrent)
	{
	_tls_entry_exit_t* p ;

	if (!pCurrent) return NULL ;

	tls_entry_exit_mutex_p () ;
		{
		//	Get new reference on next.
		p = new_reference (pCurrent->next) ;
		//	Release reference on current.
		release_reference (pCurrent) ;
		}
	tls_entry_exit_mutex_v () ;

	return p ;
	}


/*	===== new_reference ============================================================
	PRIVATE. Create a new reference on a structure.
	Creation of a reference MUST be sourrounded by mutex allocation.
	================================================================================	*/
_tls_entry_exit_t* new_reference (_tls_entry_exit_t* p)
	{
	if (!p) return NULL ;
	p->useCount++ ;
	return p ;
	}


/*	===== release_reference ========================================================
	PRIVATE. delete a reference (and eventually delete structure).
	Deletion of a reference MUST be sourrounded by mutex allocation.
	================================================================================	*/
void release_reference (_tls_entry_exit_t* p)
	{
	p->useCount-- ;
	if (p->useCount <= 0) free (p) ;
	}


/*	===== tls_call_entry_exit ======================================================
	PRIVATE. Call the entry/exit procs, one by one.
	Between two list access, the list management mutex is released. We use useCount
	to be sure that we never reach an invalid pointer (at least, we use an entry/
	exit proc which has been isolated to be removed - and we dont know about the
	rest of the list).
	================================================================================	*/
void tls_call_entry_exit (thread_id id, bool bEntry, 	_tls_thread_entry_t* threadEnv) 
	{
	_tls_entry_exit_t* p ;

	p = tls_access_first (bEntry) ;
	while (p)
		{
		p->proc (id, p->param, threadEnv) ;
		p = tls_access_next (p) ;
		}
	}


/*	===== monitoring ===============================================================
	PRIVATE. Check in the thread entries list to detect ended threads.
	These threads see their entry deleted.
	================================================================================	*/
int32 monitoring (void* unused)
	{
	int i ;
	int numberDeleted ;
	thread_info info ;
	_tls_thread_entry_t* pEntry ;

	//	We use semaphore time-outs to periodically wake up the monitoring thread,
	//	with acquire_sem_etc returning B_TIMED_OUT.
	//	When the term function is called, it set the semaphore and the thread
	//	is immediatly resumed with acquire_sem_etc returning B_NO_ERROR.
	while (acquire_sem_etc (gm_signalExit, 1, B_TIMEOUT, 1000000LL) == B_TIMED_OUT)
		{
		//	Must be reset at each iteration (ex-bug).
		numberDeleted = 0 ;
		tls_entries_mutex_p () ;
			{
			for (i=0 ; i<gm_entries_count ; i++)
				{
				pEntry = gm_sorted[i] ;
				//	If not used, continue...
				if (pEntry->sign == SIGN_FREE) continue ;
				//	Get thread info. If impossible... the thread should be down.
				if (B_NO_ERROR != get_thread_info (pEntry->ownerThread, &info))
					{
					//	As the entry is to be deleted, call the TLS exit procs.
					tls_call_entry_exit (pEntry->ownerThread, PROC_EXIT, pEntry) ;
					//	Simply note it as free.
					pEntry->sign = SIGN_FREE ;
					numberDeleted++ ;
					}
				}
			//	Update globals.
			if (numberDeleted != 0)
				{
				//	Sort-it so deleted be moved at the end of the sorted array.
				assert (gm_sorted != NULL) ;
				assert (gm_entries_count >= 0) ;
				qsort (gm_sorted, gm_entries_count, sizeof (_tls_thread_entry_t*), 
						qsort_comp_proc) ;
				//	And decrement the number of elements.
				gm_entries_count -= numberDeleted ;
				}
			}
		tls_entries_mutex_v () ;
		}
	return 0 ;
	}

#endif // __BEOS__
