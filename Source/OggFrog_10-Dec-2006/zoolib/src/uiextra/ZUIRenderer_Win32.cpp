static const char rcsid[] = "@(#) $Id: ZUIRenderer_Win32.cpp,v 1.9 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUIRenderer_Win32.h"
#include "ZTextUtil.h"
#include "ZTicks.h"

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Win32

ZUIRenderer_Win32::ZUIRenderer_Win32()
	{}

ZUIRenderer_Win32::~ZUIRenderer_Win32()
	{}

/*
AG 98-06-24. After spending a whole evening examining as many different Windows 95 controls as possible I reworked
the ZUIRenderer_Win32::sDrawEdge utility method. It may seem a little counter intuitive in its operation, but it does do
the right thing. However, it probably does *not* precisely match the Win32 native DrawEdge routine in the parameters
it takes and what their effect is -- so don't use Win32 API docs to figure out this routine.

I'll number the colors as follows:
0 = color_Shadow2
1 = color_Shadow1
2 = color_Hilite1
3 = color_Hilite2

There are four lines to draw, usually. Top left outer and inner, and bottom right outer and inner (TLO, TLI, BRI, BRO).
An up button would look like this:
3210,
and its down counterpart would be reversed
0123.
An etched edge:

1313
and a bump:
3131

It turns out that there are two ways to draw an up button, by setting the borderSoft flag in theStyle the two
top left components are interchanged, so up is 2310 and down is 1023. It "softens" the upper left edge of
the button.

You can also set the borderUpperLeftOnly flag, which will draw only the upper left components of the
top left and bottom right edges (ie TLO and BRI). This is used to get a "flat" border around something.

For more details, see AG's logbook dated 98-06-24.
*/

void ZUIRenderer_Win32::sDrawEdge(const ZDC& inDC, const ZRect& inRect, long inStyle,
		const ZRGBColor& color_Hilite1, const ZRGBColor& color_Hilite2, const ZRGBColor& color_Shadow1, const ZRGBColor& color_Shadow2)
	{
	ZDC localDC(inDC);
	bool raisedInner = ((inStyle & borderRaisedInner) != 0);
	bool raisedOuter = ((inStyle & borderRaisedOuter) != 0);
	bool soft = ((inStyle & borderSoft) != 0);
	bool upperLeftOnly = ((inStyle & borderUpperLeftOnly) != 0);

	ZRGBColor topLeftOuter, topLeftInner, bottomRightOuter, bottomRightInner;
	if (raisedOuter == raisedInner)
		{
		if (raisedOuter)
			{
			topLeftOuter = color_Hilite1;
			topLeftInner = color_Hilite2;
			}
		else
			{
			topLeftOuter = color_Shadow2;
			topLeftInner = color_Shadow1;
			}
		if (soft)
			swap(topLeftOuter, topLeftInner);
	
		if (raisedInner)
			{
			bottomRightOuter = color_Shadow2;
			bottomRightInner = color_Shadow1;
			}
		else
			{
			bottomRightOuter = color_Hilite1;
			bottomRightInner = color_Hilite2;
			}
		}
	else
		{
		if (raisedOuter)
			{
			topLeftOuter = color_Hilite1;
			topLeftInner = color_Shadow1;
			}
		else
			{
			topLeftOuter = color_Shadow1;
			topLeftInner = color_Hilite1;
			}
		if (raisedInner)
			{
			bottomRightOuter = color_Hilite1;
			bottomRightInner = color_Shadow1;
			}
		else
			{
			bottomRightOuter = color_Shadow1;
			bottomRightInner = color_Hilite1;
			}	
		}

// upperLeftOnly uses topLeftOuter and bottomRightInner to paint just the upper left and lower right
// sides of the square, which will give a flat or softly raised/lowered effect
	if (upperLeftOnly)
		{
		localDC.SetInk(topLeftOuter);
		localDC.Line(inRect.left, inRect.bottom - 2, inRect.left, inRect.top);
		localDC.Line(inRect.left, inRect.top, inRect.right - 2, inRect.top);
		localDC.SetInk(bottomRightInner);
		localDC.Line(inRect.left, inRect.bottom - 1, inRect.right - 1, inRect.bottom - 1);
		localDC.Line(inRect.right - 1, inRect.bottom - 1, inRect.right - 1, inRect.top);
		}
	else
		{
		localDC.SetInk(topLeftOuter);
		localDC.Line(inRect.left, inRect.bottom - 2, inRect.left, inRect.top);
		localDC.Line(inRect.left, inRect.top, inRect.right - 2, inRect.top);
		localDC.SetInk(bottomRightOuter);
		localDC.Line(inRect.left, inRect.bottom - 1, inRect.right - 1, inRect.bottom - 1);
		localDC.Line(inRect.right - 1, inRect.bottom - 1, inRect.right - 1, inRect.top);
		localDC.SetInk(topLeftInner);
		localDC.Line(inRect.left + 1, inRect.bottom - 3, inRect.left + 1, inRect.top + 1);
		localDC.Line(inRect.left + 1, inRect.top + 1, inRect.right - 3, inRect.top + 1);
		localDC.SetInk(bottomRightInner);
		localDC.Line(inRect.left + 1, inRect.bottom - 2, inRect.right - 2, inRect.bottom - 2);
		localDC.Line(inRect.right - 2, inRect.bottom - 2, inRect.right - 2, inRect.top + 1);
		}
	}

void ZUIRenderer_Win32::sDrawTriangle(const ZDC& inDC, ZCoord inHCoord, ZCoord inVCoord, ZCoord inBaseLength, bool inHorizontal, bool inDownArrow, const ZRGBColor& inColor)
	{
	ZDC localDC(inDC);
	localDC.SetInk(inColor);
	ZCoord arrowHeight = (inBaseLength + 1) / 2;
	if (inHorizontal)
		{
		if (inDownArrow)
			{
			for (long x = 0; x < arrowHeight; ++x)
				localDC .Line(inHCoord + x, inVCoord + x, inHCoord + x, inVCoord + inBaseLength - x - 1);
			}
		else
			{
			for (long x = 0; x < arrowHeight; ++x)
				localDC.Line(inHCoord+(arrowHeight - x - 1), inVCoord + x, inHCoord+(arrowHeight - x - 1), inVCoord + inBaseLength - x - 1);
			}
		}
	else
		{
		if (inDownArrow)
			{
			for (long x = 0; x < arrowHeight; ++x)
				localDC.Line(inHCoord + x, inVCoord + x, inHCoord + inBaseLength - x - 1, inVCoord + x);
			}
		else
			{
			for (long x = 0; x < arrowHeight; ++x)
				localDC.Line(inHCoord + x, inVCoord+(arrowHeight - x - 1), inHCoord + inBaseLength - x - 1, inVCoord+(arrowHeight - x - 1));
			}
		}
	}

// This constant is used in pixel lookup tables to indicate a non-drawn pixel.
enum
	{
// Index for no color which means draw nothing
	N = 0xFFU
	};


static bool sMungeProc_ColorizeBlack(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
// Anything that is black gets turned into what was passed in inRefCon. Otherwise it is left alone.
	if (inOutColor == ZRGBColor::sBlack)
		{
		inOutColor = *static_cast<ZRGBColorPOD*>(inRefCon);
		return true;
		}
	return false;
	}

static bool sMungeProc_ColorizeNonBlack(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
// Anything that is *not* black gets turned into what was passed in inRefCon. Otherwise it is left alone.
	if (inOutColor != ZRGBColor::sBlack)
		{
		inOutColor = *static_cast<ZRGBColorPOD*>(inRefCon);
		return true;
		}
	return false;
	}


static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

static bool sMungeProc_Tint(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = inOutColor / 2 + *static_cast<ZRGBColor*>(inRefCon);
	return true;
	}

static bool sMungeProc_Lighten(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - ((ZRGBColor::sWhite - inOutColor) / 2);
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_Win32

ZUICaptionRenderer_Win32::ZUICaptionRenderer_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1)
:	fColor_Hilite1(color_Hilite1), fColor_Shadow1(color_Shadow1)
	{}

ZUICaptionRenderer_Win32::~ZUICaptionRenderer_Win32()
	{}

