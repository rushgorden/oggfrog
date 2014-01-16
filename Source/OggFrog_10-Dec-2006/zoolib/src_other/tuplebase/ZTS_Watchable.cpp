static const char rcsid[] = "@(#) $Id: ZTS_Watchable.cpp,v 1.32 2006/11/02 22:48:15 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2005 Andrew Green and Learning in Motion, Inc.
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

#include "ZTS_Watchable.h"

#include "ZDebug.h"
#include "ZDList.h"
#include "ZLog.h"
#include "ZMemoryBlock.h"
#include "ZStream_Memory.h"
#include "ZString.h"
#include "ZTSWatcher.h"
#include "ZTupleQuisitioner.h"
#include "ZUtil_STL.h"
#include "ZUtil_Tuple.h"

using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

#define kDebug_TS_Watchable 1

#ifdef ZCONFIG_TS_Watchable_DumpStuff
#	define kDebug_DumpStuff ZCONFIG_TS_Watchable_DumpStuff
#else
#	define kDebug_DumpStuff 0
#endif

#ifdef ZCONFIG_TS_Watchable_Time
#	define kDebug_Time ZCONFIG_TS_Watchable_Time
#else
#	define kDebug_Time 0
#endif

#ifdef ZCONFIG_TS_Watchable_CountInvalidations
#	define kDebug_CountInvalidations ZCONFIG_TS_Watchable_CountInvalidations
#else
#	define kDebug_CountInvalidations 0
#endif

#define ASSERTLOCKED(a) ZAssertStop(kDebug_TS_Watchable, a.IsLocked())
#define ASSERTUNLOCKED(a) ZAssertStop(kDebug_TS_Watchable, !a.IsLocked())

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::PTuple

