static const char rcsid[] = "@(#) $Id: ZUIRendered.cpp,v 1.7 2006/07/30 21:13:59 agreen Exp $";

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

#include "ZUIRendered.h"
#include "ZTicks.h"
#include "ZDC.h"
#include "ZRegionAdorner.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane

ZUIRenderer_CaptionPane::ZUIRenderer_CaptionPane()
	{}

ZUIRenderer_CaptionPane::~ZUIRenderer_CaptionPane()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionPane_Rendered

ZUICaptionPane_Rendered::ZUICaptionPane_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption, ZRef<ZUIRenderer_CaptionPane> inRenderer)
:	ZUICaptionPane(inSuperPane, inLocator, inCaption), fRenderer(inRenderer)
	{}

ZUICaptionPane_Rendered::~ZUICaptionPane_Rendered()
	{}

void ZUICaptionPane_Rendered::Activate(bool entering)
	{
	ZUICaptionPane::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUICaptionPane_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer && fCaption)
		{
		ZDC localDC(inDC);
		if (fCaption->DrawsEntireRgn())
			localDC.SetClip(localDC.GetClip() - fCaption->GetRgn());
		ZSubPane::DoDraw(localDC, inBoundsRgn);
		ZUIDisplayState theDisplayState(false, this->GetHilite(), this->GetReallyEnabled(), false, this->GetActive());
		fRenderer->Draw(this, inDC, ZPoint::sZero, fCaption, theDisplayState);
		}
	else
		{
		ZUICaptionPane::DoDraw(inDC, inBoundsRgn);
		}
	}

ZPoint ZUICaptionPane_Rendered::GetPreferredSize()
	{
	if (fRenderer)
		return fRenderer->GetSize(this, fCaption);
	return ZPoint::sZero;
	}

ZRef<ZUIRenderer_CaptionPane> ZUICaptionPane_Rendered::GetRenderer()
	{ return fRenderer; }

void ZUICaptionPane_Rendered::SetRenderer(ZRef<ZUIRenderer_CaptionPane> inRenderer, bool inRefresh)
	{
	if (fRenderer == inRenderer)
		return;
	if (inRefresh)
		this->Refresh();
	fRenderer = inRenderer;
	if (inRefresh)
		this->Refresh();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button

ZUIRenderer_Button::ZUIRenderer_Button()
	{}

ZUIRenderer_Button::~ZUIRenderer_Button()
	{}

ZPoint ZUIRenderer_Button::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return ZPoint::sZero; }

ZPoint ZUIRenderer_Button::GetPopupLocation(ZUIButton_Rendered* inButton)
	{
	bool hasPopupGlyph;
	if (!inButton->GetAttribute("ZUIButton::HasPopupArrow", nil, &hasPopupGlyph))
		hasPopupGlyph = false;

	if (hasPopupGlyph)
		{
		bool popupRightFacing;
		if (!inButton->GetAttribute("ZUIButton::PopupRightFacing", nil, &popupRightFacing))
			popupRightFacing = false;
		if (popupRightFacing)
			return inButton->CalcBounds().TopRight();
		return inButton->CalcBounds().BottomLeft();
		}
	return ZPoint::sZero;
	}

void ZUIRenderer_Button::GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	outTopLeftInset = ZPoint::sZero;
	outBottomRightInset = ZPoint::sZero;
	}

bool ZUIRenderer_Button::ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton)
	{ return false; }

void ZUIRenderer_Button::ButtonBecameWindowTarget(ZUIButton_Rendered* inButton)
	{}

void ZUIRenderer_Button::ButtonResignedWindowTarget(ZUIButton_Rendered* inButton)
	{}
bool ZUIRenderer_Button::ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent)
	{ return false; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton_Rendered

ZUIButton_Rendered::ZUIButton_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUIRenderer_Button> inRenderer, ZRef<ZUICaption> inCaption)
:	ZUIButton_Tracked(inSuperPane, inLocator, inResponder), fRenderer(inRenderer), fCaption(inCaption)
	{}

ZUIButton_Rendered::~ZUIButton_Rendered()
	{}

void ZUIButton_Rendered::Activate(bool entering)
	{
	ZUIButton_Tracked::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIButton_Rendered::PrivateDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn, bool pressed, EZTriState hilite)
	{
	if (fRenderer)
		{
		bool isActive = this->GetActive();
		ZUIDisplayState theDisplayState(pressed, hilite, this->GetReallyEnabled(), isActive && this->IsWindowTarget(), isActive);
		fRenderer->RenderButton(this, inDC, inBoundsRgn.Bounds(), fCaption, theDisplayState);
		}
	}

ZRef<ZUIRenderer_Button> ZUIButton_Rendered::GetRenderer()
	{ return fRenderer; }

void ZUIButton_Rendered::SetRenderer(ZRef<ZUIRenderer_Button> inRenderer, bool inRefresh)
	{
	if (fRenderer == inRenderer)
		return;
	if (inRefresh)
		this->Refresh();
	fRenderer = inRenderer;
	if (inRefresh)
		this->Refresh();
	}

ZPoint ZUIButton_Rendered::CalcPreferredSize()
	{
	if (fRenderer)
		return fRenderer->GetPreferredDimensions(this, fCaption);
	return ZUIButton_Tracked::CalcPreferredSize();
	}

ZPoint ZUIButton_Rendered::GetPopupLocation()
	{
	if (fRenderer)
		return fRenderer->GetPopupLocation(this);
	return ZUIButton_Tracked::GetPopupLocation();
	}

void ZUIButton_Rendered::GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	if (fRenderer)
		fRenderer->GetInsets(this, outTopLeftInset, outBottomRightInset);
	else
		ZUIButton_Tracked::GetInsets(outTopLeftInset, outBottomRightInset);
	}

ZPoint ZUIButton_Rendered::GetLocationInset()
	{
	ZPoint theLocation = this->GetLocation();
	if (fRenderer)
		{
		ZPoint topLeftInset, bottomRightInset;
		fRenderer->GetInsets(this, topLeftInset, bottomRightInset);
		theLocation += topLeftInset;
		}
	return theLocation;
	}

ZPoint ZUIButton_Rendered::GetSizeInset()
	{
	ZPoint theSize = this->GetSize();
	if (fRenderer)
		{
		ZPoint topLeftInset, bottomRightInset;
		fRenderer->GetInsets(this, topLeftInset, bottomRightInset);
		theSize -= topLeftInset - bottomRightInset;
		}
	return theSize;
	}

bool ZUIButton_Rendered::WantsToBecomeTarget()
	{
	if (fRenderer)
		{
		if (fRenderer->ButtonWantsToBecomeTarget(this))
			{
			bool isTargettable;
			if (this->GetAttribute("ZUIButton::Targettable", nil, &isTargettable))
				return isTargettable;
			return true;
			}
		return false;
		}
	return ZUIButton_Tracked::WantsToBecomeTarget();
	}

void ZUIButton_Rendered::HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	if (inNewTarget == this && fRenderer)
		fRenderer->ButtonBecameWindowTarget(this);
	ZUIButton_Tracked::HandlerBecameWindowTarget(inOldTarget, inNewTarget);
	}

void ZUIButton_Rendered::HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
	if (inOldTarget == this && fRenderer)
		fRenderer->ButtonResignedWindowTarget(this);
	ZUIButton_Tracked::HandlerResignedWindowTarget(inOldTarget, inNewTarget);
	}

bool ZUIButton_Rendered::DoKeyDown(const ZEvent_Key& inEvent)
	{
	if (fRenderer && fRenderer->ButtonDoKeyDown(this, inEvent))
		return true;
	return ZUIButton_Tracked::DoKeyDown(inEvent);
	}

void ZUIButton_Rendered::SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh)
	{
	if (fCaption == inCaption)
		return;
	if (inRefresh)
		this->Refresh();
	fCaption = inCaption;
	if (inRefresh)
		this->Refresh();
	}

ZRef<ZUICaption> ZUIButton_Rendered::GetCaption()
	{ return fCaption; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar

ZUIRenderer_ScrollBar::ZUIRenderer_ScrollBar()
	{}

ZUIRenderer_ScrollBar::~ZUIRenderer_ScrollBar()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollBar_Rendered

ZUIScrollBar_Rendered::ZUIScrollBar_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, bool inWithBorder, ZRef<ZUIRenderer_ScrollBar> inRenderer)
:	ZUIScrollBar(inSuperPane, inPaneLocator), fWithBorder(inWithBorder), fRenderer(inRenderer)
	{}

ZUIScrollBar_Rendered::~ZUIScrollBar_Rendered()
	{}

void ZUIScrollBar_Rendered::Activate(bool entering)
	{
	ZUIScrollBar::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIScrollBar_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer)
		fRenderer->Draw(this, inDC, inBoundsRgn.Bounds());
	}

bool ZUIScrollBar_Rendered::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fRenderer)
		fRenderer->Click(this, inHitPoint, inEvent);
	return true;
	}

ZCoord ZUIScrollBar_Rendered::GetPreferredThickness()
	{
	if (fRenderer)
		return fRenderer->GetPreferredThickness(this);
	return 0;
	}

bool ZUIScrollBar_Rendered::GetWithBorder()
	{ return fWithBorder; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Base

class ZUIRenderer_ScrollBar_Base::Tracker : public ZMouseTracker, public ZPaneLocator
	{
public:
	Tracker(ZRef<ZUIRenderer_ScrollBar_Base> inRenderer, ZUIScrollBar_Rendered* inScrollBar,
					ZUIScrollBar::HitPart inHitPart, ZPoint inStartPoint);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

protected:
	ZRef<ZUIRenderer_ScrollBar_Base> fRenderer;
	ZUIScrollBar_Rendered* fScrollBar;
	ZUIScrollBar::HitPart fHitPart;
	bool fWasHit;
	bigtime_t fTime_Start;
	bigtime_t fTime_LastMove;
	} ;

ZUIRenderer_ScrollBar_Base::Tracker::Tracker(ZRef<ZUIRenderer_ScrollBar_Base> inRenderer, ZUIScrollBar_Rendered* inScrollBar,
					ZUIScrollBar::HitPart inHitPart, ZPoint inStartPoint)
:	ZMouseTracker(inScrollBar, inStartPoint), ZPaneLocator(inScrollBar->GetPaneLocator()),
	fRenderer(inRenderer), fScrollBar(inScrollBar), fHitPart(inHitPart), fWasHit(false)
	{}

bool ZUIRenderer_ScrollBar_Base::Tracker::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inPane == fScrollBar)
		{
		if (inAttribute == "ZUIScrollBar::HitPart")
			{
			(*(ZUIScrollBar::HitPart*)outResult) = fWasHit ? fHitPart : ZUIScrollBar::hitPartNone;
			return true;
			}
/*		if (inAttribute == "ZUIScrollBar::GhostThumbValue")
			{
// hard coded for now
			(*(double*)outResult) = 0.8;
			return true;
			}*/
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

void ZUIRenderer_ScrollBar_Base::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	bool theEnabled = fScrollBar->GetReallyEnabled();
	double theValue = fScrollBar->GetValue();
	double theThumbProportion = fScrollBar->GetThumbProportion();

	bool theWithBorder = fScrollBar->GetWithBorder();
	ZRect theBounds = fScrollBar->CalcBounds();

	ZUIScrollBar::HitPart theHitPart = fRenderer->FindHitPart(fScrollBar, inNext, theBounds, theEnabled, theWithBorder, theValue, theThumbProportion);

	bool isHit = (theHitPart == fHitPart);

	ZUIRange::Part rangePart = ZUIRange::PartNone;

	if (fHitPart == ZUIScrollBar::hitPartUpArrowOuter)
		rangePart = ZUIRange::PartStepUp;
	else if (fHitPart == ZUIScrollBar::hitPartUpArrowInner)
		rangePart = ZUIRange::PartStepUp;
	else if (fHitPart == ZUIScrollBar::hitPartDownArrowOuter)
		rangePart = ZUIRange::PartStepDown;
	else if (fHitPart == ZUIScrollBar::hitPartDownArrowInner)
		rangePart = ZUIRange::PartStepDown;
	else if (fHitPart == ZUIScrollBar::hitPartPageUp)
		rangePart = ZUIRange::PartPageUp;
	else if (fHitPart == ZUIScrollBar::hitPartPageDown)
		rangePart = ZUIRange::PartPageDown;

	switch (inPhase)
		{
		case ePhaseBegin:
			{
			fTime_Start = ZTicks::sNow();
			fTime_LastMove = fTime_Start;
			fScrollBar->Refresh();
			fScrollBar->SetPaneLocator(this, false);
			// Fall through
			}
		case ePhaseContinue:
			{
			if (fWasHit != isHit)
				{
				fWasHit = isHit;
				fScrollBar->Refresh();
				if (isHit)
					fScrollBar->Internal_NotifyResponders(ZUIRange::EventDown, rangePart);
				else
					fScrollBar->Internal_NotifyResponders(ZUIRange::EventUp, rangePart);
//				fScrollBar->Update();
				}
			else
				{
				bigtime_t now = ZTicks::sNow();
// For the first 1/2 second we delay 1/10 second between each firing of the still down message
				if ((now - fTime_Start > ZTicks::sPerSecond() / 2) || (now - fTime_LastMove > ZTicks::sPerSecond() / 10))
					{
					fTime_LastMove = now;
					if (isHit)
						fScrollBar->Internal_NotifyResponders(ZUIRange::EventStillDown, rangePart);
					else
						fScrollBar->Internal_NotifyResponders(ZUIRange::EventStillUp, rangePart);
					}
				}
			break;
			}
		case ePhaseEnd:
			{
			if (fWasHit)
				{
				fScrollBar->Internal_NotifyResponders(ZUIRange::EventAboutToBeReleasedWhileDown, rangePart);
				fWasHit = false;
				fScrollBar->Refresh();
				fScrollBar->Internal_NotifyResponders(ZUIRange::EventReleasedWhileDown, rangePart);
//				fScrollBar->Update();
				}
			else
				{
				fScrollBar->Internal_NotifyResponders(ZUIRange::EventReleasedWhileUp, rangePart);
				}
			fScrollBar->SetPaneLocator(fNextPaneLocator, false);
			break;
			}
		}
	}

