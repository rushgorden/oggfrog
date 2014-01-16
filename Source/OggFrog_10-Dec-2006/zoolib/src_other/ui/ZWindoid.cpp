static const char rcsid[] = "@(#) $Id: ZWindoid.cpp,v 1.13 2006/04/10 20:44:23 agreen Exp $";
/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#include "ZWindoid.h"
#include "ZMouseTracker.h"
#include "ZRegionAdorner.h"

#define ASSERTLOCKED()

#define kDebug_Windoid 3

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoidPane

class ZWindoidDragTracker : public ZMouseTracker
	{
public:
	ZWindoidDragTracker(ZWindoidPane* inWindoidPane, ZWindoid* inHitWindoid, ZPoint inHitPoint, bool inBringFront, bool inDragOutline);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	ZWindoidPane* fWindoidPane;
	ZWindoid* fWindoid;
	bool fBringFront;
	bool fDragOutline;
	ZRef<ZRegionAdorner> fAdorner;
	};

ZWindoidDragTracker::ZWindoidDragTracker(ZWindoidPane* inWindoidPane, ZWindoid* inHitWindoid, ZPoint inHitPoint, bool inBringFront, bool inDragOutline)
:	ZMouseTracker(inWindoidPane, inHitPoint),
	fWindoidPane(inWindoidPane),
	fWindoid(inHitWindoid),
	fBringFront(inBringFront),
	fDragOutline(inDragOutline)
	{}

void ZWindoidDragTracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZRect paneBounds = fWindoidPane->CalcBounds();
	inNext = paneBounds.Pin(inNext);
	inPrevious = paneBounds.Pin(inPrevious);
	if (fDragOutline)
		{
		switch (inPhase)
			{
			case ePhaseBegin:
				fAdorner = new ZRegionAdorner(ZDCPattern::sGray);
				fWindoidPane->InstallAdorner(fAdorner);
			case ePhaseContinue:
				{
				ZDC theDC = fWindoidPane->GetDC();
				ZDCRgn newRgn = fWindoid->GetStructureRgn() + (fWindoidPane->FromWindoid(fWindoid, ZPoint::sZero) + inNext - inAnchor);
				newRgn -= newRgn.Inset(2, 2);
				if (!fBringFront)
					newRgn -= fWindoidPane->CalcAboveWindoidRgn(fWindoid);
				fAdorner->SetRgn(theDC, newRgn);
				break;
				}
			case ePhaseEnd:
				{
				ZDC theDC = fWindoidPane->GetDC();
				fAdorner->SetRgn(theDC, ZDCRgn());
				fWindoidPane->RemoveAdorner(fAdorner);
				if (fBringFront)
					fWindoidPane->SetWindoidAttributes(fWindoid, nil, fWindoid->GetLocation() + inNext - inAnchor, fWindoid->fStructureRgn, fWindoid->fContentRgn, true);
				else
					fWindoid->SetLocation(fWindoid->GetLocation() + inNext - inAnchor);
				break;
				}
			}
		}
	else
		{
		switch (inPhase)
			{
			case ePhaseBegin:
				{
				if (fBringFront)
					fWindoid->BringFront(true);
				break;
				}
			case ePhaseContinue:
				{
				fWindoid->SetLocation(fWindoid->GetLocation() + inNext - inPrevious);
				fWindoidPane->Update();
				break;
				}
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoidResizeTracker

class ZWindoidResizeTracker : public ZMouseTracker
	{
public:
	ZWindoidResizeTracker(ZWindoidPane* inWindoidPane, ZWindoid* inHitWindoid, ZPoint inHitPoint, bool inBringFront, bool inDragOutline);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	ZWindoid* fWindoid;
	ZWindoidPane* fWindoidPane;
	ZPoint fHitPoint;
	ZPoint fInitialSize;
	bool fDragOutline;
	ZRef<ZRegionAdorner> fAdorner;
	};

ZWindoidResizeTracker::ZWindoidResizeTracker(ZWindoidPane* inWindoidPane, ZWindoid* inHitWindoid, ZPoint inHitPoint, bool inBringFront, bool inDragOutline)
:	ZMouseTracker(inWindoidPane, inHitPoint),
	fWindoidPane(inWindoidPane),
	fWindoid(inHitWindoid),
	fDragOutline(inDragOutline)
	{
	fInitialSize = fWindoid->GetSize();
	if (inBringFront)
		fWindoid->BringFront(true);
	}

void ZWindoidResizeTracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZRect paneBounds = fWindoidPane->CalcBounds();
	inNext = paneBounds.Pin(inNext);
	inPrevious = paneBounds.Pin(inPrevious);

	ZPoint newSize = fWindoid->ConstrainSize(fInitialSize + inNext - inAnchor);

	if (fDragOutline)
		{
		switch (inPhase)
			{
			case ePhaseBegin:
				{
				fAdorner = new ZRegionAdorner(ZDCPattern::sGray);
				fWindoidPane->InstallAdorner(fAdorner);
				}
			case ePhaseContinue:
				{
				ZDC theDC = fWindoidPane->GetDC();
				ZDCRgn newRgn = ZRect(newSize) + fWindoidPane->FromWindoid(fWindoid, ZPoint::sZero);
				newRgn -= newRgn.Inset(2, 2);
				fAdorner->SetRgn(theDC, newRgn);
				}
				break;
			case ePhaseEnd:
				{
				ZDC theDC = fWindoidPane->GetDC();
				fAdorner->SetRgn(theDC, ZDCRgn());
				fWindoidPane->RemoveAdorner(fAdorner);
				fWindoid->SetSize(newSize);
				}
				break;
			}
		}
	else
		{
		switch (inPhase)
			{
			case ePhaseContinue:
				{
				fWindoid->SetSize(newSize);
				fWindoidPane->Update();
				}
				break;
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoidPaneCaptureInput

class ZWindoidPaneCaptureInput : public ZCaptureInput
	{
public:
	ZWindoidPaneCaptureInput(ZWindoidPane* inWindoidPane);
	virtual ~ZWindoidPaneCaptureInput();

	virtual void Installed();
	virtual void Removed();
	virtual void Cancelled();

	virtual bool HandleMessage(const ZMessage& inMessage);
//	virtual bool GetCursor(ZPoint inPanePoint, ZCursor& outCursor);

protected:
	ZWindoidPane* fWindoidPane;
	};

ZWindoidPaneCaptureInput::ZWindoidPaneCaptureInput(ZWindoidPane* inWindoidPane)
:	ZCaptureInput(inWindoidPane), fWindoidPane(inWindoidPane)
	{
	ZDebugLogf(kDebug_Windoid, ("ZWindoidPaneCaptureInput, %08X", (int)this));
	}

ZWindoidPaneCaptureInput::~ZWindoidPaneCaptureInput()
	{
	ZDebugLogf(kDebug_Windoid, ("~ZWindoidPaneCaptureInput, %08X", (int)this));
	}

void ZWindoidPaneCaptureInput::Installed()
	{
	ZDebugLogf(kDebug_Windoid, ("Installed, %08X", (int)this));
	}

void ZWindoidPaneCaptureInput::Removed()
	{
	ZDebugLogf(kDebug_Windoid, ("Removed, %08X", (int)this));
	}

void ZWindoidPaneCaptureInput::Cancelled()
	{
	ZDebugLogf(kDebug_Windoid, ("Cancelled, %08X", (int)this));
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:CaptureCancelled");
	fWindoidPane->HandleCaptureMessage(theMessage);
	}

bool ZWindoidPaneCaptureInput::HandleMessage(const ZMessage& inMessage)
	{
	ZDebugLogf(kDebug_Windoid, ("HandleMessage, %08X", (int)this));
	fWindoidPane->HandleCaptureMessage(inMessage);
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoidPane

ZWindoidPane::ZWindoidPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZMessageLooper* inLooper)
:	ZSuperPane(inSuperPane, inPaneLocator),
	ZMessageReceiver(inLooper)
	{
	this->GetWindow()->RegisterIdlePane(this);
	fWindoid_Top = nil;
	fWindoid_Bottom = nil;
	fWindoid_Capture = nil;
	fCaptureInput = nil;
	fOrigin = ZPoint(7, 12);
	}

ZWindoidPane::~ZWindoidPane()
	{
	while (fWindoid_Top)
		{
		fWindoid_Top->GetLock().Acquire();
		fWindoid_Top->GetOwner()->OSWindowMustDispose(fWindoid_Top);
		}

	this->GetWindow()->UnregisterIdlePane(this);
	}

void ZWindoidPane::Activate(bool entering)
	{
	this->Internal_FixupActive();
	}

void ZWindoidPane::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZDC localDC(inDC);
//	ZDC_OffAuto localDC(inDC, false);

	ZDCRgn originalClip = localDC.GetClip();
	ZDCInk originalInk = localDC.GetInk();
	// Draw the structure parts of our windoids first
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		if (iterWindoid->fShown)
			{
			ZDCRgn lastClip = localDC.GetClip();
			ZPoint location = iterWindoid->fLocation - fOrigin;
			ZDCRgn usedRgn = iterWindoid->fStructureRgn + location;
			if (ZDCRgn currentClip = lastClip & usedRgn)
				{
				localDC.SetClip(currentClip);

				localDC.OffsetOrigin(location);
				localDC.OffsetPatternOrigin(location);

				iterWindoid->DrawStructure(localDC, iterWindoid->GetActive(), false);

				localDC.OffsetPatternOrigin(-location);
				localDC.OffsetOrigin(-location);

				localDC.SetClip(lastClip - usedRgn);
				}
			}
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	// Paint whatever is left
	localDC.SetInk(originalInk);
	localDC.Fill(localDC.GetClip());

	// Finally paint the contents of each windoid. Note that we walk down the list getting the region for each
	// windoid and seeing if it intersects what we need to draw. If it does we draw the windoid, and start
	// again from the top of the list -- the windoid is free to delete itself or change its stacking order
	// when we call its DrawContent method, and so we can't just keep walking the list. In any case, whenever
	// we see a windoid we remove its structure region from the region left to paint. Any invalidation
	// triggered by a windoid as a result of the call to DrawContent (there shouldn't be any of course) will
	// get picked up the next time our DoDraw method is called.
	while (true)
		{
		bool hitOne = false;
		iterWindoid = fWindoid_Top;
		while (iterWindoid)
			{
			if (iterWindoid->fShown)
				{
				ZPoint location = iterWindoid->fLocation - fOrigin;
				ZDCRgn iterContentRgn = iterWindoid->fContentRgn + location;
				if (ZDCRgn overlappingRgn = originalClip & iterContentRgn)
					{
					localDC.SetClip(overlappingRgn);

					localDC.OffsetOrigin(location);
					localDC.OffsetPatternOrigin(-location);

					iterWindoid->DrawContent(localDC);

					localDC.OffsetPatternOrigin(location);
					localDC.OffsetOrigin(-location);

					hitOne = true;
					}
				originalClip -= iterWindoid->fStructureRgn + location;
				}
			if (hitOne)
				break;
			iterWindoid = iterWindoid->fWindoid_Below;
			}
		if (!hitOne)
			break;
		}
	}

bool ZWindoidPane::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZWindoid* hitWindoid = this->Internal_FindWindoid(inHitPoint))
		{
		hitWindoid->Internal_HandleClick(inEvent.GetMessage());
		return true;
		}
	this->GetWindow()->BringFront();
	return true;
	}

bool ZWindoidPane::DoKeyDown(const ZEvent_Key& inEvent)
	{
	if (fWindoid_Top)
		fWindoid_Top->DispatchMessageToOwner(inEvent.GetMessage());
	return true;
	}

void ZWindoidPane::DoIdle()
	{
	ZSuperPane::DoIdle();

	ZMessage idleMessage;
	idleMessage.SetString("what", "zoolib:Idle");

	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		/// \todo We need to use a safe iterator -- windoids can dispose when they're idled.
		iterWindoid->DispatchMessageToOwner(idleMessage);
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	}

void ZWindoidPane::HandleFramePreChange(ZPoint inWindowLocation)
	{
	ZSuperPane::HandleFramePreChange(inWindowLocation);

	ZMessage preChangeMessage;
	preChangeMessage.SetString("what", "zoolib:PreVisChange");
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		iterWindoid->DispatchMessageToOwner(preChangeMessage);
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	}

ZDCRgn ZWindoidPane::HandleFramePostChange(ZPoint inWindowLocation)
	{
	ZMessage postChangeMessage;
	postChangeMessage.SetString("what", "zoolib:PostVisChange");
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		iterWindoid->DispatchMessageToOwner(postChangeMessage);
		iterWindoid = iterWindoid->fWindoid_Below;
		}

	return ZSuperPane::HandleFramePostChange(inWindowLocation);
	}