class ZTS_Watchable::PTuple
	{
public:
	PTuple(uint64 iID) : fID(iID) {}

	const uint64 fID;

	set<Watcher*> fUsingWatchers;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::PSpec

class ZTS_Watchable::PSpec
	{
public:
	PSpec(const ZTBSpec& iTBSpec)
	:	fTBSpec(iTBSpec)
		{ fTBSpec.GetPropNames(fPropNames); }

	const ZTBSpec& GetTBSpec() { return fTBSpec; }

	const set<ZTuplePropName>& GetPropNames() { return fPropNames; }

	ZTBSpec const fTBSpec;

	set<PQuery*> fUsingPQueries;
	set<ZTuplePropName> fPropNames;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::PQuery

class ZTS_Watchable::PQuery
	{
public:
	PQuery(const ZMemoryBlock& iMB, const ZTBQuery& iTBQuery)
	:	fMB(iMB), fTBQuery(iTBQuery), fResultsValid(false) {}

	ZMemoryBlock const fMB;
	ZTBQuery const fTBQuery;
	set<PSpec*> fPSpecs;

	bool fResultsValid;
	vector<uint64> fResults;

	ZooLib::DListHead<WatcherQueryUsing> fUsingWatcherQueries;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::WatcherQuery

class ZTS_Watchable::WatcherQueryTripped :
	public ZooLib::DListLink<ZTS_Watchable::WatcherQuery, ZTS_Watchable::WatcherQueryTripped>
	{};

class ZTS_Watchable::WatcherQueryUsing :
	public ZooLib::DListLink<ZTS_Watchable::WatcherQuery, ZTS_Watchable::WatcherQueryUsing>
	{};

class ZTS_Watchable::WatcherQuery :
	public ZTS_Watchable::WatcherQueryTripped,
	public ZTS_Watchable::WatcherQueryUsing
	{
public:
	WatcherQuery(Watcher* iWatcher, int64 iRefcon, PQuery* iPQuery, bool iPrefetch)
	:	fWatcher(iWatcher),
		fRefcon(iRefcon),
		fPQuery(iPQuery),
		fPrefetch(iPrefetch)
		{}

	Watcher* const fWatcher;
	int64 const fRefcon;
	PQuery* const fPQuery;
	bool const fPrefetch;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::Watcher

class ZTS_Watchable::Watcher : public ZTSWatcher
	{
public:
	Watcher(ZRef<ZTS_Watchable> iTS_Watchable);

// From ZRefCountedWithFinalization via ZTSWatcher
	virtual void Finalize();

// From ZTSWatcher
	virtual void AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCountIssued);

	virtual void DoIt(
		const uint64* iRemovedIDs, size_t iRemovedIDsCount,
		const uint64* iAddedIDs, size_t iAddedIDsCount,
		const int64* iRemovedQueries, size_t iRemovedQueriesCount,
		const ZTSWatcher::AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
		vector<uint64>& oAddedIDs,
		vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
		const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
		map<int64, vector<uint64> >& oChangedQueries);

	virtual void SetCallback(Callback_t iCallback, void* iRefcon);

	ZRef<ZTS_Watchable> fTS_Watchable;

	set<PTuple*> fPTuples;
	set<PTuple*> fTrippedPTuples;

	map<int64, WatcherQuery*> fRefcon_To_WatcherQuery;
	ZooLib::DListHead<WatcherQueryTripped> fTrippedWatcherQueries;

	Callback_t fCallback;
	void* fRefcon;
	};

ZTS_Watchable::Watcher::Watcher(ZRef<ZTS_Watchable> iTS_Watchable)
:	fTS_Watchable(iTS_Watchable),
	fCallback(nil),
	fRefcon(nil)
	{}

void ZTS_Watchable::Watcher::Finalize()
	{ fTS_Watchable->Watcher_Finalize(this); }

void ZTS_Watchable::Watcher::AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCountIssued)
	{ fTS_Watchable->Watcher_AllocateIDs(this, iCount, oBaseID, oCountIssued); }

void ZTS_Watchable::Watcher::DoIt(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const ZTSWatcher::AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oAddedIDs,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	oAddedIDs.clear();
	fTS_Watchable->Watcher_DoIt(this,
		iRemovedIDs, iRemovedIDsCount,
		iAddedIDs, iAddedIDsCount,
		iRemovedQueries, iRemovedQueriesCount,
		iAddedQueries, iAddedQueriesCount,
		oAddedIDs,
		oChangedTupleIDs, oChangedTuples,
		iWrittenTupleIDs, iWrittenTuples, iWrittenTuplesCount,
		oChangedQueries);
	}

void ZTS_Watchable::Watcher::SetCallback(ZTSWatcher::Callback_t iCallback, void* iRefcon)
	{ fTS_Watchable->Watcher_SetCallback(this, iCallback, iRefcon); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable::Quisitioner

class ZTS_Watchable::Quisitioner : public ZTupleQuisitioner
	{
public:
	Quisitioner(ZRef<ZTS_Watchable> iTS_Watchable, PQuery* iPQuery);

// From ZTupleQuisitioner
	virtual void Search(const ZTBSpec& iTBSpec, set<uint64>& ioIDs);
	virtual void Search(const ZTBSpec& iTBSpec, vector<uint64>& oIDs);

	virtual void FetchTuples(size_t iCount, const uint64* iIDs, ZTuple* oTuples);
	virtual ZTuple FetchTuple(uint64 iID);

private:
	ZRef<ZTS_Watchable> fTS_Watchable;
	PQuery* fPQuery;
	};

ZTS_Watchable::Quisitioner::Quisitioner(ZRef<ZTS_Watchable> iTS_Watchable, PQuery* iPQuery)
:	fTS_Watchable(iTS_Watchable),
	fPQuery(iPQuery)
	{}

void ZTS_Watchable::Quisitioner::Search(const ZTBSpec& iTBSpec, set<uint64>& ioIDs)
	{ fTS_Watchable->Quisitioner_Search(fPQuery, iTBSpec, ioIDs); }

void ZTS_Watchable::Quisitioner::Search(const ZTBSpec& iTBSpec, vector<uint64>& oIDs)
	{ fTS_Watchable->Quisitioner_Search(fPQuery, iTBSpec, oIDs); }

void ZTS_Watchable::Quisitioner::FetchTuples(size_t iCount, const uint64* iIDs, ZTuple* oTuples)
	{ fTS_Watchable->Quisitioner_FetchTuples(iCount, iIDs, oTuples); }

ZTuple ZTS_Watchable::Quisitioner::FetchTuple(uint64 iID)
	{ return fTS_Watchable->Quisitioner_FetchTuple(iID); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable

ZTS_Watchable::ZTS_Watchable(ZRef<ZTS> iTS)
:	fTS(iTS),
	fMutex_Structure("ZTS_Watchable::fMutex_Structure")
	{}

ZTS_Watchable::~ZTS_Watchable()
	{
	ZMutexLocker locker(fMutex_Structure);

	ZAssertStop(kDebug_TS_Watchable, fWatchers.empty());
	
	// We're retaining PQueries past the point that they're in active
	// use by any watchers, so we need to explicitly delete any
	// remaining ones, and let the map's destructor take care of
	// actually destroying its entries, which will be stale pointers
	// to PQueries, and the ZTBQuery keys.

	for (map<ZMemoryBlock, PQuery*>::iterator i = fMB_To_PQuery.begin(); i != fMB_To_PQuery.end(); ++i)
		{
		PQuery* thePQuery = (*i).second;
		for (set<PSpec*>::iterator i = thePQuery->fPSpecs.begin(), theEnd = thePQuery->fPSpecs.end();
			i != theEnd; ++i)
			{
			PSpec* thePSpec = *i;
			ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, thePSpec->fUsingPQueries, thePQuery);
			this->pReleasePSpec(thePSpec);
			}
		delete thePQuery;
		}
	}

void ZTS_Watchable::AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCount)
	{ fTS->AllocateIDs(iCount, oBaseID, oCount); }

void ZTS_Watchable::SetTuples(size_t iCount, const uint64* iIDs, const ZTuple* iTuples)
	{ this->pSetTuples(iCount, iIDs, iTuples); }

void ZTS_Watchable::GetTuples(size_t iCount, const uint64* iIDs, ZTuple* oTuples)
	{ fTS->GetTuples(iCount, iIDs, oTuples); }

void ZTS_Watchable::Search(const ZTBSpec& iTBSpec, const set<uint64>& iSkipIDs, set<uint64>& oIDs)
	{ fTS->Search(iTBSpec, iSkipIDs, oIDs); }

ZMutexBase& ZTS_Watchable::GetReadLock()
	{ return fTS->GetReadLock(); }

ZMutexBase& ZTS_Watchable::GetWriteLock()
	{ return fTS->GetWriteLock(); }

ZRef<ZTSWatcher> ZTS_Watchable::NewWatcher()
	{
	Watcher* newWatcher = new Watcher(this);
	ZMutexLocker locker(fMutex_Structure);
	fWatchers.insert(newWatcher);
	return newWatcher;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable Watcher

void ZTS_Watchable::Watcher_Finalize(Watcher* iWatcher)
	{
	ZMutexLocker locker(fMutex_Structure);
	if (iWatcher->GetRefCount() > 1)
		{
		iWatcher->FinalizationComplete();
		return;
		}

	ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, fWatchers, iWatcher);
	for (set<PTuple*>::iterator i = iWatcher->fPTuples.begin(),
		theEnd = iWatcher->fPTuples.end();
		i != theEnd; ++i)
		{
		PTuple* thePTuple = *i;
		ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, thePTuple->fUsingWatchers, iWatcher);
		this->pReleasePTuple(thePTuple);
		}

	for (map<int64, WatcherQuery*>::iterator i = iWatcher->fRefcon_To_WatcherQuery.begin(),
		theEnd = iWatcher->fRefcon_To_WatcherQuery.end();
		i != theEnd; ++i)
		{
		WatcherQuery* theWatcherQuery = (*i).second;
		
		// Detach theWatcherQuery from its PQuery.
		PQuery* thePQuery = theWatcherQuery->fPQuery;
		thePQuery->fUsingWatcherQueries.Remove(theWatcherQuery);
		this->pReleasePQuery(thePQuery);

		// If it had been tripped it will be on this watcher's list
		// of tripped queries and must be removed or we'll trip an assertion
		// in theWatcherQuery's destructor.
		iWatcher->fTrippedWatcherQueries.RemoveIfContains(theWatcherQuery);

		delete theWatcherQuery;
		}

	iWatcher->FinalizationComplete();

	delete iWatcher;
	}

void ZTS_Watchable::Watcher_AllocateIDs(
	Watcher* iWatcher, size_t iCount, uint64& oBaseID, size_t& oCountIssued)
	{ fTS->AllocateIDs(iCount, oBaseID, oCountIssued); }

template <typename C>
void sDumpIDs(const ZStrimW& s, const C& iIDs)
	{
	for (typename C::const_iterator i = iIDs.begin(); i != iIDs.end(); ++i)
		s.Writef("%llX, ", *i);
	}

void ZTS_Watchable::Watcher_SetCallback(Watcher* iWatcher,
	ZTSWatcher::Callback_t iCallback, void* iRefcon)
	{
	ZMutexLocker locker(fMutex_Structure);
	ZAssertStop(kDebug_TS_Watchable, !iWatcher->fCallback);
	iWatcher->fCallback = iCallback;
	iWatcher->fRefcon = iRefcon;

	if (iWatcher->fTrippedWatcherQueries || !iWatcher->fTrippedPTuples.empty())
		iWatcher->fCallback(iWatcher->fRefcon);
	}

void ZTS_Watchable::Watcher_DoIt(Watcher* iWatcher,
		const uint64* iRemovedIDs, size_t iRemovedIDsCount,
		const uint64* iAddedIDs, size_t iAddedIDsCount,
		const int64* iRemovedQueries, size_t iRemovedQueriesCount,
		const ZTSWatcher::AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
		vector<uint64>& oAddedIDs,
		vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
		const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
		map<int64, vector<uint64> >& oChangedQueries)
	{
	vector<ZTSWatcher::AddedQueryCombo> local_AddedQueries(iAddedQueries, iAddedQueries + iAddedQueriesCount);

	// Go through and ensure that the local data structures have ZTBQueries and
	// real ZMemoryBlocks. We do it here to simplify code that calls us.
	// For some it's easy to provide the ZMemoryBlock, for others its easy to
	// provide the ZTBQuery. Obviously it has to provide one or the other.

	bool missingQueries = false;
	for (vector<ZTSWatcher::AddedQueryCombo>::iterator i = local_AddedQueries.begin();
		i != local_AddedQueries.end(); ++i)
		{
		ZTBQuery& theTBQueryRef = i->fTBQuery;
		ZMemoryBlock& theMBRef = i->fMemoryBlock;
		if (!theTBQueryRef)
			{
			// If no query is provided, then there must be an MB.
			ZAssertStop(kDebug_TS_Watchable, theMBRef);
			missingQueries = true;
			}
		else if (!theMBRef)
			{
			theTBQueryRef.ToStream(ZStreamRWPos_MemoryBlock(theMBRef));
			}
		}

	// By this point we have MBs for every query. We may not have ZTBQueries for them though.
	if (missingQueries)
		{
		missingQueries = false;
		fMutex_Structure.Acquire();
		for (vector<ZTSWatcher::AddedQueryCombo>::iterator i = local_AddedQueries.begin();
			i != local_AddedQueries.end(); ++i)
			{
			ZTBQuery& theTBQueryRef = i->fTBQuery;
			ZMemoryBlock& theMBRef = i->fMemoryBlock;
			
			if (!theTBQueryRef)
				{
				map<ZMemoryBlock, PQuery*>::iterator existing = fMB_To_PQuery.find(theMBRef);
				if (fMB_To_PQuery.end() == existing)
					missingQueries = true;
				else
					theTBQueryRef = existing->second->fTBQuery;
				}
			}
		fMutex_Structure.Release();
		}

	if (missingQueries)
		{
		// We're still missing on or more queries. We've grabbed everything from the
		// map that we can. We'll have to regenerate from MBs. It may be
		// that something's come in in the meantime, but wtf.
		for (vector<ZTSWatcher::AddedQueryCombo>::iterator i = local_AddedQueries.begin();
			i != local_AddedQueries.end(); ++i)
			{
			ZTBQuery& theTBQueryRef = i->fTBQuery;
			if (!theTBQueryRef)
				{
				ZMemoryBlock& theMBRef = i->fMemoryBlock;
				theTBQueryRef = ZTBQuery(ZStreamRPos_Memory(theMBRef.GetData(), theMBRef.GetSize()));
				}
			}
		}

	ZLocker lockerRW(iWrittenTuplesCount ? this->GetWriteLock() : this->GetReadLock());

	ZMutexLocker locker(fMutex_Structure);

	for (size_t count = iRemovedIDsCount; count; --count)
		{
		PTuple* thePTuple = this->pGetPTupleExtant(*iRemovedIDs++);
		ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, thePTuple->fUsingWatchers, iWatcher);
		ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, iWatcher->fPTuples, thePTuple);
		ZUtil_STL::sEraseIfContains(iWatcher->fTrippedPTuples, thePTuple);
		this->pReleasePTuple(thePTuple);
		}

	for (size_t count = iAddedIDsCount; count; --count)
		{
		PTuple* thePTuple = this->pGetPTuple(*iAddedIDs++);
		ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable, thePTuple->fUsingWatchers, iWatcher);
		ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable, iWatcher->fPTuples, thePTuple);
		ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable, iWatcher->fTrippedPTuples, thePTuple);
		}

	for (size_t count = iRemovedQueriesCount; count; --count)
		{
		WatcherQuery* theWatcherQuery = ZUtil_STL::sEraseAndReturn(kDebug_TS_Watchable,
			iWatcher->fRefcon_To_WatcherQuery, *iRemovedQueries++);

		iWatcher->fTrippedWatcherQueries.RemoveIfContains(theWatcherQuery);

		PQuery* thePQuery = theWatcherQuery->fPQuery;
		thePQuery->fUsingWatcherQueries.Remove(theWatcherQuery);
		delete theWatcherQuery;
		this->pReleasePQuery(thePQuery);
		}

	for (vector<ZTSWatcher::AddedQueryCombo>::iterator i = local_AddedQueries.begin();
		i != local_AddedQueries.end(); ++i)
		{
		const ZMemoryBlock& theMB = i->fMemoryBlock;
		const int64 theRefcon = i->fRefcon;
		const bool thePrefetch = i->fPrefetch;

		map<ZMemoryBlock, PQuery*>::iterator position = fMB_To_PQuery.lower_bound(theMB);
		PQuery* thePQuery;
		if (position != fMB_To_PQuery.end() && position->first == theMB)
			{
			thePQuery = position->second;
			}
		else
			{
			thePQuery = new PQuery(theMB, i->fTBQuery);
			fMB_To_PQuery.insert(position, pair<ZMemoryBlock, PQuery*>(theMB, thePQuery));
			}

		WatcherQuery* theWatcherQuery = new WatcherQuery(iWatcher, theRefcon, thePQuery, thePrefetch);
		thePQuery->fUsingWatcherQueries.Insert(theWatcherQuery);

		ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable,
			iWatcher->fRefcon_To_WatcherQuery, theRefcon, theWatcherQuery);

		iWatcher->fTrippedWatcherQueries.Insert(theWatcherQuery);
		}

	if (size_t changedCount = iWatcher->fTrippedPTuples.size())
		{
		if (iWrittenTuplesCount)
			{
			oChangedTupleIDs.reserve(changedCount);
			const uint64* writtenTupleIDsEnd = iWrittenTupleIDs + iWrittenTuplesCount;
			for (set<PTuple*>::iterator
				i = iWatcher->fTrippedPTuples.begin(), theEnd = iWatcher->fTrippedPTuples.end();
				i != theEnd; ++i)
				{
				uint64 theID = (*i)->fID;

				// iWrittenTupleIDs/iWrittenTuples is sorted by ID.
				// If not then don't call this method.
				const uint64* wasWritten = lower_bound(iWrittenTupleIDs, writtenTupleIDsEnd, theID);
				if (wasWritten == writtenTupleIDsEnd || *wasWritten != theID)
					oChangedTupleIDs.push_back(theID);
				}
			}
		else
			{
			oChangedTupleIDs.resize(changedCount);
			vector<uint64>::iterator iter = oChangedTupleIDs.begin();
			for (set<PTuple*>::iterator
				i = iWatcher->fTrippedPTuples.begin(), theEnd = iWatcher->fTrippedPTuples.end();
				i != theEnd; ++i)
				{
				*iter++ = (*i)->fID;
				}

			}		
		oChangedTuples.resize(oChangedTupleIDs.size());
		fTS->GetTuples(oChangedTupleIDs.size(), &oChangedTupleIDs[0], &oChangedTuples[0]);
		}

	set<Watcher*> touchedWatchers;

	if (iWrittenTuplesCount)
		this->pSetTuples(iWrittenTuplesCount, iWrittenTupleIDs, iWrittenTuples, touchedWatchers);

	// We clear fTrippedPTuples *after* we've done the write, so that these changes made
	// on behalf of this watcher are not given to this watcher on its next call to us.
	iWatcher->fTrippedPTuples.clear();


	for (ZooLib::DListIteratorEraseAll<WatcherQuery, WatcherQueryTripped>
		iter = iWatcher->fTrippedWatcherQueries;iter; iter.Advance())
		{
		WatcherQuery* theWatcherQuery = iter.Current();
		PQuery* thePQuery = theWatcherQuery->fPQuery;
		this->pUpdateQueryResults(thePQuery);
		ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable,
			oChangedQueries, theWatcherQuery->fRefcon, thePQuery->fResults);

		if (theWatcherQuery->fPrefetch)
			{
			// Need to go through all the results and get the PTuple for each, and add
			// the current watcher to them if they're not already there.
			for (vector<uint64>::iterator i = thePQuery->fResults.begin(),
				theEnd = thePQuery->fResults.end(); i != theEnd; ++i)
				{
				const uint64 theID = *i;
				PTuple* thePTuple = this->pGetPTuple(theID);
				if (ZUtil_STL::sInsertIfNotContains(thePTuple->fUsingWatchers, iWatcher))
					{
					ZUtil_STL::sInsertMustNotContain(kDebug_TS_Watchable, iWatcher->fPTuples, thePTuple);
					ZAssertStop(kDebug_TS_Watchable, !ZUtil_STL::sContains(oChangedTupleIDs, theID));
					oAddedIDs.push_back(theID);
					}
				}
			}
		}

	if (size_t addedSize = oAddedIDs.size())
		{
		oChangedTupleIDs.insert(oChangedTupleIDs.end(), oAddedIDs.begin(), oAddedIDs.end());
		size_t oldSize = oChangedTuples.size();
		oChangedTuples.resize(oldSize + addedSize);
		fTS->GetTuples(addedSize, &oChangedTupleIDs[oldSize], &oChangedTuples[oldSize]);
		}

	// Notify all other affected watchers that they should call DoIt
	for (set<Watcher*>::iterator i = touchedWatchers.begin(), theEnd = touchedWatchers.end();
		i != theEnd; ++i)
		{
		Watcher* theWatcher = *i;
		if (theWatcher != iWatcher)
			{
			if (theWatcher->fCallback)
				theWatcher->fCallback(theWatcher->fRefcon);
			}
		}

	lockerRW.Release();
	locker.Release();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable Quisitioner

