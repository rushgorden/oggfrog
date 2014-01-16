static const char rcsid[] = "@(#) $Id: ZTSoup.cpp,v 1.20 2006/11/03 04:13:29 agreen Exp $";

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

#include "ZTSoup.h"

#include "ZLog.h"
#include "ZString.h"
#include "ZUtil_Tuple.h"
#include "ZUtil_STL.h"

using std::map;
using std::pair;
using std::set;
using std::vector;

#define ASSERTLOCKED(a) ZAssertStop(ZTSoup::kDebug, a.IsLocked())
#define ASSERTUNLOCKED(a) ZAssertStop(ZTSoup::kDebug, !a.IsLocked())

// =================================================================================================
/**
\defgroup Tuplesoup Tuplesoup
\sa Tuplestore

*/

// =================================================================================================
#pragma mark -
#pragma mark * Static helpers

static void sCroutonsChanged(const set<ZRef<ZTCrouton> >& iTCroutons, ZTCrouton::EChange iChange)
	{
	for (set<ZRef<ZTCrouton> >::const_iterator i = iTCroutons.begin(), theEnd = iTCroutons.end();
		i != theEnd; ++i)
		{
		try
			{
			(*i)->Changed(iChange);
			}
		catch (...)
			{}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * PSieve

class ZTSoup::PSieveUpdate : public ZooLib::DListLink<ZTSoup::PSieve, ZTSoup::PSieveUpdate>
	{};

class ZTSoup::PSieveSync : public ZooLib::DListLink<ZTSoup::PSieve, ZTSoup::PSieveSync>
	{};

class ZTSoup::PSieveChanged : public ZooLib::DListLink<ZTSoup::PSieve, ZTSoup::PSieveChanged>
	{};

class ZTSoup::PSieve : public ZTSoup::PSieveUpdate,
	public ZTSoup::PSieveSync,
	public ZTSoup::PSieveChanged
	{
public:
	void DisposingTSieve(ZTSieve* iTSieve);

	bool HasCurrent();
	bool HasPrior();

	const vector<uint64>& GetCurrent();
	bool GetCurrent(vector<uint64>& oCurrent);
	bool GetCurrent(set<uint64>& oCurrent);

	const vector<uint64>& GetPrior();
	bool GetPrior(vector<uint64>& oPrior);
	bool GetPrior(set<uint64>& oPrior);

	void GetAdded(vector<uint64>& oCurrent);
	void GetAdded(set<uint64>& oCurrent);

	void GetRemoved(vector<uint64>& oCurrent);
	void GetRemoved(set<uint64>& oCurrent);

	ZRef<ZTSoup> GetTSoup();
	ZTBQuery GetTBQuery();

	ZRef<ZTSoup> fTSoup;
	ZTBQuery fTBQuery;

	ZooLib::DListHead<ZTSieveLink, kDebug> fUsingTSieves;

	bool fServerKnown;

	bool fPrefetch;

	bool fHasResults_Current;
	bool fHasResults_Prior;

	vector<uint64> fResults_Local_Prior;
	vector<uint64> fResults_Local_Current;
	vector<uint64> fResults_Remote;

	bool fHasDiffs;
	set<uint64> fAdded;
	set<uint64> fRemoved;
	};

// =================================================================================================
#pragma mark -
#pragma mark * PCrouton

class ZTSoup::PCroutonUpdate : public ZooLib::DListLink<ZTSoup::PCrouton, ZTSoup::PCroutonUpdate>
	{};

class ZTSoup::PCroutonSync : public ZooLib::DListLink<ZTSoup::PCrouton, ZTSoup::PCroutonSync>
	{};

class ZTSoup::PCroutonChanged : public ZooLib::DListLink<ZTSoup::PCrouton, ZTSoup::PCroutonChanged>
	{};

class ZTSoup::PCroutonSyncing : public ZooLib::DListLink<ZTSoup::PCrouton, ZTSoup::PCroutonSyncing>
	{};

class ZTSoup::PCroutonPending : public ZooLib::DListLink<ZTSoup::PCrouton, ZTSoup::PCroutonPending>
	{};

class ZTSoup::PCrouton : public ZTSoup::PCroutonUpdate,
	public ZTSoup::PCroutonSync,
	public ZTSoup::PCroutonChanged,
	public ZTSoup::PCroutonSyncing,
	public ZTSoup::PCroutonPending
	{
public:
	void DisposingTCrouton(ZTCrouton* iTCrouton);

	uint64 GetID();

	bool HasCurrent();
	bool HasPrior();

	bool GetCurrent(ZTuple& oTuple);
	bool GetPrior(ZTuple& oTuple);

	void Set(const ZTuple& iTuple);

	ZRef<ZTSoup> GetTSoup();

	ZRef<ZTSoup> fTSoup;
	uint64 fID;

	ZooLib::DListHead<ZTCroutonLink, kDebug> fUsingTCroutons;

	bool fServerKnown;

	bool fHasValue_Current;
	bool fHasValue_Prior;

	ZTuple fValue_Current;
	ZTuple fValue_Prior;

	bool fWrittenLocally;
	bool fHasValue_ForServer;

	ZTuple fValue_ForServer;
	ZTuple fValue_FromServer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTSoup

/**
\class ZTSoup
\nosubgrouping
\ingroup Tuplesoup

ZTSoup and its associated classes ZTSieve and ZTCrouton provide an alternative means of working
with data in a tuplestore.

*/

ZTSoup::ZTSoup(ZRef<ZTSWatcher> iTSWatcher)
:	fTSWatcher(iTSWatcher),
	fMutex_CallSync("ZTSoup::fMutex_CallSync"),
	fMutex_CallUpdate("ZTSoup::fMutex_CallUpdate"),
	fMutex_Structure("ZTSoup::fMutex_Structure"),
	fCalled_UpdateNeeded(false),
	fCallback_UpdateNeeded(nil),
	fRefcon_UpdateNeeded(nil),
	fCalled_SyncNeeded(false),
	fCallback_SyncNeeded(nil),
	fRefcon_SyncNeeded(nil)
	{
	fTSWatcher->SetCallback(pCallback_TSWatcher, this);
	}

ZTSoup::~ZTSoup()
	{}

void ZTSoup::SetCallback_Update(
	Callback_UpdateNeeded_t iCallback_UpdateNeeded, void* iRefcon_UpdateNeeded)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	fCallback_UpdateNeeded = iCallback_UpdateNeeded;
	fRefcon_UpdateNeeded = iRefcon_UpdateNeeded;

	locker_CallUpdate.Release();

	if (fCalled_UpdateNeeded && fCallback_UpdateNeeded)
		fCallback_UpdateNeeded(fRefcon_UpdateNeeded);
	}

void ZTSoup::SetCallback_Sync(
	Callback_SyncNeeded_t iCallback_SyncNeeded, void* iRefcon_SyncNeeded)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	fCallback_SyncNeeded = iCallback_SyncNeeded;
	fRefcon_SyncNeeded = iRefcon_SyncNeeded;

	locker_CallUpdate.Release();

	if (fCalled_SyncNeeded && fCallback_SyncNeeded)
		fCallback_SyncNeeded(fRefcon_SyncNeeded);
	}

void ZTSoup::AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCountIssued)
	{ fTSWatcher->AllocateIDs(iCount, oBaseID, oCountIssued); }

