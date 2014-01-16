/*  @(#) $Id: ZDList.h,v 1.6 2006/10/29 03:49:26 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZDList__
#define __ZDList__
#include "zconfig.h"

#include "ZCompat_operator_bool.h"
#include "ZDebug.h"

namespace ZooLib {

// In these templates, P is Pointer and L is Link.

// =================================================================================================
#pragma mark -
#pragma mark * DListHead

template <typename P, int kDebug = 1>
struct DListHead
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES_T(DListHead, operator_bool_generator_type, operator_bool_type);

	DListHead() : fHeadP(nil), fSize(0) {}

	operator operator_bool_type() const { return operator_bool_generator_type::translate(fHeadP); }

	size_t Size() { return fSize; }

	void Insert(P* iP)
		{
		ZAssertStop(kDebug, !iP->fPrev);
		ZAssertStop(kDebug, !iP->fNext);
		if (!fHeadP)
			{
			ZAssertStop(kDebug, fSize == 0);
			fSize = 1;
			
			fHeadP = iP;
			iP->fPrev = iP;
			iP->fNext = iP;
			}
		else
			{
			ZAssertStop(kDebug, fSize != 0);
			++fSize;
			iP->fNext = fHeadP;
			iP->fPrev = fHeadP->fPrev;
			fHeadP->fPrev->fNext = iP;
			fHeadP->fPrev = iP;
			}
		}

	void InsertIfNotContains(P* iP)
		{
		if (!iP->fNext)
			this->Insert(iP);
		}

	void Remove(P* iP)
		{
		ZAssertStop(kDebug, fHeadP);
		ZAssertStop(kDebug, iP->fPrev);
		ZAssertStop(kDebug, iP->fNext);
		if (iP->fPrev == iP)
			{
			ZAssertStop(kDebug, fSize == 1);
			fSize = 0;
			// We have a list with a single element
			ZAssertStop(kDebug, fHeadP == iP);
			fHeadP = nil;
			}
		else
			{
			ZAssertStop(kDebug, fSize > 1);
			--fSize;
			if (fHeadP == iP)
				fHeadP = iP->fNext;
			iP->fNext->fPrev = iP->fPrev;
			iP->fPrev->fNext = iP->fNext;
			}
		iP->fNext = nil;
		iP->fPrev = nil;
		}

	void RemoveIfContains(P* iP)
		{
		if (iP->fNext)
			this->Remove(iP);
		}

	bool Contains(P* iP) const
		{
		return iP->fNext;
		}

	bool Empty() const
		{
		ZAssertStop(kDebug, (!fSize && !fHeadP) || (fSize && fHeadP));
		return !fHeadP;
		}

	P* fHeadP;
	size_t fSize;
	};

// =================================================================================================
#pragma mark -
#pragma mark * DListLink

template <typename P, typename L, int kDebug = 1>
class DListLink
	{
public:
//	DListLink(const DListLink&); // not implemented/implementable
//	DListLink& operator=(const DListLink&); // not implemented/implementable
public:
	DListLink() : fPrev(nil), fNext(nil) {}
	~DListLink() { ZAssertStop(kDebug, !fPrev && !fNext); }
	void Clear() { fPrev = nil; fNext = nil; }

	L* fPrev;
	L* fNext;
	};

// =================================================================================================
#pragma mark -
#pragma mark * DListIterator

template <typename P, typename L, int kDebug = 1>
class DListIterator
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES_T(DListIterator, operator_bool_generator_type, operator_bool_type);
public:
	DListIterator(const ZooLib::DListHead<L, kDebug>& iDListHead)
	:	fDListHead(iDListHead),
		fCurrent(iDListHead.fHeadP)
		{}

	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(fCurrent); }

	P* Current() const
		{ return static_cast<P*>(fCurrent); }

	void Advance()
		{
		ZAssertStop(kDebug, fCurrent);
		ZAssertStop(kDebug, fCurrent->fNext);
		fCurrent = fCurrent->fNext;
		if (fCurrent == fDListHead.fHeadP)
			fCurrent = nil;
		}

private:
	const ZooLib::DListHead<L, kDebug>& fDListHead;
	L* fCurrent;
	};

// =================================================================================================
#pragma mark -
#pragma mark * DListIteratorEraseAll

template <typename P, typename L, int kDebug = 1>
class DListIteratorEraseAll
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES_T(DListIteratorEraseAll, operator_bool_generator_type, operator_bool_type);
public:
	DListIteratorEraseAll(ZooLib::DListHead<L, kDebug>& ioDListHead)
	:	fDListHead(ioDListHead),
		fCurrent(ioDListHead.fHeadP)
		{
		if (fCurrent)
			{
			ZAssertStop(kDebug, fCurrent->fNext);
			ZAssertStop(kDebug, fCurrent->fPrev);
			fNext = fCurrent->fNext;
			fCurrent->fNext = nil;
			fCurrent->fPrev = nil;
			}
		else
			{
			fNext = nil;
			}
		}

	~DListIteratorEraseAll()
		{
		fDListHead.fHeadP = nil;
		fDListHead.fSize = 0;
		}

	operator operator_bool_type() const
		{
		ZAssertStop(kDebug, fCurrent && fNext || !fCurrent && !fNext);
		return operator_bool_generator_type::translate(fCurrent);
		}

	P* Current() const
		{ return static_cast<P*>(fCurrent); }

	void Advance()
		{
		ZAssertStop(kDebug, fCurrent);
		ZAssertStop(kDebug, fNext);
		if (fNext == fDListHead.fHeadP)
			{
			fCurrent = nil;
			fNext = nil;
			}
		else
			{
			ZAssertStop(kDebug, fNext->fNext);
			fCurrent = fNext;
			fNext = fNext->fNext;
			fCurrent->fNext = nil;
			fCurrent->fPrev = nil;
			}
		}

private:
	ZooLib::DListHead<L, kDebug>& fDListHead;
	L* fCurrent;
	L* fNext;
	};

} // namespace ZooLib

#endif // __ZDList__