void ZTS_Watchable::Quisitioner_Search(PQuery* iPQuery, const ZTBSpec& iTBSpec, set<uint64>& ioIDs)
	{
	set<uint64> empty;
	fTS->Search(iTBSpec, empty, ioIDs);
	PSpec* thePSpec = this->pGetPSpec(iTBSpec);
	thePSpec->fUsingPQueries.insert(iPQuery);
	iPQuery->fPSpecs.insert(thePSpec);
	}

void ZTS_Watchable::Quisitioner_Search(PQuery* iPQuery,
	const ZTBSpec& iTBSpec, vector<uint64>& oIDs)
	{
	set<uint64> results;
	this->Quisitioner_Search(iPQuery, iTBSpec, results);
	oIDs.insert(oIDs.end(), results.begin(), results.end());
	}

void ZTS_Watchable::Quisitioner_FetchTuples(size_t iCount, const uint64* iIDs, ZTuple* oTuples)
	{ fTS->GetTuples(iCount, iIDs, oTuples); }

ZTuple ZTS_Watchable::Quisitioner_FetchTuple(uint64 iID)
	{
	ZTuple result;
	fTS->GetTuples(1, &iID, &result);
	return result;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTS_Watchable private

void ZTS_Watchable::pSetTuples(size_t iCount, const uint64* iIDs, const ZTuple* iTuples)
	{
	set<Watcher*> touchedWatchers;
	this->pSetTuples(iCount, iIDs, iTuples, touchedWatchers);
	for (set<Watcher*>::iterator i = touchedWatchers.begin(), theEnd = touchedWatchers.end();
		i != theEnd; ++i)
		{
		Watcher* theWatcher = *i;
		if (theWatcher->fCallback)
			theWatcher->fCallback(theWatcher->fRefcon);
		}
	}

void ZTS_Watchable::pSetTuples(size_t iCount, const uint64* iIDs, const ZTuple* iTuples,
	set<Watcher*>& ioTouchedWatchers)
	{
	vector<ZTuple> oldVector(iCount, ZTuple());
	fTS->GetTuples(iCount, iIDs, &oldVector[0]);

	ZMutexLocker locker(fMutex_Structure);
	for (size_t x = 0; x < iCount; ++x)
		{
		ZTuple oldTuple = oldVector[x];
		ZTuple newTuple = iTuples[x];
		this->pInvalidatePSpecs(oldTuple, newTuple, ioTouchedWatchers);
		this->pInvalidateTuple(iIDs[x], ioTouchedWatchers);
		}
	fTS->SetTuples(iCount, iIDs, iTuples);
	}

void ZTS_Watchable::pInvalidatePSpecs(const ZTuple& iOld, const ZTuple& iNew,
	set<Watcher*>& ioTouchedWatchers)
	{
	ASSERTLOCKED(fMutex_Structure);

	#if kDebug_CountInvalidations
	int propNamesInvalidated = 0;
	#endif

	set<PQuery*> toInvalidate;
	for (map<ZTuplePropName, set<PSpec*> >::iterator iter = fPropName_To_PSpec.begin(),
		theEnd = fPropName_To_PSpec.end();
		iter != theEnd; ++iter)
		{
		const ZTuplePropName& theName = (*iter).first;
		const ZTupleValue& theOld = iOld.GetValue(theName);
		const ZTupleValue& theNew = iNew.GetValue(theName);
		if (theOld != theNew)
			{
			#if kDebug_CountInvalidations
			size_t beforeSize = toInvalidate.size();
			size_t affectedSpecs = 0;
			#endif

			for (set<PSpec*>::iterator
				iterPSpec = (*iter).second.begin(), theEnd = (*iter).second.end();
				iterPSpec != theEnd; ++iterPSpec)
				{
				PSpec* curPSpec = *iterPSpec;
				if (curPSpec->GetTBSpec().Matches(iOld) != curPSpec->GetTBSpec().Matches(iNew))
					{
					toInvalidate.insert(
						curPSpec->fUsingPQueries.begin(), curPSpec->fUsingPQueries.end());

					#if kDebug_CountInvalidations
					++affectedSpecs;
					#endif
					}
				}
			#if kDebug_CountInvalidations
			if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 2, "ZTS_Watchable"))
				{
				s << "Examined "
					<< ZString::sFromInt((*iter).second.size())
					<< " specs, invalidated "
					<< ZString::sFromInt(affectedSpecs)
					<< " of them, and thence "
					<< ZString::sFromInt(toInvalidate.size() - beforeSize)
					<< " queries by change to Name: " << theName
					<< "\nOld: " << theOld << "\nNew: " << theNew;
				}
			++propNamesInvalidated;
			#endif
			}
		}

	// We now have a set of all the PQueries affected by this tuple being touched.
	for (set<PQuery*>::iterator i = toInvalidate.begin(), theEnd = toInvalidate.end();
		i != theEnd; ++i)
		{
		PQuery* curPQuery = *i;

		for (ZooLib::DListIterator<WatcherQuery, WatcherQueryUsing>
			iter = curPQuery->fUsingWatcherQueries;iter; iter.Advance())
			{
			WatcherQuery* curWatcherQuery = iter.Current();
			curWatcherQuery->fWatcher->fTrippedWatcherQueries.InsertIfNotContains(curWatcherQuery);
			ioTouchedWatchers.insert(curWatcherQuery->fWatcher);
			}

		for (set<PSpec*>::iterator
			iterPSpec = curPQuery->fPSpecs.begin(), theEnd = curPQuery->fPSpecs.end();
			iterPSpec != theEnd; ++iterPSpec)
			{
			PSpec* curPSpec = *iterPSpec;
			ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, curPSpec->fUsingPQueries, curPQuery);
			this->pReleasePSpec(curPSpec);
			}
		curPQuery->fPSpecs.clear();

		curPQuery->fResults.clear();
		curPQuery->fResultsValid = false;
		// Give us a crack at actually tossing the PQuery, if it has no watchers.
		this->pReleasePQuery(curPQuery);
		}
	}