uint64 ZTSoup::AllocateID()
	{
	uint64 theID;
	for (;;)
		{
		size_t countIssued;
		fTSWatcher->AllocateIDs(1, theID, countIssued);
		if (countIssued)
			return theID;
		}
	}

void ZTSoup::Set(uint64 iID, const ZTuple& iTuple)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);
	PCrouton* thePCrouton = this->pGetPCrouton(iID);
	this->pSet(locker_Structure, thePCrouton, iTuple);
	}

uint64 ZTSoup::Add(const ZTuple& iTuple)
	{
	uint64 theID = this->AllocateID();
	this->Set(theID, iTuple);
	return theID;
	}

void ZTSoup::Register(ZRef<ZTSieve> iTSieve, const ZTBQuery& iTBQuery)
	{
	this->Register(iTSieve, iTBQuery, true);
	}

void ZTSoup::Register(ZRef<ZTSieve> iTSieve, const ZTBQuery& iTBQuery, bool iPrefetch)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	PSieve* thePSieve;
	map<ZTBQuery, PSieve>::iterator position = fTBQuery_To_PSieve.lower_bound(iTBQuery);
	if (position != fTBQuery_To_PSieve.end() && position->first == iTBQuery)
		{
		thePSieve = &position->second;
		if (iPrefetch && !thePSieve->fPrefetch)
			{
			thePSieve->fPrefetch = true;
			fPSieves_Update.InsertIfNotContains(thePSieve);
			}
		}
	else
		{
		thePSieve = &fTBQuery_To_PSieve.
			insert(position, pair<ZTBQuery, PSieve>(iTBQuery, PSieve()))->second;

		thePSieve->fTSoup = this;
		thePSieve->fTBQuery = iTBQuery;
		thePSieve->fServerKnown = false;
		thePSieve->fPrefetch = iPrefetch;
		thePSieve->fHasResults_Current = false;
		thePSieve->fHasResults_Prior = false;
		thePSieve->fHasDiffs = false;
		
		fPSieves_Update.Insert(thePSieve);

		this->pTriggerUpdate();
		}

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		{
		s.Writef("Registering against sieve, ID: %X", thePSieve);
		if (!thePSieve->fServerKnown)
			s << ", not known to server";
		}

	ZTSieve* theTSieve = iTSieve.GetObject();
	theTSieve->fPSieve = thePSieve;
	thePSieve->fUsingTSieves.Insert(theTSieve);

	if (thePSieve->fHasResults_Current)
		{
		locker_Structure.Release();
		try
			{
			iTSieve->Changed();
			}
		catch (...)
			{}
		}
	}

void ZTSoup::Register(ZRef<ZTCrouton> iTCrouton, uint64 iID)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	PCrouton* thePCrouton = this->pGetPCrouton(iID);

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		{
		s.Writef("Registering TCrouton, ID: %llX", iID);
		if (!thePCrouton->fServerKnown)
			s << ", not known to server";
		}

	ZTCrouton* theTCrouton = iTCrouton.GetObject();
	iTCrouton->fPCrouton = thePCrouton;
	thePCrouton->fUsingTCroutons.Insert(theTCrouton);

	fPCroutons_Update.InsertIfNotContains(thePCrouton);
	fPCroutons_Pending.RemoveIfContains(thePCrouton);
	this->pTriggerUpdate();

	if (thePCrouton->fHasValue_Current)
		{
		locker_Structure.Release();
		try
			{
			iTCrouton->Changed(ZTCrouton::eChange_Local);
			}
		catch (...)
			{}
		}
	}

