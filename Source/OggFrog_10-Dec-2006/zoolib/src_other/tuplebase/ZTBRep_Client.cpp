static const char rcsid[] = "@(#) $Id: ZTBRep_Client.cpp,v 1.19 2006/11/17 19:11:12 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
http://www.zoolib.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------ */

#include "ZTBRep_Client.h"

#include "ZLog.h"
#include "ZThreadSimple.h"
#include "ZTupleIndex.h"
#include "ZUtil_STL.h" // For sSortedEraseMustContain etc
using namespace ZUtil_STL;

#include "ZUtil_Tuple.h"

using std::deque;
using std::map;
using std::min;
using std::pair;
using std::set;
using std::string;
using std::vector;

#define kDebug_TBRep_Client 1

#define kDebug_ShowCommsTuples 0

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client::TransTuple

class ZTBRep_Client::TransTuple
	{
public:
	TransTuple(uint64 iID);

	uint64 fID;
	ZTuple fValue;

	bool fRequestSent;
	bool fValueReturned;

	bool fWritten;
	bool fWriteSent;

	ZTBRepTransaction::Callback_GetTuple_t fCallback_GetTuple;
	void* fRefcon;
	};

ZTBRep_Client::TransTuple::TransTuple(uint64 iID)
	{
	fID = iID;

	fRequestSent = false;
	fValueReturned = false;

	fWritten = false;
	fWriteSent = false;

	fCallback_GetTuple = nil;
	fRefcon = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client::TransSearch_t

class ZTBRep_Client::TransSearch_t
	{
public:
	Transaction* fTransaction;
	ZTBRepTransaction::Callback_Search_t fCallback;
	void* fRefcon;
	ZTuple fQueryAsTuple;
	vector<uint64> fResults;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client::TransCount_t

class ZTBRep_Client::TransCount_t
	{
public:
	Transaction* fTransaction;
	ZTBRepTransaction::Callback_Count_t fCallback;
	void* fRefcon;
	ZTuple fQueryAsTuple;
	size_t fResult;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client::Transaction

class ZTBRep_Client::Transaction : public ZTBRepTransaction
	{
public:
	Transaction(ZRef<ZTBRep_Client> iTBRep);
	~Transaction();

	virtual ZRef<ZTBRep> GetTBRep();

	virtual void Search(const ZTBQuery& iQuery, Callback_Search_t iCallback, void* iRefcon);

	virtual void Count(const ZTBQuery& iQuery, Callback_Count_t iCallback, void* iRefcon);

	virtual void GetTuples(size_t iCount, const uint64* iIDs,
					Callback_GetTuple_t iCallback, void* iRefcon);

	virtual void SetTuple(uint64 iID, const ZTuple& iTuple);

	virtual void Abort();

	virtual void Validate(Callback_Validate_t iCallback_Validate, void* iRefcon);

	virtual void AcceptFailure();

	virtual void Commit(Callback_Commit_t iCallback_Commit, void* iRefcon);

private:
	ZRef<ZTBRep_Client> fTBRep;
	uint64 fServerID;

	map<uint64, TransTuple*> fTransTuples;

	set<TransTuple*> fTransTuples_NeedWork;

	vector<TransSearch_t*> fSearches_Unsent;
	set<TransSearch_t*> fSearches_Waiting;

	vector<TransCount_t*> fCounts_Unsent;
	set<TransCount_t*> fCounts_Waiting;

	Callback_Validate_t fCallback_Validate;
	Callback_Commit_t fCallback_Commit;
	void* fRefcon;

	friend class ZTBRep_Client;
	friend class TransTuple;
	};

ZTBRep_Client::Transaction::Transaction(ZRef<ZTBRep_Client> iTBRep)
:	fTBRep(iTBRep),
	fServerID(0)
	{}

ZTBRep_Client::Transaction::~Transaction()
	{
	for (map<uint64, TransTuple*>::iterator i = fTransTuples.begin(); i != fTransTuples.end(); ++i)
		delete (*i).second;
	}

ZRef<ZTBRep> ZTBRep_Client::Transaction::GetTBRep()
	{ return fTBRep; }

void ZTBRep_Client::Transaction::Search(const ZTBQuery& iQuery, Callback_Search_t iCallback, void* iRefcon)
	{ fTBRep->Trans_Search(this, iQuery, iCallback, iRefcon); }

void ZTBRep_Client::Transaction::Count(const ZTBQuery& iQuery, Callback_Count_t iCallback, void* iRefcon)
	{ fTBRep->Trans_Count(this, iQuery, iCallback, iRefcon); }

void ZTBRep_Client::Transaction::GetTuples(size_t iCount, const uint64* iIDs,
					Callback_GetTuple_t iCallback, void* iRefcon)
	{ fTBRep->Trans_GetTuples(this, iCount, iIDs, iCallback, iRefcon); }

void ZTBRep_Client::Transaction::SetTuple(uint64 iID, const ZTuple& iTuple)
	{ fTBRep->Trans_SetTuple(this, iID, iTuple); }

void ZTBRep_Client::Transaction::Abort()
	{ fTBRep->Trans_Abort(this); }

void ZTBRep_Client::Transaction::Validate(Callback_Validate_t iCallback_Validate, void* iRefcon)
	{ fTBRep->Trans_Validate(this, iCallback_Validate, iRefcon); }

void ZTBRep_Client::Transaction::AcceptFailure()
	{ fTBRep->Trans_AcceptFailure(this); }

void ZTBRep_Client::Transaction::Commit(Callback_Commit_t iCallback_Commit, void* iRefcon)
	{ fTBRep->Trans_Commit(this, iCallback_Commit, iRefcon); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTBRep_Client

ZTBRep_Client::ZTBRep_Client()
:	fPingRequested(false),
	fPingSent(false),
	fRunConnection(false),
	fWriter_Started(false),
	fWriter_Finished(false),
	fReader_Started(false),
	fReader_Finished(false),
	fIDsNeeded(0)
	{}

ZTBRep_Client::~ZTBRep_Client()
	{
	fMutex_Structure.Acquire();
	ZAssertStop(kDebug_TBRep_Client, !fRunConnection);
	ZAssertStop(kDebug_TBRep_Client, !fWriter_Started);
	ZAssertStop(kDebug_TBRep_Client, !fWriter_Finished);
	ZAssertStop(kDebug_TBRep_Client, !fReader_Started);
	ZAssertStop(kDebug_TBRep_Client, !fReader_Finished);
	}

void ZTBRep_Client::Finalize()
	{
	// Shut down the connections.
	this->FinalizationComplete();
	}

void ZTBRep_Client::AllocateIDs(size_t iCount,
				Callback_AllocateIDs_t iCallback, void* iRefcon)
	{
	ZMutexLocker lock(fMutex_Structure);
	while (iCount)
		{
		if (fIDs.empty())
			break;
		uint64 baseID = fIDs.back().first;
		size_t countToIssue = iCount;
		if (countToIssue < fIDs.back().second)
			{
			fIDs.back().first += countToIssue;
			fIDs.back().second -= countToIssue;
			}
		else
			{
			countToIssue = fIDs.back().second;
			fIDs.pop_back();
			}
		iCount -= countToIssue;
		iCallback(iRefcon, baseID, countToIssue);
		}

	if (iCount)
		{
		AllocateIDRequest_t theRequest;
		theRequest.fCount = iCount;
		theRequest.fCallback = iCallback;
		theRequest.fRefcon = iRefcon;
		fAllocateIDPending.push_back(theRequest);
		fIDsNeeded += iCount;
		fCondition_Sender.Broadcast();
		}
	}

ZTBRepTransaction* ZTBRep_Client::CreateTransaction()
	{
	ZMutexLocker lock(fMutex_Structure);

	Transaction* theTransaction = new Transaction(this);
	sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Create_Unsent, theTransaction);

	fCondition_Sender.Broadcast();

	while (!theTransaction->fServerID)
		fCondition_Transaction.Wait(fMutex_Structure);

	return theTransaction;
	}

void ZTBRep_Client::Run()
	{
	ZMutexLocker locker(fMutex_Structure);

#if 0
	ZAssertStop(kDebug_TBRep_Client, !fRunConnection);
	ZAssertStop(kDebug_TBRep_Client, !fWriter_Started);
	ZAssertStop(kDebug_TBRep_Client, !fWriter_Finished);
	ZAssertStop(kDebug_TBRep_Client, !fReader_Started);
	ZAssertStop(kDebug_TBRep_Client, !fReader_Finished);
#endif

	ZAssertStop(kDebug_TBRep_Client, !fStreamer);
	fStreamer = this->EstablishConnection();

	if (!fStreamer)
		return;
	fRunConnection = true;

	(new ZThreadSimple<ZTBRep_Client*>(sRunWriter, this))->Start();
	(new ZThreadSimple<ZTBRep_Client*>(sRunReader, this))->Start();

	while (!fWriter_Started && !fReader_Started)
		fCondition_RW.Wait(fMutex_Structure);

	for (;;)
		{
		if (fWriter_Finished || fReader_Finished)
			{
			if (fWriter_Finished && fReader_Finished)
				break;
			fRunConnection = false;
			fCondition_RW.Broadcast();
			fCondition_Sender.Broadcast();
			// How to wake the reader?
			}
		else
			{
			fCondition_RW.Wait(fMutex_Structure);
			}
		}

	fStreamer.Clear();

	fWriter_Started = false;
	fWriter_Finished = false;
	fReader_Started = false;
	fReader_Finished = false;
	}

void ZTBRep_Client::Trans_Search(Transaction* iTransaction, const ZTBQuery& iQuery,
				ZTBRepTransaction::Callback_Search_t iCallback, void* iRefcon)
	{
	ZMutexLocker locker(fMutex_Structure);
	TransSearch_t* theTransSearch = new TransSearch_t;
	theTransSearch->fTransaction = iTransaction;
	theTransSearch->fRefcon = iRefcon;
	theTransSearch->fCallback = iCallback;
	theTransSearch->fQueryAsTuple = iQuery.AsTuple();
	iTransaction->fSearches_Unsent.push_back(theTransSearch);
	fTransactions_NeedWork.insert(iTransaction);
	fCondition_Sender.Broadcast();
	}

void ZTBRep_Client::Trans_Count(Transaction* iTransaction, const ZTBQuery& iQuery,
				ZTBRepTransaction::Callback_Count_t iCallback, void* iRefcon)
	{
	ZMutexLocker locker(fMutex_Structure);
	TransCount_t* theTransCount = new TransCount_t;
	theTransCount->fTransaction = iTransaction;
	theTransCount->fRefcon = iRefcon;
	theTransCount->fCallback = iCallback;
	theTransCount->fQueryAsTuple = iQuery.AsTuple();
	iTransaction->fCounts_Unsent.push_back(theTransCount);
	fTransactions_NeedWork.insert(iTransaction);
	fCondition_Sender.Broadcast();	
	}

void ZTBRep_Client::Trans_GetTuples(Transaction* iTransaction, size_t iCount, const uint64* iIDs,
					ZTBRepTransaction::Callback_GetTuple_t iCallback, void* iRefcon)
	{
	ZMutexLocker locker(fMutex_Structure);
	while (iCount--)
		{
		uint64 theID = *iIDs++;
		TransTuple* theTransTuple = this->Internal_GetTransTuple(iTransaction, theID);
		if (theTransTuple->fWritten)
			{
			// It's been written to, so we know its value.
			locker.Release();
			if (iCallback)
				iCallback(iRefcon, 1, &theID, &theTransTuple->fValue);
			}
		else
			{
			if (theTransTuple->fValueReturned)
				{
				locker.Release();
				if (iCallback)
					iCallback(iRefcon, 1, &theID, &theTransTuple->fValue);
				}
			else
				{
				if (theTransTuple->fRequestSent)
					{
					if (iCallback)
						{
						ZAssertStop(kDebug_TBRep_Client, !theTransTuple->fCallback_GetTuple);
						theTransTuple->fCallback_GetTuple = iCallback;
						theTransTuple->fRefcon = iRefcon;
						}
					}
				else
					{
					ZAssertStop(kDebug_TBRep_Client, !theTransTuple->fCallback_GetTuple);
					theTransTuple->fCallback_GetTuple = iCallback;
					theTransTuple->fRefcon = iRefcon;
					iTransaction->fTransTuples_NeedWork.insert(theTransTuple);
					fTransactions_NeedWork.insert(iTransaction);
					fCondition_Sender.Broadcast();
					}
				}
			}
		}
	}

void ZTBRep_Client::Trans_SetTuple(Transaction* iTransaction, uint64 iID, const ZTuple& iTuple)
	{
	ZMutexLocker locker(fMutex_Structure);
	TransTuple* theTransTuple = this->Internal_GetTransTuple(iTransaction, iID);
	theTransTuple->fWritten = true;
	theTransTuple->fWriteSent = false;
	theTransTuple->fValue = iTuple;
	iTransaction->fTransTuples_NeedWork.insert(theTransTuple);
	fTransactions_NeedWork.insert(iTransaction);
	fCondition_Sender.Broadcast();
	}

void ZTBRep_Client::Trans_Abort(Transaction* iTransaction)
	{
	ZMutexLocker lock(fMutex_Structure);

	// Wait till a pending create has finished.
	while (!iTransaction->fServerID)
		fCondition_Transaction.Wait(fMutex_Structure);

	// iTransaction must either be created or validated.
	if (!sSortedEraseIfContains(fTransactions_Created, iTransaction))
		{
		if (!sSortedEraseIfContains(fTransactions_Validated, iTransaction))
			{
			ZDebugLogf(kDebug_TBRep_Client, ("Aborting a transaction that's not created or validated"));
			return;
			}
		}

	sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Aborted_Unsent, iTransaction);
	fCondition_Sender.Broadcast();
	}

void ZTBRep_Client::Trans_Validate(Transaction* iTransaction,
				ZTBRepTransaction::Callback_Validate_t iCallback_Validate, void* iRefcon)
	{
	ZMutexLocker lock(fMutex_Structure);

	sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Created, iTransaction);
	sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Validate_Unsent, iTransaction);

	iTransaction->fCallback_Validate = iCallback_Validate;
	iTransaction->fRefcon = iRefcon;

	fCondition_Sender.Broadcast();
	}

