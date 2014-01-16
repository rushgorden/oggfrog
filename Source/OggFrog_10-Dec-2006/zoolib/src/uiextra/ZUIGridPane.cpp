static const char rcsid[] = "@(#) $Id: ZUIGridPane.cpp,v 1.7 2006/10/13 20:37:52 agreen Exp $";

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

#include "ZUIGridPane.h"


// Ideally we'd use the following, but it's unevenly available
// #include <limits.h> // for numeric_limits
// static const ZCoord_Max = numeric_limits<ZCoord>::max();
// static const ZCoord_Min = numeric_limits<ZCoord>::min();
// So we do the following. Note that these values are only strictly correct on Mac and POSIX,
// but they're mainly used to clamp very large operations which won't be visible on any platform.
#include <climits>
static const ZCoord ZCoord_Max = SHRT_MAX;
static const ZCoord ZCoord_Min = SHRT_MIN;

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane

ZUIGridPane::ZUIGridPane(ZSuperPane* superPane, ZPaneLocator* paneLocator,
		Geometry* inGeometryH, Geometry* inGeometryV,
		CellRenderer* inCellRenderer, Selection* inSelection)
:	ZSuperPane(superPane, paneLocator),
	fGeometryH(inGeometryH),
	fGeometryV(inGeometryV),
	fCellRenderer(inCellRenderer),
	fSelection(inSelection)
	{
	fScrollPos = ZPoint_T<int32>::sZero;
	fCellScrollPos = ZPoint::sZero;
	}

ZUIGridPane::~ZUIGridPane()
	{}

void ZUIGridPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZDC localDC(inDC);
	ZDCRgn clipRgn(localDC.GetClip());
	ZRect drawBounds = clipRgn.Bounds();

	// Figure out what rect won't be touched by the cells
	ZRect allCellsRect(fGeometryH->CellToPixel(true, fScrollPos.h, fGeometryH->GetCellCount(true)),
								fGeometryV->CellToPixel(false, fScrollPos.v, fGeometryV->GetCellCount(false)));

	ZDCRgn leftOverRgn(clipRgn - allCellsRect);
	localDC.Fill(leftOverRgn);

	if (fGeometryH->GetIterationCost(true) > fGeometryV->GetIterationCost(false))
		fGeometryH->DrawRangeMaster(true, localDC, drawBounds.left, drawBounds.right, drawBounds.top, drawBounds.bottom, fScrollPos.h, fScrollPos.v, fCellRenderer, fGeometryV, fSelection);
	else
		fGeometryV->DrawRangeMaster(false, localDC, drawBounds.top, drawBounds.bottom, drawBounds.left, drawBounds.right, fScrollPos.v, fScrollPos.h, fCellRenderer, fGeometryH, fSelection);
	}

bool ZUIGridPane::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fSelection)
		{
		if (fSelection->Click(this, inHitPoint, inEvent))
			return true;
		}
	return ZSuperPane::DoClick(inHitPoint, inEvent);
	}

bool ZUIGridPane::DoKeyDown(const ZEvent_Key& inEvent)
	{
	if (fSelection)
		{
		if (fSelection->KeyDown(this, inEvent))
			return true;
		}
	// Call our ZUIScrollable utility method
	if (this->DoCursorKey(inEvent))
		return true;
	return ZSuperPane::DoKeyDown(inEvent);
	}

ZDCRgn ZUIGridPane::DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds)
	{
	ZDCRgn theRgn;
	if (inOldBounds.TopLeft() != inNewBounds.TopLeft())
		{
		theRgn |= inOldBounds;
		theRgn |= inNewBounds;
		}
	else
		theRgn = ZSuperPane::DoFramePostChange(inOldBounds, inNewBounds);
	return theRgn;
	}

bool ZUIGridPane::WantsToBecomeTarget()
	{ return true; }

void ZUIGridPane::BringPointIntoViewPinned(ZPoint inPosition)
	{
	// As with ZUIScrollPane, when we're tracking in the GridPane itself, rather than any
	// contained pane, we don't do pinning, otherwise we'd never report a point being
	// outside the gridpane's bounds, and auto scrolling won't happen.
	this->BringPointIntoView(inPosition);
	}

void ZUIGridPane::BringPointIntoView(ZPoint inPosition)
	{
	ZPoint mySize = this->GetSize();
	ZRect apertureRect = this->CalcAperture();

	// Figure out how many pixels we're able to scroll
	int32 cellBoundary;
	int32 visibleBoundary;
	int32 wantedOffset;
	int32 achievableOffset;
	ZPoint_T<int32> achievedCellOffset = ZPoint_T<int32>::sZero;
	ZPoint achievedPixelOffset = ZPoint::sZero;
	if (inPosition.h >= apertureRect.right)
		{
		cellBoundary = fGeometryH->PixelToCell(true, fScrollPos.h, inPosition.h, true);
		visibleBoundary = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.right, false);
		wantedOffset = cellBoundary - visibleBoundary;
		achievableOffset = fGeometryH->PixelToCell(true, fGeometryH->GetCellCount(true), -mySize.h, true) - fScrollPos.h;
		achievedCellOffset.h = min(wantedOffset, achievableOffset);
		achievedPixelOffset.h = fGeometryH->CellToPixel(true, fScrollPos.h, fScrollPos.h + achievedCellOffset.h);

		// Don't overscroll -- we're only trying to get inPosition into view
		if (achievedPixelOffset.h > inPosition.h - apertureRect.right)
			{
			achievedPixelOffset.h = 0;
			achievedCellOffset.h = 0;
			}
		}
	else if (inPosition.h < apertureRect.left)
		{
		cellBoundary = fGeometryH->PixelToCell(true, fScrollPos.h, inPosition.h, false);
		visibleBoundary = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.left, true);
		wantedOffset = cellBoundary - visibleBoundary;
		achievableOffset = 0 - fScrollPos.h;
		achievedCellOffset.h = max(wantedOffset, achievableOffset);
		achievedPixelOffset.h = fGeometryH->CellToPixel(true, fScrollPos.h, fScrollPos.h + achievedCellOffset.h);
		}

	if (inPosition.v >= apertureRect.bottom)
		{
		cellBoundary = fGeometryV->PixelToCell(false, fScrollPos.v, inPosition.v, true);
		visibleBoundary = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.bottom, false);
		wantedOffset = cellBoundary - visibleBoundary;
		achievableOffset = fGeometryV->PixelToCell(false, fGeometryV->GetCellCount(false), -mySize.v, true) - fScrollPos.v;
		achievedCellOffset.v = min(wantedOffset, achievableOffset);
		achievedPixelOffset.v = fGeometryV->CellToPixel(false, fScrollPos.v, fScrollPos.v + achievedCellOffset.v);
		}
	else if (inPosition.v < apertureRect.top)
		{
		cellBoundary = fGeometryV->PixelToCell(false, fScrollPos.v, inPosition.v, false);
		visibleBoundary = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.top, true);
		wantedOffset = cellBoundary - visibleBoundary;
		achievableOffset = 0 - fScrollPos.v;
		achievedCellOffset.v = max(wantedOffset, achievableOffset);
		achievedPixelOffset.v = fGeometryV->CellToPixel(false, fScrollPos.v, fScrollPos.v + achievedCellOffset.v);
		}
	if (achievedCellOffset != ZPoint_T<int32>::sZero)
		{
		this->Internal_ScrollTo(fScrollPos + achievedCellOffset, true, true);
#if 0 // commented out to avoid UpdateNow() hang on Windows. -ec 99.11.19
		this->Update();
#endif
		}
	inPosition -= achievedPixelOffset;

	ZSuperPane::BringPointIntoView(inPosition);
	}

