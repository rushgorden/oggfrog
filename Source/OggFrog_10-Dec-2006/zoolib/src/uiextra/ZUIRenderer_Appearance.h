/*  @(#) $Id: ZUIRenderer_Appearance.h,v 1.8 2006/07/15 20:54:22 goingware Exp $ */

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

#ifndef __ZUIRenderer_Appearance__
#define __ZUIRenderer_Appearance__ 1
#include "zconfig.h"

#include "ZCONFIG_API.h"

namespace ZUIRenderer_Appearance
	{
	bool sCanUse();
	}

#ifndef ZCONFIG_API_Desired__Appearance
#	define ZCONFIG_API_Desired__Appearance 1
#endif


#ifndef ZCONFIG_API_Avail__Appearance
#	if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#		include <Appearance.h>
#		if !defined(UNIVERSAL_INTERFACES_VERSION) || UNIVERSAL_INTERFACES_VERSION < 0x0320
#			error We now *require* UNIVERSAL_INTERFACES_VERSION >= 0x0320
#		endif
#		if ZCONFIG(Processor, PPC)
#			define ZCONFIG_API_Avail__Appearance 1
#		endif
#	elif ZCONFIG(OS, MacOSX)
#		include <HIToolbox/Appearance.h>
#		define ZCONFIG_API_Avail__Appearance 1
#	endif
#endif

#ifndef ZCONFIG_API_Avail__Appearance
#	define ZCONFIG_API_Avail__Appearance 0
#endif


#if ZCONFIG_API_Enabled(Appearance)
#include "ZUIRendered.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Appearance

class ZUIRenderer_CaptionPane_Appearance : public ZUIRenderer_CaptionPane, private ZUICaptionRenderer
	{
public:
	ZUIRenderer_CaptionPane_Appearance();
	virtual ~ZUIRenderer_CaptionPane_Appearance();

// From ZUIRenderer_CaptionPane
	virtual void Activate(ZUICaptionPane_Rendered* inCaptionPane);
	virtual void Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption);

// From ZUICaptionRenderer
	virtual void Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);
	virtual void Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_AppearanceLabel

class ZUICaptionRenderer_AppearanceLabel : public ZUICaptionRenderer
	{
public:
	ZUICaptionRenderer_AppearanceLabel(ZRGBColor inTextColor, short inTextMode);
	virtual ~ZUICaptionRenderer_AppearanceLabel();

	virtual void Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth);
	virtual void Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo);

protected:
	ZRGBColor fTextColor;
	short fTextMode;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button_Appearance

class ZUIRenderer_Button_Appearance : public ZUIRenderer_Button
	{
public:
	ZUIRenderer_Button_Appearance();
	virtual ~ZUIRenderer_Button_Appearance();

	virtual void Activate(ZUIButton_Rendered* inButton);

	void Internal_RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState, ThemeButtonKind inButtonKind, bool inCenterH);

	static pascal void sDrawLabelCallback(const Rect *inBounds, ThemeButtonKind inButtonKind, const ThemeButtonDrawInfo* inDrawInfo, UInt32 inUserData, SInt16 inDepth, Boolean inColorDev);

	ZPoint Internal_GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption, ZPoint extraSpace, ThemeButtonKind inButtonKind);

	struct DrawLabelStruct
		{
		ZRef<ZUICaption> fCaption;
		ZUIRenderer_Button_Appearance* fRenderer;
		bool fCenterH;
		ZRect fButtonBounds;
		};
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Appearance