void ZUICaptionRenderer_Win32::Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{
	ZCoord wrapWidth(inWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	if (!inState.GetEnabled())
		{
		localDC.SetFont(inFont);
		localDC.SetTextColor(fColor_Hilite1->GetColor());
		ZTextUtil_sDraw(localDC, ZPoint(inLocation.h + 1, inLocation.v + 1), wrapWidth, inText, inTextLength);
		localDC.SetTextColor(fColor_Shadow1->GetColor());
		ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
		}
	else
		{
		localDC.SetFont(inFont);
		localDC.SetTextColor(ZRGBColor::sBlack);
		ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
		}
	}

void ZUICaptionRenderer_Win32::Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{
	if (inState.GetEnabled())
		{
// Prefer theColorPixmap
		ZDCPixmap theSourcePixmap = inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono();
		inDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	else
		{
		ZDCPixmap disabledMask = inPixmapCombo.GetMono();
		if (!disabledMask)
			{
			disabledMask = inPixmapCombo.GetColor();
			disabledMask.Munge(sMungeProc_ColorizeNonBlack, const_cast<ZRGBColorPOD*>(&ZRGBColor::sWhite));
			}
// Invert disabledMask
		disabledMask.Munge(sMungeProc_Invert, nil);

// Prefer theMonoPixmap
		ZDCPixmap theSourcePixmap = inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor();
		ZDCPixmap hilitePixmap = theSourcePixmap;
		ZDCPixmap shadowPixmap = theSourcePixmap;
		ZRGBColor theColor = fColor_Hilite1->GetColor();
		hilitePixmap.Munge(sMungeProc_ColorizeBlack, &theColor);
		theColor = fColor_Shadow1->GetColor();
		shadowPixmap.Munge(sMungeProc_ColorizeBlack, &theColor);
		inDC.Draw(inLocation + ZPoint(1,1), hilitePixmap, disabledMask);
		inDC.Draw(inLocation, shadowPixmap, disabledMask);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Win32

ZUIRenderer_CaptionPane_Win32::ZUIRenderer_CaptionPane_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1)
	{}

ZUIRenderer_CaptionPane_Win32::~ZUIRenderer_CaptionPane_Win32()
	{}

void ZUIRenderer_CaptionPane_Win32::Activate(ZUICaptionPane_Rendered* inCaptionPane)
	{}

void ZUIRenderer_CaptionPane_Win32::Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (inCaption)
		inCaption->Draw(localDC, inBounds.TopLeft(), inState, this);
	}

ZPoint ZUIRenderer_CaptionPane_Win32::GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption)
	{
	if (inCaption)
		return inCaption->GetSize();
	return ZPoint::sZero;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Button_Win32_Targettable

ZUIRenderer_Button_Win32_Targettable::ZUIRenderer_Button_Win32_Targettable()
	{}

ZUIRenderer_Button_Win32_Targettable::~ZUIRenderer_Button_Win32_Targettable()
	{}

void ZUIRenderer_Button_Win32_Targettable::Activate(ZUIButton_Rendered* inButton)
	{}

bool ZUIRenderer_Button_Win32_Targettable::ButtonWantsToBecomeTarget(ZUIButton_Rendered* inButton)
	{ return true; }

void ZUIRenderer_Button_Win32_Targettable::ButtonBecameWindowTarget(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_Button_Win32_Targettable::ButtonResignedWindowTarget(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

bool ZUIRenderer_Button_Win32_Targettable::ButtonDoKeyDown(ZUIButton_Rendered* inButton, const ZEvent_Key& inEvent)
	{
	switch (inEvent.GetCharCode())
		{
		case ZKeyboard::ccReturn:
		case ZKeyboard::ccEnter:
		case ZKeyboard::ccSpace:
			{
			inButton->Flash();
			return true;
			}
		}
	return ZUIRenderer_Button::ButtonDoKeyDown(inButton, inEvent);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Win32

ZUIRenderer_ButtonPush_Win32::ZUIRenderer_ButtonPush_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1), fColor_Hilite2(color_Hilite2), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face)
	{}

ZUIRenderer_ButtonPush_Win32::~ZUIRenderer_ButtonPush_Win32()
	{}

void ZUIRenderer_ButtonPush_Win32::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (inState.GetHilite() == eZTriState_Off)
		{
		localDC.SetInk(fColor_Face->GetColor());
		localDC.Fill(inBounds);
		sDrawEdge(localDC, inBounds, inState.GetPressed() ? edgeSunk : edgeRaised,
				fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
		}
	else
		{
		ZRGBColor theHiliteColor = fColor_Hilite1->GetColor();
// AG 99-05-07. This detail pulled from wine/controls/uitools.c line 629
		if (theHiliteColor == ZRGBColor::sWhite)
			localDC.SetInk(ZDCInk(theHiliteColor, fColor_Face->GetColor(), ZDCPattern::sGray));
		else
			localDC.SetInk(theHiliteColor);
		localDC.Fill(inBounds);
		sDrawEdge(localDC, inBounds, edgeSunk,
				fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
		}


	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.Centered(inBounds);
		if (inState.GetPressed() || (inState.GetHilite() != eZTriState_Off))
			captionBounds += ZPoint(1,1);
		inCaption->Draw(localDC, captionBounds.TopLeft(), inState, this);
		}
	if (inState.GetFocused())
		{
		localDC.SetInk(ZDCInk::sGray);
		localDC.SetMode(ZDC::modeXor);
		ZRect focusRect(inBounds.Inset(4,4));
		localDC.Frame(focusRect);
		}
	}

ZPoint ZUIRenderer_ButtonPush_Win32::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(16,7);
	if (inCaption && inCaption->IsValid())
		theSize += inCaption->GetSize();
	return theSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Win32

enum
	{
	kCheckbox_Width = 13,
	kCheckbox_Height = 13,
	kCheckbox_TextOffset = 5
	};

ZUIRenderer_ButtonCheckbox_Win32::ZUIRenderer_ButtonCheckbox_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1),
	fColor_Hilite2(color_Hilite2),
	fColor_Shadow2(color_Shadow2),
	fColor_Face(color_Face),
	fColor_Window(color_Window)
	{}

ZUIRenderer_ButtonCheckbox_Win32::~ZUIRenderer_ButtonCheckbox_Win32()
	{}

void ZUIRenderer_ButtonCheckbox_Win32::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZPoint checkboxLocation = inBounds.TopLeft();
	checkboxLocation.v += (inBounds.Height() - kCheckbox_Height)/2;
	ZRGBColorMap colorMap[6];
	colorMap[0].fPixval = 'S'; colorMap[0].fColor = fColor_Shadow2->GetColor();
	colorMap[1].fPixval = 's'; colorMap[1].fColor = fColor_Shadow1->GetColor();
	colorMap[2].fPixval = 'H'; colorMap[2].fColor = fColor_Hilite2->GetColor();
	colorMap[3].fPixval = 'h'; colorMap[3].fColor = fColor_Hilite1->GetColor();
	colorMap[4].fPixval = 'f'; colorMap[4].fColor = fColor_Face->GetColor();
	colorMap[5].fPixval = 'w'; colorMap[5].fColor = fColor_Window->GetColor();
	sRenderCheckbox(inDC, checkboxLocation, inState, colorMap, 6);
	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		ZPoint location= inBounds.TopLeft();
		location.h += kCheckbox_Width + kCheckbox_TextOffset;
		location.v += (inBounds.Height() - captionSize.v)/2;
		inCaption->Draw(inDC, location, inState, this);
		if (inState.GetFocused())
			{
			ZDC localDC(inDC);
			localDC.SetInk(ZDCInk::sGray);
			localDC.SetMode(ZDC::modeXor);
			ZRect focusRect(captionSize);
			focusRect += location;
			localDC.Frame(focusRect.Inset(-2,0));
			}
		}
	}

ZPoint ZUIRenderer_ButtonCheckbox_Win32::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(kCheckbox_Width, kCheckbox_Height);
	if (inCaption && inCaption->IsValid())
		{
		theSize = inCaption->GetSize();
// Account for focus rect by boosting width by 2
		theSize.h += kCheckbox_Width + kCheckbox_TextOffset + 2;
// Account for focus rect by boosting height by 0
		theSize.v = max(theSize.v, ZCoord(kCheckbox_Height));
		}
	return theSize;
	}

enum
	{
	kCheckbox_NormalOff = 0,
	kCheckbox_NormalOn,
	kCheckbox_NormalMixed,
	kCheckbox_PushedOff,
	kCheckbox_PushedOn,
	kCheckbox_PushedMixed,
	kCheckbox_DimmedOff,
	kCheckbox_DimmedOn,
	kCheckbox_DimmedMixed
	};

static const char* sPixvals_Checkbox[] = 
	{
// kCheckbox_NormalOff
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_NormalOn
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwSwHh"
	"sSwwwwwwSSwHh"
	"sSwSwwwSSSwHh"
	"sSwSSwSSSwwHh"
	"sSwSSSSSwwwHh"
	"sSwwSSSwwwwHh"
	"sSwwwSwwwwwHh"
	"sSwwwwwwwwwHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_NormalMixed
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwSSSSSSSwHh"
	"sSwSSSSSSSwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sSwwwwwwwwwHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_PushedOff
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_PushedOn
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSfffffffffHh"
	"sSfffffffSfHh"
	"sSffffffSSfHh"
	"sSfSfffSSSfHh"
	"sSfSSfSSSffHh"
	"sSfSSSSSfffHh"
	"sSffSSSffffHh"
	"sSfffSfffffHh"
	"sSfffffffffHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_PushedMixed
	"ssssssssssssh"
	"sSSSSSSSSSSHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfSSSSSSSfHh"
	"sSfSSSSSSSfHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sSfffffffffHh"
	"sHHHHHHHHHHHh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_DimmedOff
	"ssssssssssssh"
	"sSSSSSSSSSSfh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sfffffffffffh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_DimmedOn
	"ssssssssssssh"
	"sSSSSSSSSSSfh"
	"sSffffffffffh"
	"sSfffffffsffh"
	"sSffffffssffh"
	"sSfsfffsssffh"
	"sSfssfsssfffh"
	"sSfsssssffffh"
	"sSffsssfffffh"
	"sSfffsffffffh"
	"sSffffffffffh"
	"sfffffffffffh"
	"hhhhhhhhhhhhh"
	,
// kCheckbox_DimmedMixed
	"ssssssssssssh"
	"sSSSSSSSSSSfh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSfsssssssffh"
	"sSfsssssssffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sSffffffffffh"
	"sfffffffffffh"
	"hhhhhhhhhhhhh"
	};