void ZTBRep_Client::Trans_AcceptFailure(Transaction* iTransaction)
	{
	ZMutexLocker lock(fMutex_Structure);
	sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Failed, iTransaction);
	delete iTransaction;
	}

void ZTBRep_Client::Trans_Commit(Transaction* iTransaction,
				ZTBRepTransaction::Callback_Commit_t iCallback_Commit, void* iRefcon)
	{
	ZMutexLocker lock(fMutex_Structure);

	sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Validated, iTransaction);
	sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Commit_Unsent, iTransaction);

	iTransaction->fCallback_Commit = iCallback_Commit;
	iTransaction->fRefcon = iRefcon;
	fCondition_Sender.Broadcast();
	}

ZTBRep_Client::TransTuple* ZTBRep_Client::Internal_GetTransTuple(Transaction* iTransaction, uint64 iID)
	{
	map<uint64, TransTuple*>::iterator iter = iTransaction->fTransTuples.find(iID);
	if (iter != iTransaction->fTransTuples.end())
		return (*iter).second;

	TransTuple* newTransTuple = new TransTuple(iID);
	
	iTransaction->fTransTuples.insert(map<uint64, TransTuple*>::value_type(iID, newTransTuple));

	return newTransTuple;
	}

// =================================================================================================

static void sSend(ZMutexLocker& iLocker, const ZStreamW& iStream, const ZTuple& iTuple)
	{
	iLocker.Release();

	if (kDebug_ShowCommsTuples)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTBRep_Client"))
			s << "<< " << iTuple;
		}

	iTuple.ToStream(iStream);
	iLocker.Acquire();
	}

