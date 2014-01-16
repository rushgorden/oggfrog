static const char rcsid[] = "@(#) $Id: ZTupleIndex.cpp,v 1.22 2006/05/12 15:48:51 agreen Exp $";

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
#include "ZTupleIndex.h"

using std::set;
using std::vector;

#include "ZStrim.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex

ZTupleIndex::~ZTupleIndex()
	{}

void ZTupleIndex::Find(const ZTBSpec::CriterionSect& iCriterionSect, const set<uint64>& iSkipIDs,
	vector<const ZTBSpec::Criterion*>& oUncheckedCriteria, vector<uint64>& oIDs)
	{
	oUncheckedCriteria.reserve(iCriterionSect.size());
	for (ZTBSpec::CriterionSect::const_iterator i = iCriterionSect.begin(),
		theEnd = iCriterionSect.end();
		i != theEnd; ++i)
		{
		oUncheckedCriteria.push_back(&(*i));
		}

	this->Find(iSkipIDs, oUncheckedCriteria, oIDs);
	}

bool ZTupleIndex::sMatchIndices(const ZTBSpec::CriterionUnion& iCriterionUnion,
	const vector<ZTupleIndex*>& iIndices, vector<ZTupleIndex*>& oIndices)
	{
	for (ZTBSpec::CriterionUnion::const_iterator critIter = iCriterionUnion.begin();
		critIter != iCriterionUnion.end(); ++critIter)
		{
		size_t bestWeight = 0;
		ZTupleIndex* bestIndex = nil;
		for (vector<ZTupleIndex*>::const_iterator indIter = iIndices.begin();
			indIter != iIndices.end(); ++indIter)
			{
			if (size_t weight = (*indIter)->CanHandle(*critIter))
				{
				if (bestIndex == nil || bestWeight > weight)
					{
					bestWeight = weight;
					bestIndex = *indIter;
					}
				}
			}
		if (!bestWeight)
			return false;
		oIndices.push_back(bestIndex);
		}
	return true;
	}

void ZTupleIndex::WriteDescription(const ZStrimW& s)
	{
	s << "An index which doesn't understand WriteDescription";
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory

ZTupleIndexFactory::~ZTupleIndexFactory()
	{}