void ZWindoidPane::ScrollPreChange()
	{
	ZSuperPane::ScrollPreChange();

	ZMessage preChangeMessage;
	preChangeMessage.SetString("what", "zoolib:PreVisChange");
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		iterWindoid->DispatchMessageToOwner(preChangeMessage);
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	}

void ZWindoidPane::ScrollPostChange()
	{
	ZMessage postChangeMessage;
	postChangeMessage.SetString("what", "zoolib:PostVisChange");
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		iterWindoid->DispatchMessageToOwner(postChangeMessage);
		iterWindoid = iterWindoid->fWindoid_Below;
		}

	return ZSuperPane::ScrollPostChange();
	}

void ZWindoidPane::ReceivedMessage(const ZMessage& iMessage)
	{
	string theWhat = iMessage.GetString("what");
	if (theWhat == "ZWindoid:Message")
		{
		static_cast<ZWindoid*>(iMessage.GetPointer("windoid"))->DispatchMessage(ZMessage(iMessage.GetTuple("message")));
		return;
		}
	}

void ZWindoidPane::InvalidateWindoidWindowRgn(ZWindoid* inWindoid, const ZDCRgn& inRgn)
	{
	// Invalidate regions are passed to a ZOSWindow, and thus to inWindoid in *Window* coordinates, the
	// coords of the real underlying window, so we have to take the inRgn back to our pane's
	// coordinate system, then we can clip it against this windoid's structure and that of
	// any overlying windoids, before making a pane level call to do the real invalidation.
	if (inWindoid->fShown)
		{
		ZDCRgn realRgn = this->FromWindow(inRgn);
		realRgn &= inWindoid->fContentRgn + (inWindoid->fLocation - fOrigin);
		if (realRgn)
			this->Invalidate(realRgn - this->CalcAboveWindoidRgn(inWindoid));
		}
	}

