static const char rcsid[] = "@(#) $Id: ZTSWatcherClient.cpp,v 1.17 2006/11/02 22:48:15 agreen Exp $";

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

#include "ZTSWatcherClient.h"

#include "ZDebug.h"
#include "ZLog.h"
#include "ZMemoryBlock.h"
#include "ZStream_Count.h"
#include "ZUtil_STL.h"
#include "ZUtil_Tuple.h"

using std::map;
using std::pair;
using std::set;
using std::vector;

#define kDebug_TSWatcherClient 1

#ifdef ZCONFIG_TSWatcherClient_ShowCommsTuples
#	define kDebug_ShowCommsTuples ZCONFIG_TSWatcherClient_ShowCommsTuples
#else
#	define kDebug_ShowCommsTuples 0
#endif

#ifdef ZCONFIG_TSWatcherClient_ShowStats
#	define kDebug_ShowStats ZCONFIG_TSWatcherClient_ShowStats
#else
#	define kDebug_ShowStats 0
#endif

// =================================================================================================
#pragma mark -
#pragma mark * Helper functions

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

// =================================================================================================
#pragma mark -
#pragma mark * ZTSWatcher_Client

ZTSWatcher_Client::ZTSWatcher_Client(bool iSupports2, bool iSupports3, bool iSupports4, const ZStreamR& iStreamR, const ZStreamW& iStreamW)
:	fSupports2(iSupports2),
	fSupports3(iSupports3),
	fSupports4(iSupports4),
	fStreamR(iStreamR),
	fStreamW(iStreamW)
	{}

ZTSWatcher_Client::~ZTSWatcher_Client()
	{
	ZMutexLocker locker(fMutex);
	ZTuple request;
	request.SetString("What", "Close");
	request.ToStream(fStreamW);
	}

void ZTSWatcher_Client::DoIt(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oAddedIDs,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	oAddedIDs.clear();
	if (fSupports4)
		{
		this->pDoIt4(
			iRemovedIDs, iRemovedIDsCount,
			iAddedIDs, iAddedIDsCount,
			iRemovedQueries, iRemovedQueriesCount,
			iAddedQueries, iAddedQueriesCount,
			oAddedIDs,
			oChangedTupleIDs, oChangedTuples,
			iWrittenTupleIDs, iWrittenTuples, iWrittenTuplesCount,
			oChangedQueries);
		}
	else if (fSupports3)
		{
		this->pDoIt3(
			iRemovedIDs, iRemovedIDsCount,
			iAddedIDs, iAddedIDsCount,
			iRemovedQueries, iRemovedQueriesCount,
			iAddedQueries, iAddedQueriesCount,
			oChangedTupleIDs, oChangedTuples,
			iWrittenTupleIDs, iWrittenTuples, iWrittenTuplesCount,
			oChangedQueries);
		}
	else if (fSupports2)
		{
		this->pDoIt2(
			iRemovedIDs, iRemovedIDsCount,
			iAddedIDs, iAddedIDsCount,
			iRemovedQueries, iRemovedQueriesCount,
			iAddedQueries, iAddedQueriesCount,
			oChangedTupleIDs, oChangedTuples,
			iWrittenTupleIDs, iWrittenTuples, iWrittenTuplesCount,
			oChangedQueries);
		}
	else
		{
		this->pDoIt1(
			iRemovedIDs, iRemovedIDsCount,
			iAddedIDs, iAddedIDsCount,
			iRemovedQueries, iRemovedQueriesCount,
			iAddedQueries, iAddedQueriesCount,
			oAddedIDs,
			oChangedTupleIDs, oChangedTuples,
			iWrittenTupleIDs, iWrittenTuples, iWrittenTuplesCount,
			oChangedQueries);
		}
	}

void ZTSWatcher_Client::AllocateIDs(size_t iCount, uint64& oBaseID, size_t& oCountIssued)
	{
	ZMutexLocker locker(fMutex);
	ZTuple request;
	request.SetString("What", "AllocateIDs");
	request.SetInt32("Count", iCount);
	request.ToStream(fStreamW);

	ZTuple response(fStreamR);
	oBaseID = response.GetInt64("BaseID");
	oCountIssued = response.GetInt32("Count");
	}


