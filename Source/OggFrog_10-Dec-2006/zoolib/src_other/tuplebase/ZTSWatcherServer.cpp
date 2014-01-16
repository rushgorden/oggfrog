static const char rcsid[] = "@(#) $Id: ZTSWatcherServer.cpp,v 1.20 2006/11/03 04:13:11 agreen Exp $";

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

#include "ZTSWatcherServer.h"

#include "ZDebug.h"
#include "ZLog.h"
#include "ZMemoryBlock.h"
#include "ZStream_Count.h"
#include "ZStream_Tee.h"
#include "ZUtil_Tuple.h"

using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

#define kDebug_TSWatcherServer 1

#ifdef ZCONFIG_TSWatcherServer_ShowCommsTuples
#	define kDebug_ShowCommsTuples ZCONFIG_TSWatcherServer_ShowCommsTuples
#else
#	define kDebug_ShowCommsTuples 0
#endif

#ifdef ZCONFIG_TSWatcherServer_ShowTimes
#	define kDebug_ShowTimes ZCONFIG_TSWatcherServer_ShowTimes
#else
#	define kDebug_ShowTimes 0
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Static helpers

static inline void sWriteCount(const ZStreamW& iStreamW, uint32 iCount)
	{
	if (iCount < 0xFF)
		{
		iStreamW.WriteUInt8(iCount);
		}
	else
		{
		iStreamW.WriteUInt8(0xFF);
		iStreamW.WriteUInt32(iCount);
		}
	}

static inline uint32 sReadCount(const ZStreamR& iStreamR)
	{
	uint8 firstByte = iStreamR.ReadUInt8();
	if (firstByte < 0xFF)
		return firstByte;
	return iStreamR.ReadUInt32();
	}

static void sDumpRequest(ZRef<ZTSWatcher> iWatcher, const ZTuple& iTuple)
	{
#if kDebug_ShowCommsTuples
	if (const ZLog::S& s = ZLog::S(ZLog::eDebug, "ZTSWatcherServer"))
		{
		s.Writef(">> ZTSWatcherServer: %08X ", iWatcher.GetObject());
		ZUtil_Tuple::sToStrim(s, iTuple);
		s.Write("\n");
		}
#endif
	}

static void sSendResponse(ZRef<ZTSWatcher> iWatcher, const ZStreamW& iStreamW, const ZTuple& iTuple)
	{
#if kDebug_ShowCommsTuples
	if (const ZLog::S& s = ZLog::S(ZLog::eDebug, "ZTSWatcherServer"))
		{
		s.Writef("<< ZTSWatcherServer: %08X ", iWatcher.GetObject());
		ZUtil_Tuple::sToStrim(s, iTuple);
		s.Write("\n");
		}
#endif
	iTuple.ToStream(iStreamW);
	}

static void sAllocateIDs(ZRef<ZTSWatcher> iWatcher, const ZTuple& iReq, const ZStreamW& iStreamW)
	{
	size_t theCount = iReq.GetInt32("Count");
	uint64 baseID;
	size_t countIssued;
	iWatcher->AllocateIDs(theCount, baseID, countIssued);
	ZTuple response;
	response.SetInt64("BaseID", baseID);
	response.SetInt32("Count", countIssued);
	sSendResponse(iWatcher, iStreamW, response);
	}


// =================================================================================================
#pragma mark -
#pragma mark * sDoIt common code

// ZTSWatcher now requires that the tuples passed to it are sorted by ID, so it
// can efficiently identify overlaps between the set written by a client, and
// the set that has been written by others. Previously we'd passed a set<uint64>,
// for which that operation is obviously already efficient.

static void sSort(vector<uint64>& ioWrittenTupleIDs, vector<ZTuple>& ioWrittenTuples)
	{
	vector<uint64> newIDs;
	vector<ZTuple> newTuples;
	// Do a lower_bound insertion into newIDs, and
	// to equivalent offset of newTuples.
	for (vector<uint64>::iterator i = ioWrittenTupleIDs.begin(); i != ioWrittenTupleIDs.end(); ++i)
		{
		vector<uint64>::iterator pos = lower_bound(newIDs.begin(), newIDs.end(), *i);

		newTuples.insert(newTuples.begin() + (pos - newIDs.begin()),
			ioWrittenTuples[i - ioWrittenTupleIDs.begin()]);

		newIDs.insert(pos, *i);
		}
	ioWrittenTupleIDs.swap(newIDs);
	ioWrittenTuples.swap(newTuples);
	}

