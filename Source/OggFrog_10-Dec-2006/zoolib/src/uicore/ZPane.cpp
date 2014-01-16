static const char rcsid[] = "@(#) $Id: ZPane.cpp,v 1.9 2006/08/20 20:08:23 agreen Exp $";

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

#include "ZPane.h"
#include "ZFakeWindow.h"
#include "ZDebug.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneAdorner

ZPaneAdorner::ZPaneAdorner()
	{}

ZPaneAdorner::~ZPaneAdorner()
	{}

void ZPaneAdorner::AdornPane(const ZDC& inDC, ZSubPane* inPane, const ZDCRgn& inPaneBoundsRgn)
	{}

void ZPaneAdorner::Refresh(ZSubPane* inPane)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocator

ZPaneLocator::ZPaneLocator(ZPaneLocator* inNextLocator)
:	fReferencingPaneLocators(nil), fReferencingSubPanes(nil), fNextPaneLocator(inNextLocator)
	{
	if (fNextPaneLocator)
		fNextPaneLocator->ReferencingPaneLocatorAdded(this);
	}

ZPaneLocator::~ZPaneLocator()
	{
	while (fReferencingSubPanes)
		(*fReferencingSubPanes)[0]->PaneLocatorDeleted(this);
	while (fReferencingPaneLocators)
		(*fReferencingPaneLocators)[0]->PaneLocatorDeleted(this);
	if (fNextPaneLocator)
		fNextPaneLocator->ReferencingPaneLocatorRemoved(this);
	}

bool ZPaneLocator::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneLocation(inPane, outLocation))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneSize(inPane, outSize))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneVisible(ZSubPane* inPane, bool& outVisible)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneVisible(inPane, outVisible))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneEnabled(ZSubPane* inPane, bool& outEnabled)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneEnabled(inPane, outEnabled))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneHilite(ZSubPane* inPane, EZTriState& outHilite)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneHilite(inPane, outHilite))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneBackInk(inPane, inDC, outInk))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneCursor(ZSubPane* inPane, ZPoint inPoint, ZCursor& outCursor)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneCursor(inPane, inPoint, outCursor))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneBringsFront(ZSubPane* inPane, bool& outBringsFront)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneBringsFront(inPane, outBringsFront))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneDragDropHandler(ZSubPane* inPane, ZDragDropHandler*& outDragDropHandler)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneDragDropHandler(inPane, outDragDropHandler))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneTranslation(ZSuperPane* inPane, ZPoint& outSize)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneTranslation(inPane, outSize))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneInternalSize(ZSuperPane* inPane, ZPoint& outSize)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneInternalSize(inPane, outSize))
		return true;
	return false;
	}

bool ZPaneLocator::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (fNextPaneLocator && fNextPaneLocator->GetPaneAttribute(inPane, inAttribute, inParam, outResult))
		return true;
	return false;
	}

void ZPaneLocator::ReferencingSubPaneAdded(ZSubPane* inSubPane)
	{
	if (!fReferencingSubPanes)
		fReferencingSubPanes = new vector<ZSubPane*>;
	fReferencingSubPanes->push_back(inSubPane);
	}

void ZPaneLocator::ReferencingSubPaneRemoved(ZSubPane* inSubPane)
	{
	ZAssertStop(2, fReferencingSubPanes);
	vector<ZSubPane*>::iterator theIter = find(fReferencingSubPanes->begin(), fReferencingSubPanes->end(), inSubPane);
	ZAssertStop(2, theIter != fReferencingSubPanes->end());
	fReferencingSubPanes->erase(theIter);
	if (fReferencingSubPanes->size() == 0)
		{
		delete fReferencingSubPanes;
		fReferencingSubPanes = nil;
		}
	}

void ZPaneLocator::ReferencingPaneLocatorAdded(ZPaneLocator* inPaneLocator)
	{
	if (!fReferencingPaneLocators)
		fReferencingPaneLocators = new vector<ZPaneLocator*>;
	fReferencingPaneLocators->push_back(inPaneLocator);
	}

void ZPaneLocator::ReferencingPaneLocatorRemoved(ZPaneLocator* inPaneLocator)
	{
	ZAssertStop(2, fReferencingPaneLocators);
	vector<ZPaneLocator*>::iterator theIter = find(fReferencingPaneLocators->begin(), fReferencingPaneLocators->end(), inPaneLocator);
	ZAssertStop(2, theIter != fReferencingPaneLocators->end());
	fReferencingPaneLocators->erase(theIter);
	if (fReferencingPaneLocators->size() == 0)
		{
		delete fReferencingPaneLocators;
		fReferencingPaneLocators = nil;
		}
	}