void ZTBRep_Client::Writer(const ZStreamW& iStream)
	{
	ZMutexLocker locker(fMutex_Structure);
	bool didAnything = false;

	if (fPingRequested)
		{
		fPingRequested = false;
		ZTuple response;
		response.SetString("What", "Pong");
		sSend(locker, iStream, response);
		didAnything = true;
		}

	if (!fPingSent && fTime_LastRead + 10.0 < ZTime::sSystem())
		{
		fPingSent = true;
		ZTuple req;
		req.SetString("What", "Ping");
		sSend(locker, iStream, req);
		didAnything = true;
		}

	if (fIDsNeeded)
		{
		ZTuple reqTuple;
		reqTuple.SetString("What", "AllocateIDs");
		reqTuple.SetInt32("Count", fIDsNeeded);
		fIDsNeeded = 0;
		sSend(locker, iStream, reqTuple);
		didAnything = true;
		}

	if (!fTransactions_Create_Unsent.empty())
		{
		ZTuple reqTuple;
		reqTuple.SetString("What", "Create");

		vector<ZTupleValue>& vectorTransactionIDs = reqTuple.SetMutableVector("ClientIDs");
		for (vector<Transaction*>::iterator i = fTransactions_Create_Unsent.begin();
			i != fTransactions_Create_Unsent.end(); ++i)
			{
			Transaction* theTransaction = *i;
			vectorTransactionIDs.push_back(reinterpret_cast<int64>(theTransaction));
			sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Create_Unacked, theTransaction);
			}
		fTransactions_Create_Unsent.clear();

		sSend(locker, iStream, reqTuple);
		didAnything = true;
		}
	
	if (!fTransactions_Aborted_Unsent.empty())
		{
		ZTuple reqTuple;
		reqTuple.SetString("What", "Abort");

		vector<ZTupleValue>& vectorServerIDs = reqTuple.SetMutableVector("ServerIDs");
		for (vector<Transaction*>::iterator i = fTransactions_Aborted_Unsent.begin();
			i != fTransactions_Aborted_Unsent.end(); ++i)
			{
			Transaction* theTransaction = *i;
			vectorServerIDs.push_back(int64(theTransaction->fServerID));
			delete theTransaction;
			}
		fTransactions_Aborted_Unsent.clear();

		sSend(locker, iStream, reqTuple);
		didAnything = true;
		}

	if (!fTransactions_Validate_Unsent.empty())
		{
		if (fTransactions_NeedWork.empty())
			{
			// We don't send the validate till we've disposed of pending work.
			ZTuple reqTuple;
			reqTuple.SetString("What", "Validate");

			vector<ZTupleValue>& vectorServerIDs = reqTuple.SetMutableVector("ServerIDs");
			for (vector<Transaction*>::iterator i = fTransactions_Validate_Unsent.begin();
				i != fTransactions_Validate_Unsent.end(); ++i)
				{
				Transaction* theTransaction = *i;
				vectorServerIDs.push_back(int64(theTransaction->fServerID));
				sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Validate_Unacked, theTransaction);
				}
			fTransactions_Validate_Unsent.clear();

			sSend(locker, iStream, reqTuple);
			}
		didAnything = true;
		}

	if (!fTransactions_Commit_Unsent.empty())
		{
		ZTuple reqTuple;
		reqTuple.SetString("What", "Commit");

		vector<ZTupleValue>& vectorServerIDs = reqTuple.SetMutableVector("ServerIDs");
		for (vector<Transaction*>::iterator i = fTransactions_Commit_Unsent.begin();
			i != fTransactions_Commit_Unsent.end(); ++i)
			{
			Transaction* theTransaction = *i;
			vectorServerIDs.push_back(int64(theTransaction->fServerID));
			sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Commit_Unacked, theTransaction);
			}
		fTransactions_Commit_Unsent.clear();

		sSend(locker, iStream, reqTuple);
		didAnything = true;
		}

	while (!fTransactions_NeedWork.empty())
		{
		Transaction* theTransaction = *fTransactions_NeedWork.begin();
		fTransactions_NeedWork.erase(fTransactions_NeedWork.begin());

		if (!theTransaction->fTransTuples_NeedWork.empty())
			{
			ZTuple reqTuple;
			reqTuple.SetString("What", "Actions");
			reqTuple.SetInt64("ServerID", theTransaction->fServerID);

			// SetMutableVector is unsafe if further changes are
			// to be made to the hosting tuple. So we use SetMutableVector
			// for Gets to create an empty vectors. Then
			// we use SetMutableVector for Writes, then retrieve the safe
			// references for for Gets.
			reqTuple.SetMutableVector("Gets");
			vector<ZTupleValue>& vectorWrites = reqTuple.SetMutableVector("Writes");
			vector<ZTupleValue>& vectorGets = reqTuple.GetMutableVector("Gets");

			for (set<TransTuple*>::iterator i = theTransaction->fTransTuples_NeedWork.begin();
				i != theTransaction->fTransTuples_NeedWork.end(); ++i)
				{
				TransTuple* theTT = *i;
				if (theTT->fWritten)
					{
					if (!theTT->fWriteSent)
						{
						theTT->fWriteSent = true;
						// tell the server about the write.
						ZTuple theWriteTuple;
						theWriteTuple.SetID("ID", theTT->fID);
						theWriteTuple.SetTuple("Value", theTT->fValue);
						vectorWrites.push_back(theWriteTuple);
						}
					}
				else
					{
					if (!theTT->fRequestSent)
						{
						theTT->fRequestSent = true;
						// Ask the server to send the tuple.
						vectorGets.push_back(ZTupleValue(theTT->fID, true));
						}
					}
				}

			theTransaction->fTransTuples_NeedWork.clear();
			sSend(locker, iStream, reqTuple);
			didAnything = true;
			}

		if (!theTransaction->fSearches_Unsent.empty())
			{
			ZTuple reqTuple;
			reqTuple.SetString("What", "Search");
			reqTuple.SetInt64("ServerID", theTransaction->fServerID);

			vector<ZTupleValue>& vectorSearches = reqTuple.SetMutableVector("Searches");
			for (vector<TransSearch_t*>::iterator i = theTransaction->fSearches_Unsent.begin();
				i != theTransaction->fSearches_Unsent.end(); ++i)
				{
				TransSearch_t* theSearch = *i;
				ZTuple theTuple;
				theTuple.SetInt64("SearchID", reinterpret_cast<int64>(theSearch));
				theTuple.SetTuple("QueryAsTuple", theSearch->fQueryAsTuple);
				vectorSearches.push_back(theTuple);
				theTransaction->fSearches_Waiting.insert(theSearch);
				}
			theTransaction->fSearches_Unsent.clear();
			sSend(locker, iStream, reqTuple);
			didAnything = true;
			}

		if (!theTransaction->fCounts_Unsent.empty())
			{
			ZTuple reqTuple;
			reqTuple.SetString("What", "Count");
			reqTuple.SetInt64("ServerID", theTransaction->fServerID);

			vector<ZTupleValue>& vectorCounts = reqTuple.SetMutableVector("Counts");
			for (vector<TransCount_t*>::iterator i = theTransaction->fCounts_Unsent.begin();
				i != theTransaction->fCounts_Unsent.end(); ++i)
				{
				TransCount_t* theCount = *i;
				ZTuple theTuple;
				theTuple.SetInt64("CountID", reinterpret_cast<int64>(theCount));
				theTuple.SetTuple("QueryAsTuple", theCount->fQueryAsTuple);
				vectorCounts.push_back(theTuple);
				theTransaction->fCounts_Waiting.insert(theCount);
				}
			theTransaction->fCounts_Unsent.clear();
			sSend(locker, iStream, reqTuple);
			didAnything = true;
			}
		}

	if (!didAnything)
		{
		iStream.Flush();
		// We'll wait for no more than two seconds before going around again, thus
		// giving us a chance to see if the reader's gone dead.
		fCondition_Sender.Wait(fMutex_Structure, 2 * 1000000);
		}
	}