// =================================================================================================
#pragma mark -
#pragma mark * sDoIt1

// Unpacks a request in a ZTuple, and returns the results in a tuple.

static void sDoIt1(double iReadTime, ZRef<ZTSWatcher> iWatcher, const ZTuple& iReq, const ZStreamW& iStreamW)
	{
	ZTime start = ZTime::sSystem();

	
	vector<uint64> removedIDs;
	iReq.GetVector_T("removedIDs", back_inserter(removedIDs), uint64());


	vector<uint64> addedIDs;
	iReq.GetVector_T("addedIDs", back_inserter(addedIDs), uint64());


	vector<int64> removedQueries;
	iReq.GetVector_T("removedQueries", back_inserter(removedQueries), int64());


	vector<ZTSWatcher::AddedQueryCombo> addedQueries;
	{
	const vector<ZTupleValue>& addedQueriesV = iReq.GetVector("addedQueries");
	if (size_t theCount = addedQueriesV.size())
		{
		addedQueries.reserve(theCount);
		for (vector<ZTupleValue>::const_iterator i = addedQueriesV.begin(), theEnd = addedQueriesV.end();
			i != theEnd; ++i)
			{
			const ZTuple& entry = (*i).GetTuple();
			int64 theRefcon;
			if (entry.GetInt64("refcon", theRefcon))
				{
				ZTuple queryAsTuple;
				if (entry.GetTuple("query", queryAsTuple))
					{
					ZTSWatcher::AddedQueryCombo theCombo;
					theCombo.fRefcon = theRefcon;
					theCombo.fTBQuery = ZTBQuery(queryAsTuple);
					theCombo.fTBQuery.ToStream(ZStreamRWPos_MemoryBlock(theCombo.fMemoryBlock));
					theCombo.fPrefetch = entry.GetBool("prefetch");
					addedQueries.push_back(theCombo);
					}
				}
			}
		}
	}


	vector<uint64> writtenTupleIDs;
	vector<ZTuple> writtenTuples;
	bool writeNeededSort = false;
	{
	const vector<ZTupleValue>& writtenTuplesV = iReq.GetVector("writtenTuples");
	if (size_t theCount = writtenTuplesV.size())
		{
		writtenTupleIDs.reserve(theCount);
		writtenTuples.reserve(theCount);
		uint64 lastID = 0;
		for (vector<ZTupleValue>::const_iterator i = writtenTuplesV.begin(), theEnd = writtenTuplesV.end();
			i != theEnd; ++i)
			{
			const ZTuple& entry = (*i).GetTuple();
			uint64 theID;
			if (entry.GetID("ID", theID))
				{
				ZTuple theTuple;
				if (entry.GetTuple("tuple", theTuple))
					{
					if (lastID >= theID)
						writeNeededSort = true;
					lastID = theID;
					
					writtenTupleIDs.push_back(theID);
					writtenTuples.push_back(theTuple);
					}
				}
			}
		if (writeNeededSort)
			sSort(writtenTupleIDs, writtenTuples);
		}
	}


	vector<uint64> serverAddedIDs;
	vector<uint64> changedTupleIDs;
	vector<ZTuple> changedTuples;
	map<int64, vector<uint64> > changedQueries;

	ZTime beforeDoIt = ZTime::sSystem();

	iWatcher->DoIt(
		&removedIDs[0], removedIDs.size(),
		&addedIDs[0], addedIDs.size(),
		&removedQueries[0], removedQueries.size(),
		&addedQueries[0], addedQueries.size(),
		serverAddedIDs,
		changedTupleIDs, changedTuples,
		&writtenTupleIDs[0], &writtenTuples[0], writtenTupleIDs.size(),
		changedQueries);

	ZTime afterDoIt = ZTime::sSystem();

	ZTuple response;

	if (size_t theCount = serverAddedIDs.size())
		response.SetVector_T("addedTuples", serverAddedIDs.begin(), serverAddedIDs.end(), uint64());

	if (size_t theCount = changedTupleIDs.size())
		{
		ZAssertStop(kDebug_TSWatcherServer, changedTupleIDs.size() == changedTuples.size());
		vector<ZTupleValue>& changedTuplesV = response.SetMutableVector("changedTuples");
		changedTuplesV.reserve(theCount);
		ZTuple temp;
		vector<ZTuple>::const_iterator iterCT = changedTuples.begin();
		for (vector<uint64>::const_iterator i = changedTupleIDs.begin(), theEnd = changedTupleIDs.end();
			i != theEnd; ++i, ++iterCT)
			{
			temp.SetID("ID", *i);
			temp.SetTuple("tuple", *iterCT);
			changedTuplesV.push_back(temp);
			}
		}

	ZTime afterChangedTuples = ZTime::sSystem();
	
	if (size_t theCount = changedQueries.size())
		{
		vector<ZTupleValue>& changedQueriesV = response.SetMutableVector("changedQueries");
		changedQueriesV.reserve(theCount);
		ZTuple temp;
		for (map<int64, vector<uint64> >::iterator i = changedQueries.begin(), theEnd = changedQueries.end();
			i != theEnd; ++i)
			{
			temp.SetInt64("refcon", (*i).first);
			temp.SetVector_T("IDs", (*i).second.begin(), (*i).second.end());
			changedQueriesV.push_back(temp);
			}
		}
	
	ZTime beforeSend = ZTime::sSystem();

	uint64 bytesWritten;
	if (kDebug_ShowTimes)
		sSendResponse(iWatcher, ZStreamW_Count(bytesWritten, iStreamW), response);
	else
		sSendResponse(iWatcher, iStreamW, response);

	ZTime afterSend = ZTime::sSystem();

	if (kDebug_ShowTimes)
		{	
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcherServer"))
			{
			bool isEmpty =
				removedIDs.empty()
				&& addedIDs.empty()
				&& removedQueries.empty()
				&& writtenTuples.empty()
				&& addedQueries.empty()
				&& serverAddedIDs.empty()
				&& changedTuples.empty()
				&& changedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";

			const char sortChar = writeNeededSort ? '!' : 'w';

			s.Writef("1 %7.3fr %7.3fe %7.3fd %7.3fpt %7.3fpq %7.3fs - %3dt- %3dt+ %3dq- %3dt%c %3dq+ %3dt+ %3d~t %3d~q - %6d",
				1000*iReadTime,
				1000*(beforeDoIt-start),
				1000*(afterDoIt-beforeDoIt),
				1000*(afterChangedTuples-afterDoIt),
				1000*(beforeSend-afterChangedTuples),
				1000*(afterSend-beforeSend),
				removedIDs.size(),
				addedIDs.size(), 
				removedQueries.size(),
				writtenTuples.size(),
				sortChar,
				addedQueries.size(),
				serverAddedIDs.size(),
				changedTuples.size(),
				changedQueries.size(),
				size_t(bytesWritten)
				);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * sDoIt2, sDoit3 and sDoit4 common code

static void sDoIt_Initial(const ZStreamR& iStreamR,
	vector<uint64>& oRemovedIDs,
	vector<uint64>& oAddedIDs,
	vector<int64>& oRemovedQueries)
	{
	if (uint32 theCount = sReadCount(iStreamR))
		{
		oRemovedIDs.reserve(theCount);
		while (theCount--)
			oRemovedIDs.push_back(iStreamR.ReadUInt64());
		}


	if (uint32 theCount = sReadCount(iStreamR))
		{
		oAddedIDs.reserve(theCount);
		while (theCount--)
			oAddedIDs.push_back(iStreamR.ReadUInt64());
		}


	if (uint32 theCount = sReadCount(iStreamR))
		{
		oRemovedQueries.reserve(theCount);
		while (theCount--)
			oRemovedQueries.push_back(iStreamR.ReadInt64());
		}
	}
	

static bool	sDoIt_NextBit(const ZStreamR& iStreamR,
	vector<uint64>& oWrittenTupleIDs,
	vector<ZTuple>& oWrittenTuples)
	{
	bool writeNeededSort = false;
	if (uint32 theCount = sReadCount(iStreamR))
		{
		oWrittenTupleIDs.reserve(theCount);
		oWrittenTuples.reserve(theCount);
		uint64 lastID = 0;
		while (theCount--)
			{
			uint64 currentID = iStreamR.ReadUInt64();
			if (lastID >= currentID)
				writeNeededSort = true;
			lastID = currentID;

			oWrittenTupleIDs.push_back(currentID);
			oWrittenTuples.push_back(ZTuple(iStreamR));
			}

		if (writeNeededSort)
			sSort(oWrittenTupleIDs, oWrittenTuples);
		}
	return writeNeededSort;
	}

static void sDoIt_LastBit(const ZStreamW& iStreamW,
	const vector<uint64>& iChangedTupleIDs, const vector<ZTuple>& iChangedTuples,
	const map<int64, vector<uint64> >& iChangedQueries)
	{
	ZAssertStop(kDebug_TSWatcherServer, iChangedTupleIDs.size() == iChangedTuples.size());

	sWriteCount(iStreamW, iChangedTupleIDs.size());
	
	vector<ZTuple>::const_iterator iterCT = iChangedTuples.begin();
	for (vector<uint64>::const_iterator i = iChangedTupleIDs.begin(), theEnd = iChangedTupleIDs.end();
		i != theEnd; ++i, ++iterCT)
		{
		iStreamW.WriteUInt64(*i);
		(*iterCT).ToStream(iStreamW);
		}
	
	sWriteCount(iStreamW, iChangedQueries.size());
	for (map<int64, vector<uint64> >::const_iterator i = iChangedQueries.begin(), theEnd = iChangedQueries.end();
		i != theEnd; ++i)
		{
		iStreamW.WriteInt64((*i).first);
		const vector<uint64>& theVector = (*i).second;
		sWriteCount(iStreamW, theVector.size());
		for (vector<uint64>::const_iterator j = theVector.begin(); j != theVector.end(); ++j)
			iStreamW.WriteUInt64(*j);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * sDoIt2

static void sDoIt2(ZRef<ZTSWatcher> iWatcher, const ZStreamR& iStreamR, const ZStreamW& iStreamW)
	{
	ZTime start = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowTimes
		ZStreamR_Count theStreamR(bytesRead, iStreamR);
	#else
		const ZStreamR& theStreamR = iStreamR;
	#endif

	vector<uint64> removedIDs;
	vector<uint64> addedIDs;
	vector<int64> removedQueries;
	sDoIt_Initial(theStreamR, removedIDs, addedIDs, removedQueries);


	vector<ZTSWatcher::AddedQueryCombo> addedQueries;
	if (uint32 theCount = sReadCount(theStreamR))
		{
		addedQueries.reserve(theCount);
		while (theCount--)
			{
			ZTSWatcher::AddedQueryCombo theCombo;
			theCombo.fRefcon = theStreamR.ReadInt64();

			// This next line instantiates a ZTBQuery from theStreamR and assigns it to fTBQuery.
			// It pulls it from a ZStreamR_Tee that's reading from theStreamR, and writing to
			// a ZStreamRWPos_MemoryBlock whose output is fMemoryBlock.
			theCombo.fTBQuery = ZTBQuery(ZStreamR_Tee(
				theStreamR, ZStreamRWPos_MemoryBlock(theCombo.fMemoryBlock)));

			theCombo.fPrefetch = false;
			addedQueries.push_back(theCombo);
			}
		}

	
	vector<uint64> writtenTupleIDs;
	vector<ZTuple> writtenTuples;
	bool writeNeededSort = sDoIt_NextBit(theStreamR, writtenTupleIDs, writtenTuples);


	vector<uint64> serverAddedIDs;
	vector<uint64> changedTupleIDs;
	vector<ZTuple> changedTuples;
	map<int64, vector<uint64> > changedQueries;

	ZTime beforeDoIt = ZTime::sSystem();

	iWatcher->DoIt(
		&removedIDs[0], removedIDs.size(),
		&addedIDs[0], addedIDs.size(),
		&removedQueries[0], removedQueries.size(),
		&addedQueries[0], addedQueries.size(),
		serverAddedIDs,
		changedTupleIDs, changedTuples,
		&writtenTupleIDs[0], &writtenTuples[0], writtenTupleIDs.size(),
		changedQueries);

	ZTime afterDoIt = ZTime::sSystem();

	// We can't report server added IDs, so we don't mark any
	// queries as prefetched, so this must always be empty.
	ZAssertStop(kDebug_TSWatcherServer, serverAddedIDs.empty());

	uint64 bytesWritten;
	#if kDebug_ShowTimes
		ZStreamW_Count theStreamW(bytesWritten, iStreamW);
	#else
		const ZStreamW& theStreamW = iStreamW;
	#endif

	sDoIt_LastBit(theStreamW, changedTupleIDs, changedTuples, changedQueries);

	ZTime afterSend = ZTime::sSystem();

	if (kDebug_ShowTimes)
		{	
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcherServer"))
			{
			bool isEmpty =
				removedIDs.empty()
				&& addedIDs.empty()
				&& removedQueries.empty()
				&& writtenTuples.empty()
				&& addedQueries.empty()
				&& changedTuples.empty()
				&& changedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";

			s.Writef("2 %7.3fr %7.3fd %7.3fs - %3dt- %3dt+ %3dq- %3dtw %3dq+ %3d~t %3d~q - %6d< %6d>",
				1000*(beforeDoIt-start),
				1000*(afterDoIt-beforeDoIt),
				1000*(afterSend-afterDoIt),
				removedIDs.size(),
				addedIDs.size(), 
				removedQueries.size(),
				writtenTuples.size(),
				addedQueries.size(),
				changedTuples.size(),
				changedQueries.size(),
				size_t(bytesRead),
				size_t(bytesWritten)
				);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * sDoIt3

static void sDoIt3(ZRef<ZTSWatcher> iWatcher, const ZStreamR& iStreamR, const ZStreamW& iStreamW)
	{
	ZTime start = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowTimes
		ZStreamR_Count theStreamR(bytesRead, iStreamR);
	#else
		const ZStreamR& theStreamR = iStreamR;
	#endif

	vector<uint64> removedIDs;
	vector<uint64> addedIDs;
	vector<int64> removedQueries;
	sDoIt_Initial(theStreamR, removedIDs, addedIDs, removedQueries);

	// This is where we differ from sDoIt2 -- the caller will have prefixed the
	// each serialiazed ZTBQuery with a count of how many bytes follow. So we
	// do not have to interpret it to read it, so we may be able to avoid
	// interpreting it at all, if iWatcher currently has this query in use --
	// it will use the bytes in the ZMemoryBlock as the key to find the query.

	vector<ZTSWatcher::AddedQueryCombo> addedQueries;
	if (uint32 theCount = sReadCount(theStreamR))
		{
		addedQueries.reserve(theCount);
		while (theCount--)
			{
			int64 theRefcon = theStreamR.ReadInt64();
			size_t theSize = sReadCount(theStreamR);

			ZTSWatcher::AddedQueryCombo theCombo(theSize);
			theCombo.fRefcon = theRefcon;
			theCombo.fPrefetch = false;

			theStreamR.Read(theCombo.fMemoryBlock.GetData(), theSize);

			addedQueries.push_back(theCombo);
			}
		}

	
	vector<uint64> writtenTupleIDs;
	vector<ZTuple> writtenTuples;
	bool writeNeededSort = sDoIt_NextBit(theStreamR, writtenTupleIDs, writtenTuples);

	
	vector<uint64> serverAddedIDs;
	vector<uint64> changedTupleIDs;
	vector<ZTuple> changedTuples;
	map<int64, vector<uint64> > changedQueries;

	ZTime beforeDoIt = ZTime::sSystem();

	iWatcher->DoIt(
		&removedIDs[0], removedIDs.size(),
		&addedIDs[0], addedIDs.size(),
		&removedQueries[0], removedQueries.size(),
		&addedQueries[0], addedQueries.size(),
		serverAddedIDs,
		changedTupleIDs, changedTuples,
		&writtenTupleIDs[0], &writtenTuples[0], writtenTupleIDs.size(),
		changedQueries);

	ZTime afterDoIt = ZTime::sSystem();

	// We can't report server added IDs, so we don't mark any
	// queries as prefetched, so this must always be empty.
	ZAssertStop(kDebug_TSWatcherServer, serverAddedIDs.empty());

	uint64 bytesWritten;
	#if kDebug_ShowTimes
		ZStreamW_Count theStreamW(bytesWritten, iStreamW);
	#else
		const ZStreamW& theStreamW = iStreamW;
	#endif

	sDoIt_LastBit(theStreamW, changedTupleIDs, changedTuples, changedQueries);

	ZTime afterSend = ZTime::sSystem();

	if (kDebug_ShowTimes)
		{	
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcherServer"))
			{
			bool isEmpty =
				removedIDs.empty()
				&& addedIDs.empty()
				&& removedQueries.empty()
				&& writtenTuples.empty()
				&& addedQueries.empty()
				&& changedTuples.empty()
				&& changedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";

			const char sortChar = writeNeededSort ? '!' : 'w';

			s.Writef("3 %7.3fr %7.3fd %7.3fs - %3dt- %3dt+ %3dq- %3dt%c %3dq+ %3d~t %3d~q - %6d< %6d>",
				1000*(beforeDoIt-start),
				1000*(afterDoIt-beforeDoIt),
				1000*(afterSend-afterDoIt),
				removedIDs.size(),
				addedIDs.size(), 
				removedQueries.size(),
				writtenTuples.size(),
				sortChar,
				addedQueries.size(),
				changedTuples.size(),
				changedQueries.size(),
				size_t(bytesRead),
				size_t(bytesWritten)
				);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * sDoIt4

static void sDoIt4(ZRef<ZTSWatcher> iWatcher, const ZStreamR& iStreamR, const ZStreamW& iStreamW)
	{
	ZTime start = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowTimes
		ZStreamR_Count theStreamR(bytesRead, iStreamR);
	#else
		const ZStreamR& theStreamR = iStreamR;
	#endif

	vector<uint64> removedIDs;
	vector<uint64> addedIDs;
	vector<int64> removedQueries;
	sDoIt_Initial(theStreamR, removedIDs, addedIDs, removedQueries);

	// This is where our request parsinsg differs from sDoIt3.
	// The caller can mark a query as being prefetched (see line marked <--).

	vector<ZTSWatcher::AddedQueryCombo> addedQueries;
	if (uint32 theCount = sReadCount(theStreamR))
		{
		addedQueries.reserve(theCount);
		while (theCount--)
			{
			int64 theRefcon = theStreamR.ReadInt64();
			bool thePrefetch = theStreamR.ReadBool(); // <--
			size_t theSize = sReadCount(theStreamR);

			ZTSWatcher::AddedQueryCombo theCombo(theSize);
			theCombo.fRefcon = theRefcon;
			theCombo.fPrefetch = thePrefetch; 

			theStreamR.Read(theCombo.fMemoryBlock.GetData(), theSize);

			addedQueries.push_back(theCombo);
			}
		}

	
	vector<uint64> writtenTupleIDs;
	vector<ZTuple> writtenTuples;
	bool writeNeededSort = sDoIt_NextBit(theStreamR, writtenTupleIDs, writtenTuples);

	
	vector<uint64> serverAddedIDs;
	vector<uint64> changedTupleIDs;
	vector<ZTuple> changedTuples;
	map<int64, vector<uint64> > changedQueries;

	ZTime beforeDoIt = ZTime::sSystem();

	iWatcher->DoIt(
		&removedIDs[0], removedIDs.size(),
		&addedIDs[0], addedIDs.size(),
		&removedQueries[0], removedQueries.size(),
		&addedQueries[0], addedQueries.size(),
		serverAddedIDs,
		changedTupleIDs, changedTuples,
		&writtenTupleIDs[0], &writtenTuples[0], writtenTupleIDs.size(),
		changedQueries);

	ZTime afterDoIt = ZTime::sSystem();

	uint64 bytesWritten;
	#if kDebug_ShowTimes
		ZStreamW_Count theStreamW(bytesWritten, iStreamW);
	#else
		const ZStreamW& theStreamW = iStreamW;
	#endif

	// And this is the other change, where we send the list of
	// server-added tuple IDs. IDs that have shown up in a query
	// marked as prefetch, which the client has not registered with
	// us, and which we now consider to be registered. Their
	// actual content will be included with changedTupleIDs and changedTuples.

	sWriteCount(theStreamW, serverAddedIDs.size());
	for (vector<uint64>::const_iterator i = serverAddedIDs.begin(), theEnd = serverAddedIDs.end();
		i != theEnd; ++i)
		{
		theStreamW.WriteUInt64(*i);
		}

	// From here on we're the same as sDoIt3.
	sDoIt_LastBit(theStreamW, changedTupleIDs, changedTuples, changedQueries);

	ZTime afterSend = ZTime::sSystem();

	if (kDebug_ShowTimes)
		{	
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcherServer"))
			{
			bool isEmpty =
				removedIDs.empty()
				&& addedIDs.empty()
				&& removedQueries.empty()
				&& writtenTuples.empty()
				&& addedQueries.empty()
				&& serverAddedIDs.empty()
				&& changedTuples.empty()
				&& changedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";

			const char sortChar = writeNeededSort ? '!' : 'w';

			s.Writef("4 %7.3fr %7.3fd %7.3fs - %3dt- %3dt+ %3dq- %3dt%c %3dq+ %3dt+ %3d~t %3d~q - %6d< %6d>",
				1000*(beforeDoIt-start),
				1000*(afterDoIt-beforeDoIt),
				1000*(afterSend-afterDoIt),
				removedIDs.size(),
				addedIDs.size(), 
				removedQueries.size(),
				writtenTuples.size(),
				sortChar,
				addedQueries.size(),
				serverAddedIDs.size(),
				changedTuples.size(),
				changedQueries.size(),
				size_t(bytesRead),
				size_t(bytesWritten)
				);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTSWatcherServer

ZTSWatcherServer::ZTSWatcherServer(ZRef<ZTSWatcher> iWatcher)
:	fWatcher(iWatcher)
	{}

void ZTSWatcherServer::Run(const ZStreamR& iStreamR, const ZStreamW& iStreamW)
	{
	double readTime = 0;
	for (;;)
		{
		#if kDebug_ShowTimes
			ZStreamU_Unreader theStreamU(iStreamR);
			theStreamU.ReadInt8();
			theStreamU.Unread();
			ZTime before = ZTime::sSystem();
			ZTuple theReq(theStreamU);
			readTime = ZTime::sSystem() - before;
		#else
			ZTuple theReq(iStreamR);
		#endif

		sDumpRequest(fWatcher, theReq);

		string theWhat;
		if (theReq.GetString("What", theWhat))
			{
			if ("AllocateIDs" == theWhat)
				{
				sAllocateIDs(fWatcher, theReq, iStreamW);
				}
			else if ("DoIt" == theWhat)
				{
				sDoIt1(readTime, fWatcher, theReq, iStreamW);
				}
			else if ("NewDoIt" == theWhat || "DoIt2" == theWhat)
				{
				sDoIt2(fWatcher, iStreamR, iStreamW);
				}
			else if ("DoIt3" == theWhat)
				{
				sDoIt3(fWatcher, iStreamR, iStreamW);
				}
			else if ("DoIt4" == theWhat)
				{
				sDoIt4(fWatcher, iStreamR, iStreamW);
				}
			else if ("Close" == theWhat)
				{
				break;
				}
			}		
		}
	}
