static const char rcsid[] = "@(#) $Id: ZUIRenderer_Appearance.cpp,v 1.16 2006/08/02 21:38:59 agreen Exp $";

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

#include "ZUIRenderer_Appearance.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Appearance

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <ControlDefinitions.h>
#	include <Gestalt.h>
#endif

bool ZUIRenderer_Appearance::sCanUse()
	{
	#if ZCONFIG_API_Enabled(Appearance)
		// Is Appearance Manager installed and running?
		long response;
		if (::Gestalt(gestaltAppearanceAttr, &response) == noErr)
			{
			if (::Gestalt(gestaltAppearanceVersion, &response) == noErr)
				return response >= 0x0110;
			}
	#endif

	return false;
	}

// =================================================================================================
// =================================================================================================
#if ZCONFIG_API_Enabled(Appearance)

#include "ZDC_QD.h"
#include "ZMacOSX.h"
#include "ZString.h"
#include "ZTextUtil.h"
#include "ZTicks.h"
#include "ZUtil_Mac_LL.h"

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/Script.h>
#	include <HIToolbox/Controls.h>
#	include <HIToolbox/ControlDefinitions.h>
#else
#	include <Script.h> // For smSystemScript
#	include <Controls.h>
#	include <ControlDefinitions.h>
#endif

// =================================================================================================

static bool sMungeProc_Darken(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor.red >>= 1;
	inOutColor.green >>= 1;
	inOutColor.blue >>= 1;
	return true;
	}

static bool sMungeProc_Lighten(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - ((ZRGBColor::sWhite - inOutColor) / 2);
	return true;
	}

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

// =================================================================================================