void ZUIRenderer_ButtonCheckbox_Win32::sRenderCheckbox(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize)
	{
	ZDC localDC(inDC);

	short whichOne = 0;

	if (inState.GetHilite() == eZTriState_On)
		whichOne = kCheckbox_NormalOn;
	else if (inState.GetHilite() == eZTriState_Off)
		whichOne = kCheckbox_NormalOff;
	else
		whichOne = kCheckbox_NormalMixed;

	if (!inState.GetEnabled())
		whichOne += kCheckbox_DimmedOff - kCheckbox_NormalOff;
	else if (inState.GetPressed())
		whichOne += kCheckbox_PushedOff - kCheckbox_NormalOff;

	localDC.Pixels(inLocation, ZPoint(kCheckbox_Width, kCheckbox_Height), sPixvals_Checkbox[whichOne], inColorMap, inColorMapSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Win32

enum
	{
	kRadio_Width = 12,
	kRadio_Height = 12,
	kRadio_TextOffset = 12
	};

ZUIRenderer_ButtonRadio_Win32::ZUIRenderer_ButtonRadio_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1), fColor_Hilite2(color_Hilite2), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face), fColor_Window(color_Window)
	{}

ZUIRenderer_ButtonRadio_Win32::~ZUIRenderer_ButtonRadio_Win32()
	{}

void ZUIRenderer_ButtonRadio_Win32::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZPoint radioLocation = inBounds.TopLeft();
	radioLocation.v += (inBounds.Height() - kRadio_Height)/2;
	ZRGBColorMap colorMap[6];
	colorMap[0].fPixval = 'S'; colorMap[0].fColor = fColor_Shadow2->GetColor();
	colorMap[1].fPixval = 's'; colorMap[1].fColor = fColor_Shadow1->GetColor();
	colorMap[2].fPixval = 'H'; colorMap[2].fColor = fColor_Hilite2->GetColor();
	colorMap[3].fPixval = 'h'; colorMap[3].fColor = fColor_Hilite1->GetColor();
	colorMap[4].fPixval = 'f'; colorMap[4].fColor = fColor_Face->GetColor();
	colorMap[5].fPixval = 'w'; colorMap[5].fColor = fColor_Window->GetColor();
	sRenderRadio(inDC, radioLocation, inState, colorMap, 6);
	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		ZPoint location = inBounds.TopLeft();
		location.h += kRadio_Width + kRadio_TextOffset;
		location.v += (inBounds.Height() - captionSize.v)/2;
		inCaption->Draw(inDC, location, inState, this);
		if (inState.GetFocused())
			{
			ZDC localDC(inDC);
			localDC.SetInk(ZDCInk::sGray);
			localDC.SetMode(ZDC::modeXor);
			ZRect focusRect(captionSize);
			focusRect += location;
			localDC.Frame(focusRect.Inset(-2,0));
			}
		}
	}

ZPoint ZUIRenderer_ButtonRadio_Win32::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(kRadio_Width, kRadio_Height);
	if (inCaption && inCaption->IsValid())
		{
		theSize = inCaption->GetSize();
// Account for focus rect by boosting width by 2
		theSize.h += kRadio_Width + kRadio_TextOffset + 2;
// Account for focus rect by boosting height by 0
		theSize.v = max(theSize.v, ZCoord(kRadio_Height));
		}
	return theSize;
	}

enum
	{
	kRadio_NormalOff = 0,
	kRadio_NormalOn,
	kRadio_NormalMixed,
	kRadio_PushedOff,
	kRadio_PushedOn,
	kRadio_PushedMixed,
	kRadio_DimmedOff,
	kRadio_DimmedOn,
	kRadio_DimmedMixed
	};

static const char* sPixvals_Radio[] = 
	{
// kRadio_NormalOff
	"    ssss    "
	"  ssSSSSss  "
	" sSSwwwwSSh "
	" sSwwwwwwHh "
	"sSwwwwwwwwHh"
	"sSwwwwwwwwHh"
	"sSwwwwwwwwHh"
	"sSwwwwwwwwHh"
	" sSwwwwwwHh "
	" sHHwwwwHHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// -kRadio_NormalOn
	"    ssss    "
	"  ssSSSSss  "
	" sSSwwwwSSh "
	" sSwwwwwwHh "
	"sSwwwSSwwwHh"
	"sSwwSSSSwwHh"
	"sSwwSSSSwwHh"
	"sSwwwSSwwwHh"
	" sSwwwwwwHh "
	" sHHwwwwHHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// kRadio_NormalMixed
	"    ssss    "
	"  ssSSSSss  "
	" sSSwwwwSSh "
	" sSwwwwwwHh "
	"sSwwwwwwwwHh"
	"sSwSSSSSSwHh"
	"sSwSSSSSSwHh"
	"sSwwwwwwwwHh"
	" sSwwwwwwHh "
	" sHHwwwwHHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// kRadio_PushedOff
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSfffffffh "
	"sSffffffHfHh"
	"sSffffffffHh"
	"sSffffffffHh"
	"sSffffffffHh"
	" sSffffffHh "
	" sfffffffHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// kRadio_PushedOn
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSffffffHh "
	"sSfffSSfffHh"
	"sSffSSSSffHh"
	"sSffSSSSffHh"
	"sSfffSShffHh"
	" sSffffffHh "
	" sfffffffHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// kRadio_PushedMixed
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSffffffHh "
	"sSffffffffHh"
	"sSfSSSSSSfHh"
	"sSfSSSSSSfHh"
	"sSffffffffHh"
	" sSffffffHh "
	" sfffffffHh "
	"  hhHHHHhh  "
	"    hhhh    "
	,
// kRadio_DimmedOff
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSfffffffh "
	"sSfffffffffh"
	"sSfffffffffh"
	"sSfffffffffh"
	"sSfffffffffh"
	" sSfffffffh "
	" sffffffffh "
	"  hhffffhh  "
	"    hhhh    "
	,
// kRadio_DimmedOn
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSfffffffh "
	"sSfffssffffh"
	"sSffssssfffh"
	"sSffssssfffh"
	"sSfffssffffh"
	" sSfffffffh "
	" sfffffffff "
	"  hhffffhh  "
	"    hhhh    "
	,
// kRadio_DimmedMixed
	"    ssss    "
	"  ssSSSSss  "
	" sSSffffSSh "
	" sSfffffffh "
	"sSfffffffffh"
	"sSfssssssffh"
	"sSfssssssffh"
	"sSfffffffffh"
	" sSfffffffh "
	" sfffffffff "
	"  hhffffhh  "
	"    hhhh    "
	};

void ZUIRenderer_ButtonRadio_Win32::sRenderRadio(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize)
	{
	ZDC localDC(inDC);

	short whichOne = 0;

	if (inState.GetHilite() == eZTriState_On)
		whichOne = kRadio_NormalOn;
	else if (inState.GetHilite() == eZTriState_Off)
		whichOne = kRadio_NormalOff;
	else
		whichOne = kRadio_NormalMixed;

	if (!inState.GetEnabled() && inState.GetActive())
		whichOne += kRadio_DimmedOff - kRadio_NormalOff;
	else if (inState.GetPressed())
		whichOne += kRadio_PushedOff - kRadio_NormalOff;

	localDC.Pixels(inLocation, ZPoint(kRadio_Width, kRadio_Height), sPixvals_Radio[whichOne], inColorMap, inColorMapSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Win32

ZUIRenderer_ButtonPopup_Win32::ZUIRenderer_ButtonPopup_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
												const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, const ZRef<ZUIColor>& color_Window, ZRef<ZUIMetric> metric_ArrowSize)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1), fColor_Hilite2(color_Hilite2), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face), fColor_Arrow(color_Arrow), fColor_Window(color_Window), fMetric_ArrowSize(metric_ArrowSize)
	{}

ZUIRenderer_ButtonPopup_Win32::~ZUIRenderer_ButtonPopup_Win32()
	{}