// ==================================================

class ZUIRenderer_ScrollBar_Base::ThumbTracker : public ZMouseTracker, public ZPaneLocator
	{
public:
	ThumbTracker(ZRef<ZUIRenderer_ScrollBar_Base> theRenderer, ZUIScrollBar_Rendered* theScrollBar,
				bool dragGhost, ZPoint theStartPoint);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
	virtual bool GetPaneAttribute(ZSubPane* thePane, const string& theAttribute, void* inParam, void* outResult);

protected:
	ZRef<ZUIRenderer_ScrollBar_Base> fRenderer;
	ZUIScrollBar_Rendered* fScrollBar;
	bool fDragGhost;
	bool fInRange;
	double fInitialValue;
	double fGhostThumbValue;
	} ;

ZUIRenderer_ScrollBar_Base::ThumbTracker::ThumbTracker(ZRef<ZUIRenderer_ScrollBar_Base> theRenderer, ZUIScrollBar_Rendered* theScrollBar,
									bool dragGhost, ZPoint theStartPoint)
:	ZMouseTracker(theScrollBar, theStartPoint), ZPaneLocator(theScrollBar->GetPaneLocator()),
	fRenderer(theRenderer), fScrollBar(theScrollBar), fDragGhost(dragGhost), fInRange(true)
	{}

void ZUIRenderer_ScrollBar_Base::ThumbTracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	ZRect bounds = fScrollBar->CalcBounds();
	bool horizontal = bounds.Width() > bounds.Height();

	bool withBorder = fScrollBar->GetWithBorder();

	if (fDragGhost)
		{
		switch (inPhase)
			{
			case ePhaseBegin:
				fInitialValue = fScrollBar->GetValue();
				fScrollBar->SetPaneLocator(this, false);
				fScrollBar->Refresh();
			case ePhaseContinue:
				{
				fInRange = bounds.Inset(-32, -32).Contains(inNext);
				double newGhostThumbValue = fGhostThumbValue;
				if (fInRange)
					newGhostThumbValue = fInitialValue + fRenderer->GetDelta(fScrollBar, bounds, withBorder, fInitialValue, fScrollBar->GetThumbProportion(), inNext - inAnchor);
				else
					newGhostThumbValue = fInitialValue;
				newGhostThumbValue = max(0.0, min(newGhostThumbValue, 1.0));
				if (fGhostThumbValue != newGhostThumbValue)
					{
					fGhostThumbValue = newGhostThumbValue;
					fScrollBar->Refresh();
					}
//				fScrollBar->Update();
				}
				break;
			case ePhaseEnd:
				{
				fScrollBar->Internal_NotifyResponders(fGhostThumbValue);
				fScrollBar->Refresh();
				fScrollBar->SetPaneLocator(fNextPaneLocator, false);
				}
				break;
			}
		}
	else
		{
		double currentValue = fScrollBar->GetValue();
		double thumbProportion = fScrollBar->GetThumbProportion();

		switch (inPhase)
			{
			case ePhaseBegin:
				fInitialValue = currentValue;
				fScrollBar->SetPaneLocator(this, false);
				fScrollBar->Refresh();
			case ePhaseContinue:
				{
				fInRange = bounds.Inset(-32, -32).Contains(inNext);
				double currentDelta = 0.0;
				if (fInRange)
					currentDelta = fRenderer->GetDelta(fScrollBar, bounds, withBorder, currentValue, thumbProportion, inNext - inAnchor);
				fScrollBar->Internal_NotifyResponders(fInitialValue + currentDelta);
//				fScrollBar->Update();
				}
				break;
			case ePhaseEnd:
				{
				fScrollBar->Refresh();
				fScrollBar->SetPaneLocator(fNextPaneLocator, false);
				}
				break;
			}
		}
	}

bool ZUIRenderer_ScrollBar_Base::ThumbTracker::GetPaneAttribute(ZSubPane* thePane, const string& theAttribute, void* inParam, void* outResult)
	{
	if (theAttribute == "ZUIScrollBar::HitPart")
		{
		(*(ZUIScrollBar::HitPart*)outResult) = ZUIScrollBar::hitPartThumb;
		return true;
		}

	if (theAttribute == "ZUIScrollBar::GhostThumbValue")
		{
		if (fDragGhost)
			{
			(*(double*)outResult) = fGhostThumbValue;
			return true;
			}
		}

	return ZPaneLocator::GetPaneAttribute(thePane, theAttribute, inParam, outResult);
	}

// ==================================================

ZUIRenderer_ScrollBar_Base::ZUIRenderer_ScrollBar_Base()
	{}

ZUIRenderer_ScrollBar_Base::~ZUIRenderer_ScrollBar_Base()
	{}

void ZUIRenderer_ScrollBar_Base::Draw(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds)
	{
	bool theEnabled = inScrollBar->GetReallyEnabled();
	bool theActive = inScrollBar->GetActive();
	double theValue = inScrollBar->GetValue();
	double thumbProportion = inScrollBar->GetThumbProportion();
	bool withBorder = inScrollBar->GetWithBorder();

	double ghostThumbValue;
	if (!inScrollBar->GetAttribute("ZUIScrollBar::GhostThumbValue", nil, &ghostThumbValue))
		ghostThumbValue = -1.0;

	ZUIScrollBar::HitPart theHitPart;
	if (!inScrollBar->GetAttribute("ZUIScrollBar::HitPart", nil, &theHitPart))
		theHitPart = ZUIScrollBar::hitPartNone;
	
	this->RenderScrollBar(inScrollBar, inDC, inBounds, theEnabled, theActive, withBorder, theValue, thumbProportion, ghostThumbValue, theHitPart);
	}

void ZUIRenderer_ScrollBar_Base::Click(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	bool theEnabled = inScrollBar->GetReallyEnabled();
	if (!theEnabled)
		return;

	double theThumbProportion = inScrollBar->GetThumbProportion();

	double theValue = inScrollBar->GetValue();

	bool theWithBorder = inScrollBar->GetWithBorder();
	ZRect theBounds = inScrollBar->CalcBounds();

	ZUIScrollBar::HitPart theHitPart = this->FindHitPart(inScrollBar, inHitPoint, theBounds, theEnabled, theWithBorder, theValue, theThumbProportion);
	if (theHitPart == ZUIScrollBar::hitPartNone)
		return;
	if (theHitPart == ZUIScrollBar::hitPartThumb)
		{
		ThumbTracker* theTracker = new ThumbTracker(this, inScrollBar, inEvent.GetOption(), inHitPoint);
		theTracker->Install();
		}
	else
		{
		Tracker* theTracker = new Tracker(this, inScrollBar, theHitPart, inHitPoint);
		theTracker->Install();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider

ZUIRenderer_Slider::ZUIRenderer_Slider()
	{}

ZUIRenderer_Slider::~ZUIRenderer_Slider()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISlider_Rendered

ZUISlider_Rendered::ZUISlider_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Slider> inRenderer)
:	ZUISlider(inSuperPane, inPaneLocator), fRenderer(inRenderer)
	{}

ZUISlider_Rendered::~ZUISlider_Rendered()
	{}

void ZUISlider_Rendered::Activate(bool entering)
	{
	ZUISlider::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUISlider_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer)
		fRenderer->Draw(this, inDC, inBoundsRgn.Bounds());
	}

bool ZUISlider_Rendered::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fRenderer)
		fRenderer->Click(this, inHitPoint, inEvent);
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Base

ZUIRenderer_Slider_Base::ZUIRenderer_Slider_Base()
	{}

ZUIRenderer_Slider_Base::~ZUIRenderer_Slider_Base()
	{}

// From ZUIRenderer_Slider
void ZUIRenderer_Slider_Base::Draw(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds)
	{
	bool theEnabled = inSlider->GetReallyEnabled();
	double theValue = inSlider->GetValue();

	double ghostThumbValue;
	if (!inSlider->GetAttribute("ZUISlider::GhostThumbValue", nil, &ghostThumbValue))
		ghostThumbValue = -1.0;

	bool theThumbHilited;
	if (!inSlider->GetAttribute("ZUISlider::ThumbHilited", nil, &theThumbHilited))
		theThumbHilited = false;
	
	this->RenderSlider(inSlider, inDC, inBounds, theEnabled, theValue, ghostThumbValue, theThumbHilited);
	}

void ZUIRenderer_Slider_Base::Click(ZUISlider_Rendered* inSlider, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows

ZUIRenderer_LittleArrows::ZUIRenderer_LittleArrows()
	{}
ZUIRenderer_LittleArrows::~ZUIRenderer_LittleArrows()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUILittleArrows_Rendered

ZUILittleArrows_Rendered::ZUILittleArrows_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_LittleArrows> inRenderer)
:	ZUILittleArrows(inSuperPane, inPaneLocator), fRenderer(inRenderer)
	{}

ZUILittleArrows_Rendered::~ZUILittleArrows_Rendered()
	{}

void ZUILittleArrows_Rendered::Activate(bool entering)
	{
	ZUILittleArrows::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

ZPoint ZUILittleArrows_Rendered::GetSize()
	{
	if (fRenderer)
		return fRenderer->GetPreferredSize();
	return ZPoint::sZero;
	}

void ZUILittleArrows_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer)
		fRenderer->Draw(this, inDC, inBoundsRgn.Bounds());
	}

bool ZUILittleArrows_Rendered::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fRenderer)
		fRenderer->Click(this, inHitPoint, inEvent);
	return true;
	}

ZPoint ZUILittleArrows_Rendered::GetPreferredSize()
	{
	if (fRenderer)
		return fRenderer->GetPreferredSize();
	return ZPoint::sZero;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Base

class ZUIRenderer_LittleArrows_Base::Tracker : public ZMouseTracker, public ZPaneLocator
	{
public:
	Tracker(ZRef<ZUIRenderer_LittleArrows_Base> inRenderer, ZUILittleArrows_Rendered* inLittleArrows,
				ZUILittleArrows::HitPart inHitPart, ZPoint inStartPoint);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

protected:
	ZRef<ZUIRenderer_LittleArrows_Base> fRenderer;
	ZUILittleArrows_Rendered* fLittleArrows;
	ZUILittleArrows::HitPart fHitPart;
	bool fWasHit;
	} ;

ZUIRenderer_LittleArrows_Base::Tracker::Tracker(ZRef<ZUIRenderer_LittleArrows_Base> inRenderer, ZUILittleArrows_Rendered* inLittleArrows,
										ZUILittleArrows::HitPart inHitPart, ZPoint inStartPoint)
:	ZMouseTracker(inLittleArrows, inStartPoint), ZPaneLocator(inLittleArrows->GetPaneLocator()),
	fRenderer(inRenderer), fLittleArrows(inLittleArrows), fHitPart(inHitPart), fWasHit(false)
	{}

bool ZUIRenderer_LittleArrows_Base::Tracker::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inPane == fLittleArrows)
		{
		if (inAttribute == "ZUILittleArrows::HitPart")
			{
			(*(ZUILittleArrows::HitPart*)outResult) = fWasHit ? fHitPart : ZUILittleArrows::hitPartNone;
			return true;
			}
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

void ZUIRenderer_LittleArrows_Base::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
	bool theEnabled = fLittleArrows->GetReallyEnabled();
	ZRect theBounds = fLittleArrows->CalcBounds();

	ZUILittleArrows::HitPart theHitPart = fRenderer->FindHitPart(fLittleArrows, inNext, theBounds, theEnabled);

	bool isHit = (theHitPart == fHitPart);

	ZUIRange::Part rangePart = ZUIRange::PartNone;
	if (fHitPart == ZUILittleArrows::hitPartUp)
		rangePart = ZUIRange::PartStepUp;
	else if (fHitPart == ZUILittleArrows::hitPartDown)
		rangePart = ZUIRange::PartStepDown;

	switch (inPhase)
		{
		case ePhaseBegin:
			fLittleArrows->SetPaneLocator(this, false);
		case ePhaseContinue:
			{
			if (fWasHit != isHit)
				{
				fWasHit = isHit;
				fLittleArrows->DrawNow();
				if (isHit)
					fLittleArrows->Internal_NotifyResponders(ZUIRange::EventDown, rangePart);
				else
					fLittleArrows->Internal_NotifyResponders(ZUIRange::EventUp, rangePart);
				}
			else
				{
				if (isHit)
					fLittleArrows->Internal_NotifyResponders(ZUIRange::EventStillDown, rangePart);
				else
					fLittleArrows->Internal_NotifyResponders(ZUIRange::EventStillUp, rangePart);
				}
			}
			break;
		case ePhaseEnd:
			{
			if (fWasHit)
				{
				fLittleArrows->Internal_NotifyResponders(ZUIRange::EventAboutToBeReleasedWhileDown, rangePart);
				fWasHit = false;
				fLittleArrows->DrawNow();
				fLittleArrows->Internal_NotifyResponders(ZUIRange::EventReleasedWhileDown, rangePart);
				}
			else
				{
				fLittleArrows->Internal_NotifyResponders(ZUIRange::EventReleasedWhileUp, rangePart);
				}
			fLittleArrows->SetPaneLocator(fNextPaneLocator, false);
			}
			break;
		}
	}

