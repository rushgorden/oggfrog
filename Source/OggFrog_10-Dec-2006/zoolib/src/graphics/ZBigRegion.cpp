static const char rcsid[] = "@(#) $Id: ZBigRegion.cpp,v 1.11 2006/04/27 03:57:14 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2000 Andrew Green and Learning in Motion, Inc.
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

/************************************************************************

Copyright (c) 1987, 1988 X Consortium

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
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#include "ZBigRegion.h"
#include "ZDebug.h"
#include "ZMemory.h"

using std::max;
using std::min;
using std::vector;

#define kDebug_BigRegion 3

// =================================================================================================
#pragma mark -
#pragma mark * ZBigRegion

ZBigRegion ZBigRegion::sRects(const ZRect_T<int32>* iRects, size_t iCount, bool iAlreadySorted)
	{
	ZBigRegion resultRgn;
	if (iCount > 0)
		{
		if (iAlreadySorted)
			{
			delete[] resultRgn.fRects;
			resultRgn.fRects = new ZRect_T<int32>[iCount];
			resultRgn.fNumRectsAllocated = iCount;
			resultRgn.fNumRects = iCount;

			ZRect_T<int32> bounds;
			bounds.top = LONG_MAX;
			bounds.left = LONG_MAX;
			bounds.right = LONG_MIN;
			bounds.bottom = LONG_MIN;

			ZRect_T<int32>* currentDest = resultRgn.fRects;
			while (iCount--)
				{
				ZRect_T<int32> currentRect = *iRects++;
				if (bounds.left > currentRect.left)
					bounds.left = currentRect.left;
				if (bounds.top > currentRect.top)
					bounds.top = currentRect.top;
				if (bounds.right < currentRect.right)
					bounds.right = currentRect.right;
				if (bounds.bottom < currentRect.bottom)
					bounds.bottom = currentRect.bottom;
				*currentDest++ = currentRect;
				}
			resultRgn.fExtent = bounds;
			}
		else
			{
			ZBigRegionAccumulator theAccumulator;
			while (iCount--)
				theAccumulator.Add(*iRects++);
			resultRgn = theAccumulator.GetResult();
			}
		}
	return resultRgn;
	}

// ==================================================

ZBigRegion::ZBigRegion()
:	fRects(new ZRect_T<int32>[1]),
	fNumRectsAllocated(1),
	fNumRects(0)
	{
	fExtent.left = 0;
	fExtent.right = 0;
	fExtent.top = 0;
	fExtent.bottom = 0;
	}

ZBigRegion::ZBigRegion(const ZBigRegion& iOther)
	{
	fNumRects = iOther.fNumRects;
	fExtent = iOther.fExtent;
	fRects = new ZRect_T<int32>[fNumRects];
	fNumRectsAllocated = fNumRects;
	ZBlockCopy(iOther.fRects, fRects, fNumRects * sizeof(ZRect_T<int32>));
	}

ZBigRegion::ZBigRegion(const ZRect_T<int32>& iBounds)
:	fRects(new ZRect_T<int32>[1]),
	fNumRectsAllocated(1),
	fNumRects(1)
	{
	fExtent.left = iBounds.left;
	fExtent.top = iBounds.top;
	fExtent.right = iBounds.right;
	fExtent.bottom = iBounds.bottom;
	fRects[0] = fExtent;
	}

ZBigRegion::ZBigRegion(const ZPoint_T<int32>& iSize)
:	fRects(new ZRect_T<int32>[1]),
	fNumRectsAllocated(1),
	fNumRects(1)
	{
	fExtent.left = 0;
	fExtent.top = 0;
	fExtent.right = iSize.h;
	fExtent.bottom = iSize.v;
	fRects[0] = fExtent;
	}

ZBigRegion::~ZBigRegion()
	{
	delete[] fRects;
	}

ZBigRegion& ZBigRegion::operator=(const ZBigRegion& iOther)
	{
	if (&iOther != this)
		{
		if (fNumRectsAllocated < iOther.fNumRects)
			{
			delete[] fRects;
			fRects = new ZRect_T<int32>[iOther.fNumRects];
			fNumRectsAllocated = iOther.fNumRects;
			}

		fNumRects = iOther.fNumRects;
		fExtent = iOther.fExtent;
		ZBlockCopy(iOther.fRects, fRects, fNumRects * sizeof(ZRect_T<int32>));
		}
	return *this;
	}

ZBigRegion& ZBigRegion::operator=(const ZRect_T<int32>& iBounds)
	{
	fExtent.left = iBounds.left;
	fExtent.top = iBounds.top;
	fExtent.right = iBounds.right;
	fExtent.bottom = iBounds.bottom;
	fNumRects = 1;
	fRects[0] = fExtent;
	return *this;
	}

static void sCompress(ZBigRegion& ioRegion,
	ZBigRegion& ioTempRegion1, ZBigRegion& ioTempRegion2,
	int32 delta, bool iHorizontal, bool iGrow)
	{
	int32 shift = 1;
	ioTempRegion1 = ioRegion;
	while (delta)
		{
		if (delta & shift)
			{
			if (iHorizontal)
				ioRegion += ZPoint_T<int32>(-shift, 0);
			else
				ioRegion += ZPoint_T<int32>(0, -shift);

			if (iGrow)
				ioRegion |= ioTempRegion1;
			else
				ioRegion &= ioTempRegion1;

			delta -= shift;
			if (delta == 0)
				break;
			}

		ioTempRegion2 = ioTempRegion1;

		if (iHorizontal)
			ioTempRegion1 += ZPoint_T<int32>(-shift, 0);
		else
			ioTempRegion1 += ZPoint_T<int32>(0, -shift);

		if (iGrow)
			ioTempRegion1 |= ioTempRegion2;
		else
			ioTempRegion1 &= ioTempRegion2;
		shift <<= 1;
		}
	}

void ZBigRegion::MakeInset(int32 iInsetH, int32 iInsetV)
	{
	if (fNumRects == 0)
		return;

	if (fNumRects == 1)
		{
		// Need to normalize these
		fExtent.left += iInsetH;
		fExtent.top += iInsetV;
		fExtent.right -= iInsetH;
		fExtent.bottom -= iInsetV;
		return;
		}

	ZBigRegion tempRegion1;
	ZBigRegion tempBigRegion2;

	bool grow = iInsetH < 0;
	if (grow)
		iInsetH = -iInsetH;
	sCompress(*this, tempRegion1, tempBigRegion2, 2 * iInsetH, true, grow);

	grow = iInsetV < 0;
	if (grow)
		iInsetV = -iInsetV;
	sCompress(*this, tempRegion1, tempBigRegion2, 2*iInsetV, false, grow);

	*this += ZPoint_T<int32>(iInsetH, iInsetV);
	}

bool ZBigRegion::Contains(const ZPoint_T<int32>& iPoint) const
	{
	if (fNumRects == 0)
		return false;

	if (!fExtent.Contains(iPoint))
		return false;

	// Simple check -- later we can do a binary chop, or even just bail when the top
	// of the current rectangle is >= thePoint.v
	for (size_t x = 0; x < fNumRects; ++x)
		{
		if (fRects[x].Contains(iPoint))
			return true;
		}

	return false;
	}

bool ZBigRegion::Contains(int32 iH, int32 iV) const
	{
	if (fNumRects == 0)
		return false;

	if (!fExtent.Contains(iH, iV))
		return false;

	// Simple check -- later we can do a binary chop, or even just bail when the top
	// of the current rectangle is >= thePoint.v
	for (size_t x = 0; x < fNumRects; ++x)
		{
		if (fRects[x].Contains(iH, iV))
			return true;
		}

	return false;
	}

void ZBigRegion::MakeEmpty()
	{
	fNumRects = 0;
	fExtent.left = 0;
	fExtent.right = 0;
	fExtent.top = 0;
	fExtent.bottom = 0;
	}

bool ZBigRegion::IsEmpty() const
	{ return fNumRects == 0; }

ZRect_T<int32> ZBigRegion::Bounds() const
	{ return ZRect_T<int32>(fExtent.left, fExtent.top, fExtent.right, fExtent.bottom); }

bool ZBigRegion::IsSimpleRect() const
	{ return fNumRects <= 1; }

void ZBigRegion::Decompose(vector<ZRect_T<int32> >& oRects) const
	{
	oRects.empty();
	oRects.reserve(fNumRects);
	for (size_t x = 0; x < fNumRects; ++x)
		oRects.push_back(fRects[x]);
	}

int32 ZBigRegion::Decompose(DecomposeProc iProc, void* iRefcon) const
	{
	int32 callbacksMade = 0;
	for (size_t x = 0; x < fNumRects; ++x)
		{
		++callbacksMade;
		if (iProc(fRects[x], iRefcon))
			break;
		}
	return callbacksMade;
	}

bool ZBigRegion::operator==(const ZBigRegion& iOther) const
	{
	if (fNumRects != iOther.fNumRects)
		return false;
	else if (fNumRects == 0)
		return true;
	else if (fExtent != iOther.fExtent)
		return false;
	else
		{
		for (size_t x = 0; x < fNumRects; ++x)
			{
			if (fRects[x] != iOther.fRects[x])
				return false;
			}
		}
	return true;
	}

ZBigRegion& ZBigRegion::operator+=(const ZPoint_T<int32>& iOffset)
	{
	fExtent += iOffset;
	for (size_t x = 0; x < fNumRects; ++x)
		fRects[x] += iOffset;
	return *this;
	}

ZBigRegion& ZBigRegion::operator|=(const ZBigRegion& iOther)
	{
	sUnion(*this, iOther, *this);
	return *this;
	}

ZBigRegion& ZBigRegion::operator&=(const ZBigRegion& iOther)
	{
	sIntersection(*this, iOther, *this);
	return *this;
	}

ZBigRegion& ZBigRegion::operator-=(const ZBigRegion& iOther)
	{
	sDifference(*this, iOther, *this);
	return *this;
	}

ZBigRegion& ZBigRegion::operator^=(const ZBigRegion& iOther)
	{
	sExclusiveOr(*this, iOther, *this);
	return *this;
	}

// =================================================================================================

void ZBigRegion::sInternal_Reallocate(ZBigRegion& ioRegion, ZRect_T<int32>*& oRect)
	{
	ZRect_T<int32>* newArray = new ZRect_T<int32>[2 * ioRegion.fNumRectsAllocated];
	ZBlockCopy(ioRegion.fRects, newArray, ioRegion.fNumRectsAllocated * sizeof(ZRect_T<int32>));
	delete[] ioRegion.fRects;
	ioRegion.fRects = newArray;
	ioRegion.fNumRectsAllocated = 2 * ioRegion.fNumRectsAllocated;
	oRect = &ioRegion.fRects[ioRegion.fNumRects];
	}

inline void ZBigRegion::sInternal_SpaceCheck(ZBigRegion& ioRegion, ZRect_T<int32>*& oRect)
	{
	if (ioRegion.fNumRects >= ioRegion.fNumRectsAllocated - 1)
		sInternal_Reallocate(ioRegion, oRect);
	}

void ZBigRegion::sInternal_Copy(const ZBigRegion& iSource, ZBigRegion& oDestination)
	{
	if (&iSource == &oDestination)
		return;

	if (oDestination.fNumRectsAllocated < iSource.fNumRects)
		{
		ZRect_T<int32>* newArray = new ZRect_T<int32>[iSource.fNumRects];
		delete[] oDestination.fRects;
		oDestination.fRects = newArray;
		oDestination.fNumRectsAllocated = iSource.fNumRects;
		}

	oDestination.fNumRects = iSource.fNumRects;
	oDestination.fExtent = iSource.fExtent;

	ZBlockCopy(iSource.fRects, oDestination.fRects,
		oDestination.fNumRects * sizeof(ZRect_T<int32>));
	}

void ZBigRegion::sInternal_SetExtents(ZBigRegion& ioRegion)
	{
	if (ioRegion.fNumRects == 0)
		{
		ioRegion.fExtent.left = 0;
		ioRegion.fExtent.top = 0;
		ioRegion.fExtent.right = 0;
		ioRegion.fExtent.bottom = 0;
		return;
		}

	ZRect_T<int32>* pExtents = &ioRegion.fExtent;
	ZRect_T<int32>* pBox = ioRegion.fRects;
	ZRect_T<int32>* pBoxEnd = &pBox[ioRegion.fNumRects - 1];

	// Since pBox is the first rectangle in the region, it must have the
	// smallest y1 and since pBoxEnd is the last rectangle in the region,
	// it must have the largest y2, because of banding. Initialize left and
	// x2 from pBox and pBoxEnd, resp., as good things to initialize them
	// to...
	pExtents->left = pBox->left;
	pExtents->top = pBox->top;
	pExtents->right = pBoxEnd->right;
	pExtents->bottom = pBoxEnd->bottom;

	ZAssertStop(kDebug_BigRegion, pExtents->top < pExtents->bottom);
	while (pBox <= pBoxEnd)
		{
		if (pBox->left < pExtents->left)
			pExtents->left = pBox->left;
		if (pBox->right > pExtents->right)
			pExtents->right = pBox->right;
		++pBox;
		}
	ZAssertStop(kDebug_BigRegion, pExtents->left < pExtents->right);
	}

// ==================================================

void ZBigRegion::sUnion(const ZBigRegion& iSource1, const ZBigRegion& iSource2,
	ZBigRegion& oDestination)
	{
	// Check for all the simple cases

	if ((&iSource1 == &iSource2) || (iSource1.fNumRects == 0))
		{
		// iSource1 and iSource2 are the same, or iSource1 is empty
		if (&iSource2 != &oDestination)
			sInternal_Copy(iSource2, oDestination);
		return;
		}

	if (iSource2.fNumRects == 0)
		{
		// iSource2 is empty
		if (&iSource1 != &oDestination)
			sInternal_Copy(iSource1, oDestination);
		return;
		}

	if (iSource1.fNumRects == 1
		&& iSource1.fExtent.left <= iSource2.fExtent.left
		&& iSource1.fExtent.top <= iSource2.fExtent.top
		&& iSource1.fExtent.right >= iSource2.fExtent.right
		&& iSource1.fExtent.bottom >= iSource2.fExtent.bottom)
		{
		// iSource1 is rectangular and completely subsumes iSource2
		if (&iSource1 != &oDestination)
			sInternal_Copy(iSource1, oDestination);
		return;
		}

	if (iSource2.fNumRects == 1
		&& iSource2.fExtent.left <= iSource1.fExtent.left
		&& iSource2.fExtent.top <= iSource1.fExtent.top
		&& iSource2.fExtent.right >= iSource1.fExtent.right
		&& iSource2.fExtent.bottom >= iSource1.fExtent.bottom)
		{
		// iSource2 is rectangular and completely subsumes iSource1
		if (&iSource2 != &oDestination)
			sInternal_Copy(iSource2, oDestination);
		return;
		}

	// Gotta do some real work
	sInternal_RegionOp(oDestination, iSource1, iSource2,
		sInternal_UnionOverlapping,
		sInternal_UnionNonOverlapping,
		sInternal_UnionNonOverlapping);

	oDestination.fExtent.left = min(iSource1.fExtent.left, iSource2.fExtent.left);
	oDestination.fExtent.top = min(iSource1.fExtent.top, iSource2.fExtent.top);
	oDestination.fExtent.right = max(iSource1.fExtent.right, iSource2.fExtent.right);
	oDestination.fExtent.bottom = max(iSource1.fExtent.bottom, iSource2.fExtent.bottom);
	}

void ZBigRegion::sInternal_UnionNonOverlapping(ZBigRegion& ioRegion,
	ZRect_T<int32>* r, ZRect_T<int32>* rEnd, int32 y1, int32 y2)
	{
	ZRect_T<int32>* pNextRect = &ioRegion.fRects[ioRegion.fNumRects];

	ZAssertStop(kDebug_BigRegion, y1 < y2);

	while (r != rEnd)
		{
		ZAssertStop(kDebug_BigRegion, r->left < r->right);
		sInternal_SpaceCheck(ioRegion, pNextRect);
		pNextRect->left = r->left;
		pNextRect->top = y1;
		pNextRect->right = r->right;
		pNextRect->bottom = y2;
		++ioRegion.fNumRects;
		++pNextRect;
		++r;

		ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects<= ioRegion.fNumRectsAllocated);
		}
	}

void ZBigRegion::sInternal_UnionOverlapping(ZBigRegion& ioRegion,
	ZRect_T<int32>* r1, ZRect_T<int32>* r1End,
	ZRect_T<int32>* r2, ZRect_T<int32>* r2End,
	int32 y1, int32 y2)
	{
	ZRect_T<int32>* pNextRect = &ioRegion.fRects[ioRegion.fNumRects];

	#define MERGERECT(r) \
	if ((ioRegion.fNumRects != 0) \
		&& (pNextRect[-1].top == y1) \
		&& (pNextRect[-1].bottom == y2) \
		&& (pNextRect[-1].right >= r->left)) \
		{ \
		if (pNextRect[-1].right < r->right) \
			{ \
			pNextRect[-1].right = r->right; \
			ZAssertStop(kDebug_BigRegion, pNextRect[-1].left< pNextRect[-1].right); \
			} \
		} \
	else \
		{ \
		sInternal_SpaceCheck(ioRegion, pNextRect);\
		pNextRect->top = y1; \
		pNextRect->bottom = y2; \
		pNextRect->left = r->left; \
		pNextRect->right = r->right; \
		++ioRegion.fNumRects; \
		++pNextRect; \
		} \
	ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects<=ioRegion.fNumRectsAllocated);\
	++r;

	ZAssertStop(kDebug_BigRegion, y1< y2);
	while ((r1 != r1End) && (r2 != r2End))
		{
		if (r1->left < r2->left)
			{
			MERGERECT(r1);
			}
		else
			{
			MERGERECT(r2);
			}
		}

	if (r1 != r1End)
		{
		do
			{
			MERGERECT(r1);
			} while (r1 != r1End);
		}
	else while (r2 != r2End)
		{
		MERGERECT(r2);
		}
	}

// ==================================================

void ZBigRegion::sIntersection(const ZBigRegion& iSource1, const ZBigRegion& iSource2,
	ZBigRegion& oDestination)
	{
	if (iSource1.fNumRects == 0
		|| iSource2.fNumRects == 0
		|| iSource1.fExtent.right <= iSource2.fExtent.left
		|| iSource1.fExtent.left >= iSource2.fExtent.right
		|| iSource1.fExtent.bottom <= iSource2.fExtent.top
		|| iSource1.fExtent.top >= iSource2.fExtent.bottom)
		{
		// Either iSource1 or iSource2 are completely empty, or their extents do not overlap.
		// In either case the result is an empty region.
		oDestination.fNumRects = 0;
		oDestination.fExtent.left = 0;
		oDestination.fExtent.top = 0;
		oDestination.fExtent.right = 0;
		oDestination.fExtent.bottom = 0;
		return;
		}

	sInternal_RegionOp(oDestination, iSource1, iSource2,
		sInternal_IntersectionOverlapping, nil, nil);

	// Can't alter oDestination's extents before we call sInternal_RegionOp because
	// it might be one of the source regions and sInternal_RegionOp depends
	// on the extents of those regions being the same. Besides, this
	// way there's no checking against rectangles that will be nuked
	// due to coalescing, so we have to examine fewer rectangles.
	sInternal_SetExtents(oDestination);
	}

void ZBigRegion::sInternal_IntersectionOverlapping(ZBigRegion& ioRegion,
	ZRect_T<int32>* r1, ZRect_T<int32>* r1End,
	ZRect_T<int32>* r2, ZRect_T<int32>* r2End,
	int32 y1, int32 y2)
	{
	ZRect_T<int32>* pNextRect = &ioRegion.fRects[ioRegion.fNumRects];

	while ((r1 != r1End) && (r2 != r2End))
		{
		int32 x1 = max(r1->left,r2->left);
		int32 x2 = min(r1->right,r2->right);

		// If there's any overlap between the two rectangles, add that
		// overlap to the new region.
		// There's no need to check for subsumption because the only way
		// such a need could arise is if some region has two rectangles
		// right next to each other. Since that should never happen...
		if (x1 < x2)
			{
			ZAssertStop(kDebug_BigRegion, y1< y2);

			sInternal_SpaceCheck(ioRegion, pNextRect);
			pNextRect->left = x1;
			pNextRect->top = y1;
			pNextRect->right = x2;
			pNextRect->bottom = y2;
			++ioRegion.fNumRects;
			++pNextRect;
			ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects <= ioRegion.fNumRectsAllocated);
			}

		// Need to advance the pointers. Shift the one that extends
		// to the right the least, since the other still has a chance to
		// overlap with that region's next rectangle, if you see what I mean.
		if (r1->right < r2->right)
			{
			++r1;
			}
		else if (r2->right < r1->right)
			{
			++r2;
			}
		else
			{
			++r1;
			++r2;
			}
		}
	}

// ==================================================

void ZBigRegion::sDifference(const ZBigRegion& iSource1, const ZBigRegion& iSource2,
	ZBigRegion& oDestination)
	{
	if (iSource1.fNumRects == 0
		|| iSource2.fNumRects == 0
		|| iSource1.fExtent.right <= iSource2.fExtent.left
		|| iSource1.fExtent.left >= iSource2.fExtent.right
		|| iSource1.fExtent.bottom <= iSource2.fExtent.top
		|| iSource1.fExtent.top >= iSource2.fExtent.bottom)
		{
		// One or other source regions are completely empty, or there's no overlap.
		sInternal_Copy(iSource1, oDestination);
		return;
		}

	sInternal_RegionOp(oDestination, iSource1, iSource2,
		sInternal_DifferenceOverlapping, sInternal_DifferenceNonOverlapping, nil);

	// Can't alter destination's extents before we call sInternal_RegionOp because
	// it might be one of the source regions and sInternal_RegionOp depends
	// on the extents of those regions being the same. Besides, this
	// way there's no checking against rectangles that will be nuked
	// due to coalescing, so we have to examine fewer rectangles.
	sInternal_SetExtents(oDestination);
	}

void ZBigRegion::sInternal_DifferenceNonOverlapping(ZBigRegion& ioRegion,
	ZRect_T<int32>* r, ZRect_T<int32>* rEnd,
	int32 y1, int32 y2)
	{
	ZRect_T<int32>* pNextRect = &ioRegion.fRects[ioRegion.fNumRects];

	ZAssertStop(kDebug_BigRegion, y1<y2);

	while (r != rEnd)
		{
		ZAssertStop(kDebug_BigRegion, r->left<r->right);
		sInternal_SpaceCheck(ioRegion, pNextRect);
		pNextRect->left = r->left;
		pNextRect->top = y1;
		pNextRect->right = r->right;
		pNextRect->bottom = y2;
		++ioRegion.fNumRects;
		++pNextRect;

		ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects <= ioRegion.fNumRectsAllocated);

		++r;
		}
	}

void ZBigRegion::sInternal_DifferenceOverlapping(ZBigRegion& ioRegion,
	ZRect_T<int32>* r1, ZRect_T<int32>* r1End,
	ZRect_T<int32>* r2, ZRect_T<int32>* r2End,
	int32 y1, int32 y2)
	{
	int32 x1 = r1->left;

	ZAssertStop(kDebug_BigRegion, y1<y2);
	ZRect_T<int32>* pNextRect = &ioRegion.fRects[ioRegion.fNumRects];

	while ((r1 != r1End) && (r2 != r2End))
		{
		if (r2->right <= x1)
			{
			// Subtrahend missed the boat: go to next subtrahend.
			++r2;
			}
		else if (r2->left <= x1)
			{
			// Subtrahend preceeds minuend: nuke left edge of minuend.
			x1 = r2->right;
			if (x1 >= r1->right)
				{
				// Minuend completely covered: advance to next minuend and
				// reset left fence to edge of new minuend.
				++r1;
				if (r1 != r1End)
					x1 = r1->left;
				}
			else
				{
				// Subtrahend now used up since it doesn't
				// extend beyond minuend
				++r2;
				}
			}
		else if (r2->left < r1->right)
			{
			// Left part of subtrahend covers part of minuend: add uncovered
			// part of minuend to region and skip to next subtrahend.
			ZAssertStop(kDebug_BigRegion, x1<r2->left);
			sInternal_SpaceCheck(ioRegion, pNextRect);
			pNextRect->left = x1;
			pNextRect->top = y1;
			pNextRect->right = r2->left;
			pNextRect->bottom = y2;
			++ioRegion.fNumRects;
			++pNextRect;

			ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects<= ioRegion.fNumRectsAllocated);

			x1 = r2->right;
			if (x1 >= r1->right)
				{
				// Minuend used up: advance to new...
				++r1;
				if (r1 != r1End)
					x1 = r1->left;
				}
			else
				{
				// Subtrahend used up
				++r2;
				}
			}
		else
			{
			// Minuend used up: add any remaining piece before advancing.
			if (r1->right > x1)
				{
				sInternal_SpaceCheck(ioRegion, pNextRect);
				pNextRect->left = x1;
				pNextRect->top = y1;
				pNextRect->right = r1->right;
				pNextRect->bottom = y2;
				++ioRegion.fNumRects;
				++pNextRect;
				ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects <= ioRegion.fNumRectsAllocated);
				}
			++r1;
			x1 = r1->left;
			}
		}

	// Add remaining minuend rectangles to region.
	while (r1 != r1End)
		{
		ZAssertStop(kDebug_BigRegion, x1<r1->right);
		sInternal_SpaceCheck(ioRegion, pNextRect);
		pNextRect->left = x1;
		pNextRect->top = y1;
		pNextRect->right = r1->right;
		pNextRect->bottom = y2;
		++ioRegion.fNumRects;
		++pNextRect;

		ZAssertStop(kDebug_BigRegion, ioRegion.fNumRects <= ioRegion.fNumRectsAllocated);

		++r1;

		if (r1 != r1End)
			x1 = r1->left;
		}
	}

// ==================================================

void ZBigRegion::sExclusiveOr(const ZBigRegion& iSource1, const ZBigRegion& iSource2,
	ZBigRegion& oDestination)
	{
	ZBigRegion tempRegionA;
	ZBigRegion tempRegionB;
	sDifference(iSource1, iSource2, tempRegionA);
	sDifference(iSource2, iSource1, tempRegionB);
	sUnion(tempRegionA, tempRegionB, oDestination);
	}

// ==================================================

void ZBigRegion::sInternal_RegionOp(ZBigRegion& ioNewRegion,
	const ZBigRegion& iRegion1, const ZBigRegion& iRegion2,
	OverlappingFuncPtr iOverlapFunc,
	NonOverlappingFuncPtr iNonOverlapFunc1,
	NonOverlappingFuncPtr iNonOverlapFunc2)
	{
	// Initialization:
	// set r1, r2, r1End and r2End appropriately, preserve the important
	// parts of the destination region until the end in case it's one of
	// the two source regions, then mark the "new" region empty, allocating
	// another array of rectangles for it to use.

	ZRect_T<int32>* r1 = iRegion1.fRects;
	ZRect_T<int32>* r2 = iRegion2.fRects;
	ZRect_T<int32>* r1End = r1 + iRegion1.fNumRects;
	ZRect_T<int32>* r2End = r2 + iRegion2.fNumRects;

	ZRect_T<int32>* oldRects = ioNewRegion.fRects;

	ioNewRegion.fNumRects = 0;

	// Allocate a reasonable number of rectangles for the new region. The idea
	// is to allocate enough so the individual functions don't need to
	// reallocate and copy the array, which is time consuming, yet we don't
	// have to worry about using too much memory. I hope to be able to
	// nuke the Xrealloc() at the end of this function eventually.

	ioNewRegion.fNumRectsAllocated = 2 * max(iRegion1.fNumRects, iRegion2.fNumRects);
	ioNewRegion.fRects = new ZRect_T<int32>[ioNewRegion.fNumRectsAllocated];

	// Initialize ybot and ytop.
	// In the upcoming loop, ybot and ytop serve different functions depending
	// on whether the band being handled is an overlapping or non-overlapping
	// band.
	// In the case of a non-overlapping band (only one of the regions
	// has points in the band), ybot is the bottom of the most recent
	// intersection and thus clips the top of the rectangles in that band.
	// ytop is the top of the next intersection between the two regions and
	// serves to clip the bottom of the rectangles in the current band.
	//	For an overlapping band (where the two regions intersect), ytop clips
	// the top of the rectangles of both regions and ybot clips the bottoms.

	// Bottom of intersection
	int32 ybot = min(iRegion1.fExtent.top, iRegion2.fExtent.top);

	// prevBand serves to mark the start of the previous band so rectangles
	// can be coalesced into larger rectangles. qv. miCoalesce, above.
	// In the beginning, there is no previous band, so prevBand == curBand
	// (curBand is set later on, of course, but the first band will always
	// start at index 0). prevBand and curBand must be indices because of
	// the possible expansion, and resultant moving, of the new region's
	// array of rectangles.

	int prevBand = 0;
	for (;;)
		{
		int curBand = ioNewRegion.fNumRects;

		// This algorithm proceeds one source-band (as opposed to a
		// destination band, which is determined by where the two regions
		// intersect) at a time. r1BandEnd and r2BandEnd serve to mark the
		// rectangle after the last one in the current band for their
		// respective regions.

		ZRect_T<int32>* r1BandEnd = r1;
		while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
			r1BandEnd++;

		ZRect_T<int32>* r2BandEnd = r2;
		while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
			r2BandEnd++;


		// First handle the band that doesn't intersect, if any.
		// Note that attention is restricted to one band in the
		// non-intersecting region at once, so if a region has n
		// bands between the current position and the next place it overlaps
		// the other, this entire loop will be passed through n times.

		int32 ytop;
		int32 top;
		int32 bot;
		if (r1->top < r2->top)
			{
			top = max(r1->top,ybot);
			bot = min(r1->bottom,r2->top);

			if (iNonOverlapFunc1 && top != bot)
				iNonOverlapFunc1(ioNewRegion, r1, r1BandEnd, top, bot);

			ytop = r2->top;
			}
		else if (r2->top < r1->top)
			{
			top = max(r2->top,ybot);
			bot = min(r2->bottom,r1->top);

			if (iNonOverlapFunc2 && top != bot)
				iNonOverlapFunc2(ioNewRegion, r2, r2BandEnd, top, bot);

			ytop = r1->top;
			}
		else
			{
			ytop = r1->top;
			}


		// If any rectangles got added to the region, try and coalesce them
		// with rectangles from the previous band. Note we could just do
		// this test in miCoalesce, but some machines incur a not
		// inconsiderable cost for function calls, so...

		if (ioNewRegion.fNumRects != curBand)
			prevBand = sInternal_Coalesce(ioNewRegion, prevBand, curBand);


		// Now see if we've hit an intersecting band. The two bands only
		// intersect if ybot > ytop

		ybot = min(r1->bottom, r2->bottom);
		curBand = ioNewRegion.fNumRects;
		if (ybot > ytop)
			iOverlapFunc(ioNewRegion, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

		if (ioNewRegion.fNumRects != curBand)
			prevBand = sInternal_Coalesce(ioNewRegion, prevBand, curBand);

		// If we've finished with a band (y2 == ybot) we skip forward
		// in the region to the next band.

		if (r1->bottom == ybot)
			r1 = r1BandEnd;
		if (r2->bottom == ybot)
			r2 = r2BandEnd;

		if ((r1 == r1End) || (r2 == r2End))
			break;
		}

	// Deal with whichever region still has rectangles left.

	int32 curBand = ioNewRegion.fNumRects;
	if (iNonOverlapFunc1 && r1 != r1End)
		{
		do
			{
			ZRect_T<int32>* r1BandEnd = r1;
			while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
				r1BandEnd++;

			iNonOverlapFunc1(ioNewRegion, r1, r1BandEnd, max(r1->top,ybot), r1->bottom);
			r1 = r1BandEnd;
			} while (r1 != r1End);
		}
	else if (iNonOverlapFunc2 && r2 != r2End)
		{
		do
			{
			ZRect_T<int32>* r2BandEnd = r2;
			while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
				r2BandEnd++;

			iNonOverlapFunc2(ioNewRegion, r2, r2BandEnd, max(r2->top,ybot), r2->bottom);
			r2 = r2BandEnd;
			} while (r2 != r2End);
		}

	if (ioNewRegion.fNumRects != curBand)
		sInternal_Coalesce(ioNewRegion, prevBand, curBand);


	// A bit of cleanup. To keep regions from growing without bound,
	// we shrink the array of rectangles to match the new number of
	// rectangles in the region. This never goes to 0, however...
	// Only do this stuff if the number of rectangles allocated is more than
	// twice the number of rectangles in the region (a simple optimization...).

	if (ioNewRegion.fNumRects < (ioNewRegion.fNumRectsAllocated >> 1))
		{
		ioNewRegion.fNumRectsAllocated = ioNewRegion.fNumRects;
		if (ioNewRegion.fNumRects == 0)
			ioNewRegion.fNumRectsAllocated = 1;
		ZRect_T<int32>* newRects = new ZRect_T<int32>[ioNewRegion.fNumRectsAllocated];
		ZBlockCopy(ioNewRegion.fRects, newRects, ioNewRegion.fNumRects * sizeof(ZRect_T<int32>));
		delete[] ioNewRegion.fRects;
		ioNewRegion.fRects = newRects;
		}
	delete[] oldRects;
	}

int32 ZBigRegion::sInternal_Coalesce(ZBigRegion& ioRegion, int32 prevStart, int32 curStart)
	{
	ZRect_T<int32>* pRegEnd = &ioRegion.fRects[ioRegion.fNumRects];
	ZRect_T<int32>* pPrevBox = &ioRegion.fRects[prevStart];
	int32 prevNumRects = curStart - prevStart;

	// Figure out how many rectangles are in the current band. Have to do
	// this because multiple bands could have been added in sInternal_RegionOp
	// at the end when one region has been exhausted.
	ZRect_T<int32>* pCurBox = &ioRegion.fRects[curStart];
	int32 bandY1 = pCurBox->top;
	int32 curNumRects;
	for (curNumRects = 0; (pCurBox != pRegEnd) && (pCurBox->top == bandY1); ++curNumRects)
		++pCurBox;

	if (pCurBox != pRegEnd)
		{
		// If more than one band was added, we have to find the start
		// of the last band added so the next coalescing job can start
		// at the right place... (given when multiple bands are added,
		// this may be pointless -- see above).
		--pRegEnd;
		while (pRegEnd[-1].top == pRegEnd->top)
			--pRegEnd;
		curStart = pRegEnd - ioRegion.fRects;
		pRegEnd = ioRegion.fRects + ioRegion.fNumRects;
		}

	if ((curNumRects == prevNumRects) && (curNumRects != 0))
		{
		pCurBox -= curNumRects;

		// The bands may only be coalesced if the bottom of the previous
		// matches the top scanline of the current.
		if (pPrevBox->bottom == pCurBox->top)
			{
			// Make sure the bands have boxes in the same places. This
			// assumes that boxes have been added in such a way that they
			// cover the most area possible. I.e. two boxes in a band must
			// have some horizontal space between them.
			do
				{
				if ((pPrevBox->left != pCurBox->left) || (pPrevBox->right != pCurBox->right))
					{
					// The bands don't line up so they can't be coalesced.
					return curStart;
					}
				++pPrevBox;
				++pCurBox;
				prevNumRects -= 1;
				} while (prevNumRects != 0);

			ioRegion.fNumRects -= curNumRects;
			pCurBox -= curNumRects;
			pPrevBox -= curNumRects;

			// The bands may be merged, so set the bottom y of each box
			// in the previous band to that of the corresponding box in
			// the current band.
			do
				{
				pPrevBox->bottom = pCurBox->bottom;
				++pPrevBox;
				++pCurBox;
				curNumRects -= 1;
				} while (curNumRects != 0);

			// If only one band was added to the region, we have to backup
			// curStart to the start of the previous band.
			// If more than one band was added to the region, copy the
			// other bands down. The assumption here is that the other bands
			// came from the same region as the current one and no further
			// coalescing can be done on them since it's all been done
			// already... curStart is already in the right place.
			if (pCurBox == pRegEnd)
				{
				curStart = prevStart;
				}
			else
				{
				do
					{
					*pPrevBox++ = *pCurBox++;
					} while (pCurBox != pRegEnd);
				}
			}
		}
	return curStart;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBigRegionAccumulator

// See ZDCRgnAccumulator (in ZDCRgn.cpp) for discussion.

ZBigRegionAccumulator::ZBigRegionAccumulator()
:	fCount(0)
	{}

ZBigRegionAccumulator::~ZBigRegionAccumulator()
	{}

static inline uint32 sHammingWeight(uint32 w)
	{
	uint32 res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
	res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
	res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
	res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
	return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
	}

void ZBigRegionAccumulator::Add(const ZBigRegion& inRgn)
	{
	fStack.push_back(inRgn);

	uint32 changedBits = fCount++;
	changedBits ^= fCount;
	uint32 changedBitsCount = sHammingWeight(changedBits);
	while (--changedBitsCount)
		{
		ZBigRegion tailRgn = fStack.back();
		fStack.pop_back();
		fStack.back() |= tailRgn;
		}
	}

ZBigRegion ZBigRegionAccumulator::GetResult() const
	{
	ZBigRegion result;
	for (vector<ZBigRegion>::const_iterator i = fStack.begin(); i != fStack.end(); ++i)
		result |= *i;
	return result;
	}

// =================================================================================================
