/*  @(#) $Id: ZUI.h,v 1.6 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUI__
#define __ZUI__ 1
#include "zconfig.h"

#include "ZPane.h"
#include "ZUIColor.h"
#include "ZUIFont.h"
#include "ZUIInk.h"
#include "ZUIMetric.h"
#include "ZUIDisplayState.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer

/* AG 98-09-24. Why do we need ZUICaptionRenderer? Although we can have captions that look different by instantiating different
subclasses of ZUICaption, UI elements like buttons and static text also have policies on how text and images are rendered that vary from
platform to platform, or appearance to appearance. By defining the ZUICaptionRenderer API, a caption can call back to an object to
draw text and images, and have that text/image drawn correctly according to the rules of the appearance. Of course you're free
to subclass ZUICaption and ignore the passed in ZUICaptionRenderer, it's there to provide a service, not to be a straitjacket. It would
have been nice to be able to *describe* the rules in some kind of data structure, but Win32's "embossed" look for disabled items
is a key case where that would be insufficient, and one can easily imagine other circumstances where we would have been unable
to anticipate what will be needed and to provide for its encoding in a data structure. */

/* AG 99-02-09. Revised thinking. If the caption renderer is responsible for calculating the size of a caption, it becomes very
hard for captions to cache such information, and the UI starts getting very sluggish. The only current case where we have to take account
of captions whose size varies based on who renders them is in the Win32 look. The Win32 code can deal with this by *allowing*
one extra pixel horizontally and vertically when working with captions, without the caption itself actually being deemed to being
any different in size. Removing this mechanism has allowed caching to occur, resulting in noticeable speed improvements.*/

class ZUICaptionRenderer
	{
public:
	virtual void Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);
	virtual void Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);

	void DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);
	void DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const string& inText, const ZDCFont& inFont, ZCoord inWrapWidth);

	virtual void DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption

class ZUICaption : public ZRefCounted
	{
public:
	ZUICaption();
	virtual ~ZUICaption();

	virtual bool IsValid();
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer) = 0;
	virtual bool DrawsEntireRgn();
	virtual ZDCRgn GetRgn() = 0;

	ZPoint GetSize();
	ZRect GetBounds();

	void Draw(const ZDC& inDC, ZPoint inLocation, ZUICaptionRenderer* inCaptionRenderer);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Indirect