ZUIRenderer_LittleArrows_Base::ZUIRenderer_LittleArrows_Base()
	{}

ZUIRenderer_LittleArrows_Base::~ZUIRenderer_LittleArrows_Base()
	{}

// From ZUIRenderer_LittleArrows
void ZUIRenderer_LittleArrows_Base::Draw(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds)
	{
	ZUILittleArrows::HitPart theHitPart;
	if (!inLittleArrows->GetAttribute("ZUILittleArrows::HitPart", nil, &theHitPart))
		theHitPart = ZUILittleArrows::hitPartNone;

	this->RenderLittleArrows(inLittleArrows, inDC, inBounds, inLittleArrows->GetReallyEnabled(), inLittleArrows->GetActive(), theHitPart);
	}

void ZUIRenderer_LittleArrows_Base::Click(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZUILittleArrows::HitPart theHitPart = this->FindHitPart(inLittleArrows, inHitPoint, inLittleArrows->CalcBounds(), inLittleArrows->GetReallyEnabled());
	if (theHitPart != ZUILittleArrows::hitPartNone)
		{
		Tracker* theTracker = new Tracker(this, inLittleArrows, theHitPart, inHitPoint);
		theTracker->Install();
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar

ZUIRenderer_ProgressBar::ZUIRenderer_ProgressBar()
	{}

ZUIRenderer_ProgressBar::~ZUIRenderer_ProgressBar()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIProgressBar_Rendered

ZUIProgressBar_Rendered::ZUIProgressBar_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_ProgressBar> inRenderer)
:	ZUIProgressBar(inSuperPane, inLocator), fRenderer(inRenderer), fCurrentFrame(0)
	{}

ZUIProgressBar_Rendered::~ZUIProgressBar_Rendered()
	{}

void ZUIProgressBar_Rendered::Activate(bool entering)
	{
	ZUIProgressBar::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIProgressBar_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer)
		{
		double currentProgress;
		if (!this->GetAttribute("ZUIProgressBar::Progress", nil, &currentProgress))
			currentProgress = -1.0;
		if (currentProgress < 0.0 || currentProgress > 1.0)
			fRenderer->DrawIndeterminate(inDC, inBoundsRgn.Bounds(), this->GetReallyEnabled(), this->GetActive(), fCurrentFrame);
		else
			fRenderer->DrawDeterminate(inDC, inBoundsRgn.Bounds(), this->GetReallyEnabled(), this->GetActive(), 0.0, currentProgress);
		}
	}

ZPoint ZUIProgressBar_Rendered::GetSize()
	{
	ZPoint theSize = ZUIProgressBar::GetSize();
	if (fRenderer)
		theSize.v = fRenderer->GetHeight();
	return theSize;
	}

bigtime_t ZUIProgressBar_Rendered::NextFrame()
	{
	if (fRenderer)
		{
		double currentProgress;
		if (!this->GetAttribute("ZUIProgressBar::Progress", nil, &currentProgress))
			currentProgress = -1.0;
// If current progress is < 0 or > 1.0 then we're indeterminate.
		if (currentProgress < 0.0 || currentProgress > 1.0)
			{
			++fCurrentFrame;
			if (fCurrentFrame >= fRenderer->GetFrameCount())
				fCurrentFrame = 0;
			this->Refresh();
			return fRenderer->GetInterFrameDuration();
			}
		}
// Return a large wait time. We'll reset our next tick if someone sets our progress
	return ZTicks::sPerSecond() * 10;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows

ZUIRenderer_ChasingArrows::ZUIRenderer_ChasingArrows()
	{}

ZUIRenderer_ChasingArrows::~ZUIRenderer_ChasingArrows()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIChasingArrows_Rendered

ZUIChasingArrows_Rendered::ZUIChasingArrows_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_ChasingArrows> inRenderer)
:	ZUIChasingArrows(inSuperPane, inLocator), fRenderer(inRenderer), fCurrentFrame(0)
	{}

void ZUIChasingArrows_Rendered::Activate(bool entering)
	{
	ZUIChasingArrows::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIChasingArrows_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
//	ZDC_OffAuto localDC(inDC, false);
	ZUIChasingArrows::DoDraw(inDC, inBoundsRgn);
	if (fRenderer)
		{
		ZDC localDC(inDC);
		fRenderer->DrawFrame(localDC, ZPoint::sZero, this->GetReallyEnabled(), this->GetActive(), fCurrentFrame);
		}
	}

ZPoint ZUIChasingArrows_Rendered::GetSize()
	{
	if (fRenderer)
		return fRenderer->GetSize();
	return ZPoint::sZero;
	}

bigtime_t ZUIChasingArrows_Rendered::NextFrame()
	{
	if (fRenderer)
		{
		++fCurrentFrame;
		if (fCurrentFrame >= fRenderer->GetFrameCount())
			fCurrentFrame = 0;
		this->Refresh();
		return fRenderer->GetInterFrameDuration();
		}
	return ZTicks::sPerSecond()/4;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane

ZUIRenderer_TabPane::ZUIRenderer_TabPane()
	{}

ZUIRenderer_TabPane::~ZUIRenderer_TabPane()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITabPane_Rendered::ContentPane

class ZUITabPane_Rendered::ContentPane : public ZSuperPane
	{
public:
	ContentPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ~ContentPane();

	virtual bool GetActive();
	virtual void Activate(bool entering);
	virtual void HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual void SubHandlerBeingFreed(ZEventHr* inHandler);

protected:
	void PanelShown(bool shown);
	ZEventHr* fRememberedTarget;
	friend class ZUITabPane_Rendered;
	};

ZUITabPane_Rendered::ContentPane::ContentPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
:	ZSuperPane(inSuperPane, inLocator), fRememberedTarget(nil)
	{}

ZUITabPane_Rendered::ContentPane::~ContentPane()
	{}

void ZUITabPane_Rendered::ContentPane::SubHandlerBeingFreed(ZEventHr* inHandler)
	{
	if (inHandler == fRememberedTarget)
		fRememberedTarget = nil;
	ZSuperPane::SubHandlerBeingFreed(inHandler);
	}

void ZUITabPane_Rendered::ContentPane::Activate(bool entering)
	{
	if (this->GetVisible())
		ZSuperPane::Activate(entering);
	}

void ZUITabPane_Rendered::ContentPane::HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget)
	{
// Remember that this is our target
	fRememberedTarget = inNewTarget;

// And let the base class handle stuff
	ZSuperPane::HandlerBecameWindowTarget(inOldTarget, inNewTarget);
	}

bool ZUITabPane_Rendered::ContentPane::GetActive()
	{ return ZSuperPane::GetActive() && ZSuperPane::GetVisible(); }

void ZUITabPane_Rendered::ContentPane::PanelShown(bool shown)
	{
	if (shown)
		{
		ZSuperPane::Activate(true);
		if (fRememberedTarget)
			fRememberedTarget->BecomeWindowTarget();
		else
			this->BecomeWindowTarget();
		}
	else
		ZSuperPane::Activate(false);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUITabPane_Rendered

ZUITabPane_Rendered::ZUITabPane_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_TabPane> inRenderer)
:	ZUITabPane(inSuperPane, inLocator), ZPaneLocator(nil), fCurrentContentPane(nil), fRenderer(inRenderer)
	{
	fContentWrapper = new ZSuperPane(this, this);
	}

ZUITabPane_Rendered::~ZUITabPane_Rendered()
	{}

void ZUITabPane_Rendered::Activate(bool entering)
	{
	ZUITabPane::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUITabPane_Rendered::DeletePane(ZSuperPane* inPane)
	{
	vector<ZRef<ZUICaption> >::iterator j = fTabCaptions.begin();
	for (vector<ContentPane*>::iterator i = fContentPanes.begin(); i != fContentPanes.end() && j != fTabCaptions.end(); ++i, ++j)
		{
		if (*i == inPane)
			{
			fContentPanes.erase(i);
			fTabCaptions.erase(j);
			break;
			}
		}
	}

void ZUITabPane_Rendered::SetCurrentPane(ZSuperPane* inPane)
	{
	if (fCurrentContentPane == inPane)
		return;
// Find the real content pane pointers
	ContentPane* currentContentPane = nil;
	ContentPane* newContentPane = nil;
	for (vector<ContentPane*>::iterator i = fContentPanes.begin(); i != fContentPanes.end(); ++i)
		{
		if (fCurrentContentPane == *i)
			currentContentPane = *i;
		if (inPane == *i)
			newContentPane = *i;
		}

// This will make all contents invisible
	fCurrentContentPane = nil;
// So when the current and new contents are switched out/in
	if (currentContentPane)
		currentContentPane->PanelShown(false);
	if (newContentPane)
		newContentPane->PanelShown(true);
	fCurrentContentPane = inPane;
	this->Refresh();
	}

ZSuperPane* ZUITabPane_Rendered::GetCurrentPane()
	{ return fCurrentContentPane; }

ZSuperPane* ZUITabPane_Rendered::CreateTab(ZRef<ZUICaption> inCaption)
	{
	ContentPane* contentPane = new ContentPane(fContentWrapper, this);
	fContentPanes.push_back(contentPane);
	fTabCaptions.push_back(inCaption);
	if (fCurrentContentPane == nil)
		fCurrentContentPane = contentPane;
	return contentPane;
	}

void ZUITabPane_Rendered::SetCurrentTabCaption(ZRef<ZUICaption> inCaption)
	{
	vector<ZRef<ZUICaption> >::iterator j = fTabCaptions.begin();
	for (vector<ContentPane*>::iterator i = fContentPanes.begin(); i != fContentPanes.end() && j != fTabCaptions.end(); ++i, ++j)
		{
		if (*j == inCaption)
			this->SetCurrentTab(*i);
		}
	}

ZRef<ZUICaption> ZUITabPane_Rendered::GetCurrentTabCaption()
	{
	vector<ZRef<ZUICaption> >::iterator j = fTabCaptions.begin();
	for (vector<ContentPane*>::iterator i = fContentPanes.begin(); i != fContentPanes.end() && j != fTabCaptions.end(); ++i, ++j)
		{
		if (*i == fCurrentContentPane)
			return *j;
		}
	return ZRef<ZUICaption>();
	}

const vector<ZRef<ZUICaption> >& ZUITabPane_Rendered::GetCaptionVector()
	{ return fTabCaptions; }

ZRef<ZUIRenderer_TabPane> ZUITabPane_Rendered::GetRenderer()
	{ return fRenderer; }

bool ZUITabPane_Rendered::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	if (inPane == fContentWrapper && fRenderer)
		{
		ZRect theContentArea = fRenderer->GetContentArea(this);
		outLocation = theContentArea.TopLeft();
		return true;
		}
	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool ZUITabPane_Rendered::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (inPane == fContentWrapper && fRenderer)
		{
		ZRect theContentArea = fRenderer->GetContentArea(this);
		outSize = theContentArea.Size();
		return true;
		}
	return ZPaneLocator::GetPaneSize(inPane, outSize);
	}

bool ZUITabPane_Rendered::GetPaneVisible(ZSubPane* inPane, bool& outVisible)
	{
// Shortcut the current content wrapper
	if (inPane == fCurrentContentPane)
		{
		outVisible = true;
		return true;
		}
	if (find(fContentPanes.begin(), fContentPanes.end(), inPane) != fContentPanes.end())
		{
		outVisible = false;
		return true;
		}
	return ZPaneLocator::GetPaneVisible(inPane, outVisible);
	}

void ZUITabPane_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	if (fRenderer)
		{
		ZDCRgn uncoveredRgn(inDC.GetClip() - this->GetSubPaneRgn());
		inDC.Fill(uncoveredRgn);

		fRenderer->RenderBackground(this, inDC, inBoundsRgn.Bounds());
		}
	else
		{
		ZUITabPane::DoDraw(inDC, inBoundsRgn);
		}
	}