void ZTSoup::Register(ZRef<ZTCrouton> iTCrouton)
	{
	this->Register(iTCrouton, this->AllocateID());
	}

void ZTSoup::Sync()
	{
	ZMutexLocker locker_CallSync(fMutex_CallSync);
	ZMutexLocker locker_Structure(fMutex_Structure);

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Sync start";

	fCalled_SyncNeeded = false;

	vector<uint64> removedIDs, addedIDs;

	// We put written tuples into a map, so that they can
	// be extracted in sorted order, which ZTSWatcher::DoItFaster requires.
	map<uint64, ZTuple> writtenTupleMap;

	if (!fPCroutons_Sync.Empty())
		{
		removedIDs.reserve(fPCroutons_Sync.Size());
		addedIDs.reserve(fPCroutons_Sync.Size());
		for (ZooLib::DListIteratorEraseAll<PCrouton, PCroutonSync, kDebug> iter = fPCroutons_Sync;
			iter; iter.Advance())
			{
			PCrouton* thePCrouton = iter.Current();

			if (thePCrouton->fHasValue_ForServer)
				{
				thePCrouton->fHasValue_ForServer = false;
				writtenTupleMap.insert(
					pair<uint64, ZTuple>(thePCrouton->fID, thePCrouton->fValue_ForServer));

				if (!thePCrouton->fServerKnown)
					{
					thePCrouton->fServerKnown = true;
					addedIDs.push_back(thePCrouton->fID);
					}

				// Put it on fPCroutons_Syncing, which will be transcribed
				// to fCroutons_Pending after we've called our watcher.
				fPCroutons_Syncing.InsertIfNotContains(thePCrouton);
				}
			else if (thePCrouton->fUsingTCroutons)
				{
				// It's in use.
				if (!thePCrouton->fServerKnown)
					{
					// But not known to the server.
					thePCrouton->fServerKnown = true;
					addedIDs.push_back(thePCrouton->fID);
					}
				}
			else if (thePCrouton->fServerKnown)
				{
				// It's not in use, and is known to the server.
				thePCrouton->fServerKnown = false;
				removedIDs.push_back(thePCrouton->fID);
				thePCrouton->fHasValue_Current = false;
				thePCrouton->fHasValue_Prior = false;
				fPCroutons_Update.InsertIfNotContains(thePCrouton);
				}
			else
				{
				// It's not in use, and not known to the server, so it should have
				// been pulled from the sync list by update and deleted.
				if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSoup"))
					{
					s << "Got a PCrouton on the sync list that maybe shouldn't be there: "
						<< " On update list? "
						<< (static_cast<PCroutonUpdate*>(thePCrouton)->fNext ? "yes " : "no ")
						<< ZString::sFormat("%llX: ", thePCrouton->fID)
						<< thePCrouton->fValue_Current;
					}
				}
			}
		}

	vector<int64> removedQueries;
	vector<ZTSWatcher::AddedQueryCombo> addedQueries;

	for (ZooLib::DListIteratorEraseAll<PSieve, PSieveSync, kDebug> iter = fPSieves_Sync;
		iter; iter.Advance())
		{
		PSieve* thePSieve = iter.Current();

		if (thePSieve->fUsingTSieves)
			{
			// It's in use.
			if (!thePSieve->fServerKnown)
				{
				// But not known to the server.
				thePSieve->fServerKnown = true;

				ZTSWatcher::AddedQueryCombo theCombo;
				theCombo.fRefcon = reinterpret_cast<int64>(thePSieve);
				theCombo.fTBQuery = thePSieve->fTBQuery;
				theCombo.fPrefetch = thePSieve->fPrefetch;
				addedQueries.push_back(theCombo);
				}
			}
		else if (thePSieve->fServerKnown)
			{
			// It's not in use, and is known to the server.
			thePSieve->fServerKnown = false;
			thePSieve->fHasResults_Current = false;
			thePSieve->fHasResults_Prior = false;
			removedQueries.push_back(reinterpret_cast<int64>(thePSieve));
			fPSieves_Update.InsertIfNotContains(thePSieve);
			}
		else
			{
			// Shouldn't still be on the sync list if it's not in use and not known to the server
			if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSoup"))
				{
				s << "Got a PSieve on the sync list that maybe shouldn't be there: "
					<< " On update list? "
					<< (static_cast<PSieveUpdate*>(thePSieve)->fNext ? "yes " : "no ")
					<< ZString::sFormat("ID: %llX, value: ", reinterpret_cast<int64>(thePSieve))
					<< thePSieve->fTBQuery.AsTuple();
				}
			}
		}

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Sync releasing lock";
	locker_Structure.Release();

	vector<uint64> writtenTupleIDs;
	vector<ZTuple> writtenTuples;
	writtenTupleIDs.reserve(writtenTupleMap.size());
	writtenTuples.reserve(writtenTupleMap.size());
	for (map<uint64, ZTuple>::const_iterator i = writtenTupleMap.begin(); i != writtenTupleMap.end(); ++i)
		{
		writtenTupleIDs.push_back(i->first);
		writtenTuples.push_back(i->second);
		}

	vector<uint64> serverAddedIDs;
	vector<uint64> changedTupleIDs;
	vector<ZTuple> changedTuples;
	map<int64, vector<uint64> > changedQueries;

	fTSWatcher->DoIt(
		&removedIDs[0], removedIDs.size(),
		&addedIDs[0], addedIDs.size(),
		&removedQueries[0], removedQueries.size(),
		&addedQueries[0], addedQueries.size(),
		serverAddedIDs,
		changedTupleIDs, changedTuples,
		&writtenTupleIDs[0], &writtenTuples[0], writtenTupleIDs.size(),
		changedQueries);

	locker_Structure.Acquire();

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Sync acquired lock";

	for (vector<uint64>::iterator iterServerAddedIDs = serverAddedIDs.begin(),
		theEnd = serverAddedIDs.end();
		iterServerAddedIDs != theEnd; ++iterServerAddedIDs)
		{
		PCrouton* thePCrouton = this->pGetPCrouton(*iterServerAddedIDs);
		ZAssertStop(ZTSoup::kDebug, !thePCrouton->fServerKnown);
		thePCrouton->fServerKnown = true;
		// Stick it on the pending list, so it will last past
		// the end of the next update.
		fPCroutons_Pending.InsertIfNotContains(thePCrouton);
		}

	for (vector<uint64>::iterator iterChangedTuples = changedTupleIDs.begin(),
		theEnd = changedTupleIDs.end();
		iterChangedTuples != theEnd; ++iterChangedTuples)
		{
		map<uint64, PCrouton>::iterator iterPCrouton
			= fID_To_PCrouton.find(*iterChangedTuples);

		// We never toss a PCrouton that has not positively been unregistered.
		ZAssertStop(ZTSoup::kDebug, fID_To_PCrouton.end() != iterPCrouton);

		PCrouton* thePCrouton = &(*iterPCrouton).second;
		if (!thePCrouton->fWrittenLocally)
			{
			thePCrouton->fValue_FromServer =
				*(changedTuples.begin() + (iterChangedTuples - changedTupleIDs.begin()));

			fPCroutons_Changed.InsertIfNotContains(thePCrouton);
			}
		}

	for (map<int64, vector<uint64> >::iterator i = changedQueries.begin(),
		theEnd = changedQueries.end();
		i != theEnd; ++i)
		{
		PSieve* thePSieve = reinterpret_cast<PSieve*>((*i).first);
		thePSieve->fResults_Remote.swap((*i).second);
		fPSieves_Changed.InsertIfNotContains(thePSieve);
		}

	for (ZooLib::DListIteratorEraseAll<PCrouton, PCroutonSyncing, kDebug>
		iter = fPCroutons_Syncing;
		iter; iter.Advance())
		{
		fPCroutons_Pending.InsertIfNotContains(iter.Current());
		}

	locker_CallSync.Release();

	if (fPSieves_Update || fPSieves_Changed
		|| fPCroutons_Update || fPCroutons_Changed
		|| fPCroutons_Pending)
		{
		this->pTriggerUpdate();
		}

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Sync exit";
	}