void ZTS_Watchable::pInvalidateTuple(uint64 iID, set<Watcher*>& ioTouchedWatchers)
	{
	ASSERTLOCKED(fMutex_Structure);

	map<uint64, PTuple*>::iterator iter = fID_To_PTuple.find(iID);
	if (iter == fID_To_PTuple.end())
		return;

	PTuple* thePTuple = (*iter).second;

	for (set<Watcher*>::iterator
		i = thePTuple->fUsingWatchers.begin(), theEnd = thePTuple->fUsingWatchers.end();
		i != theEnd; ++i)
		{
		(*i)->fTrippedPTuples.insert(thePTuple);
		ioTouchedWatchers.insert(*i);
		}
	}

ZTS_Watchable::PTuple* ZTS_Watchable::pGetPTupleExtant(uint64 iID)
	{
	ASSERTLOCKED(fMutex_Structure);

	map<uint64, PTuple*>::iterator position = fID_To_PTuple.find(iID);
	ZAssertStop(kDebug_TS_Watchable, position != fID_To_PTuple.end());
	return position->second;
	}

ZTS_Watchable::PTuple* ZTS_Watchable::pGetPTuple(uint64 iID)
	{
	ASSERTLOCKED(fMutex_Structure);

	map<uint64, PTuple*>::iterator position = fID_To_PTuple.lower_bound(iID);
	if (position != fID_To_PTuple.end() && position->first == iID)
		return position->second;

	PTuple* thePTuple = new PTuple(iID);
	fID_To_PTuple.insert(position, map<uint64, PTuple*>::value_type(iID, thePTuple));
	return thePTuple;
	}