bool ZUITabPane_Rendered::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (fRenderer && fRenderer->DoClick(this, inHitPoint, inEvent))
		return true;
	return ZSuperPane::DoClick(inHitPoint, inEvent);
	}

bool ZUITabPane_Rendered::GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk)
	{
	if (inPane == fContentWrapper)
		{
		if (fRenderer && fRenderer->GetContentBackInk(this, inDC, outInk))
			return true;
		}
	return ZPaneLocator::GetPaneBackInk(inPane, inDC, outInk);
	}

void ZUITabPane_Rendered::SubPaneRemoved(ZSubPane* inPane)
	{
// Sanity check. If you want to delete a pane call our DeletePane method, that way we'll keep our
// contentPane and caption vectors in sync. This won't trip when ZSuperPane::~ZSuperPane executes,
// because we'll no longer *be* a ZUITabPane_Rendered by that time, and ZSuperPane's SubPaneRemoved
// method will be refererenced by our vtbl pointer.
	ZAssertStop(2, find(fContentPanes.begin(), fContentPanes.end(), inPane) == fContentPanes.end());
	ZUITabPane::SubPaneRemoved(inPane);
	}

ZDCRgn ZUITabPane_Rendered::DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds)
	{
	ZDCRgn theRgn;
	if (inOldBounds != inNewBounds)
		{
		theRgn = inOldBounds;
		theRgn |= inNewBounds;
		}
	return theRgn;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Base

ZUIRenderer_TabPane_Base::ZUIRenderer_TabPane_Base()
	{}

ZUIRenderer_TabPane_Base::~ZUIRenderer_TabPane_Base()
	{}

bool ZUIRenderer_TabPane_Base::DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	ZRef<ZUICaption> currentCaption(inTabPane->GetCurrentTabCaption());
	const vector<ZRef<ZUICaption> >& theVector = inTabPane->GetCaptionVector();
	for (vector<ZRef<ZUICaption> >::const_iterator i = theVector.begin(); i != theVector.end(); ++i)
		{
		ZRect theRect = this->CalcTabRect(inTabPane, *i);
		if (theRect.Contains(inHitPoint))
			{
			if (currentCaption == *i)
				return true;
			Tracker* theTracker = new Tracker(this, inTabPane, *i, inHitPoint);
			theTracker->Install();
			return true;
			}
		}
	return false;
	}

ZUIRenderer_TabPane_Base::Tracker::Tracker(ZRef<ZUIRenderer_TabPane_Base> inRenderer, ZUITabPane_Rendered* inTabPane, ZRef<ZUICaption> inHitCaption, ZPoint inStartPoint)
:	ZMouseTracker(inTabPane, inStartPoint), fRenderer(inRenderer), fTabPane(inTabPane), fCaption(inHitCaption), fWasPressed(false)
	{}

void ZUIRenderer_TabPane_Base::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
// Is the mouse within the tab?
	ZRect theTabRect = fRenderer->CalcTabRect(fTabPane, fCaption);

	switch (inPhase)
		{
		case ePhaseBegin:
		case ePhaseContinue:
			{
			bool isPressed = theTabRect.Contains(inNext);
			if (fWasPressed != isPressed)
				{
				ZUIDisplayState theState(isPressed, eZTriState_Off, true, false, true);
				ZDC theDC = fTabPane->GetDC();
				fRenderer->DrawTab(fTabPane, theDC, fCaption, theTabRect, theState);
				fWasPressed = isPressed;
				theDC.Flush();
				}
			}
			break;
		case ePhaseEnd:
			{
			if (theTabRect.Contains(inNext))
				fTabPane->SetCurrentTabCaption(fCaption);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group

ZUIRenderer_Group::ZUIRenderer_Group()
	{}
ZUIRenderer_Group::~ZUIRenderer_Group()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGroup_Rendered

class ZUIGroup_Rendered::TitlePane : public ZSuperPane
	{
public:
	TitlePane(ZUIGroup_Rendered* inUIGroup);
	virtual ~TitlePane();

	virtual ZPoint GetSize();
	virtual ZPoint GetLocation();

protected:
	ZUIGroup_Rendered* fUIGroup;
	bool fRecursing;
	};

ZUIGroup_Rendered::TitlePane::TitlePane(ZUIGroup_Rendered* inUIGroup)
:	ZSuperPane(inUIGroup, nil), fUIGroup(inUIGroup), fRecursing(false)
	{}

ZUIGroup_Rendered::TitlePane::~TitlePane()
	{}

ZPoint ZUIGroup_Rendered::TitlePane::GetSize()
	{
// WARNING. Do *not* put a pane inside a title pane that takes its size from its super pane. The title
// pane sizes itself to wrap its contents, so its contents can't depend on its super's size.
	ZAssertStopf(1, fRecursing == false, ("ZUIGroup_Rendered::TitlePane::GetSize, called recursively"));
	fRecursing = true;
	ZPoint maxExtent = ZPoint::sZero;
	for (size_t x = 0; x < fSubPanes.size(); ++x)
		{
		ZPoint currentExtent = fSubPanes[x]->CalcFrame().BottomRight();
		maxExtent.h = max(currentExtent.h, maxExtent.h);
		maxExtent.v = max(currentExtent.v, maxExtent.v);
		}
	fRecursing = false;
	return maxExtent;
	}

ZPoint ZUIGroup_Rendered::TitlePane::GetLocation()
	{
	return ZPoint(fUIGroup->GetTitlePaneHLocation(), 0);
	}

// ----------

class ZUIGroup_Rendered::InternalPane : public ZSuperPane
	{
public:
	InternalPane(ZUIGroup_Rendered* inUIGroup);
	virtual ~InternalPane();

	virtual ZPoint GetSize();
	virtual ZPoint GetLocation();

protected:
	ZUIGroup_Rendered* fUIGroup;
	};

ZUIGroup_Rendered::InternalPane::InternalPane(ZUIGroup_Rendered* inUIGroup)
:	ZSuperPane(inUIGroup, nil), fUIGroup(inUIGroup)
	{}

ZUIGroup_Rendered::InternalPane::~InternalPane()
	{}

ZPoint ZUIGroup_Rendered::InternalPane::GetSize()
	{
	ZPoint topLeftInset, bottomRightInset;
	fUIGroup->GetInsets(topLeftInset, bottomRightInset);
	ZPoint superSize = fUIGroup->GetInternalSize();
	return superSize - topLeftInset + bottomRightInset;
	}

ZPoint ZUIGroup_Rendered::InternalPane::GetLocation()
	{
	ZPoint topLeftInset, bottomRightInset;
	fUIGroup->GetInsets(topLeftInset, bottomRightInset);
	return topLeftInset;
	}

// ----------
ZUIGroup_Rendered::ZUIGroup_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_Group> inRenderer)
:	ZUIGroup(inSuperPane, inLocator), fTitlePane(nil), fInternalPane(nil), fRenderer(inRenderer)
	{}

ZUIGroup_Rendered::~ZUIGroup_Rendered()
	{
	}

void ZUIGroup_Rendered::Activate(bool entering)
	{
	ZUIGroup::Activate(entering);
	if (fRenderer)
		{
		ZPoint titlePaneSize = ZPoint::sZero;
		if (fTitlePane)
			titlePaneSize = fTitlePane->GetSize();
		fRenderer->Activate(this, titlePaneSize);
		}
	}

void ZUIGroup_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZUIGroup::DoDraw(inDC, inBoundsRgn);
	if (fRenderer)
		{
		ZPoint titlePaneSize = ZPoint::sZero;
		if (fTitlePane)
			titlePaneSize = fTitlePane->GetSize();
		fRenderer->RenderGroup(inDC, inBoundsRgn.Bounds(), this->GetReallyEnabled(), this->GetActive(), titlePaneSize);
		}
	}

ZDCInk ZUIGroup_Rendered::GetBackInk(const ZDC& inDC)
	{
	if (fRenderer)
		{
		if (ZDCInk theInk = fRenderer->GetBackInk(inDC))
			return theInk;
		}
	return ZUIGroup::GetBackInk(inDC);
	}

void ZUIGroup_Rendered::GetInsets(ZPoint& outTopLeft, ZPoint& outTopRight)
	{
	ZPoint titlePaneSize = ZPoint::sZero;
	if (fTitlePane)
		titlePaneSize = fTitlePane->GetSize();
	if (fRenderer)
		fRenderer->GetInsets(titlePaneSize, outTopLeft, outTopRight);
	}

ZSuperPane* ZUIGroup_Rendered::GetTitlePane()
	{
	if (fTitlePane == nil)
		fTitlePane = new TitlePane(this);
	return fTitlePane;
	}

ZSuperPane* ZUIGroup_Rendered::GetInternalPane()
	{
	if (fInternalPane == nil)
		fInternalPane = new InternalPane(this);
	return fInternalPane;
	}

ZCoord ZUIGroup_Rendered::GetTitlePaneHLocation()
	{
	if (fRenderer)
		return fRenderer->GetTitlePaneHLocation();
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame

ZUIRenderer_FocusFrame::ZUIRenderer_FocusFrame()
	{}

ZUIRenderer_FocusFrame::~ZUIRenderer_FocusFrame()
	{}

bool ZUIRenderer_FocusFrame::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{ return false; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFocusFrame_Rendered

ZUIFocusFrame_Rendered::ZUIFocusFrame_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_FocusFrame> inRenderer)
:	ZUIFocusFrame(inSuperPane, inPaneLocator), fRenderer(inRenderer)
	{}

ZUIFocusFrame_Rendered::~ZUIFocusFrame_Rendered()
	{}

void ZUIFocusFrame_Rendered::Activate(bool entering)
	{
	ZUIFocusFrame::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIFocusFrame_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
// Do what ZSuperPane would do, but remove the border rgn before filling
//	ZDCRgn uncoveredRgn = inBoundsRgn - this->GetSubPaneRgn() - this->GetBorderRgn();
	ZDCRgn uncoveredRgn = inBoundsRgn - this->GetSubPaneRgn();
	inDC.Fill(uncoveredRgn);

	if (fRenderer)
		{
		ZUIDisplayState theState(false, eZTriState_Off, this->GetReallyEnabled(), this->GetBorderShown(), this->GetActive());
		fRenderer->Draw(this, inDC, inBoundsRgn.Bounds(), theState);
		}
	}

ZDCInk ZUIFocusFrame_Rendered::GetInternalBackInk(const ZDC& inDC)
	{
	if (fRenderer)
		{
		ZDCInk theInk;
		ZUIDisplayState theState(false, eZTriState_Off, this->GetReallyEnabled(), this->GetBorderShown(), this->GetActive());
		if (fRenderer->GetInternalBackInk(this, theState, inDC, theInk))
			return theInk;
		}
	return ZUIFocusFrame::GetInternalBackInk(inDC);
	}

ZDCRgn ZUIFocusFrame_Rendered::DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds)
	{
	ZDCRgn theRgn;
	if (fRenderer && inOldBounds != inNewBounds)
		{
		ZPoint topLeftInset, bottomRightInset;
		this->GetInsets(topLeftInset, bottomRightInset);

		ZRect insetOldBounds(inOldBounds);
		insetOldBounds.left += topLeftInset.h;
		insetOldBounds.top += topLeftInset.v;
		insetOldBounds.right += bottomRightInset.h;
		insetOldBounds.bottom += bottomRightInset.v;

		ZRect insetNewBounds(inNewBounds);
		insetNewBounds.left += topLeftInset.h;
		insetNewBounds.top += topLeftInset.v;
		insetNewBounds.right += bottomRightInset.h;
		insetNewBounds.bottom += bottomRightInset.v;
		theRgn = ZDCRgn(inOldBounds) - insetOldBounds;
		theRgn |= ZDCRgn(inNewBounds) - insetNewBounds;
		}
	return theRgn;
	}

void ZUIFocusFrame_Rendered::GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	outTopLeftInset = outBottomRightInset = ZPoint::sZero;
	if (fRenderer)
		fRenderer->GetInsets(this, outTopLeftInset, outBottomRightInset);
	}

ZDCRgn ZUIFocusFrame_Rendered::GetBorderRgn()
	{
	if (fRenderer)
		return fRenderer->GetBorderRgn(this, this->CalcBounds());
	return ZDCRgn();
	}

ZPoint ZUIFocusFrame_Rendered::GetTranslation()
	{
	ZPoint inheritedTranslation = ZSuperPane::GetTranslation();
	if (fRenderer)
		{
		ZPoint topLeftInset, bottomRightInset;
		fRenderer->GetInsets(this, topLeftInset, bottomRightInset);
		inheritedTranslation -= topLeftInset;
		}
	return inheritedTranslation;
	}