static void sDrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{
	if (inDC.GetDepth() >= 4)
		{
		ZDCPixmap theSourcePixmap(inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono());
		if (inState.GetEnabled() && inState.GetActive())
			{
			if (inState.GetPressed())
				{
				// Darken
				theSourcePixmap.Munge(sMungeProc_Darken, nil);
				}
			else
				{
				// normal -- do nothing to the pixmap
				}
			}
		else
			{
			// Lighten
			theSourcePixmap.Munge(sMungeProc_Lighten, nil);
			}
		inDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	else
		{
		ZDCPixmap theSourcePixmap(inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor());
		if (inState.GetEnabled() && inState.GetActive())
			{
			if (inState.GetPressed())
				{
				// Draw b & w swapped
				theSourcePixmap.Munge(sMungeProc_Invert, nil);
				}
			else
				{
				// normal -- do nothing to the pixmap
				}
			}
		else
			{
			// Draw black pixels with 50% dither, white pixels as normal
			// theSourcePixmap.Munge(sMungeProc_Dither, nil, nil);
			}
		inDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	}
// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Appearance

class ZUIColor_Appearance : public ZUIColor
	{
public:
	ZUIColor_Appearance(ThemeBrush inBrush);
	virtual ~ZUIColor_Appearance();
	virtual ZRGBColor GetColor();
protected:
	ThemeBrush fThemeBrush;
	};

ZUIColor_Appearance::ZUIColor_Appearance(ThemeBrush inBrush)
:	fThemeBrush(inBrush)
	{}

ZUIColor_Appearance::~ZUIColor_Appearance()
	{}

ZRGBColor ZUIColor_Appearance::GetColor()
	{
	RGBColor theColor;
	if (::GetThemeBrushAsColor(fThemeBrush, 16, true, &theColor) == noErr)
		return theColor;
	return ZRGBColor::sBlack;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Appearance

class ZUIInk_Appearance : public ZUIInk
	{
public:
	ZUIInk_Appearance(ThemeBrush inBrush);
	virtual ~ZUIInk_Appearance();
	virtual ZDCInk GetInk();
protected:
	ThemeBrush fThemeBrush;
	};

ZUIInk_Appearance::ZUIInk_Appearance(ThemeBrush inBrush)
:	fThemeBrush(inBrush)
	{}

ZUIInk_Appearance::~ZUIInk_Appearance()
	{}

ZDCInk ZUIInk_Appearance::GetInk()
	{
	ZDC theDCScratch = ZDCScratch::sGet();
	ZDCSetupForQD theSetupForQD(theDCScratch, true);

	// Reset the port to flush any prior pen pix pat etc.
	::NormalizeThemeDrawingState();

	CGrafPtr theWorkPort = theSetupForQD.GetCGrafPtr();

	ZDCInk resultInk;
	RGBColor tempRGBColor;
	if (noErr == ::SetThemePen(fThemeBrush, 16, true))
		{
		#if ACCESSOR_CALLS_ARE_FUNCTIONS
			PixPatHandle thePenPPH = ::NewPixPat();
			::GetPortPenPixPat(theWorkPort, thePenPPH);
			if (thePenPPH[0]->patMap[0]->pixelSize == 1)
				{
				RGBColor foreColor, backColor;
				::GetPortForeColor(theWorkPort, &foreColor);
				::GetPortBackColor(theWorkPort, &backColor);
				ZDCPattern thePattern = *reinterpret_cast<ZDCPattern*>(&thePenPPH[0]->pat1Data);
				resultInk = ZDCInk(ZRGBColor(foreColor), ZRGBColor(foreColor), thePattern);
				}
			else
				{
				ZDCPixmap thePixmap;
				ZUtil_Mac_LL::sPixmapFromPixPatHandle(thePenPPH, &thePixmap, nil);
				resultInk = thePixmap;
				}
			::DisposePixPat(thePenPPH);
		#else
			// If the fgColor field is non zero, then we're using a two color pattern. This was determined
			// empirically, not from any documented source. Your Milage May Vary.
			if (theWorkPort->fgColor != 0 || theWorkPort->pnPixPat[0]->patMap[0]->pixelSize == 1)
				{
				ZDCPattern thePattern = *reinterpret_cast<ZDCPattern*>(&theWorkPort->pnPixPat[0]->pat1Data);
				resultInk = ZDCInk(ZRGBColor(theWorkPort->rgbFgColor), ZRGBColor(theWorkPort->rgbBkColor), thePattern);
				}
			else
				{
				ZDCPixmap thePixmap;
				ZUtil_Mac_LL::sPixmapFromPixPatHandle(theWorkPort->pnPixPat, &thePixmap, nil);
				resultInk = thePixmap;
				}
		#endif
		}
	else if (::GetThemeBrushAsColor(fThemeBrush, 16, true, &tempRGBColor) == noErr)
		{
		resultInk = ZRGBColor(tempRGBColor);
		}
	else
		{
		resultInk = ZRGBColor::sBlack;
		}

	return resultInk;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Appearance

class ZUIFont_Appearance : public ZUIFont
	{
public:
	ZUIFont_Appearance(ThemeFontID inThemeFontID);
	virtual ~ZUIFont_Appearance();
	virtual ZDCFont GetFont();
protected:
	ThemeFontID fThemeFontID;
	};

ZUIFont_Appearance::ZUIFont_Appearance(ThemeFontID inThemeFontID)
:	fThemeFontID(inThemeFontID)
	{}

ZUIFont_Appearance::~ZUIFont_Appearance()
	{}

ZDCFont ZUIFont_Appearance::GetFont()
	{
	Str255 theFontName;
	SInt16 theFontSize;
	Style theStyle;
	if (::GetThemeFont(fThemeFontID, smSystemScript, theFontName, &theFontSize, &theStyle) == noErr)
		return ZDCFont(ZString::sFromPString(theFontName), theStyle, theFontSize);
	return ZDCFont();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Appearance

ZUIRenderer_CaptionPane_Appearance::ZUIRenderer_CaptionPane_Appearance()
	{}

ZUIRenderer_CaptionPane_Appearance::~ZUIRenderer_CaptionPane_Appearance()
	{}

void ZUIRenderer_CaptionPane_Appearance::Activate(ZUICaptionPane_Rendered* inCaptionPane)
	{ inCaptionPane->Refresh(); }

void ZUIRenderer_CaptionPane_Appearance::Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (inCaption)
		inCaption->Draw(localDC, inBounds.TopLeft(), inState, this);
	}

ZPoint ZUIRenderer_CaptionPane_Appearance::GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption)
	{
	if (inCaption)
		return inCaption->GetSize();
	return ZPoint::sZero;
	}

void ZUIRenderer_CaptionPane_Appearance::Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{
	ZCoord wrapWidth(inWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	localDC.SetFont(inFont);
	RGBColor theTextColor;
	::GetThemeTextColor(inState.GetEnabled() && inState.GetActive() ? kThemeTextColorDialogActive : kThemeTextColorDialogInactive, localDC.GetDepth(), localDC.GetDepth() >= 4, &theTextColor);
	localDC.SetTextColor(theTextColor);
	ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
	}

void ZUIRenderer_CaptionPane_Appearance::Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{ sDrawCaptionPixmap(inDC, inLocation, inState, inPixmapCombo); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_AppearanceLabel

ZUICaptionRenderer_AppearanceLabel::ZUICaptionRenderer_AppearanceLabel(ZRGBColor inTextColor, short inTextMode)
:	fTextColor(inTextColor), fTextMode(inTextMode)
	{}

ZUICaptionRenderer_AppearanceLabel::~ZUICaptionRenderer_AppearanceLabel()
	{}

void ZUICaptionRenderer_AppearanceLabel::Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{
	ZCoord wrapWidth(inWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	localDC.SetFont(inFont);
	localDC.SetTextColor(fTextColor);
	ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
	}

void ZUICaptionRenderer_AppearanceLabel::Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{ sDrawCaptionPixmap(inDC, inLocation, inState, inPixmapCombo); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Appearance

ZUIRenderer_Button_Appearance::ZUIRenderer_Button_Appearance()
	{}

ZUIRenderer_Button_Appearance::~ZUIRenderer_Button_Appearance()
	{}

static ThemeButtonDrawUPP sDrawLabelCallbackUPP = NewThemeButtonDrawUPP(ZUIRenderer_Button_Appearance::sDrawLabelCallback);

void ZUIRenderer_Button_Appearance::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_Button_Appearance::Internal_RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState, ThemeButtonKind inButtonKind, bool inCenterH)
	{
	ZDC localDC(inDC);
	ZDCSetupForQD theSetupForQD(localDC, true);

	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect buttonBounds(inBounds + qdOffset);
	
	ThemeButtonDrawInfo theInfo;
	if (inState.GetEnabled() && inState.GetActive())
		{
		if (inState.GetPressed())
			theInfo.state = kThemeStatePressed;
		else
			theInfo.state = kThemeStateActive;
		}
	else
		{
		theInfo.state = kThemeStateInactive;
		}

	if (inState.GetHilite() == eZTriState_Off)
		theInfo.value = kThemeButtonOff;
	else if (inState.GetHilite() == eZTriState_Mixed)
		theInfo.value = kThemeButtonMixed;
	else
		theInfo.value = kThemeButtonOn;

	theInfo.adornment = kThemeAdornmentNone;
	if (inState.GetFocused())
		theInfo.adornment |= kThemeAdornmentFocus;

	if (inCaption)
		{
		DrawLabelStruct theDrawLabelStruct;
		theDrawLabelStruct.fCaption = inCaption;
		theDrawLabelStruct.fRenderer = this;
		theDrawLabelStruct.fCenterH = inCenterH;
		theDrawLabelStruct.fButtonBounds = buttonBounds;
		::DrawThemeButton(&buttonBounds, inButtonKind, &theInfo, nil, nil, sDrawLabelCallbackUPP, reinterpret_cast<long>(&theDrawLabelStruct));
		}
	else
		{
		::DrawThemeButton(&buttonBounds, inButtonKind, &theInfo, nil, nil, nil, 0);
		}
	}

pascal void ZUIRenderer_Button_Appearance::sDrawLabelCallback(const Rect *inBounds, ThemeButtonKind inButtonKind, const ThemeButtonDrawInfo* inDrawInfo, UInt32 inUserData, SInt16 inDepth, Boolean inColorDev)
	{
	if (inUserData)
		{
		DrawLabelStruct* theDrawLabelStruct = reinterpret_cast<DrawLabelStruct*>(inUserData);
		if (theDrawLabelStruct->fRenderer)
			{
			bool pressed = (inDrawInfo->state == kThemeStatePressed);
			EZTriState hilite;
			if (inDrawInfo->value == kThemeButtonOff)
				hilite = eZTriState_Off;
			else if (inDrawInfo->value == kThemeButtonMixed)
				hilite = eZTriState_Mixed;
			else
				hilite = eZTriState_On;
			bool enabledAndActive = (inDrawInfo->state != kThemeStateInactive);
			ZUIDisplayState theDisplayState(pressed, hilite, enabledAndActive, false, enabledAndActive);

			// Get theDC_Native set up ASAP, so we haven't had a chance to make calls that could switch the port around
			CGrafPtr currentPort;
			GDHandle currentGDHandle;
			::GetGWorld(&currentPort, &currentGDHandle);

			ZDC_NativeQD theDC_Native(currentPort);

			#if ACCESSOR_CALLS_ARE_FUNCTIONS
				RGBColor theForeColor;
				::GetForeColor(&theForeColor);
				ZUICaptionRenderer_AppearanceLabel theRenderer(theForeColor, ::GetPortTextMode(currentPort));
			#else
				ZUICaptionRenderer_AppearanceLabel theRenderer(currentPort->rgbFgColor, currentPort->txMode);
			#endif

			ZRect realDestBounds = *inBounds;

			ZRect captionBounds(theDrawLabelStruct->fCaption->GetSize());
		
			if (theDrawLabelStruct->fCenterH)
				{
				captionBounds = captionBounds.Centered(realDestBounds);
				}
			else
				{
				captionBounds += realDestBounds.TopLeft() - captionBounds.TopLeft();
				captionBounds = captionBounds.CenteredV(realDestBounds);
				}

			theDC_Native.SetClip(theDC_Native.GetClip() & captionBounds);
			theDrawLabelStruct->fCaption->Draw(theDC_Native, captionBounds.TopLeft(), theDisplayState, &theRenderer);
			}
		}
	}

ZPoint ZUIRenderer_Button_Appearance::Internal_GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption, ZPoint extraSpace, ThemeButtonKind inButtonKind)
	{
	Rect startBounds = { 0, 0, 100, 100 };
	Rect contentBounds;
	ThemeButtonDrawInfo theInfo;
	theInfo.state = kThemeStateActive;
	theInfo.value = kThemeButtonOff;
	theInfo.adornment = kThemeAdornmentNone;

	::GetThemeButtonContentBounds(&startBounds, inButtonKind, &theInfo, &contentBounds);

	ZPoint theSize(extraSpace.h + contentBounds.left - startBounds.left + startBounds.right - contentBounds.right,
				extraSpace.v + contentBounds.top - startBounds.top + startBounds.bottom - contentBounds.bottom);

	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		theSize += captionSize;
		}

	return theSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Appearance

ZUIRenderer_ButtonPush_Appearance::ZUIRenderer_ButtonPush_Appearance()
	{}

ZUIRenderer_ButtonPush_Appearance::~ZUIRenderer_ButtonPush_Appearance()
	{}

void ZUIRenderer_ButtonPush_Appearance::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;
	
	ThemeButtonDrawInfo theInfo;
	if (inState.GetEnabled() && inState.GetActive())
		{
		if (inState.GetPressed())
			theInfo.state = kThemeStatePressed;
		else
			theInfo.state = kThemeStateActive;
		}
	else
		{
		theInfo.state = kThemeStateInactive;
		}

	if (inState.GetHilite() == eZTriState_Off)
		theInfo.value = kThemeButtonOff;
	else if (inState.GetHilite() == eZTriState_Mixed)
		theInfo.value = kThemeButtonMixed;
	else
		theInfo.value = kThemeButtonOn;

	theInfo.adornment = kThemeAdornmentNone;

	if (inState.GetFocused())
		theInfo.adornment |= kThemeAdornmentFocus;

	if (isDefault)
		theInfo.adornment |= kThemeAdornmentDefault;

	ZRect buttonBounds;
	if (ZCONFIG(OS, Carbon) && ZMacOSX::sIsMacOSX())
		{
		// For aqua we *don't* use the appearance manager API. That's
		// because the background bounds of a push button is something like
		// 4 pixels on the left, top and right edges, and 8 on the bottom.
		// We manually use (1, 0, -1 and -3) for the insets, just for now,
		// until I upgrade the API to allow a button to be big, but only
		// responsive in its actual rect, not its background.
		buttonBounds.left = inBounds.left + 1;
		buttonBounds.top = inBounds.top;
		buttonBounds.right = inBounds.right - 1;
		buttonBounds.bottom = inBounds.bottom - 3;		
		}
	else
		{
		Rect startBounds = { 0, 0, 100, 100 };
		Rect backgroundBounds;
		::GetThemeButtonBackgroundBounds(&startBounds, kThemePushButton, &theInfo, &backgroundBounds);

		buttonBounds.left = inBounds.left + (startBounds.left - backgroundBounds.left);
		buttonBounds.right = inBounds.right - (backgroundBounds.right - startBounds.right);
		buttonBounds.top = inBounds.top + (startBounds.top - backgroundBounds.top);
		buttonBounds.bottom = inBounds.bottom - (backgroundBounds.bottom - startBounds.bottom);
		}

	ZDC localDC(inDC);

	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();
	Rect qdButtonBounds = buttonBounds + qdOffset;

	if (inCaption)
		{
		DrawLabelStruct theDrawLabelStruct;
		theDrawLabelStruct.fCaption = inCaption;
		theDrawLabelStruct.fRenderer = this;
		theDrawLabelStruct.fCenterH = true;
		theDrawLabelStruct.fButtonBounds = qdButtonBounds;
		::DrawThemeButton(&qdButtonBounds, kThemePushButton, &theInfo, nil, nil, sDrawLabelCallbackUPP, reinterpret_cast<long>(&theDrawLabelStruct));
		}
	else
		{
		::DrawThemeButton(&qdButtonBounds, kThemePushButton, &theInfo, nil, nil, nil, 0);
		}
	}

ZPoint ZUIRenderer_ButtonPush_Appearance::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;

	ThemeButtonDrawInfo theInfo;
	theInfo.state = kThemeStateActive;
	theInfo.value = kThemeButtonOff;
	theInfo.adornment = kThemeAdornmentNone;

	if (isDefault)
		theInfo.adornment |= kThemeAdornmentDefault;

	Rect startBounds = { 0, 0, 100, 100 };

	Rect contentBounds;
	::GetThemeButtonContentBounds(&startBounds, kThemePushButton, &theInfo, &contentBounds);

	ZPoint theSize;

	if (ZCONFIG(OS, Carbon) && ZMacOSX::sIsMacOSX())
		{
		theSize.h = (contentBounds.left - startBounds.left) + (startBounds.right - contentBounds.right)
					+ 2;
		theSize.v = (contentBounds.top - startBounds.top) + (startBounds.bottom - contentBounds.bottom)
					+ 3;
		}
	else
		{
		Rect backgroundBounds;
		::GetThemeButtonBackgroundBounds(&startBounds, kThemePushButton, &theInfo, &backgroundBounds);

		theSize.h = (contentBounds.left - startBounds.left) + (startBounds.right - contentBounds.right)
					+ (startBounds.left - backgroundBounds.left) + (backgroundBounds.right - startBounds.right);
		theSize.v = (contentBounds.top - startBounds.top) + (startBounds.bottom - contentBounds.bottom)
					+ (startBounds.top - backgroundBounds.top) + (backgroundBounds.bottom - startBounds.bottom);
		}

	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		theSize += captionSize;
		}

	return theSize;
	}

void ZUIRenderer_ButtonPush_Appearance::GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;

	ThemeButtonDrawInfo theInfo;
	theInfo.state = kThemeStateActive;
	theInfo.value = kThemeButtonOff;
	theInfo.adornment = kThemeAdornmentNone;

	if (isDefault)
		theInfo.adornment |= kThemeAdornmentDefault;

	if (ZCONFIG(OS, Carbon) && ZMacOSX::sIsMacOSX())
		{
		outTopLeftInset.h = -1;
		outTopLeftInset.v = 0;

		outBottomRightInset.h = 1;
		outBottomRightInset.v = 3;
		}
	else
		{
		Rect startBounds = { 0, 0, 100, 100 };

		Rect backgroundBounds;
		::GetThemeButtonBackgroundBounds(&startBounds, kThemePushButton, &theInfo, &backgroundBounds);

		outTopLeftInset.h = startBounds.left - backgroundBounds.left;
		outTopLeftInset.v = startBounds.top - backgroundBounds.top;

		outBottomRightInset.h = startBounds.right - backgroundBounds.right;
		outBottomRightInset.v = startBounds.bottom - backgroundBounds.bottom;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Appearance

ZUIRenderer_ButtonRadio_Appearance::ZUIRenderer_ButtonRadio_Appearance()
	{}

ZUIRenderer_ButtonRadio_Appearance::~ZUIRenderer_ButtonRadio_Appearance()
	{}

void ZUIRenderer_ButtonRadio_Appearance::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{ this->Internal_RenderButton(inButton, inDC, inBounds, inCaption, inState, kThemeRadioButton, false); }

ZPoint ZUIRenderer_ButtonRadio_Appearance::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return this->Internal_GetPreferredDimensions(inButton, inCaption, ZPoint::sZero, kThemeRadioButton); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Appearance

ZUIRenderer_ButtonCheckbox_Appearance::ZUIRenderer_ButtonCheckbox_Appearance()
	{}

ZUIRenderer_ButtonCheckbox_Appearance::~ZUIRenderer_ButtonCheckbox_Appearance()
	{}

void ZUIRenderer_ButtonCheckbox_Appearance::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{ this->Internal_RenderButton(inButton, inDC, inBounds, inCaption, inState, kThemeCheckBox, false); }

ZPoint ZUIRenderer_ButtonCheckbox_Appearance::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return this->Internal_GetPreferredDimensions(inButton, inCaption, ZPoint::sZero, kThemeCheckBox); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Appearance

ZUIRenderer_ButtonPopup_Appearance::ZUIRenderer_ButtonPopup_Appearance()
	{}

ZUIRenderer_ButtonPopup_Appearance::~ZUIRenderer_ButtonPopup_Appearance()
	{}

void ZUIRenderer_ButtonPopup_Appearance::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{ this->Internal_RenderButton(inButton, inDC, inBounds, inCaption, inState, kThemePopupButton, false); }

ZPoint ZUIRenderer_ButtonPopup_Appearance::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return this->Internal_GetPreferredDimensions(inButton, inCaption, ZPoint(12,0), kThemePopupButton); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonBevel_Appearance

ZUIRenderer_ButtonBevel_Appearance::ZUIRenderer_ButtonBevel_Appearance()
	{}

ZUIRenderer_ButtonBevel_Appearance::~ZUIRenderer_ButtonBevel_Appearance()
	{}

void ZUIRenderer_ButtonBevel_Appearance::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZCoord bevelWidth;
	if (!inButton->GetAttribute("ZUIButton::BevelWidth", nil, &bevelWidth))
		bevelWidth = 1;
	bevelWidth = max((ZCoord)1, min(bevelWidth, (ZCoord)3));

	ThemeButtonKind theThemeButtonKind;
	if (bevelWidth == 1)
		theThemeButtonKind = kThemeSmallBevelButton;
	else if (bevelWidth == 2)
		theThemeButtonKind = kThemeMediumBevelButton;
	else
		theThemeButtonKind = kThemeLargeBevelButton;
	this->Internal_RenderButton(inButton, inDC, inBounds, inCaption, inState, theThemeButtonKind, true);
	}

ZPoint ZUIRenderer_ButtonBevel_Appearance::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZCoord bevelWidth;
	if (!inButton->GetAttribute("ZUIButton::BevelWidth", nil, &bevelWidth))
		bevelWidth = 1;
	bevelWidth = max((ZCoord)1, min(bevelWidth, (ZCoord)3));

	ThemeButtonKind theThemeButtonKind;
	if (bevelWidth == 1)
		theThemeButtonKind = kThemeSmallBevelButton;
	else if (bevelWidth == 2)
		theThemeButtonKind = kThemeMediumBevelButton;
	else
		theThemeButtonKind = kThemeLargeBevelButton;

	return this->Internal_GetPreferredDimensions(inButton, inCaption, ZPoint::sZero, theThemeButtonKind);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Appearance

ZUIRenderer_ScrollBar_Appearance::ZUIRenderer_ScrollBar_Appearance()
	{}

ZUIRenderer_ScrollBar_Appearance::~ZUIRenderer_ScrollBar_Appearance()
	{}

void ZUIRenderer_ScrollBar_Appearance::Activate(ZUIScrollBar_Rendered* inScrollBar)
	{ inScrollBar->Refresh(); }

ZCoord ZUIRenderer_ScrollBar_Appearance::GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar)
	{
	if (inScrollBar && inScrollBar->GetWithBorder())
		return 16;
	return 15;
	}

void ZUIRenderer_ScrollBar_Appearance::RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inWithBorder, double inValue, double inThumbProportion, double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart)
	{
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect localBounds = inBounds;
	ZDC localDC(inDC);
	if (!inWithBorder)
		{
		localDC.SetClip(localDC.GetClip() & localBounds);
		if (horizontal)
			{
			localBounds.left -= 1;
			localBounds.right += 1;
			localBounds.bottom += 1;
			}
		else
			{
			localBounds.top -= 1;
			localBounds.bottom += 1;
			localBounds.right += 1;
			}
		}

	bool slideable = inThumbProportion < 1.0;
	ThemeTrackEnableState theEnableState;
	if (inActive)
		{
		if (inEnabled)
			{
			if (slideable)
				theEnableState = kThemeTrackActive;
			else
				theEnableState = kThemeTrackNothingToScroll;
			}
		else
			{
			theEnableState = kThemeTrackDisabled;
			}
		}
	else
		{
		theEnableState = kThemeTrackInactive;
		}

	ThemeTrackPressState thePressState = 0;
	if (inHitPart != ZUIScrollBar::hitPartNone)
		{
		if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartUpArrowOuter)
			{
			thePressState = kThemeLeftOutsideArrowPressed;
			}
		else if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartUpArrowInner)
			{
			// AG 2002-06-21. Note that the classic appearance manager does not
			// seem to handle this case correctly -- nothing is drawn hilited.
			thePressState = kThemeLeftInsideArrowPressed;
			}
		else if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartDownArrowOuter)
			{
			thePressState = kThemeRightOutsideArrowPressed;
			}
		else if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartDownArrowInner)
			{
			thePressState = kThemeRightInsideArrowPressed;
			}
		else if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartPageUp)
			{
			thePressState = kThemeLeftTrackPressed;
			}
		else if (inHitPart == ZUIScrollBar::ZUIScrollBar::hitPartPageDown)
			{
			thePressState = kThemeRightTrackPressed;
			}
		else if (inHitPart == ZUIScrollBar::hitPartThumb)
			{
			thePressState = kThemeThumbPressed;
			}
		}
		
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect qdRect = localBounds + qdOffset;
	Rect resultTrackRect;
	::DrawThemeScrollBarArrows(&qdRect, theEnableState, thePressState, horizontal, &resultTrackRect);

	// Now check the track
	ThemeTrackDrawInfo theInfo;
	theInfo.bounds = resultTrackRect;
	theInfo.attributes = kThemeTrackShowThumb;
	if (horizontal)
		theInfo.attributes |= kThemeTrackHorizontal;
	theInfo.enableState = theEnableState;
	theInfo.trackInfo.scrollbar.pressState = thePressState;

	theInfo.min = 0;
	theInfo.trackInfo.scrollbar.viewsize = 0x100000 * inThumbProportion;
	theInfo.max = 0x100000 - theInfo.trackInfo.scrollbar.viewsize;
	theInfo.kind = kThemeScrollBar;

	RgnHandle ghostThumbRgnHandle = nil;
	if (inGhostThumbValue >= 0.0 && inGhostThumbValue <= 1.0)
		{
		ghostThumbRgnHandle = ::NewRgn();
		theInfo.value = theInfo.max * inGhostThumbValue;
		::GetThemeTrackThumbRgn(&theInfo, ghostThumbRgnHandle);
		}

	theInfo.value = theInfo.max * inValue;

	::DrawThemeTrack(&theInfo, ghostThumbRgnHandle, nil, 0);
	if (ghostThumbRgnHandle)
		::DisposeRgn(ghostThumbRgnHandle);
	}

