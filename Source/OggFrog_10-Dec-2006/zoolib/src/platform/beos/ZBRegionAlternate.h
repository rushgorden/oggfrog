/*  @(#) $Id: ZBRegionAlternate.h,v 1.1.1.1 2002/02/17 18:41:32 agreen Exp $ */

// AG 2000-02-27. See ZBRegionAlternate.cpp for a discussion of what, why and how.

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

#ifndef __ZBRegionAlternate__
#define __ZBRegionAlternate__ 1
#include "zconfig.h"

#if ZCONFIG(API_Graphics, Be)

#include <interface/Region.h>

void ZBRegionAlternate_Assign(BRegion* inBRegionLHS, BRegion* inBRegionRHS);

void ZBRegionAlternate_Set(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom);
void ZBRegionAlternate_Set(BRegion* inBRegion, clipping_rect inNewBounds);

void ZBRegionAlternate_MakeEmpty(BRegion* inBRegion);
bool ZBRegionAlternate_IsEmpty(const BRegion* inBRegion);

bool ZBRegionAlternate_Contains(const BRegion* inBRegion, int32 inX, int32 inY);

void ZBRegionAlternate_OffsetBy(BRegion* inBRegion, int32 dh, int32 dv);

void ZBRegionAlternate_Include(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom);
void ZBRegionAlternate_Include(BRegion* inBRegion, clipping_rect inRect);
void ZBRegionAlternate_Include(BRegion* inBRegion, const BRegion* inOther);

void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom);
void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, clipping_rect inRect);
void ZBRegionAlternate_IntersectWith(BRegion* inBRegion, const BRegion* inOther);

void ZBRegionAlternate_Exclude(BRegion* inBRegion, int32 inLeft, int32 inTop, int32 inRight, int32 inBottom);
void ZBRegionAlternate_Exclude(BRegion* inBRegion, clipping_rect inRect);
void ZBRegionAlternate_Exclude(BRegion* inBRegion, const BRegion* inOther);


//clipping_rect ZBRegionAlternate_Frame() { return fExtent; }
//clipping_rect ZBRegionAlternate_RectAt(int32 inIndex) { return fRects[inIndex]; }
//int32 ZBRegionAlternate_CountRects() { return fNumRects; }


#endif // ZCONFIG(API_Graphics, Be)

#endif // __ZBRegionAlternate__