void ZUIGridPane::BringCellIntoView(ZPoint_T<int32> inCell)
	{
	ZPoint mySize = this->GetSize();
	ZRect apertureRect = this->CalcAperture();
	int32 visibleCellLeft = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.left, true);
	int32 visibleCellRight = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.right, false);
	int32 visibleCellTop = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.top, true);
	int32 visibleCellBottom = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.bottom, false);
	ZPoint_T<int32> achievedCellOffset = ZPoint_T<int32>::sZero;

	int32 wantedOffset;
	int32 achievableOffset;
	if (inCell.h > visibleCellRight)
		{
		wantedOffset = inCell.h - visibleCellRight;
		achievableOffset = fGeometryH->PixelToCell(true, fGeometryH->GetCellCount(true), -mySize.h, true) - fScrollPos.h;
		achievedCellOffset.h = min(wantedOffset, achievableOffset);
		}
	else if (inCell.h < visibleCellLeft)
		{
		wantedOffset = inCell.h - visibleCellLeft;
		achievableOffset = 0 - fScrollPos.h;
		achievedCellOffset.h = max(wantedOffset, achievableOffset);
		}

	if (inCell.v > visibleCellBottom)
		{
		wantedOffset = inCell.v - visibleCellBottom;
		achievableOffset = fGeometryV->PixelToCell(false, fGeometryV->GetCellCount(false), -mySize.v, true) - fScrollPos.v;
		achievedCellOffset.v = min(wantedOffset, achievableOffset);
		}
	else if (inCell.v < visibleCellTop)
		{
		wantedOffset = inCell.v - visibleCellTop;
		achievableOffset = 0 - fScrollPos.v;
		achievedCellOffset.v = max(wantedOffset, achievableOffset);
		}
	if (achievedCellOffset != ZPoint_T<int32>::sZero)
		{
		this->Internal_ScrollTo(fScrollPos + achievedCellOffset, true, true);
		}
	// We've scrolled as much as we can using cell scrolling. Now let's see how much further we have
	// to move, and trust that there's a superpane that can move us the additional distance we need.
	ZCoord cellPositionH = fGeometryH->CellToPixel(true, fScrollPos.h, inCell.h);
	ZCoord cellPositionV = fGeometryV->CellToPixel(false, fScrollPos.v, inCell.v);
	ZSuperPane::BringPointIntoView(ZPoint(cellPositionH, cellPositionV));
	}

ZPoint_T<int32> ZUIGridPane::FindCell(ZPoint inPoint, bool inClampIt)
	{
	ZPoint mySize = this->GetSize();
	int32 theCellH = fGeometryH->PixelToCell(true, fScrollPos.h, inPoint.h, false, inClampIt);
	int32 theCellV = fGeometryV->PixelToCell(false, fScrollPos.v, inPoint.v, false, inClampIt);

/*	if (inClampIt)
		{
		if (theCellH < 0)
			theCellH = 0;
		else if (theCellH >= fGeometryH->GetCellCount(true))
			theCellH = fGeometryH->GetCellCount(true) - 1;

		if (theCellV < 0)
			theCellV = 0;
		else if (theCellV >= fGeometryV->GetCellCount(false))
			theCellV = fGeometryV->GetCellCount(false) - 1;
		}*/

	return ZPoint_T<int32>(theCellH, theCellV);
	}

bool ZUIGridPane::GetCellRect(ZPoint_T<int32> inCell, ZRect& outRect)
	{
	if (inCell.h < 0 || inCell.h >= fGeometryH->GetCellCount(true))
		return false;
	if (inCell.v < 0 || inCell.v >= fGeometryV->GetCellCount(false))
		return false;
	outRect.left = fGeometryH->CellToPixel(true, fScrollPos.h, inCell.h);
	outRect.right = outRect.left + fGeometryH->CellToPixel(true, inCell.h, inCell.h + 1);
	outRect.top = fGeometryV->CellToPixel(false, fScrollPos.v, inCell.v);
	outRect.bottom = outRect.top + fGeometryV->CellToPixel(false, inCell.v, inCell.v + 1);
	return true;
	}