void ZTSoup::Update()
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	fCalled_UpdateNeeded = false;

	if (!fPSieves_Update && !fPSieves_Changed
		&& !fPCroutons_Update && !fPCroutons_Changed
		&& !fPCroutons_Pending)
		{
		return;
		}

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		{
		s << "Update start";
		}

	for (ZooLib::DListIteratorEraseAll<PCrouton, PCroutonUpdate, kDebug> iter = fPCroutons_Update;
		iter; iter.Advance())
		{
		PCrouton* thePCrouton = iter.Current();

		if (thePCrouton->fWrittenLocally)
			{
			// A local change has been made, mark it so that Sync knows to
			// send the value to the server.
			thePCrouton->fWrittenLocally = false;
			thePCrouton->fHasValue_ForServer = true;
			thePCrouton->fValue_ForServer = thePCrouton->fValue_Current;
			fPCroutons_Sync.InsertIfNotContains(thePCrouton);
			}

		if (!thePCrouton->fHasValue_ForServer)
			{
			// It's not waiting to be sent to the server.
			if (thePCrouton->fUsingTCroutons)
				{
				// It's in use.
				if (!thePCrouton->fServerKnown)
					{
					// But not known to the server.
					fPCroutons_Sync.InsertIfNotContains(thePCrouton);
					}
				}
			else if (!fPCroutons_Syncing.Contains(thePCrouton)
				&& !fPCroutons_Pending.Contains(thePCrouton))
				{
				// It's not in use, and it's not syncing or pending.
				if (thePCrouton->fServerKnown)
					{
					// It's not in use, not syncing or pending, and is known to the server.
					fPCroutons_Sync.InsertIfNotContains(thePCrouton);
					}
				else
					{
					// It's not in use, not syncing or pending, is not known
					// to the server so we can toss it.
					fPCroutons_Sync.RemoveIfContains(thePCrouton);
					fPCroutons_Changed.RemoveIfContains(thePCrouton);
					if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
						s.Writef("Deleting PCrouton, ID: %llX", thePCrouton->fID);

					ZUtil_STL::sEraseMustContain(kDebug, fID_To_PCrouton, thePCrouton->fID);
					}
				}
			}
		}

	for (ZooLib::DListIteratorEraseAll<PSieve, PSieveUpdate, kDebug> iter = fPSieves_Update;
		iter; iter.Advance())
		{
		PSieve* thePSieve = iter.Current();

		if (thePSieve->fUsingTSieves)
			{
			// It's in use.
			if (!thePSieve->fServerKnown)
				{
				// But not known to the server.
				fPSieves_Sync.InsertIfNotContains(thePSieve);
				}
			}
		else if (thePSieve->fServerKnown)
			{
			// It's not in use and is known to the server.
			fPSieves_Sync.InsertIfNotContains(thePSieve);
			}
		else
			{
			// It's not in use, is not known to the server, so we can toss it.
			fPSieves_Sync.RemoveIfContains(thePSieve);
			fPSieves_Changed.RemoveIfContains(thePSieve);
			ZUtil_STL::sEraseMustContain(kDebug, fTBQuery_To_PSieve, thePSieve->fTBQuery);
			}
		}

	// Pick up remotely changed croutons.
	set<ZRef<ZTCrouton> > localTCroutons;
	for (ZooLib::DListIteratorEraseAll<PCrouton, PCroutonChanged, kDebug>
		iter = fPCroutons_Changed;
		iter; iter.Advance())
		{
		PCrouton* thePCrouton = iter.Current();

		thePCrouton->fHasValue_Prior = thePCrouton->fHasValue_Current;
		thePCrouton->fHasValue_Current = true;
		thePCrouton->fValue_Prior = thePCrouton->fValue_Current;
		thePCrouton->fValue_Current = thePCrouton->fValue_FromServer;

		for (ZooLib::DListIterator<ZTCrouton, ZTCroutonLink, kDebug>
			iter = thePCrouton->fUsingTCroutons;
			iter; iter.Advance())
			{ localTCroutons.insert(iter.Current()); }
		}

	// Pick up remotely changed sieves
	set<ZRef<ZTSieve> > localTSieves;
	for (ZooLib::DListIteratorEraseAll<PSieve, PSieveChanged, kDebug> iter = fPSieves_Changed;
		iter; iter.Advance())
		{
		PSieve* thePSieve = iter.Current();

		thePSieve->fHasResults_Prior = thePSieve->fHasResults_Current;
		thePSieve->fHasResults_Current = true;

		thePSieve->fResults_Local_Prior.swap(thePSieve->fResults_Local_Current);
		thePSieve->fResults_Remote.swap(thePSieve->fResults_Local_Current);
		thePSieve->fHasDiffs = false;
		thePSieve->fAdded.clear();
		thePSieve->fRemoved.clear();

		for (ZooLib::DListIterator<ZTSieve, ZTSieveLink, kDebug> iter = thePSieve->fUsingTSieves;
			iter; iter.Advance())
			{ localTSieves.insert(iter.Current()); }
		}

	if (fPCroutons_Pending)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
			s.Writef("Moving %d croutons from pending to update", fPCroutons_Pending.Size());

		for (ZooLib::DListIteratorEraseAll<PCrouton, PCroutonPending, kDebug>
			iter = fPCroutons_Pending;
			iter; iter.Advance())
			{
			fPCroutons_Update.InsertIfNotContains(iter.Current());
			}

		this->pTriggerUpdate();
		}

	if (!fPCroutons_Sync.Empty() || !fPSieves_Sync.Empty())
		this->pTriggerSync();

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Update releasing lock";

	locker_Structure.Release();

	for (set<ZRef<ZTSieve> >::iterator j = localTSieves.begin(), theEnd = localTSieves.end();
		j != theEnd; ++j)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
			s.Writef("Firing sieve with  %d croutons", (*j)->fPSieve->fResults_Local_Current.size());
		try
			{
			(*j)->Changed();
			}
		catch (...)
			{}
		}
	sCroutonsChanged(localTCroutons, ZTCrouton::eChange_Remote);

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		s << "Update exit";
	}