void ZWindoidPane::InvalidateWindoidRgn(ZWindoid* inWindoid, const ZDCRgn& inRgn)
	{
	if (inWindoid->fShown)
		{
		ZDCRgn realRgn = (inRgn & inWindoid->fStructureRgn) + (inWindoid->fLocation - fOrigin);
		this->Invalidate(realRgn - this->CalcAboveWindoidRgn(inWindoid));
		}
	}

ZDCRgn ZWindoidPane::GetWindoidVisibleContentRgn(ZWindoid* inWindoid)
	{
	ZDCRgn resultRgn;
	if (inWindoid->fShown)
		{
		resultRgn = inWindoid->fContentRgn + (inWindoid->fLocation - fOrigin);
		resultRgn &= this->CalcApertureRgn();
		resultRgn -= this->CalcAboveWindoidRgn(inWindoid);
		resultRgn = this->ToWindow(resultRgn);
		}
	return resultRgn;
	}

ZDCRgn ZWindoidPane::CalcAboveWindoidRgn(ZWindoid* inWindoid)
	{
	ZDCRgn resultRgn;
	ZWindoid* iterWindoid = inWindoid->fWindoid_Above;
	while (iterWindoid)
		{
		if (iterWindoid->fShown)
			resultRgn |= iterWindoid->fStructureRgn + (iterWindoid->fLocation - fOrigin);
		iterWindoid = iterWindoid->fWindoid_Above;
		}
	return resultRgn;
	}

static void sFlash(ZDC& inDC, const ZDCRgn& inRgn)
	{
	if (inRgn)
		{
		inDC.Invert(inRgn);
		ZThread::sSleep(150);
		inDC.Invert(inRgn);
		}
	}

