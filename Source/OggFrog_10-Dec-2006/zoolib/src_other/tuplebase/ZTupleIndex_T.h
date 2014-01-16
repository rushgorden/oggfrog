/*  @(#) $Id: ZTupleIndex_T.h,v 1.15 2006/07/23 21:50:42 agreen Exp $ */

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

#ifndef __ZTupleIndex_T__
#define __ZTupleIndex_T__
#include "zconfig.h"

#include "ZCompare.h"
#include "ZTupleIndex.h"
#include "ZUtil_STL.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZooLib::TypeOf_T

namespace ZooLib {
template <class T> struct TypeOf_T;

template <> struct TypeOf_T<ZType> { static const ZType sType = eZType_Type; };
template <> struct TypeOf_T<uint64> { static const ZType sType = eZType_ID; };
template <> struct TypeOf_T<int8> { static const ZType sType = eZType_Int8; };
template <> struct TypeOf_T<int16> { static const ZType sType = eZType_Int16; };
template <> struct TypeOf_T<int32> { static const ZType sType = eZType_Int32; };
template <> struct TypeOf_T<int64> { static const ZType sType = eZType_Int64; };
template <> struct TypeOf_T<bool> { static const ZType sType = eZType_Bool; };
template <> struct TypeOf_T<float> { static const ZType sType = eZType_Float; };
template <> struct TypeOf_T<double> { static const ZType sType = eZType_Double; };
template <> struct TypeOf_T<ZTime> { static const ZType sType = eZType_Time; };
template <> struct TypeOf_T<void*> { static const ZType sType = eZType_Pointer; };
template <> struct TypeOf_T<ZRectPOD> { static const ZType sType = eZType_Rect; };
template <> struct TypeOf_T<ZPointPOD> { static const ZType sType = eZType_Point; };
template <> struct TypeOf_T<std::string> { static const ZType sType = eZType_String; };
template <> struct TypeOf_T<ZTuple> { static const ZType sType = eZType_Type; };

template <> struct TypeOf_T<std::vector<ZTupleValue> >
	{ static const ZType sType = eZType_Vector; };
} // namespace ZooLib

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_T

class ZTupleIndex_T
	{
public:
	template <class P>
	bool sCheckCriteria_Equal_T(const std::string& iPropName,
		const ZTBSpec::CriterionSect& iCriterionSect);

	template <class P>
	static void sGetCriterion_Equal_T(const std::string& iPropName,
		std::vector<const ZTBSpec::Criterion*>& ioCriteria, P& oKeyValue);
	};

template <class P>
bool ZTupleIndex_T::sCheckCriteria_Equal_T(const std::string& iPropName,
	const ZTBSpec::CriterionSect& iCriterionSect)
	{
	for (ZTBSpec::CriterionSect::const_iterator
		critIter = iCriterionSect.begin(), theEnd = iCriterionSect.end();
		critIter != theEnd; ++critIter)
		{
		if ((*critIter).GetComparator().fRel == ZTBSpec::eRel_Equal
			&& (*critIter).GetComparator().fStrength == 0
			&& (*critIter).GetTupleValue().TypeOf() == ZooLib::TypeOf_T<P>::sType
			&& (*critIter).GetPropName() == iPropName)
			{
			return true;
			}
		}
	return false;
	}

