/*  @(#) $Id: ZTBServer.h,v 1.7 2006/07/23 21:50:42 agreen Exp $ */

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

#ifndef __ZTBServer__
#define __ZTBServer__ 1
#include "zconfig.h"

#include "ZRefCount.h"
#include "ZThread.h"
#include "ZTuple.h"

#include <set>

class ZStreamR;
class ZStreamW;
class ZTB;
class ZTBRep;

// =================================================================================================
#pragma mark -
#pragma mark * ZTBServer

class ZTBServer
	{
private:
	class Transaction;
	class Search_t;
	class Count_t;

public:
	ZTBServer(ZTB iTB);
	ZTBServer(ZTB iTB, const std::string& iLogFacility);

	~ZTBServer();

	void RunReader(const ZStreamR& iStream);
	void RunWriter(const ZStreamW& iStream);

private:
	static void sCallback_AllocateIDs(void* iRefcon, uint64 iBaseID, size_t iCount);
	void Handle_AllocateIDs(const ZTuple& iReq);

	void Handle_Create(const ZTuple& iReq);

	static void sCallback_GetTupleForSearch(void* iRefcon, size_t iCount, const uint64* iIDs, const ZTuple* iTuples);

	static void sCallback_Search(void* iRefcon, std::vector<uint64>& ioResults);
	void Handle_Search(const ZTuple& iReq);

	static void sCallback_Count(void* iRefcon, size_t iResult);
	void Handle_Count(const ZTuple& iReq);

	void Handle_Abort(const ZTuple& iReq);

	static void sCallback_Validate(bool iSucceeded, void* iRefcon);
	void Handle_Validate(const ZTuple& iReq);

	static void sCallback_Commit(void* iRefcon);
	void Handle_Commit(const ZTuple& iReq);

	static void sCallback_GetTuple(void* iRefcon, size_t iCount, const uint64* iIDs, const ZTuple* iTuples);
	void Handle_Actions(const ZTuple& iReq);

	void Reader(const ZStreamR& iStream);
	void Writer(const ZStreamW& iStream);

	void TearDown();

	ZRef<ZTBRep> fTBRep;

	ZMutex fMutex_Structure;
	ZCondition fCondition_Sender;
		bool fReaderExited;
		bool fWriterExited;

		ZTime fTime_LastRead;
		bool fPingRequested;
		bool fPingSent;

		std::vector<Transaction*> fTransactions_Create_Unsent;
		std::vector<Transaction*> fTransactions_Created;

		std::vector<Transaction*> fTransactions_Validate_Waiting;
		std::vector<Transaction*> fTransactions_Validate_Succeeded;
		std::vector<Transaction*> fTransactions_Validate_Failed;
		std::vector<Transaction*> fTransactions_Validated;

		std::vector<Transaction*> fTransactions_Commit_Waiting;
		std::vector<Transaction*> fTransactions_Commit_Acked;

		std::set<Transaction*> fTransactions_HaveTuplesToSend;

		std::vector<Search_t*> fSearches_Waiting;
		std::vector<Search_t*> fSearches_Unsent;

		std::vector<Count_t*> fCounts_Waiting;
		std::vector<Count_t*> fCounts_Unsent;

		std::vector<std::pair<uint64, size_t> > fIDs;

	std::string fLogFacility;
	};

#endif // __ZTBServer__