bool ZUIGridPane::GetCellsRect(ZRect_T<int32> inCells, ZRect& outRect)
	{
	if (inCells.left < 0)
		inCells.left = 0;
	if (inCells.left > fGeometryH->GetCellCount(true))
		inCells.left = fGeometryH->GetCellCount(true);

	if (inCells.right < 0)
		inCells.right = 0;
	if (inCells.right > fGeometryH->GetCellCount(true))
		inCells.right = fGeometryH->GetCellCount(true);

	if (inCells.top < 0)
		inCells.top = 0;
	if (inCells.top > fGeometryV->GetCellCount(false))
		inCells.top = fGeometryV->GetCellCount(false);

	if (inCells.bottom < 0)
		inCells.bottom = 0;
	if (inCells.bottom > fGeometryV->GetCellCount(false))
		inCells.bottom = fGeometryV->GetCellCount(false);

	if (inCells.IsEmpty())
		return false;

	outRect.left = fGeometryH->CellToPixel(true, fScrollPos.h, inCells.left);
	outRect.right = fGeometryH->CellToPixel(true, fScrollPos.h, inCells.right);
	outRect.top = fGeometryV->CellToPixel(false, fScrollPos.v, inCells.top);
	outRect.bottom = fGeometryV->CellToPixel(false, fScrollPos.v, inCells.bottom);
	return true;
	}

ZDCRgn ZUIGridPane::GetCellPixels(const ZBigRegion& inCells)
	{
	ZDCRgn theRgn;
	vector<ZRect_T<int32> > cellsAsBigRects;
	inCells.Decompose(cellsAsBigRects);
	for (vector<ZRect_T<int32> >::iterator i = cellsAsBigRects.begin(); i != cellsAsBigRects.end(); ++i)
		{
		ZRect theRect;
		if (this->GetCellsRect(*i, theRect))
			theRgn |= theRect;
		}
	return theRgn;
	}

void ZUIGridPane::InvalidateCell(ZPoint_T<int32> inCell)
	{
	ZRect theRect;
	if (this->GetCellRect(inCell, theRect))
		this->Invalidate(theRect);
	}

void ZUIGridPane::ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint_T<int32> subExtent(fGeometryH->GetCellCount(true), fGeometryV->GetCellCount(false));
	ZPoint_T<int32> mySize(subExtent - this->Internal_CalcBottomCell(this->GetSize()));

	ZRect_T<int32> shownExtent;
	shownExtent.left = min(int32(0), fScrollPos.h);
	shownExtent.top = min(int32(0), fScrollPos.v);
	shownExtent.right = max(subExtent.h, mySize.h + fScrollPos.h);
	shownExtent.bottom = max(subExtent.v, mySize.v + fScrollPos.v);

	ZPoint_T<int32> integerDelta;
	integerDelta.h = int32(inDelta.h * (shownExtent.Width() - mySize.h));
	integerDelta.v = int32(inDelta.v * (shownExtent.Height() - mySize.v));

	this->ScrollTo(fScrollPos + integerDelta, inDrawIt, inDoNotifications);
	}

void ZUIGridPane::ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint_T<int32> subExtent(fGeometryH->GetCellCount(true), fGeometryV->GetCellCount(false));
	ZPoint_T<int32> mySize(subExtent - this->Internal_CalcBottomCell(this->GetSize()));

	ZRect_T<int32> shownExtent;
	shownExtent.left = min(int32(0), fScrollPos.h);
	shownExtent.top = min(int32(0), fScrollPos.v);
	shownExtent.right = max(subExtent.h, mySize.h + fScrollPos.h);
	shownExtent.bottom = max(subExtent.v, mySize.v + fScrollPos.v);

	ZPoint_T<int32> newTranslation;
	newTranslation.h = int32(inNewValues.h * (shownExtent.Width() - mySize.h));
	newTranslation.v = int32(inNewValues.v * (shownExtent.Height() - mySize.v));

	this->ScrollTo(newTranslation, inDrawIt, inDoNotifications);
	}

void ZUIGridPane::ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint_T<int32> scrollStep;
	if (!this->GetAttribute("ZUIGridPane::ScrollStep", nil, &scrollStep))
		scrollStep = ZPoint_T<int32>(1, 1);

	ZPoint_T<int32> theDelta(ZPoint_T<int32>::sZero);
	if (inHorizontal)
		theDelta.h = scrollStep.h;
	else
		theDelta.v = scrollStep.v;
	if (!inIncrease)
		theDelta *= -1;

	this->ScrollTo(fScrollPos + theDelta, inDrawIt, inDoNotifications);
	}

void ZUIGridPane::ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint_T<int32> newScrollPos(fScrollPos);
	ZPoint mySize = this->GetSize();
	if (inIncrease)
		{
		if (inHorizontal)
			newScrollPos.h = fGeometryH->PixelToCell(true, fScrollPos.h, mySize.h, false);
		else
			newScrollPos.v = fGeometryV->PixelToCell(false, fScrollPos.v, mySize.v, false);
		}
	else
		{
		if (inHorizontal)
			newScrollPos.h = fGeometryH->PixelToCell(true, fScrollPos.h, -mySize.h, true);
		else
			newScrollPos.v = fGeometryV->PixelToCell(false, fScrollPos.v, -mySize.v, true);
		}
	this->ScrollTo(newScrollPos, inDrawIt, inDoNotifications);
	}

ZPoint_T<double> ZUIGridPane::GetScrollValues()
	{
	ZPoint_T<int32> subExtent(fGeometryH->GetCellCount(true), fGeometryV->GetCellCount(false));
	ZPoint_T<int32> mySize(subExtent - this->Internal_CalcBottomCell(this->GetSize()));

	ZRect_T<int32> shownExtent;
	shownExtent.left = min(int32(0), fScrollPos.h);
	shownExtent.top = min(int32(0), fScrollPos.v);
	shownExtent.right = max(subExtent.h, mySize.h + fScrollPos.h);
	shownExtent.bottom = max(subExtent.v, mySize.v + fScrollPos.v);

	ZPoint_T<double> translation(fScrollPos.h, fScrollPos.v);
	ZPoint_T<double> extent(shownExtent.Width(), shownExtent.Height());
	ZPoint_T<double> size(mySize.h, mySize.v);

	double scrollValueH = translation.h/(extent.h - size.h);
	if (extent.h == size.h)
		scrollValueH = 0.0;
	double scrollValueV = translation.v/(extent.v - size.v);
	if (extent.v == size.v)
		scrollValueV = 0.0;

	return ZPoint_T<double>(scrollValueH, scrollValueV);
	}