ZUIScrollBar::HitPart ZUIRenderer_ScrollBar_Appearance::FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion)
	{
	if (!inEnabled)
		return ZUIScrollBar::hitPartNone;
	if (inThumbProportion > 1.0)
		return ZUIScrollBar::hitPartNone;

	// Compensate if we're not supposed to show our border
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect localBounds(inBounds);
	if (!inWithBorder)
		{
		if (horizontal)
			{
			localBounds.left -= 1;
			localBounds.right += 1;
			localBounds.bottom += 1;
			}
		else
			{
			localBounds.top -= 1;
			localBounds.bottom += 1;
			localBounds.right += 1;
			}
		}
	// First check the arrows
	Rect qdRect(localBounds);
	Rect resultTrackRect;
	ControlPartCode resultPartCode;
	if (::HitTestThemeScrollBarArrows(&qdRect, kThemeTrackActive, 0, horizontal, inHitPoint, &resultTrackRect, &resultPartCode))
		{
		ThemeScrollBarArrowStyle theArrowStyle;
		::GetThemeScrollBarArrowStyle(&theArrowStyle);
		if (theArrowStyle == kThemeScrollBarArrowsLowerRight)
			{
			if (resultPartCode == kControlUpButtonPart)
				return ZUIScrollBar::hitPartUpArrowInner;
			else if (resultPartCode == kControlDownButtonPart)
				return ZUIScrollBar::hitPartDownArrowOuter;
			}
		else if (theArrowStyle == kThemeScrollBarArrowsSingle)
			{
			if (resultPartCode == kControlUpButtonPart)
				return ZUIScrollBar::hitPartUpArrowOuter;
			else if (resultPartCode == kControlDownButtonPart)
				return ZUIScrollBar::hitPartDownArrowOuter;
			}
		else
			{
			switch (resultPartCode)
				{
				case kControlUpButtonPart:
					return ZUIScrollBar::hitPartUpArrowOuter;
				case kControlDownButtonPart:
					return ZUIScrollBar::hitPartDownArrowOuter;
				case 28: // no symbolic name for this part
					return ZUIScrollBar::hitPartUpArrowInner;
				case 29: // no symbolic name for this part
					return ZUIScrollBar::hitPartDownArrowInner;
				}
			}
		return ZUIScrollBar::hitPartNone;
		}
	// Now check the track
	ThemeTrackDrawInfo theInfo;

	theInfo.min = 0;
	theInfo.trackInfo.scrollbar.viewsize = 0x100000 * inThumbProportion;
	theInfo.max = 0x100000 - theInfo.trackInfo.scrollbar.viewsize;
	theInfo.value = theInfo.max * inValue;

	theInfo.kind = kThemeScrollBar;
	theInfo.bounds = resultTrackRect;
	theInfo.attributes = kThemeTrackShowThumb;
	if (horizontal)
		theInfo.attributes |= kThemeTrackHorizontal;
	theInfo.enableState = kThemeTrackActive;
	theInfo.trackInfo.scrollbar.pressState = 0;
	if (::HitTestThemeTrack(&theInfo, inHitPoint, &resultPartCode))
		{
		if (resultPartCode == kControlPageUpPart)
			return ZUIScrollBar::hitPartPageUp;
		else if (resultPartCode == kControlPageDownPart)
			return ZUIScrollBar::hitPartPageDown;
		else if (resultPartCode == kControlIndicatorPart)
			return ZUIScrollBar::hitPartThumb;
		return ZUIScrollBar::hitPartNone;
		}
	return ZUIScrollBar::hitPartNone;
	}

