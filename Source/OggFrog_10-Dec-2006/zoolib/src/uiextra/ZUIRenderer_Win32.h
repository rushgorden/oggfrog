/*  @(#) $Id: ZUIRenderer_Win32.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUIRenderer_Win32__
#define __ZUIRenderer_Win32__ 1
#include "zconfig.h"

#include "ZDC.h"
#include "ZUIRendered.h"
#include "ZUIMetric.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Win32

class ZUIRenderer_Win32
	{
/* AG 98-05-21 ZUIRenderer_Win32 is a private base for all Win32 control renderers so they
get access to the constants and facilities defined herein */
public:
	ZUIRenderer_Win32();
	virtual ~ZUIRenderer_Win32();

	enum
		{
		borderSunkInner = 0, borderRaisedInner = 1,
		borderSunkOuter = 0, borderRaisedOuter = 2,
		borderHard = 0, borderSoft = 4,
		borderUpperLeftOnly = 8,
		edgeBump = borderRaisedOuter | borderSunkInner,
		edgeEtched = borderSunkOuter | borderRaisedInner,
		edgeRaised = borderRaisedOuter | borderRaisedInner,
		edgeRaisedSoft = borderRaisedOuter | borderRaisedInner | borderSoft,
		edgeSunk = borderSunkOuter | borderSunkInner,
		edgeSunkSoft = borderSunkOuter | borderSunkInner | borderSoft,
		edgeFlat = borderSunkOuter | borderRaisedInner | borderUpperLeftOnly
		};
	static void sDrawEdge(const ZDC& inDC, const ZRect& inRect, long inStyle,
		const ZRGBColor& color_Hilite1, const ZRGBColor& color_Hilite2, const ZRGBColor& color_Shadow1, const ZRGBColor& color_Shadow2);
	static void sDrawTriangle(const ZDC& inDC, ZCoord inHCoord, ZCoord inVCoord, ZCoord inBaseLength, bool inHorizontal, bool inDownArrow, const ZRGBColor& inColor);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_Win32

class ZUICaptionRenderer_Win32 : public ZUICaptionRenderer
	{
public:
	ZUICaptionRenderer_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1);
	virtual ~ZUICaptionRenderer_Win32();

	virtual void Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);
	virtual void Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Shadow1;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Win32

class ZUIRenderer_CaptionPane_Win32 : public ZUIRenderer_CaptionPane, public ZUICaptionRenderer_Win32
	{
public:
	ZUIRenderer_CaptionPane_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1);
	virtual ~ZUIRenderer_CaptionPane_Win32();

	virtual void Activate(ZUICaptionPane_Rendered* inCaptionPane);
	virtual void Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button_Win32_Targettable

class ZUIRenderer_Button_Win32_Targettable : public ZUIRenderer_Button
	{
public:
	ZUIRenderer_Button_Win32_Targettable();
	virtual ~ZUIRenderer_Button_Win32_Targettable();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual bool ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonBecameWindowTarget(ZUIButton_Rendered* inButton);
	virtual void ButtonResignedWindowTarget(ZUIButton_Rendered* inButton);
	virtual bool ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Win32

class ZUIRenderer_ButtonPush_Win32 : public ZUIRenderer_Button_Win32_Targettable, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ButtonPush_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face);
	virtual ~ZUIRenderer_ButtonPush_Win32();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
//	ZRef<ZUIColor> fColor_ScrollBarTrack;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Win32

class ZUIRenderer_ButtonRadio_Win32 : public ZUIRenderer_Button_Win32_Targettable, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ButtonRadio_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window);
	virtual ~ZUIRenderer_ButtonRadio_Win32();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderRadio(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Window;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Win32

class ZUIRenderer_ButtonCheckbox_Win32 : public ZUIRenderer_Button_Win32_Targettable, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ButtonCheckbox_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window);
	virtual ~ZUIRenderer_ButtonCheckbox_Win32();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderCheckbox(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Window;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Win32

class ZUIRenderer_ButtonPopup_Win32 : public ZUIRenderer_Button_Win32_Targettable, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ButtonPopup_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
													const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, const ZRef<ZUIColor>& color_Window, ZRef<ZUIMetric> metric_ArrowSize);
	virtual ~ZUIRenderer_ButtonPopup_Win32();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Arrow;
	ZRef<ZUIColor> fColor_Window;
	ZRef<ZUIMetric> fMetric_ArrowSize;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonDisclosure_Win32