void ZWindoidPane::SetWindoidAttributes(ZWindoid* inWindoid, ZWindoid* inBehindWindoid, ZPoint inLocation, const ZDCRgn& inStructureRgn, const ZDCRgn& inContentRgn, bool inShown)
	{
	ZAssertStop(0, inWindoid != inBehindWindoid);

	inLocation -= fOrigin;

	bool priorShown = inWindoid->fShown;
	ZPoint priorLocation = inWindoid->fLocation - fOrigin;
	ZWindoid* priorBehindWindoid = inWindoid->fWindoid_Above;
	ZDCRgn priorEffectiveStructureRgn;
	ZDCRgn priorEffectiveContentRgn;
	if (priorShown)
		{
		priorEffectiveStructureRgn = inWindoid->fStructureRgn;
		priorEffectiveContentRgn = inWindoid->fContentRgn;
		}

	ZDCRgn afterEffectiveStructureRgn;
	ZDCRgn afterEffectiveContentRgn;
	if (inShown)
		{
		afterEffectiveStructureRgn = inStructureRgn;
		afterEffectiveContentRgn = inContentRgn;
		}

	bool effectiveStructureRgnChanged = priorEffectiveStructureRgn != afterEffectiveStructureRgn;
	bool effectiveContentRgnChanged = priorEffectiveContentRgn != afterEffectiveContentRgn;

	// Unlink the windoid
	if (inWindoid->fWindoid_Below)
		inWindoid->fWindoid_Below->fWindoid_Above = inWindoid->fWindoid_Above;
	if (inWindoid->fWindoid_Above)
		inWindoid->fWindoid_Above->fWindoid_Below = inWindoid->fWindoid_Below;
	if (fWindoid_Top == inWindoid)
		fWindoid_Top = inWindoid->fWindoid_Below;
	if (fWindoid_Bottom == inWindoid)
		fWindoid_Bottom = inWindoid->fWindoid_Above;
	inWindoid->fWindoid_Above = nil;
	inWindoid->fWindoid_Below = nil;

	// Intersecting windoids below that will have a changed visible region.
	vector<ZWindoid*> affectedWindoids;

	// Identify the windoids that will have their vis rgn affected by these changes, and
	// accumulate the regions of windoids above inWindoid before and after the changes.
	ZDCRgn priorAboveRgn;
	ZDCRgn afterAboveRgn;
	bool priorHit = (priorBehindWindoid == nil);
	bool afterHit = (inBehindWindoid == nil);
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		if (iterWindoid->fShown)
			{
			ZDCRgn iterRgn = iterWindoid->fStructureRgn + (iterWindoid->fLocation - fOrigin);
			if (priorHit || afterHit)
				{
				ZDCRgn priorObscuredRgn;
				if (priorHit)
					priorObscuredRgn = iterRgn & (priorEffectiveStructureRgn + priorLocation);
				ZDCRgn afterObscuredRgn;
				if (afterHit)
					afterObscuredRgn = iterRgn & (afterEffectiveStructureRgn + inLocation);
				if (priorObscuredRgn != afterObscuredRgn)
					affectedWindoids.push_back(iterWindoid);
				}
			if (!priorHit)
				priorAboveRgn |= iterRgn;
			if (!afterHit)
				afterAboveRgn |= iterRgn;
			}
		if (iterWindoid == priorBehindWindoid)
			priorHit = true;
		if (iterWindoid == inBehindWindoid)
			afterHit = true;
		iterWindoid = iterWindoid->fWindoid_Below;
		}

	ZDC paneDC = this->GetDC();
	ZDCRgn apertureRgn = paneDC.GetClip();
	ZDCRgn priorVisRgn = (priorEffectiveStructureRgn - (priorAboveRgn - priorLocation)) & (apertureRgn - priorLocation);
	ZDCRgn afterVisRgn = (afterEffectiveStructureRgn - (afterAboveRgn - inLocation)) & (apertureRgn - inLocation);

	if (priorLocation != inLocation || effectiveStructureRgnChanged || effectiveContentRgnChanged || priorVisRgn != afterVisRgn)
		affectedWindoids.push_back(inWindoid);

	// Run through affectedWindoids and let them know that their vis region is going to be changing
	ZMessage preChangeMessage;
	preChangeMessage.SetString("what", "zoolib:PreVisChange");
	for (size_t x = 0; x < affectedWindoids.size(); ++x)
		affectedWindoids[x]->DispatchMessageToOwner(preChangeMessage);
	ZDCRgn invalRgn;
	if (priorLocation != inLocation)
		{
		// We're moving the windoid, blit whatever we can. The region we can move
		// is the intersection of what was and what will be visible.
		ZDCRgn movableRgn = priorVisRgn & afterVisRgn;
		if (movableRgn)
			{
			ZDC moveDC(paneDC);
			moveDC.SetClip(moveDC.GetClip() & (movableRgn + inLocation));
			ZRect movableBounds = movableRgn.Bounds();
			moveDC.CopyFrom(movableBounds.TopLeft() + inLocation, moveDC, movableBounds + priorLocation);
			}
		// We need to invalidate the whole of the old and new vis rgns, less whatever we were
		// able to move to the destination.
		invalRgn = ((priorVisRgn + priorLocation) | (afterVisRgn + inLocation)) - (movableRgn + inLocation);
		}
	else
		{
		// We're not moving the windoid, so the inval rgn is the difference of the old and new rgns.
		invalRgn = (priorVisRgn + priorLocation) ^ (afterVisRgn + inLocation);
		}

	if (effectiveStructureRgnChanged)
		{
		// The structure rgn changed, add in the structure less the content, both before and after.
		ZDCRgn changedRgn = ((priorEffectiveStructureRgn - priorEffectiveContentRgn) + priorLocation) - priorAboveRgn;
		changedRgn |= ((afterEffectiveStructureRgn - afterEffectiveContentRgn) + inLocation) - afterAboveRgn;
		// Clip down to our aperture
		invalRgn |= (changedRgn & apertureRgn);
		}

	// Link inWindoid back into place
	if (inBehindWindoid)
		{
		inWindoid->fWindoid_Below = inBehindWindoid->fWindoid_Below;
		if (inWindoid->fWindoid_Below)
			inWindoid->fWindoid_Below->fWindoid_Above = inWindoid;
		else
			fWindoid_Bottom = inWindoid;
		inWindoid->fWindoid_Above = inBehindWindoid;
		inBehindWindoid->fWindoid_Below = inWindoid;
		}
	else
		{
		inWindoid->fWindoid_Below = fWindoid_Top;
		if (fWindoid_Top)
			fWindoid_Top->fWindoid_Above = inWindoid;
		fWindoid_Top = inWindoid;
		}

	// Set its location, shown flag and regions
	inWindoid->fLocation = inLocation + fOrigin;
	inWindoid->fShown = inShown;
	inWindoid->fStructureRgn = inStructureRgn;
	inWindoid->fContentRgn = inContentRgn;

	// Walk through all the windoids, redrawing any structure region that intersects invalRgn, and removing
	// it from invalRgn.
	ZDCRgn clipRgn = invalRgn;
	iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		if (iterWindoid->fShown)
			{
			ZPoint location = iterWindoid->fLocation - fOrigin;
			ZDCRgn iterStructureRgn = iterWindoid->fStructureRgn + location;
			if (ZDCRgn currentClip = clipRgn & iterStructureRgn)
				{
				paneDC.SetClip(currentClip);

				paneDC.OffsetOrigin(location);
				paneDC.OffsetPatternOrigin(-location);

				iterWindoid->DrawStructure(paneDC, iterWindoid->GetActive(), false);

				paneDC.OffsetPatternOrigin(location);
				paneDC.OffsetOrigin(-location);

				invalRgn -= (currentClip - (iterWindoid->fContentRgn + location));
				clipRgn -= currentClip;
				}
			}
		iterWindoid = iterWindoid->fWindoid_Below;
		}

	// Invalidate invalRgn
	this->Invalidate(invalRgn);

	// Run through affectedWindoids and let them know that their vis region is okay again
	ZMessage postChangeMessage;
	postChangeMessage.SetString("what", "zoolib:PostVisChange");
	for (size_t x = 0; x < affectedWindoids.size(); ++x)
		affectedWindoids[x]->DispatchMessageToOwner(postChangeMessage);
	this->Internal_FixupActive();

	if (priorLocation != inLocation || effectiveStructureRgnChanged || effectiveContentRgnChanged)
		{
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:FrameChange");
		inWindoid->DispatchMessageToOwner(theMessage);
		}
	}