double ZUIRenderer_ScrollBar_Appearance::GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta)
	{
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect localBounds(inBounds);
	if (!inWithBorder)
		{
		if (horizontal)
			{
			localBounds.left -= 1;
			localBounds.right += 1;
			localBounds.bottom += 1;
			}
		else
			{
			localBounds.top -= 1;
			localBounds.bottom += 1;
			localBounds.right += 1;
			}
		}

	// AG 98-09-29. I'm not sure that this is correct.
	ThemeTrackDrawInfo theInfo;
	Rect qdBounds(localBounds);
	::GetThemeScrollBarTrackRect(&qdBounds, kThemeTrackActive, kThemeThumbPressed, horizontal, &theInfo.bounds);

	theInfo.min = 0;
	theInfo.trackInfo.scrollbar.viewsize = 0x100000 * inThumbProportion;
	theInfo.max = 0x100000 - theInfo.trackInfo.scrollbar.viewsize;
	theInfo.value = theInfo.max * inValue;
	theInfo.kind = kThemeScrollBar;

	theInfo.attributes = kThemeTrackShowThumb;
	if (horizontal)
		theInfo.attributes |= kThemeTrackHorizontal;
	theInfo.enableState = kThemeTrackActive;
	theInfo.trackInfo.scrollbar.pressState = kThemeThumbPressed;

	SInt32 offset(horizontal ? inPixelDelta.h : inPixelDelta.v);
	SInt32 delta;
	::GetThemeTrackLiveValue(&theInfo, offset < 0 ? -offset : offset, &delta);
	if (offset < 0)
		delta = -delta;
	return ((double)delta) / ((double)(0x100000 - theInfo.trackInfo.scrollbar.viewsize));
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Appearance

ZUIRenderer_LittleArrows_Appearance::ZUIRenderer_LittleArrows_Appearance()
	{}

ZUIRenderer_LittleArrows_Appearance::~ZUIRenderer_LittleArrows_Appearance()
	{}

void ZUIRenderer_LittleArrows_Appearance::Activate(ZUILittleArrows_Rendered* inLittleArrows)
	{ inLittleArrows->Refresh(); }

ZPoint ZUIRenderer_LittleArrows_Appearance::GetPreferredSize()
	{
	return ZPoint(13,23);
	}

void ZUIRenderer_LittleArrows_Appearance::RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart)
	{
	ZDC localDC(inDC);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect buttonBounds(inBounds + qdOffset);

	ThemeButtonDrawInfo theInfo;
	if (inEnabled && inActive)
		{
		if (inHitPart == ZUILittleArrows::hitPartUp)
			theInfo.state = kThemeStatePressedUp;
		else if (inHitPart == ZUILittleArrows::hitPartDown)
			theInfo.state = kThemeStatePressedDown;
		else
			theInfo.state = kThemeStateActive;
		}
	else
		theInfo.state = kThemeStateInactive;

	theInfo.value = kThemeButtonOff;
	theInfo.adornment = kThemeAdornmentNone;
	::DrawThemeButton(&buttonBounds, kThemeIncDecButton, &theInfo, nil, nil, nil, 0);
	}