ZPoint ZUIFocusFrame_Rendered::GetInternalSize()
	{
	ZPoint inheritedInternalSize = ZSuperPane::GetInternalSize();
	if (fRenderer)
		{
		ZPoint topLeftInset, bottomRightInset;
		fRenderer->GetInsets(this, topLeftInset, bottomRightInset);
		inheritedInternalSize -= topLeftInset-bottomRightInset;
		}
	return inheritedInternalSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter

ZUIRenderer_Splitter::ZUIRenderer_Splitter()
	{}

ZUIRenderer_Splitter::~ZUIRenderer_Splitter()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter_Standard

ZUIRenderer_Splitter_Standard::ZUIRenderer_Splitter_Standard()
	{}

ZUIRenderer_Splitter_Standard::~ZUIRenderer_Splitter_Standard()
	{}

// AG 99-04-06. Moved the platinum splitter code to here, thus making it available for all renderer suites (otherwise I've
// got a bunch more splitters to write.)
void ZUIRenderer_Splitter_Standard::Activate(ZUISplitter_Rendered* inSplitter)
	{ inSplitter->Refresh(); }

ZCoord ZUIRenderer_Splitter_Standard::GetSplitterThickness(bool inHorizontal)
	{ return 5; }

void ZUIRenderer_Splitter_Standard::DrawSplitter(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	localDC.PenNormal();
	if (inState.GetEnabled() && inState.GetActive())
		localDC.SetInk(ZRGBColor::sBlack);
	else
		localDC.SetInk(ZRGBColor(0x5555));

//	localDC.SetInk(inState.GetEnabled() && inState.GetActive() ? ZRGBColor::sBlack : ZRGBColor(0x5555));
	if (inHorizontal)
		{
		localDC.Line(inLocation.h, inLocation.v, inLocation.h + inLength - 1, inLocation.v);
		localDC.Line(inLocation.h, inLocation.v + 4, inLocation.h + inLength - 1, inLocation.v + 4);
		if (inState.GetEnabled() && inState.GetActive())
			{
			ZRGBColor tempColor = inState.GetPressed() ? ZRGBColor(0xCCCC) : ZRGBColor(ZRGBColor::sWhite);
			localDC.SetInk(tempColor);
			localDC.Line(inLocation.h, inLocation.v + 1, inLocation.h + inLength - 1, inLocation.v + 1);
			localDC.Pixel(inLocation.h, inLocation.v + 2, tempColor);

			localDC.SetInk(inState.GetPressed() ? ZRGBColor(0x9999) : ZRGBColor(0xCCCC));
			localDC.Line(inLocation.h + 1, inLocation.v + 2, inLocation.h + inLength - 1, inLocation.v + 2);

			localDC.SetInk(inState.GetPressed() ? ZRGBColor(0x6666) : ZRGBColor(0x9999));
			localDC.Line(inLocation.h, inLocation.v + 3, inLocation.h + inLength - 1, inLocation.v + 3);
			localDC.Line(inLocation.h + inLength - 1, inLocation.v + 1, inLocation.h + inLength - 1, inLocation.v + 3);
			}
		else
			{
			localDC.SetInk(ZRGBColor(0xDDDD));
			localDC.Fill(ZRect(inLocation.h, inLocation.v + 1, inLocation.h + inLength, inLocation.v + 4));
			}
		}
	else
		{
		localDC.Line(inLocation.h, inLocation.v, inLocation.h, inLocation.v + inLength - 1);
		localDC.Line(inLocation.h + 4, inLocation.v, inLocation.h + 4, inLocation.v + inLength - 1);
		if (inState.GetEnabled() && inState.GetActive())
			{
			ZRGBColor tempColor = inState.GetPressed() ? ZRGBColor(0xCCCC) : ZRGBColor(ZRGBColor::sWhite);
			localDC.SetInk(tempColor);
			localDC.Line(inLocation.h + 1, inLocation.v, inLocation.h + 1, inLocation.v + inLength - 1);
			localDC.Pixel(inLocation.h + 2, inLocation.v, tempColor);

			localDC.SetInk(inState.GetPressed() ? ZRGBColor(0x9999) : ZRGBColor(0xCCCC));
			localDC.Line(inLocation.h + 2, inLocation.v+1, inLocation.h+2, inLocation.v + inLength - 1);

			localDC.SetInk(inState.GetPressed() ? ZRGBColor(0x6666) : ZRGBColor(0x9999));
			localDC.Line(inLocation.h + 3, inLocation.v, inLocation.h + 3, inLocation.v + inLength - 1);
			localDC.Line(inLocation.h + 1, inLocation.v + inLength - 1, inLocation.h + 3, inLocation.v + inLength - 1);
			}
		else
			{
			localDC.SetInk(ZRGBColor(0xDDDD));
			localDC.Fill(ZRect(inLocation.h + 1, inLocation.v, inLocation.h + 4, inLocation.v + inLength));
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUISplitter_Rendered

static void sCalcSplitterDistances(double inSplitProportion, ZCoord inSplitterThickness, ZCoord inOverallLength, ZCoord& outFirstLength, ZCoord& outSecondPosition, ZCoord& outSecondLength)
	{
	ZCoord availableLength = inOverallLength - inSplitterThickness;
	outFirstLength = ZCoord(availableLength * inSplitProportion);
	outSecondPosition = outFirstLength + inSplitterThickness;
	outSecondLength = inOverallLength - outSecondPosition;
	}

ZUISplitter_Rendered::ZUISplitter_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Splitter> inRenderer)
:	ZUISplitter(inSuperPane, inPaneLocator), ZPaneLocator(nil), fRenderer(inRenderer)
	{
	fFirstPane = new ZSuperPane(this, this);
	fSecondPane = new ZSuperPane(this, this);
	}

ZUISplitter_Rendered::~ZUISplitter_Rendered()
	{}

void ZUISplitter_Rendered::Activate(bool entering)
	{
	ZUISplitter::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

// From ZSuperPane/ZSubPane
void ZUISplitter_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZUISplitter::DoDraw(inDC, inBoundsRgn);
	bool pane1Visible, pane2Visible;
	this->Internal_GetVisibility(pane1Visible, pane2Visible);
	if (pane1Visible && pane2Visible)
		{
		bool isPressed;
		if (!this->GetAttribute("ZUISplitter_Renderer::Pressed", nil, &isPressed))
			isPressed = false;

		bool isHorizontal;
		ZPoint mySize;
		ZCoord firstLength, secondPosition, secondLength;
		this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);

		ZUIDisplayState theState(isPressed, eZTriState_Normal, this->GetReallyEnabled(), false, this->GetActive());
		if (isHorizontal)
			fRenderer->DrawSplitter(inDC, ZPoint(0, firstLength), mySize.h, true, theState);
		else
			fRenderer->DrawSplitter(inDC, ZPoint(firstLength, 0), mySize.v, false, theState);
		}
	}

class ZUISplitter_Rendered::Tracker : public ZMouseTracker, public ZPaneLocator
	{
public:
	Tracker(ZUISplitter_Rendered* inSplitter, ZPoint inStartPoint, double inStartFirstLength, bool inDragOutline);
	virtual ~Tracker();

// From ZMouseTracker
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);

// From ZPaneLocator
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

protected:
	ZCoord fStartFirstLength;
	ZUISplitter_Rendered* fSplitter;
	ZRef<ZRegionAdorner> fAdorner;
	bool fDragOutline;
	};

ZUISplitter_Rendered::Tracker::Tracker(ZUISplitter_Rendered* inSplitter, ZPoint inStartPoint, double inStartFirstLength, bool inDragOutline)
:	ZMouseTracker(inSplitter, inStartPoint),
	ZPaneLocator(inSplitter->GetPaneLocator()),
	fSplitter(inSplitter),
	fStartFirstLength(ZCoord(inStartFirstLength)),
	fDragOutline(inDragOutline)
	{}

ZUISplitter_Rendered::Tracker::~Tracker()
	{}

bool ZUISplitter_Rendered::Tracker::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (inPane == fSplitter)
		{
		if (inAttribute == "ZUISplitter_Renderer::Pressed")
			{
			(*(bool*)outResult) = true;
			return true;
			}
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

void ZUISplitter_Rendered::Tracker::TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved)
	{
// Figure out what rectangle the splitter should occupy
	bool isHorizontal = fSplitter->GetOrientation();
	ZPoint internalSize = fSplitter->GetInternalSize();

	ZCoord splitterThickness = fSplitter->fRenderer->GetSplitterThickness(isHorizontal);

	ZCoord newPane1Size, newPane2Size;

	ZRect newSplitterBounds;
	if (isHorizontal)
		{
		newPane1Size = max(ZCoord(0), min(ZCoord(fStartFirstLength + inNext.v - inAnchor.v), ZCoord(internalSize.v - splitterThickness)));
		newSplitterBounds.left = 0;
		newSplitterBounds.right = internalSize.h;
		newSplitterBounds.top = newPane1Size;
		newSplitterBounds.bottom = newSplitterBounds.top + splitterThickness;
		newPane2Size = internalSize.v - newPane1Size - splitterThickness;
		}
	else
		{
		newPane1Size = max(ZCoord(0), min(ZCoord(fStartFirstLength + inNext.h - inAnchor.h), ZCoord(internalSize.h - splitterThickness)));
		newSplitterBounds.top = 0;
		newSplitterBounds.bottom = internalSize.v;
		newSplitterBounds.left = newPane1Size;
		newSplitterBounds.right = newPane1Size + splitterThickness;
		newPane2Size = internalSize.h - newPane1Size - splitterThickness;
		}

	if (fDragOutline)
		{
		ZDC theDC = fSplitter->GetDC();
		switch (inPhase)
			{
			case ePhaseBegin:
				fSplitter->SetPaneLocator(this, false);
				fSplitter->DoDraw(theDC, fSplitter->CalcBoundsRgn());
				fAdorner = new ZRegionAdorner(ZDCPattern::sGray);
				fSplitter->InstallAdorner(fAdorner);
			case ePhaseContinue:
				fAdorner->SetRgn(theDC, newSplitterBounds);
				break;
			case ePhaseEnd:
				fSplitter->SetPaneLocator(fNextPaneLocator, false);
				fAdorner->SetRgn(theDC, ZDCRgn());
				fSplitter->RemoveAdorner(fAdorner);
				fSplitter->DoDraw(theDC, fSplitter->CalcBoundsRgn());
				fSplitter->Internal_NotifyResponders(newPane1Size, newPane2Size);
				break;
			}
		}
	else
		{
		switch (inPhase)
			{
			case ePhaseBegin:
				{
				fSplitter->SetPaneLocator(this, false);
				fSplitter->DoDraw(fSplitter->GetDC(), fSplitter->CalcBoundsRgn());
				}
			case ePhaseContinue:
				{
				fSplitter->Internal_NotifyResponders(newPane1Size, newPane2Size);
//				fSplitter->Update();
				}
				break;
			case ePhaseEnd:
				{
				fSplitter->SetPaneLocator(fNextPaneLocator, false);
				fSplitter->DoDraw(fSplitter->GetDC(), fSplitter->CalcBoundsRgn());
				fSplitter->Internal_NotifyResponders(newPane1Size, newPane2Size);
				}
				break;
			}
		}
	}

bool ZUISplitter_Rendered::DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	bool pane1Visible, pane2Visible;
	this->Internal_GetVisibility(pane1Visible, pane2Visible);
	if (pane1Visible && pane2Visible)
		{
		bool isHorizontal;
		ZPoint mySize;
		ZCoord firstLength, secondPosition, secondLength;
		this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);

		if (isHorizontal && ZRect(0, firstLength, mySize.h, secondPosition).Contains(inHitPoint) || !isHorizontal && ZRect(firstLength, 0, secondPosition, mySize.v).Contains(inHitPoint))
			{
			Tracker* theTracker = new Tracker(this, inHitPoint, firstLength, inEvent.GetOption());
			theTracker->Install();
			return true;
			}
		}
	return ZUISplitter::DoClick(inHitPoint, inEvent);
	}

static const char sSplitterCursorVMono[] =
	"                "
	"                "
	"      XXX       "
	"      XXX       "
	"      XXX       "
	"   X  XXX  X    "
	"  XX  XXX  XX   "
	" XXXXXXXXXXXXX  "
	"  XX  XXX  XX   "
	"   X  XXX  X    "
	"      XXX       "
	"      XXX       "
	"      XXX       "
	"                "
	"                "
	"                ";

static const char sSplitterCursorVMask[] =
	"XXXXXXXXXXXXXXXX"
	"XXXXX     XXXXXX"
	"XXXXX     XXXXXX"
	"XXXXX     XXXXXX"
	"XXX         XXXX"
	"XX           XXX"
	"X             XX"
	"               X"
	"X             XX"
	"XX           XXX"
	"XXX         XXXX"
	"XXXXX     XXXXXX"
	"XXXXX     XXXXXX"
	"XXXXX     XXXXXX"
	"XXXXXXXXXXXXXXXX"
	"XXXXXXXXXXXXXXXX";

static const char sSplitterCursorHMono[] =
	"                "
	"       X        "
	"      XXX       "
	"     XXXXX      "
	"       X        "
	"       X        "
	"  XXXXXXXXXXX   "
	"  XXXXXXXXXXX   "
	"  XXXXXXXXXXX   "
	"       X        "
	"       X        "
	"     XXXXX      "
	"      XXX       "
	"       X        "
	"                "
	"                ";

