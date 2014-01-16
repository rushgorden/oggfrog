/*  @(#) $Id: ZUIRenderer_Platinum.h,v 1.6 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUIRenderer_Platinum__
#define __ZUIRenderer_Platinum__ 1
#include "zconfig.h"

#if !defined(ZCONFIG_UIRenderer_PlatinumAvailable)
#	define ZCONFIG_UIRenderer_PlatinumAvailable 1
#endif

#if ZCONFIG_UIRenderer_PlatinumAvailable

#include "ZDC.h"
#include "ZUIRendered.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Platinum

class ZUIRenderer_Platinum
	{
/* AG 98-05-21 ZUIRenderer_Platinum is a private base for all Platinum control renderers so that
they get access to the constants and facilities defined herein (fewer ::'s needed).*/
protected:
	ZUIRenderer_Platinum();
	~ZUIRenderer_Platinum();

public:
// Drawing utility methods
	static void sDrawSide_TopLeft(const ZDC& inDC, const ZRect& inRect, ZCoord leftInset, ZCoord topInset, ZCoord rightInset, ZCoord bottomInset);
	static void sDrawSide_BottomRight(const ZDC& inDC, const ZRect& inRect, ZCoord leftInset, ZCoord topInset, ZCoord rightInset, ZCoord bottomInset);
	static void sDrawBevel_TopLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawBevel_BottomRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawCorner_TopLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawCorner_TopRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawCorner_BottomLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawCorner_BottomRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount);
	static void sDrawTriangle(const ZDC& inDC, const ZRGBColor& inColor, ZCoord inHCoord, ZCoord inVCoord, ZCoord inBaseLength, bool inHorizontal, bool inDownArrow, bool inHollow, bool inGrayPattern);


// The platinum color table stuff
	enum
		{
		kColorRamp_Black = 0,
		kColorRamp_Gray_0 = 0,
		kColorRamp_Gray_1 = 1,
		kColorRamp_Gray_2 = 2,
		kColorRamp_Gray_3 = 3,
		kColorRamp_Gray_4 = 4,
		kColorRamp_Gray_5 = 5,
		kColorRamp_Gray_6 = 6,
		kColorRamp_Gray_7 = 7,
		kColorRamp_Gray_8 = 8,
		kColorRamp_Gray_9 = 9,
		kColorRamp_Gray_A = 10,
		kColorRamp_Gray_B = 11,
		kColorRamp_Gray_C = 12,
		kColorRamp_Gray_D = 13,
		kColorRamp_Gray_E = 14,
		kColorRamp_Gray_F = 15,
		kColorRamp_White = 15,
		kColorRamp_Purple1 = 16,
		kColorRamp_Purple2 = 17,
		kColorRamp_Purple3 = 18,
		kColorRamp_Purple4 = 19
		};

	static ZRGBColor sLookupColor(int inColorIndex) { return sColorTable[inColorIndex]; }

// Table of grayscale colors
	static ZRGBColor sColorTable[];
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_Platinum

class ZUICaptionRenderer_Platinum : public ZUICaptionRenderer
	{
public:
	ZUICaptionRenderer_Platinum();
	virtual ~ZUICaptionRenderer_Platinum();

	virtual void Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);

	virtual void Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Platinum