void ZUIRenderer_ButtonPopup_Win32::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (inCaption && inCaption->IsValid())
		{
		ZPoint captionSize = inCaption->GetSize();
		localDC.SetInk(inState.GetEnabled() ? fColor_Window->GetColor() : fColor_Face->GetColor());
		localDC.Fill(inBounds.Inset(2,2));
		sDrawEdge(localDC, inBounds, edgeSunkSoft, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());

		ZRect arrowRect;
		arrowRect.right = inBounds.right - 2;
		arrowRect.left = arrowRect.right - fMetric_ArrowSize->GetMetric();
		arrowRect.top = inBounds.top + 2;
		arrowRect.bottom = inBounds.bottom - 2;
		ZUIRenderer_ScrollBar_Win32::sRenderArrow(localDC, arrowRect, false, true, inState.GetPressed(), inState.GetEnabled(),
					fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(), fColor_Face->GetColor(), fColor_Arrow->GetColor());

		ZRect captionRect = inBounds;
		captionRect.left += 6;
		captionRect.top += 3;
		captionRect.right -= fMetric_ArrowSize->GetMetric() + 2;
		captionRect.bottom -= 3;

		ZDCRgn oldClip(localDC.GetClip());
		localDC.SetClip(oldClip & captionRect);
		inCaption->Draw(localDC, ZPoint(captionRect.left, captionRect.top), inState, this);

		if (inState.GetEnabled() && inState.GetFocused())
			{
			localDC.SetInk(ZDCInk::sGray);
			localDC.SetMode(ZDC::modeXor);
			ZRect focusRect = inBounds;
			focusRect.left += 2 + 1;
			focusRect.top += 2 + 1;
			focusRect.right = arrowRect.left - 1;
			focusRect.bottom -= 2 + 1;
			localDC.Frame(focusRect);
			}
		}
	else
		{
		ZUIRenderer_ScrollBar_Win32::sRenderArrow(localDC, inBounds, false, true, inState.GetPressed(), inState.GetEnabled(),
					fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(), fColor_Face->GetColor(), fColor_Arrow->GetColor());
		}
	}

ZPoint ZUIRenderer_ButtonPopup_Win32::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(fMetric_ArrowSize->GetMetric(), fMetric_ArrowSize->GetMetric());
	if (inCaption && inCaption->IsValid())
		{
		ZPoint captionSize = inCaption->GetSize();
// 2 border, 1 gap, 1 focus rect, captionSize.v, 1 focus rect, 1 gap, 2 border
		theSize.v = 2 + 1 + captionSize.v + 1 + 1 + 2;
// 2 border, 1 gap, 1 focus rect, 2 gap, captionSize.h, 2 gap, 1 focus rect, 1 gap, fMetric_ArrowSize, 2 border
		theSize.h = 2 + 1 + 1 + 2 + captionSize.h + 2 + 1 + 1 + fMetric_ArrowSize->GetMetric() + 2;
		}
	return theSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonDisclosure_Win32

ZUIRenderer_ButtonDisclosure_Win32::ZUIRenderer_ButtonDisclosure_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1), fColor_Hilite2(color_Hilite2), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face), fColor_Window(color_Window)
	{}

ZUIRenderer_ButtonDisclosure_Win32::~ZUIRenderer_ButtonDisclosure_Win32()
	{}

enum
	{
	kDisclosure_Width = 9,
	kDisclosure_RowBytes = 12,
	kDisclosure_Height = 9
	};

void ZUIRenderer_ButtonDisclosure_Win32::Activate(ZUIButton_Rendered* inButton)
	{}

void ZUIRenderer_ButtonDisclosure_Win32::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZPoint dislosureLocation = inBounds.TopLeft();
	dislosureLocation.v += (inBounds.Height() - kDisclosure_Height) / 2;
	ZRGBColorMap colorMap[6];
	colorMap[0].fPixval = 'S'; colorMap[0].fColor = fColor_Shadow2->GetColor();
	colorMap[1].fPixval = 's'; colorMap[1].fColor = fColor_Shadow1->GetColor();
	colorMap[2].fPixval = 'H'; colorMap[2].fColor = fColor_Hilite2->GetColor();
	colorMap[3].fPixval = 'h'; colorMap[3].fColor = fColor_Hilite1->GetColor();
	colorMap[4].fPixval = 'f'; colorMap[4].fColor = fColor_Face->GetColor();
	colorMap[5].fPixval = 'w'; colorMap[5].fColor = fColor_Window->GetColor();
	sRenderDisclosure(inDC, dislosureLocation, inState, colorMap, 6);
	}

ZPoint ZUIRenderer_ButtonDisclosure_Win32::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return ZPoint(kDisclosure_Width, kDisclosure_Height); }

enum
	{
	kDisclosure_NormalOff = 0,
	kDisclosure_NormalOn,
	kDisclosure_PushedOff,
	kDisclosure_PushedOn,
	kDisclosure_DimmedOff,
	kDisclosure_DimmedOn
	};

static const char* sPixvals_Disclosure[] = 
	{
// kDisclosure_NormalOff
	"sssssssss   "
	"swwwwwwws   "
	"swwwSwwws   "
	"swwwSwwws   "
	"swSSSSSws   "
	"swwwSwwws   "
	"swwwSwwws   "
	"swwwwwwws   "
	"sssssssss   "
	,
// kDisclosure_NormalOn
	"sssssssss   "
	"swwwwwwws   "
	"swwwwwwws   "
	"swwwwwwws   "
	"swSSSSSws   "
	"swwwwwwws   "
	"swwwwwwws   "
	"swwwwwwws   "
	"sssssssss   "
	,
// kDisclosure_PushedOff
	"sssssssss   "
	"sfffffffs   "
	"sfffSfffs   "
	"sfffSfffs   "
	"sfSSSSSfs   "
	"sfffSfffs   "
	"sfffSfffs   "
	"sfffffffs   "
	"sssssssss   "
	,
// kDisclosure_PushedOn
	"sssssssss   "
	"sfffffffs   "
	"sfffffffs   "
	"sfffffffs   "
	"sfSSSSSfs   "
	"sfffffffs   "
	"sfffffffs   "
	"sfffffffs   "
	"sssssssss   "
	,
// kDisclosure_DimmedOff -- same as pushed (for now)
	"sssssssss   "
	"sfffffffs   "
	"sfffSfffs   "
	"sfffSfffs   "
	"sfSSSSSfs   "
	"sfffSfffs   "
	"sfffSfffs   "
	"sfffffffs   "
	"sssssssss   "
	,
// kDisclosure_DimmedOn -- same as pushed (for now)
	"sssssssss   "
	"sfffffffs   "
	"sfffffffs   "
	"sfffffffs   "
	"sfSSSSSfs   "
	"sfffffffs   "
	"sfffffffs   "
	"sfffffffs   "
	"sssssssss   "
	};

void ZUIRenderer_ButtonDisclosure_Win32::sRenderDisclosure(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState, const ZRGBColorMap* inColorMap, size_t inColorMapSize)
	{
	ZDC localDC(inDC);

	short whichOne = 0;

	if (inState.GetHilite() == eZTriState_Off)
		whichOne = kDisclosure_NormalOff;
	else
		whichOne = kDisclosure_NormalOn;

	if (!inState.GetEnabled())
		whichOne += kDisclosure_DimmedOff - kDisclosure_NormalOff;
	else if (inState.GetPressed())
		whichOne += kDisclosure_PushedOff - kDisclosure_NormalOff;

	localDC.Pixels(inLocation, ZPoint(kDisclosure_RowBytes, kDisclosure_Height), sPixvals_Disclosure[whichOne], inColorMap, inColorMapSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Win32

ZUIRenderer_ScrollBar_Win32::ZUIRenderer_ScrollBar_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
													const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, const ZRef<ZUIColor>& color_Track,
													ZRef<ZUIMetric> metric_ArrowSize, ZRef<ZUIMetric> metric_MinThumbLength)
:	fColor_Hilite1(color_Hilite1), fColor_Hilite2(color_Hilite2), fColor_Shadow1(color_Shadow1), fColor_Shadow2(color_Shadow2),
	fColor_Face(color_Face), fColor_Arrow(color_Arrow), fColor_Track(color_Track),
	fMetric_ArrowSize(metric_ArrowSize), fMetric_MinThumbLength(metric_MinThumbLength)
	{}

void ZUIRenderer_ScrollBar_Win32::Activate(ZUIScrollBar_Rendered* inScrollBar)
	{}

void ZUIRenderer_ScrollBar_Win32::RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inBorder, double inValue, double inThumbProportion, double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart)
	{
	ZDC localDC(inDC);
	bool horizontal = inBounds.Width() > inBounds.Height();
	double realValue = inValue;
	if (inGhostThumbValue >= 0.0 && inGhostThumbValue <= 1.0)
		realValue = inGhostThumbValue;
	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	sCalcAllRects(inBounds, horizontal, realValue, inThumbProportion, fMetric_MinThumbLength->GetMetric(), fMetric_ArrowSize->GetMetric(), thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect);
	bool slideable = inThumbProportion < 1.0;
	if (inEnabled && slideable)
		{
		sRenderArrow(localDC, upArrowRect, horizontal, false, inHitPart == ZUIScrollBar::hitPartUpArrowOuter, true,
									fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(),
									fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(),
									fColor_Face->GetColor(), fColor_Arrow->GetColor());
		sRenderArrow(localDC, downArrowRect, horizontal, true, inHitPart == ZUIScrollBar::hitPartDownArrowOuter, true, 
									fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(),
									fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(),
									fColor_Face->GetColor(), fColor_Arrow->GetColor());
		sRenderTrack(localDC, pageUpRect, inHitPart == ZUIScrollBar::hitPartPageUp, fColor_Track->GetColor(), fColor_Hilite2->GetColor());
		sRenderTrack(localDC, pageDownRect, inHitPart == ZUIScrollBar::hitPartPageDown, fColor_Track->GetColor(), fColor_Hilite2->GetColor());
		sRenderThumb(localDC, thumbRect, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(),
									fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(),
									fColor_Face->GetColor());
		}
	else
		{
		sRenderArrow(localDC, upArrowRect, horizontal, false, inHitPart == ZUIScrollBar::hitPartUpArrowOuter, false, 
									fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(),
									fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(),
									fColor_Face->GetColor(), fColor_Arrow->GetColor());
		sRenderArrow(localDC, downArrowRect, horizontal, true, inHitPart == ZUIScrollBar::hitPartDownArrowOuter, false, 
									fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(),
									fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(),
									fColor_Face->GetColor(), fColor_Arrow->GetColor());
		sRenderTrack(localDC, trackRect, false, fColor_Track->GetColor(), fColor_Hilite2->GetColor());
		}
	}