static const char sSplitterCursorHMask[] =
	"XXXXXXX XXXXXXXX"
	"XXXXXX   XXXXXXX"
	"XXXXX     XXXXXX"
	"XXXX       XXXXX"
	"XXXX       XXXXX"
	"X             XX"
	"X             XX"
	"X             XX"
	"X             XX"
	"X             XX"
	"XXXX       XXXXX"
	"XXXX       XXXXX"
	"XXXXX     XXXXXX"
	"XXXXXX   XXXXXXX"
	"XXXXXXX XXXXXXXX"
	"XXXXXXXXXXXXXXXX";

bool ZUISplitter_Rendered::GetCursor(ZPoint inPoint, ZCursor& outCursor)
	{
	if (this->GetReallyEnabled() && this->GetActive())
		{
		bool pane1Visible, pane2Visible;
		this->Internal_GetVisibility(pane1Visible, pane2Visible);
		if (pane1Visible && pane2Visible)
			{
			bool isHorizontal;
			ZPoint mySize;
			ZCoord firstLength, secondPosition, secondLength;
			this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);
			ZRGBColorMap colorMap[2];
			colorMap[0].fPixval = ' '; colorMap[0].fColor = ZRGBColor::sWhite;
			colorMap[1].fPixval = 'X'; colorMap[1].fColor = ZRGBColor::sBlack;

			if (isHorizontal)
				{
				if (ZRect(0, firstLength, mySize.h, secondPosition).Contains(inPoint))
					{
					ZDCPixmap monoPixmap(ZPoint(16, 16), sSplitterCursorHMono, colorMap, 2);
					ZDCPixmap maskPixmap(ZPoint(16, 16), sSplitterCursorHMask, colorMap, 2);
					outCursor.Set(ZDCPixmap(), monoPixmap, maskPixmap, ZPoint(7, 7));
					return true;
					}
				}
			else
				{
				if (ZRect(firstLength, 0, secondPosition, mySize.v).Contains(inPoint))
					{
					ZDCPixmap monoPixmap(ZPoint(16, 16), sSplitterCursorVMono, colorMap, 2);
					ZDCPixmap maskPixmap(ZPoint(16, 16), sSplitterCursorVMask, colorMap, 2);
					outCursor.Set(ZDCPixmap(), monoPixmap, maskPixmap, ZPoint(7, 7));
					return true;
					}
				}
			}
		}
	return ZUISplitter::GetCursor(inPoint, outCursor);
	}

void ZUISplitter_Rendered::HandleFramePreChange(ZPoint inWindowLocation)
	{
	ZSuperPane::HandleFramePreChange(inWindowLocation);
	bool pane1Visible, pane2Visible;
	this->Internal_GetVisibility(pane1Visible, pane2Visible);
	if (pane1Visible && pane2Visible)
		{
		bool isHorizontal;
		ZPoint mySize;
		ZCoord firstLength, secondPosition, secondLength;
		this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);

		if (isHorizontal)
			fPriorSplitterBounds = ZRect(0, firstLength, mySize.h, secondPosition);
		else
			fPriorSplitterBounds = ZRect(firstLength, 0, secondPosition, mySize.v);
		}
	else
		fPriorSplitterBounds = ZRect::sZero;
	fPriorSplitterBounds += inWindowLocation;
	}

ZDCRgn ZUISplitter_Rendered::HandleFramePostChange(ZPoint inWindowLocation)
	{
	ZDCRgn theRgn = ZUISplitter::HandleFramePostChange(inWindowLocation);

	ZRect newSplitterBounds;
	bool pane1Visible, pane2Visible;
	this->Internal_GetVisibility(pane1Visible, pane2Visible);
	if (pane1Visible && pane2Visible)
		{
		bool isHorizontal;
		ZPoint mySize;
		ZCoord firstLength, secondPosition, secondLength;
		this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);

		if (isHorizontal)
			newSplitterBounds = ZRect(0, firstLength, mySize.h, secondPosition);
		else
			newSplitterBounds = ZRect(firstLength, 0, secondPosition, mySize.v);
		}
	else
		newSplitterBounds = ZRect::sZero;

	newSplitterBounds += inWindowLocation;

	if (newSplitterBounds != fPriorSplitterBounds)
		{
		theRgn |= newSplitterBounds;
		theRgn |= fPriorSplitterBounds;
		}

	return theRgn;
	}

// From ZUISplitter
ZCoord ZUISplitter_Rendered::CalcPane1Size(double inProportion)
	{
	bool isHorizontal = this->GetOrientation();
	ZPoint internalSize = this->GetInternalSize();
	ZCoord splitterThickness = fRenderer->GetSplitterThickness(isHorizontal);
	ZCoord internalLength = internalSize.h;
	if (isHorizontal)
		internalLength = internalSize.v;
	return ZCoord(inProportion * (internalLength - splitterThickness));
	}

ZCoord ZUISplitter_Rendered::GetSplitterThickness()
	{ return fRenderer->GetSplitterThickness(this->GetOrientation()); }

void ZUISplitter_Rendered::GetPanes(ZSuperPane*& outFirstPane, ZSuperPane*& outSecondPane)
	{
	outFirstPane = fFirstPane;
	outSecondPane = fSecondPane;
	}

// From ZPaneLocator
bool ZUISplitter_Rendered::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	if (inPane == fFirstPane || inPane == fSecondPane)
		{
		outLocation = ZPoint::sZero;
		if (inPane == fFirstPane)
			return true;

		bool pane1Visible, pane2Visible;
		this->Internal_GetVisibility(pane1Visible, pane2Visible);
		if (pane1Visible && pane2Visible)
			{
			bool isHorizontal;
			ZPoint mySize;
			ZCoord firstLength, secondPosition, secondLength;
			this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);
	
			if (isHorizontal)
				{
				outLocation.h = 0;
				outLocation.v = secondPosition;
				}
			else
				{
				outLocation.h = secondPosition;
				outLocation.v = 0;
				}
			}
		return true;
		}
	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

bool ZUISplitter_Rendered::GetPaneSize(ZSubPane* inPane, ZPoint& outSize)
	{
	if (inPane == fFirstPane || inPane == fSecondPane)
		{
		bool pane1Visible, pane2Visible;
		this->Internal_GetVisibility(pane1Visible, pane2Visible);
		if (pane1Visible && pane2Visible)
			{
			bool isHorizontal;
			ZPoint mySize;
			ZCoord firstLength, secondPosition, secondLength;
			this->Internal_GetDistances(isHorizontal, mySize, firstLength, secondPosition, secondLength);

			if (inPane == fFirstPane)
				{
				if (isHorizontal)
					{
					outSize.h = mySize.h;
					outSize.v = firstLength;
					}
				else
					{
					outSize.h = firstLength;
					outSize.v = mySize.v;
					}
				return true;
				}
			if (inPane == fSecondPane)
				{
				if (isHorizontal)
					{
					outSize.h = mySize.h;
					outSize.v = secondLength;
					}
				else
					{
					outSize.h = secondLength;
					outSize.v = mySize.v;
					}
				return true;
				}
			}
		else
			{
			outSize = this->GetInternalSize();
			}
		}
	return ZPaneLocator::GetPaneSize(inPane, outSize);
	}

bool ZUISplitter_Rendered::GetPaneVisible(ZSubPane* inPane, bool& outVisible)
	{
	if (inPane == fFirstPane || inPane == fSecondPane)
		{
		bool pane1Visible, pane2Visible;
		this->Internal_GetVisibility(pane1Visible, pane2Visible);
		if (inPane == fFirstPane)
			outVisible = pane1Visible;
		else
			outVisible = pane2Visible;
		return true;
		}
	return ZPaneLocator::GetPaneVisible(inPane, outVisible);
	}

void ZUISplitter_Rendered::Internal_GetVisibility(bool& outPane1Visible, bool& outPane2Visible)
	{
	long paneVisibility = 3;
	if (!this->GetAttribute("ZUISplitter::PaneVisibility", nil, &paneVisibility))
		paneVisibility = 3;
	paneVisibility = max(0L, min(paneVisibility, 3L));

	outPane1Visible = (paneVisibility & 1) != 0;
	outPane2Visible = (paneVisibility & 2) != 0;
	}

void ZUISplitter_Rendered::Internal_GetDistances(bool& outHorizontal, ZPoint& outSize, ZCoord& outFirstLength, ZCoord& outSecondPosition, ZCoord& outSecondLength)
	{
// Note that this should only be called (and only need to be called) when both pane1 and pane2 are visible.
	outHorizontal = this->GetOrientation();
	outSize = this->GetInternalSize();
	ZCoord internalLength = outSize.h;
	if (outHorizontal)
		internalLength = outSize.v;

	ZCoord splitterThickness = fRenderer->GetSplitterThickness(outHorizontal);
	ZCoord paneSize;
	if (this->GetAttribute("ZUISplitter::SizePane1", nil, &paneSize))
		{
		outFirstLength = max(ZCoord(0), min(ZCoord(paneSize), ZCoord(internalLength - splitterThickness)));
		outSecondPosition = outFirstLength + splitterThickness;
		outSecondLength = internalLength - outSecondPosition;
		}
	else if (this->GetAttribute("ZUISplitter::SizePane2", nil, &paneSize))
		{
		outSecondLength = max(ZCoord(0), min(ZCoord(paneSize), ZCoord(internalLength - splitterThickness)));
		outSecondPosition = internalLength - outSecondLength;
		outFirstLength = outSecondPosition - splitterThickness;
		}
	else
		{
		outFirstLength = (internalLength - splitterThickness) / 2;
		outSecondPosition = outFirstLength + splitterThickness;
		outSecondLength = internalLength - outSecondPosition;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider

ZUIRenderer_Divider::ZUIRenderer_Divider()
	{}

ZUIRenderer_Divider::~ZUIRenderer_Divider()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIDivider_Rendered

ZUIDivider_Rendered::ZUIDivider_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Divider> inRenderer)
:	ZUIDivider(inSuperPane, inPaneLocator), fRenderer(inRenderer)
	{}

ZUIDivider_Rendered::~ZUIDivider_Rendered()
	{}

void ZUIDivider_Rendered::Activate(bool entering)
	{
	ZUIDivider::Activate(entering);
	if (fRenderer)
		fRenderer->Activate(this);
	}

void ZUIDivider_Rendered::DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn)
	{
	ZSubPane::DoDraw(inDC, inBoundsRgn);
	if (fRenderer)
		{
		ZRect bounds = inBoundsRgn.Bounds();
		bool isHorizontal = this->GetOrientation();
		ZCoord dividerThickness = fRenderer->GetDividerThickness();
		ZUIDisplayState theState(false, eZTriState_Normal, this->GetReallyEnabled(), false, this->GetActive());
// Divider is drawn centred in our bounds
		if (isHorizontal)
			fRenderer->DrawDivider(inDC, ZPoint(bounds.left, bounds.top + (bounds.Height() - dividerThickness)/2), bounds.Width(), isHorizontal, theState);
		else
			fRenderer->DrawDivider(inDC, ZPoint(bounds.left+(bounds.Width() - dividerThickness)/2, bounds.top), bounds.Height(), isHorizontal, theState);
		}
	}
// From ZUIDivider
ZCoord ZUIDivider_Rendered::GetPreferredThickness()
	{
	if (fRenderer)
		return fRenderer->GetDividerThickness();
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Indirect

ZUIRenderer_CaptionPane_Indirect::ZUIRenderer_CaptionPane_Indirect(const ZRef<ZUIRenderer_CaptionPane>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_CaptionPane_Indirect::~ZUIRenderer_CaptionPane_Indirect()
	{}

void ZUIRenderer_CaptionPane_Indirect::Activate(ZUICaptionPane_Rendered* inCaptionPane)
	{
	if (ZRef<ZUIRenderer_CaptionPane> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inCaptionPane);
	}

void ZUIRenderer_CaptionPane_Indirect::Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	if (ZRef<ZUIRenderer_CaptionPane> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inCaptionPane, inDC, inBounds, inCaption, inState);
	}

ZPoint ZUIRenderer_CaptionPane_Indirect::GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIRenderer_CaptionPane> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetSize(inCaptionPane, inCaption);
	return ZPoint::sZero;
	}

void ZUIRenderer_CaptionPane_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_CaptionPane>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_CaptionPane> ZUIRenderer_CaptionPane_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button_Indirect

ZUIRenderer_Button_Indirect::ZUIRenderer_Button_Indirect(const ZRef<ZUIRenderer_Button>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_Button_Indirect::~ZUIRenderer_Button_Indirect()
	{}

void ZUIRenderer_Button_Indirect::Activate(ZUIButton_Rendered* inButton)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inButton);
	}

void ZUIRenderer_Button_Indirect::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		theRealRenderer->RenderButton(inButton, inDC, inBounds, inCaption, inState);
	}