ZUILittleArrows::HitPart ZUIRenderer_LittleArrows_Appearance::FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled)
	{
	if (!inEnabled || !inBounds.Contains(inHitPoint))
		return ZUILittleArrows::hitPartNone;
	if (inHitPoint.v < (inBounds.top + inBounds.Height()/2))
		return ZUILittleArrows::hitPartUp;
	return ZUILittleArrows::hitPartDown;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Appearance

ZUIRenderer_Divider_Appearance::ZUIRenderer_Divider_Appearance()
	{}

ZUIRenderer_Divider_Appearance::~ZUIRenderer_Divider_Appearance()
	{}

void ZUIRenderer_Divider_Appearance::Activate(ZUIDivider_Rendered* inDivider)
	{ inDivider->Refresh(); }

ZCoord ZUIRenderer_Divider_Appearance::GetDividerThickness()
	{
	return 2;
	}

void ZUIRenderer_Divider_Appearance::DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect qdRect;
	qdRect.left = inLocation.h + qdOffset.h;
	qdRect.top = inLocation.v + qdOffset.v;
	if (inHorizontal)
		{
		qdRect.right = inLocation.h + qdOffset.h + inLength;
		qdRect.bottom = inLocation.v + qdOffset.v + 2;
		}
	else
		{
		qdRect.right = inLocation.h + qdOffset.h + 2;
		qdRect.bottom = inLocation.v + qdOffset.v + inLength;
		}
	::DrawThemeSeparator(&qdRect, inState.GetEnabled() && inState.GetActive() ? kThemeStateActive : kThemeStateInactive);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Appearance

static const ZCoord kProgressHeight = 14;

ZUIRenderer_ProgressBar_Appearance::ZUIRenderer_ProgressBar_Appearance()
	{}

ZUIRenderer_ProgressBar_Appearance::~ZUIRenderer_ProgressBar_Appearance()
	{}

void ZUIRenderer_ProgressBar_Appearance::Activate(ZUIProgressBar_Rendered* inProgressBar)
	{ inProgressBar->Refresh(); }

long ZUIRenderer_ProgressBar_Appearance::GetFrameCount()
	{ return 16; }

ZCoord ZUIRenderer_ProgressBar_Appearance::GetHeight()
	{ return kProgressHeight; }

void ZUIRenderer_ProgressBar_Appearance::DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge)
	{
	ZDC localDC(inDC);
	localDC.SetInk(ZRGBColor::sBlack);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	ThemeTrackDrawInfo theInfo;
	theInfo.kind = kThemeProgressBar;
	theInfo.bounds = inBounds + qdOffset;
	theInfo.min = 0;
	theInfo.max = 0x7FFFFFFFL;
	theInfo.value = 0x7FFFFFFFL * inRightEdge;
	theInfo.attributes = kThemeTrackHorizontal;
	if (inEnabled)
		{
		if (inActive)
			theInfo.enableState = kThemeTrackActive;
		else
			theInfo.enableState = kThemeTrackInactive;
		}
	else
		{
		theInfo.enableState = kThemeTrackDisabled;
		}
	theInfo.trackInfo.progress.phase = 0;
	::DrawThemeTrack(&theInfo, nil, nil, 0);
	}