void ZTSWatcher_Client::pDoIt1(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oAddedIDs,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	ZTime beforeLock = ZTime::sSystem();
	ZMutexLocker locker(fMutex);

	ZTuple request;

	if (iRemovedIDsCount)
		request.SetVector_T("removedIDs", iRemovedIDs, iRemovedIDs + iRemovedIDsCount, uint64());

	if (iAddedIDsCount)
		request.SetVector_T("addedIDs", iAddedIDs, iAddedIDs + iAddedIDsCount, uint64());

	if (iRemovedQueriesCount)
		request.SetVector_T("removedQueries", iRemovedQueries, iRemovedQueries + iRemovedQueriesCount, int64());


	if (iAddedQueriesCount)
		{
		vector<ZTupleValue>& addedQueriesV = request.SetMutableVector("addedQueries");
		for (size_t count = iAddedQueriesCount; count; --count)
			{
			const AddedQueryCombo& theAQC = *iAddedQueries++;

			ZTuple temp;

			temp.SetInt64("refcon", theAQC.fRefcon);

			if (theAQC.fPrefetch)
				temp.SetBool("prefetch", true);

			ZTBQuery theTBQuery = theAQC.fTBQuery;

			if (!theTBQuery)
				{
				ZAssert(theAQC.fMemoryBlock);
				theTBQuery = ZTBQuery(ZStreamRPos_MemoryBlock(theAQC.fMemoryBlock));
				}
			temp.SetTuple("query", theTBQuery.AsTuple());

			addedQueriesV.push_back(temp);
			}
		}


	if (iWrittenTuplesCount)
		{
		vector<ZTupleValue>& writtenTuplesV = request.SetMutableVector("writtenTuples");
		for (size_t count = iWrittenTuplesCount; count; --count)
			{
			ZTuple temp;
			temp.SetID("ID", *iWrittenTupleIDs++);
			temp.SetTuple("tuple", *iWrittenTuples++);
			writtenTuplesV.push_back(temp);
			}
		}

	request.SetString("What", "DoIt");
	if (kDebug_ShowCommsTuples)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcher_Client"))
			s << "<< " << request;
		}

	ZTime beforeSend = ZTime::sSystem();

	uint64 bytesWritten;
	if (kDebug_ShowStats)
		request.ToStream(ZStreamW_Count(bytesWritten, fStreamW));
	else
		request.ToStream(fStreamW);

	ZTime afterSend = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowStats
		ZStreamU_Unreader theStreamR(fStreamR);
		theStreamR.ReadInt8();
		theStreamR.Unread();
		ZTime startReceiving = ZTime::sSystem();
		ZTuple response(ZStreamR_Count(bytesRead, theStreamR));
	#else
		ZTime startReceiving = ZTime::sSystem();
		ZTuple response(fStreamR);
	#endif

	ZTime afterReceive = ZTime::sSystem();

	if (kDebug_ShowCommsTuples)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::eInfo, "ZTSWatcher_Client"))
			s << ">> " << response;
		}

	oAddedIDs.clear();
	response.GetVector_T("addedTuples", back_inserter(oAddedIDs), uint64());
	
	oChangedTupleIDs.clear();
	oChangedTuples.clear();
	const vector<ZTupleValue>& changedTuplesV = response.GetVector("changedTuples");
	if (size_t theCount = changedTuplesV.size())
		{
		oChangedTupleIDs.reserve(theCount);
		oChangedTuples.reserve(theCount);
		for (vector<ZTupleValue>::const_iterator i = changedTuplesV.begin(), theEnd = changedTuplesV.end();
			i != theEnd; ++i)
			{
			const ZTuple& entry = (*i).GetTuple();

			uint64 theID;
			if (entry.GetID("ID", theID))
				{
				ZTuple theTuple;
				if (entry.GetTuple("tuple", theTuple))
					{
					oChangedTupleIDs.push_back(theID);
					oChangedTuples.push_back(theTuple);
					}
				}
			}
		}


	oChangedQueries.clear();
	const vector<ZTupleValue>& changedQueriesV = response.GetVector("changedQueries");
	for (vector<ZTupleValue>::const_iterator i = changedQueriesV.begin(), theEnd = changedQueriesV.end();
		i != theEnd; ++i)
		{
		const ZTuple& entry = (*i).GetTuple();

		int64 theRefcon;
		if (entry.GetInt64("refcon", theRefcon))
			{
			pair<map<int64, vector<uint64> >::iterator, bool> pos =
				oChangedQueries.insert(pair<int64, vector<uint64> >(theRefcon, vector<uint64>()));
			vector<uint64>& theIDs = (*pos.first).second;
			theIDs.reserve(entry.GetVector("IDs").size());
			entry.GetVector_T("IDs", back_inserter(theIDs), uint64());
			}
		}

	ZTime allDone = ZTime::sSystem();

	if (kDebug_ShowStats)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSWatcher_Client"))
			{
			bool isEmpty =
				!iRemovedIDsCount
				&& !iAddedIDsCount
				&& !iRemovedQueriesCount
				&& !iWrittenTuplesCount
				&& !iAddedQueriesCount
				&& oAddedIDs.empty()
				&& oChangedTupleIDs.empty()
				&& oChangedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";
			s.Writef("1 %7.3fp %7.3fs %7.3fd %7.3fr %7.3fe - %3dt- %3dt+ %3dq- %3dtw %3dq+ %3dt+ %3d~t %3d~q - %6d> %6d<",
				1000*(beforeSend-beforeLock),
				1000*(afterSend-beforeSend),
				1000*(startReceiving-afterSend),
				1000*(afterReceive-startReceiving),
				1000*(allDone-afterReceive),
				iRemovedIDsCount,
				iAddedIDsCount, 
				iRemovedQueriesCount,
				iWrittenTuplesCount,
				iAddedQueriesCount,
				oAddedIDs.size(),
				oChangedTupleIDs.size(),
				oChangedQueries.size(),
				size_t(bytesWritten),
				size_t(bytesRead)
				);
			}
		}
	}