ZPoint ZUIRenderer_Button_Indirect::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetPreferredDimensions(inButton, inCaption);
	return ZPoint::sZero;
	}

void ZUIRenderer_Button_Indirect::GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		theRealRenderer->GetInsets(inButton, outTopLeftInset, outBottomRightInset);
	}

bool ZUIRenderer_Button_Indirect::ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		return theRealRenderer->ButtonWantsToBecomeTarget(inButton);
	return ZUIRenderer_Button::ButtonWantsToBecomeTarget(inButton);
	}

void ZUIRenderer_Button_Indirect::ButtonBecameWindowTarget(ZUIButton_Rendered* inButton)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		theRealRenderer->ButtonBecameWindowTarget(inButton);
	else
		ZUIRenderer_Button::ButtonBecameWindowTarget(inButton);
	}

void ZUIRenderer_Button_Indirect::ButtonResignedWindowTarget(ZUIButton_Rendered* inButton)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		theRealRenderer->ButtonResignedWindowTarget(inButton);
	else
		ZUIRenderer_Button::ButtonResignedWindowTarget(inButton);
	}

bool ZUIRenderer_Button_Indirect::ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent)
	{
	if (ZRef<ZUIRenderer_Button> theRealRenderer = fRealRenderer)
		return theRealRenderer->ButtonDoKeyDown(inButton, inEvent);
	return ZUIRenderer_Button::ButtonDoKeyDown(inButton, inEvent);
	}

void ZUIRenderer_Button_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_Button>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_Button> ZUIRenderer_Button_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Indirect

ZUIRenderer_ScrollBar_Indirect::ZUIRenderer_ScrollBar_Indirect(const ZRef<ZUIRenderer_ScrollBar>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_ScrollBar_Indirect::~ZUIRenderer_ScrollBar_Indirect()
	{}

void ZUIRenderer_ScrollBar_Indirect::Activate(ZUIScrollBar_Rendered* inScrollBar)
	{
	if (ZRef<ZUIRenderer_ScrollBar> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inScrollBar);
	}

void ZUIRenderer_ScrollBar_Indirect::Draw(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds)
	{
	if (ZRef<ZUIRenderer_ScrollBar> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inScrollBar, inDC, inBounds);
	}

void ZUIRenderer_ScrollBar_Indirect::Click(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZRef<ZUIRenderer_ScrollBar> theRealRenderer = fRealRenderer)
		theRealRenderer->Click(inScrollBar, inHitPoint, inEvent);
	}

ZCoord ZUIRenderer_ScrollBar_Indirect::GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar)
	{
	if (ZRef<ZUIRenderer_ScrollBar> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetPreferredThickness(inScrollBar);
	return 0;
	}

void ZUIRenderer_ScrollBar_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_ScrollBar>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_ScrollBar> ZUIRenderer_ScrollBar_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Indirect

ZUIRenderer_Slider_Indirect::ZUIRenderer_Slider_Indirect(const ZRef<ZUIRenderer_Slider>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_Slider_Indirect::~ZUIRenderer_Slider_Indirect()
	{}

void ZUIRenderer_Slider_Indirect::Activate(ZUISlider_Rendered* inSlider)
	{
	if (ZRef<ZUIRenderer_Slider> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inSlider);
	}

void ZUIRenderer_Slider_Indirect::Draw(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds)
	{
	if (ZRef<ZUIRenderer_Slider> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inSlider, inDC, inBounds);
	}

void ZUIRenderer_Slider_Indirect::Click(ZUISlider_Rendered* inSlider, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZRef<ZUIRenderer_Slider> theRealRenderer = fRealRenderer)
		theRealRenderer->Click(inSlider, inHitPoint, inEvent);
	}

void ZUIRenderer_Slider_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_Slider>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_Slider> ZUIRenderer_Slider_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Indirect

ZUIRenderer_LittleArrows_Indirect::ZUIRenderer_LittleArrows_Indirect(const ZRef<ZUIRenderer_LittleArrows>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_LittleArrows_Indirect::~ZUIRenderer_LittleArrows_Indirect()
	{}

void ZUIRenderer_LittleArrows_Indirect::Activate(ZUILittleArrows_Rendered* inLittleArrows)
	{
	if (ZRef<ZUIRenderer_LittleArrows> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inLittleArrows);
	}

void ZUIRenderer_LittleArrows_Indirect::Draw(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds)
	{
	if (ZRef<ZUIRenderer_LittleArrows> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inLittleArrows, inDC, inBounds);
	}

void ZUIRenderer_LittleArrows_Indirect::Click(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZRef<ZUIRenderer_LittleArrows> theRealRenderer = fRealRenderer)
		theRealRenderer->Click(inLittleArrows, inHitPoint, inEvent);
	}

ZPoint ZUIRenderer_LittleArrows_Indirect::GetPreferredSize()
	{
	if (ZRef<ZUIRenderer_LittleArrows> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetPreferredSize();
	return ZPoint::sZero;
	}

void ZUIRenderer_LittleArrows_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_LittleArrows>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_LittleArrows> ZUIRenderer_LittleArrows_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Indirect
ZUIRenderer_ProgressBar_Indirect::ZUIRenderer_ProgressBar_Indirect(const ZRef<ZUIRenderer_ProgressBar>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_ProgressBar_Indirect::~ZUIRenderer_ProgressBar_Indirect()
	{}

void ZUIRenderer_ProgressBar_Indirect::Activate(ZUIProgressBar_Rendered* inProgressBar)
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inProgressBar);
	}

long ZUIRenderer_ProgressBar_Indirect::GetFrameCount()
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetFrameCount();
	return 0;
	}

ZCoord ZUIRenderer_ProgressBar_Indirect::GetHeight()
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetHeight();
	return 0;
	}

void ZUIRenderer_ProgressBar_Indirect::DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge)
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		theRealRenderer->DrawDeterminate(inDC, inBounds, inEnabled, inActive, inLeftEdge, inRightEdge);
	}

void ZUIRenderer_ProgressBar_Indirect::DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame)
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		theRealRenderer->DrawIndeterminate(inDC, inBounds, inEnabled, inActive, inFrame);
	}