ZPoint ZWindoidPane::ToWindoid(ZWindoid* inWindoid, ZPoint inPanePoint)
	{
	return inPanePoint - (inWindoid->fLocation - fOrigin);
	}

ZPoint ZWindoidPane::FromWindoid(ZWindoid* inWindoid, ZPoint inWindoidPoint)
	{
	return inWindoidPoint + (inWindoid->fLocation - fOrigin);
	}

void ZWindoidPane::Internal_AddWindoid(ZWindoid* inWindoid)
	{
	ZAssertLocked(kDebug_Windoid, this->GetLock());

	inWindoid->fWindoid_Above = nil;

	inWindoid->fWindoid_Below = fWindoid_Top;
	if (fWindoid_Top)
		fWindoid_Top->fWindoid_Above = inWindoid;
	fWindoid_Top = inWindoid;

	if (fWindoid_Bottom == nil)
		fWindoid_Bottom = inWindoid;
	}

void ZWindoidPane::Internal_RemoveWindoid(ZWindoid* inWindoid)
	{
	ZAssertLocked(kDebug_Windoid, this->GetLock());
	if (inWindoid->fWindoid_Below)
		inWindoid->fWindoid_Below->fWindoid_Above = inWindoid->fWindoid_Above;
	if (inWindoid->fWindoid_Above)
		inWindoid->fWindoid_Above->fWindoid_Below = inWindoid->fWindoid_Below;
	if (fWindoid_Top == inWindoid)
		fWindoid_Top = inWindoid->fWindoid_Below;
	if (fWindoid_Bottom == inWindoid)
		fWindoid_Bottom = inWindoid->fWindoid_Above;
	}

void ZWindoidPane::Internal_FixupActive()
	{
	bool paneActive = this->GetActive();
	bool seenShownWindoid = false;
	ZDC paneDC = this->GetDC();
	ZDCRgn clipRgn = paneDC.GetClip();
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		if (iterWindoid->fShown)
			{
			ZPoint location = iterWindoid->fLocation - fOrigin;
			ZDCRgn iterStructureRgn = iterWindoid->fStructureRgn + location;
			bool neededActive = !seenShownWindoid && paneActive;// && iterWindoid == fWindoid_Top;
			if (iterWindoid->fActive != neededActive)
				{
				iterWindoid->fActive = neededActive;
				if (ZDCRgn currentClip = clipRgn & iterStructureRgn)
					{
					paneDC.SetClip(currentClip);

					paneDC.OffsetOrigin(location);
					paneDC.OffsetPatternOrigin(-location);

					iterWindoid->DrawStructure(paneDC, iterWindoid->GetActive(), false);

					paneDC.OffsetPatternOrigin(location);
					paneDC.OffsetOrigin(-location);
					}
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:Activate");
				theMessage.SetBool("active", neededActive);
				iterWindoid->DispatchMessageToOwner(theMessage);
				}
			seenShownWindoid = true;
			clipRgn -= iterStructureRgn;
			}
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	}

ZWindoid* ZWindoidPane::Internal_FindWindoid(ZPoint inPanePoint)
	{
	ZWindoid* iterWindoid = fWindoid_Top;
	while (iterWindoid)
		{
		if (iterWindoid->fShown && iterWindoid->fStructureRgn.Contains(inPanePoint - (iterWindoid->fLocation - fOrigin)))
			return iterWindoid;
		iterWindoid = iterWindoid->fWindoid_Below;
		}
	return nil;
	}

