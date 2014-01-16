static const char rcsid[] = "@(#) $Id: ZUICartesianPlane.cpp,v 1.3 2005/11/13 18:53:08 agreen Exp $";

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

#include "ZUICartesianPlane.h"
#include "ZRegionAdorner.h"
#include "ZUI.h" // For ZUISelection

ZUICartesianPlane::ZUICartesianPlane()
	{}

ZUICartesianPlane::~ ZUICartesianPlane()
	{
	}

bool ZUICartesianPlane::Click(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	long hitIndex = this->IndexFromPoint(inHitPoint);
	if (hitIndex >= 0)
		{
		long newHitIndex = this->BringIndexToFront(hitIndex);
		if (newHitIndex != hitIndex)
			{
			this->InvalidateIndex(newHitIndex);
			hitIndex = newHitIndex;
			}

		if (inEvent.GetShift())
			{
			if (this->GetIndexSelected(hitIndex))
				this->SetIndexSelected(hitIndex, false, true);
			else
				this->SetIndexSelected(hitIndex, true, true);
			}
		else
			{
			if (!this->GetIndexSelected(hitIndex))
				{
				set<long> newSelection;
				newSelection.insert(hitIndex);
				this->SetSelectedIndices(newSelection, true);
				}
			}

		if (this->GetIndexSelected(hitIndex))
			{
			if (inEvent.GetClickCount() > 1)
				this->HandleDoubleClick(hitIndex, inHitPoint, inEvent);
			else
				this->HandleDrag(hitIndex, inHitPoint, inEvent);
			}
		}
	else
		{
		if (!inEvent.GetShift())
			{
			set<long> newSelection;
			this->SetSelectedIndices(newSelection, true);
			}
		this->HandleBackgroundClick(inHitPoint, inEvent);
		}
	return true;
	}

ZDCRgn ZUICartesianPlane::RegionFromIndices(const set<long>& inIndices)
	{
	ZDCRgn theRgn;
	for (set<long>::const_iterator i = inIndices.begin(); i != inIndices.end(); ++i)
		theRgn |= this->RegionFromIndex(*i);
	return theRgn;
	}

void ZUICartesianPlane::InvalidateIndex(long inIndex)
	{ this->Invalidate(this->RegionFromIndex(inIndex)); }

long ZUICartesianPlane::BringIndexToFront(long inIndex)
	{ return inIndex; }

bool ZUICartesianPlane::GetIndexSelected(long inIndex)
	{
	set<long> currentSelection;
	this->GetSelectedIndices(currentSelection);
	return currentSelection.find(inIndex) != currentSelection.end();
	}

void ZUICartesianPlane::SetIndexSelected(long inIndex, bool selectIt, bool inNotify)
	{
	set<long> currentSelection;
	this->GetSelectedIndices(currentSelection);
	if (selectIt)
		{
		if (currentSelection.find(inIndex) == currentSelection.end())
			{
			currentSelection.insert(inIndex);
			this->SetSelectedIndices(currentSelection, inNotify);
			}
		}
	else
		{
		if (currentSelection.find(inIndex) != currentSelection.end())
			{
			currentSelection.erase(inIndex);
			this->SetSelectedIndices(currentSelection, inNotify);
			}
		}
	}

bool ZUICartesianPlane::HandleDoubleClick(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}
bool ZUICartesianPlane::HandleBackgroundClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}
bool ZUICartesianPlane::HandleDrag(long inHitIndex, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}

ZPoint ZUICartesianPlane::FromPane(ZPoint inPanePoint)
	{ return inPanePoint; }

// ==================================================
#pragma mark -
#pragma mark * ZUICartesianPlane::Selection::TrackerBackground

ZUICartesianPlane::TrackerBackground::TrackerBackground(ZSubPane* inSubPane, ZUICartesianPlane* inPlane, ZPoint inHitPoint, ZUISelection* inSelection)
:	ZMouseTracker(inSubPane, inHitPoint), fCartesianPlane(inPlane), fUISelection(inSelection)
	{
	fCartesianPlane->GetSelectedIndices(fOriginalSelection);
	fRegionAdorner = new ZRegionAdorner(ZDCPattern::sGray);
	}

ZUICartesianPlane::TrackerBackground::~TrackerBackground()
	{
	}

void ZUICartesianPlane::TrackerBackground::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	if (inPhase == ePhaseBegin)
		this->GetPane()->InstallAdorner(fRegionAdorner);

	ZPoint paneToEngineOffset = fCartesianPlane->FromPane(ZPoint::sZero);

	ZPoint localAnchor = inAnchor + paneToEngineOffset;
	ZPoint localPrevious = inPrevious + paneToEngineOffset;
	ZPoint localNext = inNext + paneToEngineOffset;


	ZDCRgn currentRegion = ZRect::sMinimalRect(localAnchor, localNext);

	set<long> indicesInRegion;
	fCartesianPlane->IndicesFromRegion(currentRegion, indicesInRegion);

	set<long> newSelection;
	set_difference(indicesInRegion.begin(), indicesInRegion.end(), fOriginalSelection.begin(), fOriginalSelection.end(), inserter(newSelection, newSelection.begin()));
	set_difference(fOriginalSelection.begin(), fOriginalSelection.end(), indicesInRegion.begin(), indicesInRegion.end(), inserter(newSelection, newSelection.begin()));

	if (inPhase == ePhaseEnd && fOriginalSelection != newSelection)
		{
		fCartesianPlane->SetSelectedIndices(newSelection, false);
		if (fUISelection)
			fUISelection->Internal_NotifyResponders(ZUISelection::SelectionChanged);
		}
	else
		{
		set<long> priorSelection;
		fCartesianPlane->GetSelectedIndices(priorSelection);
		if (priorSelection != newSelection)
			{
			fCartesianPlane->SetSelectedIndices(newSelection, false);
			if (fUISelection)
				fUISelection->Internal_NotifyResponders(ZUISelection::SelectionFeedbackChanged);
			}
		}

	ZDC theDC = this->GetPane()->GetDC();
	if (inPhase == ePhaseEnd)
		{
		fRegionAdorner->SetRgn(theDC, ZDCRgn());
		this->GetPane()->RemoveAdorner(fRegionAdorner);
		}
	else
		{
		fRegionAdorner->SetRgn(theDC, (currentRegion - paneToEngineOffset).Outline());
		}
	}
