static const char rcsid[] = "@(#) $Id: ZBRegionAlternate.cpp,v 1.4 2006/10/16 03:20:44 agreen Exp $";

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
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



/************************************************************************

Copyright 2000 Andrew Green. ag@zoolib.org
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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
ANDREW GREEN BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

************************************************************************/

/*
AG 2000-02-28. ZBRegionAlternate provides an alternative suite of functions for manipulating BRegions. In
some situations it runs *much* faster than native BRegion calls do. If the source data is well formed (ie
was created by ZBRegionAlternate in the first place, or alrady has rectangles sorted within horizontal bands and
no overlaps from one band to another) it runs faster than BRegion for all operations except Exclude, where it's
either equivalent in runtime, or takes somewhat longer, again depending on the precise nature of the source data.

This code originally came from X-Windows, although I'd already reworked it several times for other reasons and
so looks quite different. BeOS uses an inclusive model for all its bounding rectangles, so that the rect (1,1,1,1)
describes not an empty rectangle but instead the rectangle enclosing one pixel at (1,1). This differs from
common practice, and certainly differs from what the X source expects. To keep things clear, instead of
modifying the code to take account of this directly I have instead modified it so that any use of a rect's right or
bottom fields has one added to it, and any store into a rect's right or bottom field has one subtracted from it. This
keeps the structure and logic of the code unchanged from the original source, at the expense of there being some
awkward constructions (eg if (r1->right + 1 > r2->right + 1), or r1->right = -1 + r2->right + 1).

The other significant addition is code to parse source data and verify that it is in a form that will allow
the real code to work correctly, and if not, to normalize it such that it does meet the required constraints.
The detection of malformed data and its normalization is not optimal. It would be possible to fold the detection
into the normal process of band scanning, and it would also be possible to normalize only those bands that
actually need it, on demand as it were. I have not done that yet as this is not mission-critical code for me,
merely the result of an unexpectedly involved investigation.
*/

#include "ZBRegionAlternate.h"

#if ZCONFIG(API_Graphics, Be)

#include <cstdlib> // For malloc, free, memcpy
#include <vector> // For vector, also pulls in min & max

#ifndef ZAssert
#	define ZAssert(a) ((void)0)
#endif

class ZBRA
	{
public:
// To make a ZBRA's memory layout match that of a BRegion we need to force a space for a vptr, hence this method
	virtual void Dummy() {}

	long fNumRects;
	long fNumRectsAllocated;
	clipping_rect fExtent;
	clipping_rect* fRects;
	};

// ==================================================
// Forward declarations

static clipping_rect* sAllocateRects(size_t inCount);

static void sUnion(const ZBRA* source1, const ZBRA* source2, ZBRA* destination);
static void sIntersection(const ZBRA* source1, const ZBRA* source2, ZBRA* destination);
static void sDifference(const ZBRA* source1, const ZBRA* source2, ZBRA* destination);

typedef void (*NonOverlappingFunctionPtr)(ZBRA* pReg, clipping_rect* r, clipping_rect* rEnd, long y1, long y2);
typedef void (*OverlappingFunctionPtr)(ZBRA* pReg, clipping_rect* r1, clipping_rect* r1End, clipping_rect* r2, clipping_rect* r2End, long y1, long y2);

static void sRegionOp(ZBRA* destRegion, const ZBRA* reg1, const ZBRA* reg2,
	OverlappingFunctionPtr overlapFunc, NonOverlappingFunctionPtr nonOverlapFunc1, NonOverlappingFunctionPtr nonOverlapFunc2);


// ==================================================

void ZBRegionAlternate_Assign(BRegion* inBRegionLHS, BRegion* inBRegionRHS)
	{
	if (inBRegionLHS != inBRegionRHS)
		{
		ZBRA* braLHS = reinterpret_cast<ZBRA*>(inBRegionLHS);
		ZBRA* braRHS = reinterpret_cast<ZBRA*>(inBRegionRHS);

		if (braLHS->fNumRectsAllocated < braRHS->fNumRects)
			{
			::free(braLHS->fRects);
			braLHS->fRects = sAllocateRects(braRHS->fNumRectsAllocated);
			braLHS->fNumRectsAllocated = braRHS->fNumRectsAllocated;
			}

		braLHS->fNumRects = braRHS->fNumRects;
		braLHS->fExtent = braRHS->fExtent;
		::memcpy(braLHS->fRects, braRHS->fRects, sizeof(clipping_rect) * braRHS->fNumRects);
		}
	}