void ZTS_Watchable::pReleasePTuple(PTuple* iPTuple)
	{
	ASSERTLOCKED(fMutex_Structure);

	if (!iPTuple->fUsingWatchers.empty())
		return;

	ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, fID_To_PTuple, iPTuple->fID);
	delete iPTuple;
	}

ZTS_Watchable::PSpec* ZTS_Watchable::pGetPSpec(const ZTBSpec& iTBSpec)
	{
	ASSERTLOCKED(fMutex_Structure);

	map<ZTBSpec, PSpec*>::iterator position = fTBSpec_To_PSpec.lower_bound(iTBSpec);
	if (position != fTBSpec_To_PSpec.end() && position->first == iTBSpec)
		return position->second;

	PSpec* thePSpec = new PSpec(iTBSpec);

	fTBSpec_To_PSpec.insert(position, pair<ZTBSpec, PSpec*>(iTBSpec, thePSpec));

	const set<ZTuplePropName>& propNames = thePSpec->GetPropNames();

	for (set<ZTuplePropName>::const_iterator i = propNames.begin(), theEnd = propNames.end();
		i != theEnd; ++i)
		{
		map<ZTuplePropName, set<PSpec*> >::iterator j = fPropName_To_PSpec.find(*i);
		if (fPropName_To_PSpec.end() == j)
			{
			// Nothing yet stored under the propName *i.
			fPropName_To_PSpec.insert(pair<ZTuplePropName, set<PSpec*> >(*i, set<PSpec*>()));
			j = fPropName_To_PSpec.find(*i);
			}
		(*j).second.insert(thePSpec);
		}

	return thePSpec;
	}