ZUIScrollBar::HitPart ZUIRenderer_ScrollBar_Win32::FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion)
	{
	if (inThumbProportion >= 1.0)
		return ZUIScrollBar::hitPartNone;

	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	sCalcAllRects(inBounds, horizontal, inValue, inThumbProportion, fMetric_MinThumbLength->GetMetric(), fMetric_ArrowSize->GetMetric(), thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect);
	if (upArrowRect.Contains(inHitPoint))
		return ZUIScrollBar::hitPartUpArrowOuter;
	else if (downArrowRect.Contains(inHitPoint))
		return ZUIScrollBar::hitPartDownArrowOuter;
	else if (pageUpRect.Contains(inHitPoint))
		return ZUIScrollBar::hitPartPageUp;
	else if (pageDownRect.Contains(inHitPoint))
		return ZUIScrollBar::hitPartPageDown;
	else if (thumbRect.Contains(inHitPoint))
		return ZUIScrollBar::hitPartThumb;
	return ZUIScrollBar::hitPartNone;
	}

double ZUIRenderer_ScrollBar_Win32::GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta)
	{
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect thumbRect, trackRect, dummyRect;
	sCalcAllRects(inBounds, horizontal, inValue, inThumbProportion, fMetric_MinThumbLength->GetMetric(), fMetric_ArrowSize->GetMetric(), thumbRect, trackRect, dummyRect, dummyRect, dummyRect, dummyRect);
	double thumbLength;
	double trackLength;
	double pixelDelta;
	if (horizontal)
		{
		thumbLength = thumbRect.Width();
		trackLength = trackRect.Width();
		pixelDelta = inPixelDelta.h;
		}
	else
		{
		thumbLength = thumbRect.Height();
		trackLength = trackRect.Height();
		pixelDelta = inPixelDelta.v;
		}
	return pixelDelta / (trackLength - thumbLength);
	}

ZCoord ZUIRenderer_ScrollBar_Win32::GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar)
	{ return fMetric_ArrowSize->GetMetric(); }

void ZUIRenderer_ScrollBar_Win32::sRenderTrack(ZDC& inDC, const ZRect& inTrackRect, bool inHilited, const ZRGBColor& inColor_Track, const ZRGBColor& inColor_Hilite)
	{
	if (inHilited)
		{
		if (inColor_Track == ZRGBColor::sWhite)
			inDC.SetInk(ZDCInk(ZRGBColor::sWhite - inColor_Track, ZRGBColor::sWhite - inColor_Hilite, ZDCPattern::sGray));
		else
			inDC.SetInk(ZRGBColor::sWhite - inColor_Track);
		}
	else
		{
		if (inColor_Track == ZRGBColor::sWhite)
			inDC.SetInk(ZDCInk(inColor_Track, inColor_Hilite, ZDCPattern::sGray));
		else
			inDC.SetInk(inColor_Track);
		}
	inDC.Fill(inTrackRect);
	}

void ZUIRenderer_ScrollBar_Win32::sRenderThumb(ZDC& inDC, const ZRect& inThumbRect,
					const ZRGBColor& inColor_Hilite1, const ZRGBColor& inColor_Hilite2,
					const ZRGBColor& inColor_Shadow1, const ZRGBColor& inColor_Shadow2,
					const ZRGBColor& inColor_Face)
	{
	inDC.SetInk(inColor_Face);
	inDC.Fill(inThumbRect.Inset(2,2));
	sDrawEdge(inDC, inThumbRect, edgeRaisedSoft, inColor_Hilite1, inColor_Hilite2, inColor_Shadow1, inColor_Shadow2);
	}

void ZUIRenderer_ScrollBar_Win32::sRenderArrow(ZDC& inDC, const ZRect& inArrowRect, bool inHorizontal, bool inDownArrow, bool inPressed, bool inEnabled,
												const ZRGBColor& inColor_Hilite1, const ZRGBColor& inColor_Hilite2,
												const ZRGBColor& inColor_Shadow1, const ZRGBColor& inColor_Shadow2,
												const ZRGBColor& inColor_Face, const ZRGBColor& inColor_Arrow)
	{
// Figure out the coords of the triangle
	ZCoord minimalBoundsDimension = min(inArrowRect.Width(), inArrowRect.Height());
	ZCoord triangleHeight;
	if (minimalBoundsDimension < 16)
		triangleHeight = minimalBoundsDimension / 4;
	else
		triangleHeight = ((minimalBoundsDimension - 1) / 3) - 1;
	ZCoord triangleBaseLength = triangleHeight * 2 - 1;
	ZCoord hOrigin;
	ZCoord vOrigin;
	if (inHorizontal)
		{
		hOrigin = inArrowRect.left+(inArrowRect.Width() - triangleHeight)/2;
		vOrigin = inArrowRect.top+(inArrowRect.Height() - triangleBaseLength)/2;
		}
	else
		{
		hOrigin = inArrowRect.left+(inArrowRect.Width() - triangleBaseLength)/2;
		vOrigin = inArrowRect.top+(inArrowRect.Height() - triangleHeight)/2;
		}

	if (inEnabled)
		{
		sDrawEdge(inDC, inArrowRect, inPressed ? edgeFlat : edgeRaisedSoft, inColor_Hilite1, inColor_Hilite2, inColor_Shadow1, inColor_Shadow2);
		inDC.SetInk(inColor_Face);
		if (inPressed)
			inDC.Fill(inArrowRect.Inset(1,1));
		else
			inDC.Fill(inArrowRect.Inset(2,2));
		if (inPressed)
			{
			hOrigin += 1;
			vOrigin += 1;
			}	
		sDrawTriangle(inDC, hOrigin, vOrigin, triangleBaseLength, inHorizontal, inDownArrow, inColor_Arrow);
		}
	else
		{
		sDrawEdge(inDC, inArrowRect, edgeRaisedSoft, inColor_Hilite1, inColor_Hilite2, inColor_Shadow1, inColor_Shadow2);
		inDC.SetInk(inColor_Face);
		if (inPressed)
			inDC.Fill(inArrowRect.Inset(1,1));
		else
			inDC.Fill(inArrowRect.Inset(2,2));
		sDrawTriangle(inDC, hOrigin + 1, vOrigin + 1, triangleBaseLength, inHorizontal, inDownArrow, inColor_Hilite1);
		sDrawTriangle(inDC, hOrigin, vOrigin, triangleBaseLength, inHorizontal, inDownArrow, inColor_Shadow1);
		}
	}