ZPoint_T<double> ZUIGridPane::GetScrollThumbProportions()
	{
	ZPoint_T<int32> subExtent(fGeometryH->GetCellCount(true), fGeometryV->GetCellCount(false));
	ZPoint_T<int32> mySize(subExtent - this->Internal_CalcBottomCell(this->GetSize()));

	ZRect_T<int32> shownExtent;
	shownExtent.left = min(int32(0), fScrollPos.h);
	shownExtent.top = min(int32(0), fScrollPos.v);
	shownExtent.right = max(subExtent.h, mySize.h + fScrollPos.h);
	shownExtent.bottom = max(subExtent.v, mySize.v + fScrollPos.v);

	ZPoint_T<double> extent(shownExtent.Width(), shownExtent.Height());
	ZPoint_T<double> size(mySize.h, mySize.v);
	ZPoint_T<double> result = size/extent;
	if (extent.h == 0.0)
		result.h = 1.0;
	if (extent.v == 0.0)
		result.v = 1.0;
	return result;
	}

void ZUIGridPane::ScrollBy(ZPoint_T<int32> inDelta, bool inDrawIt, bool inDoNotifications)
	{ this->ScrollTo(fScrollPos + inDelta, inDrawIt, inDoNotifications); }

void ZUIGridPane::ScrollTo(ZPoint_T<int32> inNewScrollPos, bool inDrawIt, bool inDoNotifications)
	{
	ZPoint_T<int32> maxOffset(this->Internal_CalcBottomCell(this->GetSize()));
	maxOffset.h = max(maxOffset.h, fScrollPos.h);
	inNewScrollPos.h = min(maxOffset.h, inNewScrollPos.h);
	inNewScrollPos.h = max(inNewScrollPos.h, min(int32(0), fScrollPos.h));

	maxOffset.v = max(maxOffset.v, fScrollPos.v);
	inNewScrollPos.v = min(maxOffset.v, inNewScrollPos.v);
	inNewScrollPos.v = max(inNewScrollPos.v, min(int32(0), fScrollPos.v));

	this->Internal_ScrollTo(inNewScrollPos, inDrawIt, inDoNotifications);
	}

void ZUIGridPane::Internal_ScrollTo(ZPoint_T<int32> inNewScrollPos, bool inDrawIt, bool inDoNotifications)
	{
	if (fScrollPos != inNewScrollPos)
		{
		if (inDrawIt)
			{
			ZCoord hOffset = fGeometryH->CellToPixel(true, inNewScrollPos.h, fScrollPos.h);
			ZCoord vOffset = fGeometryV->CellToPixel(false, inNewScrollPos.v, fScrollPos.v);
		
			this->GetDC().Scroll(this->CalcVisibleBoundsRgn().Bounds(), hOffset, vOffset);
			}
		fScrollPos = inNewScrollPos;
		if (inDoNotifications)
			this->Internal_NotifyResponders();
		}
	}

ZPoint_T<int32> ZUIGridPane::Internal_CalcBottomCell(ZPoint visiblePixels)
	{
	int32 cellCountH = fGeometryH->GetCellCount(true);
	int32 currentColumn = fGeometryH->PixelToCell(true, cellCountH, -visiblePixels.h, true);
	int32 cellCountV = fGeometryV->GetCellCount(false);
	int32 currentRow = fGeometryV->PixelToCell(false, cellCountV, -visiblePixels.v, true);
	return ZPoint_T<int32>(currentColumn, currentRow);
	}

ZPoint_T<int32> ZUIGridPane::GetMaxCell()
	{ return ZPoint_T<int32>(fGeometryH->GetCellCount(true), fGeometryV->GetCellCount(false)); }

bool ZUIGridPane::CellInRange(ZPoint_T<int32> inCell)
	{ return inCell.h >= 0 && inCell.v >= 0 && inCell.h < fGeometryH->GetCellCount(true) && inCell.v < fGeometryV->GetCellCount(false); }

ZRect_T<int32> ZUIGridPane::GetVisibleCells(bool inOnlyCompletelyVisible)
	{
	ZRect apertureRect = this->CalcAperture();
	ZRect_T<int32> visibleCells;
	if (inOnlyCompletelyVisible)
		{
		visibleCells.left = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.left, true);
		visibleCells.right = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.right, false);
		visibleCells.top = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.top, true);
		visibleCells.bottom = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.bottom, false);
		}
	else
		{
		visibleCells.left = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.left, false);
		visibleCells.right = fGeometryH->PixelToCell(true, fScrollPos.h, apertureRect.right, true);
		visibleCells.top = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.top, false);
		visibleCells.bottom = fGeometryV->PixelToCell(false, fScrollPos.v, apertureRect.bottom, true);
		}
	return visibleCells;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::CellRenderer

ZUIGridPane::CellRenderer::CellRenderer()
	{}

ZUIGridPane::CellRenderer::~CellRenderer()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection

ZUIGridPane::Selection::Selection()
	{}

ZUIGridPane::Selection::~Selection()
	{}

bool ZUIGridPane::Selection::Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{ return false; }