class ZUICaption_Indirect : public ZUICaption
	{
public:
	ZUICaption_Indirect(ZRef<ZUICaption> inRealCaption);
	virtual ~ZUICaption_Indirect();

	virtual bool IsValid();
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual bool DrawsEntireRgn();
	virtual ZDCRgn GetRgn();

	void SetRealCaption(ZRef<ZUICaption> inRealCaption);
	ZRef<ZUICaption> GetRealCaption();

protected:
	ZRef<ZUICaption> fRealCaption;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Dynamic

class ZUICaption_Dynamic : public ZUICaption
	{
public:
	class Provider;

	ZUICaption_Dynamic(Provider* inProvider);
	virtual ~ZUICaption_Dynamic();

	virtual bool IsValid();
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual bool DrawsEntireRgn();
	virtual ZDCRgn GetRgn();

protected:
	Provider* fProvider;
	};

class ZUICaption_Dynamic::Provider
	{
public:
	virtual ZRef<ZUICaption> ProvideUICaption(ZUICaption_Dynamic* inCaption) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Text

class ZUICaption_Text : public ZUICaption
	{
public:
	ZUICaption_Text();
	ZUICaption_Text(const string& inText, ZRef<ZUIFont> inUIFont, ZCoord inWrapWidth);
	virtual ~ZUICaption_Text();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

	void SetText(const string& inText);
	const string& GetText();

protected:
	string fText;
	ZRef<ZUIFont> fUIFont;
	ZCoord fWrapWidth;
	ZPoint fCachedSize;
	long fChangeCount;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_SimpleText

class ZUICaption_SimpleText : public ZUICaption
	{
public:
	ZUICaption_SimpleText();
	ZUICaption_SimpleText(const string& inText, ZRef<ZUIFont> inUIFont, ZCoord inWrapWidth);
	virtual ~ZUICaption_SimpleText();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

	void SetText(const string& inText);
	const string& GetText();

	void SetTextColor(const ZRGBColor& inTextColor);
	const ZRGBColor& GetTextColor() const
		{ return fTextColor; }

protected:
	string fText;
	ZRef<ZUIFont> fUIFont;
	ZRGBColor fTextColor;
	ZCoord fWrapWidth;
	ZPoint fCachedSize;
	long fChangeCount;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Pix

class ZUICaption_Pix : public ZUICaption
	{
public:
	ZUICaption_Pix();
	ZUICaption_Pix(const ZDCPixmap& inPixmap);
	ZUICaption_Pix(const ZDCPixmapCombo& inPixmapCombo);
	ZUICaption_Pix(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap);
	virtual ~ZUICaption_Pix();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);

	virtual ZDCRgn GetRgn();
	virtual bool DrawsEntireRgn();

protected:
	ZDCPixmapCombo fPixmapCombo;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Composite_H

class ZUICaption_Composite_H : public ZUICaption
	{
public:
	ZUICaption_Composite_H();
	virtual ~ZUICaption_Composite_H();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

	void Add(ZRef<ZUICaption> theCaption);

protected:
	vector<ZRef<ZUICaption> > fCaptions;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaption_Composite_V

class ZUICaption_Composite_V : public ZUICaption
	{
public:
	ZUICaption_Composite_V();
	virtual ~ZUICaption_Composite_V();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

	void Add(ZRef<ZUICaption> theCaption);

protected:
	vector<ZRef<ZUICaption> > fCaptions;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionPane

class ZUICaptionPane : public ZSubPane
	{
public:
	ZUICaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, ZRef<ZUICaption> inCaption);
	virtual ~ZUICaptionPane();

	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual ZPoint GetSize();

	virtual ZPoint GetPreferredSize();

	void SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh);
	ZRef<ZUICaption> GetCaption();

// Deprecated
	void SetCaptionWithRedraw(ZRef<ZUICaption> inCaption);

protected:
	ZRef<ZUICaption> fCaption;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIIconRenderer

class ZUIIconRenderer : public ZRefCounted
	{
public:
	ZUIIconRenderer();
	virtual ~ZUIIconRenderer();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled);

	static ZDCPixmap sMakeHollowPixmap(const ZDCPixmap& inMask, const ZRGBColor& inForeColor, const ZRGBColor& inBackColor);
	static ZRect sCalcMaskBoundRect(const ZDCPixmap& inMask); // -ec 01.02.22
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIIconRenderer_Indirect

class ZUIIconRenderer_Indirect : public ZUIIconRenderer
	{
public:
	ZUIIconRenderer_Indirect(const ZRef<ZUIIconRenderer>& inRealRenderer);
	virtual ~ZUIIconRenderer_Indirect();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled);

	void SetRealRenderer(const ZRef<ZUIIconRenderer>& inRealRenderer);
	ZRef<ZUIIconRenderer> GetRealRenderer();

protected:
	ZRef<ZUIIconRenderer> fRealRenderer;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIButton

class ZUIButton : public ZSubPane
	{
public:
	class Responder;

	ZUIButton(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, Responder* inResponder);
	virtual ~ZUIButton();

// From ZSubPane
	virtual ZPoint GetSize();

// Our protocol
	virtual void Flash();
	virtual ZPoint GetPopupLocation();
	virtual ZPoint CalcPreferredSize();

	virtual void GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZPoint GetLocationInset();
	virtual ZPoint GetSizeInset();

	virtual void SetCaption(ZRef<ZUICaption> inCaption, bool inRefresh);

	Responder* GetResponder() const
		{ return fResponder; }
	void SetResponder(Responder* inResponder);

	enum Event
		{
		ButtonDoubleClick,
		ButtonDown,
		ButtonStillDown,
		ButtonStillUp,
		ButtonUp,
		ButtonReleasedWhileUp,
		ButtonAboutToBeReleasedWhileDown,
		ButtonReleasedWhileDown,
		ButtonSqueezed
		};

protected:
	Responder* fResponder;
	};

// ----------

class ZUIButton::Responder
	{
public:
	virtual bool HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent);
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIButton_Tracked

// Base class for buttons that are tracked by ZooLib
class ZUIButton_Tracked : public ZUIButton
	{
public:
	ZUIButton_Tracked(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator, Responder* inResponder);
	virtual ~ZUIButton_Tracked();

	virtual void DoDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn);
	virtual bool DoClick(ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	virtual void Flash();

protected:
	virtual void PrivateDraw(const ZDC& inDC, const ZDCRgn& inBoundsRgn, bool pressed, EZTriState hilite) = 0;

private:
	class Tracker;
	friend class Tracker;

	bool GetPressed();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRange

class ZUIRange
	{
protected:
	ZUIRange();
	virtual ~ZUIRange();

public:
	enum Event
		{
		EventDown,
		EventStillDown,
		EventStillUp,
		EventUp,
		EventReleasedWhileUp,
		EventAboutToBeReleasedWhileDown,
		EventReleasedWhileDown
		};

	enum Part
		{
		PartNone,
		PartStepUp,
		PartPageUp,
		PartStepDown,
		PartPageDown
		};

	class Responder;

	virtual double GetValue() = 0;

	void AddResponder(Responder* inResponder);
	void RemoveResponder(Responder* inResponder);

	void Internal_NotifyResponders(Event inEvent, Part inRangePart);
	void Internal_NotifyResponders(double inNewvalue);

protected:
	vector<Responder*> fResponders;
	};

// ----------

class ZUIRange::Responder
	{
public:
	virtual void RangeGone(ZUIRange* inRange);
	virtual bool HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart);
	virtual bool HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollBar

class ZUIScrollBar : public ZSubPane, public ZUIRange
	{
public:
	ZUIScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIScrollBar();

// From ZUIRange
	virtual double GetValue();

// Our protocol
	virtual double GetThumbProportion();

	virtual ZCoord GetPreferredThickness() = 0;

	enum HitPart
		{ hitPartNone, hitPartThumb,
			hitPartUpArrowOuter, hitPartUpArrowInner, hitPartDownArrowOuter, hitPartDownArrowInner,
			hitPartPageUp, hitPartPageDown };
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUISlider

class ZUISlider : public ZSubPane, public ZUIRange
	{
public:
	ZUISlider(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUISlider();

// From ZUIRange
	virtual double GetValue();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUILittleArrows

class ZUILittleArrows : public ZSubPane, public ZUIRange
	{
public:
	ZUILittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUILittleArrows();

// From ZUIRange
	virtual double GetValue();

// Our protocol
	virtual ZPoint GetPreferredSize() = 0;

	enum HitPart
		{ hitPartNone, hitPartUp, hitPartDown };
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIProgressBar

class ZUIProgressBar : public ZSubPane
	{
public:
	ZUIProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIProgressBar();

// From ZSubPane
	virtual void DoIdle();

// Our protocol
	virtual bigtime_t NextFrame() = 0;

protected:
	bigtime_t fNextTick;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIChasingArrows

class ZUIChasingArrows : public ZSubPane
	{
public:
	ZUIChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIChasingArrows();

// From ZSubPane
	virtual void DoIdle();

// Our protocol
	virtual bigtime_t NextFrame() = 0;

protected:
	bigtime_t fNextTick;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAlternatePane

class ZUIAlternatePane : public ZSuperPane
	{
public:
	ZUIAlternatePane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIAlternatePane();

// Our protocol
	virtual void DeletePane(ZSuperPane* inPane) = 0;
	virtual void SetCurrentPane(ZSuperPane* inPane) = 0;
	virtual ZSuperPane* GetCurrentPane() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUITabPane

class ZUITabPane : public ZUIAlternatePane
	{
public:
	ZUITabPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUITabPane();

// Our protocol
	virtual ZSuperPane* CreateTab(ZRef<ZUICaption> inCaption) = 0;

	virtual void SetCurrentTab(ZSuperPane* inPane);
	virtual ZSuperPane* GetCurrentTab();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIGroup

class ZUIGroup : public ZSuperPane
	{
public:
	ZUIGroup(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIGroup();

// Our protocol
	virtual void GetInsets(ZPoint& outTopLeft, ZPoint& outTopRight) = 0;
	virtual ZSuperPane* GetTitlePane() = 0;
	virtual ZSuperPane* GetInternalPane() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUISelection

class ZUISelection
	{
protected:
	ZUISelection();
	virtual ~ZUISelection();

public:
// Our protocol
	class Responder;

	enum Event
		{
		SelectionFeedbackChanged,
		SelectionChanged,
		ContentChanged
		};

	void AddResponder(Responder* inResponder);
	void RemoveResponder(Responder* inResponder);

	void Internal_NotifyResponders(Event inEvent);

protected:
	vector<Responder*> fResponders;
	};

// ----------

class ZUISelection::Responder
	{
public:
	virtual void SelectionGone(ZUISelection* inSelection);
	virtual void HandleUISelectionEvent(ZUISelection* inSelection, ZUISelection::Event inEvent) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIScrollable

class ZUIScrollable
	{
protected:
	ZUIScrollable();
	virtual ~ZUIScrollable();

public:
	class Responder;

	virtual void ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications) = 0;
	virtual void ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications) = 0;
	virtual void ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications) = 0;
	virtual void ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications) = 0;

	virtual ZPoint_T<double> GetScrollValues() = 0;
	virtual ZPoint_T<double> GetScrollThumbProportions() = 0;
	void AddResponder(Responder* inResponder);
	void RemoveResponder(Responder* inResponder);

	void Internal_NotifyResponders();

	bool DoCursorKey(const ZEvent_Key& inEvent);

protected:
	vector<Responder*> fResponders;
	};

// ----------

class ZUIScrollable::Responder
	{
public:
	virtual void ScrollableGone(ZUIScrollable* inScrollable);
	virtual void ScrollableChanged(ZUIScrollable* inScrollable);
	};

// ==================================================
#pragma mark -
#pragma mark * ZUITextContainer

// (Very) abstract base class for text editing things -- like panes or text objects in the painter

class ZUITextContainer
	{
public:
	ZUITextContainer();
	virtual ~ZUITextContainer();

	virtual size_t GetTextLength() = 0;

	virtual void GetText_Impl(char* outDest, size_t inOffset, size_t inLength, size_t& outLength) = 0;
	void GetText(char* outDest, size_t inOffset, size_t inLength, size_t& outLength);
	string GetText(size_t inOffset, size_t inLength);
	string GetText();

	char GetChar(size_t inOffset);

	virtual void SetText_Impl(const char* inSource, size_t inLength) = 0;
	void SetText(const char* inSource, size_t inLength);
	void SetText(const string& inText);

	virtual void InsertText_Impl(size_t inInsertOffset, const char* inSource, size_t inLength) = 0;
	void InsertText(size_t inInsertOffset, const char* inSource, size_t inLength);
	void InsertText(size_t inInsertOffset, const string& inText);

	virtual void DeleteText(size_t inOffset, size_t inLength) = 0;

	virtual void ReplaceText_Impl(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength) = 0;
	void ReplaceText(size_t inReplaceOffset, size_t inReplaceLength, const char* inSource, size_t inLength);
	void ReplaceText(size_t inReplaceOffset, size_t inReplaceLength, const string& inText);

	virtual void GetSelection(size_t& outOffset, size_t& outLength) = 0;
	virtual void SetSelection(size_t inOffset, size_t inLength) = 0;

	virtual void ReplaceSelection_Impl(const char* inSource, size_t inLength);
	void ReplaceSelection(const char* inSource, size_t inLength);
	void ReplaceSelection(const string& inText);
	};

// ==================================================
#pragma mark -
#pragma mark * ZUITextPane

// Abstract base class for Text Panes -- forces any concrete subclass to support
// the pane, scrollable, text container and selection interfaces.

class ZUITextPane : public ZSubPane, public ZUIScrollable, public ZUITextContainer, public ZUISelection
	{
public:
	ZUITextPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUITextPane();

	virtual void SetFont(const ZDCFont& inFont) = 0;
	virtual long GetUserChangeCount() = 0;
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIFocusFrame

class ZUIFocusFrame : public ZSuperPane
	{
public:
	ZUIFocusFrame(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIFocusFrame();

// From ZEventHr
	virtual void HandlerBecameWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);
	virtual void HandlerResignedWindowTarget(ZEventHr* inOldTarget, ZEventHr* inNewTarget);

// Our protocol
	virtual void GetInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset) = 0;

	virtual ZDCRgn GetBorderRgn();

	bool GetBorderShown();
	};

// ==================================================
#pragma mark -
#pragma mark * ZUISplitter

class ZUISplitter : public ZSuperPane
	{
public:
	class Responder;

	ZUISplitter(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUISplitter();

	virtual ZCoord CalcPane1Size(double inProportion) = 0;
	virtual ZCoord GetSplitterThickness() = 0;
	virtual void GetPanes(ZSuperPane*& outFirstPane, ZSuperPane*& outSecondPane) = 0;

	ZSuperPane* GetPane1();
	ZSuperPane* GetPane2();

	bool GetOrientation();

	void AddResponder(Responder* inResponder);
	void RemoveResponder(Responder* inResponder);
	void Internal_NotifyResponders(ZCoord inNewPane1Size, ZCoord inNewPane2Size);

protected:
	vector<Responder*> fResponders;
	};

// ----------

class ZUISplitter::Responder
	{
public:
	virtual void SplitterChanged(ZUISplitter* inSplitter, ZCoord inNewPane1Size, ZCoord inNewPane2Size) = 0;
	virtual void SplitterGone(ZUISplitter* inSplitter);
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIDivider

class ZUIDivider : public ZSubPane
	{
public:
	ZUIDivider(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIDivider();

	bool GetOrientation();

	virtual ZCoord GetPreferredThickness() = 0;
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIScrollPane

class ZUIScrollPane : public ZSuperPane, public ZUIScrollable
	{
public:
	ZUIScrollPane(ZSuperPane* inSuperPane, ZPaneLocator* inPaneLocator);
	virtual ~ZUIScrollPane();

// From ZSuperPane/ZSubPane
	virtual bool DoKeyDown(const ZEvent_Key& inEvent);

	virtual ZPoint GetTranslation();

	virtual void BringPointIntoViewPinned(ZPoint inPosition);
	virtual void BringPointIntoView(ZPoint inPosition);

	virtual bool WantsToBecomeTarget();
// From ZUIScrollable
	virtual void ScrollBy(ZPoint_T<double> inDelta, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollTo(ZPoint_T<double> inNewValues, bool inDrawIt, bool inDoNotifications);

	virtual void ScrollStep(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);
	virtual void ScrollPage(bool inIncrease, bool inHorizontal, bool inDrawIt, bool inDoNotifications);

	virtual ZPoint_T<double> GetScrollValues();
	virtual ZPoint_T<double> GetScrollThumbProportions();

// Our protocol
	void ScrollBy(ZPoint inDelta, bool inDrawIt, bool inDoNotifications);
	void ScrollTo(ZPoint inNewTranslation, bool inDrawIt, bool inDoNotifications);
	ZPoint GetSubPaneExtent();
protected:
	void Internal_ScrollTo(ZPoint inNewTranslation, bool inDrawIt, bool inDoNotifications);

	ZPoint fTranslation;
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIScrollableScrollableConnector

class ZUIScrollableScrollableConnector : public ZUIScrollable::Responder
	{
public:
	ZUIScrollableScrollableConnector(ZUIScrollable* inScrollable1, ZUIScrollable* inScrollable2, bool inSlaveHorizontal);
	virtual ~ZUIScrollableScrollableConnector();

// From ZUIScrollable::Responder
	virtual void ScrollableChanged(ZUIScrollable* inScrollable);
	virtual void ScrollableGone(ZUIScrollable* inScrollable);

protected:
	ZUIScrollable* fScrollable1;
	ZUIScrollable* fScrollable2;
	bool fSlaveHorizontal;
	};

// ==================================================
#pragma mark -
#pragma mark * ZUIScrollableScrollBarConnector

class ZUIScrollableScrollBarConnector : public ZUIScrollable::Responder,
							public ZUIRange::Responder,
							public ZPaneLocatorSimple
	{
public:
	ZUIScrollableScrollBarConnector(ZUIScrollBar* inScrollBar, ZUIScrollable* inScrollable, ZSubPane* inAdjacentPane, bool inHorizontal);
	ZUIScrollableScrollBarConnector(ZUIScrollBar* inScrollBar, ZUIScrollable* inScrollable, ZSubPane* inAdjacentPane, bool inHorizontal, ZCoord topOffset, ZCoord inBottomOffset);
	virtual ~ZUIScrollableScrollBarConnector();

// From ZUIScrollable::Responder
	virtual void ScrollableChanged(ZUIScrollable* inScrollable);
	virtual void ScrollableGone(ZUIScrollable* inScrollable);

// From ZUIRangeResponder
	virtual void RangeGone(ZUIRange* inRange);
	virtual bool HandleUIRangeEvent(ZUIRange* inRange, ZUIRange::Event inEvent, ZUIRange::Part inRangePart);
	virtual bool HandleUIRangeSliderEvent(ZUIRange* inRange, double inNewValue);

// From ZPaneLocator
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);
	virtual bool GetPaneSize(ZSubPane* inPane, ZPoint& outSize);
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);

protected:
	bool fHorizontal;
	ZCoord fTopOffset;
	ZCoord fBottomOffset;
	ZUIScrollBar* fScrollBar;
	ZUIScrollable* fScrollable;
	ZSubPane* fAdjacentPane;
	};

// =================================================================================================

#endif // __ZUI__