class ZUIRenderer_ButtonDisclosure_Win32 : public ZUIRenderer_Button, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ButtonDisclosure_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window);
	virtual ~ZUIRenderer_ButtonDisclosure_Win32();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderDisclosure(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Window;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Win32

class ZUIRenderer_ScrollBar_Win32 : public ZUIRenderer_ScrollBar_Base, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ScrollBar_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
													const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, const ZRef<ZUIColor>& color_Track,
													ZRef<ZUIMetric> metric_ArrowSize, ZRef<ZUIMetric> metric_MinThumbLength);

// From ZUIRenderer_ScrollBar
	virtual void Activate(ZUIScrollBar_Rendered* inScrollBar);
	virtual ZCoord GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar);

// From ZUIRenderer_ScrollBar_Base
	virtual void RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inBorder, double inValue, double inThumbProportion, double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart);
	virtual ZUIScrollBar::HitPart FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion);
	virtual double GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta);

// Implementation
	static void sCalcAllRects(const ZRect& inBounds, bool inHorizontal, double inValue, double inThumbProportion, ZCoord inMinThumbLength, ZCoord inArrowSize,
						ZRect& outThumbRect, ZRect& outTrackRect,
						ZRect& outUpArrowRect, ZRect& outDownArrowRect,
						ZRect& outPageUpRect, ZRect& outPageDownRect);

	static void sRenderTrack(ZDC& inDC, const ZRect& inTrackRect, bool inHilited, const ZRGBColor& inColor_Track, const ZRGBColor& inColor_Hilite1);
	static void sRenderThumb(ZDC& inDC, const ZRect& inThumbRect,
						const ZRGBColor& inColor_Hilite1, const ZRGBColor& inColor_Hilite2,
						const ZRGBColor& inColor_Shadow1, const ZRGBColor& inColor_Shadow2,
						const ZRGBColor& inColor_Face);

	static void sRenderArrow(ZDC& inDC, const ZRect& inArrowRect, bool inHorizontal, bool inDownArrow, bool inPressed, bool inEnabled,
						const ZRGBColor& inColor_Hilite1, const ZRGBColor& inColor_Hilite2,
						const ZRGBColor& inColor_Shadow1, const ZRGBColor& inColor_Shadow2,
						const ZRGBColor& inColor_Face, const ZRGBColor& inColor_Arrow);

protected:
	ZRef<ZUIMetric> fMetric_ArrowSize;
	ZRef<ZUIMetric> fMetric_MinThumbLength;

	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Arrow;
	ZRef<ZUIColor> fColor_Track;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Win32

class ZUIRenderer_LittleArrows_Win32 : public ZUIRenderer_LittleArrows_Base, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_LittleArrows_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
													const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, ZRef<ZUIMetric> metric_ButtonWidth, ZRef<ZUIMetric> metric_ButtonHeight);
	virtual ~ZUIRenderer_LittleArrows_Win32();

// From ZUIRenderer_LittleArrows
	virtual void Activate(ZUILittleArrows_Rendered* inLittleArrows);
	virtual ZPoint GetPreferredSize();

// From ZUIRenderer_LittleArrows_Base
	virtual void RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart);
	virtual ZUILittleArrows::HitPart FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled);

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Arrow;
	ZRef<ZUIMetric> fMetric_ButtonWidth;
	ZRef<ZUIMetric> fMetric_ButtonHeight;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Win32

class ZUIRenderer_ProgressBar_Win32 : public ZUIRenderer_ProgressBar, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_ProgressBar_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Highlight);
	virtual ~ZUIRenderer_ProgressBar_Win32();

	virtual void Activate(ZUIProgressBar_Rendered* inProgessBar);
	virtual long GetFrameCount();
	virtual ZCoord GetHeight();
	virtual void DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge);
	virtual void DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame);
	virtual bigtime_t GetInterFrameDuration();

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Highlight;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Win32

class ZUIRenderer_TabPane_Win32 : public ZUIRenderer_TabPane, public ZUICaptionRenderer_Win32, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_TabPane_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face);
	virtual ~ZUIRenderer_TabPane_Win32();

	virtual void Activate(ZUITabPane_Rendered* inTabPane);
	virtual ZRect GetContentArea(ZUITabPane_Rendered* inTabPane);
	virtual bool GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk);

	virtual void RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds);
	virtual bool DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);

	void DrawTab(const ZDC& inDC, ZRef<ZUICaption> inCaption, ZPoint location, ZCoord midWidth, bool isTarget, bool isCurrent, bool enabled);