bool ZUIGridPane::Selection::KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent)
	{ return false; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Single::Tracker

ZUIGridPane::Selection_Single::Tracker::Tracker(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Single* inSelection, bool inRestoreInitialSelectionIfOutOfBounds)
:	ZMouseTracker(inGridPane, inHitPoint), fGridPane(inGridPane), fSelection(inSelection), fRestoreInitialSelectionIfOutOfBounds(inRestoreInitialSelectionIfOutOfBounds)
	{
	fInitialSelectedCell = fSelection->GetSelectedCell();
	}

ZUIGridPane::Selection_Single::Tracker::~Tracker()
	{
	}

void ZUIGridPane::Selection_Single::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZPoint_T<int32> maxCell = fGridPane->GetMaxCell();
	ZPoint_T<int32> hitCell = fGridPane->FindCell(inNext, false);
	if (hitCell.h < 0 || hitCell.v < 0 || hitCell.h >= maxCell.h || hitCell.v >= maxCell.v)
		{
		if (fRestoreInitialSelectionIfOutOfBounds)
			hitCell = fInitialSelectedCell;
		else
			hitCell = ZPoint_T<int32>(-1, - 1);
		}

	ZPoint_T<int32> lastHitCell = fSelection->GetSelectedCell();
	if (lastHitCell != hitCell)
		{
		fSelection->SetSelectedCell(fGridPane, hitCell, false);
		fSelection->Internal_NotifyResponders(ZUISelection::SelectionFeedbackChanged);
		}

	if (inPhase == ePhaseEnd && hitCell != fInitialSelectedCell)
		fSelection->Internal_NotifyResponders(ZUISelection::SelectionChanged);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Single

ZUIGridPane::Selection_Single::Selection_Single()
	{}

ZUIGridPane::Selection_Single::~Selection_Single()
	{}

// From ZUIGridPane::Selection
bool ZUIGridPane::Selection_Single::GetSelected(ZPoint_T<int32> inCell)
	{ return this->GetSelectedCell() == inCell; }

bool ZUIGridPane::Selection_Single::Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZMouseTracker* theTracker = new ZUIGridPane::Selection_Single::Tracker(inGridPane, inHitPoint, this, false);
	theTracker->Install();
	return true;
	}

bool ZUIGridPane::Selection_Single::KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent)
	{
	ZPoint_T<int32> oldSelectedCell = this->GetSelectedCell();
	ZPoint_T<int32> newSelectedCell = oldSelectedCell;
	ZPoint_T<int32> maxCell = inGridPane->GetMaxCell();
	ZRect_T<int32> visibleCells = inGridPane->GetVisibleCells(true);
	bool handledIt = true;
	if (oldSelectedCell.h >= 0 && oldSelectedCell.v >= 0)
		{
		switch (inEvent.GetCharCode())
			{
			case ZKeyboard::ccHome:
				newSelectedCell = ZPoint_T<int32>(0,0);
				break;
			case ZKeyboard::ccEnd:
				newSelectedCell = maxCell;
				break;
			case ZKeyboard::ccLeft:
				newSelectedCell.h -= 1;
				break;
			case ZKeyboard::ccUp:
				newSelectedCell.v -= 1;
				break;
			case ZKeyboard::ccRight:
				newSelectedCell.h += 1;
				break;
			case ZKeyboard::ccDown:
				newSelectedCell.v += 1;
				break;
			default:
				handledIt = false;
				break;
			}		
		}
	else
		{
		switch (inEvent.GetCharCode())
			{
			case ZKeyboard::ccHome:
				newSelectedCell = ZPoint_T<int32>(0,0);
				break;
			case ZKeyboard::ccEnd:
				newSelectedCell = maxCell;
				break;
			case ZKeyboard::ccLeft:
				newSelectedCell = visibleCells.TopLeft();
				break;
			case ZKeyboard::ccUp:
				newSelectedCell = visibleCells.TopLeft();
				break;
			case ZKeyboard::ccRight:
				newSelectedCell = visibleCells.BottomRight() - ZPoint_T<int32>(1,1);
				break;
			case ZKeyboard::ccDown:
				newSelectedCell = visibleCells.BottomRight() - ZPoint_T<int32>(1,1);
				break;
			default:
				handledIt = false;
				break;
			}		
		}

	if (handledIt)
		{
		newSelectedCell.h = max(int32(0), min(int32(newSelectedCell.h), int32(maxCell.h - 1)));
		newSelectedCell.v = max(int32(0), min(int32(newSelectedCell.v), int32(maxCell.v - 1)));
		int32 edgeToShowH = newSelectedCell.h > oldSelectedCell.h && oldSelectedCell.h >= 0 ? newSelectedCell.h + 1 : newSelectedCell.h;
		int32 edgeToShowV = newSelectedCell.v > oldSelectedCell.v && oldSelectedCell.v >= 0 ? newSelectedCell.v + 1 : newSelectedCell.v;
		inGridPane->BringCellIntoView(ZPoint_T<int32>(edgeToShowH, edgeToShowV));

		this->SetSelectedCell(inGridPane, newSelectedCell, true);
		return true;
		}
	return false;
	}