void ZTSoup::pCallback_TSWatcher(void* iRefcon)
	{
	ZTSoup* theTSoup = static_cast<ZTSoup*>(iRefcon);

	ASSERTUNLOCKED(theTSoup->fMutex_Structure);

	ZMutexLocker locker(theTSoup->fMutex_Structure);

	theTSoup->pTriggerSync();
	}

void ZTSoup::pDisposingTSieve(ZTSieve* iTSieve)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	PSieve* thePSieve = iTSieve->fPSieve;

	thePSieve->fUsingTSieves.Remove(iTSieve);

	if (!thePSieve->fUsingTSieves)
		{
		fPSieves_Update.InsertIfNotContains(thePSieve);
		this->pTriggerUpdate();
		}
	}

void ZTSoup::pDisposingTCrouton(ZTCrouton* iTCrouton)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	PCrouton* thePCrouton = iTCrouton->fPCrouton;

	if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
		{
		s.Writef("Disposing TCrouton, ID: %llX", thePCrouton->fID);
		}

	thePCrouton->fUsingTCroutons.Remove(iTCrouton);

	if (!thePCrouton->fUsingTCroutons)
		{
		fPCroutons_Update.InsertIfNotContains(thePCrouton);
		this->pTriggerUpdate();
		}
	}

bool ZTSoup::pHasCurrent(PSieve* iPSieve)
	{
	ZMutexLocker locker(fMutex_Structure);

	return iPSieve->fHasResults_Current;
	}

bool ZTSoup::pHasPrior(PSieve* iPSieve)
	{
	ZMutexLocker locker(fMutex_Structure);

	return iPSieve->fHasResults_Prior;
	}

const std::vector<uint64>& ZTSoup::pSieveGetCurrent(PSieve* iPSieve)
	{
	ZMutexLocker locker(fMutex_Structure);
	return iPSieve->fResults_Local_Current;
	}