void ZTSWatcher_Client::pDoIt2(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	ZAssert(fSupports2);
	
	ZMutexLocker locker(fMutex);

	ZTime beforeSend = ZTime::sSystem();

	ZTuple request;
	request.SetString("What", "NewDoIt");
	request.ToStream(fStreamW);

	uint64 bytesWritten;
	#if kDebug_ShowStats
		ZStreamW_Count theStreamW(bytesWritten, fStreamW);
	#else
		const ZStreamW& theStreamW = fStreamW;
	#endif

	sWriteCount(theStreamW, iRemovedIDsCount);
	for (size_t count = iRemovedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iRemovedIDs++);


	sWriteCount(theStreamW, iAddedIDsCount);
	for (size_t count = iAddedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iAddedIDs++);


	sWriteCount(theStreamW, iRemovedQueriesCount);
	for (size_t count = iRemovedQueriesCount; count; --count)
		theStreamW.WriteInt64(*iRemovedQueries++);


	sWriteCount(theStreamW, iAddedQueriesCount);
	for (size_t count = iAddedQueriesCount; count; --count)
		{
		const AddedQueryCombo& theAQC = *iAddedQueries++;
		theStreamW.WriteInt64(theAQC.fRefcon);
		if (theAQC.fMemoryBlock)
			{
			theStreamW.Write(theAQC.fMemoryBlock.GetData(), theAQC.fMemoryBlock.GetSize());
			}
		else
			{
			ZAssert(theAQC.fTBQuery);
			theAQC.fTBQuery.ToStream(theStreamW);
			}
		}


	sWriteCount(theStreamW, iWrittenTuplesCount);
	for (size_t count = iWrittenTuplesCount; count; --count)
		{
		theStreamW.WriteInt64(*iWrittenTupleIDs++);
		(*iWrittenTuples++).ToStream(theStreamW);
		}


	ZTime afterSend = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowStats
		ZStreamU_Unreader theStreamU(fStreamR);
		theStreamU.ReadInt8();
		theStreamU.Unread();
		ZTime startReceiving = ZTime::sSystem();
		ZStreamR_Count theStreamR(bytesRead, theStreamU);
	#else
		ZTime startReceiving = ZTime::sSystem();
		const ZStreamR& theStreamR = fStreamR;
	#endif

	
	oChangedTupleIDs.clear();
	oChangedTuples.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		oChangedTupleIDs.reserve(theCount);
		oChangedTuples.reserve(theCount);
		while (theCount--)
			{
			oChangedTupleIDs.push_back(theStreamR.ReadUInt64());
			oChangedTuples.push_back(ZTuple(theStreamR));
			}
		}


	oChangedQueries.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		while (theCount--)
			{
			int64 theRefcon = theStreamR.ReadInt64();
			pair<map<int64, vector<uint64> >::iterator, bool> pos =
				oChangedQueries.insert(pair<int64, vector<uint64> >(theRefcon, vector<uint64>()));
			
			vector<uint64>& theVector = (*pos.first).second;
			if (uint32 theIDCount = sReadCount(theStreamR))
				{
				theVector.reserve(theIDCount);
				while (theIDCount--)
					theVector.push_back(theStreamR.ReadUInt64());
				}
			}
		}

	ZTime allDone = ZTime::sSystem();

	if (kDebug_ShowStats)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSWatcher_Client"))
			{
			bool isEmpty =
				!iRemovedIDsCount
				&& !iAddedIDsCount
				&& !iRemovedQueriesCount
				&& !iWrittenTuplesCount
				&& !iAddedQueriesCount
				&& oChangedTupleIDs.empty()
				&& oChangedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";
			s.Writef("2 %7.3fs %7.3fd %7.3fr - %3dt- %3dt+ %3dq- %3dtw %3dq+ %3d~t %3d~q - %6d> %6d<",
				1000*(afterSend-beforeSend),
				1000*(startReceiving-afterSend),
				1000*(allDone-startReceiving),
				iRemovedIDsCount,
				iAddedIDsCount, 
				iRemovedQueriesCount,
				iWrittenTuplesCount,
				iAddedQueriesCount,
				oChangedTupleIDs.size(),
				oChangedQueries.size(),
				size_t(bytesWritten),
				size_t(bytesRead)
				);
			}
		}
	}