void ZBRegionAlternate_Set(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom)
	{
	ZBRA* theBRA = reinterpret_cast<ZBRA*>(inBRegion);
	if (theBRA->fNumRectsAllocated > 8)
		{
		::free(theBRA->fRects);
		theBRA->fRects = sAllocateRects(8);
		theBRA->fNumRectsAllocated = 8;
		}

	if (inRight < inLeft || inBottom < inTop)
		{
		theBRA->fNumRects = 0;
		theBRA->fExtent.left = LONG_MAX;
		theBRA->fExtent.top = LONG_MAX;
		theBRA->fExtent.right = LONG_MIN;
		theBRA->fExtent.bottom = LONG_MIN;
		theBRA->fRects[0] = theBRA->fExtent;
		}
	else
		{
		theBRA->fNumRects = 1;
		theBRA->fExtent.left = inLeft;
		theBRA->fExtent.top = inTop;
		theBRA->fExtent.right = inRight;
		theBRA->fExtent.bottom = inBottom;
		theBRA->fRects[0] = theBRA->fExtent;
		}
	}

void ZBRegionAlternate_Set(BRegion* inBRegion, clipping_rect inNewBounds)
	{
	ZBRA* theBRA = reinterpret_cast<ZBRA*>(inBRegion);
	if (theBRA->fNumRectsAllocated > 8)
		{
		::free(theBRA->fRects);
		theBRA->fRects = sAllocateRects(8);
		theBRA->fNumRectsAllocated = 8;
		}

	if (inNewBounds.right < inNewBounds.left || inNewBounds.bottom < inNewBounds.top)
		{
		theBRA->fNumRects = 0;
		theBRA->fExtent.left = LONG_MAX;
		theBRA->fExtent.top = LONG_MAX;
		theBRA->fExtent.right = LONG_MIN;
		theBRA->fExtent.bottom = LONG_MIN;
		theBRA->fRects[0] = theBRA->fExtent;
		}
	else
		{
		theBRA->fNumRects = 1;
		theBRA->fRects[0] = inNewBounds;
		theBRA->fExtent = inNewBounds;
		}
	}

void ZBRegionAlternate_MakeEmpty(BRegion* inBRegion)
	{
	ZBRA* theBRA = reinterpret_cast<ZBRA*>(inBRegion);
	theBRA->fNumRects = 0;
	theBRA->fExtent.left = LONG_MAX;
	theBRA->fExtent.top = LONG_MAX;
	theBRA->fExtent.right = LONG_MIN;
	theBRA->fExtent.bottom = LONG_MIN;
	}

bool ZBRegionAlternate_IsEmpty(const BRegion* inBRegion)
	{ return reinterpret_cast<const ZBRA*>(inBRegion)->fNumRects == 0; }

bool ZBRegionAlternate_Contains(const BRegion* inBRegion, int32 inX, int32 inY)
	{
	const ZBRA* theBRA = reinterpret_cast<const ZBRA*>(inBRegion);
	if (theBRA->fNumRects == 0)
		return false;

	if (inX < theBRA->fExtent.left || inX > theBRA->fExtent.right || inY < theBRA->fExtent.top || inY > theBRA->fExtent.bottom)
		return false;
// Simple check -- later we can do a binary chop, or even just bail when the top of the current rectangle > inY
	for (long x = 0; x < theBRA->fNumRects; ++x)
		{
		if (inX >= theBRA->fRects[x].left && inX <= theBRA->fRects[x].right && inY >= theBRA->fRects[x].top && inY <= theBRA->fRects[x].bottom)
			return true;
		}
	return false;
	}

void ZBRegionAlternate_OffsetBy(BRegion* inBRegion, int32 dh, int32 dv)
	{
	ZBRA* theBRA = reinterpret_cast<ZBRA*>(inBRegion);
	theBRA->fExtent.left += dh;
	theBRA->fExtent.top += dv;
	theBRA->fExtent.right += dh;
	theBRA->fExtent.bottom += dv;
	long count = theBRA->fNumRects + 1;
	clipping_rect* currentRect = theBRA->fRects;
	while (--count)
		{
		currentRect->left += dh;
		currentRect->top += dv;
		currentRect->right += dh;
		currentRect->bottom += dv;
		++currentRect;
		}
	}