class ZUIRenderer_CaptionPane_Platinum : public ZUIRenderer_CaptionPane, public ZUICaptionRenderer_Platinum
	{
public:
	ZUIRenderer_CaptionPane_Platinum();
	virtual ~ZUIRenderer_CaptionPane_Platinum();

	virtual void Activate(ZUICaptionPane_Rendered* inCaptionPane);
	virtual void Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Platinum

class ZUIRenderer_ButtonPush_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonPush_Platinum();
	virtual ~ZUIRenderer_ButtonPush_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	virtual void GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	static void sRenderPush(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	static void sRenderFocusRing(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& theState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Platinum

class ZUIRenderer_ButtonRadio_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonRadio_Platinum();
	virtual ~ZUIRenderer_ButtonRadio_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderRadio(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Platinum

class ZUIRenderer_ButtonCheckbox_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonCheckbox_Platinum();
	virtual ~ZUIRenderer_ButtonCheckbox_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderCheckbox(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Platinum

class ZUIRenderer_ButtonPopup_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonPopup_Platinum();
	virtual ~ZUIRenderer_ButtonPopup_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderPopup(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState, bool isArrowOnly, bool isPullDown);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonBevel_Platinum

class ZUIRenderer_ButtonBevel_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonBevel_Platinum();
	virtual ~ZUIRenderer_ButtonBevel_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderBevel(const ZDC& inDC, short inBevelWidth, const ZRect& inButtonRect, const ZUIDisplayState& inState);
	static void sRenderBevelPopup(const ZDC& inDC, short inBevelWidth, const ZRect& inButtonRect,
						bool inPopupRightFacing, bool inPopupCentred, ZCoord inPopupBaseLength, const ZUIDisplayState& theState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonDisclosure_Platinum

class ZUIRenderer_ButtonDisclosure_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonDisclosure_Platinum();
	virtual ~ZUIRenderer_ButtonDisclosure_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);

	static void sRenderDisclosure(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPlacard_Platinum

class ZUIRenderer_ButtonPlacard_Platinum : public ZUIRenderer_Button, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ButtonPlacard_Platinum();
	virtual ~ZUIRenderer_ButtonPlacard_Platinum();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);

	static void sRender_Placard(const ZDC& inDC, const ZRect& inRect, bool inEnabled);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Platinum

class ZUIRenderer_ScrollBar_Platinum : public ZUIRenderer_ScrollBar_Base, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ScrollBar_Platinum();
	virtual ~ZUIRenderer_ScrollBar_Platinum();

// From ZUIRenderer_ScrollBar
	virtual void Activate(ZUIScrollBar_Rendered* inScrollBar);
	virtual ZCoord GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar);

// From ZUIRenderer_ScrollBar_Base
	virtual void RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inWithBorder, double inValue, double inThumbProportion,  double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart);
	virtual ZUIScrollBar::HitPart FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion);
	virtual double GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta);

// Implementation
	static void sCalcAllRects(const ZRect& inBounds, bool inHorizontal, double inThumbValue, double inThumbProportion,
										ZRect& outThumbRect, ZRect& outTrackRect,
										ZRect& outUpArrowRect, ZRect& outDownArrowRect,
										ZRect& outPageUpRect, ZRect& outPageDownRect);
	static void sRenderTrackSlideable(ZDC& inDC, const ZRect& inTrackBounds, bool inHorizontal);
	static void sRenderTrackNotSlideable(ZDC& inDC, const ZRect& inTrackBounds, bool inHorizontal);
	static void sRenderScrollBarDisabled(ZDC& inDC, const ZRect& inBounds, bool inHorizontal);
	static void sRenderGhostThumb(ZDC& inDC, const ZRect& inThumbBounds, bool inHorizontal);
	static void sRenderThumb(ZDC& inDC, const ZRect& inThumbBounds, bool inHorizontal, bool inPressed);
	static void sRenderArrow(ZDC& inDC, const ZRect& inArrowBounds, bool inHorizontal, bool inDownArrow, bool inPressed, bool inSlideable);
	static void sRenderArrowDisabled(ZDC& inDC, const ZRect& inArrowBounds, bool inHorizontal, bool inDownArrow);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Platinum

class ZUIRenderer_Slider_Platinum : public ZUIRenderer_Slider_Base
	{
public:
	ZUIRenderer_Slider_Platinum();
	virtual ~ZUIRenderer_Slider_Platinum();

// From ZUIRenderer_Slider_Base
	virtual void Activate(ZUISlider_Rendered* inSlider);
	virtual void RenderSlider(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, double inValue, double inGhostThumbValue, bool inThumbHilited);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Platinum

class ZUIRenderer_LittleArrows_Platinum : public ZUIRenderer_LittleArrows_Base, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_LittleArrows_Platinum();
	virtual ~ZUIRenderer_LittleArrows_Platinum();

// From ZUIRenderer_LittleArrows
	virtual void Activate(ZUILittleArrows_Rendered* inLittleArrows);
	virtual ZPoint GetPreferredSize();

// From ZUIRenderer_LittleArrows_Base
	virtual void RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart);
	virtual ZUILittleArrows::HitPart FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Platinum

class ZUIRenderer_ProgressBar_Platinum : public ZUIRenderer_ProgressBar, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ProgressBar_Platinum();
	virtual ~ZUIRenderer_ProgressBar_Platinum();

	virtual void Activate(ZUIProgressBar_Rendered* inProgressBar);
	virtual long GetFrameCount();
	virtual ZCoord GetHeight();
	virtual void DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool enabled, bool inActive, double leftEdge, double rightEdge);
	virtual void DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool enabled, bool inActive, long frame);
	virtual bigtime_t GetInterFrameDuration();