void ZTSWatcher_Client::pDoIt3(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	ZAssert(fSupports3);
	
	ZMutexLocker locker(fMutex);

	ZTime beforeSend = ZTime::sSystem();

	ZTuple request;
	request.SetString("What", "DoIt3");
	request.ToStream(fStreamW);

	uint64 bytesWritten;
	#if kDebug_ShowStats
		ZStreamW_Count theStreamW(bytesWritten, fStreamW);
	#else
		const ZStreamW& theStreamW = fStreamW;
	#endif


	sWriteCount(theStreamW, iRemovedIDsCount);
	for (size_t count = iRemovedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iRemovedIDs++);


	sWriteCount(theStreamW, iAddedIDsCount);
	for (size_t count = iAddedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iAddedIDs++);


	sWriteCount(theStreamW, iRemovedQueriesCount);
	for (size_t count = iRemovedQueriesCount; count; --count)
		theStreamW.WriteInt64(*iRemovedQueries++);


	sWriteCount(theStreamW, iAddedQueriesCount);
	for (size_t count = iAddedQueriesCount; count; --count)
		{
		const AddedQueryCombo& theAQC = *iAddedQueries++;

		theStreamW.WriteInt64(theAQC.fRefcon);
		ZMemoryBlock theMB = theAQC.fMemoryBlock;

		if (!theMB)
			{
			ZAssert(theAQC.fTBQuery);
			theAQC.fTBQuery.ToStream(ZStreamRWPos_MemoryBlock(theMB, 1024));
			}

		sWriteCount(theStreamW, theMB.GetSize());
		theStreamW.Write(theMB.GetData(), theMB.GetSize());
		}


	sWriteCount(theStreamW, iWrittenTuplesCount);
	for (size_t count = iWrittenTuplesCount; count; --count)
		{
		theStreamW.WriteInt64(*iWrittenTupleIDs++);
		(*iWrittenTuples++).ToStream(theStreamW);
		}

	ZTime afterSend = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowStats
		ZStreamU_Unreader theStreamU(fStreamR);
		theStreamU.ReadInt8();
		theStreamU.Unread();
		ZTime startReceiving = ZTime::sSystem();
		ZStreamR_Count theStreamR(bytesRead, theStreamU);
	#else
		ZTime startReceiving = ZTime::sSystem();
		const ZStreamR& theStreamR = fStreamR;
	#endif

	
	oChangedTupleIDs.clear();
	oChangedTuples.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		oChangedTupleIDs.reserve(theCount);
		oChangedTuples.reserve(theCount);
		while (theCount--)
			{
			oChangedTupleIDs.push_back(theStreamR.ReadUInt64());
			oChangedTuples.push_back(ZTuple(theStreamR));
			}
		}


	oChangedQueries.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		while (theCount--)
			{
			int64 theRefcon = theStreamR.ReadInt64();
			pair<map<int64, vector<uint64> >::iterator, bool> pos =
				oChangedQueries.insert(pair<int64, vector<uint64> >(theRefcon, vector<uint64>()));
			
			vector<uint64>& theVector = (*pos.first).second;
			if (uint32 theIDCount = sReadCount(theStreamR))
				{
				theVector.reserve(theIDCount);
				while (theIDCount--)
					theVector.push_back(theStreamR.ReadUInt64());
				}
			}
		}

	ZTime allDone = ZTime::sSystem();

	if (kDebug_ShowStats)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSWatcher_Client"))
			{
			bool isEmpty =
				!iRemovedIDsCount
				&& !iAddedIDsCount
				&& !iRemovedQueriesCount
				&& !iWrittenTuplesCount
				&& !iAddedQueriesCount
				&& oChangedTupleIDs.empty()
				&& oChangedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";
			s.Writef("3 %7.3fs %7.3fd %7.3fr - %3dt- %3dt+ %3dq- %3dtw %3dq+ %3d~t %3d~q - %6d> %6d<",
				1000*(afterSend-beforeSend),
				1000*(startReceiving-afterSend),
				1000*(allDone-startReceiving),
				iRemovedIDsCount,
				iAddedIDsCount, 
				iRemovedQueriesCount,
				iWrittenTuplesCount,
				iAddedQueriesCount,
				oChangedTupleIDs.size(),
				oChangedQueries.size(),
				size_t(bytesWritten),
				size_t(bytesRead)
				);
			}
		}
	}