void ZUIRenderer_ProgressBar_Appearance::DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame)
	{
	ZDC localDC(inDC);
	localDC.SetInk(ZRGBColor::sBlack);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	ThemeTrackDrawInfo theInfo;
	theInfo.kind = kThemeIndeterminateBar;
	theInfo.bounds = inBounds + qdOffset;
	theInfo.min = 0;
	theInfo.max = 0;
	theInfo.value = 0;
	theInfo.attributes = kThemeTrackHorizontal;
	if (inEnabled)
		{
		if (inActive)
			theInfo.enableState = kThemeTrackActive;
		else
			theInfo.enableState = kThemeTrackInactive;
		}
	else
		{
		theInfo.enableState = kThemeTrackDisabled;
		}
	theInfo.trackInfo.progress.phase = (inFrame % 16) + 1;
	::DrawThemeTrack(&theInfo, nil, nil, 0);
	}

bigtime_t ZUIRenderer_ProgressBar_Appearance::GetInterFrameDuration()
	{
	// Go through all frames twice a second
	return ZTicks::sPerSecond()/(2*4);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Appearance

static const ZCoord kChasingArrowsWidth = 16;
static const ZCoord kChasingArrowsHeight = 16;

ZUIRenderer_ChasingArrows_Appearance::ZUIRenderer_ChasingArrows_Appearance()
	{}

ZUIRenderer_ChasingArrows_Appearance::~ZUIRenderer_ChasingArrows_Appearance()
	{}

void ZUIRenderer_ChasingArrows_Appearance::Activate(ZUIChasingArrows_Rendered* inChasingArrows)
	{ inChasingArrows->Refresh(); }

long ZUIRenderer_ChasingArrows_Appearance::GetFrameCount()
	{
	// Use an arbitrarily large number
	return 0x7FFF;
	}

ZPoint ZUIRenderer_ChasingArrows_Appearance::GetSize()
	{ return ZPoint(kChasingArrowsWidth, kChasingArrowsHeight); }

void ZUIRenderer_ChasingArrows_Appearance::DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame)
	{
	ZDC localDC(inDC);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();
	Rect qdRect = inBounds + qdOffset;
	::DrawThemeChasingArrows(&qdRect, inFrame, inEnabled && inActive ? kThemeStateActive : kThemeStateInactive, nil, 0);
	}