void ZUIRenderer_ScrollBar_Win32::sCalcAllRects(const ZRect& inBounds, bool inHorizontal, double inValue, double inThumbProportion, ZCoord inMinThumbLength, ZCoord inArrowSize, 
						ZRect& outThumbRect, ZRect& outTrackRect,
						ZRect& outUpArrowRect, ZRect& outDownArrowRect,
						ZRect& outPageUpRect, ZRect& outPageDownRect)
	{
	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	upArrowRect = inBounds;
	downArrowRect = inBounds;
	pageUpRect = inBounds;
	pageDownRect = inBounds;
	ZCoord trackLength, trackOrigin;
	if (inHorizontal)
		{
		upArrowRect.right = upArrowRect.left + inArrowSize;
		downArrowRect.left = downArrowRect.right - inArrowSize;
		trackRect = inBounds.Inset(inArrowSize, 0);
		trackLength = trackRect.Width();
		trackOrigin = trackRect.Left();
		}
	else
		{
		upArrowRect.bottom = upArrowRect.top + inArrowSize;
		downArrowRect.top = downArrowRect.bottom - inArrowSize;
		trackRect = inBounds.Inset(0, inArrowSize);
		trackLength = trackRect.Height();
		trackOrigin = trackRect.Top();
		}

// How big should the thumb be?
	ZCoord thumbLength = ZCoord(trackLength * inThumbProportion);
	if (thumbLength < inMinThumbLength)
		thumbLength = inMinThumbLength;


	ZCoord thumbOrigin = trackOrigin + ZCoord((trackLength - thumbLength) * inValue);
// Skip drawing the thumb if there's no room on the track
	if (thumbLength <= trackLength)
		{
		if (inHorizontal)
			{
			thumbRect = ZRect(thumbOrigin, trackRect.top, thumbOrigin + thumbLength, trackRect.bottom);
			pageUpRect.left = upArrowRect.right;
			pageUpRect.right = thumbRect.left;
			pageDownRect.left = thumbRect.right;
			pageDownRect.right = downArrowRect.left;
			}
		else
			{
			thumbRect = ZRect(trackRect.left, thumbOrigin, trackRect.right, thumbOrigin + thumbLength);
			pageUpRect.top = upArrowRect.bottom;
			pageUpRect.bottom = thumbRect.top;
			pageDownRect.top = thumbRect.bottom;
			pageDownRect.bottom = downArrowRect.top;
			}
		}
	else
		{
		thumbRect = ZRect::sZero;
		pageUpRect = ZRect::sZero;
		pageDownRect = ZRect::sZero;
		}
	outThumbRect = thumbRect;
	outTrackRect = trackRect;
	outUpArrowRect = upArrowRect;
	outDownArrowRect = downArrowRect;
	outPageUpRect = pageUpRect;
	outPageDownRect = pageDownRect;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Win32

ZUIRenderer_LittleArrows_Win32::ZUIRenderer_LittleArrows_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2,
													const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Arrow, ZRef<ZUIMetric> metric_ButtonWidth, ZRef<ZUIMetric> metric_ButtonHeight)
:	fColor_Hilite1(color_Hilite1), fColor_Hilite2(color_Hilite2), fColor_Shadow1(color_Shadow1), fColor_Shadow2(color_Shadow2),
	fColor_Face(color_Face), fColor_Arrow(color_Arrow), fMetric_ButtonWidth(metric_ButtonWidth), fMetric_ButtonHeight(metric_ButtonHeight)
	{}

ZUIRenderer_LittleArrows_Win32::~ZUIRenderer_LittleArrows_Win32()
	{}

void ZUIRenderer_LittleArrows_Win32::Activate(ZUILittleArrows_Rendered* inLittleArrows)
	{}

ZPoint ZUIRenderer_LittleArrows_Win32::GetPreferredSize()
	{
	return ZPoint(fMetric_ButtonWidth->GetMetric(), fMetric_ButtonHeight->GetMetric()*2);
	}

// From ZUIRenderer_LittleArrows_Base
void ZUIRenderer_LittleArrows_Win32::RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart)
	{
	ZRect upperBounds(inBounds);
	upperBounds.bottom = upperBounds.top + inBounds.Height()/2;
	ZRect lowerBounds(inBounds);
	lowerBounds.top = upperBounds.bottom;
	ZDC localDC(inDC);
	ZUIRenderer_ScrollBar_Win32::sRenderArrow(localDC, upperBounds, false, false, inHitPart == ZUILittleArrows::hitPartUp, inEnabled,
				fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(), fColor_Face->GetColor(), fColor_Arrow->GetColor());
	ZUIRenderer_ScrollBar_Win32::sRenderArrow(localDC, lowerBounds, false, true, inHitPart == ZUILittleArrows::hitPartDown, inEnabled,
				fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor(), fColor_Face->GetColor(), fColor_Arrow->GetColor());

	}

ZUILittleArrows::HitPart ZUIRenderer_LittleArrows_Win32::FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled)
	{
	if (!inEnabled || !inBounds.Contains(inHitPoint))
		return ZUILittleArrows::hitPartNone;
	if (inHitPoint.v < (inBounds.top + inBounds.Height()/2))
		return ZUILittleArrows::hitPartUp;
	return ZUILittleArrows::hitPartDown;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Win32

enum
	{
	progress_Height = 14
	};

ZUIRenderer_ProgressBar_Win32::ZUIRenderer_ProgressBar_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Highlight)
:	fColor_Hilite1(color_Hilite1), fColor_Hilite2(color_Hilite2), fColor_Shadow1(color_Shadow1), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face), fColor_Highlight(color_Highlight)
	{}

ZUIRenderer_ProgressBar_Win32::~ZUIRenderer_ProgressBar_Win32()
	{}

void ZUIRenderer_ProgressBar_Win32::Activate(ZUIProgressBar_Rendered* inProgessBar)
	{}

long ZUIRenderer_ProgressBar_Win32::GetFrameCount()
	{ return 8; }

ZCoord ZUIRenderer_ProgressBar_Win32::GetHeight()
	{ return progress_Height; }

void ZUIRenderer_ProgressBar_Win32::DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, double inLeftEdge, double inRightEdge)
	{
	ZDC localDC(inDC);
	sDrawEdge(localDC, inBounds, edgeSunkSoft | borderUpperLeftOnly,
			fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());

	localDC.SetInk(fColor_Face->GetColor());
	localDC.Fill(inBounds.Inset(1,1));

	ZRect progressBounds = inBounds.Inset(3,3);
	ZCoord leftEdge = progressBounds.left + ZCoord(inLeftEdge * progressBounds.Width());
	ZCoord rightEdge = progressBounds.left + ZCoord(inRightEdge * progressBounds.Width());
	localDC.SetInk(fColor_Highlight->GetColor());
	localDC.Fill(ZRect(leftEdge, progressBounds.top, rightEdge, progressBounds.bottom));
	}

static ZDCPattern sProgressPattern = { {0xe0, 0x70, 0x38, 0x1C, 0x0e, 0x07, 0x83, 0xc1} };

void ZUIRenderer_ProgressBar_Win32::DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long inFrame)
	{
	ZDC localDC(inDC);
	sDrawEdge(localDC, inBounds, edgeSunkSoft | borderUpperLeftOnly,
			fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());

	localDC.SetPenWidth(2);
	localDC.SetInk(fColor_Face->GetColor());
	localDC.Frame(inBounds.Inset(1,1));

	ZDCPattern localPattern;
	ZDCPattern::sOffset(0, inFrame, sProgressPattern.pat, localPattern.pat);

	localDC.SetInk(ZDCInk(fColor_Highlight->GetColor(), fColor_Face->GetColor(), localPattern));
	localDC.Fill(inBounds.Inset(3,3));
	}

bigtime_t ZUIRenderer_ProgressBar_Win32::GetInterFrameDuration()
	{ return ZTicks::sPerSecond()/(2*8); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Win32

static const ZCoord kTabHeight = 22;
static const ZCoord kPanelEndMargin = 2;

ZUIRenderer_TabPane_Win32::ZUIRenderer_TabPane_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face)
:	ZUICaptionRenderer_Win32(color_Hilite1, color_Shadow1), fColor_Hilite2(color_Hilite2), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face)
	{}

ZUIRenderer_TabPane_Win32::~ZUIRenderer_TabPane_Win32()
	{}

void ZUIRenderer_TabPane_Win32::Activate(ZUITabPane_Rendered* inTabPane)
	{}

ZRect ZUIRenderer_TabPane_Win32::GetContentArea(ZUITabPane_Rendered* inTabPane)
	{
	ZRect theRect(inTabPane->GetSize());
	theRect.top += kTabHeight + 2;
	theRect.left += 3;
	theRect.right -= 3;
	theRect.bottom -= 3;
	return theRect;
	}

bool ZUIRenderer_TabPane_Win32::GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk)
	{
	return false;
	}

