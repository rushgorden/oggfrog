static const char rcsid[]
= "@(#) $Id: ZTupleIndex_String.cpp,v 1.9 2006/11/02 18:08:34 agreen Exp $";

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
#include "ZTupleIndex_String.h"

#include "ZStrim.h"

using std::pair;
using std::set;
using std::string;
using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_String::Compare

ZTupleIndex_String::Compare::Compare()
:	fTextCollator(1)
	{}

bool ZTupleIndex_String::Compare::operator()
	(const value_type& iLeft, const value_type& iRight) const
	{
	if (int compare = fTextCollator.Compare(iLeft.fValue, iRight.fValue))
		return compare < 0;
	return iLeft.fID < iRight.fID;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_String

ZTupleIndex_String::ZTupleIndex_String(const ZTuplePropName& iPropName)
:	fPropName(iPropName)
	{}

void ZTupleIndex_String::Add(uint64 iID, const ZTuple* iTuple)
	{
	Key theKey;
	if (this->pKeyFromTuple(iID, iTuple, theKey))
		fSet.insert(theKey);
	}

void ZTupleIndex_String::Remove(uint64 iID, const ZTuple* iTuple)
	{
	Key theKey;
	if (this->pKeyFromTuple(iID, iTuple, theKey))
		fSet.erase(theKey);
	}

static void sExtractConstraint(const ZTuplePropName& iPropName,
	vector<const ZTBSpec::Criterion*>& ioCriteria, string16& oValue)
	{
	for (vector<const ZTBSpec::Criterion*>::iterator critIter = ioCriteria.begin();
		critIter != ioCriteria.end();
		/*no increment*/)
		{
		// It's a criteria on the current property.
		if (1 == (*critIter)->GetComparator().fStrength
			&& ZTBSpec::eRel_Equal == (*critIter)->GetComparator().fRel
			&& eZType_String == (*critIter)->GetTupleValue().TypeOf()
			&& (*critIter)->GetPropName().Equals(iPropName))
			{
			// It's a 1-strength equality search on our property name,
			// so we can handle it. Remove the criteria from ioCriteria
			// and return success.
			oValue = ZUnicode::sAsUTF16((*critIter)->GetTupleValue().GetString());
			ioCriteria.erase(critIter);
			return;
			}
		++critIter;
		}
	// Can't get here.
	ZUnimplemented();
	}

static bool sCheckForConstraint(
	const ZTuplePropName& iPropName, const ZTBSpec::CriterionSect& iCriterionSect)
	{
	for (ZTBSpec::CriterionSect::const_iterator critIter = iCriterionSect.begin(),
		theEnd = iCriterionSect.end();
		critIter != theEnd; ++critIter)
		{
		// It's a criteria on the current property.
		if (1 == (*critIter).GetComparator().fStrength
			&& ZTBSpec::eRel_Equal == (*critIter).GetComparator().fRel
			&& eZType_String == (*critIter).GetTupleValue().TypeOf()
			&& (*critIter).GetPropName().Equals(iPropName))
			{
			// It's a 1-strength equality search on our property name,
			// so we can handle it. Remove the criteria from ioCriteria
			// and return success.
			return true;
			}
		}
	// We didn't find a 1-strength equality search on our property name.
	return false;
	}

void ZTupleIndex_String::Find(const set<uint64>& iSkipIDs,
		vector<const ZTBSpec::Criterion*>& ioCriteria, vector<uint64>& oIDs)
	{
	Key theKey;
	sExtractConstraint(fPropName, ioCriteria, theKey.fValue);

	theKey.fID = 0;
	set<Key, Compare>::iterator lowerBound = fSet.lower_bound(theKey);

	theKey.fID = kMaxID;
	set<Key, Compare>::iterator upperBound = fSet.upper_bound(theKey);
		
	if (iSkipIDs.empty())
		{
		for (/*no init*/; lowerBound != upperBound; ++lowerBound)
			oIDs.push_back((*lowerBound).fID);
		}
	else
		{
		for (/*no init*/; lowerBound != upperBound; ++lowerBound)
			{
			if (iSkipIDs.end() == iSkipIDs.find((*lowerBound).fID))
				oIDs.push_back((*lowerBound).fID);
			}
		}
	}

size_t ZTupleIndex_String::CanHandle(const ZTBSpec::CriterionSect& iCriterionSect)
	{
	if (!sCheckForConstraint(fPropName, iCriterionSect))
		return 0;

	// Our result is the number of tuples we'll likely have to visit
	// to provide all tuples matching iCriterionSect.
	size_t theSize = fSet.size();

	// Divide it by 2, to match up with what a general index would claim.
	theSize = theSize / 2;

	// We add one to ensure we don't return zero.
	return theSize + 1;
	}

void ZTupleIndex_String::WriteDescription(const ZStrimW& s)
	{
	s << "ZTupleIndex_String. ";
	s.Writef("%d entries, indexing on ");
	s << fPropName.AsString();
	}

bool ZTupleIndex_String::pKeyFromTuple(uint64 iID, const ZTuple* iTuple, Key& oKey)
	{
	ZTuple::const_iterator propIter = iTuple->IteratorOf(fPropName);
	if (propIter == iTuple->end())
		return false;
	if (iTuple->TypeOf(propIter) != eZType_String)
		return false;
	oKey.fValue = ZUnicode::sAsUTF16(iTuple->GetString(propIter));
	oKey.fID = iID;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory_String

ZTupleIndexFactory_String::ZTupleIndexFactory_String(const std::string& iPropName)
:	fPropName(iPropName)
	{}

ZTupleIndex* ZTupleIndexFactory_String::Make()
	{ return new ZTupleIndex_String(fPropName); }