bigtime_t ZUIRenderer_ChasingArrows_Appearance::GetInterFrameDuration()
	{ return ZTicks::sPerSecond() / 10; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Appearance

ZUIRenderer_TabPane_Appearance::ZUIRenderer_TabPane_Appearance()
	{}

ZUIRenderer_TabPane_Appearance::~ZUIRenderer_TabPane_Appearance()
	{}

void ZUIRenderer_TabPane_Appearance::Activate(ZUITabPane_Rendered* inTabPane)
	{ inTabPane->Refresh(); }

ZRect ZUIRenderer_TabPane_Appearance::GetContentArea(ZUITabPane_Rendered* inTabPane)
	{
	return inTabPane->CalcBounds().Inset(3,3);
	}

bool ZUIRenderer_TabPane_Appearance::GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk)
	{
	return false;
	}

void ZUIRenderer_TabPane_Appearance::RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds)
	{
	bool hasLargeTabs;
	if (!inTabPane->GetAttribute("ZUITabPane::LargeTab", nil, &hasLargeTabs))
		hasLargeTabs = true;

	bool enabled = inTabPane->GetReallyEnabled() && inTabPane->GetActive();

	ZDC localDC(inDC);
	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();
	Rect qdRect = inBounds + qdOffset;
	::DrawThemeTabPane(&qdRect, enabled ? kThemeStateActive : kThemeStateInactive);
	}

bool ZUIRenderer_TabPane_Appearance::DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Appearance

ZUIRenderer_Group_Appearance::ZUIRenderer_Group_Appearance(bool isPrimary)
:	fIsPrimary(isPrimary)
	{}

ZUIRenderer_Group_Appearance::~ZUIRenderer_Group_Appearance()
	{}

void ZUIRenderer_Group_Appearance::Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize)
	{
	ZRect theBounds = inGroup->CalcBounds();
	if (inTitleSize.v)
		theBounds.top += inTitleSize.v - 4;
	inGroup->Invalidate(ZDCRgn(theBounds) - theBounds.Inset(2,2));
	}

void ZUIRenderer_Group_Appearance::RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize) 
	{
	ZDC localDC(inDC);
	if (inTitleSize.h > 0)
		{
		ZRect clipRect(inBounds.left + 8, inBounds.top, inBounds.left + 12 + inTitleSize.h, inBounds.top + inTitleSize.v);
		localDC.SetClip(localDC.GetClip() - clipRect);
		}

	ZRect realBounds = inBounds;
	if (inTitleSize.v)
		realBounds.top += inTitleSize.v - 4;

	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect qdRect(realBounds + qdOffset);
	if (fIsPrimary)
		::DrawThemePrimaryGroup(&qdRect, inEnabled && inActive ? kThemeStateActive : kThemeStateInactive);
	else
		::DrawThemeSecondaryGroup(&qdRect, inEnabled && inActive ? kThemeStateActive : kThemeStateInactive);
	}

ZCoord ZUIRenderer_Group_Appearance::GetTitlePaneHLocation()
	{ return 10; }

ZDCInk ZUIRenderer_Group_Appearance::GetBackInk(const ZDC& inDC)
	{ return ZDCInk(); }

void ZUIRenderer_Group_Appearance::GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight)
	{
	// Different themes may have difference values, but these are the maximal values, and there's no API to query anyway.
	outTopLeft = ZPoint(10, max(inTitleSize.v + 10, 12));
	outBottomRight = ZPoint(-10, -10);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrameEditText_Appearance

ZUIRenderer_FocusFrameEditText_Appearance::ZUIRenderer_FocusFrameEditText_Appearance()
	{}

ZUIRenderer_FocusFrameEditText_Appearance::~ZUIRenderer_FocusFrameEditText_Appearance()
	{}

void ZUIRenderer_FocusFrameEditText_Appearance::Activate(ZUIFocusFrame_Rendered* inFocusFrame)
	{
	inFocusFrame->Invalidate(this->GetBorderRgn(inFocusFrame, inFocusFrame->CalcBounds()));
	}

void ZUIRenderer_FocusFrameEditText_Appearance::Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;

	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect qdRect;
	if (showFocusBorder)
		qdRect = inBounds.Inset(3,3) + qdOffset;
	else
		qdRect = inBounds.Inset(2,2) + qdOffset;
	::DrawThemeEditTextFrame(&qdRect, inState.GetEnabled() && inState.GetActive() ? kThemeStateActive : kThemeStateInactive);
	if (showFocusBorder && inState.GetEnabled() && inState.GetActive() && inState.GetFocused())
		::DrawThemeFocusRect(&qdRect, true);
	}

void ZUIRenderer_FocusFrameEditText_Appearance::GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		{
		outTopLeftInset = ZPoint(3, 3);
		outBottomRightInset = ZPoint(-3, -3);
		}
	else
		{
		outTopLeftInset = ZPoint(2, 2);
		outBottomRightInset = ZPoint(-2, -2);
		}
	}

ZDCRgn ZUIRenderer_FocusFrameEditText_Appearance::GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds)
	{
	ZRect outerRect(inBounds);
	ZRect innerRect(inBounds);
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		innerRect.MakeInset(3, 3);
	else
		innerRect.MakeInset(2, 2);
	return ZDCRgn(outerRect) - innerRect;
	}

bool ZUIRenderer_FocusFrameEditText_Appearance::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{
	outInk = ZDCInk::sWhite;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrameListBox_Appearance

ZUIRenderer_FocusFrameListBox_Appearance::ZUIRenderer_FocusFrameListBox_Appearance(ZRef<ZUIInk> inInternalBackInk)
:	fInternalBackInk(inInternalBackInk)
	{}

ZUIRenderer_FocusFrameListBox_Appearance::~ZUIRenderer_FocusFrameListBox_Appearance()
	{}

void ZUIRenderer_FocusFrameListBox_Appearance::Activate(ZUIFocusFrame_Rendered* inFocusFrame)
	{
	inFocusFrame->Invalidate(this->GetBorderRgn(inFocusFrame, inFocusFrame->CalcBounds()));
	}

void ZUIRenderer_FocusFrameListBox_Appearance::Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;

	ZDCSetupForQD theSetupForQD(localDC, true);
	ZPoint qdOffset = theSetupForQD.GetOffset();

	Rect qdRect;
	if (showFocusBorder)
		qdRect = inBounds.Inset(3,3) + qdOffset;
	else
		qdRect = inBounds.Inset(2,2) + qdOffset;
	::DrawThemeListBoxFrame(&qdRect, inState.GetEnabled() && inState.GetActive() ? kThemeStateActive : kThemeStateInactive);
	if (showFocusBorder && inState.GetEnabled() && inState.GetFocused() && inState.GetActive())
		::DrawThemeFocusRect(&qdRect, true);
	}