void ZUIRenderer_TabPane_Win32::RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds)
	{
	bool enabled = inTabPane->GetReallyEnabled();

	ZDC localDC(inDC);

	ZRect topBounds(inBounds);
	topBounds.bottom = topBounds.top + kTabHeight;
	if (localDC.GetClip() & topBounds)
		{
		const vector<ZRef<ZUICaption> >& theVector = inTabPane->GetCaptionVector();
		ZRef<ZUICaption> currentCaption(inTabPane->GetCurrentTabCaption());
		ZCoord leftEdge = kPanelEndMargin;
		for (vector<ZRef<ZUICaption> >::const_iterator i = theVector.begin(); i != theVector.end(); ++i)
			{
			if (*i)
				{
				ZPoint captionSize = (*i)->GetSize();
				this->DrawTab(localDC, *i, ZPoint(leftEdge, inBounds.Top()), captionSize.h, false, *i == currentCaption, enabled);
				leftEdge += captionSize.h + 4;
				}
			}
		}

	ZRect borderBounds = inBounds;
	borderBounds.top += kTabHeight;

// Draw the border area
	sDrawEdge(localDC, borderBounds, edgeRaised, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
	}

void ZUIRenderer_TabPane_Win32::DrawTab(const ZDC& inDC, ZRef<ZUICaption> inCaption, ZPoint location, ZCoord midWidth, bool isTarget, bool isCurrent, bool enabled)
	{
	ZRect tabRect(midWidth + 4, kTabHeight);
	if (!isCurrent)
		tabRect.bottom -= 2;
	tabRect += location;

	sDrawEdge(inDC, tabRect, edgeRaised, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
	if (inCaption)
		{
		ZRect midSectionFrame(tabRect);
		midSectionFrame.left += 2;
		midSectionFrame.right -= 2;
		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.Centered(midSectionFrame);
		ZUIDisplayState inState(false, eZTriState_Off, enabled);
		inCaption->Draw(inDC, captionBounds.TopLeft(), inState, this);
		}
	}

bool ZUIRenderer_TabPane_Win32::DoClick(ZUITabPane_Rendered* inTabPane, ZPoint inHitPoint, const ZEvent_Mouse& inEvent)
	{
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Win32

ZUIRenderer_Group_Win32::ZUIRenderer_Group_Win32(bool isPrimary, const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2)
:	fIsPrimary(isPrimary), fColor_Hilite1(color_Hilite1), fColor_Hilite2(color_Hilite2), fColor_Shadow1(color_Shadow1), fColor_Shadow2(color_Shadow2)
	{
// fIsPrimary is not used yet
	}
ZUIRenderer_Group_Win32::~ZUIRenderer_Group_Win32()
	{
	}

void ZUIRenderer_Group_Win32::Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize)
	{}

void ZUIRenderer_Group_Win32::RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize) 
	{
	ZDC localDC(inDC);
	if (inTitleSize.h > 0)
		{
		ZRect clipRect(inBounds.left + 8, inBounds.top, inBounds.left + inTitleSize.h + 12, inBounds.top + inTitleSize.v);
		localDC.SetClip(localDC.GetClip() - clipRect);
		}
	ZRect realBounds(inBounds);
	realBounds.top += inTitleSize.v / 2;

	if (inEnabled)
		sDrawEdge(localDC, realBounds, edgeEtched, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
	else
		sDrawEdge(localDC, realBounds, edgeFlat, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
	}

ZCoord ZUIRenderer_Group_Win32::GetTitlePaneHLocation()
	{ return 10; }

ZDCInk ZUIRenderer_Group_Win32::GetBackInk(const ZDC& inDC)
	{ return ZDCInk();}

void ZUIRenderer_Group_Win32::GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight)
	{
	outTopLeft = ZPoint(10, max(ZCoord(inTitleSize.v + 4), ZCoord(12)));
	outBottomRight = ZPoint(-10, -10);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Win32

ZUIRenderer_FocusFrame_Win32::ZUIRenderer_FocusFrame_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Hilite2, const ZRef<ZUIColor>& color_Shadow1, const ZRef<ZUIColor>& color_Shadow2, const ZRef<ZUIColor>& color_Face, const ZRef<ZUIColor>& color_Window)
:	fColor_Hilite1(color_Hilite1), fColor_Hilite2(color_Hilite2), fColor_Shadow1(color_Shadow1), fColor_Shadow2(color_Shadow2), fColor_Face(color_Face), fColor_Window(color_Window)
	{}

ZUIRenderer_FocusFrame_Win32::~ZUIRenderer_FocusFrame_Win32()
	{}

void ZUIRenderer_FocusFrame_Win32::Activate(ZUIFocusFrame_Rendered* inFocusFrame)
	{}

void ZUIRenderer_FocusFrame_Win32::Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	sDrawEdge(inDC, inBounds, edgeSunkSoft, fColor_Hilite1->GetColor(), fColor_Hilite2->GetColor(), fColor_Shadow1->GetColor(), fColor_Shadow2->GetColor());
	}

void ZUIRenderer_FocusFrame_Win32::GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	outTopLeftInset = ZPoint(2,2);
	outBottomRightInset = ZPoint(-2, - 2);
	}

ZDCRgn ZUIRenderer_FocusFrame_Win32::GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds)
	{ return ZDCRgn(); }

bool ZUIRenderer_FocusFrame_Win32::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{
	outInk = inState.GetEnabled() ? fColor_Window->GetColor() : fColor_Face->GetColor();
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Win32

ZUIRenderer_Divider_Win32::ZUIRenderer_Divider_Win32(const ZRef<ZUIColor>& color_Hilite1, const ZRef<ZUIColor>& color_Shadow1)
:	fColor_Hilite1(color_Hilite1), fColor_Shadow1(color_Shadow1)
	{}

ZUIRenderer_Divider_Win32::~ZUIRenderer_Divider_Win32()
	{}

void ZUIRenderer_Divider_Win32::Activate(ZUIDivider_Rendered* inDivider)
	{}

ZCoord ZUIRenderer_Divider_Win32::GetDividerThickness()
	{ return 2; }

void ZUIRenderer_Divider_Win32::DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	localDC.PenNormal();

	if (inHorizontal)
		{
		localDC.SetInk(fColor_Shadow1->GetColor());
		localDC.Line(inLocation.h, inLocation.v, inLocation.h + inLength - 1, inLocation.v);

		localDC.SetInk(fColor_Hilite1->GetColor());
		localDC.Line(inLocation.h + 1, inLocation.v + 1, inLocation.h + inLength - 1, inLocation.v + 1);
		}
	else
		{
		localDC.SetInk(fColor_Shadow1->GetColor());
		localDC.Line(inLocation.h, inLocation.v, inLocation.h, inLocation.v + inLength - 1);

		localDC.SetInk(fColor_Hilite1->GetColor());
		localDC.Line(inLocation.h + 1, inLocation.v + 1, inLocation.h, inLocation.v + inLength - 1);
		}
	}

// =================================================================================================
// =================================================================================================
#pragma mark -
#pragma mark * ZUIIconRenderer_Win32

class ZUIIconRenderer_Win32 : public ZUIIconRenderer
	{
public:
	ZUIIconRenderer_Win32(const ZRef<ZUIColor>& inColor_Hilighlight);
	virtual ~ZUIIconRenderer_Win32();

	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled);
protected:
	ZRef<ZUIColor> fColor_Highlight;
	};

ZUIIconRenderer_Win32::ZUIIconRenderer_Win32(const ZRef<ZUIColor>& inColor_Hilighlight)
:	fColor_Highlight(inColor_Hilighlight)
	{}

ZUIIconRenderer_Win32::~ZUIIconRenderer_Win32()
	{}