void ZTBRep_Client::RunWriter()
	{
	ZMutexLocker locker(fMutex_Structure);
	fWriter_Started = true;
	fCondition_RW.Broadcast();
	if (fStreamer)
		{
		const ZStreamW& theStream = fStreamer->GetStreamW();
		while (fRunConnection)
			{
			locker.Release();
			try
				{
				this->Writer(theStream);
				}
			catch (...)
				{}
			locker.Acquire();
			}
		}
	fWriter_Finished = true;
	fCondition_RW.Broadcast();
	}

void ZTBRep_Client::sRunWriter(ZTBRep_Client* iTBRep)
	{ iTBRep->RunWriter(); }


static bool abortIt = false;
void ZTBRep_Client::Reader(const ZStreamR& iStream)
	{
	ZTuple theResp(iStream);

	if (abortIt)
		throw ("sefsdfsdfsfssdf");
	
	ZMutexLocker locker(fMutex_Structure);
	fTime_LastRead = ZTime::sSystem();

	string theWhat = theResp.GetString("What");

	if (kDebug_ShowCommsTuples)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTBRep_Client"))
			s << ">> " << theResp;
		}

	if (theWhat == "Ping")
		{
		fPingRequested = true;
		fCondition_Sender.Broadcast();
		}
	else if (theWhat == "Pong")
		{
		fPingSent = false;
		}
	else if (theWhat == "AllocateIDs_Ack")
		{
		uint64 baseID = theResp.GetInt64("BaseID");
		size_t count = theResp.GetInt32("Count");
		// Distribute them to waiting requests.
		while (count && !fAllocateIDPending.empty())
			{
			size_t countToIssue = min(count, fAllocateIDPending.front().fCount);
			fAllocateIDPending.front().fCallback(fAllocateIDPending.front().fRefcon, baseID, countToIssue);
			baseID += countToIssue;
			count -= countToIssue;
			fAllocateIDPending.front().fCount -= countToIssue;
			if (fAllocateIDPending.front().fCount == 0)
				fAllocateIDPending.pop_front();
			}
		if (count)
			fIDs.push_back(pair<uint64, size_t>(baseID, count));
		}
	else if (theWhat == "Create_Ack")
		{
		const vector<ZTupleValue>& vectorIDs = theResp.GetVector("IDs");
		for (vector<ZTupleValue>::const_iterator i = vectorIDs.begin(); i != vectorIDs.end(); ++i)
			{
			ZTuple theTuple = (*i).GetTuple();
			Transaction* theTransaction = reinterpret_cast<Transaction*>(theTuple.GetInt64("ClientID"));

			sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Create_Unacked, theTransaction);
			sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Created, theTransaction);
	
			ZAssertStop(kDebug_TBRep_Client, theTransaction->fServerID == 0);
	
			theTransaction->fServerID = theTuple.GetInt64("ServerID");
			fCondition_Transaction.Broadcast();
			}
		}
	else if (theWhat == "Validate_Succeeded")
		{
		const vector<ZTupleValue>& vectorClientIDs = theResp.GetVector("ClientIDs");
		for (vector<ZTupleValue>::const_iterator i = vectorClientIDs.begin(); i != vectorClientIDs.end(); ++i)
			{
			Transaction* theTransaction = reinterpret_cast<Transaction*>((*i).GetInt64());

			sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Validate_Unacked, theTransaction);
			sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Validated, theTransaction);

			if (theTransaction->fCallback_Validate)
				theTransaction->fCallback_Validate(true, theTransaction->fRefcon);
			}
		}
	else if (theWhat == "Validate_Failed")
		{
		const vector<ZTupleValue>& vectorClientIDs = theResp.GetVector("ClientIDs");
		for (vector<ZTupleValue>::const_iterator i = vectorClientIDs.begin(); i != vectorClientIDs.end(); ++i)
			{
			Transaction* theTransaction = reinterpret_cast<Transaction*>((*i).GetInt64());

			sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Validate_Unacked, theTransaction);
			sSortedInsertMustNotContain(kDebug_TBRep_Client, fTransactions_Failed, theTransaction);

			if (theTransaction->fCallback_Validate)
				theTransaction->fCallback_Validate(false, theTransaction->fRefcon);
			}
		}
	else if (theWhat == "Commit_Ack")
		{
		const vector<ZTupleValue>& vectorClientIDs = theResp.GetVector("ClientIDs");
		for (vector<ZTupleValue>::const_iterator i = vectorClientIDs.begin(); i != vectorClientIDs.end(); ++i)
			{
			Transaction* theTransaction = reinterpret_cast<Transaction*>((*i).GetInt64());

			sSortedEraseMustContain(kDebug_TBRep_Client, fTransactions_Commit_Unacked, theTransaction);

			if (theTransaction->fCallback_Commit)
				theTransaction->fCallback_Commit(theTransaction->fRefcon);
			delete theTransaction;
			}
		}
	else if (theWhat == "GetTuples_Ack")
		{
		const vector<ZTupleValue>& vectorTransactions = theResp.GetVector("Transactions");
		for (vector<ZTupleValue>::const_iterator i = vectorTransactions.begin(); i != vectorTransactions.end(); ++i)
			{
			ZTuple theTransactionTuple = (*i).GetTuple();
			Transaction* theTransaction = reinterpret_cast<Transaction*>(theTransactionTuple.GetInt64("ClientID"));

			const vector<ZTupleValue>& vectorIDTuples = theTransactionTuple.GetVector("IDValues");
			for (vector<ZTupleValue>::const_iterator i = vectorIDTuples.begin(); i != vectorIDTuples.end(); ++i)
				{
				ZTuple theTuple = (*i).GetTuple();
				uint64 theID = theTuple.GetID("ID");
				TransTuple* theTransTuple = this->Internal_GetTransTuple(theTransaction, theID);
				if (!theTransTuple->fWritten)
					{
					theTransTuple->fValueReturned = true;
					theTransTuple->fValue = theTuple.GetTuple("Value");
					if (theTransTuple->fCallback_GetTuple)
						{
						ZTBRepTransaction::Callback_GetTuple_t theCallback = theTransTuple->fCallback_GetTuple;
						void* theRefcon = theTransTuple->fRefcon;
						theTransTuple->fCallback_GetTuple = nil;
						theTransTuple->fRefcon = nil;
						ZTuple theValue = theTransTuple->fValue;
						theCallback(theRefcon, 1, &theID, &theValue);
						}
					}
				}
			}
		}
	else if (theWhat == "Search_Ack")
		{
		const vector<ZTupleValue>& vectorSearches = theResp.GetVector("Searches");
		for (vector<ZTupleValue>::const_iterator i = vectorSearches.begin();
			i != vectorSearches.end(); ++i)
			{
			ZTuple theSearchTuple = (*i).GetTuple();
			TransSearch_t* theSearch = reinterpret_cast<TransSearch_t*>(theSearchTuple.GetInt64("SearchID"));
			Transaction* theTransaction = theSearch->fTransaction;
			theTransaction->fSearches_Waiting.erase(theSearch);
			
			vector<uint64> vectorResultIDs;
			theSearchTuple.GetVector_T("Results", back_inserter(vectorResultIDs), uint64());

			theSearch->fCallback(theSearch->fRefcon, vectorResultIDs);
			delete theSearch;
			}
		}
	else if (theWhat == "Count_Ack")
		{
		const vector<ZTupleValue>& vectorCounts = theResp.GetVector("Counts");
		for (vector<ZTupleValue>::const_iterator i = vectorCounts.begin();
			i != vectorCounts.end(); ++i)
			{
			ZTuple theCountTuple = (*i).GetTuple();
			TransCount_t* theCount = reinterpret_cast<TransCount_t*>(theCountTuple.GetInt64("CountID"));
			Transaction* theTransaction = theCount->fTransaction;
			theTransaction->fCounts_Waiting.erase(theCount);
			
			theCount->fCallback(theCount->fRefcon, theCountTuple.GetInt64("Result"));
			delete theCount;
			}
		}
	else
		{
		printf("Unrecognized response\n");
		}
	}

void ZTBRep_Client::RunReader()
	{
	ZMutexLocker locker(fMutex_Structure);
	fReader_Started = true;
	fCondition_RW.Broadcast();
	if (fStreamer)
		{
		const ZStreamR& theStream = fStreamer->GetStreamR();
		while (fRunConnection)
			{
			locker.Release();
			try
				{
				this->Reader(theStream);
				}
			catch (...)
				{}
			locker.Acquire();
			}
		}
	fReader_Finished = true;
	fCondition_RW.Broadcast();
	}

void ZTBRep_Client::sRunReader(ZTBRep_Client* iTBRep)
	{ iTBRep->RunReader(); }