void ZWindoidPane::QueueMessageForWindoid(ZWindoid* inWindoid, const ZMessage& inMessage)
	{
	ZMessage envelope;
	envelope.SetString("what", "ZWindoid:Message");
	envelope.SetPointer("windoid", inWindoid);
	envelope.SetTuple("message", inMessage.AsTuple());
	this->PostMessage(envelope);
	}

void ZWindoidPane::AcquireCapture(ZWindoid* inWindoid)
	{
	if (!fCaptureInput)
		{
		fCaptureInput = new ZWindoidPaneCaptureInput(this);
		fCaptureInput->Install();
		}

	if (fWindoid_Capture != inWindoid)
		{
		ZWindoid* oldWindoidCapture = fWindoid_Capture;
		fWindoid_Capture = inWindoid;
		if (oldWindoidCapture)
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:CaptureCancelled");
			oldWindoidCapture->DispatchMessageToOwner(theMessage);
			}
		}
	}

void ZWindoidPane::ReleaseCapture(ZWindoid* inWindoid)
	{
	if (fWindoid_Capture && fWindoid_Capture == inWindoid)
		{
		if (ZWindoidPaneCaptureInput* theCaptureInput = fCaptureInput)
			{
			fCaptureInput = nil;
			theCaptureInput->Finish();
			}
		fWindoid_Capture = nil;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:CaptureReleased");
		inWindoid->DispatchMessageToOwner(theMessage);
		}
	}

void ZWindoidPane::HandleCaptureMessage(const ZMessage& inMessage)
	{
	if (fWindoid_Capture)
		fWindoid_Capture->DispatchMessageToOwner(inMessage);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindoid

ZWindoid::ZWindoid(ZWindoidPane* inWindoidPane)
	{
	fWindoidPane = inWindoidPane;
	fShown = false;
	fActive = false;
	fLocation = ZPoint::sZero;
	fLayer = 0;

	fWindoid_Above = nil;
	fWindoid_Below = nil;
	fWindoidPane->Internal_AddWindoid(this);

	// Reacquire the windoid pane's lock, because our creator will be releasing our window lock
	// which _is_ our pane's lock.
	fWindoidPane->GetLock().Acquire();
	}

ZWindoid::~ZWindoid()
	{
	fWindoidPane->Internal_RemoveWindoid(this);
	fWindoidPane->GetLock().Release();
	}

ZMutexBase& ZWindoid::GetLock()
	{
	return fWindoidPane->GetLock();
	}

void ZWindoid::QueueMessage(const ZMessage& iMessage)
	{
	fWindoidPane->QueueMessageForWindoid(this, iMessage);
	}

void ZWindoid::SetShownState(ZShownState inState)
	{
	ASSERTLOCKED();
	bool newShown = inState != eZShownStateHidden;
	if (newShown != fShown)
		fWindoidPane->SetWindoidAttributes(this, fWindoid_Above, fLocation, fStructureRgn, fContentRgn, newShown);
	}

ZShownState ZWindoid::GetShownState()
	{
	ASSERTLOCKED();
	return fShown ? eZShownStateShown : eZShownStateHidden;
	}

void ZWindoid::SetTitle(const string& inTitle)
	{
	ASSERTLOCKED();
	}

string ZWindoid::GetTitle()
	{
	ASSERTLOCKED();
	return string();
	}

void ZWindoid::SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap)
	{
	ASSERTLOCKED();
	}

ZCoord ZWindoid::GetTitleIconPreferredHeight()
	{
	ASSERTLOCKED();
	return 0;
	}

void ZWindoid::SetCursor(const ZCursor& inCursor)
	{
	ASSERTLOCKED();
	}

void ZWindoid::SetLocation(ZPoint inLocation)
	{
	ASSERTLOCKED();
	if (fLocation == inLocation)
		return;
	fWindoidPane->SetWindoidAttributes(this, fWindoid_Above, inLocation, fStructureRgn, fContentRgn, fShown);
	}

ZPoint ZWindoid::GetLocation()
	{
	ASSERTLOCKED();
	return fLocation;
	}

void ZWindoid::PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists)
	{
	ASSERTLOCKED();
	ZUnimplemented();
	}

void ZWindoid::EndPoseModal()
	{
	ASSERTLOCKED();
	ZUnimplemented();
	}

bool ZWindoid::WaitingForPoseModal()
	{
	ASSERTLOCKED();
	return false;
	}

