#if __BEOS__
/*	================================================================================
	Project		: TLS
	File		: TLSdata.c
	Copyright	: 1997 - Laurent POINTAL
	Distribution: Freeware, source available
	--------------------------------------------------------------------------------
	Management of data handling for TLS.
	================================================================================	*/

#include <KernelKit.h>
// MDC added these two
#include <SupportDefs.h>
#include <Errors.h>
#include <assert.h>
#pragma lib_export on
#include "TLS.h"
#pragma lib_export reset
#include "TLSpriv.h"



/*	===== tls_alloc ================================================================
	PUBLIC. Allocate a new TLS index.
	================================================================================	*/
status_t tls_alloc (int* pIndex)
	{
	int index ;
	status_t status;
	if (!pIndex) return B_BAD_VALUE ;

	//	Check parameter.
	assert (pIndex != NULL) ;
	if (pIndex == NULL) return B_BAD_VALUE ;

	//	Get a new index (using mutual exclusion).
	if ((status = tls_index_mutex_p ()) == B_NO_ERROR)
		{
		status = tls_create_index (&index) ;
		tls_index_mutex_v () ;
		}
	if (status != B_NO_ERROR) 
		{
		//	Set to an invalid index (ie: use without error checking will 
		//	resolve to errors).
		*pIndex = -1 ;
		return status ;
		}

	//	Finish.
	*pIndex = index ;
	return B_NO_ERROR ;
	}


/*	===== tls_free =================================================================
	PUBLIC. Free TLS index.
	================================================================================	*/
status_t tls_free (int index)
	{
	status_t status ;

	//	Check parameter.
	assert (index >= 0) ;
	if (index < 0) return B_BAD_INDEX ;

	//	Request to free the index (using mutual exclusion).
	if ((status = tls_index_mutex_p ()) == B_NO_ERROR)
		{
		if (tls_check_index (index))
			{
			tls_delete_index (index) ;
			}
		else
			status = B_BAD_INDEX ;
		tls_index_mutex_v () ;
		}

	//	Finish.
	return status ;
	}


/*	===== tls_set ==================================================================
	PUBLIC. Set a TLS data at an index.
	================================================================================	*/
status_t tls_set (int index, tls_data_t data)
	{
	int indexOk ;
	_tls_thread_entry_t* e = NULL ;
	thread_id id ;
	status_t status ;

	//	Check parameter.
	assert (index >= 0) ;
	if (index < 0) return B_BAD_VALUE ;

	//	Check index value (verify index has been allocated).
	if ((status = tls_index_mutex_p ()) == B_NO_ERROR)
		{
		indexOk = tls_check_index (index) ;
		tls_index_mutex_v () ;
		}
	else
		return status ;
	if (!indexOk) return B_BAD_INDEX ;

	//	Get the entry for the thread.
	id = find_thread (NULL) ;
	if ((status = tls_entries_mutex_p ()) == B_NO_ERROR)
		{
		if ((status = tls_get_entry (id, &e)) != B_NO_ERROR)
			status = tls_create_entry (id, &e) ;
		tls_entries_mutex_v () ;
		}
	
	//	Set it using the set_inproc function.
	if (status == B_NO_ERROR)
		return tls_set_inproc (index, data, e) ;
	else
		return status ;
	}

/*	===== tls_get ==================================================================
	PUBLIC. Get a TLS data at an index.
	================================================================================	*/
status_t tls_get (int index, tls_data_t* pData) 
	{
	int indexOk ;
	_tls_thread_entry_t* e = NULL ;
	thread_id id ;
	status_t status ;

	//	Check parameter.
	assert (pData != NULL) ;
	if (pData == NULL) return B_BAD_VALUE ;
	*pData = TLS_INVALID_DATA ;
	assert (index >= 0) ;
	if (index < 0) return B_BAD_VALUE ;

	//	Check index value (verify index has been allocated).
	if ((status = tls_index_mutex_p ()) == B_NO_ERROR)
		{
		indexOk = tls_check_index (index) ;
		tls_index_mutex_v () ;
		}
	else
		return status ;
	if (!indexOk) return B_BAD_INDEX ;

	//	Get the entry for the thread.
	id = find_thread (NULL) ;
	if ((status = tls_entries_mutex_p ()) == B_NO_ERROR)
		{
		if ((status = tls_get_entry (id, &e)) != B_NO_ERROR)
			status = tls_create_entry (id, &e) ;
		tls_entries_mutex_v () ;
		}
	
	//	Get it using the get_inproc function.
	if (status == B_NO_ERROR)
		return tls_get_inproc (index, pData, e) ;
	else
		return status ;
	}


/*	===== tls_set_inproc ===========================================================
	PUBLIC. 
	Note: tls_set_inproc DONT take the mutex, because it dont access directly to
	the thread entries list.
	It is called by tls_set when the entry is founded, and by entry/exit procs
	which get the threadEnv parameter from their caller.
	================================================================================	*/
