/*  @(#) $Id: ZTupleIndex.h,v 1.20 2006/11/02 18:08:34 agreen Exp $ */

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

#ifndef __ZTupleIndex__
#define __ZTupleIndex__
#include "zconfig.h"

#include "ZTBSpec.h"

#include <deque>
#include <list>
#include <set>
#include <vector>

#ifndef ZCONFIG_TupleIndex_Debug
#	define ZCONFIG_TupleIndex_Debug 3
#endif

class ZStrimW;

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex

class ZTupleIndex
	{
public:
	virtual ~ZTupleIndex();

	virtual void Add(uint64 iID, const ZTuple* iTuple) = 0;

	virtual void Remove(uint64 iID, const ZTuple* iTuple) = 0;

	virtual void Find(const std::set<uint64>& iSkipIDs,
		std::vector<const ZTBSpec::Criterion*>& ioCriteria, std::vector<uint64>& oIDs) = 0;

	virtual size_t CanHandle(const ZTBSpec::CriterionSect& iCriterionSect) = 0;

	void Find(const ZTBSpec::CriterionSect& iCriterionSect, const std::set<uint64>& iSkipIDs,
		std::vector<const ZTBSpec::Criterion*>& oUncheckedCriteria, std::vector<uint64>& oIDs);

	static bool sMatchIndices(const ZTBSpec::CriterionUnion& iCriterionUnion,
		const std::vector<ZTupleIndex*>& iIndices, std::vector<ZTupleIndex*>& oIndices);

	virtual void WriteDescription(const ZStrimW& s);

	static const uint64 kMaxID = 0xFFFFFFFFFFFFFFFFULL;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory

class ZTupleIndexFactory : public ZRefCounted
	{
public:
	virtual ~ZTupleIndexFactory();
	virtual ZTupleIndex* Make() = 0;
	};

#endif // __ZTupleIndex__
