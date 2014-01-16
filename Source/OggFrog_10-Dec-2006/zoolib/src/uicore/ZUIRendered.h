/*  @(#) $Id: ZUIRendered.h,v 1.5 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUIRendered__
#define __ZUIRendered__ 1
#include "zconfig.h"

#include "ZMouseTracker.h"
#include "ZPane.h"
#include "ZRefCount.h"
#include "ZUI.h"
#include "ZUIFactory.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane

class ZUICaptionPane_Rendered;

class ZUIRenderer_CaptionPane : public ZRefCounted
	{
public:
	ZUIRenderer_CaptionPane();
	virtual ~ZUIRenderer_CaptionPane();

	virtual void Activate(ZUICaptionPane_Rendered* inCaptionPane) = 0;
	virtual void Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState) = 0;
	virtual ZPoint GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionPane_Rendered

class ZUICaptionPane_Rendered : public ZUICaptionPane
	{
public:
	ZUICaptionPane_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption, ZRef<ZUIRenderer_CaptionPane> inRenderer);
	virtual ~ZUICaptionPane_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane via ZUICaptionPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);

	virtual ZPoint GetPreferredSize();

// Our protocol
	ZRef<ZUIRenderer_CaptionPane> GetRenderer();
	void SetRenderer(ZRef<ZUIRenderer_CaptionPane> inRenderer, bool inRefresh);

protected:
	ZRef<ZUIRenderer_CaptionPane> fRenderer;
	};

// =================================================================================================
// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button

class ZUIButton_Rendered;

class ZUIRenderer_Button : public ZRefCounted
	{
public:
	ZUIRenderer_Button();
	virtual ~ZUIRenderer_Button();

	virtual void Activate(ZUIButton_Rendered* inButton) = 0;
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState) = 0;
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	virtual ZPoint GetPopupLocation(ZUIButton_Rendered* inButton);

	virtual void GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual bool ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonBecameWindowTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonResignedWindowTarget(ZUIButton_Rendered* inButton);
	virtual bool ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton_Rendered

class ZUIButton_Rendered : public ZUIButton_Tracked
	{
public:
	ZUIButton_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUIRenderer_Button> inRenderer, ZRef<ZUICaption> inCaption);
	virtual ~ZUIButton_Rendered();

// From ZEventHr (& ZSubPane)
	virtual bool WantsToBecomeTarget();

// From ZEventHr via ZUIButtonTracked
	virtual void Activate(bool entering);
	virtual void HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual void HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);

// From ZUIButton_Tracked/ZUIButton
	virtual void PrivateDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn, bool inPressed, EZTriState inHilite);
	virtual ZPoint CalcPreferredSize();
	virtual ZPoint GetPopupLocation();
	virtual void GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZPoint GetLocationInset();
	virtual ZPoint GetSizeInset();
	virtual void SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh);

// Our protocol
	ZRef<ZUIRenderer_Button> GetRenderer();
	void SetRenderer(ZRef<ZUIRenderer_Button> inRenderer, bool inRefresh);

	ZRef<ZUICaption> GetCaption();

protected:
	ZRef<ZUIRenderer_Button> fRenderer;
	ZRef<ZUICaption> fCaption;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar

class ZUIScrollBar_Rendered;
class ZUIRenderer_ScrollBar : public ZRefCounted
	{
public:
	ZUIRenderer_ScrollBar();
	virtual ~ZUIRenderer_ScrollBar();

	virtual void Activate(ZUIScrollBar_Rendered* inScrollBar) = 0;
	virtual void Draw(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds) = 0;
	virtual void Click(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZEvent_Mouse& inEvent) = 0;

	virtual ZCoord GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollBar_Rendered

class ZUIScrollBar_Rendered : public ZUIScrollBar
	{
public:
	ZUIScrollBar_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, bool inWithBorder, ZRef<ZUIRenderer_ScrollBar> inRenderer);
	virtual ~ZUIScrollBar_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// From ZUIScrollBar
	virtual ZCoord GetPreferredThickness();

	bool GetWithBorder();
protected:
	bool fWithBorder;
	ZRef<ZUIRenderer_ScrollBar> fRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Base

class ZUIRenderer_ScrollBar_Base : public ZUIRenderer_ScrollBar
	{
public:
	ZUIRenderer_ScrollBar_Base();
	virtual ~ZUIRenderer_ScrollBar_Base();

// From ZUIRenderer_ScrollBar
	virtual void Draw(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// Our protocol
	virtual void RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inBorder, double inValue, double inThumbProportion,  double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart) = 0;
	virtual ZUIScrollBar::HitPart FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion) = 0;
	virtual double GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta) = 0;
protected:
	class Tracker;
	class ThumbTracker;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider

class ZUISlider_Rendered;
class ZUIRenderer_Slider : public ZRefCounted
	{
public:
	ZUIRenderer_Slider();
	virtual ~ZUIRenderer_Slider();

	virtual void Activate(ZUISlider_Rendered* inSlider) = 0;
	virtual void Draw(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds) = 0;
	virtual void Click(ZUISlider_Rendered* inSlider, ZPoint inHitPoint, const ZEvent_Mouse& inEvent) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUISlider_Rendered

class ZUISlider_Rendered : public ZUISlider
	{
public:
	ZUISlider_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Slider> inRenderer);
	virtual ~ZUISlider_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

protected:
	ZRef<ZUIRenderer_Slider> fRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Base

class ZUIRenderer_Slider_Base : public ZUIRenderer_Slider
	{
public:
	ZUIRenderer_Slider_Base();
	virtual ~ZUIRenderer_Slider_Base();

// From ZUIRenderer_Slider
	virtual void Draw(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUISlider_Rendered* inSlider, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// Our protocol
	virtual void RenderSlider(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, double inValue, double inGhostThumbValue, bool inThumbHilited) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows

class ZUILittleArrows_Rendered;
class ZUIRenderer_LittleArrows : public ZRefCounted
	{
public:
	ZUIRenderer_LittleArrows();
	virtual ~ZUIRenderer_LittleArrows();

	virtual void Activate(ZUILittleArrows_Rendered* inLittleArrows) = 0;
	virtual void Draw(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds) = 0;
	virtual void Click(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZEvent_Mouse& inEvent) = 0;

	virtual ZPoint GetPreferredSize() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUILittleArrows_Rendered

class ZUILittleArrows_Rendered : public ZUILittleArrows
	{
public:
	ZUILittleArrows_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_LittleArrows> inRenderer);
	virtual ~ZUILittleArrows_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual ZPoint GetSize();
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// From ZUILittleArrows
	virtual ZPoint GetPreferredSize();

protected:
	ZRef<ZUIRenderer_LittleArrows> fRenderer;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Base

class ZUIRenderer_LittleArrows_Base : public ZUIRenderer_LittleArrows
	{
public:
	ZUIRenderer_LittleArrows_Base();
	virtual ~ZUIRenderer_LittleArrows_Base();

// From ZUIRenderer_LittleArrows
	virtual void Draw(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

// Our protocol
	virtual void RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart) = 0;
	virtual ZUILittleArrows::HitPart FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled) = 0;

	class Tracker;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar

class ZUIProgressBar_Rendered;
class ZUIRenderer_ProgressBar : public ZRefCounted
	{
public:
	ZUIRenderer_ProgressBar();
	virtual ~ZUIRenderer_ProgressBar();

	virtual void Activate(ZUIProgressBar_Rendered* inProgressBar) = 0;
	virtual long GetFrameCount() = 0;
	virtual ZCoord GetHeight() = 0;
	virtual void DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inleftEdge, double inRightEdge) = 0;
	virtual void DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame) = 0;
	virtual bigtime_t GetInterFrameDuration() = 0;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZUIProgressBar_Rendered

class ZUIProgressBar_Rendered : public ZUIProgressBar
	{
public:
	ZUIProgressBar_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_ProgressBar> inRenderer);
	virtual ~ZUIProgressBar_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual ZPoint GetSize();

// From ZUIProgressBar
	virtual bigtime_t NextFrame();
protected:
	ZRef<ZUIRenderer_ProgressBar> fRenderer;
	long fCurrentFrame;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows

class ZUIChasingArrows_Rendered;
class ZUIRenderer_ChasingArrows : public ZRefCounted
	{
public:
	ZUIRenderer_ChasingArrows();
	virtual ~ZUIRenderer_ChasingArrows();

	virtual void Activate(ZUIChasingArrows_Rendered* inChasingArrows) = 0;
	virtual long GetFrameCount() = 0;
	virtual ZPoint GetSize() = 0;
	virtual void DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame) = 0;
	virtual bigtime_t GetInterFrameDuration() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIChasingArrows_Rendered

class ZUIChasingArrows_Rendered : public ZUIChasingArrows
	{
public:
	ZUIChasingArrows_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_ChasingArrows> inRenderer);

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual ZPoint GetSize();

// From ZUIChasingArrows
	virtual bigtime_t NextFrame();
protected:
	ZRef<ZUIRenderer_ChasingArrows> fRenderer;
	long fCurrentFrame;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane

class ZUITabPane_Rendered;

class ZUIRenderer_TabPane : public ZRefCounted
	{
public:
	ZUIRenderer_TabPane();
	virtual ~ZUIRenderer_TabPane();

	virtual void Activate(ZUITabPane_Rendered* inTabPane) = 0;
	virtual ZRect GetContentArea(ZUITabPane_Rendered* inTabPane) = 0;
	virtual bool GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& inInk) = 0;

	virtual void RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds) = 0;
	virtual bool DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent) = 0;
	}; 

// =================================================================================================
#pragma mark -
#pragma mark * ZUITabPane_Rendered

class ZUITabPane_Rendered : public ZUITabPane, public ZPaneLocator
	{
public:
	ZUITabPane_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_TabPane> inRenderer);
	virtual ~ZUITabPane_Rendered();

// From ZUIAlternatePane
	virtual void DeletePane(ZSuperPane* inPane);
	virtual void SetCurrentPane(ZSuperPane* inPane);
	virtual ZSuperPane* GetCurrentPane();

// From ZUITabPane
	virtual ZSuperPane* CreateTab(ZRef<ZUICaption> inCaption);

	void SetCurrentTabCaption(ZRef<ZUICaption> inCaption);
	ZRef<ZUICaption> GetCurrentTabCaption();

// From ZPaneLocator
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
	virtual bool GetPaneVisible(ZSubPane* inPane, bool& outVisible);

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSuperPane/ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);

	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool GetPaneBackInk(ZSubPane* inPane, const ZDC& inDC, ZDCInk& outInk);
	virtual void SubPaneRemoved(ZSubPane* inPane);

	virtual ZDCRgn DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds);

// Our protocol
	ZRef<ZUIRenderer_TabPane> GetRenderer();
	const vector<ZRef<ZUICaption> >& GetCaptionVector();
protected:
	class ContentPane;
	friend class ContentPane;

	vector<ContentPane*> fContentPanes;
	vector<ZRef<ZUICaption> > fTabCaptions;
	ZSuperPane* fCurrentContentPane;
	ZSuperPane* fContentWrapper;
	ZRef<ZUIRenderer_TabPane> fRenderer;
	};

// =================================================================================================
// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Base

class ZUIRenderer_TabPane_Base : public ZUIRenderer_TabPane
	{
public:
	ZUIRenderer_TabPane_Base();
	virtual ~ZUIRenderer_TabPane_Base();

	virtual bool DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual ZRect CalcTabRect(ZUITabPane_Rendered* inTabPane, ZRef<ZUICaption> inCaption) = 0;
	virtual void DrawTab(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZRef<ZUICaption> inCaption, const ZRect& inTabRect, const ZUIDisplayState& inState) = 0;
protected:
	class Tracker;
	friend class Tracker;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Base::Tracker

class ZUIRenderer_TabPane_Base::Tracker : public ZMouseTracker
	{
public:
	Tracker(ZRef<ZUIRenderer_TabPane_Base> inRenderer, ZUITabPane_Rendered* inTabPane, ZRef<ZUICaption> inHitCaption, ZPoint inStartPoint);
	virtual void TrackIt(Phase inPhase, ZPoint inAnchor, ZPoint inPrevious, ZPoint inNext, bool inMoved);
protected:
	ZRef<ZUIRenderer_TabPane_Base> fRenderer;
	ZUITabPane_Rendered* fTabPane;
	ZRef<ZUICaption> fCaption;
	bool fWasPressed;
	} ;

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group

class ZUIGroup_Rendered;
class ZUIRenderer_Group : public ZRefCounted
	{
public:
	ZUIRenderer_Group();
	virtual ~ZUIRenderer_Group();

	virtual void Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize) = 0;
	virtual void RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize) = 0;
	virtual ZCoord GetTitlePaneHLocation() = 0;
	virtual ZDCInk GetBackInk(const ZDC& inDC) = 0;
	virtual void GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGroup_Rendered

class ZUIGroup_Rendered : public ZUIGroup
	{
public:
	ZUIGroup_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUIRenderer_Group> inRenderer);
	virtual ~ZUIGroup_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSuperPane via ZUIGroup
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual ZDCInk GetBackInk(const ZDC& inDC);

// From ZUIGroup
	virtual void GetInsets(ZPoint& outTopLeft, ZPoint& outTopRight);
	virtual ZSuperPane* GetTitlePane();
	virtual ZSuperPane* GetInternalPane();

protected:
// Called by TitlePane
	ZCoord GetTitlePaneHLocation();

	class TitlePane;
	friend class TitlePane;

	class InternalPane;

	TitlePane* fTitlePane;
	InternalPane* fInternalPane;
	ZRef<ZUIRenderer_Group> fRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame

class ZUIFocusFrame_Rendered;
class ZUIRenderer_FocusFrame : public ZRefCounted
	{
public:
	ZUIRenderer_FocusFrame();
	virtual ~ZUIRenderer_FocusFrame();

	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame) = 0;
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState) = 0;
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset) = 0;
	virtual ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds) = 0;
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFocusFrame_Rendered

class ZUIFocusFrame_Rendered : public ZUIFocusFrame
	{
public:
	ZUIFocusFrame_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_FocusFrame> inRenderer);
	virtual ~ZUIFocusFrame_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual ZDCRgn DoFramePostChange(const ZRect& inOldBounds, const ZRect& inNewBounds);
	virtual ZDCInk GetInternalBackInk(const ZDC& inDC);
	virtual ZPoint GetTranslation();
	virtual ZPoint GetInternalSize();

// From ZUIFocusFrame
	virtual void GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZDCRgn GetBorderRgn();

protected:
	ZRef<ZUIRenderer_FocusFrame> fRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter

class ZUISplitter_Rendered;
class ZUIRenderer_Splitter : public ZRefCounted
	{
public:
	ZUIRenderer_Splitter();
	virtual ~ZUIRenderer_Splitter();

	virtual void Activate(ZUISplitter_Rendered* inSplitter) = 0;
	virtual ZCoord GetSplitterThickness(bool inHorizontal) = 0;
	virtual void DrawSplitter(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter_Standard

class ZUIRenderer_Splitter_Standard : public ZUIRenderer_Splitter
	{
public:
	ZUIRenderer_Splitter_Standard();
	virtual ~ZUIRenderer_Splitter_Standard();

	virtual void Activate(ZUISplitter_Rendered* inSplitter);
	virtual ZCoord GetSplitterThickness(bool inHorizontal);
	virtual void DrawSplitter(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUISplitter_Rendered

class ZUISplitter_Rendered : public ZUISplitter, public ZPaneLocator
	{
public:
	ZUISplitter_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Splitter> inRenderer);
	virtual ~ZUISplitter_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSuperPane/ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual bool GetCursor(ZPoint inPoint, ZCursor& outCursor);

	virtual void HandleFramePreChange(ZPoint inWindowLocation);
	virtual ZDCRgn HandleFramePostChange(ZPoint inWindowLocation);

// From ZUISplitter
	virtual ZCoord CalcPane1Size(double inProportion);
	virtual ZCoord GetSplitterThickness();
	virtual void GetPanes(ZSuperPane*& outFirstPane, ZSuperPane*& outSecondPane);

// From ZPaneLocator
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
	virtual bool GetPaneVisible(ZSubPane* inPane, bool& outVisible);

protected:
	void Internal_GetVisibility(bool& outPane1Visible, bool& outPane2Visible);
	void Internal_GetDistances(bool& outHorizontal, ZPoint& outSize, ZCoord& outFirstLength, ZCoord& outSecondPosition, ZCoord& outSecondLength);

	ZRef<ZUIRenderer_Splitter> fRenderer;
	ZSuperPane* fFirstPane;
	ZSuperPane* fSecondPane;

	class Tracker;
	friend class Tracker;

private:
	ZRect fPriorSplitterBounds;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider

class ZUIDivider_Rendered;
class ZUIRenderer_Divider : public ZRefCounted
	{
public:
	ZUIRenderer_Divider();
	virtual ~ZUIRenderer_Divider();

	virtual void Activate(ZUIDivider_Rendered* inDivider) = 0;
	virtual ZCoord GetDividerThickness() = 0;
	virtual void DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIDivider_Rendered

class ZUIDivider_Rendered : public ZUIDivider
	{
public:
	ZUIDivider_Rendered(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUIRenderer_Divider> inRenderer);
	virtual ~ZUIDivider_Rendered();

// From ZEventHr
	virtual void Activate(bool entering);

// From ZSubPane
	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);

// From ZUIDivider
	virtual ZCoord GetPreferredThickness();

protected:
	ZRef<ZUIRenderer_Divider> fRenderer;
	};

// =================================================================================================
// Indirect renderers -- take a real renderer as a parameter and pass calls along
// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Indirect

class ZUIRenderer_CaptionPane_Indirect : public ZUIRenderer_CaptionPane
	{
public:
	ZUIRenderer_CaptionPane_Indirect(const ZRef<ZUIRenderer_CaptionPane>& inRealRenderer);
	virtual ~ZUIRenderer_CaptionPane_Indirect();

	virtual void Activate(ZUICaptionPane_Rendered* inCaptionPane);
	virtual void Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption);

	void SetRealRenderer(const ZRef<ZUIRenderer_CaptionPane>& inRealRenderer);
	ZRef<ZUIRenderer_CaptionPane> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_CaptionPane> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button_Indirect

class ZUIRenderer_Button_Indirect : public ZUIRenderer_Button
	{
public:
	ZUIRenderer_Button_Indirect(const ZRef<ZUIRenderer_Button>& inRealRenderer);
	virtual ~ZUIRenderer_Button_Indirect();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inButtonRect, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	virtual void GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual bool ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonBecameWindowTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonResignedWindowTarget(ZUIButton_Rendered* inButton);
	virtual bool ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent);

	void SetRealRenderer(const ZRef<ZUIRenderer_Button>& inRealRenderer);
	ZRef<ZUIRenderer_Button> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_Button> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Indirect

class ZUIRenderer_ScrollBar_Indirect : public ZUIRenderer_ScrollBar
	{
public:
	ZUIRenderer_ScrollBar_Indirect(const ZRef<ZUIRenderer_ScrollBar>& inRealRenderer);
	virtual ~ZUIRenderer_ScrollBar_Indirect();

	virtual void Activate(ZUIScrollBar_Rendered* inScrollBar);
	virtual void Draw(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual ZCoord GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar);

	void SetRealRenderer(const ZRef<ZUIRenderer_ScrollBar>& inRealRenderer);
	ZRef<ZUIRenderer_ScrollBar> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_ScrollBar> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Indirect

class ZUIRenderer_Slider_Indirect : public ZUIRenderer_Slider
	{
public:
	ZUIRenderer_Slider_Indirect(const ZRef<ZUIRenderer_Slider>& inRealRenderer);
	virtual ~ZUIRenderer_Slider_Indirect();

	virtual void Activate(ZUISlider_Rendered* inSlider);
	virtual void Draw(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUISlider_Rendered* inSlider, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	void SetRealRenderer(const ZRef<ZUIRenderer_Slider>& inRealRenderer);
	ZRef<ZUIRenderer_Slider> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_Slider> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Indirect

class ZUIRenderer_LittleArrows_Indirect : public ZUIRenderer_LittleArrows
	{
public:
	ZUIRenderer_LittleArrows_Indirect(const ZRef<ZUIRenderer_LittleArrows>& inRealRenderer);
	virtual ~ZUIRenderer_LittleArrows_Indirect();

	virtual void Activate(ZUILittleArrows_Rendered* inLittleArrows);
	virtual void Draw(ZUILittleArrows_Rendered* inLittleArrows,const ZDC& inDC, const ZRect& inBounds);
	virtual void Click(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	virtual ZPoint GetPreferredSize();

	void SetRealRenderer(const ZRef<ZUIRenderer_LittleArrows>& inRealRenderer);
	ZRef<ZUIRenderer_LittleArrows> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_LittleArrows> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Indirect

class ZUIRenderer_ProgressBar_Indirect : public ZUIRenderer_ProgressBar
	{
public:
	ZUIRenderer_ProgressBar_Indirect(const ZRef<ZUIRenderer_ProgressBar>& inRealRenderer);
	virtual ~ZUIRenderer_ProgressBar_Indirect();

	virtual void Activate(ZUIProgressBar_Rendered* inProgressBar);
	virtual long GetFrameCount();
	virtual ZCoord GetHeight();
	virtual void DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge);
	virtual void DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame);
	virtual bigtime_t GetInterFrameDuration();

	void SetRealRenderer(const ZRef<ZUIRenderer_ProgressBar>& inRealRenderer);
	ZRef<ZUIRenderer_ProgressBar> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_ProgressBar> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Indirect

class ZUIRenderer_ChasingArrows_Indirect : public ZUIRenderer_ChasingArrows
	{
public:
	ZUIRenderer_ChasingArrows_Indirect(const ZRef<ZUIRenderer_ChasingArrows>& inRealRenderer);
	virtual ~ZUIRenderer_ChasingArrows_Indirect();

	virtual void Activate(ZUIChasingArrows_Rendered* inChasingArrows);
	virtual long GetFrameCount();
	virtual ZPoint GetSize();
	virtual void DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame);
	virtual bigtime_t GetInterFrameDuration();

	void SetRealRenderer(const ZRef<ZUIRenderer_ChasingArrows>& inRealRenderer);
	ZRef<ZUIRenderer_ChasingArrows> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_ChasingArrows> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Indirect

class ZUIRenderer_TabPane_Indirect : public ZUIRenderer_TabPane
	{
public:
	ZUIRenderer_TabPane_Indirect(const ZRef<ZUIRenderer_TabPane>& inRealRenderer);
	virtual ~ZUIRenderer_TabPane_Indirect();

	virtual void Activate(ZUITabPane_Rendered* inTabPane);
	virtual ZRect GetContentArea(ZUITabPane_Rendered* inTabPane);
	virtual bool GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& inInk);

	virtual void RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds);
	virtual bool DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	void SetRealRenderer(const ZRef<ZUIRenderer_TabPane>& inRealRenderer);
	ZRef<ZUIRenderer_TabPane> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_TabPane> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Indirect

class ZUIRenderer_Group_Indirect : public ZUIRenderer_Group
	{
public:
	ZUIRenderer_Group_Indirect(const ZRef<ZUIRenderer_Group>& inRealRenderer);
	virtual ~ZUIRenderer_Group_Indirect();

	virtual void Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize);
	virtual void RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize);
	virtual ZCoord GetTitlePaneHLocation();
	virtual ZDCInk GetBackInk(const ZDC& inDC);
	virtual void GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight);

	void SetRealRenderer(const ZRef<ZUIRenderer_Group>& inRealRenderer);
	ZRef<ZUIRenderer_Group> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_Group> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame_Indirect

class ZUIRenderer_FocusFrame_Indirect : public ZUIRenderer_FocusFrame
	{
public:
	ZUIRenderer_FocusFrame_Indirect(const ZRef<ZUIRenderer_FocusFrame>& inRealRenderer);
	virtual ~ZUIRenderer_FocusFrame_Indirect();

	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame);
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds);
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);

	void SetRealRenderer(const ZRef<ZUIRenderer_FocusFrame>& inRealRenderer);
	ZRef<ZUIRenderer_FocusFrame> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_FocusFrame> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Splitter_Indirect

class ZUIRenderer_Splitter_Indirect : public ZUIRenderer_Splitter
	{
public:
	ZUIRenderer_Splitter_Indirect(const ZRef<ZUIRenderer_Splitter>& inRealRenderer);
	virtual ~ZUIRenderer_Splitter_Indirect();

	virtual void Activate(ZUISplitter_Rendered* inSplitter);
	virtual ZCoord GetSplitterThickness(bool inHorizontal);
	virtual void DrawSplitter(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);

	void SetRealRenderer(const ZRef<ZUIRenderer_Splitter>& inRealRenderer);
	ZRef<ZUIRenderer_Splitter> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_Splitter> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Indirect

class ZUIRenderer_Divider_Indirect : public ZUIRenderer_Divider
	{
public:
	ZUIRenderer_Divider_Indirect(const ZRef<ZUIRenderer_Divider>& inRealRenderer);
	virtual ~ZUIRenderer_Divider_Indirect();

	virtual void Activate(ZUIDivider_Rendered* inDivider);
	virtual ZCoord GetDividerThickness();
	virtual void DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);

	void SetRealRenderer(const ZRef<ZUIRenderer_Divider>& inRealRenderer);
	ZRef<ZUIRenderer_Divider> GetRealRenderer();
protected:
	ZRef<ZUIRenderer_Divider> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory

class ZUIRendererFactory : public ZRefCounted
	{
public:
	ZUIRendererFactory();
	virtual ~ZUIRendererFactory();

	virtual string GetName() = 0;

	virtual ZRef<ZUIRenderer_CaptionPane> Get_Renderer_CaptionPane() = 0;

	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPush() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonRadio() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonCheckbox() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPopup() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonBevel() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonDisclosure() = 0;
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPlacard() = 0;

	virtual ZRef<ZUIRenderer_ScrollBar> Get_Renderer_ScrollBar() = 0;
	virtual ZRef<ZUIRenderer_Slider> Get_Renderer_Slider() = 0;
	virtual ZRef<ZUIRenderer_LittleArrows> Get_Renderer_LittleArrows() = 0;

	virtual ZRef<ZUIRenderer_ProgressBar> Get_Renderer_ProgressBar() = 0;
	virtual ZRef<ZUIRenderer_ChasingArrows> Get_Renderer_ChasingArrows() = 0;

	virtual ZRef<ZUIRenderer_TabPane> Get_Renderer_TabPane() = 0;

	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupPrimary() = 0;
	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupSecondary() = 0;

	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameEditText() = 0;
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameListBox() = 0;
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameDateControl() = 0;

	virtual ZRef<ZUIRenderer_Splitter> Get_Renderer_Splitter() = 0;

	virtual ZRef<ZUIRenderer_Divider> Get_Renderer_Divider() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Indirect

class ZUIRendererFactory_Caching : public ZUIRendererFactory
	{
public:
	ZUIRendererFactory_Caching();
	virtual ~ZUIRendererFactory_Caching();

// From ZUIRendererFactory
	virtual ZRef<ZUIRenderer_CaptionPane> Get_Renderer_CaptionPane();

	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPush();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonRadio();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonCheckbox();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPopup();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonBevel();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonDisclosure();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPlacard();

	virtual ZRef<ZUIRenderer_ScrollBar> Get_Renderer_ScrollBar();
	virtual ZRef<ZUIRenderer_Slider> Get_Renderer_Slider();
	virtual ZRef<ZUIRenderer_LittleArrows> Get_Renderer_LittleArrows();

	virtual ZRef<ZUIRenderer_ProgressBar> Get_Renderer_ProgressBar();
	virtual ZRef<ZUIRenderer_ChasingArrows> Get_Renderer_ChasingArrows();

	virtual ZRef<ZUIRenderer_TabPane> Get_Renderer_TabPane();

	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupPrimary();
	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupSecondary();

	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameEditText();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameListBox();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameDateControl();

	virtual ZRef<ZUIRenderer_Splitter> Get_Renderer_Splitter();

	virtual ZRef<ZUIRenderer_Divider> Get_Renderer_Divider();

// Our protocol
	virtual ZRef<ZUIRenderer_CaptionPane> Make_Renderer_CaptionPane() = 0;

	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPush() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonRadio() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonCheckbox() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPopup() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonBevel() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonDisclosure() = 0;
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPlacard() = 0;

	virtual ZRef<ZUIRenderer_ScrollBar> Make_Renderer_ScrollBar() = 0;
	virtual ZRef<ZUIRenderer_Slider> Make_Renderer_Slider() = 0;
	virtual ZRef<ZUIRenderer_LittleArrows> Make_Renderer_LittleArrows() = 0;

	virtual ZRef<ZUIRenderer_ProgressBar> Make_Renderer_ProgressBar() = 0;
	virtual ZRef<ZUIRenderer_ChasingArrows> Make_Renderer_ChasingArrows() = 0;

	virtual ZRef<ZUIRenderer_TabPane> Make_Renderer_TabPane() = 0;

	virtual ZRef<ZUIRenderer_Group> Make_Renderer_GroupPrimary() = 0;
	virtual ZRef<ZUIRenderer_Group> Make_Renderer_GroupSecondary() = 0;

	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameEditText() = 0;
	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameListBox() = 0;
	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameDateControl() = 0;

	virtual ZRef<ZUIRenderer_Splitter> Make_Renderer_Splitter() = 0;

	virtual ZRef<ZUIRenderer_Divider> Make_Renderer_Divider() = 0;

protected:
	ZRef<ZUIRenderer_CaptionPane> fRenderer_CaptionPane;

	ZRef<ZUIRenderer_Button> fRenderer_ButtonPush;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonRadio;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonCheckbox;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonPopup;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonBevel;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonDisclosure;
	ZRef<ZUIRenderer_Button> fRenderer_ButtonPlacard;

	ZRef<ZUIRenderer_ScrollBar> fRenderer_ScrollBar;
	ZRef<ZUIRenderer_Slider> fRenderer_Slider;
	ZRef<ZUIRenderer_LittleArrows> fRenderer_LittleArrows;

	ZRef<ZUIRenderer_ProgressBar> fRenderer_ProgressBar;
	ZRef<ZUIRenderer_ChasingArrows> fRenderer_ChasingArrows;

	ZRef<ZUIRenderer_TabPane> fRenderer_TabPane;

	ZRef<ZUIRenderer_Group> fRenderer_GroupPrimary;
	ZRef<ZUIRenderer_Group> fRenderer_GroupSecondary;

	ZRef<ZUIRenderer_FocusFrame> fRenderer_FocusFrameEditText;
	ZRef<ZUIRenderer_FocusFrame> fRenderer_FocusFrameListBox;
	ZRef<ZUIRenderer_FocusFrame> fRenderer_FocusFrameDateControl;

	ZRef<ZUIRenderer_Splitter> fRenderer_Splitter;

	ZRef<ZUIRenderer_Divider> fRenderer_Divider;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Indirect

class ZUIRendererFactory_Indirect : public ZUIRendererFactory
	{
public:
	ZUIRendererFactory_Indirect(const ZRef<ZUIRendererFactory>& realFactory);
	virtual ~ZUIRendererFactory_Indirect();

	ZRef<ZUIRendererFactory> GetRealFactory();
	void SetRealFactory(ZRef<ZUIRendererFactory> realFactory);

	virtual string GetName();

// From ZUIRendererFactory
	virtual ZRef<ZUIRenderer_CaptionPane> Get_Renderer_CaptionPane();

	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPush();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonRadio();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonCheckbox();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPopup();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonBevel();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonDisclosure();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPlacard();

	virtual ZRef<ZUIRenderer_ScrollBar> Get_Renderer_ScrollBar();
	virtual ZRef<ZUIRenderer_Slider> Get_Renderer_Slider();
	virtual ZRef<ZUIRenderer_LittleArrows> Get_Renderer_LittleArrows();

	virtual ZRef<ZUIRenderer_ProgressBar> Get_Renderer_ProgressBar();
	virtual ZRef<ZUIRenderer_ChasingArrows> Get_Renderer_ChasingArrows();

	virtual ZRef<ZUIRenderer_TabPane> Get_Renderer_TabPane();

	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupPrimary();
	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupSecondary();

	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameEditText();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameListBox();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameDateControl();

	virtual ZRef<ZUIRenderer_Splitter> Get_Renderer_Splitter();

	virtual ZRef<ZUIRenderer_Divider> Get_Renderer_Divider();

protected:
	ZRef<ZUIRendererFactory> fRealFactory;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFactory_Rendered

// A subclass of ZUIFactory that uses renderers to create its controls
class ZUIFactory_Rendered : public ZUIFactory
	{
public:
	ZUIFactory_Rendered(ZRef<ZUIRendererFactory> inRendererFactory);
	virtual ~ZUIFactory_Rendered();

	ZRef<ZUIRendererFactory> GetRendererFactory();
	void SetRendererFactory(ZRef<ZUIRendererFactory> inRendererFactory);

// From ZUIFactory
	virtual ZUICaptionPane* Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption);

	virtual ZUIButton* Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder);
	virtual ZUIButton* Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);

	virtual ZUIScrollBar* Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool inWithBorder);
	virtual ZUISlider* Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUILittleArrows* Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIProgressBar* Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIChasingArrows* Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUITabPane* Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIGroup* Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIGroup* Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIFocusFrame* Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUISplitter* Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIDivider* Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

protected:
	ZRef<ZUIRendererFactory> fRendererFactory;
	};

// =================================================================================================

#endif // __ZUIRendered__