// Our protocol
bool ZUIGridPane::Selection_Single::HasSelectedCell()
	{
	ZPoint_T<int32> selection = this->GetSelectedCell();
	return selection.h >= 0 && selection.v >= 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_SingleStandard

ZUIGridPane::Selection_SingleStandard::Selection_SingleStandard()
:	fSelectedCell(-1, -1)
	{}

ZUIGridPane::Selection_SingleStandard::~Selection_SingleStandard()
	{}

// From ZUIGridPane::Selection_Single
ZPoint_T<int32> ZUIGridPane::Selection_SingleStandard::GetSelectedCell()
	{ return fSelectedCell; }

void ZUIGridPane::Selection_SingleStandard::SetSelectedCell(ZUIGridPane* inGridPane, ZPoint_T<int32> inCell, bool inNotify)
	{
	if (inCell != fSelectedCell)
		{
		if (fSelectedCell.h >= 0 && fSelectedCell.v >= 0)
			{
			ZRect theRect;
			if (inGridPane->GetCellRect(fSelectedCell, theRect))
				inGridPane->Invalidate(theRect);
			}
		fSelectedCell = inCell;
		if (fSelectedCell.h >= 0 && fSelectedCell.v >= 0)
			{
			ZRect theRect;
			if (inGridPane->GetCellRect(fSelectedCell, theRect))
				inGridPane->Invalidate(theRect);
			}
		if (inNotify)
			this->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	}

void ZUIGridPane::Selection_SingleStandard::SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify)
	{
	this->SetSelectedCell(inGridPane, ZPoint_T<int32>(-1, -1), inNotify);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple

ZUIGridPane::Selection_Multiple::Selection_Multiple()
	{}

ZUIGridPane::Selection_Multiple::~Selection_Multiple()
	{}

// From ZUIGridPane::Selection
bool ZUIGridPane::Selection_Multiple::GetSelected(ZPoint_T<int32> inCell)
	{ return this->GetSelectedCells().Contains(inCell); }

bool ZUIGridPane::Selection_Multiple::Click(ZUIGridPane* inGridPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZMouseTracker* theTracker;
	if (inEvent.GetShift())
		theTracker = new ZUIGridPane::Selection_Multiple::TrackerShift(inGridPane, inHitPoint, this);
	else if (inEvent.GetCommand())
		theTracker = new ZUIGridPane::Selection_Multiple::TrackerCommand(inGridPane, inHitPoint, this);
	else
		theTracker = new ZUIGridPane::Selection_Multiple::TrackerSingle(inGridPane, inHitPoint, this);

	theTracker->Install();
	return true;
	}

bool ZUIGridPane::Selection_Multiple::KeyDown(ZUIGridPane* inGridPane, const ZEvent_Key& inEvent)
	{
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerSingle

ZUIGridPane::Selection_Multiple::TrackerSingle::TrackerSingle(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection)
:	ZMouseTracker(inGridPane, inHitPoint), fGridPane(inGridPane), fSelection(inSelection)
	{
	fPriorSelection = fSelection->GetSelectedCells();
	}

ZUIGridPane::Selection_Multiple::TrackerSingle::~TrackerSingle()
	{}

void ZUIGridPane::Selection_Multiple::TrackerSingle::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZPoint_T<int32> currentCell = fGridPane->FindCell(inNext, false);

	ZBigRegion newSelection;

	if (fGridPane->CellInRange(currentCell))
		newSelection = ZRect_T<int32>(currentCell.h, currentCell.v, currentCell.h + 1, currentCell.v + 1);

	if (inPhase == ePhaseEnd && fPriorSelection != newSelection)
		{
		fSelection->SetSelectedCells(fGridPane, newSelection, false);
		fSelection->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	else
		{
		ZBigRegion priorSelection = fSelection->GetSelectedCells();
		if (priorSelection != newSelection)
			{
			fSelection->SetSelectedCells(fGridPane, newSelection, false);
			fSelection->Internal_NotifyResponders(ZUISelection::SelectionFeedbackChanged);
			}
		}
#if 0 // commented out to avoid UpdateNow() hang on Windows. -ec 99.11.19
	fGridPane->Update();
#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerShift

ZUIGridPane::Selection_Multiple::TrackerShift::TrackerShift(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection)
:	ZMouseTracker(inGridPane, inHitPoint), fGridPane(inGridPane), fSelection(inSelection)
	{
	ZPoint_T<int32> hitCell = inGridPane->FindCell(inHitPoint, true);
	fAnchorCell = hitCell;
	fPriorSelection = fSelection->GetSelectedCells();
	if (!fPriorSelection.IsEmpty())
		{
		ZRect_T<int32> priorBounds = fPriorSelection.Bounds();
		if (hitCell.h <= priorBounds.left)
			fAnchorCell.h = priorBounds.right - 1;
		else if (hitCell.h >= priorBounds.right)
			fAnchorCell.h = priorBounds.left;
		else
			fAnchorCell.h = priorBounds.left;
	
		if (hitCell.v <= priorBounds.top)
			fAnchorCell.v = priorBounds.bottom - 1;
		else if (hitCell.v >= priorBounds.bottom)
			fAnchorCell.v = priorBounds.top;
		else
			fAnchorCell.v = priorBounds.top;
		}
	}

ZUIGridPane::Selection_Multiple::TrackerShift::~TrackerShift()
	{}

void ZUIGridPane::Selection_Multiple::TrackerShift::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZBigRegion newSelection = ZRect_T<int32>::sMinimalRect(fAnchorCell, fGridPane->FindCell(inNext, true));

	if (inPhase == ePhaseEnd && fPriorSelection != newSelection)
		{
		fSelection->SetSelectedCells(fGridPane, newSelection, false);
		fSelection->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	else
		{
		ZBigRegion priorSelection = fSelection->GetSelectedCells();
		if (priorSelection != newSelection)
			{
			fSelection->SetSelectedCells(fGridPane, newSelection, false);
			fSelection->Internal_NotifyResponders(ZUISelection::SelectionFeedbackChanged);
			}
		}
#if 0 // commented out to avoid UpdateNow() hang on Windows. -ec 99.11.19
	fGridPane->Update();
#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_Multiple::TrackerCommand

ZUIGridPane::Selection_Multiple::TrackerCommand::TrackerCommand(ZUIGridPane* inGridPane, ZPoint inHitPoint, Selection_Multiple* inSelection)
:	ZMouseTracker(inGridPane, inHitPoint), fGridPane(inGridPane), fSelection(inSelection)
	{
	fPriorSelection = fSelection->GetSelectedCells();
	ZPoint_T<int32> hitCell = inGridPane->FindCell(inHitPoint, false);
	fDeselecting = fPriorSelection.Contains(hitCell);
	}

ZUIGridPane::Selection_Multiple::TrackerCommand::~TrackerCommand()
	{}

void ZUIGridPane::Selection_Multiple::TrackerCommand::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZPoint_T<int32> currentCell = fGridPane->FindCell(inNext, false);

	ZBigRegion priorSelection = fSelection->GetSelectedCells();
	ZBigRegion newSelection = priorSelection;

	if (fGridPane->CellInRange(currentCell))
		{
		if (fDeselecting)
			newSelection -= ZRect_T<int32>(currentCell.h, currentCell.v, currentCell.h + 1, currentCell.v + 1);
		else
			newSelection |= ZRect_T<int32>(currentCell.h, currentCell.v, currentCell.h + 1, currentCell.v + 1);
		}

	if (inPhase == ePhaseEnd && fPriorSelection != newSelection)
		{
		fSelection->SetSelectedCells(fGridPane, newSelection, false);
		fSelection->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	else
		{
		ZBigRegion priorSelection = fSelection->GetSelectedCells();
		if (priorSelection != newSelection)
			{
			fSelection->SetSelectedCells(fGridPane, newSelection, false);
			fSelection->Internal_NotifyResponders(ZUISelection::SelectionFeedbackChanged);
			}
		}
#if 0 // commented out to avoid UpdateNow() hang on Windows. -ec 99.11.19
	fGridPane->Update();
#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Selection_MultipleStandard

ZUIGridPane::Selection_MultipleStandard::Selection_MultipleStandard()
	{}

ZUIGridPane::Selection_MultipleStandard::~Selection_MultipleStandard()
	{}

// From Selection_Multiple
ZBigRegion ZUIGridPane::Selection_MultipleStandard::GetSelectedCells()
	{
	return fSelection;
	}

void ZUIGridPane::Selection_MultipleStandard::SetSelectedCells(ZUIGridPane* inGridPane, const ZBigRegion& inNewSelection, bool inNotify)
	{
	if (fSelection != inNewSelection)
		{
		ZDCRgn theInvalRegion = inGridPane->GetCellPixels(fSelection ^ inNewSelection);
		fSelection = inNewSelection;
		inGridPane->Invalidate(theInvalRegion);
		if (inNotify)
			this->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	}

void ZUIGridPane::Selection_MultipleStandard::SetEmptySelection(ZUIGridPane* inGridPane, bool inNotify)
	{
	ZBigRegion emptySelection;
	this->SetSelectedCells(inGridPane, emptySelection, inNotify);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry

ZUIGridPane::Geometry::Geometry()
	{}

ZUIGridPane::Geometry::~Geometry()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry_Fixed

ZUIGridPane::Geometry_Fixed::Geometry_Fixed()
	{}

ZUIGridPane::Geometry_Fixed::~Geometry_Fixed()
	{}

int32 ZUIGridPane::Geometry_Fixed::GetIterationCost(bool inHorizontal)
	{
// An arbitrary number smaller than Geometry_Variable
	return 500;
	}

void ZUIGridPane::Geometry_Fixed::DrawRangeMaster(bool inHorizontal, const ZDC& inDC,
										ZCoord inBeginCoordMaster, ZCoord inEndCoordMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inBeginCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Geometry* inSlaveGeometry, Selection* inSelection)
	{
	ZCoord cellSize = this->GetCellSize(inHorizontal);
	int32 maxCell = this->GetCellCount(inHorizontal);

	// Update inBeginCellMaster to the first cell we're gonna deal with
	inBeginCellMaster += (inBeginCoordMaster / cellSize);

	// Coerce the coordinate to the top of inBeginCellMaster
	ZCoord masterOrigin = inBeginCoordMaster - (inBeginCoordMaster % cellSize);
	for (int32 currentCell = inBeginCellMaster; currentCell < maxCell; ++currentCell)
		{
		inSlaveGeometry->DrawRangeSlave(!inHorizontal, inDC, masterOrigin, cellSize, inBeginCoordSlave, inEndCoordSlave, currentCell, inBeginCellSlave, inCellRenderer, inSelection);
		masterOrigin += cellSize;
		if (masterOrigin > inEndCoordMaster)
			break;
		}
	}

void ZUIGridPane::Geometry_Fixed::DrawRangeSlave(bool inHorizontal, const ZDC& inDC,
										ZCoord inOriginMaster, ZCoord inCellSizeMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Selection* inSelection)
	{
	if (!inCellRenderer)
		return;
	ZCoord cellSize = this->GetCellSize(inHorizontal);
	int32 maxCell = this->GetCellCount(inHorizontal);
	inBeginCellSlave += (inBeginCoordSlave / cellSize);
	ZCoord slaveOrigin = inBeginCoordSlave - (inBeginCoordSlave % cellSize);

	// We handle the horizontal/vertical issue by a call to theCellBounds.Flipped() if necessary
	ZRect theCellBounds;
	theCellBounds.top = inOriginMaster;
	theCellBounds.bottom = inOriginMaster + inCellSizeMaster;
	theCellBounds.left = slaveOrigin;
	theCellBounds.right = slaveOrigin + cellSize;

	for (int32 currentCell = inBeginCellSlave; currentCell < maxCell; ++currentCell)
		{
		ZPoint_T<int32> theCell(currentCell, inCellMaster);
		ZRect tempCellBounds = theCellBounds;
		if (!inHorizontal)
			{
			theCell = theCell.Flipped();
			tempCellBounds = theCellBounds.Flipped();
			}

		bool isSelected = false;
		if (inSelection)
			isSelected = inSelection->GetSelected(theCell);
		inCellRenderer->DrawCell(inDC, theCell, tempCellBounds, isSelected, false);

		theCellBounds.left += cellSize;
		theCellBounds.right += cellSize;
		}
	}

int32 ZUIGridPane::Geometry_Fixed::PixelToCell(bool inHorizontal, int32 inOriginCell, ZCoord inPixel, bool inRoundUp, bool inClamp)
	{
	ZCoord cellSize = this->GetCellSize(inHorizontal);
	int32 resultCell;
	if (inPixel >= 0)
		{
		if (inRoundUp)
			resultCell = inOriginCell + ((inPixel + cellSize - 1) / cellSize);
		else
			resultCell = inOriginCell + (inPixel / cellSize);
		}
	else
		{
		if (inRoundUp)
			resultCell = inOriginCell + (inPixel / cellSize);
		else
			resultCell = inOriginCell + ((inPixel - cellSize + 1) / cellSize);
		}
	if (inClamp)
		return max(int32(0), min(resultCell, this->GetCellCount(inHorizontal)));
	return resultCell;
	}

ZCoord ZUIGridPane::Geometry_Fixed::CellToPixel(bool inHorizontal, int32 inOriginCell, int32 inCell)
	{
	ZCoord cellSize = this->GetCellSize(inHorizontal);
	double result = cellSize * (inCell - inOriginCell);

	if (result > ZCoord_Max)
		return ZCoord_Max;
	else if (result < ZCoord_Min)
		return ZCoord_Min;
	return ZCoord(result);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGridPane::Geometry_Variable

ZUIGridPane::Geometry_Variable::Geometry_Variable()
	{}

ZUIGridPane::Geometry_Variable::~Geometry_Variable()
	{}

int32 ZUIGridPane::Geometry_Variable::GetIterationCost(bool inHorizontal)
	{
	// An arbitrary number larger than Geometry_Fixed
	return 1000;
	}

void ZUIGridPane::Geometry_Variable::DrawRangeMaster(bool inHorizontal, const ZDC& inDC,
										ZCoord inBeginCoordMaster, ZCoord inEndCoordMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inBeginCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Geometry* inSlaveGeometry, Selection* inSelection)
	{
	int32 maxCell = this->GetCellCount(inHorizontal);
	ZCoord masterOrigin = 0;
	for (int32 currentCell = inBeginCellMaster; currentCell < maxCell; ++currentCell)
		{
		ZCoord cellSize = this->GetCellSize(inHorizontal, currentCell);
		if (masterOrigin + cellSize >= inBeginCoordMaster)
			inSlaveGeometry->DrawRangeSlave(!inHorizontal, inDC, masterOrigin, cellSize, inBeginCoordSlave, inEndCoordSlave, currentCell, inBeginCellSlave, inCellRenderer, inSelection);
		masterOrigin += cellSize;
		if (masterOrigin >= inEndCoordMaster)
			break;
		}
	}

void ZUIGridPane::Geometry_Variable::DrawRangeSlave(bool inHorizontal, const ZDC& inDC,
										ZCoord inOriginMaster, ZCoord inCellSizeMaster,
										ZCoord inBeginCoordSlave, ZCoord inEndCoordSlave,
										int32 inCellMaster, int32 inBeginCellSlave,
										ZUIGridPane::CellRenderer* inCellRenderer, Selection* inSelection)
	{
	if (!inCellRenderer)
		return;
	int32 maxCell = this->GetCellCount(inHorizontal);

	// We handle the horizontal/vertical issue by a call to theCellBounds.Flipped() if necessary
	ZRect theCellBounds;
	theCellBounds.top = inOriginMaster;
	theCellBounds.bottom = inOriginMaster + inCellSizeMaster;
	theCellBounds.left = 0;
	for (int32 currentCell = inBeginCellSlave; currentCell < maxCell; ++currentCell)
		{
		ZCoord cellSize = this->GetCellSize(inHorizontal, currentCell);
		if (theCellBounds.left + cellSize >= inBeginCoordSlave)
			{
			theCellBounds.right = theCellBounds.left + cellSize;
			ZPoint_T<int32> theCell(currentCell, inCellMaster);
			ZRect tempCellBounds = theCellBounds;
			if (!inHorizontal)
				{
				theCell = theCell.Flipped();
				tempCellBounds = theCellBounds.Flipped();
				}

			bool isSelected = false;
			if (inSelection)
				isSelected = inSelection->GetSelected(theCell);
			inCellRenderer->DrawCell(inDC, theCell, tempCellBounds, isSelected, false);

#if 0
			bool isSelected = false;
			if (inSelection)
				isSelected = inSelection->GetSelected(inHorizontal ? theCell : theCell.Flipped());
			inCellRenderer->DrawCell(inDC, inHorizontal ? theCell : theCell.Flipped(), inHorizontal ? theCellBounds : theCellBounds.Flipped(), isSelected, false);
#endif
			}
		theCellBounds.left += cellSize;
		if (theCellBounds.left >= inEndCoordSlave)
			break;
		}
	}

int32 ZUIGridPane::Geometry_Variable::PixelToCell(bool inHorizontal, int32 inOriginCell, ZCoord inPixel, bool inRoundUp, bool inClamp)
	{
	int32 currentCell = inOriginCell;
	int32 maxCell = this->GetCellCount(inHorizontal);
	ZCoord cellSize;
	if (inPixel >= 0)
		{
		// Handle the edge case of when we get started with zero pixels
		if (inPixel == 0)
			return currentCell;
		while (currentCell < maxCell)
			{
			cellSize = this->GetCellSize(inHorizontal, currentCell);
			if (cellSize >= inPixel)
				{
				if (inRoundUp)
					return currentCell + 1;
				else
					return currentCell;
				}
			inPixel -= cellSize;
			currentCell += 1;
			}
		}
	else
		{
		inPixel = -inPixel;
		if (inPixel == 0)
			return currentCell;
		if (inRoundUp)
			{
			while (currentCell > 0)
				{
				if (currentCell <= maxCell)
					cellSize = this->GetCellSize(inHorizontal, currentCell - 1);
				else
					cellSize = 0;
				if (cellSize > inPixel)
					return currentCell;
				inPixel -= cellSize;
				currentCell -= 1;
				}
			}
		else
			{
			while (currentCell > 0)
				{
				if (currentCell <= maxCell)
					cellSize = this->GetCellSize(inHorizontal, currentCell - 1);
				else
					cellSize = 0;
				if (cellSize >= inPixel)
					return currentCell - 1;
				inPixel -= cellSize;
				currentCell -= 1;
				}
			}
		}
	return currentCell;
	}

ZCoord ZUIGridPane::Geometry_Variable::CellToPixel(bool inHorizontal, int32 inOriginCell, int32 inCell)
	{
	if (inOriginCell == inCell)
		return 0;

	int32 currentCell, endCell;
	if (inOriginCell <= inCell)
		{
		currentCell = inOriginCell;
		endCell = inCell;
		}
	else
		{
		currentCell = inCell;
		endCell = inOriginCell;
		}
	int32 cellCount = this->GetCellCount(inHorizontal);
	currentCell = min(cellCount, max(int32(0), currentCell));
	endCell = min(cellCount, max(int32(0), endCell));
	ZCoord pixels = 0;
	while (currentCell < endCell)
		{
		ZCoord newPixels = pixels + this->GetCellSize(inHorizontal, currentCell);
		// Check for wrap around
		if (newPixels < pixels)
			{
			if (inOriginCell <= inCell)
				return ZCoord_Max;
			return ZCoord_Min;
			}
		pixels = newPixels;
		++currentCell;
		}
	if (inOriginCell <= inCell)
		return pixels;
	return -pixels;
	}