void ZPaneLocator::PaneLocatorDeleted(ZPaneLocator* inPaneLocator)
	{
	ZAssertStop(2, inPaneLocator == fNextPaneLocator);
	fNextPaneLocator->ReferencingPaneLocatorRemoved(this);
	fNextPaneLocator = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorSimple

ZPaneLocatorSimple::ZPaneLocatorSimple(ZPaneLocator* inNextLocator)
:	ZPaneLocator(inNextLocator)
	{}

void ZPaneLocatorSimple::ReferencingPaneLocatorRemoved(ZPaneLocator* inPaneLocator)
	{
	ZPaneLocator::ReferencingPaneLocatorRemoved(inPaneLocator);
	if (!fReferencingSubPanes && !fReferencingPaneLocators)
		delete this;
	}

void ZPaneLocatorSimple::ReferencingSubPaneRemoved(ZSubPane* inSubPane)
	{
	ZPaneLocator::ReferencingSubPaneRemoved(inSubPane);
	if (!fReferencingSubPanes && !fReferencingPaneLocators)
		delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorFixed

ZPaneLocatorFixed::ZPaneLocatorFixed(ZPoint inLocation, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fLocation(inLocation), fGotSize(false)
	{}

ZPaneLocatorFixed::ZPaneLocatorFixed(ZPoint inLocation, ZPoint inSize, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fLocation(inLocation), fSize(inSize), fGotSize(true)
	{}

ZPaneLocatorFixed::ZPaneLocatorFixed(ZCoord inLeft, ZCoord inTop, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fLocation(inLeft, inTop), fGotSize(false)
	{}

ZPaneLocatorFixed::ZPaneLocatorFixed(ZCoord inLeft, ZCoord inTop, ZCoord inWidth, ZCoord inHeight, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fLocation(inLeft, inTop), fSize(inWidth, inHeight), fGotSize(true)
	{}

bool ZPaneLocatorFixed::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	outLocation = fLocation;
	return true;
	}

bool ZPaneLocatorFixed::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (fGotSize)
		{
		outSize = fSize;
		return true;
		}
	return ZPaneLocatorSimple::GetPaneSize(inPane, outSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPaneLocatorFixedAligned

ZPaneLocatorFixedAligned::ZPaneLocatorFixedAligned(ZPoint inPosition, EZAlignment inAlignmentH, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fPosition(inPosition), fAlignmentH(inAlignmentH), fAlignmentV(eZAlignment_Top)
	{}

ZPaneLocatorFixedAligned::ZPaneLocatorFixedAligned(ZPoint inPosition, EZAlignment inAlignmentH, EZAlignment inAlignmentV, ZPaneLocator* inNextLocator)
:	ZPaneLocatorSimple(inNextLocator), fPosition(inPosition), fAlignmentH(inAlignmentH), fAlignmentV(inAlignmentV)
	{}

bool ZPaneLocatorFixedAligned::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	outLocation = fPosition;
	ZPoint paneSize = inPane->GetSize();
	switch (fAlignmentH)
		{
		case eZAlignment_Left:
			break;
		case eZAlignment_Center://teJustCenter:
			outLocation.h = outLocation.h - paneSize.h/2;
			break;
		case eZAlignment_Right:
			outLocation.h = outLocation.h - paneSize.h;
			break;
		}
	switch (fAlignmentV)
		{
		case eZAlignment_Top:
			break;
		case eZAlignment_Center://teJustCenter:
			outLocation.v = outLocation.v - paneSize.v/2;
			break;
		case eZAlignment_Bottom:
			outLocation.v = outLocation.v - paneSize.v;
			break;
		}
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZSubPane

ZSubPane::ZSubPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZEventHr(inSuperPane), fSuperPane(inSuperPane), fPaneLocator(inPaneLocator), fAdorners(nil)
	{
	ZAssertStop(2, fSuperPane);
	fSuperPane->SubPaneAdded(this);
	if (fPaneLocator)
		fPaneLocator->ReferencingSubPaneAdded(this);
	}

ZSubPane::ZSubPane(ZEventHr* inNextHandler, ZPaneLocator* inPaneLocator)
:	ZEventHr(inNextHandler), fSuperPane(nil), fPaneLocator(inPaneLocator), fAdorners(nil)
	{
	if (fPaneLocator)
		fPaneLocator->ReferencingSubPaneAdded(this);
	}

ZSubPane::~ZSubPane()
	{
	if (fPaneLocator)
		fPaneLocator->ReferencingSubPaneRemoved(this);
	if (fSuperPane)
		fSuperPane->SubPaneRemoved(this);
	delete fAdorners;
	}

bool ZSubPane::WantsToBecomeTarget()
	{
	// Most panes do *not* want to become the *actual* target. They
	// can pass stuff up and down the chain, and they may have menu commands
	// etc, but they only deal with that stuff by virtue of being in the chain.
	return false;
	}

bool ZSubPane::WantsToBecomeTabTarget()
	{
	// If we want to become the target in general, then we'll
	// want to become it by tabbing (if we're visible && enabled of course)
	return this->WantsToBecomeTarget() && this->GetReallyEnabled() && this->GetReallyVisible();
	}

void ZSubPane::BeInSuperPane(ZSuperPane* inSuperPane)
	{
	if (fSuperPane != inSuperPane)
		{
		if (fSuperPane)
			fSuperPane->SubPaneRemoved(this);
		fSuperPane = inSuperPane;
		if (fSuperPane)
			fSuperPane->SubPaneAdded(this);
		}
	}

ZSuperPane* ZSubPane::GetOutermostSuperPane()
	{
	if (fSuperPane)
		return fSuperPane->GetOutermostSuperPane();
	ZDebugStopf(1, ("ZSubPane::GetOutermostSuperPane, should have been overridden somewher"));
	return nil;
	}

ZFakeWindow* ZSubPane::GetWindow()
	{
	if (fSuperPane)
		return fSuperPane->GetWindow();
	return nil;
	}

void ZSubPane::Activate(bool inEntering)
	{}

ZMutexBase& ZSubPane::GetLock()
	{ return this->GetWindow()->GetWindowLock(); }

void ZSubPane::HandleDraw(const ZDC& inDC)
	{
	// inDC has been setup with our coords, but with our superPane's clip rgn. If we don't
	// have a superpane, then the clip is set to our visible bounds.
	ZDC localDC(inDC);

	ZDCRgn oldClip = localDC.GetClip();
	ZDCRgn boundsRgn = this->CalcBoundsRgn();
	if (ZDCRgn newClip = oldClip & boundsRgn)
		{
		localDC.SetClip(newClip);
		if (ZDCInk theInk = this->GetBackInk(inDC))
			localDC.SetInk(theInk);

		this->DoDraw(localDC, boundsRgn);
		}

	if (fAdorners)
		{
		for (size_t x = 0; x < fAdorners->size(); ++x)
			fAdorners[x][0]->AdornPane(inDC, this, boundsRgn);
		}
	}

void ZSubPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	inDC.Fill(inBoundsRgn);
	}

void ZSubPane::DrawNow()
	{
	this->Refresh();
	this->Update();
	}

#if 1
ZDC ZSubPane::GetDC()
	{
	ZDC theDC(this->GetWindow()->GetWindowCanvas(), ZPoint::sZero);
	theDC.SetOrigin(this->ToWindow(ZPoint::sZero));
	theDC.SetPatternOrigin(this->GetCumulativeTranslation());
	theDC.SetClip(this->CalcVisibleBoundsRgn());
	return theDC;
	}

ZDC ZSubPane::GetDC(ZDCRgn& outBoundsRgn)
	{
	ZDC theDC(this->GetWindow()->GetWindowCanvas(), ZPoint::sZero);
	theDC.SetOrigin(this->ToWindow(ZPoint::sZero));
	theDC.SetPatternOrigin(this->GetCumulativeTranslation());
	outBoundsRgn = this->CalcBoundsRgn();
	if (this->GetVisible())
		{
		if (fSuperPane)
			theDC.SetClip(outBoundsRgn & this->FromSuper(fSuperPane->CalcVisibleBoundsRgn()));
		else
			theDC.SetClip(outBoundsRgn);
		}
	return theDC;
	}
#endif

ZSubPane* ZSubPane::FindPane(ZPoint inPoint)
	{ return this; }

bool ZSubPane::HandleClick(ZPoint theHitPoint, const ZEvent_Mouse& theEvent)
	{
	if (this->GetBringsFront())
		{
		ZFakeWindow* myWindow = this->GetWindow();
		if (myWindow)
			myWindow->BringFront();
		}

	if (!this->GetEnabled())
		return false;

	if (this->WantsToBecomeTarget())
		this->BecomeWindowTarget();

	return this->DoClick(theHitPoint, theEvent);
	}

bool ZSubPane::DoClick(ZPoint theHitPoint, const ZEvent_Mouse& theEvent)
	{ return false; }

bool ZSubPane::GetBringsFront()
	{
	bool bringsFront = false;
	if (fPaneLocator && fPaneLocator->GetPaneBringsFront(this, bringsFront))
		return bringsFront;
	return true;
	}

void ZSubPane::MouseMotion(ZPoint inPoint, ZMouse::Motion inMotion)
	{
	if (inMotion == ZMouse::eMotionEnter || inMotion == ZMouse::eMotionLeave)
		this->InvalidateCursor();
	}

bool ZSubPane::GetCursor(ZPoint inPoint, ZCursor& outCursor)
	{
	if (fPaneLocator)
		return fPaneLocator->GetPaneCursor(this, inPoint, outCursor);
	return false;
	}

void ZSubPane::InvalidateCursor()
	{
	ZFakeWindow* myWindow = this->GetWindow();
	if (myWindow)
		myWindow->InvalidateWindowCursor();
	}

ZDragDropHandler* ZSubPane::GetDragDropHandler()
	{
	ZDragDropHandler* theDragDropHandler;
	if (fPaneLocator && fPaneLocator->GetPaneDragDropHandler(this, theDragDropHandler))
		return theDragDropHandler;
	return nil;
	}

void ZSubPane::Invalidate(const ZDCRgn& inRgn)
	{
	ZFakeWindow* myWindow = this->GetWindow();
	if (this->GetVisible() && myWindow)
		myWindow->InvalidateWindowRegion(this->ToWindow(inRgn & this->CalcApertureRgn()));
	}

void ZSubPane::Refresh()
	{
	if (!this->GetVisible())
		return;
	if (fAdorners)
		{
		for (size_t x = 0; x < fAdorners->size(); ++x)
			fAdorners[x][0]->Refresh(this);
		}
	this->Invalidate(this->CalcVisibleBoundsRgn());
	}

void ZSubPane::Update()
	{
	ZFakeWindow* myWindow = this->GetWindow();
	if (myWindow)
		myWindow->UpdateWindowNow();
	}

void ZSubPane::SetPaneLocator(ZPaneLocator* inPaneLocator, bool inRefresh)
	{
	if (fPaneLocator == inPaneLocator)
		return;
	if (inRefresh)
		this->Refresh();
	if (fPaneLocator)
		fPaneLocator->ReferencingSubPaneRemoved(this);
	fPaneLocator = inPaneLocator;
	if (fPaneLocator)
		fPaneLocator->ReferencingSubPaneAdded(this);
	if (inRefresh)
		this->Refresh();
	}

void ZSubPane::PaneLocatorDeleted(ZPaneLocator* thePaneLocator)
	{
	ZAssertStop(2, thePaneLocator == fPaneLocator);
	fPaneLocator->ReferencingSubPaneRemoved(this);
	fPaneLocator = nil;
	}

ZPoint ZSubPane::GetLocation()
	{
	ZPoint theLocation;
	if (fPaneLocator && fPaneLocator->GetPaneLocation(this, theLocation))
		return theLocation;
	return ZPoint::sZero;
	}

ZPoint ZSubPane::GetSize()
	{
	ZPoint theSize;
	if (fPaneLocator && fPaneLocator->GetPaneSize(this, theSize))
		return theSize;
	if (fSuperPane)
		return fSuperPane->GetInternalSize();
	return ZPoint::sZero;
	}

ZPoint ZSubPane::GetCumulativeTranslation()
	{
	if (fSuperPane)
		return fSuperPane->GetCumulativeTranslation();
	return ZPoint::sZero;
	}

bool ZSubPane::GetVisible()
	{
	bool theVisible;
	if (fPaneLocator && fPaneLocator->GetPaneVisible(this, theVisible))
		return theVisible;
	return true;
	}

bool ZSubPane::GetReallyVisible()
	{
	if (this->GetVisible())
		{
		if (fSuperPane)
			return fSuperPane->GetReallyVisible();
		return true;
		}
	return false;
	}

bool ZSubPane::GetActive()
	{
	if (ZFakeWindow* myWindow = this->GetWindow())
		return myWindow->GetWindowActive();
	return true;
	}

bool ZSubPane::GetEnabled()
	{
	bool theEnabled;
	if (fPaneLocator && fPaneLocator->GetPaneEnabled(this, theEnabled))
		return theEnabled;
	return true;
	}

bool ZSubPane::GetReallyEnabled()
	{
	if (this->GetEnabled())
		{
		if (fSuperPane)
			return fSuperPane->GetReallyEnabled();
		return true;
		}
	return false;
	}

EZTriState ZSubPane::GetHilite()
	{
	EZTriState theHilite;
	if (fPaneLocator && fPaneLocator->GetPaneHilite(this, theHilite))
		return theHilite;
	return eZTriState_Off;
	}

ZDCInk ZSubPane::GetBackInk(const ZDC& inDC)
	{
	ZDCInk theInk;
	if (fPaneLocator && fPaneLocator->GetPaneBackInk(this, inDC, theInk))
		return theInk;
	return ZDCInk();
	}

ZDCInk ZSubPane::GetRealBackInk(const ZDC& inDC)
	{
	if (ZDCInk theInk = this->GetBackInk(inDC))
		return theInk;
	if (fSuperPane)
		{
		if (ZDCInk theInk = fSuperPane->GetInternalBackInk(inDC))
			return theInk;
		return fSuperPane->GetRealBackInk(inDC);
		}
	return ZDCInk::sWhite;
	}

bool ZSubPane::GetAttribute(const string& inAttribute, void* inParam, void* outResult)
	{
	if (fPaneLocator)
		return fPaneLocator->GetPaneAttribute(this, inAttribute, inParam, outResult);
	return false;
	}

bool ZSubPane::IsWindowTarget()
	{
	return this->GetWindowTarget() == this;
	}

void ZSubPane::DoIdle()
	{}

ZDCRgn ZSubPane::CalcBoundsRgn()
	{
	// By making this method virtual, you can have panes with holes in them (useful for
	// measurement in motion and opendoc)
	return ZRect(this->GetSize());
	}

ZRect ZSubPane::CalcBounds()
	{ return this->CalcBoundsRgn().Bounds(); }

ZDCRgn ZSubPane::CalcVisibleBoundsRgn()
	{
	// Calculate the portion of our bounds that would be actually visible to the user
	// if our window were front most
	ZDCRgn myBoundsRgn;
	if (this->GetVisible())
		{
		myBoundsRgn = this->CalcBoundsRgn();
		if (fSuperPane)
			myBoundsRgn &= this->FromSuper(fSuperPane->CalcVisibleBoundsRgn());
		}
	return myBoundsRgn;
	}

ZRect ZSubPane::CalcVisibleBounds()
	{ return this->CalcVisibleBoundsRgn().Bounds(); }

ZDCRgn ZSubPane::CalcApertureRgn()
	{
	ZDCRgn myBoundsRgn = this->CalcBoundsRgn();
	if (fSuperPane)
		myBoundsRgn &= this->FromSuper(fSuperPane->CalcApertureRgn());
	return myBoundsRgn;
	}

ZRect ZSubPane::CalcAperture()
	{ return this->CalcApertureRgn().Bounds(); }

ZDCRgn ZSubPane::CalcFrameRgn()
	{ return this->ToSuper(this->CalcBoundsRgn()); }

ZRect ZSubPane::CalcFrame()
	{ return this->CalcFrameRgn().Bounds(); }

void ZSubPane::FramePreChange()
	{
	ZPoint locationInWindow = this->ToWindow(ZPoint::sZero);
	this->HandleFramePreChange(locationInWindow);
	}

void ZSubPane::FramePostChange()
	{
	ZPoint locationInWindow = this->ToWindow(ZPoint::sZero);
	if (ZDCRgn invalidRgn = this->HandleFramePostChange(locationInWindow))
		this->GetWindow()->InvalidateWindowRegion(invalidRgn);
	}

void ZSubPane::HandleFramePreChange(ZPoint inWindowLocation)
	{
	this->DoFramePreChange();
	fPriorBounds = ZRect::sZero;
	if (this->GetVisible())
		fPriorBounds = ZRect(this->GetSize())+inWindowLocation;
	}

void ZSubPane::DoFramePreChange()
	{}

ZDCRgn ZSubPane::HandleFramePostChange(ZPoint inWindowLocation)
	{
	ZRect newBounds(ZRect::sZero);
	if (this->GetVisible())
		newBounds = ZRect(this->GetSize())+inWindowLocation;
	return this->DoFramePostChange(fPriorBounds, newBounds);
	}

ZDCRgn ZSubPane::DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds)
	{
	// For a sub pane, if it has changed at all we invalidate both the old and new bounds. This assumes
	// that a sub pane's contents are generally very dependent on its bounds and need to be completely
	// refreshed when those bounds change. If you're able to preserve more of a particular subpane's
	// content you can override this method appropriately.
	ZDCRgn theRgn;
	if (inOldBounds != inNewBounds)
		{
		theRgn |= inOldBounds;
		theRgn |= inNewBounds;
		}
	return theRgn;
	}

void ZSubPane::ScrollPreChange()
	{}

void ZSubPane::ScrollPostChange()
	{}

void ZSubPane::InstallAdorner(ZRef<ZPaneAdorner> theAdorner)
	{
	if (theAdorner)
		{
		if (fAdorners == nil)
			{
			// This special form of the vector constructor will allocate enough space for one
			// element immediately instead of doing a reallocation with push_back
			fAdorners = new vector<ZRef<ZPaneAdorner> >(1, theAdorner);
			}
		else
			{
			fAdorners->push_back(theAdorner);
			}
		}
	}

void ZSubPane::RemoveAdorner(ZRef<ZPaneAdorner> theAdorner)
	{
	if (theAdorner)
		{
		ZAssertStop(2, fAdorners != nil);
		vector<ZRef<ZPaneAdorner> >::iterator theIter = find(fAdorners->begin(), fAdorners->end(), theAdorner);
		ZAssertStop(2, theIter != fAdorners->end());
		fAdorners->erase(theIter);
		if (fAdorners->size() == 0)
			{
			delete fAdorners;
			fAdorners = nil;
			}
		}
	}

void ZSubPane::BringPointIntoViewPinned(ZPoint inPosition)
	{ this->BringPointIntoView(this->CalcBounds().Pin(inPosition)); }

void ZSubPane::BringPointIntoView(ZPoint inPosition)
	{
	if (fSuperPane)
		fSuperPane->BringPointIntoView(this->ToSuper(inPosition));
	}
void ZSubPane::BringSelectionIntoView()
	{}

ZPoint ZSubPane::ToSuper(const ZPoint& inPoint)
	{
	if (fSuperPane)
		return inPoint + this->GetLocation() - fSuperPane->GetTranslation();
	ZDebugStopf(1, ("Somebody's gotta overide ToSuper(const ZPoint& inPoint)!"));
	return inPoint + this->GetLocation();
	}

ZPoint ZSubPane::FromSuper(const ZPoint& inPoint)
	{ return inPoint - this->ToSuper(ZPoint::sZero); }

ZRect ZSubPane::ToSuper(const ZRect& inRect)
	{ return inRect + this->ToSuper(ZPoint::sZero); }

ZRect ZSubPane::FromSuper(const ZRect& inRect)
	{ return inRect + this->FromSuper(ZPoint::sZero); }

ZDCRgn ZSubPane::ToSuper(const ZDCRgn& inRgn)
	{ return inRgn + this->ToSuper(ZPoint::sZero); }

ZDCRgn ZSubPane::FromSuper(const ZDCRgn& inRgn)
	{ return inRgn + this->FromSuper(ZPoint::sZero); }

ZPoint ZSubPane::ToWindow(const ZPoint& inPoint)
	{
	ZPoint newPoint = this->ToSuper(inPoint);
	if (fSuperPane)
		return fSuperPane->ToWindow(newPoint);
	ZDebugStopf(1, ("Somebody's gotta overide ToWindow(const ZPoint& inPoint)!"));
	return newPoint;
	}
ZPoint ZSubPane::FromWindow(const ZPoint& inPoint)
	{ return inPoint - this->ToWindow(ZPoint::sZero); }

ZRect ZSubPane::ToWindow(const ZRect& inRect)
	{ return inRect + this->ToWindow(ZPoint::sZero); }

ZRect ZSubPane::FromWindow(const ZRect& inRect)
	{ return inRect + this->FromWindow(ZPoint::sZero); }

ZDCRgn ZSubPane::ToWindow(const ZDCRgn& inRgn)
	{ return inRgn + this->ToWindow(ZPoint::sZero); }

ZDCRgn ZSubPane::FromWindow(const ZDCRgn& inRgn)
	{ return inRgn + this->FromWindow(ZPoint::sZero); }

ZPoint ZSubPane::ToGlobal(const ZPoint& inPoint)
	{
	ZPoint newPoint = this->ToSuper(inPoint);
	if (fSuperPane)
		return fSuperPane->ToGlobal(newPoint);
	ZDebugStopf(1, ("Somebody's gotta overide ToGlobal(const ZPoint& inPoint)!"));
	return newPoint;
	}
ZPoint ZSubPane::FromGlobal(const ZPoint& inPoint)
	{ return inPoint - this->ToGlobal(ZPoint::sZero); }

ZRect ZSubPane::ToGlobal(const ZRect& inRect)
	{ return inRect + this->ToGlobal(ZPoint::sZero); }

ZRect ZSubPane::FromGlobal(const ZRect& inRect)
	{ return inRect + this->FromGlobal(ZPoint::sZero); }

ZDCRgn ZSubPane::ToGlobal(const ZDCRgn& inRgn)
	{ return inRgn + this->ToGlobal(ZPoint::sZero); }

ZDCRgn ZSubPane::FromGlobal(const ZDCRgn& inRgn)
	{ return inRgn + this->FromGlobal(ZPoint::sZero); }

// =================================================================================================
#pragma mark -
#pragma mark * ZSuperPane

ZSuperPane::ZSuperPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator)
:	ZSubPane(inSuperPane, inPaneLocator)
	{}

ZSuperPane::ZSuperPane(ZEventHr* inNextHandler, ZPaneLocator* inPaneLocator)
:	ZSubPane(inNextHandler, inPaneLocator)
	{}

ZSuperPane::~ZSuperPane()
	{
	while (fSubPanes.size())
		delete fSubPanes.back();
	}

void ZSuperPane::InsertTabTargets(vector<ZEventHr*>& inOutTargets)
	{
	ZSubPane::InsertTabTargets(inOutTargets);
	// Walk through our sub panes, asking them to insert their possible targets
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		fSubPanes[x]->InsertTabTargets(inOutTargets);
	}

ZSuperPane* ZSuperPane::GetOutermostSuperPane()
	{
	if (fSuperPane)
		return fSuperPane->GetOutermostSuperPane();
	return this;
	}

void ZSuperPane::Activate(bool inEntering)
	{
	ZSubPane::Activate(inEntering);

	for (size_t x = 0; x < fSubPanes.size(); ++x)
		fSubPanes[x]->Activate(inEntering);
	}

void ZSuperPane::HandleDraw(const ZDC& inDC)
	{
	ZDC localDC(inDC);

	// Fix up the pattern origin
	ZPoint translation(this->GetTranslation());
	localDC.OffsetPatternOrigin(translation);

	ZDCRgn oldClip = localDC.GetClip();
	ZDCRgn boundsRgn = this->CalcBoundsRgn();
	if (ZDCRgn newClip = oldClip & boundsRgn)
		{
		// SuperPanes draw their own content, their adorners and then their sub panes
		localDC.SetClip(newClip);

		if (ZDCInk backInk = this->GetBackInk(localDC))
			localDC.SetInk(backInk);

		this->DoDraw(localDC, boundsRgn);

		if (fAdorners)
			{
			for (size_t x = 0; x < fAdorners->size(); ++x)
				(*fAdorners)[x]->AdornPane(inDC, this, boundsRgn);
			}

		if (ZDCInk internalBackInk = this->GetInternalBackInk(localDC))
			localDC.SetInk(internalBackInk);

		for (size_t x = 0; x < fSubPanes.size(); ++x)
			{
			if (fSubPanes[x]->GetVisible())
				{
				// Here we modify the ZDC's coordinate system to match the pane's
				ZPoint coordOffset(fSubPanes[x]->GetLocation() - translation);
				localDC.OffsetOrigin(coordOffset);
				// And tell our sub pane to draw itself
				fSubPanes[x]->HandleDraw(localDC);
				// Restore the coordinate system
				localDC.OffsetOrigin(-coordOffset);
				}
			}
		}
	else
		{
		if (fAdorners)
			{
			for (size_t x = 0; x < fAdorners->size(); ++x)
				(*fAdorners)[x]->AdornPane(inDC, this, boundsRgn);
			}
		}
	}

void ZSuperPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	inDC.Fill(inDC.GetClip() - this->GetSubPaneRgn());
	}

ZSubPane* ZSuperPane::FindPane(ZPoint inPoint)
	{
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		{
		if (fSubPanes[x]->GetVisible() && fSubPanes[x]->CalcFrameRgn().Contains(inPoint))
			return fSubPanes[x]->FindPane(inPoint - fSubPanes[x]->GetLocation() + this->GetTranslation());
		}
	return ZSubPane::FindPane(inPoint);
	}

bool ZSuperPane::HandleClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (!this->GetEnabled())
		return false;
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		{
		if (fSubPanes[x]->GetVisible() && fSubPanes[x]->CalcFrameRgn().Contains(inHitPoint))
			{
			ZPoint coordOffset = fSubPanes[x]->GetLocation() - this->GetTranslation();
			// Patterns are offset by the translation only, so adjacent subpanes' patterns will align with one another
			if (fSubPanes[x]->HandleClick(inHitPoint - coordOffset, inEvent))
				return true;
			}
		}
	return ZSubPane::HandleClick(inHitPoint, inEvent);
	}

ZPoint ZSuperPane::GetCumulativeTranslation()
	{ return ZSubPane::GetCumulativeTranslation()+this->GetTranslation(); }

void ZSuperPane::HandleFramePreChange(ZPoint inWindowLocation)
	{
	ZSubPane::HandleFramePreChange(inWindowLocation);

	ZPoint myTranslation = this->GetTranslation();
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		fSubPanes[x]->HandleFramePreChange(inWindowLocation - myTranslation + fSubPanes[x]->GetLocation());
	}

ZDCRgn ZSuperPane::HandleFramePostChange(ZPoint inWindowLocation)
	{
	ZDCRgn theRgn = ZSubPane::HandleFramePostChange(inWindowLocation);
	ZPoint myTranslation = this->GetTranslation();
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		theRgn |= fSubPanes[x]->HandleFramePostChange(inWindowLocation - myTranslation + fSubPanes[x]->GetLocation());
	return theRgn;
	}

ZDCRgn ZSuperPane::DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds)
	{
	// We assume that most superpane's contents are *not* massively dependent on their boundaries.
	// Generally a super pane just paints its bounds a solid color, so we need only invalidate the
	// differences between the old and new bounds. If you have a superpane which (for example) draws
	// a line around its boundary then you should override this method and calculate the additional
	// area that needs invalidating.
	ZDCRgn theRgn;
	if (inOldBounds != inNewBounds)
		{
		theRgn = inOldBounds;
		theRgn ^= inNewBounds;
		}
	return theRgn;
	}

void ZSuperPane::ScrollPreChange()
	{
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		fSubPanes[x]->ScrollPreChange();
	}

void ZSuperPane::ScrollPostChange()
	{
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		fSubPanes[x]->ScrollPostChange();
	}


ZPoint ZSuperPane::GetTranslation()
	{
	ZPoint theTranslation;
	if (fPaneLocator && fPaneLocator->GetPaneTranslation(this, theTranslation))
		return theTranslation;
	return ZPoint::sZero;
	}

ZPoint ZSuperPane::GetInternalSize()
	{
	ZPoint thePoint;
	if (fPaneLocator && fPaneLocator->GetPaneInternalSize(this, thePoint))
		return thePoint;
	return this->GetSize();
	}

ZDCInk ZSuperPane::GetInternalBackInk(const ZDC& inDC)
	{
	// We'll augment this with a ZPaneLocator call at some point
	return ZDCInk();
	}

void ZSuperPane::SubPaneAdded(ZSubPane* inPane)
	{ fSubPanes.push_back(inPane); }

void ZSuperPane::SubPaneRemoved(ZSubPane* inPane)
	{ fSubPanes.erase(find(fSubPanes.begin(), fSubPanes.end(), inPane)); }

size_t ZSuperPane::GetSubPaneCount()
	{ return fSubPanes.size(); }

size_t ZSuperPane::GetSubPaneIndex(ZSubPane* inPane)
	{
	vector<ZSubPane*>::iterator iter = find(fSubPanes.begin(), fSubPanes.end(), inPane);
	if (iter == fSubPanes.end())
		return fSubPanes.size();
	return iter - fSubPanes.begin();
	}

ZSubPane* ZSuperPane::GetSubPaneAt(size_t inIndex)
	{
	if (inIndex >= fSubPanes.size())
		return nil;
	return fSubPanes[inIndex];
	}

ZDCRgn ZSuperPane::GetSubPaneRgn()
	{
	ZDCRgn theSubPaneRgn;
	ZPoint translation = this->GetTranslation();
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		{
		if (fSubPanes[x]->GetVisible())
			{
			ZPoint coordOffset = fSubPanes[x]->GetLocation() - translation;
			theSubPaneRgn |= fSubPanes[x]->CalcBoundsRgn() + coordOffset;
			}
		}
	return theSubPaneRgn;
	}

// =================================================================================================