void ZBRegionAlternate_Include(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom)
	{
	ZBRA dummy;
	dummy.fExtent.left = inLeft;
	dummy.fExtent.top = inTop;
	dummy.fExtent.right = inRight;
	dummy.fExtent.bottom = inBottom;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sUnion(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_Include(BRegion* inBRegion, clipping_rect inRect)
	{
	ZBRA dummy;
	dummy.fExtent = inRect;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sUnion(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_Include(BRegion* inBRegion, const BRegion* inOther)
	{ sUnion(reinterpret_cast<ZBRA*>(inBRegion), reinterpret_cast<const ZBRA*>(inOther), reinterpret_cast<ZBRA*>(inBRegion)); }

void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom)
	{
	ZBRA dummy;
	dummy.fExtent.left = inLeft;
	dummy.fExtent.top = inTop;
	dummy.fExtent.right = inRight;
	dummy.fExtent.bottom = inBottom;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sIntersection(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, clipping_rect inRect)
	{
	ZBRA dummy;
	dummy.fExtent = inRect;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sIntersection(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, const BRegion* inOther)
	{ sIntersection(reinterpret_cast<ZBRA*>(inBRegion), reinterpret_cast<const ZBRA*>(inOther), reinterpret_cast<ZBRA*>(inBRegion)); }

void ZBRegionAlternate_Exclude(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom)
	{
	ZBRA dummy;
	dummy.fExtent.left = inLeft;
	dummy.fExtent.top = inTop;
	dummy.fExtent.right = inRight;
	dummy.fExtent.bottom = inBottom;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sDifference(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_Exclude(BRegion* inBRegion, clipping_rect inRect)
	{
	ZBRA dummy;
	dummy.fExtent = inRect;
	dummy.fRects = &dummy.fExtent;
	dummy.fNumRects = 1;
	dummy.fNumRectsAllocated = 1;
	sDifference(reinterpret_cast<ZBRA*>(inBRegion), &dummy, reinterpret_cast<ZBRA*>(inBRegion));
	}

void ZBRegionAlternate_Exclude(BRegion* inBRegion, const BRegion* inOther)
	{ sDifference(reinterpret_cast<ZBRA*>(inBRegion), reinterpret_cast<const ZBRA*>(inOther), reinterpret_cast<ZBRA*>(inBRegion)); }

// =================================================================================================

static clipping_rect* sAllocateRects(size_t inCount)
	{ return reinterpret_cast<clipping_rect*>(::malloc(inCount * sizeof(clipping_rect))); }

static void sReallocate(ZBRA* inRegion, clipping_rect*& outRect)
	{
	clipping_rect* newArray = sAllocateRects(2* inRegion->fNumRectsAllocated);
	::memcpy(newArray, inRegion->fRects, sizeof(clipping_rect) * inRegion->fNumRectsAllocated);
	::free(inRegion->fRects);
	inRegion->fRects = newArray;
	inRegion->fNumRectsAllocated = 2*inRegion->fNumRectsAllocated;
	outRect = &inRegion->fRects[inRegion->fNumRects];
	}

static inline void sSpaceCheck(ZBRA* inRegion, clipping_rect*& outRect)
	{
	if (inRegion->fNumRects >= inRegion->fNumRectsAllocated - 1)
		sReallocate(inRegion, outRect);
	}

static void sCopy(const ZBRA* inSource, ZBRA* inDestination)
	{
	if (inDestination == inSource)
		return;
	if (inDestination->fNumRectsAllocated < inSource->fNumRects)
		{
		clipping_rect* newArray = sAllocateRects(inSource->fNumRects);
		::free(inDestination->fRects);
		inDestination->fRects = newArray;
		inDestination->fNumRectsAllocated = inSource->fNumRects;
		}
	inDestination->fNumRects = inSource->fNumRects;
	inDestination->fExtent = inSource->fExtent;
	::memcpy(inDestination->fRects, inSource->fRects, inDestination->fNumRects * sizeof(clipping_rect));
	}

static void sSetExtents(ZBRA* inRegion)
	{
	if (inRegion->fNumRects == 0)
		{
		inRegion->fExtent.left = LONG_MAX;
		inRegion->fExtent.top = LONG_MAX;
		inRegion->fExtent.right = LONG_MIN;
		inRegion->fExtent.bottom = LONG_MIN;
		return;
		}

	clipping_rect* pExtents = &inRegion->fExtent;
	clipping_rect* pBox = inRegion->fRects;
	clipping_rect* pBoxEnd = &pBox[inRegion->fNumRects - 1];

// Since pBox is the first rectangle in the region, it must have the smallest y1 and since pBoxEnd is the last rectangle in the region, it must
// have the largest y2, because of banding. Initialize left and x2 from  pBox and pBoxEnd, resp., as good things to initialize them to...
	pExtents->left = pBox->left;
	pExtents->top = pBox->top;
	pExtents->right = pBoxEnd->right;
	pExtents->bottom = pBoxEnd->bottom;

//	ZAssertStop(2, pExtents->top < pExtents->bottom + 1);
	while (pBox <= pBoxEnd)
		{
		if (pExtents->left > pBox->left)
			pExtents->left = pBox->left;
		if (pExtents->right < pBox->right)
			pExtents->right = pBox->right;
		++pBox;
		}
//	ZAssertStop(2, pExtents->left < pExtents->right + 1);
	}

// =================================================================================================

static void sUnionNonOverlapping(ZBRA* pReg, clipping_rect* r, clipping_rect* rEnd, long y1, long y2)
	{
	clipping_rect* pNextRect = &pReg->fRects[pReg->fNumRects];

	ZAssert(y1 < y2);

	while (r != rEnd)
		{
		ZAssert(r->left < r->right + 1);
		sSpaceCheck(pReg, pNextRect);
		pNextRect->left = r->left;
		pNextRect->top = y1;
//		pNextRect->right + 1 = r->right + 1;
		pNextRect->right = -1 + r->right + 1;
//		pNextRect->bottom + 1 = y2;
		pNextRect->bottom = -1 + y2;
		++pReg->fNumRects;
		++pNextRect;
		++r;

		ZAssert(pReg->fNumRects<= pReg->fNumRectsAllocated);
		}
	}

static void sMergeRect(ZBRA* pReg, clipping_rect*& pNextRect, clipping_rect*& inRect, long y1, long y2)
	{
	if ((pReg->fNumRects != 0) && (pNextRect[-1].top == y1) && (pNextRect[-1].bottom + 1 == y2) && (pNextRect[-1].right + 1 >= inRect->left))
		{
		if (pNextRect[-1].right + 1 < inRect->right + 1)
			{
//			pNextRect[-1].right + 1 = inRect->right + 1;
			pNextRect[-1].right = -1 + inRect->right + 1;
			ZAssert(pNextRect[-1].left< pNextRect[-1].right + 1);
			}
		}
	else
		{
		sSpaceCheck(pReg, pNextRect);
		pNextRect->top = y1;
//		pNextRect->bottom + 1 = y2;*/
		pNextRect->bottom = -1 + y2;
		pNextRect->left = inRect->left;
//		pNextRect->right + 1 = inRect->right + 1;
		pNextRect->right = -1 + inRect->right + 1;
		++pReg->fNumRects;
		pNextRect += 1;
		}
	ZAssert(pReg->fNumRects<=pReg->fNumRectsAllocated);
	++inRect;
	}

static void sUnionOverlapping(ZBRA* pReg, clipping_rect* r1, clipping_rect* r1End, clipping_rect* r2, clipping_rect* r2End, long y1, long y2)
	{
	clipping_rect* pNextRect = &pReg->fRects[pReg->fNumRects];

	ZAssert(y1< y2);
	while ((r1 != r1End) && (r2 != r2End))
		{
		if (r1->left < r2->left)
			{
			sMergeRect(pReg, pNextRect, r1, y1, y2);
			}
		else
			{
			sMergeRect(pReg, pNextRect, r2, y1, y2);
			}
		}

	if (r1 != r1End)
		{
		do
			{
			sMergeRect(pReg, pNextRect, r1, y1, y2);
			} while (r1 != r1End);
		}
	else while (r2 != r2End)
		{
		sMergeRect(pReg, pNextRect, r2, y1, y2);
		}
	}

static void sUnion(const ZBRA* source1, const ZBRA* source2, ZBRA* destination)
	{
// Check for all the simple cases

// source1 and source2 are the same, or source1 is empty
	if ((source1 == source2) || (source1->fNumRects == 0))
		{
		if (destination != source2)
			sCopy(source2, destination);
		return;
		}
// source 2 is empty
	if (source2->fNumRects == 0)
		{
		if (destination != source1)
			sCopy(source1, destination);
		return;
		}
// source1 is rectangular and completely subsumes source2
	if ((source1->fNumRects == 1) &&
		(source1->fExtent.left <= source2->fExtent.left) &&
		(source1->fExtent.top <= source2->fExtent.top) &&
		(source1->fExtent.right >= source2->fExtent.right) &&
		(source1->fExtent.bottom >= source2->fExtent.bottom))
		{
		if (destination != source1)
			sCopy(source1, destination);
		return;
		}
// source2 is rectangular and completely subsumes source1
	if ((source2->fNumRects == 1) &&
		(source2->fExtent.left <= source1->fExtent.left) &&
		(source2->fExtent.top <= source1->fExtent.top) &&
		(source2->fExtent.right >= source1->fExtent.right) &&
		(source2->fExtent.bottom >= source1->fExtent.bottom))
		{
		if (destination != source2)
			sCopy(source2, destination);
		return;
		}
// Gotta do some real work
	sRegionOp(destination, source1, source2, sUnionOverlapping, sUnionNonOverlapping, sUnionNonOverlapping);
	destination->fExtent.left = min(source1->fExtent.left, source2->fExtent.left);
	destination->fExtent.top = min(source1->fExtent.top, source2->fExtent.top);
	destination->fExtent.right = max(source1->fExtent.right, source2->fExtent.right);
	destination->fExtent.bottom = max(source1->fExtent.bottom, source2->fExtent.bottom);
	}


// =================================================================================================

static void sIntersectionOverlapping(ZBRA* pReg, clipping_rect* r1, clipping_rect* r1End, clipping_rect* r2, clipping_rect* r2End, long y1, long y2)
	{
	clipping_rect* pNextRect = &pReg->fRects[pReg->fNumRects];

	while ((r1 != r1End) && (r2 != r2End))
		{
		long x1 = max(r1->left, r2->left);
		long x2 = min(r1->right + 1, r2->right + 1);

// If there's any overlap between the two rectangles, add that overlap to the new region. There's no need to check for
// subsumption because the only way such a need could arise is if some region has two rectangles
// right next to each other. Since that should never happen...
		if (x1 < x2)
			{
			ZAssert(y1< y2);

			sSpaceCheck(pReg, pNextRect);
			pNextRect->left = x1;
			pNextRect->top = y1;
//			pNextRect->right + 1 = x2;
			pNextRect->right = -1 + x2;
//			pNextRect->bottom + 1 = y2;
			pNextRect->bottom = -1 + y2;
			++pReg->fNumRects;
			++pNextRect;
			ZAssert(pReg->fNumRects <= pReg->fNumRectsAllocated);
			}

// Need to advance the pointers. Shift the one that extends to the right the least, since the other still has a chance to
// overlap with that region's next rectangle, if you see what I mean.
		if (r1->right + 1 < r2->right + 1)
			{
			++r1;
			}
		else if (r2->right + 1 < r1->right + 1)
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

static void sIntersection(const ZBRA* source1, const ZBRA* source2, ZBRA* destination)
	{
// See if this can be handled trivially
	if ((source1->fNumRects == 0) || (source2->fNumRects == 0) ||
		((source1->fExtent.right + 1 <= source2->fExtent.left) ||
		(source1->fExtent.left >= source2->fExtent.right + 1) ||
		(source1->fExtent.bottom + 1 <= source2->fExtent.top) && (source1->fExtent.top >= source2->fExtent.bottom + 1)))
		destination->fNumRects = 0;
	else
		sRegionOp(destination, source1, source2, sIntersectionOverlapping, nil, nil);
// Can't alter destination's extents before we call sRegionOp because it might be one of the source regions and sRegionOp depends
// on the extents of those regions being the same. Besides, this way there's no checking against rectangles that will be nuked
// due to coalescing, so we have to examine fewer rectangles.
	sSetExtents(destination);
	}

// =================================================================================================

static void sDifferenceNonOverlapping(ZBRA* pReg, clipping_rect* r, clipping_rect* rEnd, long y1, long y2)
	{
	clipping_rect* pNextRect = &pReg->fRects[pReg->fNumRects];

	ZAssert(y1<y2);

	while (r != rEnd)
		{
		ZAssert(r->left < r->right + 1);
		sSpaceCheck(pReg, pNextRect);
		pNextRect->left = r->left;
		pNextRect->top = y1;
//		pNextRect->right + 1 = r->right + 1;
		pNextRect->right = -1 + r->right + 1;
//		pNextRect->bottom + 1 = y2;
		pNextRect->bottom = -1 + y2;
		++pReg->fNumRects;
		++pNextRect;

		ZAssert(pReg->fNumRects <= pReg->fNumRectsAllocated);

		++r;
		}
	}

static void sDifferenceOverlapping(ZBRA* pReg, clipping_rect* r1, clipping_rect* r1End, clipping_rect* r2, clipping_rect* r2End, long y1, long y2)
	{
	long x1 = r1->left;

	ZAssert(y1<y2);
	clipping_rect* pNextRect = &pReg->fRects[pReg->fNumRects];

	while ((r1 != r1End) && (r2 != r2End))
		{
		if (r2->right + 1 <= x1)
			{
// Subtrahend missed the boat: go to next subtrahend.
			++r2;
			}
		else if (r2->left <= x1)
			{
// Subtrahend preceeds minuend: nuke left edge of minuend.
			x1 = r2->right + 1;
			if (x1 >= r1->right + 1)
				{
// Minuend completely covered: advance to next minuend and
// reset left fence to edge of new minuend.
				++r1;
				if (r1 != r1End)
					x1 = r1->left;
				}
			else
				{
// Subtrahend now used up since it doesn't extend beyond
// minuend
				++r2;
				}
			}
		else if (r2->left < r1->right + 1)
			{
// Left part of subtrahend covers part of minuend: add uncovered
// part of minuend to region and skip to next subtrahend.
			ZAssert(x1<r2->left);
			sSpaceCheck(pReg, pNextRect);
			pNextRect->left = x1;
			pNextRect->top = y1;
//			pNextRect->right + 1 = r2->left;
			pNextRect->right = -1 + r2->left;
//			pNextRect->bottom + 1 = y2;
			pNextRect->bottom = -1 + y2;
			++pReg->fNumRects;
			++pNextRect;

			ZAssert(pReg->fNumRects<= pReg->fNumRectsAllocated);

			x1 = r2->right + 1;
			if (x1 >= r1->right + 1)
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
			if (r1->right + 1 > x1)
				{
				sSpaceCheck(pReg, pNextRect);
				pNextRect->left = x1;
				pNextRect->top = y1;
//				pNextRect->right + 1 = r1->right + 1;
				pNextRect->right = -1 + r1->right + 1;
//				pNextRect->bottom + 1 = y2;
				pNextRect->bottom = -1 + y2;
				++pReg->fNumRects;
				++pNextRect;
				ZAssert(pReg->fNumRects<=pReg->fNumRectsAllocated);
				}
			++r1;
			x1 = r1->left;
			}
		}

// Add remaining minuend rectangles to region.
	while (r1 != r1End)
		{
		ZAssert(x1<r1->right + 1);
		sSpaceCheck(pReg, pNextRect);
		pNextRect->left = x1;
		pNextRect->top = y1;
//		pNextRect->right + 1 = r1->right + 1;
		pNextRect->right = -1 + r1->right + 1;
//		pNextRect->bottom + 1 = y2;
		pNextRect->bottom = -1 + y2;
		++pReg->fNumRects;
		++pNextRect;

		ZAssert(pReg->fNumRects<= pReg->fNumRectsAllocated);

		++r1;
		if (r1 != r1End)
			{
			x1 = r1->left;
			}
		}
	}

static void sDifference(const ZBRA* source1, const ZBRA* source2, ZBRA* destination)
	{
// See if this can be handled trivially
	if ((source1->fNumRects == 0) || (source2->fNumRects == 0) ||
		((source1->fExtent.right + 1 <= source2->fExtent.left) ||
		(source1->fExtent.left >= source2->fExtent.right + 1) ||
		(source1->fExtent.bottom + 1 <= source2->fExtent.top) && (source1->fExtent.top >= source2->fExtent.bottom + 1)))
		{
		sCopy(source1, destination);
		return;
		}
	sRegionOp(destination, source1, source2, sDifferenceOverlapping, sDifferenceNonOverlapping, nil);
// Can't alter destination's extents before we call sRegionOp because it might be one of the source regions and sRegionOp depends
// on the extents of those regions being the same. Besides, this way there's no checking against rectangles that will be nuked
// due to coalescing, so we have to examine fewer rectangles.
	sSetExtents(destination);
	}

// =================================================================================================

static long sCoalesce(ZBRA* pRegion, long prevStart, long curStart)
	{
	clipping_rect* pRegEnd = &pRegion->fRects[pRegion->fNumRects];
	clipping_rect* pPrevBox = &pRegion->fRects[prevStart];
	long prevNumRects = curStart - prevStart;

// Figure out how many rectangles are in the current band. Have to do
// this because multiple bands could have been added in sRegionOp
// at the end when one region has been exhausted.
	clipping_rect* pCurBox = &pRegion->fRects[curStart];
	long bandY1 = pCurBox->top;
	long curNumRects;
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
		curStart = pRegEnd - pRegion->fRects;
		pRegEnd = pRegion->fRects + pRegion->fNumRects;
		}

	if ((curNumRects == prevNumRects) && (curNumRects != 0))
		{
		pCurBox -= curNumRects;

// The bands may only be coalesced if the bottom of the previous
// matches the top scanline of the current.
		if (pPrevBox->bottom + 1 == pCurBox->top)
			{
// Make sure the bands have boxes in the same places. This assumes that boxes have been added in such a way that they
// cover the most area possible. I.e. two boxes in a band must have some horizontal space between them.
			do
				{
				if ((pPrevBox->left != pCurBox->left) || (pPrevBox->right + 1 != pCurBox->right + 1))
					{
// The bands don't line up so they can't be coalesced.
					return curStart;
					}
				++pPrevBox;
				++pCurBox;
				prevNumRects -= 1;
				} while (prevNumRects != 0);

			pRegion->fNumRects -= curNumRects;
			pCurBox -= curNumRects;
			pPrevBox -= curNumRects;

// The bands may be merged, so set the bottom y of each box in the previous band to that of
// the corresponding box in the current band.
			do
				{
//				pPrevBox->bottom + 1 = pCurBox->bottom + 1;
				pPrevBox->bottom = -1 + pCurBox->bottom + 1;
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


static void sTranscribeAndNormalize(clipping_rect* inSourceStart, clipping_rect* inSourceEnd, clipping_rect*& outRects, clipping_rect*& outRectsEnd, clipping_rect*& outRectsToFree)
	{
// Walk through the source, band by band, looking for problems
	clipping_rect* iter = inSourceStart;
	bool gotProblem = false;
	while (iter != inSourceEnd && !gotProblem)
		{
		long currentTop = iter->top;

		while ((iter != inSourceEnd) && (iter->top == currentTop))
			{
			if (iter != inSourceStart)
				{
				if (iter->left < iter[-1].left || iter->bottom != iter[-1].bottom)
					{
					gotProblem = true;
					break;
					}
				}
			++iter;
			}
		}

	if (!gotProblem)
		{
		outRects = inSourceStart;
		outRectsEnd = inSourceEnd;
		outRectsToFree = nil;
		return;
		}

// We have two jobs to do here. We have to proceed through the source rects, band by band. Within a band we have to
// write out a set of rectangles that are sorted horizontally. And the bands themselves may have to be split into more than
// one real band.
	vector<clipping_rect> newData;

	iter = inSourceStart;
	while (iter != inSourceEnd)
		{
		vector<clipping_rect> sortedBand;
		long currentTop = iter->top;

		long smallestBottom = LONG_MAX;
		while ((iter != inSourceEnd) && (iter->top == currentTop))
			{
			int x;
			for (x = 0; x < sortedBand.size(); ++x)
				{
				if (sortedBand[x].left >= iter->left)
					break;
				}
			if (smallestBottom  > iter->bottom)
				smallestBottom = iter->bottom;
			sortedBand.insert(sortedBand.begin() + x, *iter);
			++iter;
			}

// We have a horizontally sorted band. Now to pass over it repeatedly, writing out new bands such that
// all rectangles in a band have the same bottom field.
		while (true)
			{
			bool gotAnyLarger = false;
			long nextSmallestBottom = LONG_MAX;
			for (int x = 0; x < sortedBand.size(); ++x)
				{
				clipping_rect realRect = sortedBand[x];
				if (realRect.bottom >= smallestBottom)
					{
					if (realRect.bottom > smallestBottom)
						{
						if (nextSmallestBottom > realRect.bottom)
							nextSmallestBottom = realRect.bottom;
						realRect.bottom = smallestBottom;
						gotAnyLarger = true;
						}
					realRect.top = currentTop;
					newData.push_back(realRect);
					}
				}
			if (!gotAnyLarger)
				break;
			currentTop = smallestBottom + 1;
			smallestBottom = nextSmallestBottom;
			}
		}
	ZAssert(newData.size() > 0);
	outRects = sAllocateRects(newData.size());
	outRectsEnd = outRects + newData.size();
	outRectsToFree = outRects;
	::memcpy(outRects, newData.begin(), sizeof(clipping_rect) * newData.size());
	}

static void sRegionOp(ZBRA* destRegion, const ZBRA* reg1, const ZBRA* reg2,
	OverlappingFunctionPtr overlapFunc, NonOverlappingFunctionPtr nonOverlapFunc1, NonOverlappingFunctionPtr nonOverlapFunc2)
	{
	ZAssert(reg1->fNumRects != 0 && reg2->fNumRects != 0);

	clipping_rect* r1;
	clipping_rect* r1End;
	clipping_rect* r1ToFree;
	sTranscribeAndNormalize(reg1->fRects, reg1->fRects + reg1->fNumRects, r1, r1End, r1ToFree);


	clipping_rect* r2;
	clipping_rect* r2End;
	clipping_rect* r2ToFree;
	sTranscribeAndNormalize(reg2->fRects, reg2->fRects + reg2->fNumRects, r2, r2End, r2ToFree);


// Initialization:
// set r1, r2, r1End and r2End appropriately, preserve the important parts of the destination region until the end in case it's one of
// the two source regions, then mark the "new" region empty, allocating another array of rectangles for it to use.

	clipping_rect* oldRects = destRegion->fRects;

	destRegion->fNumRects = 0;

// Allocate a reasonable number of rectangles for the new region. The idea is to allocate enough so the individual functions don't need to
// reallocate and copy the array, which is time consuming, yet we don't have to worry about using too much memory. I hope to be able to
// nuke the Xrealloc() at the end of this function eventually.

	destRegion->fNumRectsAllocated = max(reg1->fNumRects, reg2->fNumRects) * 2;
	destRegion->fRects = sAllocateRects(destRegion->fNumRectsAllocated);

// Initialize ybot and ytop.
// In the upcoming loop, ybot and ytop serve different functions depending on whether the band being handled is an overlapping or non-overlapping
// band. In the case of a non-overlapping band (only one of the regions has points in the band), ybot is the bottom of the most recent
// intersection and thus clips the top of the rectangles in that band. ytop is the top of the next intersection between the two regions and
// serves to clip the bottom of the rectangles in the current band. For an overlapping band (where the two regions intersect), ytop clips
// the top of the rectangles of both regions and ybot clips the bottoms.
	long ybot;
	if (reg1->fExtent.top < reg2->fExtent.top)
		ybot = reg1->fExtent.top;
	else
		ybot = reg2->fExtent.top;

// prevBand serves to mark the start of the previous band so rectangles can be coalesced into larger rectangles. qv. miCoalesce, above.
// In the beginning, there is no previous band, so prevBand == curBand (curBand is set later on, of course, but the first band will always
// start at index 0). prevBand and curBand must be indices because of the possible expansion, and resultant moving, of the new region's
// array of rectangles.

	int destPrevBand = 0;
	while (true)
		{
		int destCurBand = destRegion->fNumRects;


// This algorithm proceeds one source-band (as opposed to a destination band, which is determined by where the two regions
// intersect) at a time. r1BandEnd and r2BandEnd serve to mark the rectangle after the last one in the current band for their
// respective regions.

		clipping_rect* r1BandEnd = r1;
		while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top))
			++r1BandEnd;

		clipping_rect* r2BandEnd = r2;
		while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top))
			++r2BandEnd;


// First handle the band that doesn't intersect, if any. Note that attention is restricted to one band in the
// non-intersecting region at once, so if a region has n bands between the current position and the next place it overlaps
// the other, this entire loop will be passed through n times.

		long ytop;
		long top;
		long bot;
		if (r1->top < r2->top)
			{
			top = max(r1->top, ybot);
			bot = min(r1->bottom + 1, r2->top);

			if ((top != bot) && (nonOverlapFunc1 != nil))
				nonOverlapFunc1(destRegion, r1, r1BandEnd, top, bot);

			ytop = r2->top;
			}
		else if (r2->top < r1->top)
			{
			top = max(r2->top,ybot);
			bot = min(r2->bottom + 1, r1->top);

			if ((top != bot) && (nonOverlapFunc2 != nil))
				nonOverlapFunc2(destRegion, r2, r2BandEnd, top, bot);

			ytop = r1->top;
			}
		else
			{
			ytop = r1->top;
			}


// If any rectangles got added to the region, try and coalesce them with rectangles from the previous band. Note we could just do
// this test in miCoalesce, but some machines incur a not inconsiderable cost for function calls, so...
		if (destRegion->fNumRects != destCurBand)
			destPrevBand = sCoalesce(destRegion, destPrevBand, destCurBand);


// Now see if we've hit an intersecting band. The two bands only intersect if ybot > ytop
		ybot = min(r1->bottom + 1, r2->bottom + 1);
		destCurBand = destRegion->fNumRects;
		if (ybot > ytop)
			overlapFunc(destRegion, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot);

		if (destRegion->fNumRects != destCurBand)
			destPrevBand = sCoalesce(destRegion, destPrevBand, destCurBand);

// If we've finished with a band (y2 == ybot) we skip forward in the region to the next band.
		if (r1->bottom + 1 == ybot)
			r1 = r1BandEnd;
		if (r2->bottom + 1 == ybot)
			r2 = r2BandEnd;

		if ((r1 == r1End) || (r2 == r2End))
			break;
		}

// Deal with whichever region still has rectangles left.
	long destCurBand = destRegion->fNumRects;
	if (r1 != r1End)
		{
		if (nonOverlapFunc1 != nil)
			{
			do
				{
				clipping_rect* r1BandEnd = r1;
				while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top))
					++r1BandEnd;

				nonOverlapFunc1(destRegion, r1, r1BandEnd, max(r1->top,ybot), r1->bottom + 1);
				r1 = r1BandEnd;
				} while (r1 != r1End);
			}
		}
	else if ((r2 != r2End) && (nonOverlapFunc2 != nil))
		{
		do
			{
			clipping_rect* r2BandEnd = r2;
			while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top))
				 ++r2BandEnd;

			nonOverlapFunc2(destRegion, r2, r2BandEnd, max(r2->top,ybot), r2->bottom + 1);
			r2 = r2BandEnd;
			} while (r2 != r2End);
		}

	if (destRegion->fNumRects != destCurBand)
		sCoalesce(destRegion, destPrevBand, destCurBand);


// A bit of cleanup. To keep regions from growing without bound, we shrink the array of rectangles to match the new number of
// rectangles in the region. This never goes to 0, however... Only do this stuff if the number of rectangles allocated is more
// than twice the number of rectangles in the region (a simple optimization...).
	if (destRegion->fNumRects < (destRegion->fNumRectsAllocated >> 1))
		{
		destRegion->fNumRectsAllocated = destRegion->fNumRects;
		if (destRegion->fNumRects == 0)
			destRegion->fNumRectsAllocated = 1;
		clipping_rect* newRects = sAllocateRects(destRegion->fNumRectsAllocated);
		::memcpy(newRects, destRegion->fRects, sizeof(clipping_rect)*destRegion->fNumRects);
		::free(destRegion->fRects);
		destRegion->fRects = newRects;
		}
	::free(oldRects);

	if (r1ToFree)
		::free(r1ToFree);
	if (r2ToFree)
		::free(r2ToFree);
	}

// =================================================================================================

#endif // ZCONFIG(API_Graphics, Be)