bool ZTSoup::pSieveGetCurrent(PSieve* iPSieve, vector<uint64>& oCurrent)
	{
	ZMutexLocker locker(fMutex_Structure);

	if (iPSieve->fHasResults_Current)
		{
		oCurrent = iPSieve->fResults_Local_Current;
		return true;
		}
	return false;
	}

bool ZTSoup::pSieveGetCurrent(PSieve* iPSieve, set<uint64>& oCurrent)
	{
	ZMutexLocker locker(fMutex_Structure);

	oCurrent.clear();
	if (iPSieve->fHasResults_Current)
		{
		oCurrent.insert(iPSieve->fResults_Local_Current.begin(), iPSieve->fResults_Local_Current.end());
		return true;
		}
	return false;
	}

const std::vector<uint64>& ZTSoup::pSieveGetPrior(PSieve* iPSieve)
	{
	ZMutexLocker locker(fMutex_Structure);
	return iPSieve->fResults_Local_Prior;
	}

bool ZTSoup::pSieveGetPrior(PSieve* iPSieve, vector<uint64>& oPrior)
	{
	ZMutexLocker locker(fMutex_Structure);

	if (iPSieve->fHasResults_Prior)
		{
		oPrior = iPSieve->fResults_Local_Prior;
		return true;
		}
	return false;
	}

bool ZTSoup::pSieveGetPrior(PSieve* iPSieve, set<uint64>& oPrior)
	{
	ZMutexLocker locker(fMutex_Structure);

	oPrior.clear();
	if (iPSieve->fHasResults_Prior)
		{
		oPrior.insert(iPSieve->fResults_Local_Prior.begin(), iPSieve->fResults_Local_Prior.end());
		return true;
		}
	return false;
	}

void ZTSoup::pSieveGetAdded(PSieve* iPSieve, set<uint64>& oAdded)
	{
	ZMutexLocker locker(fMutex_Structure);

	this->pCheckSieveDiffs(iPSieve);
	oAdded = iPSieve->fAdded;
	}

void ZTSoup::pSieveGetRemoved(PSieve* iPSieve, set<uint64>& oRemoved)
	{
	ZMutexLocker locker(fMutex_Structure);

	this->pCheckSieveDiffs(iPSieve);
	oRemoved = iPSieve->fRemoved;
	}

void ZTSoup::pCheckSieveDiffs(PSieve* iPSieve)
	{
	ASSERTLOCKED(fMutex_Structure);

	if (!iPSieve->fHasDiffs)
		{
		iPSieve->fHasDiffs = true;
		ZAssertStop(kDebug, iPSieve->fAdded.empty());
		ZAssertStop(kDebug, iPSieve->fRemoved.empty());

		set<uint64> current(
			iPSieve->fResults_Local_Current.begin(), iPSieve->fResults_Local_Current.end());

		set<uint64> prior(
			iPSieve->fResults_Local_Prior.begin(), iPSieve->fResults_Local_Prior.end());

		set_difference(current.begin(), current.end(),
					prior.begin(), prior.end(),
					inserter(iPSieve->fAdded, iPSieve->fAdded.end()));

		set_difference(prior.begin(), prior.end(),
					current.begin(), current.end(),
					inserter(iPSieve->fRemoved, iPSieve->fRemoved.end()));
		}
	}

bool ZTSoup::pHasCurrent(PCrouton* iPCrouton)
	{
	ZMutexLocker locker(fMutex_Structure);

	return iPCrouton->fHasValue_Current;
	}

bool ZTSoup::pHasPrior(PCrouton* iPCrouton)
	{
	ZMutexLocker locker(fMutex_Structure);

	return iPCrouton->fHasValue_Prior;
	}

bool ZTSoup::pCroutonGetCurrent(PCrouton* iPCrouton, ZTuple& oTuple)
	{
	ZMutexLocker locker(fMutex_Structure);

	if (iPCrouton->fHasValue_Current)
		{
		oTuple = iPCrouton->fValue_Current;
		return true;
		}
	return false;
	}

bool ZTSoup::pCroutonGetPrior(PCrouton* iPCrouton, ZTuple& oTuple)
	{
	ZMutexLocker locker(fMutex_Structure);

	if (iPCrouton->fHasValue_Prior)
		{
		oTuple = iPCrouton->fValue_Prior;
		return true;
		}
	return false;
	}

ZTSoup::PCrouton* ZTSoup::pGetPCrouton(uint64 iID)
	{
	ASSERTLOCKED(fMutex_Structure);

	map<uint64, PCrouton>::iterator position = fID_To_PCrouton.lower_bound(iID);
	if (position != fID_To_PCrouton.end() && position->first == iID)
		return &position->second;

	PCrouton* thePCrouton = &fID_To_PCrouton.
		insert(position, pair<uint64, PCrouton>(iID, PCrouton()))->second;

	thePCrouton->fTSoup = this;
	thePCrouton->fID = iID;
	thePCrouton->fServerKnown = false;
	thePCrouton->fHasValue_Current = false;
	thePCrouton->fHasValue_Prior = false;
	thePCrouton->fWrittenLocally = false;
	thePCrouton->fHasValue_ForServer = false;

	return thePCrouton;
	}