protected:
	static ZDCPixmap sPixmapNormal;
	static ZDCPixmap sPixmapDisabled;
	static ZDCPixmap sPixmapMono;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Platinum

class ZUIRenderer_ChasingArrows_Platinum : public ZUIRenderer_ChasingArrows, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_ChasingArrows_Platinum();
	virtual ~ZUIRenderer_ChasingArrows_Platinum();

	virtual void Activate(ZUIChasingArrows_Rendered* inChasingArrows);
	virtual long GetFrameCount();
	virtual ZPoint GetSize();
	virtual void DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool enabled, bool inActive, long frame);
	virtual bigtime_t GetInterFrameDuration();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Platinum

class ZUIRenderer_TabPane_Platinum : public ZUIRenderer_TabPane_Base, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_TabPane_Platinum();
	virtual ~ZUIRenderer_TabPane_Platinum();

// From ZUIRenderer_TabPane
	virtual void Activate(ZUITabPane_Rendered* inTabPane);
	virtual ZRect GetContentArea(ZUITabPane_Rendered* inTabPane);
	virtual bool GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk);

	virtual void RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds);

// From ZUIRenderer_TabPane_Base
	virtual ZRect CalcTabRect(ZUITabPane_Rendered* inTabPane, ZRef<ZUICaption> inCaption);
	virtual void DrawTab(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZRef<ZUICaption> inCaption, const ZRect& inTabRect, const ZUIDisplayState& inState);

// Our protocol
	static void sRenderTab(const ZDC& inDC, ZPoint inLocation, ZCoord inMidWidth, bool inLarge, bool inPressed, EZTriState inHilite, bool inEnabled);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Platinum

class ZUIRenderer_Group_Platinum : public ZUIRenderer_Group, public ZUICaptionRenderer_Platinum, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_Group_Platinum(bool isPrimary);
	virtual ~ZUIRenderer_Group_Platinum();

	virtual void Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize);
	virtual void RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize);
	virtual ZCoord GetTitlePaneHLocation();
	virtual ZDCInk GetBackInk(const ZDC& inDC);
	virtual void GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight);
protected:
	bool fIsPrimary;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame_Platinum

class ZUIRenderer_FocusFrame_Platinum : public ZUIRenderer_FocusFrame, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_FocusFrame_Platinum();
	virtual ~ZUIRenderer_FocusFrame_Platinum();

	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame);
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds);
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Platinum

class ZUIRenderer_Divider_Platinum : public ZUIRenderer_Divider, private ZUIRenderer_Platinum
	{
public:
	ZUIRenderer_Divider_Platinum();
	virtual ~ZUIRenderer_Divider_Platinum();

	virtual void Activate(ZUIDivider_Rendered* inDivider);
	virtual ZCoord GetDividerThickness();
	virtual void DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Platinum

class ZUIRendererFactory_Platinum : public ZUIRendererFactory_Caching
	{
public:
	ZUIRendererFactory_Platinum();
	virtual ~ZUIRendererFactory_Platinum();

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
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Platinum

class ZUIAttributeFactory_Platinum : public ZUIAttributeFactory_Caching
	{
public:
	ZUIAttributeFactory_Platinum();
	virtual ~ZUIAttributeFactory_Platinum();

	virtual string GetName();

	virtual ZRef<ZUIColor> MakeColor(const string& inColorName);
	virtual ZRef<ZUIInk> MakeInk(const string& inInkName);
	virtual ZRef<ZUIFont> MakeFont(const string& inFontName);
	virtual ZRef<ZUIMetric> MakeMetric(const string& inMetricName);
	};

// =================================================================================================

#endif // ZCONFIG_UIRenderer_PlatinumAvailable

#endif // __ZUIRenderer_Platinum__