void ZTSWatcher_Client::pDoIt4(
	const uint64* iRemovedIDs, size_t iRemovedIDsCount,
	const uint64* iAddedIDs, size_t iAddedIDsCount,
	const int64* iRemovedQueries, size_t iRemovedQueriesCount,
	const AddedQueryCombo* iAddedQueries, size_t iAddedQueriesCount,
	vector<uint64>& oAddedIDs,
	vector<uint64>& oChangedTupleIDs, vector<ZTuple>& oChangedTuples,
	const uint64* iWrittenTupleIDs, const ZTuple* iWrittenTuples, size_t iWrittenTuplesCount,
	map<int64, vector<uint64> >& oChangedQueries)
	{
	ZAssert(fSupports4);
	
	ZMutexLocker locker(fMutex);

	ZTime beforeSend = ZTime::sSystem();

	ZTuple request;
	request.SetString("What", "DoIt4");
	request.ToStream(fStreamW);

	uint64 bytesWritten;
	#if kDebug_ShowStats
		ZStreamW_Count theStreamW(bytesWritten, fStreamW);
	#else
		const ZStreamW& theStreamW = fStreamW;
	#endif


	sWriteCount(theStreamW, iRemovedIDsCount);
	for (size_t count = iRemovedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iRemovedIDs++);


	sWriteCount(theStreamW, iAddedIDsCount);
	for (size_t count = iAddedIDsCount; count; --count)
		theStreamW.WriteUInt64(*iAddedIDs++);


	sWriteCount(theStreamW, iRemovedQueriesCount);
	for (size_t count = iRemovedQueriesCount; count; --count)
		theStreamW.WriteInt64(*iRemovedQueries++);


	sWriteCount(theStreamW, iAddedQueriesCount);
	for (size_t count = iAddedQueriesCount; count; --count)
		{
		const AddedQueryCombo& theAQC = *iAddedQueries++;

		theStreamW.WriteInt64(theAQC.fRefcon);
		theStreamW.WriteBool(theAQC.fPrefetch);

		ZMemoryBlock theMB = theAQC.fMemoryBlock;

		if (!theMB)
			{
			ZAssert(theAQC.fTBQuery);
			theAQC.fTBQuery.ToStream(ZStreamRWPos_MemoryBlock(theMB, 1024));
			}

		sWriteCount(theStreamW, theMB.GetSize());
		theStreamW.Write(theMB.GetData(), theMB.GetSize());
		}


	sWriteCount(theStreamW, iWrittenTuplesCount);
	for (size_t count = iWrittenTuplesCount; count; --count)
		{
		theStreamW.WriteInt64(*iWrittenTupleIDs++);
		(*iWrittenTuples++).ToStream(theStreamW);
		}

	ZTime afterSend = ZTime::sSystem();

	uint64 bytesRead;
	#if kDebug_ShowStats
		ZStreamU_Unreader theStreamU(fStreamR);
		theStreamU.ReadInt8();
		theStreamU.Unread();
		ZTime startReceiving = ZTime::sSystem();
		ZStreamR_Count theStreamR(bytesRead, theStreamU);
	#else
		ZTime startReceiving = ZTime::sSystem();
		const ZStreamR& theStreamR = fStreamR;
	#endif

	
	oAddedIDs.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		oAddedIDs.reserve(theCount);
		while (theCount--)
			oAddedIDs.push_back(theStreamR.ReadUInt64());
		}

	oChangedTupleIDs.clear();
	oChangedTuples.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		oChangedTupleIDs.reserve(theCount);
		oChangedTuples.reserve(theCount);
		while (theCount--)
			{
			oChangedTupleIDs.push_back(theStreamR.ReadUInt64());
			oChangedTuples.push_back(ZTuple(theStreamR));
			}
		}


	oChangedQueries.clear();
	if (uint32 theCount = sReadCount(theStreamR))
		{
		while (theCount--)
			{
			int64 theRefcon = theStreamR.ReadInt64();
			pair<map<int64, vector<uint64> >::iterator, bool> pos =
				oChangedQueries.insert(pair<int64, vector<uint64> >(theRefcon, vector<uint64>()));
			
			vector<uint64>& theVector = (*pos.first).second;
			if (uint32 theIDCount = sReadCount(theStreamR))
				{
				theVector.reserve(theIDCount);
				while (theIDCount--)
					theVector.push_back(theStreamR.ReadUInt64());
				}
			}
		}

	ZTime allDone = ZTime::sSystem();

	if (kDebug_ShowStats)
		{
		if (const ZLog::S& s = ZLog::S(ZLog::ePriority_Notice, "ZTSWatcher_Client"))
			{
			bool isEmpty =
				!iRemovedIDsCount
				&& !iAddedIDsCount
				&& !iRemovedQueriesCount
				&& !iWrittenTuplesCount
				&& !iAddedQueriesCount
				&& oAddedIDs.empty()
				&& oChangedTupleIDs.empty()
				&& oChangedQueries.empty();

			if (isEmpty)
				s << "!";
			else
				s << " ";
			s.Writef("4 %7.3fs %7.3fd %7.3fr - %3dt- %3dt+ %3dq- %3dtw %3dq+ %3dt+ %3d~t %3d~q - %6d> %6d<",
				1000*(afterSend-beforeSend),
				1000*(startReceiving-afterSend),
				1000*(allDone-startReceiving),
				iRemovedIDsCount,
				iAddedIDsCount, 
				iRemovedQueriesCount,
				iWrittenTuplesCount,
				iAddedQueriesCount,
				oAddedIDs.size(),
				oChangedTupleIDs.size(),
				oChangedQueries.size(),
				size_t(bytesWritten),
				size_t(bytesRead)
				);
			}
		}
	}

void ZTSWatcher_Client::SetCallback(Callback_t iCallback, void* iRefcon)
	{
	}