void ZTSoup::pSetCroutonFromTCrouton(PCrouton* iPCrouton, const ZTuple& iTuple)
	{
	ZMutexLocker locker_CallUpdate(fMutex_CallUpdate);
	ZMutexLocker locker_Structure(fMutex_Structure);

	// There must be at least one ZTCrouton using the PCrouton, or else how were we called?
	ZAssertStop(kDebug, iPCrouton->fUsingTCroutons);
	this->pSet(locker_Structure, iPCrouton, iTuple);
	}

void ZTSoup::pSet(ZMutexLocker& iLocker_Structure, PCrouton* iPCrouton, const ZTuple& iTuple)
	{
	ASSERTLOCKED(fMutex_CallUpdate);
	ASSERTLOCKED(fMutex_Structure);

	iPCrouton->fHasValue_Prior = iPCrouton->fHasValue_Current;
	iPCrouton->fHasValue_Current = true;
	iPCrouton->fValue_Prior = iPCrouton->fValue_Current;
	iPCrouton->fValue_Current = iTuple;
	iPCrouton->fWrittenLocally = true;

	fPCroutons_Update.InsertIfNotContains(iPCrouton);

	this->pTriggerUpdate();

	set<ZRef<ZTCrouton> > localTCroutons;

	for (ZooLib::DListIterator<ZTCrouton, ZTCroutonLink, kDebug> iter = iPCrouton->fUsingTCroutons;
		iter; iter.Advance())
		{ localTCroutons.insert(iter.Current());}

	iLocker_Structure.Release();

	sCroutonsChanged(localTCroutons, ZTCrouton::eChange_Local);
	}

void ZTSoup::pTriggerUpdate()
	{
	ASSERTLOCKED(fMutex_Structure);

	if (!fCalled_UpdateNeeded)
		{
		fCalled_UpdateNeeded = true;
		if (fCallback_UpdateNeeded)
			fCallback_UpdateNeeded(fRefcon_UpdateNeeded);
		}
	}