bigtime_t ZUIRenderer_ProgressBar_Indirect::GetInterFrameDuration()
	{
	if (ZRef<ZUIRenderer_ProgressBar> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetInterFrameDuration();
// Default to once per second
	return ZTicks::sPerSecond();
	}

void ZUIRenderer_ProgressBar_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_ProgressBar>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_ProgressBar> ZUIRenderer_ProgressBar_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Indirect

ZUIRenderer_ChasingArrows_Indirect::ZUIRenderer_ChasingArrows_Indirect(const ZRef<ZUIRenderer_ChasingArrows>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_ChasingArrows_Indirect::~ZUIRenderer_ChasingArrows_Indirect()
	{}

void ZUIRenderer_ChasingArrows_Indirect::Activate(ZUIChasingArrows_Rendered* inChasingArrows)
	{
	if (ZRef<ZUIRenderer_ChasingArrows> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inChasingArrows);
	}

long ZUIRenderer_ChasingArrows_Indirect::GetFrameCount()
	{
	if (ZRef<ZUIRenderer_ChasingArrows> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetFrameCount();
	return 0;
	}

ZPoint ZUIRenderer_ChasingArrows_Indirect::GetSize()
	{
	if (ZRef<ZUIRenderer_ChasingArrows> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetSize();
	return ZPoint::sZero;
	}

void ZUIRenderer_ChasingArrows_Indirect::DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame)
	{
	if (ZRef<ZUIRenderer_ChasingArrows> theRealRenderer = fRealRenderer)
		theRealRenderer->DrawFrame(inDC, inBounds, inEnabled, inActive, inFrame);
	}

bigtime_t ZUIRenderer_ChasingArrows_Indirect::GetInterFrameDuration()
	{
	if (ZRef<ZUIRenderer_ChasingArrows> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetInterFrameDuration();
// Default to once per second
	return ZTicks::sPerSecond();
	}

void ZUIRenderer_ChasingArrows_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_ChasingArrows>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_ChasingArrows> ZUIRenderer_ChasingArrows_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Indirect

ZUIRenderer_TabPane_Indirect::ZUIRenderer_TabPane_Indirect(const ZRef<ZUIRenderer_TabPane>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_TabPane_Indirect::~ZUIRenderer_TabPane_Indirect()
	{}

void ZUIRenderer_TabPane_Indirect::Activate(ZUITabPane_Rendered* inTabPane)
	{
	if (ZRef<ZUIRenderer_TabPane> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inTabPane);
	}

ZRect ZUIRenderer_TabPane_Indirect::GetContentArea(ZUITabPane_Rendered* inTabPane)
	{
	if (ZRef<ZUIRenderer_TabPane> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetContentArea(inTabPane);
	return inTabPane->GetSize();
	}

bool ZUIRenderer_TabPane_Indirect::GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk)
	{
	if (ZRef<ZUIRenderer_TabPane> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetContentBackInk(inTabPane, inDC, outInk);
	return false;
	}

void ZUIRenderer_TabPane_Indirect::RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds)
	{
	if (ZRef<ZUIRenderer_TabPane> theRealRenderer = fRealRenderer)
		theRealRenderer->RenderBackground(inTabPane, inDC, inBounds);
	}

bool ZUIRenderer_TabPane_Indirect::DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	if (ZRef<ZUIRenderer_TabPane> theRealRenderer = fRealRenderer)
		return theRealRenderer->DoClick(inTabPane, inHitPoint, inEvent);
	return false;
	}

void ZUIRenderer_TabPane_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_TabPane>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_TabPane> ZUIRenderer_TabPane_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Indirect

ZUIRenderer_Group_Indirect::ZUIRenderer_Group_Indirect(const ZRef<ZUIRenderer_Group>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_Group_Indirect::~ZUIRenderer_Group_Indirect()
	{}

void ZUIRenderer_Group_Indirect::Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize)
	{
	if (ZRef<ZUIRenderer_Group> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inGroup, inTitleSize);
	}

void ZUIRenderer_Group_Indirect::RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize)
	{
	if (ZRef<ZUIRenderer_Group> theRealRenderer = fRealRenderer)
		theRealRenderer->RenderGroup(inDC, inBounds, inEnabled, inActive, inTitleSize);
	}

ZCoord ZUIRenderer_Group_Indirect::GetTitlePaneHLocation()
	{
	if (ZRef<ZUIRenderer_Group> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetTitlePaneHLocation();
	return 0;
	}

ZDCInk ZUIRenderer_Group_Indirect::GetBackInk(const ZDC& inDC)
	{
	if (ZRef<ZUIRenderer_Group> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetBackInk(inDC);
	return ZDCInk();
	}

void ZUIRenderer_Group_Indirect::GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight)
	{
	outTopLeft = outBottomRight = ZPoint::sZero;
	if (ZRef<ZUIRenderer_Group> theRealRenderer = fRealRenderer)
		theRealRenderer->GetInsets(inTitleSize, outTopLeft, outBottomRight);
	}

void ZUIRenderer_Group_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_Group>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_Group> ZUIRenderer_Group_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame_Indirect

ZUIRenderer_FocusFrame_Indirect::ZUIRenderer_FocusFrame_Indirect(const ZRef<ZUIRenderer_FocusFrame>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_FocusFrame_Indirect::~ZUIRenderer_FocusFrame_Indirect()
	{}

void ZUIRenderer_FocusFrame_Indirect::Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	if (ZRef<ZUIRenderer_FocusFrame> theRealRenderer = fRealRenderer)
		theRealRenderer->Draw(inFocusFrame, inDC, inBounds, inState);
	}

void ZUIRenderer_FocusFrame_Indirect::Activate(ZUIFocusFrame_Rendered* inFocusFrame)
	{
	if (ZRef<ZUIRenderer_FocusFrame> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inFocusFrame);
	}

void ZUIRenderer_FocusFrame_Indirect::GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	if (ZRef<ZUIRenderer_FocusFrame> theRealRenderer = fRealRenderer)
		theRealRenderer->GetInsets(inFocusFrame, outTopLeftInset, outBottomRightInset);
	}

ZDCRgn ZUIRenderer_FocusFrame_Indirect::GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds)
	{
	if (ZRef<ZUIRenderer_FocusFrame> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetBorderRgn(inFocusFrame, inBounds);
	return ZDCRgn();
	}

bool ZUIRenderer_FocusFrame_Indirect::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{
	if (ZRef<ZUIRenderer_FocusFrame> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetInternalBackInk(inFocusFrame, inState, inDC, outInk);
	return false;
	}

void ZUIRenderer_FocusFrame_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_FocusFrame>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_FocusFrame> ZUIRenderer_FocusFrame_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter_Indirect

ZUIRenderer_Splitter_Indirect::ZUIRenderer_Splitter_Indirect(const ZRef<ZUIRenderer_Splitter>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}

ZUIRenderer_Splitter_Indirect::~ZUIRenderer_Splitter_Indirect()
	{}

void ZUIRenderer_Splitter_Indirect::Activate(ZUISplitter_Rendered* inSplitter)
	{
	if (ZRef<ZUIRenderer_Splitter> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inSplitter);
	}

ZCoord ZUIRenderer_Splitter_Indirect::GetSplitterThickness(bool inHorizontal)
	{
	if (ZRef<ZUIRenderer_Splitter> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetSplitterThickness(inHorizontal);
	return 0;
	}

void ZUIRenderer_Splitter_Indirect::DrawSplitter(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	if (ZRef<ZUIRenderer_Splitter> theRealRenderer = fRealRenderer)
		theRealRenderer->DrawSplitter(inDC, inLocation, inLength, inHorizontal, inState);
	}

void ZUIRenderer_Splitter_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_Splitter>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_Splitter> ZUIRenderer_Splitter_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Indirect

ZUIRenderer_Divider_Indirect::ZUIRenderer_Divider_Indirect(const ZRef<ZUIRenderer_Divider>& inRealRenderer)
:	fRealRenderer(inRealRenderer)
	{}
ZUIRenderer_Divider_Indirect::~ZUIRenderer_Divider_Indirect()
	{}

void ZUIRenderer_Divider_Indirect::Activate(ZUIDivider_Rendered* inDivider)
	{
	if (ZRef<ZUIRenderer_Divider> theRealRenderer = fRealRenderer)
		theRealRenderer->Activate(inDivider);
	}

ZCoord ZUIRenderer_Divider_Indirect::GetDividerThickness()
	{
	if (ZRef<ZUIRenderer_Divider> theRealRenderer = fRealRenderer)
		return theRealRenderer->GetDividerThickness();
	return 0;
	}

void ZUIRenderer_Divider_Indirect::DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	if (ZRef<ZUIRenderer_Divider> theRealRenderer = fRealRenderer)
		theRealRenderer->DrawDivider(inDC, inLocation, inLength, inHorizontal, inState);
	}

void ZUIRenderer_Divider_Indirect::SetRealRenderer(const ZRef<ZUIRenderer_Divider>& inRealRenderer)
	{ fRealRenderer = inRealRenderer; }

ZRef<ZUIRenderer_Divider> ZUIRenderer_Divider_Indirect::GetRealRenderer()
	{ return fRealRenderer; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory

ZUIRendererFactory::ZUIRendererFactory()
	{}

ZUIRendererFactory::~ZUIRendererFactory()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Caching

ZUIRendererFactory_Caching::ZUIRendererFactory_Caching()
	{}

ZUIRendererFactory_Caching::~ZUIRendererFactory_Caching()
	{}

// From ZUIRendererFactory
ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Caching::Get_Renderer_CaptionPane()
	{
	if (!fRenderer_CaptionPane)
		fRenderer_CaptionPane = this->Make_Renderer_CaptionPane();
	return fRenderer_CaptionPane;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonPush()
	{
	if (!fRenderer_ButtonPush)
		fRenderer_ButtonPush = this->Make_Renderer_ButtonPush();
	return fRenderer_ButtonPush;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonRadio()
	{
	if (!fRenderer_ButtonRadio)
		fRenderer_ButtonRadio = this->Make_Renderer_ButtonRadio();
	return fRenderer_ButtonRadio;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonCheckbox()
	{
	if (!fRenderer_ButtonCheckbox)
		fRenderer_ButtonCheckbox = this->Make_Renderer_ButtonCheckbox();
	return fRenderer_ButtonCheckbox;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonPopup()
	{
	if (!fRenderer_ButtonPopup)
		fRenderer_ButtonPopup = this->Make_Renderer_ButtonPopup();
	return fRenderer_ButtonPopup;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonBevel()
	{
	if (!fRenderer_ButtonBevel)
		fRenderer_ButtonBevel = this->Make_Renderer_ButtonBevel();
	return fRenderer_ButtonBevel;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonDisclosure()
	{
	if (!fRenderer_ButtonDisclosure)
		fRenderer_ButtonDisclosure = this->Make_Renderer_ButtonDisclosure();
	return fRenderer_ButtonDisclosure;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Caching::Get_Renderer_ButtonPlacard()
	{
	if (!fRenderer_ButtonPlacard)
		fRenderer_ButtonPlacard = this->Make_Renderer_ButtonPlacard();
	return fRenderer_ButtonPlacard;
	}

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Caching::Get_Renderer_ScrollBar()
	{
	if (!fRenderer_ScrollBar)
		fRenderer_ScrollBar = this->Make_Renderer_ScrollBar();
	return fRenderer_ScrollBar;
	}

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Caching::Get_Renderer_Slider()
	{
	if (!fRenderer_Slider)
		fRenderer_Slider = this->Make_Renderer_Slider();
	return fRenderer_Slider;
	}

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Caching::Get_Renderer_LittleArrows()
	{
	if (!fRenderer_LittleArrows)
		fRenderer_LittleArrows = this->Make_Renderer_LittleArrows();
	return fRenderer_LittleArrows;
	}

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Caching::Get_Renderer_ProgressBar()
	{
	if (!fRenderer_ProgressBar)
		fRenderer_ProgressBar = this->Make_Renderer_ProgressBar();
	return fRenderer_ProgressBar;
	}

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Caching::Get_Renderer_ChasingArrows()
	{
	if (!fRenderer_ChasingArrows)
		fRenderer_ChasingArrows = this->Make_Renderer_ChasingArrows();
	return fRenderer_ChasingArrows;
	}

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Caching::Get_Renderer_TabPane()
	{
	if (!fRenderer_TabPane)
		fRenderer_TabPane = this->Make_Renderer_TabPane();
	return fRenderer_TabPane;
	}

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Caching::Get_Renderer_GroupPrimary()
	{
	if (!fRenderer_GroupPrimary)
		fRenderer_GroupPrimary = this->Make_Renderer_GroupPrimary();
	return fRenderer_GroupPrimary;
	}

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Caching::Get_Renderer_GroupSecondary()
	{
	if (!fRenderer_GroupSecondary)
		fRenderer_GroupSecondary = this->Make_Renderer_GroupSecondary();
	return fRenderer_GroupSecondary;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Caching::Get_Renderer_FocusFrameEditText()
	{
	if (!fRenderer_FocusFrameEditText)
		fRenderer_FocusFrameEditText = this->Make_Renderer_FocusFrameEditText();
	return fRenderer_FocusFrameEditText;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Caching::Get_Renderer_FocusFrameListBox()
	{
	if (!fRenderer_FocusFrameListBox)
		fRenderer_FocusFrameListBox = this->Make_Renderer_FocusFrameListBox();
	return fRenderer_FocusFrameListBox;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Caching::Get_Renderer_FocusFrameDateControl()
	{
	if (!fRenderer_FocusFrameDateControl)
		fRenderer_FocusFrameDateControl = this->Make_Renderer_FocusFrameDateControl();
	return fRenderer_FocusFrameDateControl;
	}

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Caching::Get_Renderer_Splitter()
	{
	if (!fRenderer_Splitter)
		fRenderer_Splitter = this->Make_Renderer_Splitter();
	return fRenderer_Splitter;
	}

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Caching::Get_Renderer_Divider()
	{
	if (!fRenderer_Divider)
		fRenderer_Divider = this->Make_Renderer_Divider();
	return fRenderer_Divider;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Indirect

ZUIRendererFactory_Indirect::ZUIRendererFactory_Indirect(const ZRef<ZUIRendererFactory>& realFactory)
:	fRealFactory(realFactory)
	{}

ZUIRendererFactory_Indirect::~ZUIRendererFactory_Indirect()
	{}

ZRef<ZUIRendererFactory> ZUIRendererFactory_Indirect::GetRealFactory()
	{ return fRealFactory; }

void ZUIRendererFactory_Indirect::SetRealFactory(ZRef<ZUIRendererFactory> realFactory)
	{ fRealFactory = realFactory; }

string ZUIRendererFactory_Indirect::GetName()
	{ return fRealFactory->GetName(); }

ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Indirect::Get_Renderer_CaptionPane()
	{ return fRealFactory->Get_Renderer_CaptionPane(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonPush()
	{ return fRealFactory->Get_Renderer_ButtonPush(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonRadio()
	{ return fRealFactory->Get_Renderer_ButtonRadio(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonCheckbox()
	{ return fRealFactory->Get_Renderer_ButtonCheckbox(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonPopup()
	{ return fRealFactory->Get_Renderer_ButtonPopup(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonBevel()
	{ return fRealFactory->Get_Renderer_ButtonBevel(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonDisclosure()
	{ return fRealFactory->Get_Renderer_ButtonDisclosure(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Indirect::Get_Renderer_ButtonPlacard()
	{ return fRealFactory->Get_Renderer_ButtonPlacard(); }

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Indirect::Get_Renderer_ScrollBar()
	{ return fRealFactory->Get_Renderer_ScrollBar(); }

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Indirect::Get_Renderer_Slider()
	{ return fRealFactory->Get_Renderer_Slider(); }

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Indirect::Get_Renderer_LittleArrows()
	{ return fRealFactory->Get_Renderer_LittleArrows(); }

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Indirect::Get_Renderer_ProgressBar()
	{ return fRealFactory->Get_Renderer_ProgressBar(); }

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Indirect::Get_Renderer_ChasingArrows()
	{ return fRealFactory->Get_Renderer_ChasingArrows(); }

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Indirect::Get_Renderer_TabPane()
	{ return fRealFactory->Get_Renderer_TabPane(); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Indirect::Get_Renderer_GroupPrimary()
	{ return fRealFactory->Get_Renderer_GroupPrimary(); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Indirect::Get_Renderer_GroupSecondary()
	{ return fRealFactory->Get_Renderer_GroupSecondary(); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Indirect::Get_Renderer_FocusFrameEditText()
	{ return fRealFactory->Get_Renderer_FocusFrameEditText(); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Indirect::Get_Renderer_FocusFrameListBox()
	{ return fRealFactory->Get_Renderer_FocusFrameListBox(); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Indirect::Get_Renderer_FocusFrameDateControl()
	{ return fRealFactory->Get_Renderer_FocusFrameDateControl(); }

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Indirect::Get_Renderer_Splitter()
	{ return fRealFactory->Get_Renderer_Splitter(); }

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Indirect::Get_Renderer_Divider()
	{ return fRealFactory->Get_Renderer_Divider(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFactory_Rendered

ZUIFactory_Rendered::ZUIFactory_Rendered(ZRef<ZUIRendererFactory> inRendererFactory)
:	fRendererFactory(inRendererFactory)
	{}

ZUIFactory_Rendered::~ZUIFactory_Rendered()
	{}

ZRef<ZUIRendererFactory> ZUIFactory_Rendered::GetRendererFactory()
	{ return fRendererFactory; }

void ZUIFactory_Rendered::SetRendererFactory(ZRef<ZUIRendererFactory> inRendererFactory)
	{ fRendererFactory = inRendererFactory; }

// From ZUIFactory
ZUICaptionPane* ZUIFactory_Rendered::Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption)
	{ return new ZUICaptionPane_Rendered(inSuperPane, inLocator, inCaption, fRendererFactory->Get_Renderer_CaptionPane()); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonPush(), inCaption); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonRadio(), inCaption); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonCheckbox(), inCaption); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonPopup(), inCaption); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonBevel(), inCaption); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonDisclosure(), ZRef<ZUICaption>()); }

ZUIButton* ZUIFactory_Rendered::Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{ return new ZUIButton_Rendered(inSuperPane, inLocator, inResponder, fRendererFactory->Get_Renderer_ButtonPlacard(), inCaption); }

ZUIScrollBar* ZUIFactory_Rendered::Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool withBorder)
	{ return new ZUIScrollBar_Rendered(inSuperPane, inLocator, withBorder, fRendererFactory->Get_Renderer_ScrollBar()); }

ZUISlider* ZUIFactory_Rendered::Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUISlider_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_Slider()); }

ZUILittleArrows* ZUIFactory_Rendered::Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUILittleArrows_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_LittleArrows()); }

ZUIProgressBar* ZUIFactory_Rendered::Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIProgressBar_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_ProgressBar()); }

ZUIChasingArrows* ZUIFactory_Rendered::Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIChasingArrows_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_ChasingArrows()); }

ZUITabPane* ZUIFactory_Rendered::Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUITabPane_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_TabPane()); }

ZUIGroup* ZUIFactory_Rendered::Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIGroup_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_GroupPrimary()); }

ZUIGroup* ZUIFactory_Rendered::Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIGroup_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_GroupSecondary()); }

ZUIFocusFrame* ZUIFactory_Rendered::Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIFocusFrame_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_FocusFrameEditText()); }

ZUIFocusFrame* ZUIFactory_Rendered::Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIFocusFrame_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_FocusFrameListBox()); }

ZUIFocusFrame* ZUIFactory_Rendered::Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIFocusFrame_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_FocusFrameDateControl()); }

ZUISplitter* ZUIFactory_Rendered::Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUISplitter_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_Splitter()); }

ZUIDivider* ZUIFactory_Rendered::Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{ return new ZUIDivider_Rendered(inSuperPane, inLocator, fRendererFactory->Get_Renderer_Divider()); }

// =================================================================================================