template <class P>
void ZTupleIndex_T::sGetCriterion_Equal_T(const std::string& iPropName,
	std::vector<const ZTBSpec::Criterion*>& ioCriteria, P& oKeyValue)
	{
	for (std::vector<const ZTBSpec::Criterion*>::iterator critIter = ioCriteria.begin();
		critIter != ioCriteria.end(); ++critIter)
		{
		if (iPropName == (*critIter)->GetPropName())
			{
			if ((*critIter)->GetComparator().fRel == ZTBSpec::eRel_Equal
				&& (*critIter)->GetComparator().fStrength == 0
				&& (*critIter)->GetTupleValue().TypeOf() == ZooLib::TypeOf_T<P>::sType
				&& (*critIter)->GetPropName() == iPropName)
				{
				(*critIter)->GetTupleValue().Get_T(oKeyValue);
				ioCriteria.erase(critIter);
				return;
				}
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_T1

template <class P0>
class ZTupleIndex_T1 : public ZTupleIndex, ZTupleIndex_T
	{
public:
	class Key
		{
	public:
		Key() {}

		Key(const P0& iP0)
		:	fP0(iP0), fID(0)
			{}

		Key(const P0& iP0, uint64 iID)
		:	fP0(iP0), fID(iID)
			{}

		bool operator<(const Key& iOther) const
			{
			if (int compare = ZooLib::sCompare_T(fP0, iOther.fP0))
				return compare < 0;

			return fID < iOther.fID;
			}

		bool operator==(const Key& iOther) const
			{ return fID == iOther.fID && fP0 == iOther.fP0; }

		P0 fP0;
		uint64 fID;
		};

	ZTupleIndex_T1(const std::string& iPropName0)
	:	fPropName0(iPropName0),
		fDummy0()
		{}

	// From ZTupleIndex
	virtual void Add(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sInsertMustNotContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Remove(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sEraseMustContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Find(const set<uint64>& iSkipIDs,
		std::vector<const ZTBSpec::Criterion*>& ioCriteria, std::vector<uint64>& oIDs)
		{
		Key theKey;
		sGetCriterion_Equal_T(fPropName0, ioCriteria, theKey.fP0);
		
		theKey.fID = 0;
		typename std::set<Key>::const_iterator lower = fSet.lower_bound(theKey);

		theKey.fID = kMaxID;
		typename std::set<Key>::const_iterator upper = fSet.upper_bound(theKey);
		
		if (iSkipIDs.empty())
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				oIDs.push_back((*i).fID);
			}
		else
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				{
				if (iSkipIDs.end() == iSkipIDs.find((*i).fID))
					oIDs.push_back((*i).fID);
				}
			}
		}

	virtual size_t CanHandle(const ZTBSpec::CriterionSect& iCriterionSect)
		{
		if (!sCheckCriteria_Equal_T<P0>(fPropName0, iCriterionSect))
			return 0;

		return fSet.size() / 8 + 1;
		}

private:
	// Our protocol
	bool pKeyFromTuple(uint64 iID, const ZTuple& iTuple, Key& oKey)
		{
		if (!iTuple.Get_T(fPropName0, oKey.fP0))
			return false;
		oKey.fID = iID;
		return true;
		}

	std::set<Key> fSet;
	std::string fPropName0;
	const P0 fDummy0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory_T1

template <class P0>
class ZTupleIndexFactory_T1 : public ZTupleIndexFactory
	{
public:
	ZTupleIndexFactory_T1(const std::string& iPropName0)
	:	fPropName0(iPropName0)
		{}

	// From ZTupleIndexFactory
	virtual ZTupleIndex* Make()
		{ return new ZTupleIndex_T1<P0>(fPropName0); }

private:
	std::string fPropName0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_T2

template <class P0, class P1>
class ZTupleIndex_T2 : public ZTupleIndex, ZTupleIndex_T
	{
public:
	class Key
		{
	public:
		Key() {}

		Key(const P0& iP0, const P1& iP1)
		:	fP0(iP0), fP1(iP1), fID(0)
			{}

		Key(const P0& iP0, const P1& iP1, uint64 iID)
		:	fP0(iP0), fP1(iP1), fID(iID)
			{}

		bool operator<(const Key& iOther) const
			{
			if (int compare = ZooLib::sCompare_T(fP0, iOther.fP0))
				return compare < 0;

			if (int compare = ZooLib::sCompare_T(fP1, iOther.fP1))
				return compare < 0;

			return fID < iOther.fID;
			}

		bool operator==(const Key& iOther) const
			{ return fID == iOther.fID && fP0 == iOther.fP0 && fP1 == iOther.fP1; }

		P0 fP0;
		P1 fP1;
		uint64 fID;
		};

	ZTupleIndex_T2(const std::string& iPropName0, const std::string& iPropName1)
	:	fPropName0(iPropName0), fPropName1(iPropName1),
		fDummy0(), fDummy1()
		{}

	// From ZTupleIndex
	virtual void Add(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sInsertMustNotContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Remove(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sEraseMustContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Find(const set<uint64>& iSkipIDs,
		std::vector<const ZTBSpec::Criterion*>& ioCriteria, std::vector<uint64>& oIDs)
		{
		Key theKey;

		sGetCriterion_Equal_T(fPropName0, ioCriteria, theKey.fP0);

		sGetCriterion_Equal_T(fPropName1, ioCriteria, theKey.fP1);
		
		theKey.fID = 0;
		typename std::set<Key>::const_iterator lower = fSet.lower_bound(theKey);

		theKey.fID = kMaxID;
		typename std::set<Key>::const_iterator upper = fSet.upper_bound(theKey);
		
		if (iSkipIDs.empty())
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				oIDs.push_back((*i).fID);
			}
		else
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				{
				if (iSkipIDs.end() == iSkipIDs.find((*i).fID))
					oIDs.push_back((*i).fID);
				}
			}
		}

	virtual size_t CanHandle(const ZTBSpec::CriterionSect& iCriterionSect)
		{
		if (!sCheckCriteria_Equal_T<P0>(fPropName0, iCriterionSect))
			return 0;

		if (!sCheckCriteria_Equal_T<P1>(fPropName1, iCriterionSect))
			return 0;

		return fSet.size() / 16 + 1;
		}

private:
	// Our protocol
	bool pKeyFromTuple(uint64 iID, const ZTuple& iTuple, Key& oKey)
		{
		if (!iTuple.Get_T(fPropName0, oKey.fP0))
			return false;
		if (!iTuple.Get_T(fPropName1, oKey.fP1))
			return false;
		oKey.fID = iID;
		return true;
		}

	std::set<Key> fSet;
	std::string fPropName0;
	std::string fPropName1;
	const P0 fDummy0;
	const P1 fDummy1;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory_T2

template <class P0, class P1>
class ZTupleIndexFactory_T2 : public ZTupleIndexFactory
	{
public:
	ZTupleIndexFactory_T2(const std::string& iPropName0, const std::string& iPropName1)
	:	fPropName0(iPropName0), fPropName1(iPropName1)
		{}

	// From ZTupleIndexFactory
	virtual ZTupleIndex* Make()
		{ return new ZTupleIndex_T2<P0, P1>(fPropName0, fPropName1); }

private:
	std::string fPropName0;
	std::string fPropName1;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndex_T3

template <class P0, class P1, class P2>
class ZTupleIndex_T3 : public ZTupleIndex, ZTupleIndex_T
	{
public:
	class Key
		{
	public:
		Key() {}

		Key(const P0& iP0, const P1& iP1, const P2& iP2)
		:	fP0(iP0), fP1(iP1), fP2(iP2), fID(0)
			{}
		Key(const P0& iP0, const P1& iP1, const P2& iP2, uint64 iID)
		:	fP0(iP0), fP1(iP1), fP2(iP2), fID(iID)
			{}

		bool operator<(const Key& iOther) const
			{
			if (int compare = ZooLib::sCompare_T(fP0, iOther.fP0))
				return compare < 0;

			if (int compare = ZooLib::sCompare_T(fP1, iOther.fP1))
				return compare < 0;

			if (int compare = ZooLib::sCompare_T(fP2, iOther.fP2))
				return compare < 0;

			return fID < iOther.fID;
			}

		bool operator==(const Key& iOther) const
			{
			return fID == iOther.fID
				&& fP0 == iOther.fP0
				&& fP1 == iOther.fP1
				&& fP2 == iOther.fP2;
			}

		P0 fP0;
		P1 fP1;
		P2 fP2;
		uint64 fID;
		};

	ZTupleIndex_T3(const std::string& iPropName0,
		const std::string& iPropName1, const std::string& iPropName2)
	:	fPropName0(iPropName0), fPropName1(iPropName1), fPropName2(iPropName2),
		fDummy0(), fDummy1(), fDummy2()
		{}

	// From ZTupleIndex
	virtual void Add(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sInsertMustNotContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Remove(uint64 iID, const ZTuple& iTuple)
		{
		Key theKey;
		if (this->pKeyFromTuple(iID, iTuple, theKey))
			ZUtil_STL::sEraseMustContain(ZCONFIG_TupleIndex_Debug, fSet, theKey);
		}

	virtual void Find(const set<uint64>& iSkipIDs,
		std::vector<const ZTBSpec::Criterion*>& ioCriteria, std::vector<uint64>& oIDs)
		{
		Key theKey;

		sGetCriterion_Equal_T(fPropName0, ioCriteria, theKey.fP0);

		sGetCriterion_Equal_T(fPropName1, ioCriteria, theKey.fP1);
		
		sGetCriterion_Equal_T(fPropName2, ioCriteria, theKey.fP2);
		
		theKey.fID = 0;
		typename std::set<Key>::const_iterator lower = fSet.lower_bound(theKey);

		theKey.fID = kMaxID;
		typename std::set<Key>::const_iterator upper = fSet.upper_bound(theKey);
		
		if (iSkipIDs.empty())
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				oIDs.push_back((*i).fID);
			}
		else
			{
			for (typename std::set<Key>::const_iterator i = lower; i != upper; ++i)
				{
				if (iSkipIDs.end() == iSkipIDs.find((*i).fID))
					oIDs.push_back((*i).fID);
				}
			}
		}

	virtual size_t CanHandle(const ZTBSpec::CriterionSect& iCriterionSect)
		{
		if (!sCheckCriteria_Equal_T<P0>(fPropName0, iCriterionSect))
			return 0;

		if (!sCheckCriteria_Equal_T<P1>(fPropName1, iCriterionSect))
			return 0;

		if (!sCheckCriteria_Equal_T<P2>(fPropName2, iCriterionSect))
			return 0;

		return fSet.size() / 32 + 1;
		}

private:
	// Our protocol
	bool pKeyFromTuple(uint64 iID, const ZTuple& iTuple, Key& oKey)
		{
		if (!iTuple.Get_T(fPropName0, oKey.fP0))
			return false;
		if (!iTuple.Get_T(fPropName1, oKey.fP1))
			return false;
		if (!iTuple.Get_T(fPropName2, oKey.fP2))
			return false;
		oKey.fID = iID;
		return true;
		}

	std::set<Key> fSet;
	std::string fPropName0;
	std::string fPropName1;
	std::string fPropName2;
	const P0 fDummy0;
	const P1 fDummy1;
	const P2 fDummy2;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZTupleIndexFactory_T3

template <class P0, class P1, class P2>
class ZTupleIndexFactory_T3 : public ZTupleIndexFactory
	{
public:
	ZTupleIndexFactory_T3(const std::string& iPropName0,
		const std::string& iPropName1, const std::string& iPropName2)
	:	fPropName0(iPropName0), fPropName1(iPropName1), fPropName2(iPropName2)
		{}

	// From ZTupleIndexFactory
	virtual ZTupleIndex* Make()
		{ return new ZTupleIndex_T3<P0, P1, P2>(fPropName0, fPropName1, fPropName2); }

private:
	std::string fPropName0;
	std::string fPropName1;
	std::string fPropName2;
	};

#endif // __ZTupleIndex_T__