protected:
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Win32

class ZUIRenderer_Group_Win32 : public ZUIRenderer_Group, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_Group_Win32(bool isPrimary, const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2);
	virtual ~ZUIRenderer_Group_Win32();

	virtual void Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize);
	virtual void RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize);
	virtual ZCoord GetTitlePaneHLocation();
	virtual ZDCInk GetBackInk(const ZDC& inDC);
	virtual void GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight);

protected:
	bool fIsPrimary;

	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame_Win32

class ZUIRenderer_FocusFrame_Win32 : public ZUIRenderer_FocusFrame, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_FocusFrame_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window);
	virtual ~ZUIRenderer_FocusFrame_Win32();
	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame);
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds);
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_Window;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Win32

class ZUIRenderer_Divider_Win32 : public ZUIRenderer_Divider, private ZUIRenderer_Win32
	{
public:
	ZUIRenderer_Divider_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1);
	virtual ~ZUIRenderer_Divider_Win32();

	virtual void Activate(ZUIDivider_Rendered* inDivider);
	virtual ZCoord GetDividerThickness();
	virtual void DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Shadow1;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Win32

class ZUIRendererFactory_Win32 : public ZUIRendererFactory_Caching
	{
public:
	ZUIRendererFactory_Win32();
	virtual ~ZUIRendererFactory_Win32();

// From ZUIRendererFactory
	virtual string GetName();

// From ZUIRendererFactory_Caching
	virtual ZRef<ZUIRenderer_CaptionPane> Make_Renderer_CaptionPane();

	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPush();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonRadio();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonCheckbox();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPopup();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonBevel();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonDisclosure();
	virtual ZRef<ZUIRenderer_Button> Make_Renderer_ButtonPlacard();

	virtual ZRef<ZUIRenderer_ScrollBar> Make_Renderer_ScrollBar();
	virtual ZRef<ZUIRenderer_Slider> Make_Renderer_Slider();
	virtual ZRef<ZUIRenderer_LittleArrows> Make_Renderer_LittleArrows();

	virtual ZRef<ZUIRenderer_ProgressBar> Make_Renderer_ProgressBar();
	virtual ZRef<ZUIRenderer_ChasingArrows> Make_Renderer_ChasingArrows();

	virtual ZRef<ZUIRenderer_TabPane> Make_Renderer_TabPane();

	virtual ZRef<ZUIRenderer_Group> Make_Renderer_GroupPrimary();
	virtual ZRef<ZUIRenderer_Group> Make_Renderer_GroupSecondary();

	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameEditText();
	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameListBox();
	virtual ZRef<ZUIRenderer_FocusFrame> Make_Renderer_FocusFrameDateControl();

	virtual ZRef<ZUIRenderer_Splitter> Make_Renderer_Splitter();

	virtual ZRef<ZUIRenderer_Divider> Make_Renderer_Divider();

protected:
	ZRef<ZUIColor> fColor_Hilite1;
	ZRef<ZUIColor> fColor_Hilite2;
	ZRef<ZUIColor> fColor_Shadow1;
	ZRef<ZUIColor> fColor_Shadow2;
	ZRef<ZUIColor> fColor_Face;
	ZRef<ZUIColor> fColor_ButtonText;
	ZRef<ZUIColor> fColor_ScrollBarTrack;
	ZRef<ZUIColor> fColor_Window;
	ZRef<ZUIColor> fColor_Highlight;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Win32

class ZUIAttributeFactory_Win32 : public ZUIAttributeFactory_Caching
	{
public:
	ZUIAttributeFactory_Win32();
	virtual ~ZUIAttributeFactory_Win32();

	virtual string GetName();

	virtual ZRef<ZUIColor> MakeColor(const string& inColorName);
	virtual ZRef<ZUIInk> MakeInk(const string& inInkName);
	virtual ZRef<ZUIFont> MakeFont(const string& inFontName);
	virtual ZRef<ZUIMetric> MakeMetric(const string& inMetricName);

	virtual ZRef<ZUIIconRenderer> GetIconRenderer();

protected:
	ZRef<ZUIColor> fColor_Highlight;
	};

// =================================================================================================

#endif // __ZUIRenderer_Win32__