void ZTSoup::pTriggerSync()
	{
	ASSERTLOCKED(fMutex_Structure);

	if (!fCalled_SyncNeeded)
		{
		fCalled_SyncNeeded = true;
		if (fCallback_SyncNeeded)
			fCallback_SyncNeeded(fRefcon_SyncNeeded);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTSieve

ZTSieve::ZTSieve()
	{}

ZTSieve::~ZTSieve()
	{
	if (fPSieve)
		fPSieve->DisposingTSieve(this);
	}

void ZTSieve::Changed()
	{}

bool ZTSieve::HasCurrent()
	{ return fPSieve->HasCurrent(); }

bool ZTSieve::HasPrior()
	{ return fPSieve->HasPrior(); }

const std::vector<uint64>& ZTSieve::GetCurrent()
	{ return fPSieve->GetCurrent(); }

bool ZTSieve::GetCurrent(vector<uint64>& oCurrent)
	{ return fPSieve->GetCurrent(oCurrent); }

bool ZTSieve::GetCurrent(set<uint64>& oCurrent)
	{ return fPSieve->GetCurrent(oCurrent); }

const std::vector<uint64>& ZTSieve::GetPrior()
	{ return fPSieve->GetPrior(); }

bool ZTSieve::GetPrior(vector<uint64>& oPrior)
	{ return fPSieve->GetPrior(oPrior); }

bool ZTSieve::GetPrior(set<uint64>& oPrior)
	{ return fPSieve->GetPrior(oPrior); }

void ZTSieve::GetAdded(set<uint64>& oCurrent)
	{ return fPSieve->GetAdded(oCurrent); }

void ZTSieve::GetRemoved(set<uint64>& oCurrent)
	{ return fPSieve->GetRemoved(oCurrent); }

ZRef<ZTSoup> ZTSieve::GetTSoup()
	{ return fPSieve->GetTSoup(); }

ZTBQuery ZTSieve::GetTBQuery()
	{ return fPSieve->GetTBQuery(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTCrouton

ZTCrouton::ZTCrouton()
	{}

ZTCrouton::~ZTCrouton()
	{
	if (fPCrouton)
		fPCrouton->DisposingTCrouton(this);
	}

void ZTCrouton::Changed(EChange iChange)
	{}

uint64 ZTCrouton::GetID()
	{ return fPCrouton->GetID(); }

bool ZTCrouton::HasCurrent()
	{ return fPCrouton->HasCurrent(); }

bool ZTCrouton::HasPrior()
	{ return fPCrouton->HasPrior(); }

bool ZTCrouton::GetCurrent(ZTuple& oTuple)
	{ return fPCrouton->GetCurrent(oTuple); }

ZTuple ZTCrouton::GetCurrent()
	{
	ZTuple result;
	fPCrouton->GetCurrent(result);
	return result;
	}

bool ZTCrouton::GetPrior(ZTuple& oTuple)
	{ return fPCrouton->GetPrior(oTuple); }

ZTuple ZTCrouton::GetPrior()
	{
	ZTuple result;
	fPCrouton->GetPrior(result);
	return result;
	}

void ZTCrouton::Set(const ZTuple& iTuple)
	{ fPCrouton->Set(iTuple); }

ZRef<ZTSoup> ZTCrouton::GetTSoup()
	{ return fPCrouton->GetTSoup(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZTSoup::PSieve

void ZTSoup::PSieve::DisposingTSieve(ZTSieve* iTSieve)
	{ fTSoup->pDisposingTSieve(iTSieve); }

bool ZTSoup::PSieve::HasCurrent()
	{ return fTSoup->pHasCurrent(this); }

bool ZTSoup::PSieve::HasPrior()
	{ return fTSoup->pHasPrior(this); }

const vector<uint64>& ZTSoup::PSieve::GetCurrent()
	{ return fTSoup->pSieveGetCurrent(this); }

bool ZTSoup::PSieve::GetCurrent(vector<uint64>& oCurrent)
	{ return fTSoup->pSieveGetCurrent(this, oCurrent); }

bool ZTSoup::PSieve::GetCurrent(set<uint64>& oCurrent)
	{ return fTSoup->pSieveGetCurrent(this, oCurrent); }

const vector<uint64>& ZTSoup::PSieve::GetPrior()
	{ return fTSoup->pSieveGetPrior(this); }

bool ZTSoup::PSieve::GetPrior(vector<uint64>& oCurrent)
	{ return fTSoup->pSieveGetPrior(this, oCurrent); }

bool ZTSoup::PSieve::GetPrior(set<uint64>& oCurrent)
	{ return fTSoup->pSieveGetPrior(this, oCurrent); }

void ZTSoup::PSieve::GetAdded(set<uint64>& oAdded)
	{ fTSoup->pSieveGetAdded(this, oAdded); }

void ZTSoup::PSieve::GetRemoved(set<uint64>& oRemoved)
	{ fTSoup->pSieveGetRemoved(this, oRemoved); }

ZRef<ZTSoup> ZTSoup::PSieve::GetTSoup()
	{ return fTSoup; }

ZTBQuery ZTSoup::PSieve::GetTBQuery()
	{ return fTBQuery; }

// =================================================================================================
#pragma mark -
#pragma mark * ZTSoup::PCrouton

void ZTSoup::PCrouton::DisposingTCrouton(ZTCrouton* iTCrouton)
	{ fTSoup->pDisposingTCrouton(iTCrouton); }

uint64 ZTSoup::PCrouton::GetID()
	{ return fID; }

bool ZTSoup::PCrouton::HasCurrent()
	{ return fTSoup->pHasCurrent(this); }

bool ZTSoup::PCrouton::HasPrior()
	{ return fTSoup->pHasPrior(this); }
	
bool ZTSoup::PCrouton::GetCurrent(ZTuple& oTuple)
	{ return fTSoup->pCroutonGetCurrent(this, oTuple); }

bool ZTSoup::PCrouton::GetPrior(ZTuple& oTuple)
	{ return fTSoup->pCroutonGetPrior(this, oTuple); }

void ZTSoup::PCrouton::Set(const ZTuple& iTuple)
	{ fTSoup->pSetCroutonFromTCrouton(this, iTuple); }

ZRef<ZTSoup> ZTSoup::PCrouton::GetTSoup()
	{ return fTSoup; }

// =================================================================================================
#pragma mark -
#pragma mark * ZTCrouton_Bowl

ZTCrouton_Bowl::ZTCrouton_Bowl()
:	fTBowl(nil)
	{}

void ZTCrouton_Bowl::Changed(EChange iChange)
	{
	if (fTBowl)
		fTBowl->CroutonChanged(this, iChange);
	}

ZRef<ZTBowl> ZTCrouton_Bowl::GetBowl() const
	{ return fTBowl; }

// =================================================================================================
#pragma mark -
#pragma mark * ZTBowl

ZTBowl::ZTBowl()
	{}

ZTBowl::~ZTBowl()
	{
	for (vector<ZRef<ZTCrouton> >::iterator i = fTCroutons.begin(); i != fTCroutons.end(); ++i)
		ZRefStaticCast<ZTCrouton_Bowl>(*i)->fTBowl = nil;
	fTCroutons.clear();
	}

void ZTBowl::Changed()
	{
	set<uint64> removedIDs;
	this->GetRemoved(removedIDs);

	for (vector<ZRef<ZTCrouton> >::iterator i = fTCroutons.begin();
		i != fTCroutons.end(); /*no increment*/)
		{
		if (ZUtil_STL::sContains(removedIDs, (*i)->GetID()))
			{
			ZRef<ZTCrouton> theTCrouton = *i;
			if (const ZLog::S& s = ZLog::S(ZLog::eDebug + 1, "ZTSoup"))
				{
				s << "Removing TCrouton, ID: " << ZString::sFormat("%llX", theTCrouton->GetID())
					<< ", Address: " << ZString::sFormat("%X", theTCrouton.GetObject())
					<< ", Refcount: " << ZString::sFormat("%d", theTCrouton->GetRefCount());
				}

			ZRefStaticCast<ZTCrouton_Bowl>(*i)->fTBowl = nil;
			i = fTCroutons.erase(i);
			}
		else
			{
			++i;
			}
		}

	set<uint64> addedIDs;
	this->GetAdded(addedIDs);
	for (set<uint64>::const_iterator i = addedIDs.begin(); i != addedIDs.end(); ++i)
		{
		ZRef<ZTCrouton_Bowl> newCrouton = this->MakeCrouton();
		newCrouton->fTBowl = this;
		fTCroutons.push_back(newCrouton);
		this->GetTSoup()->Register(newCrouton, *i);
		}
	}

ZRef<ZTCrouton_Bowl> ZTBowl::MakeCrouton()
	{ return new ZTCrouton_Bowl; }

void ZTBowl::CroutonChanged(ZRef<ZTCrouton> iCrouton, ZTCrouton::EChange iChange)
	{}

const vector<ZRef<ZTCrouton> >& ZTBowl::GetCroutons()
	{ return fTCroutons; }