void ZWindoid::BringFront(bool inMakeVisible)
	{
	ASSERTLOCKED();
	bool wasShown = fShown;
	bool showIt = fShown || inMakeVisible;
	fWindoidPane->SetWindoidAttributes(this, nil, fLocation, fStructureRgn, fContentRgn, showIt);
	if (inMakeVisible)
		fWindoidPane->GetWindow()->BringFront(false);
	}

bool ZWindoid::GetActive()
	{
	ASSERTLOCKED();
	return fActive;
	}

void ZWindoid::GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	ASSERTLOCKED();
	outTopLeftInset = fContentRgn.Bounds().TopLeft() - fStructureRgn.Bounds().TopLeft();
	outTopLeftInset = fContentRgn.Bounds().BottomRight() - fStructureRgn.Bounds().BottomRight();
	}

ZPoint ZWindoid::GetOrigin()
	{
	ASSERTLOCKED();
	return fWindoidPane->ToWindow(fWindoidPane->FromWindoid(this, ZPoint::sZero));
	}

bool ZWindoid::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();
	return false;
	}

ZDCRgn ZWindoid::GetVisibleContentRgn()
	{
	ASSERTLOCKED();
	return fWindoidPane->GetWindoidVisibleContentRgn(this);
	}

ZPoint ZWindoid::ToGlobal(const ZPoint& inWindowPoint)
	{
	ASSERTLOCKED();
	return fWindoidPane->ToGlobal(fWindoidPane->FromWindow(inWindowPoint));
	}

void ZWindoid::InvalidateRegion(const ZDCRgn& inBadRgn)
	{
	ASSERTLOCKED();
	fWindoidPane->InvalidateWindoidWindowRgn(this, inBadRgn);
	}

void ZWindoid::UpdateNow()
	{
	ASSERTLOCKED();
	fWindoidPane->Update();
	}

ZRef<ZDCCanvas> ZWindoid::GetCanvas()
	{
	ASSERTLOCKED();
	return fWindoidPane->GetWindow()->GetWindowCanvas();
	}

void ZWindoid::AcquireCapture()
	{
	ASSERTLOCKED();
	fWindoidPane->AcquireCapture(this);
	}

void ZWindoid::ReleaseCapture()
	{
	ASSERTLOCKED();
	fWindoidPane->ReleaseCapture(this);
	}

void ZWindoid::GetNative(void* outNative)
	{
	ASSERTLOCKED();
	fWindoidPane->GetWindow()->GetWindowNative(outNative);
	}

void ZWindoid::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	}

string ZWindoid::FindHitPart(ZPoint inWindoidPoint)
	{
	return "content";
	}

void ZWindoid::HandleClick(ZPoint inWindoidPoint, const string& inHitPart, const ZMessage& inMessage)
	{
	if (inHitPart == "drag")
		{
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
		bool bringFront = !inMessage.GetBool("command");
		bool dragOutline = inMessage.GetBool("option");
#else
		bool bringFront = !inMessage.GetBool("control");
		bool dragOutline = inMessage.GetBool("shift");
#endif
		(new ZWindoidDragTracker(fWindoidPane, this, fWindoidPane->FromWindow(inMessage.GetPoint("where")), bringFront, dragOutline))->Install();
		}
	else if (inHitPart == "close")
		{
		}
	else if (inHitPart == "zoom")
		{
		}
	else if (inHitPart == "size")
		{
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
		bool bringFront = !inMessage.GetBool("command");
		bool dragOutline = inMessage.GetBool("option");
#else
		bool bringFront = !inMessage.GetBool("control");
		bool dragOutline = inMessage.GetBool("shift");
#endif
		(new ZWindoidResizeTracker(fWindoidPane, this, fWindoidPane->FromWindow(inMessage.GetPoint("where")), bringFront, dragOutline))->Install();
		}
	else if (inHitPart == "content")
		{
		this->DispatchMessageToOwner(inMessage);
		}
	else
		{
		this->DispatchMessageToOwner(inMessage);
		}
	}

void ZWindoid::SetRegions(const ZDCRgn& inStructure, const ZDCRgn& inContent)
	{
	fWindoidPane->SetWindoidAttributes(this, fWindoid_Above, fLocation, inStructure, inContent, fShown);
	}

void ZWindoid::SetRegions(ZPoint inLocation, const ZDCRgn& inStructure, const ZDCRgn& inContent)
	{
	fWindoidPane->SetWindoidAttributes(this, fWindoid_Above, inLocation, inStructure, inContent, fShown);
	}

void ZWindoid::Internal_HandleClick(const ZMessage& inMessage)
	{
	ZPoint windoidPoint = fWindoidPane->ToWindoid(this, fWindoidPane->FromWindow(ZPoint(inMessage.GetPoint("where"))));
	string hitPart = this->FindHitPart(windoidPoint);
	this->HandleClick(windoidPoint, hitPart, inMessage);
	}