void ZTS_Watchable::pReleasePSpec(PSpec* iPSpec)
	{
	ASSERTLOCKED(fMutex_Structure);

	if (!iPSpec->fUsingPQueries.empty())
		return;

	const set<ZTuplePropName>& propNames = iPSpec->GetPropNames();

	for (set<ZTuplePropName>::const_iterator i = propNames.begin(), theEnd = propNames.end();
		i != theEnd; ++i)
		{
		map<ZTuplePropName, set<PSpec*> >::iterator j = fPropName_To_PSpec.find(*i);
		ZAssertStop(kDebug_TS_Watchable, j != fPropName_To_PSpec.end());
		ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, (*j).second, iPSpec);
		}

	ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, fTBSpec_To_PSpec, iPSpec->GetTBSpec());
	delete iPSpec;
	}

void ZTS_Watchable::pReleasePQuery(PQuery* iPQuery)
	{
	ASSERTLOCKED(fMutex_Structure);

	// Can't toss it if it's in use.
	if (iPQuery->fUsingWatcherQueries)
		return;

	// Don't toss it if it still has valid results.
	if (iPQuery->fResultsValid)
		return;

	for (set<PSpec*>::iterator i = iPQuery->fPSpecs.begin(), theEnd = iPQuery->fPSpecs.end();
		i != theEnd; ++i)
		{
		PSpec* thePSpec = *i;
		ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, thePSpec->fUsingPQueries, iPQuery);
		this->pReleasePSpec(thePSpec);
		}

	ZUtil_STL::sEraseMustContain(kDebug_TS_Watchable, fMB_To_PQuery, iPQuery->fMB);
	delete iPQuery;
	}

void ZTS_Watchable::pUpdateQueryResults(PQuery* iPQuery)
	{
	ASSERTLOCKED(fMutex_Structure);

	if (iPQuery->fResultsValid)
		return;

	iPQuery->fResultsValid = true;

	if (ZRef<ZTBQueryNode> theNode = iPQuery->fTBQuery.GetNode())
		Quisitioner(this, iPQuery).Query(theNode, nil, iPQuery->fResults);
	}