void ZUIRenderer_FocusFrameListBox_Appearance::GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		{
		outTopLeftInset = ZPoint(3, 3);
		outBottomRightInset = ZPoint(-3, -3);
		}
	else
		{
		outTopLeftInset = ZPoint(2, 2);
		outBottomRightInset = ZPoint(-2, -2);
		}
	}

ZDCRgn ZUIRenderer_FocusFrameListBox_Appearance::GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds)
	{
	ZRect outerRect(inBounds);
	ZRect innerRect(inBounds);
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		innerRect.MakeInset(3, 3);
	else
		innerRect.MakeInset(2, 2);
	return ZDCRgn(outerRect) - innerRect;
	}

bool ZUIRenderer_FocusFrameListBox_Appearance::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{
//##	if (fInternalBackInk)
//##		outInk = fInternalBackInk->GetInk();
//##	else
		outInk = ZDCInk::sWhite;
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Appearance

ZUIRendererFactory_Appearance::ZUIRendererFactory_Appearance()
	{}
ZUIRendererFactory_Appearance::~ZUIRendererFactory_Appearance()
	{}

string ZUIRendererFactory_Appearance::GetName()
	{ return "MacOS Appearance Manager"; }

ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Appearance::Make_Renderer_CaptionPane()
	{ return new ZUIRenderer_CaptionPane_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonPush()
	{ return new ZUIRenderer_ButtonPush_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonRadio()
	{ return new ZUIRenderer_ButtonRadio_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonCheckbox()
	{ return new ZUIRenderer_ButtonCheckbox_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonPopup()
	{ return new ZUIRenderer_ButtonPopup_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonBevel()
	{ return new ZUIRenderer_ButtonBevel_Appearance; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonDisclosure()
	{ return ZRef<ZUIRenderer_Button>(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Appearance::Make_Renderer_ButtonPlacard()
	{ return ZRef<ZUIRenderer_Button>(); }

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Appearance::Make_Renderer_ScrollBar()
	{ return new ZUIRenderer_ScrollBar_Appearance; }

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Appearance::Make_Renderer_Slider()
	{ return ZRef<ZUIRenderer_Slider>(); }

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Appearance::Make_Renderer_LittleArrows()
	{ return new ZUIRenderer_LittleArrows_Appearance; }

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Appearance::Make_Renderer_ProgressBar()
	{ return new ZUIRenderer_ProgressBar_Appearance; }

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Appearance::Make_Renderer_ChasingArrows()
	{ return new ZUIRenderer_ChasingArrows_Appearance; }

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Appearance::Make_Renderer_TabPane()
	{ return new ZUIRenderer_TabPane_Appearance; }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Appearance::Make_Renderer_GroupPrimary()
	{ return new ZUIRenderer_Group_Appearance(true); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Appearance::Make_Renderer_GroupSecondary()
	{ return new ZUIRenderer_Group_Appearance(false); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Appearance::Make_Renderer_FocusFrameEditText()
	{ return new ZUIRenderer_FocusFrameEditText_Appearance; }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Appearance::Make_Renderer_FocusFrameListBox()
	{ return new ZUIRenderer_FocusFrameListBox_Appearance(new ZUIInk_Appearance(kThemeBrushListViewBackground)); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Appearance::Make_Renderer_FocusFrameDateControl()
	{
	//??
	return new ZUIRenderer_FocusFrameEditText_Appearance;
	}

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Appearance::Make_Renderer_Splitter()
	{ return new ZUIRenderer_Splitter_Standard; }

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Appearance::Make_Renderer_Divider()
	{ return new ZUIRenderer_Divider_Appearance; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Appearance

ZUIAttributeFactory_Appearance::ZUIAttributeFactory_Appearance()
	{}

ZUIAttributeFactory_Appearance::~ZUIAttributeFactory_Appearance()
	{}

string ZUIAttributeFactory_Appearance::GetName()
	{ return "MacOS Appearance Manager"; }

ZRef<ZUIColor> ZUIAttributeFactory_Appearance::GetColor(const string& inColorName)
	{
	if (inColorName == "Text")
		return new ZUIColor_Fixed(ZRGBColor::sBlack);
	if (inColorName == "TextHighlight")
		return new ZUIColor_MacHighlight(false);
	return ZUIAttributeFactory::GetColor(inColorName);
	}

ZRef<ZUIInk> ZUIAttributeFactory_Appearance::GetInk(const string& inInkName)
	{
	if (inInkName == "WindowBackground_Document")
		return new ZUIInk_Appearance(kThemeBrushDocumentWindowBackground);
	if (inInkName == "WindowBackground_Dialog")
		return new ZUIInk_Appearance(kThemeBrushDialogBackgroundActive);
	if (inInkName == "Background_Highlight")
		return new ZUIInk_MacHighlight(true);
	if (inInkName == "List_Background")
		return new ZUIInk_Appearance(kThemeBrushListViewBackground);
	if (inInkName == "List_Separator")
		return new ZUIInk_Appearance(kThemeBrushListViewSeparator);

	return ZUIAttributeFactory::GetInk(inInkName);
	}

ZRef<ZUIFont> ZUIAttributeFactory_Appearance::GetFont(const string& inFontName)
	{
	if (inFontName == "SystemLarge")
		return new ZUIFont_Appearance(kThemeSystemFont);
//		return new ZUIFont_Appearance(kThemePushButtonFont);

	if (inFontName == "SystemSmall")
		return new ZUIFont_Appearance(kThemeSmallSystemFont);
	if (inFontName == "SystemSmallEmphasized")
		return new ZUIFont_Appearance(kThemeSmallEmphasizedSystemFont);
	if (inFontName == "Application")
		return new ZUIFont_Appearance(kThemeViewsFont);
	if (inFontName == "Monospaced")
		return new ZUIFont_Fixed(ZDCFont::sMonospaced9);
	return ZUIAttributeFactory::GetFont(inFontName);
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Appearance::GetMetric(const string& inMetricName)
	{
	// We'll just use the inherited versions for now
	return ZUIAttributeFactory::GetMetric(inMetricName);
	}

#endif // ZCONFIG_API_Enabled(Appearance)
// =================================================================================================
// =================================================================================================
