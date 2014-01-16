/*  @(#) $Id: ZUICartesianPlane.h,v 1.1.1.1 2002/02/17 18:40:01 agreen Exp $ */

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

#ifndef __ZUICartesianPlane__
#define __ZUICartesianPlane__ 1
#include "zconfig.h"

#include "ZMouseTracker.h"

#include <set>

class ZUICartesianPlane
	{
public:
	ZUICartesianPlane();
	virtual ~ZUICartesianPlane();

	bool Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// Layout style methods
	virtual long IndexFromPoint(ZPoint inPoint) = 0;
	virtual void IndicesFromRegion(ZDCRgn inRgn, set<long>& outIndices) = 0;
	virtual ZPoint PointFromIndex(long inIndex) = 0;
	virtual ZDCRgn RegionFromIndex(long inIndex) = 0;
	virtual ZDCRgn RegionFromIndices(const set<long>& inIndices);

	virtual void Invalidate(const ZDCRgn& inRgn) = 0;
	virtual void InvalidateIndex(long inIndex);

	virtual long BringIndexToFront(long inIndex);

// Selection
	virtual bool GetIndexSelected(long inIndex);
	virtual void SetIndexSelected(long inIndex, bool selectIt, bool inNotify);

	virtual void GetSelectedIndices(set<long>& outSelection) = 0;
	virtual void SetSelectedIndices(const set<long>& inSelection, bool inNotify) = 0;


// Higher level clicks
	virtual bool HandleDoubleClick(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool HandleBackgroundClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool HandleDrag(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// Coordinate transformation
	virtual ZPoint FromPane(ZPoint inPanePoint);

	class Tracker;
	class TrackerBackground;
protected:
	};

// ==================================================

class ZRegionAdorner;
class ZUISelection;
class ZUICartesianPlane::TrackerBackground : public ZMouseTracker
	{
public:
	TrackerBackground(ZSubPane* inSubPane, ZUICartesianPlane* inPlane, ZPoint inHitPoint, ZUISelection* inUISelection);
	virtual ~TrackerBackground();

	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	ZUICartesianPlane* fCartesianPlane;
	ZRef<ZRegionAdorner> fRegionAdorner;
	set<long> fOriginalSelection;
	ZUISelection* fUISelection;
	};

// ==================================================

#endif // __ZUICartesianPlane__