status_t tls_set_inproc (int index, tls_data_t data, void* threadEnv)
	{
	_tls_thread_entry_t* e = (_tls_thread_entry_t*) threadEnv ; 
	status_t status ;

	//	Check parameter.
	assert (index >= 0) ;
	if (index < 0) return B_BAD_INDEX ;
	assert (threadEnv != NULL) ;
	if (e == NULL) return B_BAD_VALUE ;
	assert (e->sign == SIGN_OK) ;
	if (e->sign != SIGN_OK) return B_BAD_VALUE ;

	//	Check if its necessary to expand the data array.
	if (index >= e->datasCount)
		{
		if ((status = tls_expand_values (e, gm_indexs_allocated_count)) != B_NO_ERROR) 
			return status ;
		}

	//	Finish.
	e->datas[index] = data ;
	return B_NO_ERROR ;
	}


/*	===== tls_get_inproc ===========================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_get_inproc (int index, tls_data_t* pData, void* threadEnv)
	{
	_tls_thread_entry_t* e = (_tls_thread_entry_t*) threadEnv ; 
	status_t status ;

	//	Check parameter.
	assert (index >= 0) ;
	if (index < 0) return B_BAD_INDEX ;
	assert (pData != NULL) ;
	if (pData == NULL) return B_BAD_VALUE ;
	assert (threadEnv != NULL) ;
	if (e == NULL) return B_BAD_VALUE ;
	assert (e->sign == SIGN_OK) ;
	if (e->sign != SIGN_OK) return B_BAD_VALUE ;

	//	Check if its necessary to expand the data array.
	if (index >= e->datasCount)
		{
		if ((status = tls_expand_values (e, gm_indexs_allocated_count)) != B_NO_ERROR) 
			return status ;
		}

	//	Finish.
	*pData = e->datas[index] ;
	return B_NO_ERROR ;
	}


/*	===== tls_install_entry_proc ===================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_install_entry_proc (tls_proc_t proc, int param) 
	{
	return tls_create_entry_exit (PROC_ENTRY, proc, param) ;
	}


/*	===== tls_remove_entry_proc ====================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_remove_entry_proc (tls_proc_t proc, int param) 
	{
	return tls_remove_entry_exit (PROC_ENTRY, proc, param) ;
	}


/*	===== tls_install_exit_proc ====================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_install_exit_proc (tls_proc_t proc, int param) 
	{
	return tls_create_entry_exit (PROC_EXIT, proc, param) ;
	}


/*	===== tls_remove_exit_proc =====================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_remove_exit_proc (tls_proc_t proc, int param) 
	{
	return tls_remove_entry_exit (PROC_EXIT, proc, param) ;
	}


/*	===== tls_entering_procs =======================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_entering_procs ()
	{
	thread_id id = find_thread (NULL) ;
	_tls_thread_entry_t* e = NULL ;
	status_t status ;
	bool newlyCreated = false ;

	//	Search for the thread entry.
	//	If not founded, create it and dont call entry procs because tls_create_entry
	//	automatically call them.
	//	If founded, call entry procs.
	if ((status = tls_entries_mutex_p ()) == B_NO_ERROR)
		{
		if ((status = tls_get_entry (id, &e)) != B_NO_ERROR)
			status = tls_create_entry (id, &e) ;
		else
			tls_call_entry_exit (id, PROC_ENTRY, e) ;
		tls_entries_mutex_v () ;
		}
	if (status != B_NO_ERROR) return status ;
	
	//	Finish
	return B_NO_ERROR ;
	}


/*	===== tls_exiting_procs ========================================================
	PUBLIC. 
	================================================================================	*/
status_t tls_exiting_procs () 
	{
	thread_id id = find_thread (NULL) ;
	_tls_thread_entry_t* e = NULL ;
	status_t status ;

	//	Search for the thread entry. 
	if ((status = tls_entries_mutex_p ()) == B_NO_ERROR)
		{
		status = tls_get_entry (id, &e) ;
		tls_entries_mutex_v () ;
		}

	//	If the thread has no entry, then we should not call its exit procs.
	if (status == B_NAME_NOT_FOUND) return B_NO_ERROR ;

	//	Other errors are... errors.
	if (status != B_NO_ERROR) return status ;

	//	Free the entry structure (avoid monitoring thread to have to do this at
	//	a bad time, in a bad thread). The entry deletion function will automatically 
	//	call exiting procs.
	if ((status = tls_entries_mutex_p ()) == B_NO_ERROR)
		{
		status = tls_delete_entry (e) ;
		tls_entries_mutex_v () ;
		}

	//	Finish.
	return status ;
	}


#endif // __BEOS__