class ZUIRenderer_ButtonPush_Appearance : public ZUIRenderer_Button_Appearance
	{
public:
	ZUIRenderer_ButtonPush_Appearance();
	virtual ~ZUIRenderer_ButtonPush_Appearance();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	virtual void GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Appearance

class ZUIRenderer_ButtonRadio_Appearance : public ZUIRenderer_Button_Appearance
	{
public:
	ZUIRenderer_ButtonRadio_Appearance();
	virtual ~ZUIRenderer_ButtonRadio_Appearance();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Appearance

class ZUIRenderer_ButtonCheckbox_Appearance : public ZUIRenderer_Button_Appearance
	{
public:
	ZUIRenderer_ButtonCheckbox_Appearance();
	virtual ~ZUIRenderer_ButtonCheckbox_Appearance();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Appearance

class ZUIRenderer_ButtonPopup_Appearance : public ZUIRenderer_Button_Appearance
	{
public:
	ZUIRenderer_ButtonPopup_Appearance();
	virtual ~ZUIRenderer_ButtonPopup_Appearance();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonBevel_Appearance

class ZUIRenderer_ButtonBevel_Appearance : public ZUIRenderer_Button_Appearance
	{
public:
	ZUIRenderer_ButtonBevel_Appearance();
	virtual ~ZUIRenderer_ButtonBevel_Appearance();

	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonDisclosure_Appearance

class ZUIRenderer_ButtonDisclosure_Appearance : public ZUIRenderer_Button
	{
public:
	ZUIRenderer_ButtonDisclosure_Appearance();
	virtual ~ZUIRenderer_ButtonDisclosure_Appearance();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	virtual ZPoint GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPlacard_Appearance

class ZUIRenderer_ButtonPlacard_Appearance : public ZUIRenderer_Button
	{
public:
	ZUIRenderer_ButtonPlacard_Appearance();
	virtual ~ZUIRenderer_ButtonPlacard_Appearance();

	virtual void Activate(ZUIButton_Rendered* inButton);
	virtual void RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Appearance

class ZUIRenderer_ScrollBar_Appearance : public ZUIRenderer_ScrollBar_Base
	{
public:
	ZUIRenderer_ScrollBar_Appearance();
	virtual ~ZUIRenderer_ScrollBar_Appearance();

// From ZUIRenderer_ScrollBar
	virtual void Activate(ZUIScrollBar_Rendered* inScrollBar);
	virtual ZCoord GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar);

// From ZUIRenderer_ScrollBar_Base
	virtual void RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inWithBorder, double inValue, double inThumbProportion, double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart);
	virtual ZUIScrollBar::HitPart FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion);
	virtual double GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Appearance

class ZUIRenderer_LittleArrows_Appearance : public ZUIRenderer_LittleArrows_Base
	{
public:
	ZUIRenderer_LittleArrows_Appearance();
	virtual ~ZUIRenderer_LittleArrows_Appearance();

// From ZUIRenderer_LittleArrows
	virtual void Activate(ZUILittleArrows_Rendered* inLittleArrows);
	virtual ZPoint GetPreferredSize();

// From ZUIRenderer_LittleArrows_Base
	virtual void RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart);
	virtual ZUILittleArrows::HitPart FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Appearance

class ZUIRenderer_ProgressBar_Appearance : public ZUIRenderer_ProgressBar
	{
public:
	ZUIRenderer_ProgressBar_Appearance();
	virtual ~ZUIRenderer_ProgressBar_Appearance();

	virtual void Activate(ZUIProgressBar_Rendered* inProgressBar);
	virtual long GetFrameCount();
	virtual ZCoord GetHeight();
	virtual void DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge);
	virtual void DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame);
	virtual bigtime_t GetInterFrameDuration();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Appearance

class ZUIRenderer_ChasingArrows_Appearance : public ZUIRenderer_ChasingArrows
	{
public:
	ZUIRenderer_ChasingArrows_Appearance();
	virtual ~ZUIRenderer_ChasingArrows_Appearance();

	virtual void Activate(ZUIChasingArrows_Rendered* inChasingArrows);
	virtual long GetFrameCount();
	virtual ZPoint GetSize();
	virtual void DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame);
	virtual bigtime_t GetInterFrameDuration();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Appearance

class ZUIRenderer_TabPane_Appearance : public ZUIRenderer_TabPane
	{
public:
	ZUIRenderer_TabPane_Appearance();
	virtual ~ZUIRenderer_TabPane_Appearance();

// From ZUIRenderer_TabPane
	virtual void Activate(ZUITabPane_Rendered* inTabPane);

	virtual ZRect GetContentArea(ZUITabPane_Rendered* inTabPane);
	virtual bool GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk);

	virtual void RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds);
	virtual bool DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Appearance

class ZUIRenderer_Group_Appearance : public ZUIRenderer_Group
	{
public:
	ZUIRenderer_Group_Appearance(bool isPrimary);
	virtual ~ZUIRenderer_Group_Appearance();

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
#pragma mark * ZUIRenderer_FocusFrameEditText_Appearance

class ZUIRenderer_FocusFrameEditText_Appearance : public ZUIRenderer_FocusFrame
	{
public:
	ZUIRenderer_FocusFrameEditText_Appearance();
	virtual ~ZUIRenderer_FocusFrameEditText_Appearance();

	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame);
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds);
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrameListBox_Appearance

class ZUIRenderer_FocusFrameListBox_Appearance : public ZUIRenderer_FocusFrame
	{
public:
	ZUIRenderer_FocusFrameListBox_Appearance(ZRef<ZUIInk> inInternalBackInk);
	virtual ~ZUIRenderer_FocusFrameListBox_Appearance();

	virtual void Activate(ZUIFocusFrame_Rendered* inFocusFrame);
	virtual void Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState);
	virtual void GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);
	virtual ZDCRgn GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds);
	virtual bool GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk);

protected:
	ZRef<ZUIInk> fInternalBackInk;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Appearance

class ZUIRenderer_Divider_Appearance : public ZUIRenderer_Divider
	{
public:
	ZUIRenderer_Divider_Appearance();
	virtual ~ZUIRenderer_Divider_Appearance();

	virtual void Activate(ZUIDivider_Rendered* inDivider);
	virtual ZCoord GetDividerThickness();
	virtual void DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Appearance

class ZUIRendererFactory_Appearance : public ZUIRendererFactory_Caching
	{
public:
	ZUIRendererFactory_Appearance();
	virtual ~ZUIRendererFactory_Appearance();

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
#pragma mark * ZUIRendererFactory_Indirect

class ZUIAttributeFactory_Appearance : public ZUIAttributeFactory
	{
public:
	ZUIAttributeFactory_Appearance();
	virtual ~ZUIAttributeFactory_Appearance();

	virtual string GetName();

	virtual ZRef<ZUIColor> GetColor(const string& inColorName);
	virtual ZRef<ZUIInk> GetInk(const string& inInkName);
	virtual ZRef<ZUIFont> GetFont(const string& inFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& inMetricName);
	};

// =================================================================================================

#endif // ZCONFIG_API_Enabled(Appearance)

#endif // __ZUIRenderer_Appearance__