void ZUIIconRenderer_Win32::Draw(const ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo, bool inSelected, bool inOpen, bool inEnabled)
	{
	ZDC localDC(inDC);

	ZDCPixmap theSourcePixmap;
	if (inOpen)
		{
		if (inEnabled && inSelected)
			{
			ZRGBColor theBlackColor = ZRGBColor::sBlack / 2 + fColor_Highlight->GetColor() / 2;
			ZRGBColor theWhiteColor = ZRGBColor::sWhite / 2 + fColor_Highlight->GetColor() / 2;
			theSourcePixmap = sMakeHollowPixmap(inPixmapCombo.GetMask(), theBlackColor, theWhiteColor);
			}
		else
			{
			theSourcePixmap = sMakeHollowPixmap(inPixmapCombo.GetMask(), ZRGBColor::sBlack, ZRGBColor::sWhite);
			if (!inEnabled)
				theSourcePixmap.Munge(sMungeProc_Lighten, nil);
			}
		}
	else
		{
		theSourcePixmap = inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono();
		if (inEnabled)
			{
			if (inSelected)
				{
				ZRGBColor theColor = fColor_Highlight->GetColor() / 2;
				theSourcePixmap.Munge(sMungeProc_Tint, &theColor);
				}
			}
		else
			theSourcePixmap.Munge(sMungeProc_Lighten, nil);
		}

	localDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Win32

ZUIRendererFactory_Win32::ZUIRendererFactory_Win32()
	{
#if ZCONFIG(OS, Win32)
	fColor_ButtonText = new ZUIColor_Win32System(COLOR_BTNTEXT);
	fColor_Highlight = new ZUIColor_Win32System(COLOR_HIGHLIGHT);
	fColor_Face = new ZUIColor_Win32System(COLOR_3DFACE);
	fColor_Hilite1 = new ZUIColor_Win32System(COLOR_3DHILIGHT);
	fColor_Hilite2 = new ZUIColor_Win32System(COLOR_3DLIGHT);
	fColor_Shadow1 = new ZUIColor_Win32System(COLOR_3DSHADOW);
	fColor_Shadow2 = new ZUIColor_Win32System(COLOR_3DDKSHADOW);
	fColor_ScrollBarTrack = new ZUIColor_Win32SystemLightened(COLOR_SCROLLBAR);
	fColor_Window = new ZUIColor_Win32System(COLOR_WINDOW);
#else // ZCONFIG(OS, Win32)
// AG 98-12-14. Note that we return Win32-style colors. On the Mac, and all right-thinking
// systems, standard colors run like this: 0x0000, 0x1111, 0x2222, ... 0xEEEE, 0xFFFF. This
// spreads 16 values over the whole range. Windows uses a lot of colors like 0xD0 and 0x80, which
// are harder to work with -- you end up with wierd rounding and arithmetic problems. But we want
// to be accurate in emulating the colors of windows elements. In any case, a user is free to
// go in and change any of these values on Windows, we're just returning the standard ones as shipped.
	fColor_ButtonText = new ZUIColor_Fixed(ZRGBColor(0x0000));
	fColor_Highlight = new ZUIColor_Fixed(ZRGBColor::sBlue/2);
	fColor_Face = new ZUIColor_Fixed(ZRGBColor(0xC000));
	fColor_Hilite1 = new ZUIColor_Fixed(ZRGBColor(0xFFFF));
	fColor_Hilite2 = new ZUIColor_Fixed(ZRGBColor(0xD000));
	fColor_Shadow1 = new ZUIColor_Fixed(ZRGBColor(0x8000));
	fColor_Shadow2 = new ZUIColor_Fixed(ZRGBColor(0x0000));
	fColor_ScrollBarTrack = new ZUIColor_Fixed(ZRGBColor(0xE000)); // ??? Correct ???
	fColor_Window = new ZUIColor_Fixed(ZRGBColor(0xFFFF));
#endif // ZCONFIG(OS, Win32)
	}

ZUIRendererFactory_Win32::~ZUIRendererFactory_Win32()
	{}

// From ZUIRendererFactory
string ZUIRendererFactory_Win32::GetName()
	{ return "Win32"; }

// From ZUIRendererFactory_Caching
ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Win32::Make_Renderer_CaptionPane()
	{ return new ZUIRenderer_CaptionPane_Win32(fColor_Hilite1, fColor_Shadow1); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonPush()
	{ return new ZUIRenderer_ButtonPush_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonRadio()
	{ return new ZUIRenderer_ButtonRadio_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonCheckbox()
	{ return new ZUIRenderer_ButtonCheckbox_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonPopup()
	{
#if ZCONFIG(OS, Win32)
	return new ZUIRenderer_ButtonPopup_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, fColor_Window, new ZUIMetric_Win32System(SM_CXHSCROLL, -1));
#else
	return new ZUIRenderer_ButtonPopup_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, fColor_Window, new ZUIMetric_Fixed(15));
#endif
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonBevel()
	{ return this->Get_Renderer_ButtonPush(); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonDisclosure()
	{ return new ZUIRenderer_ButtonDisclosure_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Win32::Make_Renderer_ButtonPlacard()
	{ return ZRef<ZUIRenderer_Button>() /*new ZUIRenderer_ButtonPlacard_Win32()*/; }

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Win32::Make_Renderer_ScrollBar()
	{
#if ZCONFIG(OS, Win32)
	return new ZUIRenderer_ScrollBar_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, fColor_ScrollBarTrack, new ZUIMetric_Win32System(SM_CXHSCROLL, -1), new ZUIMetric_Win32System(SM_CXHTHUMB));
#else
	return new ZUIRenderer_ScrollBar_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, fColor_ScrollBarTrack, new ZUIMetric_Fixed(15), new ZUIMetric_Fixed(15));
#endif // ZCONFIG(OS, Win32)
	}

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Win32::Make_Renderer_Slider()
	{ return ZRef<ZUIRenderer_Slider>(); }

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Win32::Make_Renderer_LittleArrows()
	{
#if ZCONFIG(OS, Win32)
	return new ZUIRenderer_LittleArrows_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, new ZUIMetric_Win32System(SM_CXVSCROLL, -1), new ZUIMetric_Win32System(SM_CXHSCROLL, -1));
#else
	return new ZUIRenderer_LittleArrows_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_ButtonText, new ZUIMetric_Fixed(15), new ZUIMetric_Fixed(15));
#endif
	}

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Win32::Make_Renderer_ProgressBar()
	{ return new ZUIRenderer_ProgressBar_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Highlight); }

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Win32::Make_Renderer_ChasingArrows()
	{ return ZRef<ZUIRenderer_ChasingArrows>() /*new ZUIRenderer_ChasingArrows_Win32()*/; }

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Win32::Make_Renderer_TabPane()
	{ return new ZUIRenderer_TabPane_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Win32::Make_Renderer_GroupPrimary()
	{ return new ZUIRenderer_Group_Win32(true, fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Win32::Make_Renderer_GroupSecondary()
	{ return new ZUIRenderer_Group_Win32(false, fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Win32::Make_Renderer_FocusFrameEditText()
	{ return new ZUIRenderer_FocusFrame_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Win32::Make_Renderer_FocusFrameListBox()
	{ return new ZUIRenderer_FocusFrame_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Win32::Make_Renderer_FocusFrameDateControl()
	{ return new ZUIRenderer_FocusFrame_Win32(fColor_Hilite1, fColor_Hilite2, fColor_Shadow1, fColor_Shadow2, fColor_Face, fColor_Window); }

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Win32::Make_Renderer_Splitter()
	{ return new ZUIRenderer_Splitter_Standard; }

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Win32::Make_Renderer_Divider()
	{ return new ZUIRenderer_Divider_Win32(fColor_Hilite1, fColor_Shadow1); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Win32

ZUIAttributeFactory_Win32::ZUIAttributeFactory_Win32()
	{
#if ZCONFIG(OS, Win32)
	fColor_Highlight = new ZUIColor_Win32System(COLOR_HIGHLIGHT);
#else // ZCONFIG(OS, Win32)
	fColor_Highlight = new ZUIColor_Fixed(ZRGBColor::sBlue/2);
#endif // ZCONFIG(OS, Win32)
	}

ZUIAttributeFactory_Win32::~ZUIAttributeFactory_Win32()
	{}

string ZUIAttributeFactory_Win32::GetName()
	{ return "Win32"; }

ZRef<ZUIColor> ZUIAttributeFactory_Win32::MakeColor(const string& inColorName)
	{
#if ZCONFIG(OS, Win32)
	if (inColorName == "Text")
		return new ZUIColor_Win32System(COLOR_WINDOWTEXT);
	if (inColorName == "TextHighlight")
		return new ZUIColor_Win32System(COLOR_HIGHLIGHTTEXT);
#else // ZCONFIG(OS, Win32)
	if (inColorName == "Text")
		return new ZUIColor_Fixed(ZRGBColor::sBlack);
	if (inColorName == "TextHighlight")
		return new ZUIColor_Fixed(ZRGBColor::sWhite);
#endif // ZCONFIG(OS, Win32)
	return ZUIAttributeFactory_Caching::MakeColor(inColorName);
	}

ZRef<ZUIInk> ZUIAttributeFactory_Win32::MakeInk(const string& inInkName)
	{
#if ZCONFIG(OS, Win32)
	if (inInkName == "WindowBackground_Document")
		return new ZUIInk_Win32System(COLOR_WINDOW);
	if (inInkName == "WindowBackground_Dialog")
		return new ZUIInk_Win32System(COLOR_3DFACE); // ??
	if (inInkName == "Background_Highlight")
		return new ZUIInk_Win32System(COLOR_HIGHLIGHT);
#else // ZCONFIG(OS, Win32)
	if (inInkName == "WindowBackground_Document")
		return new ZUIInk_Fixed(ZRGBColor(0xFFFF));
	if (inInkName == "WindowBackground_Dialog")
		return new ZUIInk_Fixed(ZRGBColor(0xC000));
	if (inInkName == "Background_Highlight")
		return new ZUIInk_Fixed(ZRGBColor::sBlue/2);
#endif // ZCONFIG(OS, Win32)
	return ZUIAttributeFactory_Caching::MakeInk(inInkName);
	}

ZRef<ZUIFont> ZUIAttributeFactory_Win32::MakeFont(const string& inFontName)
	{
#if ZCONFIG(OS, Win32)
	if (inFontName == "SystemLarge")
		return new ZUIFont_Win32System(ZUIFont_Win32System::eWhichFontMenu);
	if (inFontName == "SystemSmall")
		return new ZUIFont_Win32System(ZUIFont_Win32System::eWhichFontMenu);
	if (inFontName == "SystemSmallEmphasized")
		return new ZUIFont_Win32System(ZUIFont_Win32System::eWhichFontMenu);
	if (inFontName == "Application")
		return new ZUIFont_Win32System(ZUIFont_Win32System::eWhichFontMenu);
	if (inFontName == "Monospaced")
		return new ZUIFont_Fixed(ZDCFont::sMonospaced9);
#else // ZCONFIG(OS, Win32)
	if (inFontName == "SystemLarge")
		return new ZUIFont_Fixed(ZDCFont::sSystem);
	if (inFontName == "SystemSmall")
		return new ZUIFont_Fixed(ZDCFont::sApp10Bold);
	if (inFontName == "SystemSmallEmphasized")
		return new ZUIFont_Fixed(ZDCFont::sApp10Bold);
	if (inFontName == "Application")
		return new ZUIFont_Fixed(ZDCFont::sApp9);
	if (inFontName == "Monospaced")
		return new ZUIFont_Fixed(ZDCFont::sMonospaced9);
#endif // ZCONFIG(OS, Win32)
	return ZUIAttributeFactory_Caching::MakeFont(inFontName);
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Win32::MakeMetric(const string& inMetricName)
	{
	return ZUIAttributeFactory_Caching::MakeMetric(inMetricName);
	}

ZRef<ZUIIconRenderer> ZUIAttributeFactory_Win32::GetIconRenderer()
	{
	return new ZUIIconRenderer_Win32(fColor_Highlight);
	}

// =================================================================================================
