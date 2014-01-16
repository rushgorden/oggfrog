static const char rcsid[] = "@(#) $Id: ZUIRenderer_Platinum.cpp,v 1.12 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUIRenderer_Platinum.h"
#include "ZTextUtil.h"
#include "ZTicks.h"

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
#pragma mark -
#pragma mark * ZUIRenderer_Platinum

// ZUIRenderer_Platinum. Utility base class stuff.

ZRGBColor ZUIRenderer_Platinum::sColorTable[]=
	{
	ZRGBColor(0x0000),
	ZRGBColor(0x1111),
	ZRGBColor(0x2222),
	ZRGBColor(0x3333),
	ZRGBColor(0x4444),
	ZRGBColor(0x5555),
	ZRGBColor(0x6666),
	ZRGBColor(0x7777),
	ZRGBColor(0x8888),
	ZRGBColor(0x9999),
	ZRGBColor(0xAAAA),
	ZRGBColor(0xBBBB),
	ZRGBColor(0xCCCC),
	ZRGBColor(0xDDDD),
	ZRGBColor(0xEEEE),
	ZRGBColor(0xFFFF),
	ZRGBColor(0xCCCC, 0xCCCC, 0xFFFF), // P1
	ZRGBColor(0x9999, 0x9999, 0xFFFF), // P2
	ZRGBColor(0x6666, 0x6666, 0xCCCC), // P3
	ZRGBColor(0x3333, 0x3333, 0x9999), // P4
	};

static const size_t sColorTableSize = countof(ZUIRenderer_Platinum::sColorTable);

static ZRGBColorMap sColorMap[]=
	{
	{ '_', { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF } },
	{ 'E', { 0xEEEE, 0xEEEE, 0xEEEE, 0xFFFF } },
	{ 'D', { 0xDDDD, 0xDDDD, 0xDDDD, 0xFFFF } },
	{ 'C', { 0xCCCC, 0xCCCC, 0xCCCC, 0xFFFF } },
	{ 'B', { 0xBBBB, 0xBBBB, 0xBBBB, 0xFFFF } },
	{ 'A', { 0xAAAA, 0xAAAA, 0xAAAA, 0xFFFF } },
	{ '9', { 0x9999, 0x9999, 0x9999, 0xFFFF } },
	{ '8', { 0x8888, 0x8888, 0x8888, 0xFFFF } },
	{ '7', { 0x7777, 0x7777, 0x7777, 0xFFFF } },
	{ '6', { 0x6666, 0x6666, 0x6666, 0xFFFF } },
	{ '5', { 0x5555, 0x5555, 0x5555, 0xFFFF } },
	{ '4', { 0x4444, 0x4444, 0x4444, 0xFFFF } },
	{ '3', { 0x3333, 0x3333, 0x3333, 0xFFFF } },
	{ '2', { 0x2222, 0x2222, 0x2222, 0xFFFF } },
	{ '1', { 0x1111, 0x1111, 0x1111, 0xFFFF } },
	{ '0', { 0x0000, 0x0000, 0x0000, 0xFFFF } },
	{ 'P', { 0xCCCC, 0xCCCC, 0xFFFF, 0xFFFF } },
	{ 'Q', { 0x9999, 0x9999, 0xFFFF, 0xFFFF } },
	{ 'R', { 0x6666, 0x6666, 0xCCCC, 0xFFFF } },
	{ 'S', { 0x3333, 0x3333, 0x9999, 0xFFFF } },
	{ ' ', { 0x0000, 0x0000, 0x0000, 0x0000 } } // Transparent alpha
	};

static const size_t sColorMapSize = sizeof(sColorMap) / sizeof(ZRGBColorMap);

static ZDCPixmapNS::PixelDesc sPlatinumPixelDesc(sColorMap, sColorMapSize);

ZUIRenderer_Platinum::ZUIRenderer_Platinum()
	{}

ZUIRenderer_Platinum::~ZUIRenderer_Platinum()
	{}

void ZUIRenderer_Platinum::sDrawSide_BottomRight(const ZDC& inDC, const ZRect& inRect, ZCoord leftInset, ZCoord topInset, ZCoord rightInset, ZCoord bottomInset)
	{
	inDC.Line(inRect.left + leftInset, inRect.bottom - 1 - bottomInset, inRect.right - 1 - rightInset, inRect.bottom - 1 - bottomInset);
	inDC.Line(inRect.right - 1 - rightInset, inRect.bottom - 1 - bottomInset, inRect.right - 1 - rightInset, inRect.top + topInset);
	}

void ZUIRenderer_Platinum::sDrawSide_TopLeft(const ZDC& inDC, const ZRect& inRect, ZCoord leftInset, ZCoord topInset, ZCoord rightInset, ZCoord bottomInset)
	{
	inDC.Line(inRect.left + leftInset, inRect.bottom - 1 - bottomInset, inRect.left + leftInset, inRect.top + topInset);
	inDC.Line(inRect.left + leftInset, inRect.top + topInset, inRect.right - 1 - rightInset, inRect.top + topInset);
	}

void ZUIRenderer_Platinum::sDrawBevel_TopLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	ZDC localDC(inDC);
	for (short counter = 0; counter < colorCount; ++counter)
		{
		localDC.SetInk(colors[counter]);
		sDrawSide_TopLeft(localDC, inRect.Inset(counter, counter), 0,0,0,0);
		}
	}

void ZUIRenderer_Platinum::sDrawBevel_BottomRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	ZDC localDC(inDC);
	for (short counter = 0; counter < colorCount; ++counter)
		{
		localDC.SetInk(colors[counter]);
		sDrawSide_BottomRight(localDC, inRect.Inset(counter, counter), 0,0,0,0);
		}
	}

void ZUIRenderer_Platinum::sDrawCorner_BottomLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	for (short counter = 0; counter < colorCount; ++counter)
		inDC.Pixel(inRect.left + counter, inRect.bottom - 1 - counter, colors[counter]);
	}

void ZUIRenderer_Platinum::sDrawCorner_BottomRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	for (short counter = 0; counter < colorCount; ++counter)
		inDC.Pixel(inRect.right - 1 - counter, inRect.bottom - 1 - counter, colors[counter]);
	}

void ZUIRenderer_Platinum::sDrawCorner_TopLeft(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	for (short counter = 0; counter < colorCount; ++counter)
		inDC.Pixel(inRect.left + counter, inRect.top + counter, colors[counter]);
	}

void ZUIRenderer_Platinum::sDrawCorner_TopRight(const ZDC& inDC, const ZRect& inRect, ZRGBColor colors[], short colorCount)
	{
	for (short counter = 0; counter < colorCount; ++counter)
		inDC.Pixel(inRect.right - 1 - counter, inRect.top + counter, colors[counter]);
	}

void ZUIRenderer_Platinum::sDrawTriangle(const ZDC& inDC, const ZRGBColor& inColor, ZCoord inHCoord, ZCoord inVCoord, ZCoord inBaseLength, bool inHorizontal, bool inDownArrow, bool inHollow, bool inGrayPattern)
	{
	ZDC localDC(inDC);
	ZCoord arrowHeight = (inBaseLength + 1) / 2;
	if (inGrayPattern)
		localDC.SetInk(ZDCInk(inColor, ZRGBColor::sWhite, ZDCPattern::sGray));
	else
		localDC.SetInk(inColor);
	if (inHorizontal)
		{
		if (inDownArrow)
			{
			if (inHollow)
				localDC.Line(inHCoord, inVCoord, inHCoord, inVCoord + inBaseLength - 1);
			for (long x = 0; x < arrowHeight; ++x)
				{
				if (inHollow)
					{
					localDC.Pixel(inHCoord + x, inVCoord + x, inColor);
					localDC.Pixel(inHCoord + x, inVCoord + inBaseLength - x - 1, inColor);
					}
				else
					localDC.Line(inHCoord + x, inVCoord + x, inHCoord + x, inVCoord + inBaseLength - x - 1);
				}
			}
		else
			{
			if (inHollow)
				localDC.Line(inHCoord + arrowHeight - 1, inVCoord, inHCoord + arrowHeight - 1, inVCoord + inBaseLength - 1);
			for (long x = 0; x < arrowHeight; ++x)
				{
				if (inHollow)
					{
					localDC.Pixel(inHCoord + (arrowHeight - x - 1), inVCoord + x, inColor);
					localDC.Pixel(inHCoord + (arrowHeight - x - 1), inVCoord + inBaseLength - x - 1, inColor);
					}
				else
					localDC.Line(inHCoord + (arrowHeight - x - 1), inVCoord + x, inHCoord + (arrowHeight - x - 1), inVCoord + inBaseLength - x - 1);
				}
			}
		}
	else
		{
		if (inDownArrow)
			{
			if (inHollow)
				localDC.Line(inHCoord, inVCoord, inHCoord + inBaseLength - 1, inVCoord);
			for (long x = 0; x < arrowHeight; ++x)
				{
				if (inHollow)
					{
					localDC.Pixel(inHCoord + x, inVCoord + x, inColor);
					localDC.Pixel(inHCoord + inBaseLength - x - 1, inVCoord + x, inColor);
					}
				else
					localDC.Line(inHCoord + x, inVCoord + x, inHCoord + inBaseLength - x - 1, inVCoord + x);
				}
			}
		else
			{
			if (inHollow)
				localDC.Line(inHCoord, inVCoord + arrowHeight - 1, inHCoord + inBaseLength - 1, inVCoord + arrowHeight - 1);
			for (long x = 0; x < arrowHeight; ++x)
				{
				if (inHollow)
					{
					localDC.Pixel(inHCoord + x, inVCoord + (arrowHeight - x - 1), inColor);
					localDC.Pixel(inHCoord + inBaseLength - x - 1, inVCoord + (arrowHeight - x - 1), inColor);
					}
				else
					localDC.Line(inHCoord + x, inVCoord + (arrowHeight - x - 1), inHCoord + inBaseLength - x - 1, inVCoord + (arrowHeight - x - 1));
				}
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUICaptionRenderer_Platinum

ZUICaptionRenderer_Platinum::ZUICaptionRenderer_Platinum()
	{}

ZUICaptionRenderer_Platinum::~ZUICaptionRenderer_Platinum()
	{}

void ZUICaptionRenderer_Platinum::Imp_DrawCaptionText(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const char* inText, size_t inTextLength, const ZDCFont& inFont, ZCoord inWrapWidth)
	{
	ZCoord wrapWidth(inWrapWidth);
	if (wrapWidth <= 0)
		wrapWidth = 32767;

	ZDC localDC(inDC);
	localDC.SetFont(inFont);
	localDC.SetTextColor(ZRGBColor::sBlack);
	if (localDC.GetDepth() >= 4)
		{
		if (!(inState.GetEnabled() && inState.GetActive()))
			localDC.SetTextColor(ZUIRenderer_Platinum::sLookupColor(ZUIRenderer_Platinum::kColorRamp_Gray_8));
		else if (inState.GetPressed())
			localDC.SetTextColor(ZUIRenderer_Platinum::sLookupColor(ZUIRenderer_Platinum::kColorRamp_White));
		}
	else
		{
// We're not handling disabled yet -- it needs to be dithered gray
		if (inState.GetPressed())// || inState.GetHilite() != eZTriState_Off)
			localDC.SetTextColor(ZRGBColor::sWhite);
		}
	ZTextUtil_sDraw(localDC, inLocation, wrapWidth, inText, inTextLength);
	}

void ZUICaptionRenderer_Platinum::Imp_DrawCaptionPixmap(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, const ZDCPixmapCombo& inPixmapCombo)
	{
	if (inDC.GetDepth() >= 4)
		{
		ZDCPixmap theSourcePixmap = inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono();
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
		ZDCPixmap theSourcePixmap= inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor();
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
//			theSourcePixmap.Munge(sMungeProc_Dither, nil, nil);
			}
		inDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_CaptionPane_Platinum

ZUIRenderer_CaptionPane_Platinum::ZUIRenderer_CaptionPane_Platinum()
	{}

ZUIRenderer_CaptionPane_Platinum::~ZUIRenderer_CaptionPane_Platinum()
	{}

void ZUIRenderer_CaptionPane_Platinum::Activate(ZUICaptionPane_Rendered* inCaptionPane)
	{ inCaptionPane->Refresh(); }

void ZUIRenderer_CaptionPane_Platinum::Draw(ZUICaptionPane_Rendered* inCaptionPane, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	if (inCaption)
		inCaption->Draw(inDC, inBounds.TopLeft(), inState, this);
	}

ZPoint ZUIRenderer_CaptionPane_Platinum::GetSize(ZUICaptionPane_Rendered* inCaptionPane, ZRef<ZUICaption> inCaption)
	{
	if (inCaption)
		return inCaption->GetSize();
	return ZPoint::sZero;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPush_Platinum

ZUIRenderer_ButtonPush_Platinum::ZUIRenderer_ButtonPush_Platinum()
	{}

ZUIRenderer_ButtonPush_Platinum::~ZUIRenderer_ButtonPush_Platinum()
	{}

void ZUIRenderer_ButtonPush_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonPush_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;

	ZRect realRect = inBounds;
	if (isDefault)
		realRect = inBounds.Inset(3,3);

	sRenderPush(inDC, realRect, inState);
	if (isDefault)
		sRenderFocusRing(inDC, realRect, inState);

	if (inCaption)
		{
		ZDC localDC(inDC);
		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.Centered(realRect);
		inCaption->Draw(localDC, captionBounds.TopLeft(), inState, this);
		}
	}

ZPoint ZUIRenderer_ButtonPush_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;

	ZPoint theSize(16,5);
	if (isDefault)
		theSize += ZPoint(6, 6);

	if (inCaption)
		theSize += inCaption->GetSize();
	return theSize;
	}

void ZUIRenderer_ButtonPush_Platinum::GetInsets(ZUIButton_Rendered* inButton, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	bool isDefault;
	if (!inButton->GetAttribute("ZUIButton::IsDefault", nil, &isDefault))
		isDefault = false;
	if (isDefault)
		{
		outTopLeftInset.h = outTopLeftInset.v = 3;
		outBottomRightInset.h = outBottomRightInset.v = -3;
		}
	else
		{
		outTopLeftInset.h = outTopLeftInset.v = 0;
		outBottomRightInset.h = outBottomRightInset.v = 0;
		}
	}

void ZUIRenderer_ButtonPush_Platinum::sRenderPush(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (localDC.GetDepth() < 4)
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
// Draw the frame for the control outside of everything else
			localDC.SetInk(sLookupColor(kColorRamp_Black));
			localDC.Frame(inBounds, ZPoint(6, 6));

// Paint the face of the control
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Black : kColorRamp_White));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));
			}
		else
			{
			localDC.SetInk(ZDCInk::sGray);
			localDC.Frame(inBounds, ZPoint(6, 6));

			localDC.SetInk(sLookupColor(kColorRamp_White));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));
			}
		}
	else
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
// Draw the frame for the control outside of everything else
			localDC.SetInk(sLookupColor(kColorRamp_Black));
			localDC.Frame(inBounds, ZPoint(6, 6));

// Paint the face of the control first
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_6 : kColorRamp_Gray_D));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));

// LIGHT EDGES
// Start by rendering the bevelled edges of the sides facing the light source
			if (inState.GetPressed())
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_4));
				localDC.Line(inBounds.left + 1, inBounds.bottom - 3, inBounds.left + 1, inBounds.top + 2);
				localDC.Line(inBounds.left + 2, inBounds.top + 1, inBounds.right - 3, inBounds.top + 1);
				}

			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_5 : kColorRamp_White));
			sDrawSide_TopLeft(localDC, inBounds, 2, 2, 3, 3);
			localDC.Pixel(inBounds.left + 3, inBounds.top + 3, sLookupColor(inState.GetPressed() ? kColorRamp_Gray_5 : kColorRamp_White));

			if (inState.GetPressed())
				{
				localDC.Pixel(inBounds.left + 2, inBounds.top + 2, sLookupColor(kColorRamp_Gray_4));
				ZRGBColor aColor(sLookupColor(kColorRamp_Gray_7));
				localDC.Pixel(inBounds.left + 2, inBounds.bottom - 2, aColor);
				localDC.Pixel(inBounds.right - 2, inBounds.top + 2, aColor);
				}
			else
				{
				ZRGBColor aColor(sLookupColor(kColorRamp_Gray_B));
				localDC.Pixel(inBounds.left + 1, inBounds.top + 2, aColor);
				localDC.Pixel(inBounds.left + 2, inBounds.top + 1, aColor);
				localDC.Pixel(inBounds.left + 1, inBounds.bottom - 3, aColor);
				localDC.Pixel(inBounds.left + 2, inBounds.bottom - 2, aColor);
				localDC.Pixel(inBounds.right - 3, inBounds.top + 1, aColor);
				localDC.Pixel(inBounds.right - 2, inBounds.top + 2, aColor);
				}


// SHADOW EDGES
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_8 : kColorRamp_Gray_7));
			localDC.Line(inBounds.left + 3, inBounds.bottom - 2, inBounds.right - 3, inBounds.bottom - 2);
			localDC.Line(inBounds.right - 2, inBounds.bottom - 3, inBounds.right - 2, inBounds.top + 3);

			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_7 : kColorRamp_Gray_A));
			sDrawSide_BottomRight(localDC, inBounds, 3, 3, 2, 2);

			localDC.Pixel(inBounds.right - 3, inBounds.bottom - 3, sLookupColor(inState.GetPressed() ? kColorRamp_Gray_8 : kColorRamp_Gray_7));

			localDC.Pixel(inBounds.right - 4, inBounds.bottom - 4, sLookupColor(inState.GetPressed() ? kColorRamp_Gray_7 : kColorRamp_Gray_A));
			}
		else
			{
// Disabled
			localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
			localDC.Frame(inBounds, ZPoint(6, 6));

// Paint the face of the control
			localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));
			}
		}
	}

static const char* sPixvals_FocusRing[8] = 
	{
// TL
		"   20   "
		"  0DD   "
		" 0DDA   "
		"2DDA7   "
		"0DA70   ",
// TR
		"02      "
		"DC0     "
		"AAB0    "
		"7AA82   "
		"07A70   ",
// BL
		"0CA70   "
		"2CAA7   "
		" 0BAA   "
		"  087   "
		"   20   ",
// BR
		"07A70   "
		"7A872   "
		"A870    "
		"770     "
		"02      ",
// Disabled
// TL
		"   88   "
		"  8BB   "
		" 8BBB   "
		"8BBBB   "
		"8BBB8   ",
// TR
		"88      "
		"BB8     "
		"BBB8    "
		"BBBB8   "
		"8BBB8   ",
// BL
		"8BBB8   "
		"8BBBB   "
		" 8BBB   "
		"  8BB   "
		"   88   ",
// BR
		"8BBB8   "
		"BBBB8   "
		"BBB8    "
		"BB8     "
		"88      "
	};

void ZUIRenderer_ButtonPush_Platinum::sRenderFocusRing(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZRect focusRect(inBounds.Inset( - 3, -3));
	ZDC localDC(inDC);
/*	if (localDC.GetDepth() < 4)
		{
		if (inState.GetEnabled())
			{
			}
		else
			{
			}
		}
	else*/
		{
		if (inState.GetEnabled() && inState.GetActive())
			localDC.SetInk(sLookupColor(kColorRamp_Black));
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
// Black lines -- outer ring
		localDC.Line(focusRect.left + 5, focusRect.top, focusRect.right - 6, focusRect.top);
		localDC.Line(focusRect.left, focusRect.top + 5, focusRect.left, focusRect.bottom - 6);
		localDC.Line(focusRect.left + 5, focusRect.bottom - 1, focusRect.right - 6, focusRect.bottom - 1);
		localDC.Line(focusRect.right - 1, focusRect.top + 5, focusRect.right - 1, focusRect.bottom - 6);
// Middle ring
// Top Left
		if (inState.GetEnabled() && inState.GetActive())
			localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
		localDC.Line(focusRect.left + 5, focusRect.top + 1, focusRect.right - 6, focusRect.top + 1);
		localDC.Line(focusRect.left + 1, focusRect.top + 5, focusRect.left + 1, focusRect.bottom - 6);
// Bottom Right
		if (inState.GetEnabled() && inState.GetActive())
			localDC.SetInk(sLookupColor(kColorRamp_Gray_7));
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
		localDC.Line(focusRect.left + 5, focusRect.bottom - 2, focusRect.right - 6, focusRect.bottom - 2);
		localDC.Line(focusRect.right - 2, focusRect.top + 5, focusRect.right - 2, focusRect.bottom - 6);
// Inner ring
// Top Left
		if (inState.GetEnabled() && inState.GetActive())
			localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
		localDC.Line(focusRect.left + 5, focusRect.top + 2, focusRect.right - 6, focusRect.top + 2);
		localDC.Line(focusRect.left + 2, focusRect.top + 5, focusRect.left + 2, focusRect.bottom - 6);
// Bottom Right
		localDC.Line(focusRect.left + 5, focusRect.bottom - 3, focusRect.right - 6, focusRect.bottom - 3);
		localDC.Line(focusRect.right - 3, focusRect.top + 5, focusRect.right - 3, focusRect.bottom - 6);
// Corners
		if (inState.GetEnabled() && inState.GetActive())
			{
			localDC.Pixels(focusRect.left, focusRect.top, 8, 5, sPixvals_FocusRing[0], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.right - 5, focusRect.top, 8, 5, sPixvals_FocusRing[1], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.left, focusRect.bottom - 5, 8, 5, sPixvals_FocusRing[2], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.right - 5, focusRect.bottom - 5, 8, 5, sPixvals_FocusRing[3], sColorMap, sColorMapSize);
			}
		else
			{
			localDC.Pixels(focusRect.left, focusRect.top, 8, 5, sPixvals_FocusRing[4], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.right - 5, focusRect.top, 8, 5, sPixvals_FocusRing[5], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.left, focusRect.bottom - 5, 8, 5, sPixvals_FocusRing[6], sColorMap, sColorMapSize);
			localDC.Pixels(focusRect.right - 5, focusRect.bottom - 5, 8, 5, sPixvals_FocusRing[7], sColorMap, sColorMapSize);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonRadio_Platinum

enum
	{
	kRadio_Width = 12,
	kRadio_Height = 12,
	kRadio_TextOffset = 5
	};

ZUIRenderer_ButtonRadio_Platinum::ZUIRenderer_ButtonRadio_Platinum()
	{}

ZUIRenderer_ButtonRadio_Platinum::~ZUIRenderer_ButtonRadio_Platinum()
	{}

void ZUIRenderer_ButtonRadio_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonRadio_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZPoint radioLocation = inBounds.TopLeft();
	radioLocation.v += (inBounds.Height() - kRadio_Height) / 2;
	sRenderRadio(inDC, radioLocation, inState);
	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		ZPoint location = inBounds.TopLeft();
		location.h += kRadio_Width + kRadio_TextOffset;
		location.v += (inBounds.Height() - captionSize.v) / 2;
		ZUIDisplayState newState(inState);
		newState.SetPressed(false);
		newState.SetHilite(eZTriState_Off);
		inCaption->Draw(inDC, location, newState, this);
		}
	}

ZPoint ZUIRenderer_ButtonRadio_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(kRadio_Width, kRadio_Height);
	if (inCaption)
		{
		theSize = inCaption->GetSize();
		theSize.h += kRadio_Width + kRadio_TextOffset;
		theSize.v = max(theSize.v, ZCoord(kRadio_Height));
		}
	return theSize;
	}

static const char* sPixvals_Radio[] = 
	{
// kRadio_NormalColorOff
		"   A4004A   "
		"  05DDDB40  "
		" 0BDE___D80 "
		"A5DE__EEDB4A"
		"4DE__EEDDB84"
		"0D__EEDDBB80"
		"0D_EEDDBBA80"
		"4B_EDDBBAA84"
		"A5DDDBBAA84A"
		" 08BBBAA880 "
		"  05888840  "
		"   A4004A   "
		,
// kRadio_NormalColorOn
		"   A2002A   "
		"  20455540  "
		" 2457778890 "
		"A0570000994A"
		"2470000009B4"
		"057000000BB0"
		"057000000BD0"
		"258000000D_4"
		"A2890000D_4A"
		" 2999BBD__0 "
		"  24BBD_40  "
		"   A4004A   "
		,
// kRadio_NormalColorMixed
		"   A4004A   "
		"  05DDDB40  "
		" 0BDE___D80 "
		"A5DE__EEDB4A"
		"4DE__EEDDB84"
		"0D_000000B80"
		"0D_000000A80"
		"4B_EDDBBAA84"
		"A5DDDBBAA84A"
		" 08BBBAA880 "
		"  05888840  "
		"   A4004A   "
		,
// kRadio_PushedColorOff
		"   A2002A   "
		"  20444440  "
		" 2445566670 "
		"A0455667774A"
		"245566777894"
		"045667778890"
		"046677788990"
		"2467778899B4"
		"A26778899B5A"
		" 4778899BB0 "
		"  24999B50  "
		"   A4004A   "
		,
// kRadio_PushedColorOn
		"   A2002A   "
		"  20444440  "
		" 2445566670 "
		"A0450000774A"
		"245000000894"
		"045000000890"
		"046000000990"
		"2460000009B4"
		"A26700009B5A"
		" 4778899BB0 "
		"  24999B50  "
		"   A4004A   "
		,
// kRadio_PushedColorMixed
		"   A2002A   "
		"  20444440  "
		" 2445566670 "
		"A0455667774A"
		"245566777890"
		"045000000890"
		"046000000990"
		"2467778899B4"
		"A26778899B5A"
		" 4778899BB0 "
		"  24999B50  "
		"   A4004A   "
		,
// kRadio_DimmedColorOff
		"    8888    "
		"  88DDDD88  "
		" 8DDDDDDDD8 "
		" 8DDDDDDDD8 "
		"8DDDDDDDDDD8"
		"8DDDDDDDDDD8"
		"8DDDDDDDDDD8"
		"8DDDDDDDDDD8"
		" 8DDDDDDDD8 "
		" 8DDDDDDDD8 "
		"  88DDDD88  "
		"    8888    "
		,
// kRadio_DimmedColorOn
		"    8888    "
		"  88DDDD88  "
		" 8DDDDDDDD8 "
		" 8DD7777DD8 "
		"8DD777777DD8"
		"8DD777777DD8"
		"8DD777777DD8"
		"8DD777777DD8"
		" 8DD7777DD8 "
		" 8DDDDDDDD8 "
		"  88DDDD88  "
		"    8888    "
		,
// kRadio_DimmedColorMixed
		"    8888    "
		"  88DDDD88  "
		" 8DDDDDDDD8 "
		" 8DDDDDDDD8 "
		"8DDDDDDDDDD8"
		"8DD777777DD8"
		"8DD777777DD8"
		"8DDDDDDDDDD8"
		" 8DDDDDDDD8 "
		" 8DDDDDDDD8 "
		"  88DDDD88  "
		"    8888    "
		,
// kRadio_NormalBWOff
		"    0000    "
		"  00____00  "
		" 0________0 "
		" 0________0 "
		"0__________0"
		"0__________0"
		"0__________0"
		"0__________0"
		" 0________0 "
		" 0________0 "
		"  00____00  "
		"    0000    "
		,
// kRadio_NormalBWOn
		"    0000    "
		"  00____00  "
		" 0________0 "
		" 0__0000__0 "
		"0__000000__0"
		"0__000000__0"
		"0__000000__0"
		"0__000000__0"
		" 0__0000__0 "
		" 0________0 "
		"  00____00  "
		"    0000    "
		,
// kRadio_NormalBWMixed
		"    0000    "
		"  00____00  "
		" 0________0 "
		" 0________0 "
		"0__________0"
		"0__000000__0"
		"0__000000__0"
		"0__________0"
		" 0________0 "
		" 0________0 "
		"  00____00  "
		"    0000    "
		,
// kRadio_PushedBWOff
		"    0000    "
		"  00000000  "
		" 000____000 "
		" 00______00 "
		"00________00"
		"00________00"
		"00________00"
		"00________00"
		" 00______00 "
		" 000____000 "
		"  00000000  "
		"    0000    "
		,
// kRadio_PushedBWOn
		"    0000    "
		"  00000000  "
		" 000____000 "
		" 00_0000_00 "
		"00_000000_00"
		"00_000000_00"
		"00_000000_00"
		"00_000000_00"
		" 00_0000_00 "
		" 000____000 "
		"  00000000  "
		"    0000    "
		,
// kRadio_PushedBWMixed
		"    0000    "
		"  00000000  "
		" 000____000 "
		" 00______00 "
		"00________00"
		"00_000000_00"
		"00_000000_00"
		"00________00"
		" 00______00 "
		" 000____000 "
		"  00000000  "
		"    0000    "
		,
// kRadio_DimmedBWOff
		"    _0_0    "
		"  _0____0_  "
		" __________ "
		" 0________0 "
		"0__________0"
		"____________"
		"0__________0"
		"____________"
		" __________ "
		" 0________0 "
		"  0_____0_  "
		"    _0_0    "
		,
// kRadio_DimmedBWOn
		"    _0_0    "
		"  00____0_  "
		" __________ "
		" 0___0_0__0 "
		"0___0_0_0__0"
		"___0_0_0____"
		"0___0_0_0__0"
		"___0_0_0____"
		" 0__0_0____ "
		" 0________0 "
		"  0_____0_  "
		"    _0_0    "
		,
// kRadio_DimmedBWMixed
		"    _0_0    "
		"  _0____0_  "
		" __________ "
		" 0________0 "
		"0__________0"
		"0___0_0_0__0"
		"0__0_0_0___0"
		"____________"
		" __________ "
		" 0________0 "
		"  0_____0_  "
		"    _0_0    "
	};

enum
	{
	kRadio_NormalColorOff = 0,
	kRadio_NormalColorOn,
	kRadio_NormalColorMixed,
	kRadio_PushedColorOff,
	kRadio_PushedColorOn,
	kRadio_PushedColorMixed,
	kRadio_DimmedColorOff,
	kRadio_DimmedColorOn,
	kRadio_DimmedColorMixed,
	kRadio_NormalBWOff,
	kRadio_NormalBWOn,
	kRadio_NormalBWMixed,
	kRadio_PushedBWOff,
	kRadio_PushedBWOn,
	kRadio_PushedBWMixed,
	kRadio_DimmedBWOff,
	kRadio_DimmedBWOn,
	kRadio_DimmedBWMixed
	};

void ZUIRenderer_ButtonRadio_Platinum::sRenderRadio(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	short depth = localDC.GetDepth();

	short whichOne = 0;

	if (inState.GetHilite() == eZTriState_On)
		whichOne = kRadio_NormalColorOn;
	else if (inState.GetHilite() == eZTriState_Off)
		whichOne = kRadio_NormalColorOff;
	else
		whichOne = kRadio_NormalColorMixed;

	if (!(inState.GetEnabled() && inState.GetActive()))
		whichOne += kRadio_DimmedColorOff - kRadio_NormalColorOff;
	else if (inState.GetPressed())
		whichOne += kRadio_PushedColorOff - kRadio_NormalColorOff;

	if (depth < 4)
		whichOne += kRadio_NormalBWOff - kRadio_NormalColorOff;

	localDC.Pixels(inLocation, ZPoint(kRadio_Width, kRadio_Height), sPixvals_Radio[whichOne], sColorMap, sColorMapSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonCheckbox_Platinum

enum
	{
	kCheckbox_RowBytes = 16,
	kCheckbox_Width = 12,
	kCheckbox_Height = 12,
	kCheckbox_TextOffset = 5
	};

ZUIRenderer_ButtonCheckbox_Platinum::ZUIRenderer_ButtonCheckbox_Platinum()
	{}

ZUIRenderer_ButtonCheckbox_Platinum::~ZUIRenderer_ButtonCheckbox_Platinum()
	{}

void ZUIRenderer_ButtonCheckbox_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonCheckbox_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZPoint checkboxLocation = inBounds.TopLeft();
	checkboxLocation.v += (inBounds.Height() - kCheckbox_Height) / 2;
	sRenderCheckbox(inDC, checkboxLocation, inState);
	if (inCaption)
		{
		ZPoint captionSize = inCaption->GetSize();
		ZPoint location = inBounds.TopLeft();
		location.h += kCheckbox_Width + kCheckbox_TextOffset;
		location.v += (inBounds.Height() - captionSize.v) / 2;
		ZUIDisplayState newState(inState);
		newState.SetPressed(false);
		newState.SetHilite(eZTriState_Off);
		inCaption->Draw(inDC, location, newState, this);
		}
	}

ZPoint ZUIRenderer_ButtonCheckbox_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(kCheckbox_Width, kCheckbox_Height);
	if (inCaption)
		{
		theSize = inCaption->GetSize();
		theSize.h += kCheckbox_Width + kCheckbox_TextOffset;
		theSize.v = max(theSize.v, ZCoord(kCheckbox_Height));
		}
	return theSize;
	}

static const char* sPixvals_Checkbox[] = 
	{
// kCheckbox_NormalColorOff
		"000000000000    "
		"0_________D0    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0D8888888880    "
		"000000000000    "
		,
// kCheckbox_NormalColorOn
		"000000000000    "
		"0_________D00   "
		"0_DDDDDDDD007A  "
		"0_DDDDDDD000A   "
		"0_DDDDDD0050    "
		"0_00DDD00770    "
		"0_D00D007A80    "
		"0_DA0007AD80    "
		"0_DDA07ADD80    "
		"0_DDD7ADDD80    "
		"0D8888888880    "
		"000000000000    "
		,
// kCheckbox_NormalColorOnClassic
		"000000000000    "
		"0_________D0    "
		"0_D0DDDD0D80    "
		"0_D00DD00780    "
		"0_DD00007A80    "
		"0_DDD007AD80    "
		"0_DD0000DD80    "
		"0_D007A00D80    "
		"0_D07ADD0780    "
		"0_DDDDDDDD80    "
		"0D8888888880    "
		"000000000000    "
		,
// kCheckbox_NormalColorMixed
		"000000000000    "
		"0_________D0    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0_D000000A80    "
		"0_D000000A80    "
		"0_DDAAAAAA80    "
		"0_DDDDDDDD80    "
		"0_DDDDDDDD80    "
		"0D8888888880    "
		"000000000000    "
		,
// kCheckbox_PushedColorOff
		"000000000000    "
		"055555555570    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"079999999990    "
		"000000000000    "
		,
// kCheckbox_PushedColorOn
		"000000000000    "
		"0555555555700   "
		"05777777770057  "
		"0577777770007   "
		"057777770040    "
		"050077700450    "
		"057007004590    "
		"057500045790    "
		"057750457790    "
		"057774577790    "
		"079999999990    "
		"000000000000    "
		,
// kCheckbox_PushedColorOnClassic
		"000000000000    "
		"055555555570    "
		"057077770790    "
		"057007700490    "
		"057700004590    "
		"057770045790    "
		"057700007790    "
		"057004500790    "
		"057045770490    "
		"057757777590    "
		"079999999990    "
		"000000000000    "
		,
// kCheckbox_PushedColorMixed
		"000000000000    "
		"055555555570    "
		"057777777790    "
		"057777777790    "
		"057777777790    "
		"057000000590    "
		"057000000590    "
		"057755555590    "
		"057777777790    "
		"057777777790    "
		"079999999990    "
		"000000000000    "
		,
// kCheckbox_DimmedColorOff
		"888888888888    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"888888888888    "
		,
// kCheckbox_DimmedColorOn
		"888888888888    "
		"8DDDDDDDDDD88   "
		"8DDDDDDDDD8888  "
		"8DDDDDDDD8888   "
		"8DDDDDDD88D8    "
		"8D88DDD88DD8    "
		"8DD88D88DDD8    "
		"8DDD888DDDD8    "
		"8DDDD8DDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD0    "
		"888888888888    "
		,
// kCheckbox_DimmedColorOnClassic
		"888888888888    "
		"8DDDDDDDDDD8    "
		"8DD8DDDD8DD8    "
		"8DD88DD88DD8    "
		"8DDD8888DDD8    "
		"8DDDD88DDDD8    "
		"8DDD8888DDD8    "
		"8DD88DD88DD8    "
		"8DD8DDDD8DD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"888888888888    "
		,
// kCheckbox_DimmedColorMixed
		"888888888888    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DD888888DD8    "
		"8DD888888DD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"8DDDDDDDDDD8    "
		"888888888888    "
		,
// kCheckbox_NormalBWOff
		"000000000000    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"000000000000    "
		,
// kCheckbox_NormalBWOn
		"000000000000    "
		"0__________00   "
		"0_________00    "
		"0________000    "
		"0_______00_0    "
		"0_00___00__0    "
		"0__00_00___0    "
		"0___000____0    "
		"0____0_____0    "
		"0__________0    "
		"0__________0    "
		"000000000000    "
		,
// kCheckbox_NormalBWOnClassic
		"000000000000    "
		"00________00    "
		"0_0______0_0    "
		"0__0____0__0    "
		"0___0__0___0    "
		"0____00____0    "
		"0____00____0    "
		"0___0__0___0    "
		"0__0____0__0    "
		"0_0______0_0    "
		"00________00    "
		"000000000000    "
		,
// kCheckbox_NormalBWMixed
		"000000000000    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__000000__0    "
		"0__000000__0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"0__________0    "
		"000000000000    "
		,
// kCheckbox_PushedBWOff
		"000000000000    "
		"000000000000    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"000000000000    "
		"000000000000    "
		,
// kCheckbox_PushedBWOn
		"000000000000    "
		"0000000000000   "
		"00________0000  "
		"00_______0000   "
		"00______0000    "
		"0000___00_00    "
		"00_00_00__00    "
		"00__000___00    "
		"00___0____00    "
		"00________00    "
		"000000000000    "
		"000000000000    "
		,
// kCheckbox_PushedBWOnClassic
		"000000000000    "
		"000000000000    "
		"000______000    "
		"00_0____0_00    "
		"00__0__0__00    "
		"00___00___00    "
		"00___00___00    "
		"00__0__0__00    "
		"00_0____0_00    "
		"000______000    "
		"000000000000    "
		"000000000000    "
		,
// kCheckbox_PushedBWMixed
		"000000000000    "
		"000000000000    "
		"00________00    "
		"00________00    "
		"00________00    "
		"00_000000_00    "
		"00_000000_00    "
		"00________00    "
		"00________00    "
		"00________00    "
		"000000000000    "
		"000000000000    "
		,
// kCheckbox_DimmedBWOff
		"_0_0_0_0_0_0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0_0_0_0_0_0_    "
		,
// kCheckbox_DimmedBWOn
		"_0_0_0_0_0_0    "
		"0___________0   "
		"___________0    "
		"0_________0_    "
		"_________0_0    "
		"0_0_____0___    "
		"___0___0___0    "
		"0___0_0_____    "
		"_____0_____0    "
		"0___________    "
		"___________0    "
		"0_0_0_0_0_0_    "
		,
// kCheckbox_DimmedBWOnClassic
		"_0_0_0_0_0_0    "
		"00________00    "
		"0_0______0_0    "
		"0__0____0__0    "
		"0___0__0___0    "
		"0____00____0    "
		"0____00____0    "
		"0___0__0___0    "
		"0__0____0__0    "
		"0_0______0_0    "
		"00________00    "
		"0_0_0_0_0_0_    "
		,
// kCheckbox_DimmedBWMixed
		"_0_0_0_0_0_0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0___0_0_0__0    "
		"___0_0_0___0    "
		"0___________    "
		"___________0    "
		"0___________    "
		"___________0    "
		"0_0_0_0_0_0_    "
	};

enum
	{
	kCheckbox_NormalColorOff = 0,
	kCheckbox_NormalColorOn,
	kCheckbox_NormalColorOnClassic,
	kCheckbox_NormalColorMixed,
	kCheckbox_PushedColorOff,
	kCheckbox_PushedColorOn,
	kCheckbox_PushedColorOnClassic,
	kCheckbox_PushedColorMixed,
	kCheckbox_DimmedColorOff,
	kCheckbox_DimmedColorOn,
	kCheckbox_DimmedColorOnClassic,
	kCheckbox_DimmedColorMixed,
	kCheckbox_NormalBWOff,
	kCheckbox_NormalBWOn,
	kCheckbox_NormalBWOnClassic,
	kCheckbox_NormalBWMixed,
	kCheckbox_PushedBWOff,
	kCheckbox_PushedBWOn,
	kCheckbox_PushedBWOnClassic,
	kCheckbox_PushedBWMixed,
	kCheckbox_DimmedBWOff,
	kCheckbox_DimmedBWOn,
	kCheckbox_DimmedBWOnClassic,
	kCheckbox_DimmedBWMixed
	};

void ZUIRenderer_ButtonCheckbox_Platinum::sRenderCheckbox(const ZDC& inDC, const ZPoint& inLocation, const ZUIDisplayState& inState)
	{
	bool useClassicLook = false;
	ZDC localDC(inDC);
	short depth = localDC.GetDepth();

	short whichOne = 0;

	if (inState.GetHilite() == eZTriState_On)
		{
		if (useClassicLook)
			whichOne = kCheckbox_NormalColorOnClassic;
		else
			whichOne = kCheckbox_NormalColorOn;
		}
	else if (inState.GetHilite() == eZTriState_Off)
		whichOne = kCheckbox_NormalColorOff;
	else
		whichOne = kCheckbox_NormalColorMixed;

	if (!(inState.GetEnabled() && inState.GetActive()))
		whichOne += kCheckbox_DimmedColorOff - kCheckbox_NormalColorOff;
	else if (inState.GetPressed())
		whichOne += kCheckbox_PushedColorOff - kCheckbox_NormalColorOff;

	if (depth < 4)
		whichOne += kCheckbox_NormalBWOff - kCheckbox_NormalColorOff;

	localDC.Pixels(inLocation, ZPoint(kCheckbox_RowBytes, kCheckbox_Height), sPixvals_Checkbox[whichOne], sColorMap, sColorMapSize);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPopup_Platinum

const short kPopup_RightInset = 24; // Used to position the title rect
const short kPopup_ArrowLeftInset = 6; // Used to position the popup arrow
const short kPopup_TitleInset = 8; // Apple specification
const short kPopup_TitleTrailingInset = 3; // Space between end of title and the button portion of the popup
const short kPopup_LabelOffset = 2; // Offset of label from popup
const short kPopup_ArrowButtonWidth = 22; // Width used in drawing the arrow only
const short kPopup_ArrowButtonHeight = 18; // Height used for drawing arrow only
const short kPopup_ArrowHeight = 5; // Actual height of the arrow
const short kPopup_ArrowWidth = 9; // Actual width of the arrow at widest
const short kPopup_MultiArrowHeight = 10; // Actual height of the two arrow
const short kPopup_MultiArrowWidth = 7; // Actual width of the arrow at widest
const short kPopup_TopOffset = 4; // Used to figure out the best height
const short kPopup_BottomOffset = 2; // for the popup when calculating the best rect

ZUIRenderer_ButtonPopup_Platinum::ZUIRenderer_ButtonPopup_Platinum()
	{}

ZUIRenderer_ButtonPopup_Platinum::~ZUIRenderer_ButtonPopup_Platinum()
	{}

void ZUIRenderer_ButtonPopup_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

ZPoint ZUIRenderer_ButtonPopup_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZPoint theSize(kPopup_ArrowButtonWidth, kPopup_ArrowButtonHeight);
	if (inCaption && inCaption->IsValid())
		{
		theSize = inCaption->GetSize();
		theSize.h += kPopup_TitleInset + kPopup_TitleTrailingInset + kPopup_ArrowButtonWidth;
		theSize.v += 5;
		}
	return theSize;
	}

void ZUIRenderer_ButtonPopup_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	sRenderPopup(inDC, inBounds, inState, !inCaption->IsValid(), false);
	if (inCaption && inCaption->IsValid())
		{
		ZDC localDC(inDC);
		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.CenteredV(inBounds);
		captionBounds += ZPoint(kPopup_TitleInset, 0);
		localDC.SetClip(localDC.GetClip() & captionBounds);
		inCaption->Draw(localDC, captionBounds.TopLeft(), inState, this);
		}
	}

void ZUIRenderer_ButtonPopup_Platinum::sRenderPopup(const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState, bool isArrowOnly, bool isPullDown)
	{
	bool miniArrows = inBounds.Height() < kPopup_ArrowButtonHeight;
	ZCoord popupButtonWidth = miniArrows ? kPopup_ArrowButtonWidth - 3 : kPopup_ArrowButtonWidth;

	ZDC localDC(inDC);
	if (localDC.GetDepth() < 4)
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
// Draw the frame for the control outside of everything else
			localDC.SetInk(sLookupColor(kColorRamp_Black));
			localDC.Frame(inBounds, ZPoint(6, 6));

// Paint the face of the control
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Black : kColorRamp_White));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));
			}
		else
			{
			localDC.SetInk(ZDCInk::sGray);
			localDC.Frame(inBounds, ZPoint(6, 6));

			localDC.SetInk(sLookupColor(kColorRamp_White));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4, 4));
			}
		}
	else
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
// Draw the frame for the control outside of everything else
			localDC.SetInk(sLookupColor(kColorRamp_Black));
			localDC.Frame(inBounds, ZPoint(6, 6));

// First make sure the face of the control is drawn
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_6 : kColorRamp_Gray_D));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4,4));
			if (!isArrowOnly)
				{
// EDGES ON BODY OF POPUP
// LIGHT EDGES
// Start by rendering the bevelled edges of the sides facing the light source
				localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_5 : kColorRamp_White));
				localDC.Line(inBounds.left + 1, inBounds.bottom - 3, inBounds.left + 1, inBounds.top + 2);
				localDC.Line(inBounds.left + 2, inBounds.top + 1, inBounds.right - popupButtonWidth, inBounds.top + 1);

// SHADOW EDGES
// Render the shadow bevels
				localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_7 : kColorRamp_Gray_A));
				sDrawSide_BottomRight(localDC, inBounds, 2, 2, popupButtonWidth - 1, 1);
				if (!inState.GetPressed())
					localDC.Pixel(inBounds.right - (popupButtonWidth), inBounds.top + 1, sLookupColor(kColorRamp_Gray_E));
				}

// EDGES ON ARROW BUTTON
// LIGHT EDGES
// Start by rendering the bevelled edges of the
// sides facing the light source
// Setup the appropriate width for the popup
// arrow portion of the rendering
			ZCoord arrowButtonWidth = popupButtonWidth;
			if (isArrowOnly)
				arrowButtonWidth = inBounds.Width();

			if (inState.GetPressed())
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_4));
				sDrawSide_TopLeft(localDC, inBounds, inBounds.Width() - (arrowButtonWidth - 1), 1, 2, 1);
				}
			else
				{
				localDC.Pixel(inBounds.right - 3, inBounds.top + 1, sLookupColor(kColorRamp_Gray_B));

				localDC.Pixel(inBounds.right - 2, inBounds.top + 2, sLookupColor(kColorRamp_Gray_A));
				}

			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_6 : kColorRamp_White));
			sDrawSide_TopLeft(localDC, inBounds, inBounds.Width() - (arrowButtonWidth - 2), 2, 3, 3);


// SHADOW EDGES
// Render the shadow bevels
			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_8 : kColorRamp_Gray_7));
			localDC.Line(inBounds.left + (inBounds.Width() - (arrowButtonWidth - 2)), inBounds.bottom - 2, inBounds.right - 3, inBounds.bottom - 2);
			localDC.Line(inBounds.right - 2, inBounds.bottom - 3, inBounds.right - 2, inBounds.top + 2);

			localDC.SetInk(sLookupColor(inState.GetPressed() ? kColorRamp_Gray_7 : kColorRamp_Gray_A));
			sDrawSide_BottomRight(localDC, inBounds, inBounds.Width() - (arrowButtonWidth - 3), 3, 2, 2);

			localDC.Pixel(inBounds.right - 3, inBounds.bottom - 3, sLookupColor(inState.GetPressed() ? kColorRamp_Gray_8 : kColorRamp_Gray_7));

			if (!inState.GetPressed() && !isArrowOnly)
				localDC.Pixel(inBounds.right - (popupButtonWidth - 1), inBounds.bottom - 2, sLookupColor(kColorRamp_Gray_B));
			}
		else
			{
// DISABLED
			localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
			localDC.Frame(inBounds, ZPoint(6, 6));

// First make sure the face of the control is drawn
			localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
			localDC.Fill(inBounds.Inset(1,1), ZPoint(4,4));
			if (!isArrowOnly)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
				localDC.Line(inBounds.right - popupButtonWidth, inBounds.top + 2, inBounds.right - popupButtonWidth, inBounds.bottom - 2 );
				}
			}
		}

// ARROW(S)
	localDC.SetInk(sLookupColor(kColorRamp_Black));

	ZCoord leftEdge, rightEdge;
	ZCoord rowCount = miniArrows ? 2 : 3; // Zero based so one less than real row count

	if (localDC.GetDepth() < 4)
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
			if (inState.GetPressed())
				localDC.SetInk(sLookupColor(kColorRamp_White));
			}
		else
			localDC.SetInk(ZDCInk::sGray);
		}
	else
		{
		if (inState.GetEnabled() && inState.GetActive())
			{
			if (inState.GetPressed())
				localDC.SetInk(sLookupColor(kColorRamp_White));
			else
				localDC.SetInk(sLookupColor(kColorRamp_Black));
			}
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
		}
	if (isPullDown)
		{
// For the pulldown menu there is only one arrow
// drawn that faces down, figure out the left
// and right edges based on whether we are
// drawing only the arrow portion or the entire
// popup
		ZCoord leftEdge, rightEdge;
		ZCoord start = ((inBounds.Height() - kPopup_ArrowHeight)/2) +1;
		if (isArrowOnly)
			{
			leftEdge = inBounds.Width() - ((inBounds.Width() - kPopup_ArrowWidth) / 2);
			rightEdge = leftEdge - (kPopup_ArrowWidth - 1);
			}
		else
			{
			leftEdge = kPopup_ArrowButtonWidth - 6;
			rightEdge = leftEdge - (kPopup_ArrowWidth - 1);
			}
// Arrow drawing loop draws 5 rows to make the arrow
		for (ZCoord counter = 0; counter <= 4; ++counter)
			{
			localDC.Line(inBounds.right - (leftEdge - counter), inBounds.top + start + counter,
							inBounds.right - (rightEdge + counter), inBounds.top + start + counter);
			}
		}
	else
		{
		ZCoord arrowWidth, topStart;
// Mini arrow drawing values
		if (miniArrows)
			{
			arrowWidth = kPopup_MultiArrowWidth - 2;
			topStart = (inBounds.Height() - (((rowCount + 1) * 2) + 2)) / 2;
			leftEdge = (popupButtonWidth - arrowWidth) - 2;
			rightEdge = leftEdge - (kPopup_MultiArrowWidth - 3);

// If we are only drawing the arrow portion of
// the popup we need to set it up a little
// differently
			if (isArrowOnly)
				{
				leftEdge = inBounds.Width() - ((inBounds.Width() - arrowWidth) / 2);
				rightEdge = leftEdge - (arrowWidth - 1);
				}
			}
		else
			{
// Normal arrow drawing values
			arrowWidth = kPopup_MultiArrowWidth;
			topStart = (inBounds.Height() - (((rowCount + 1) * 2) + 2)) / 2;
			leftEdge = (popupButtonWidth - arrowWidth);
			rightEdge = leftEdge - (kPopup_MultiArrowWidth - 1);
			}

// TOP ARROW
// Arrow drawing loop draws 4 rows to make the
// arrow, the top arrow points upwards
		for (ZCoord counter = 0; counter <= rowCount; ++counter)
			{
			localDC.Line(inBounds.right - (leftEdge - (rowCount - counter)), inBounds.top + topStart + counter,
								inBounds.right - (rightEdge + (rowCount - counter)), inBounds.top + topStart + counter );
			}

// BOTTOM ARROW
// Arrow drawing loop draws 4 rows to make the
// arrow, the bottom arrow points downwards
		ZCoord botStart = topStart + (miniArrows ? 5 : 6);
		for (ZCoord counter = 0; counter <= rowCount; counter++ )
			{
			localDC.Line(inBounds.right - (leftEdge - counter), inBounds.top + botStart + counter,
							inBounds.right - (rightEdge + counter), inBounds.top + botStart + counter );
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonBevel_Platinum

ZUIRenderer_ButtonBevel_Platinum::ZUIRenderer_ButtonBevel_Platinum()
	{}

ZUIRenderer_ButtonBevel_Platinum::~ZUIRenderer_ButtonBevel_Platinum()
	{}

void ZUIRenderer_ButtonBevel_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonBevel_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{
	ZCoord bevelWidth;
	if (!inButton->GetAttribute("ZUIButton::BevelWidth", nil, &bevelWidth))
		bevelWidth = 1;
	bevelWidth = max((ZCoord)1, min(bevelWidth, (ZCoord)3));

	sRenderBevel(inDC, bevelWidth, inBounds, inState);

	bool hasPopupGlyph;
	if (!inButton->GetAttribute("ZUIButton::HasPopupArrow", nil, &hasPopupGlyph))
		hasPopupGlyph = false;

	if (inCaption && inCaption->IsValid())
		{
		ZRect localButtonRect = inBounds;

		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.Centered(localButtonRect);
		inCaption->Draw(inDC, captionBounds.TopLeft(), inState, this);
		}

	if (hasPopupGlyph)
		{
		bool popupRightFacing;
		if (!inButton->GetAttribute("ZUIButton::PopupRightFacing", nil, &popupRightFacing))
			popupRightFacing = false;
		bool popupCentred = true;
		if (!inButton->GetAttribute("ZUIButton::PopupCentred", nil, &popupCentred))
			popupCentred = false;
		ZCoord popupBaseLength;
		if (!inButton->GetAttribute("ZUIButton::PopupBaseLength", nil, &popupBaseLength))
			popupBaseLength = 5;
		sRenderBevelPopup(inDC, bevelWidth, inBounds, popupRightFacing, popupCentred, popupBaseLength, inState);
		}
	}

void ZUIRenderer_ButtonBevel_Platinum::sRenderBevel(const ZDC& inDC, short bevelWidth, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	if (inState.GetEnabled() && inState.GetActive())
		{
		bool backgroundDown = (inState.GetPressed() || inState.GetHilite() != eZTriState_Off);
		if (localDC.GetDepth() < 4)
			{
			if (backgroundDown)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Black));
				localDC.Fill(inBounds);
				}
			else
				{
				localDC.SetInk(sLookupColor(kColorRamp_White));
				localDC.Fill(inBounds.Inset(1,1));
				localDC.SetInk(sLookupColor(kColorRamp_Black));
				localDC.Frame(inBounds);
				}
			}
		else
			{
			ZRGBColor colorArray[3];
// FRAME BUTTON
			localDC.SetInk(sLookupColor(backgroundDown ? kColorRamp_Gray_1 : kColorRamp_Gray_6));
			sDrawSide_TopLeft(localDC, inBounds, 0,0,1,1);

			localDC.SetInk(sLookupColor(backgroundDown ? kColorRamp_Gray_4 : kColorRamp_Gray_3));
			sDrawSide_BottomRight(localDC, inBounds, 1,1,0,0);

// TOP RIGHT
			ZRGBColor aColor = (bevelWidth == 2 ? sLookupColor(backgroundDown ? kColorRamp_Gray_1 : kColorRamp_Gray_6) : sLookupColor(backgroundDown ? kColorRamp_Gray_2 : kColorRamp_Gray_5));
			localDC.Pixel(inBounds.right - 1, inBounds.top, aColor);

// BOTTOM LEFT
			localDC.Pixel(inBounds.left, inBounds.bottom - 1, aColor);

// FACE COLOR
			ZRect localFrame(inBounds.Inset(1,1));
			localDC.SetInk(sLookupColor(backgroundDown ? kColorRamp_Gray_8 : kColorRamp_Gray_C));
			localDC.Fill(localFrame);

// LIGHT BEVELS
// Setup Colors
			switch (bevelWidth)
				{
				case 1:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_8 : kColorRamp_White);
				break;

				case 2:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_4 : kColorRamp_Gray_C);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_6 : kColorRamp_White);
				break;

				case 3:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_3 : kColorRamp_Gray_C);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_4 : kColorRamp_Gray_E);
					colorArray[2] = sLookupColor(backgroundDown ? kColorRamp_Gray_6 : kColorRamp_White);
				break;
				}

// Draw top and left edges
			sDrawBevel_TopLeft(localDC, localFrame, colorArray, bevelWidth);

// SHADOW BEVELS
// Setup Colors
			switch (bevelWidth)
				{
				case 1:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_A : kColorRamp_Gray_8);
				break;

				case 2:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_B : kColorRamp_Gray_7);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_9 : kColorRamp_Gray_9);
				break;

				case 3:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_B : kColorRamp_Gray_5);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_A : kColorRamp_Gray_7);
					colorArray[2] = sLookupColor(backgroundDown ? kColorRamp_Gray_9 : kColorRamp_Gray_9);
				break;
				}

// Draw bottom and right edges
			sDrawBevel_BottomRight(localDC, localFrame, colorArray, bevelWidth);

// CORNER PIXELS
			switch (bevelWidth)
				{
				case 1:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_7 : kColorRamp_Gray_C);
				break;

				case 2:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_7 : kColorRamp_Gray_A);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_8 : kColorRamp_Gray_C);
				break;

				case 3:
					colorArray[0] = sLookupColor(backgroundDown ? kColorRamp_Gray_4 : kColorRamp_Gray_A);
					colorArray[1] = sLookupColor(backgroundDown ? kColorRamp_Gray_7 : kColorRamp_Gray_B);
					colorArray[2] = sLookupColor(backgroundDown ? kColorRamp_Gray_8 : kColorRamp_Gray_C);
				break;
				}

// Paint corner pixels
// TOP RIGHT
			sDrawCorner_TopRight(localDC, localFrame, colorArray, bevelWidth);

// BOTTOM LEFT
			sDrawCorner_BottomLeft(localDC, localFrame, colorArray, bevelWidth);
			}
		}
	else
		{
// DISABLED
		bool isHilited = (inState.GetHilite() != eZTriState_Off);
		if (localDC.GetDepth() < 4)
			{
			if (isHilited)
				{
				localDC.SetInk(ZDCInk::sGray);
				localDC.Fill(inBounds);
				}
			else
				{
				localDC.SetInk(sLookupColor(kColorRamp_White));
				localDC.Fill(inBounds.Inset(1,1));
				localDC.SetInk(ZDCInk::sGray);
				localDC.Frame(inBounds);
				}
			}
		else
			{
			localDC.SetInk(sLookupColor(isHilited ? kColorRamp_Gray_7 : kColorRamp_Gray_A));
			sDrawSide_TopLeft(localDC, inBounds, 0, 0, 0, 0);
			if (bevelWidth == 3)
				localDC.SetInk(sLookupColor(isHilited ? kColorRamp_Gray_9 : kColorRamp_Gray_8));
			else
				localDC.SetInk(sLookupColor(isHilited ? kColorRamp_Gray_9 : kColorRamp_Gray_9));
			sDrawSide_BottomRight(localDC, inBounds, 1, 1, 0, 0);
			if (isHilited)
				{
				if (bevelWidth != 2)
					{
					localDC.Pixel(inBounds.right - 1, inBounds.top, sLookupColor(kColorRamp_Gray_8));
					localDC.Pixel(inBounds.left, inBounds.bottom - 1, sLookupColor(kColorRamp_Gray_8));
					}
				}
			else
				{
				if (bevelWidth == 3)
					{
					localDC.Pixel(inBounds.right - 1, inBounds.top, sLookupColor(kColorRamp_Gray_9));
					localDC.Pixel(inBounds.left, inBounds.bottom - 1, sLookupColor(kColorRamp_Gray_9));
					}
				}
			localDC.SetInk(sLookupColor(isHilited ? kColorRamp_Gray_B : kColorRamp_Gray_D));
			localDC.Fill(inBounds.Inset(1,1));
			}
		}
	}

void ZUIRenderer_ButtonBevel_Platinum::sRenderBevelPopup(const ZDC& inDC, short inBevelWidth, const ZRect& inBounds,
						bool inPopupRightFacing, bool inPopupCentred, ZCoord inPopupBaseLength, const ZUIDisplayState& inState)
	{
// Figure out the arrow's dimensions
	ZCoord theTriangleHeight = (inPopupBaseLength + 1)/2;
	ZCoord theWidth = inPopupRightFacing ? inPopupBaseLength : theTriangleHeight;
	ZCoord theHeight = inPopupRightFacing ? theTriangleHeight : inPopupBaseLength;

	ZCoord hOrigin = inBounds.right - theWidth - inBevelWidth - 2;
	ZCoord vOrigin;
	if (inPopupCentred)
		vOrigin = inBounds.top + (inBounds.Height() - theHeight) / 2;
	else
		vOrigin = inBounds.bottom - theHeight - inBevelWidth - 2;

	ZRGBColor theColor;
	bool useGrayPattern = false;
	if (inState.GetEnabled() && inState.GetActive())
		theColor = sLookupColor(kColorRamp_Black);
	else
		{
		if (inDC.GetDepth() >= 4)
			theColor = sLookupColor(kColorRamp_Gray_7);
		else
			{
			theColor = sLookupColor(kColorRamp_Black);
			useGrayPattern = true;
			}
		}
	sDrawTriangle(inDC, theColor, hOrigin, vOrigin, inPopupBaseLength, true, !inPopupRightFacing, false, useGrayPattern);
	}

ZPoint ZUIRenderer_ButtonBevel_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{
	ZCoord bevelWidth;
	if (!inButton->GetAttribute("ZUIButton::BevelWidth", nil, &bevelWidth))
		bevelWidth = 1;
	bevelWidth = max((ZCoord)1, min(bevelWidth, (ZCoord)3));

	ZPoint theSize = ZPoint::sZero;
	if (inCaption)
		theSize = inCaption->GetSize();

// Account for the bevel and the one pixel border
	theSize += ZPoint(bevelWidth*2 + 2,bevelWidth*2 + 2);

	return theSize;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonDisclosure_Platinum

ZUIRenderer_ButtonDisclosure_Platinum::ZUIRenderer_ButtonDisclosure_Platinum()
	{}

ZUIRenderer_ButtonDisclosure_Platinum::~ZUIRenderer_ButtonDisclosure_Platinum()
	{}

void ZUIRenderer_ButtonDisclosure_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonDisclosure_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{ sRenderDisclosure(inDC, inBounds.TopLeft(), inState); }

enum
	{
	kDisclosure_Width = 12,
	kDisclosure_Height = 12
	};

static const char* sPixelVals_Disclosure[] =
	{
// kDisclosure_LeftIntermediate
		"            "
		"  0         "
		"  00        "
		"  0S0       "
		"  0SS0      "
		"  0SSS0     "
		"  0SSSS0    "
		"  0SSSSS0   "
		"  0SSSSSS0  "
		"  000000000 "
		"            "
		"            "
		,
// kDisclosure_LeftIntermediateBW
		"            "
		"  0         "
		"  00        "
		"  000       "
		"  0000      "
		"  00000     "
		"  000000    "
		"  0000000   "
		"  00000000  "
		"  000000000 "
		"            "
		"            "
		,
// kDisclosure_RightIntermediate
		"            "
		"         0  "
		"        00  "
		"       0S0  "
		"      0SS0  "
		"     0SSS0  "
		"    0SSSS0  "
		"   0SSSSS0  "
		"  0SSSSSS0  "
		" 000000000  "
		"            "
		"            "
		,
// kDisclosure_RightIntermediateBW
		"            "
		"         0  "
		"        00  "
		"       000  "
		"      0000  "
		"     00000  "
		"    000000  "
		"   0000000  "
		"  00000000  "
		" 000000000  "
		"            "
		"            "
		,
// kDisclosure_Left
		"       0    "
		"      008   "
		"     0P08   "
		"    0QP08   "
		"   0QQP08   "
		"  0RQQP08   "
		"   0RQP08   "
		"    0RP08   "
		"     0Q08   "
		"      008   "
		"       08   "
		"        8   "
		,
// kDisclosure_Right
		"    0       "
		"    00      "
		"    0P0     "
		"    0PQ0    "
		"    0PQQ0   "
		"    0PQQR0  "
		"    0PQR08B "
		"    0PR08B  "
		"    0Q08B   "
		"    008B    "
		"    08B     "
		"     B      "
		,
// kDisclosure_Down
		"            "
		"            "
		"            "
		"00000000000 "
		" 0PPPPPPQ084"
		"  0QQQQR08B "
		"   0QQR08B  "
		"    0R08B   "
		"     08B    "
		"      B     "
		"            "
		"            "
		,
// kDisclosure_LeftPressed
		"       0    "
		"      00    "
		"     0S0    "
		"    0SS0    "
		"   0SSS0    "
		"  0SSSS0    "
		"   0SSS0    "
		"    0SS0    "
		"     0S0    "
		"      00    "
		"       0    "
		"            "
		,
// kDisclosure_RightPressed
		"    0       "
		"    00      "
		"    0S0     "
		"    0SS0    "
		"    0SSS0   "
		"    0SSSS0  "
		"    0SSS0   "
		"    0SS0    "
		"    0S0     "
		"    00      "
		"    0       "
		"            "
		,
// kDisclosure_DownPressed
		"            "
		"            "
		"            "
		"00000000000 "
		" 0SSSSSSS0  "
		"  0SSSSS0   "
		"   0SSS0    "
		"    0S0     "
		"     0      "
		"            "
		"            "
		"            "
		,
// kDisclosure_LeftDisabled
		"       A    "
		"      AA    "
		"     AAA    "
		"    AAAA    "
		"   AAAAA    "
		"  AAAAAA    "
		"   AAAAA    "
		"    AAAA    "
		"     AAA    "
		"      AA    "
		"       A    "
		"            "
		,
// kDisclosure_RightDisabled
		"    A       "
		"    AA      "
		"    AAA     "
		"    AAAA    "
		"    AAAAA   "
		"    AAAAAA  "
		"    AAAAA   "
		"    AAAA    "
		"    AAA     "
		"    AA      "
		"    A       "
		"            "
		,
// kDisclosure_DownDisabled
		"            "
		"            "
		"            "
		"AAAAAAAAAAA "
		" AAAAAAAAA  "
		"  AAAAAAA   "
		"   AAAAA    "
		"    AAA     "
		"     A      "
		"            "
		"            "
		"            "
		,
// kDisclosure_LeftBW
		"       0    "
		"      00    "
		"     0_0    "
		"    0__0    "
		"   0___0    "
		"  0____0    "
		"   0___0    "
		"    0__0    "
		"     0_0    "
		"      00    "
		"       0    "
		"            "
		,
// kDisclosure_RightBW
		"    0       "
		"    00      "
		"    0_0     "
		"    0__0    "
		"    0___0   "
		"    0____0  "
		"    0___0   "
		"    0__0    "
		"    0_0     "
		"    00      "
		"    0       "
		"            "
		,
// kDisclosure_DownBW
		"            "
		"            "
		"            "
		"00000000000 "
		" 0_______0  "
		"  0_____0   "
		"   0___0    "
		"    0_0     "
		"     0      "
		"            "
		"            "
		"            "
		,
// kDisclosure_LeftPressedBW
		"       0    "
		"      00    "
		"     000    "
		"    0000    "
		"   00000    "
		"  000000    "
		"   00000    "
		"    0000    "
		"     000    "
		"      00    "
		"       0    "
		"            "
		,
// kDisclosure_RightPressedBW
		"    0       "
		"    00      "
		"    000     "
		"    0000    "
		"    00000   "
		"    000000  "
		"    00000   "
		"    0000    "
		"    000     "
		"    00      "
		"    0       "
		"            "
		,
// kDisclosure_DownPressedBW
		"            "
		"            "
		"            "
		"00000000000 "
		" 000000000  "
		"  0000000   "
		"   00000    "
		"    000     "
		"     0      "
		"            "
		"            "
		"            "
		,
// kDisclosure_LeftDisabledBW
		"       0    "
		"      0_    "
		"     0_0    "
		"    0___    "
		"   0___0    "
		"  0_____    "
		"   0___0    "
		"    0___    "
		"     0_0    "
		"      0_    "
		"       0    "
		"            "
		,
// kDisclosure_RightDisabledBW
		"    0       "
		"    _0      "
		"    0_0     "
		"    ___0    "
		"    0___0   "
		"    _____0  "
		"    0___0   "
		"    ___0    "
		"    0_0     "
		"    _0      "
		"    0       "
		"            "
		,
// kDisclosure_DownDisabledBW
		"            "
		"            "
		"            "
		"0_0_0_0_0_0 "
		" 0_______0  "
		"  0_____0   "
		"   0___0    "
		"    0_0     "
		"     0      "
		"            "
		"            "
		"            "
	};

enum
	{
	kDisclosure_LeftIntermediate = 0,
	kDisclosure_LeftIntermediateBW,
	kDisclosure_RightIntermediate,
	kDisclosure_RightIntermediateBW,
	kDisclosure_Left,
	kDisclosure_Right,
	kDisclosure_Down,
	kDisclosure_LeftPressed,
	kDisclosure_RightPressed,
	kDisclosure_DownPressed,
	kDisclosure_LeftDisabled,
	kDisclosure_RightDisabled,
	kDisclosure_DownDisabled,
	kDisclosure_LeftBW,
	kDisclosure_RightBW,
	kDisclosure_DownBW,
	kDisclosure_LeftPressedBW,
	kDisclosure_RightPressedBW,
	kDisclosure_DownPressedBW,
	kDisclosure_LeftDisabledBW,
	kDisclosure_RightDisabledBW,
	kDisclosure_DownDisabledBW
	};

void ZUIRenderer_ButtonDisclosure_Platinum::sRenderDisclosure(const ZDC& inDC, const ZPoint& theLocation, const ZUIDisplayState& inState)
	{
	bool rightFacing = true;
	short depth = inDC.GetDepth();

	short whichOne = 0;
	if (inState.GetHilite() == eZTriState_Mixed)
		{
		if (rightFacing)
			whichOne = kDisclosure_RightIntermediate;
		else
			whichOne = kDisclosure_LeftIntermediate;
		if (depth >= 4)
			whichOne += kDisclosure_RightIntermediateBW - kDisclosure_RightIntermediate;
// We don't draw a disabled intermediate disclosure, nor do we have a pressed version
		}
	else
		{
		if (inState.GetHilite() == eZTriState_On)
			whichOne = kDisclosure_Down;
		else
			{
			if (rightFacing)
				whichOne = kDisclosure_Right;
			else
				whichOne = kDisclosure_Left;
			}

		if (!(inState.GetEnabled() && inState.GetActive()))
			whichOne += kDisclosure_RightDisabled - kDisclosure_Right;
		else if (inState.GetPressed())
			whichOne += kDisclosure_RightPressed - kDisclosure_Right;

		if (depth < 4)
			whichOne += kDisclosure_RightBW - kDisclosure_Right;
		}

	inDC.Pixels(theLocation, ZPoint(kDisclosure_Width, kDisclosure_Height), sPixelVals_Disclosure[whichOne], sColorMap, sColorMapSize);
	}

ZPoint ZUIRenderer_ButtonDisclosure_Platinum::GetPreferredDimensions(ZUIButton_Rendered* inButton, ZRef<ZUICaption> inCaption)
	{ return ZPoint(kDisclosure_Width, kDisclosure_Height); }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ButtonPlacard_Platinum

ZUIRenderer_ButtonPlacard_Platinum::ZUIRenderer_ButtonPlacard_Platinum()
	{}

ZUIRenderer_ButtonPlacard_Platinum::~ZUIRenderer_ButtonPlacard_Platinum()
	{}

void ZUIRenderer_ButtonPlacard_Platinum::Activate(ZUIButton_Rendered* inButton)
	{ inButton->Refresh(); }

void ZUIRenderer_ButtonPlacard_Platinum::RenderButton(ZUIButton_Rendered* inButton, const ZDC& inDC, const ZRect& inBounds, ZRef<ZUICaption> inCaption, const ZUIDisplayState& inState)
	{ sRender_Placard(inDC, inBounds, inState.GetEnabled() && inState.GetActive()); }

void ZUIRenderer_ButtonPlacard_Platinum::sRender_Placard(const ZDC& inDC, const ZRect& theRect, bool enabled)
	{
	ZDC localDC(inDC);

// BORDER
// The border is always drawn in black
	if (enabled || localDC.GetDepth() < 4)
		localDC.SetInk(sLookupColor(kColorRamp_Black));
	else
		localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
	localDC.Frame(theRect);

// FACE
	if (localDC.GetDepth() < 4)
		{
// BLACK & WHITE
		localDC.SetInk(sLookupColor(kColorRamp_White));
		localDC.Fill(theRect.Inset(1,1));
		}
	else
		{
// COLOR
// The face is painted with the face color if enabled
// and active, otherwise it is painted with the
// standard AGA color gray1
		if (enabled)
			localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
		else
			localDC.SetInk(sLookupColor(kColorRamp_Gray_E));
		localDC.Fill(theRect.Inset(1,1));

// The shadows are only drawn if we are enabled and active
		if (enabled)
			{
			localDC.SetInk(sLookupColor(kColorRamp_White));
			sDrawSide_TopLeft(localDC, theRect.Inset(1,1), 0, 0, 1, 1);
			localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			sDrawSide_BottomRight(localDC, theRect.Inset(1,1), 1, 1, 0, 0);
			}
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ScrollBar_Platinum

static const ZCoord kIndicatorPixelInset = 4;
static const ZCoord kMinThumbLength = 16;

ZUIRenderer_ScrollBar_Platinum::ZUIRenderer_ScrollBar_Platinum()
	{}

ZUIRenderer_ScrollBar_Platinum::~ZUIRenderer_ScrollBar_Platinum()
	{}

void ZUIRenderer_ScrollBar_Platinum::Activate(ZUIScrollBar_Rendered* inScrollBar)
	{ inScrollBar->Refresh(); }

ZCoord ZUIRenderer_ScrollBar_Platinum::GetPreferredThickness(ZUIScrollBar_Rendered* inScrollBar)
	{
	if (inScrollBar && inScrollBar->GetWithBorder())
		return 16;
	return 15;
	}

void ZUIRenderer_ScrollBar_Platinum::RenderScrollBar(ZUIScrollBar_Rendered* inScrollBar, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, bool inWithBorder, double inValue, double inThumbProportion, double inGhostThumbValue, ZUIScrollBar::HitPart inHitPart)
	{
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZRect localBounds(inBounds);
//	ZDC_OffAuto localDC(inDC, false);
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

	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	sCalcAllRects(localBounds, horizontal, inValue, inThumbProportion, thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect);

	if (inEnabled && inActive)
		{
		bool slideable = inThumbProportion < 1.0;

// Do the arrows first
		sRenderArrow(localDC, upArrowRect, horizontal, false, inHitPart == ZUIScrollBar::hitPartUpArrowOuter, slideable);
		sRenderArrow(localDC, downArrowRect, horizontal, true, inHitPart == ZUIScrollBar::hitPartDownArrowOuter, slideable);
// Now clip down to the track rectangle
		localDC.SetClip(localDC.GetClip() & trackRect);
		if (slideable)
			{
			if (thumbRect.IsEmpty())
				{
				sRenderTrackNotSlideable(localDC, trackRect, horizontal);
				}
			else
				{
				sRenderTrackSlideable(localDC, pageUpRect, horizontal);
				sRenderTrackSlideable(localDC, pageDownRect, horizontal);
				if (inGhostThumbValue >= 0.0 && inGhostThumbValue <= 1.0)
					{
					ZRect dummyRect, ghostThumbRect;
					sCalcAllRects(localBounds, horizontal, inGhostThumbValue, inThumbProportion, ghostThumbRect, dummyRect, dummyRect, dummyRect, dummyRect, dummyRect);
					sRenderGhostThumb(localDC, ghostThumbRect, horizontal);
					}
				sRenderThumb(localDC, thumbRect, horizontal, inHitPart == ZUIScrollBar::hitPartThumb);
				}
			}
		else
			{
			sRenderTrackNotSlideable(localDC, trackRect, horizontal);
			}
		}
	else
		{
		sRenderScrollBarDisabled(localDC, localBounds, horizontal);
		}
	}

ZUIScrollBar::HitPart ZUIRenderer_ScrollBar_Platinum::FindHitPart(ZUIScrollBar_Rendered* inScrollBar, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled, bool inWithBorder, double inValue, double inThumbProportion)
	{
	if (inThumbProportion >= 1.0)
		return ZUIScrollBar::hitPartNone;
	ZRect localBounds(inBounds);
	bool horizontal = (inBounds.Width() > inBounds.Height());
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
	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	sCalcAllRects(localBounds, horizontal, inValue, inThumbProportion, thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect);
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

double ZUIRenderer_ScrollBar_Platinum::GetDelta(ZUIScrollBar_Rendered* inScrollBar, const ZRect& inBounds, bool inWithBorder, double inValue, double inThumbProportion, ZPoint inPixelDelta)
	{
	ZRect localBounds(inBounds);
	bool horizontal = (inBounds.Width() > inBounds.Height());
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
	ZRect trackRect, thumbRect, dummyRect;
	sCalcAllRects(localBounds, horizontal, inValue, inThumbProportion, thumbRect, trackRect, dummyRect, dummyRect, dummyRect, dummyRect);
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

void ZUIRenderer_ScrollBar_Platinum::sCalcAllRects(const ZRect& inBounds, bool inHorizontal, double inThumbValue, double inThumbProportion,
										ZRect& outThumbRect, ZRect& outTrackRect,
										ZRect& outUpArrowRect, ZRect& outDownArrowRect,
										ZRect& outPageUpRect, ZRect& outPageDownRect)
	{
// We have to use locals, then copy them back to the parameters, otherwise things break when the
// same rect is referenced by more than one parameter
	ZRect thumbRect, trackRect, upArrowRect, downArrowRect, pageUpRect, pageDownRect;
	upArrowRect = inBounds;
	downArrowRect = inBounds;
	pageUpRect = inBounds;
	pageDownRect = inBounds;
	bool horizontal = inBounds.Width() > inBounds.Height();
	ZCoord arrowSize = horizontal ? inBounds.Height() : inBounds.Width();
	ZCoord trackLength, trackOrigin;
	if (horizontal)
		{
		upArrowRect.right = upArrowRect.left + arrowSize;
		downArrowRect.left = downArrowRect.right - arrowSize;
		trackRect = inBounds.Inset(arrowSize, 0);
		trackLength = trackRect.Width();
		trackOrigin = trackRect.Left();
		}
	else
		{
		upArrowRect.bottom = upArrowRect.top + arrowSize;
		downArrowRect.top = downArrowRect.bottom - arrowSize;
		trackRect = inBounds.Inset(0, arrowSize);
		trackLength = trackRect.Height();
		trackOrigin = trackRect.Top();
		}

// How big should the thumb be?
	ZCoord thumbLength = ZCoord(trackLength * inThumbProportion);
	if (thumbLength < kMinThumbLength)
		thumbLength = kMinThumbLength;

// We subtract 1 and and add 2 in this calc to allow the black edges of the thumb to overlap the track ends
	ZCoord thumbOrigin = trackOrigin - 1 + ZCoord((trackLength - thumbLength + 2) * inThumbValue);

// Skip drawing the thumb if there's no room on the track
	if (thumbLength <= trackLength)
		{
		if (horizontal)
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

void ZUIRenderer_ScrollBar_Platinum::sRenderTrackSlideable(ZDC& inDC, const ZRect& inTrackBounds, bool inHorizontal)
	{
	ZRect r(inTrackBounds);
	if (inDC.GetDepth() >= 4)
		{
		if (inHorizontal)
			{
			inDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			inDC.Fill(ZRect(r.left + 2, r.top + 3, r.right, r.bottom - 3));

			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.right - 1, r.top);
			inDC.Line(r.left, r.bottom - 1, r.right - 1, r.bottom - 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_7));
			inDC.Line(r.left, r.bottom - 3, r.left, r.top + 1);
			inDC.Line(r.left, r.top + 1, r.right - 1, r.top + 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_8));
			inDC.Line(r.left + 1, r.bottom - 4, r.left + 1, r.top + 2);
			inDC.Line(r.left + 1, r.top + 2, r.right - 1, r.top + 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_C));
			inDC.Line(r.left, r.bottom - 2, r.right - 1, r.bottom - 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_B));
			inDC.Line(r.left + 1, r.bottom - 3, r.right - 1, r.bottom - 3);
			}
		else
			{
			inDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			inDC.Fill(ZRect(r.left + 3, r.top + 2, r.right - 3, r.bottom));

			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.left, r.bottom - 1);
			inDC.Line(r.right - 1, r.top, r.right - 1, r.bottom - 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_7));
			inDC.Line(r.left + 1, r.bottom - 1, r.left + 1, r.top);
			inDC.Line(r.left + 1, r.top, r.right - 3, r.top);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_8));
			inDC.Line(r.left + 2, r.bottom - 1, r.left + 2, r.top + 1);
			inDC.Line(r.left + 2, r.top + 1, r.right - 4, r.top + 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_C));
			inDC.Line(r.right - 2, r.top, r.right - 2, r.bottom - 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_B));
			inDC.Line(r.right - 3, r.top + 1, r.right - 3, r.bottom - 1);
			}
		}
	else // 1 bit
		{
		inDC.SetInk(ZRGBColor::sBlack);
		inDC.Frame(r);
		if (inHorizontal)
			{
			inDC.SetInk(ZDCInk::sGray);
			inDC.Fill(r.Inset(0, 1));
			inDC.Line(r.left, r.top + 1, r.left, r.bottom - 2);
			inDC.Line(r.right - 1, r.top + 1, r.right - 1, r.bottom - 2);
			}
		else
			{
			inDC.SetInk(ZDCInk::sGray);
			inDC.Fill(r.Inset(1, 0));
			inDC.Line(r.left + 1, r.top, r.right - 2, r.top);
			inDC.Line(r.left + 1, r.bottom - 1, r.right - 2, r.bottom - 1);
			}
		}
	}

void ZUIRenderer_ScrollBar_Platinum::sRenderTrackNotSlideable(ZDC& inDC, const ZRect& inTrackBounds, bool inHorizontal)
	{
	ZRect r(inTrackBounds);
	if (inDC.GetDepth() >= 4)
		{
		inDC.SetInk(sLookupColor(kColorRamp_Gray_E));
		if (inHorizontal)
			{
			inDC.Fill(r.Inset(0, 1));
			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.right - 1, r.top);
			inDC.Line(r.left, r.bottom - 1, r.right - 1, r.bottom - 1);
			}
		else
			{
			inDC.Fill(r.Inset(1, 0));
			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.left, r.bottom - 1);
			inDC.Line(r.right - 1, r.top, r.right - 1, r.bottom - 1);
			}
		}
	else // 1 bit
		{
		inDC.SetInk(ZDCInk::sWhite);
		if (inHorizontal)
			{
			inDC.Fill(r.Inset(0, 1));
			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.right - 1, r.top);
			inDC.Line(r.left, r.bottom - 1, r.right - 1, r.bottom - 1);
			}
		else
			{
			inDC.Fill(r.Inset(1, 0));
			inDC.SetInk(sLookupColor(kColorRamp_Black));
			inDC.Line(r.left, r.top, r.left, r.bottom - 1);
			inDC.Line(r.right - 1, r.top, r.right - 1, r.bottom - 1);
			}
		}
	}

void ZUIRenderer_ScrollBar_Platinum::sRenderScrollBarDisabled(ZDC& inDC, const ZRect& inBounds, bool inHorizontal)
	{
	ZRect r(inBounds);
	if (inDC.GetDepth() >= 4)
		{
		inDC.SetInk(sLookupColor(kColorRamp_Gray_E));
		inDC.Fill(r.Inset(1,1));
		inDC.SetInk(sLookupColor(kColorRamp_Gray_5));
		inDC.Frame(r);
		}
	else // 1 bit
		{
		inDC.SetInk(ZDCInk::sWhite);
		inDC.Fill(r.Inset(1,1));
		inDC.SetInk(ZDCInk::sBlack);
		inDC.Frame(r);
		}
	}

void ZUIRenderer_ScrollBar_Platinum::sRenderGhostThumb(ZDC& inDC, const ZRect& inThumbBounds, bool inHorizontal)
	{
	ZRect r(inThumbBounds);
	if (inDC.GetDepth() >= 4)
		{
		if (inHorizontal)
			{
			inDC.SetInk(sLookupColor(kColorRamp_Gray_B));
			inDC.Fill(r.Inset(1,3));

			inDC.Pixel(r.left, r.top + 1, sLookupColor(kColorRamp_Gray_4));
			inDC.Pixel(r.right - 1, r.top + 1, sLookupColor(kColorRamp_Gray_4));

			inDC.Pixel(r.left, r.top + 2, sLookupColor(kColorRamp_Gray_5));
			inDC.Pixel(r.right - 1, r.top + 2, sLookupColor(kColorRamp_Gray_5));

			inDC.SetInk(sLookupColor(kColorRamp_Gray_7));
			inDC.Line(r.left, r.top + 3, r.left, r.bottom - 4);
			inDC.Line(r.right - 1, r.top + 3, r.right - 1, r.bottom - 4);

			inDC.Pixel(r.left, r.bottom - 3, sLookupColor(kColorRamp_Gray_9));
			inDC.Pixel(r.right - 1, r.bottom - 3, sLookupColor(kColorRamp_Gray_9));

			inDC.Pixel(r.left, r.bottom - 2, sLookupColor(kColorRamp_Gray_B));
			inDC.Pixel(r.right - 1, r.bottom - 2, sLookupColor(kColorRamp_Gray_B));

			inDC.SetInk(sLookupColor(kColorRamp_Gray_9));
			inDC.Line(r.left + 1, r.top + 1, r.right - 2, r.top + 1);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			inDC.Line(r.left + 1, r.top + 2, r.right - 2, r.top + 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_C));
			inDC.Line(r.left + 1, r.bottom - 3, r.right - 2, r.bottom - 3);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_D));
			inDC.Line(r.left + 1, r.bottom - 2, r.right - 2, r.bottom - 2);
			}
		else
			{
			inDC.SetInk(sLookupColor(kColorRamp_Gray_B));
			inDC.Fill(r.Inset(3,1));

			inDC.Pixel(r.left + 1, r.top, sLookupColor(kColorRamp_Gray_4));
			inDC.Pixel(r.left + 1, r.bottom - 1, sLookupColor(kColorRamp_Gray_4));

			inDC.Pixel(r.left + 2, r.top, sLookupColor(kColorRamp_Gray_5));
			inDC.Pixel(r.left + 2, r.bottom - 1, sLookupColor(kColorRamp_Gray_5));

			inDC.SetInk(sLookupColor(kColorRamp_Gray_7));
			inDC.Line(r.left + 3, r.top, r.right - 4, r.top);
			inDC.Line(r.left + 3, r.bottom - 1, r.right - 4, r.bottom - 1);

			inDC.Pixel(r.right - 3, r.top, sLookupColor(kColorRamp_Gray_9));
			inDC.Pixel(r.right - 3, r.bottom - 1, sLookupColor(kColorRamp_Gray_9));

			inDC.Pixel(r.right - 2, r.top, sLookupColor(kColorRamp_Gray_B));
			inDC.Pixel(r.right - 2, r.bottom - 1, sLookupColor(kColorRamp_Gray_B));

			inDC.SetInk(sLookupColor(kColorRamp_Gray_9));
			inDC.Line(r.left + 1, r.top + 1, r.left + 1, r.bottom - 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			inDC.Line(r.left + 2, r.top + 1, r.left + 2, r.bottom - 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_C));
			inDC.Line(r.right - 3, r.top + 1, r.right - 3, r.bottom - 2);

			inDC.SetInk(sLookupColor(kColorRamp_Gray_D));
			inDC.Line(r.right - 2, r.top + 1, r.right - 2, r.bottom - 2);
			}
// Do the tickmarks
		ZPoint moveDir = ZPoint::sZero;
		ZPoint drawLen = ZPoint::sZero;
		ZPoint drawLenShort = ZPoint::sZero;
		if (inHorizontal)
			{
			moveDir.h = 1;
			drawLen.v = r.Height() - 10;
			drawLenShort.v = drawLen.v - 1;
			}
		else
			{
			moveDir.v = 1;
			drawLen.h = r.Width() - 10;
			drawLenShort.h = drawLen.h - 1;
			}
		ZPoint drawDir = moveDir.Flipped();
		ZPoint currPt(r.left + kIndicatorPixelInset + (moveDir.h*(r.Width()/2 - 8)),
							r.top + kIndicatorPixelInset + (moveDir.v*(r.Height()/2 - 8)));
		for (long x = 0; x < 4; ++x)
			{
			inDC.SetInk(sLookupColor(kColorRamp_Gray_D));
			inDC.Line(currPt.h, currPt.v, currPt.h + drawDir.h + drawLenShort.h, currPt.v + drawDir.v + drawLenShort.v);
			inDC.SetInk(sLookupColor(kColorRamp_Gray_9));
			inDC.Line(currPt.h + drawDir.h + moveDir.h, currPt.v + drawDir.v + moveDir.v, currPt.h + drawDir.h + moveDir.h + drawLen.h, currPt.v + drawDir.v + moveDir.v + drawLen.v);
			currPt += moveDir*2;
			}
		}
	else
		{
		inDC.PenNormal();
		inDC.Frame(r);
		}
	}

void ZUIRenderer_ScrollBar_Platinum::sRenderThumb(ZDC& inDC, const ZRect& inThumbBounds, bool inHorizontal, bool inPressed)
	{
	ZRect r(inThumbBounds);
	enum { Fill, Light, TL, BR, Dark, kIndicatorColors };
	char colorIndexes[kIndicatorColors];
	if (inDC.GetDepth() >= 4)
		{
		if (inPressed)
			{
			colorIndexes[Fill] = kColorRamp_Gray_7;
			colorIndexes[Light] = kColorRamp_Gray_C;
			colorIndexes[TL] = kColorRamp_Gray_A;
			colorIndexes[BR] = kColorRamp_Gray_5;
			colorIndexes[Dark] = kColorRamp_Gray_2;
			}
		else
			{
			colorIndexes[Fill] = kColorRamp_Gray_A;
			colorIndexes[Light] = kColorRamp_Gray_E;
			colorIndexes[TL] = kColorRamp_Gray_C;
			colorIndexes[BR] = kColorRamp_Gray_7;
			colorIndexes[Dark] = kColorRamp_Gray_5;
			}
		}
	else
		{
		if (inPressed)
			{
			colorIndexes[Fill] = kColorRamp_Black;
			colorIndexes[Light] = kColorRamp_Black;
			colorIndexes[TL] = kColorRamp_Black;
			colorIndexes[BR] = kColorRamp_Black;
			colorIndexes[Dark] = kColorRamp_White;
			}
		else
			{
			colorIndexes[Fill] = kColorRamp_White;
			colorIndexes[Light] = kColorRamp_White;
			colorIndexes[TL] = kColorRamp_White;
			colorIndexes[BR] = kColorRamp_White;
			colorIndexes[Dark] = kColorRamp_Black;
			}
		}

// Draw the thumb background
	inDC.SetInk(sLookupColor(kColorRamp_Black));
	inDC.Frame(r);
	if (inDC.GetDepth() >= 4)
		{
		inDC.SetInk(sLookupColor(colorIndexes[Fill]));
		inDC.Fill(r.Inset(2,2));

		inDC.Pixel(r.left + 1, r.bottom - 2, sLookupColor(colorIndexes[Fill]));
		inDC.Pixel(r.right - 2, r.top + 1, sLookupColor(colorIndexes[Fill]));

		inDC.SetInk(sLookupColor(colorIndexes[TL]));
		inDC.Line(r.left + 1, r.bottom - 3, r.left + 1, r.top + 2);
		inDC.Line(r.left + 2, r.top + 1, r.right - 3, r.top + 1);

		inDC.SetInk(sLookupColor(colorIndexes[BR]));
		inDC.Line(r.right - 2, r.top + 2, r.right - 2, r.bottom - 2);
		inDC.Line(r.right - 2, r.bottom - 2, r.left + 2, r.bottom - 2);

		inDC.Pixel(r.left + 1, r.top + 1, sLookupColor(colorIndexes[Light]));
		}
	else
		{
		inDC.SetInk(sLookupColor(colorIndexes[Fill]));
		inDC.Fill(r.Inset(1,1));
		}

// Do the tickmarks
	ZPoint moveDir = ZPoint::sZero;
	ZPoint drawLen = ZPoint::sZero;
	ZPoint drawLenShort = ZPoint::sZero;
	if (inHorizontal)
		{
		moveDir.h = 1;
		drawLen.v = r.Height() - 10;
		drawLenShort.v = drawLen.v - 1;
		}
	else
		{
		moveDir.v = 1;
		drawLen.h = r.Width() - 10;
		drawLenShort.h = drawLen.h - 1;
		}
	ZPoint drawDir = moveDir.Flipped();
	ZPoint currPt(r.left + kIndicatorPixelInset + (moveDir.h*(r.Width()/2 - 8)),
						r.top + kIndicatorPixelInset + (moveDir.v*(r.Height()/2 - 8)));
	for (long x = 0; x < 4; ++x)
		{
		inDC.Pixel(currPt.h, currPt.v, sLookupColor(colorIndexes[Light]));
		inDC.SetInk(sLookupColor(colorIndexes[TL]));
		inDC.Line(currPt.h + drawDir.h, currPt.v + drawDir.v, currPt.h + drawDir.h + drawLenShort.h, currPt.v + drawDir.v + drawLenShort.v);
		inDC.SetInk(sLookupColor(colorIndexes[Dark]));
		inDC.Line(currPt.h + drawDir.h + moveDir.h, currPt.v + drawDir.v + moveDir.v, currPt.h + drawDir.h + moveDir.h + drawLen.h, currPt.v + drawDir.v + moveDir.v + drawLen.v);
		currPt += moveDir*2;
		}

	}

void ZUIRenderer_ScrollBar_Platinum::sRenderArrow(ZDC& inDC, const ZRect& inArrowBounds, bool inHorizontal, bool inDownArrow, bool inPressed, bool inSlideable)
	{
	ZRect r(inArrowBounds);

	enum { Divider, Fill, TL, BR, Arrow, kNumArrowColors };
	char colorIndexes[kNumArrowColors];

	if (inDC.GetDepth() >= 4)
		{
		if (inPressed)
			{
			colorIndexes[Divider] = kColorRamp_Black;
			colorIndexes[Fill] = kColorRamp_Gray_7;
			colorIndexes[TL] = kColorRamp_Gray_5;
			colorIndexes[BR] = kColorRamp_Gray_9;
			colorIndexes[Arrow] = kColorRamp_Black;
			}
		else if (inSlideable)
			{
			colorIndexes[Divider] = kColorRamp_Black;
			colorIndexes[Fill] = kColorRamp_Gray_D;
			colorIndexes[TL] = kColorRamp_White;
			colorIndexes[BR] = kColorRamp_Gray_A;
			colorIndexes[Arrow] = kColorRamp_Black;
			}
		else	// not slideable
			{
			colorIndexes[Divider] = kColorRamp_Gray_5;
			colorIndexes[Fill] = kColorRamp_Gray_E;
			colorIndexes[TL] = kColorRamp_Gray_E;
			colorIndexes[BR] = kColorRamp_Gray_E;
			colorIndexes[Arrow] = kColorRamp_Gray_8;
			}
		}
	else
		{
		if (inPressed)
			{
			colorIndexes[Divider] = kColorRamp_Black;
			colorIndexes[Fill] = kColorRamp_Black;
			colorIndexes[TL] = kColorRamp_Black;
			colorIndexes[BR] = kColorRamp_Black;
			colorIndexes[Arrow] = kColorRamp_White;
			}
		else // unpressed + slideable
			{
			colorIndexes[Divider] = kColorRamp_Black;
			colorIndexes[Fill] = kColorRamp_White;
			colorIndexes[TL] = kColorRamp_White;
			colorIndexes[BR] = kColorRamp_White;
			colorIndexes[Arrow] = kColorRamp_Black;
			}
		}

// Left border
	if (inHorizontal && inDownArrow)
		inDC.SetInk(sLookupColor(colorIndexes[Divider]));
	else
		inDC.SetInk(sLookupColor(kColorRamp_Black));
	inDC.Line(r.left, r.top, r.left, r.bottom - 1);

// Right border
	if (inHorizontal && !inDownArrow)
		inDC.SetInk(sLookupColor(colorIndexes[Divider]));
	else
		inDC.SetInk(sLookupColor(kColorRamp_Black));
	inDC.Line(r.right - 1, r.top, r.right - 1, r.bottom - 1);

// Top border
	if (!inHorizontal && inDownArrow)
		inDC.SetInk(sLookupColor(colorIndexes[Divider]));
	else
		inDC.SetInk(sLookupColor(kColorRamp_Black));
	inDC.Line(r.left + 1, r.top, r.right - 2, r.top);

// Bottom border
	if (!inHorizontal && !inDownArrow)
		inDC.SetInk(sLookupColor(colorIndexes[Divider]));
	else
		inDC.SetInk(sLookupColor(kColorRamp_Black));
	inDC.Line(r.left + 1, r.bottom - 1, r.right - 2, r.bottom - 1);

	inDC.SetInk(sLookupColor(colorIndexes[TL]));
	inDC.Line(r.left + 1, r.top + 1, r.left + 1, r.bottom - 3);
	inDC.Line(r.left + 1, r.top + 1, r.right - 3, r.top + 1);

	inDC.SetInk(sLookupColor(colorIndexes[BR]));
	inDC.Line(r.left + 2, r.bottom - 2, r.right - 2, r.bottom - 2);
	inDC.Line(r.right - 2, r.top + 2, r.right - 2, r.bottom - 2);

	inDC.Pixel(r.left + 1, r.bottom - 2, sLookupColor(colorIndexes[Fill]));
	inDC.Pixel(r.right - 2, r.top + 1, sLookupColor(colorIndexes[Fill]));

	inDC.SetInk(sLookupColor(colorIndexes[Fill]));
	inDC.Fill(r.Inset(2,2));

	if (inHorizontal)
		sDrawTriangle(inDC, sLookupColor(colorIndexes[Arrow]), r.left + 6, r.top + r.Height()/2 - 4, 8, true, inDownArrow, inDC.GetDepth() < 4, false);
	else
		sDrawTriangle(inDC, sLookupColor(colorIndexes[Arrow]), r.left + r.Width()/2 - 4, r.top + 6, 8, false, inDownArrow, inDC.GetDepth() < 4, false);
	}

void ZUIRenderer_ScrollBar_Platinum::sRenderArrowDisabled(ZDC& inDC, const ZRect& inArrowBounds, bool inHorizontal, bool inDownArrow)
	{
	ZRect r(inArrowBounds);
	if (inDC.GetDepth() >= 4)
		{
		inDC.SetInk(sLookupColor(kColorRamp_Gray_E));
		inDC.Fill(r.Inset(1,1));
		}
	else
		{
		inDC.SetInk(sLookupColor(kColorRamp_White));
		inDC.Fill(r.Inset(1,1));
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Slider_Platinum

ZUIRenderer_Slider_Platinum::ZUIRenderer_Slider_Platinum()
	{}

ZUIRenderer_Slider_Platinum::~ZUIRenderer_Slider_Platinum()
	{}

void ZUIRenderer_Slider_Platinum::Activate(ZUISlider_Rendered* inSlider)
	{ inSlider->Refresh(); }

// From ZUIRenderer_Slider_Base
void ZUIRenderer_Slider_Platinum::RenderSlider(ZUISlider_Rendered* inSlider, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, double inValue, double inGhostThumbValue, bool inThumbHilited)
	{
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_LittleArrows_Platinum

ZUIRenderer_LittleArrows_Platinum::ZUIRenderer_LittleArrows_Platinum()
	{}

ZUIRenderer_LittleArrows_Platinum::~ZUIRenderer_LittleArrows_Platinum()
	{}

void ZUIRenderer_LittleArrows_Platinum::Activate(ZUILittleArrows_Rendered* inLittleArrows)
	{ inLittleArrows->Refresh(); }

ZPoint ZUIRenderer_LittleArrows_Platinum::GetPreferredSize()
	{
	return ZPoint(13,23);
	}

// From ZUIRenderer_LittleArrows_Base
void ZUIRenderer_LittleArrows_Platinum::RenderLittleArrows(ZUILittleArrows_Rendered* inLittleArrows, const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZUILittleArrows::HitPart inHitPart)
	{
	enum { Border, Fill, TL, BR, Corner, Arrow, kNumColors };
	char colorIndexes[kNumColors];
	char pressedColorIndexes[kNumColors];

	ZDC localDC(inDC);
	if (localDC.GetDepth() >= 4)
		{
		if (inEnabled && inActive)
			{
			colorIndexes[Border] = kColorRamp_Black;
			colorIndexes[Fill] = kColorRamp_Gray_D;
			colorIndexes[TL] = kColorRamp_White;
			colorIndexes[BR] = kColorRamp_Gray_A;
			colorIndexes[Corner] = kColorRamp_Gray_D;
			colorIndexes[Arrow] = kColorRamp_Black;
			}
		else
			{
			colorIndexes[Border] = kColorRamp_Gray_8;
			colorIndexes[Fill] = kColorRamp_Gray_D;
			colorIndexes[TL] = kColorRamp_Gray_D;
			colorIndexes[BR] = kColorRamp_Gray_D;
			colorIndexes[Corner] = kColorRamp_Gray_D;
			colorIndexes[Arrow] = kColorRamp_Gray_8;
			}
//		pressedColorIndexes[Border] = kColorRamp_Black;
		pressedColorIndexes[Fill] = kColorRamp_Gray_7;
		pressedColorIndexes[TL] = kColorRamp_Gray_5;
		pressedColorIndexes[BR] = kColorRamp_Gray_9;
		pressedColorIndexes[Corner] = kColorRamp_Gray_7;
		pressedColorIndexes[Arrow] = kColorRamp_White;
		}
	else
		{
		colorIndexes[Border] = kColorRamp_Black;
		colorIndexes[Fill] = kColorRamp_White;
		colorIndexes[TL] = kColorRamp_White;
		colorIndexes[BR] = kColorRamp_White;
		colorIndexes[Corner] = kColorRamp_White;
		colorIndexes[Arrow] = kColorRamp_Black;
//		pressedColorIndexes[Border] = kColorRamp_Black;
		pressedColorIndexes[Fill] = kColorRamp_Black;
		pressedColorIndexes[TL] = kColorRamp_Black;
		pressedColorIndexes[BR] = kColorRamp_Black;
		pressedColorIndexes[Corner] = kColorRamp_Black;
		pressedColorIndexes[Arrow] = kColorRamp_White;
		}
// Border
	localDC.SetInk(sLookupColor(colorIndexes[Border]));
	localDC.Line(inBounds.left + 1, inBounds.top, inBounds.right - 2, inBounds.top);
	localDC.Line(inBounds.left, inBounds.top + 1, inBounds.left, inBounds.bottom - 2);
	localDC.Line(inBounds.left + 1, inBounds.bottom - 1, inBounds.right - 2, inBounds.bottom - 1);
	localDC.Line(inBounds.right - 1, inBounds.top + 1, inBounds.right - 1, inBounds.bottom - 2);

// Figure out the dividing line and bounds of up and down buttons
	ZCoord divider = inBounds.top + inBounds.Height()/2;
	ZRect upperBounds(inBounds.left + 1, inBounds.top + 1, inBounds.right - 1, divider);
	ZRect lowerBounds(inBounds.left + 1, divider + 1, inBounds.right - 1, inBounds.bottom - 1);

// Line across the middle
	localDC.Line(inBounds.left, divider, inBounds.right - 1, divider);

// Draw the interiors of the two buttons
	for (long x = 0; x < 2; ++x)
		{
		ZRect r(x == 0 ? upperBounds : lowerBounds);
		bool pressed = inEnabled && inActive && ((x == 0 && inHitPart == ZUILittleArrows::hitPartUp) || (x == 1 && inHitPart == ZUILittleArrows::hitPartDown));
// Top left
		localDC.SetInk(sLookupColor(pressed ? pressedColorIndexes[TL] : colorIndexes[TL]));
		localDC.Line(r.left, r.top, r.right - 2, r.top);
		localDC.Line(r.left, r.top, r.left, r.bottom - 2);
// Bottom right
		localDC.SetInk(sLookupColor(pressed ? pressedColorIndexes[BR] : colorIndexes[BR]));
		localDC.Line(r.right - 1, r.top + 1, r.right - 1, r.bottom - 1);
		localDC.Line(r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1);
// Corners
		localDC.Pixel(r.right - 1, r.top, sLookupColor(pressed ? pressedColorIndexes[Corner] : colorIndexes[Corner]));
		localDC.Pixel(r.left, r.bottom - 1, sLookupColor(pressed ? pressedColorIndexes[Corner] : colorIndexes[Corner]));
// Interior
		localDC.SetInk(sLookupColor(pressed ? pressedColorIndexes[Fill] : colorIndexes[Fill]));
		localDC.Fill(r.Inset(1,1));

// And now the arrow
		sDrawTriangle(localDC, sLookupColor(colorIndexes[Arrow]), r.left + 2, r.top + r.Height()/2 - 2, 7, false, x == 1, localDC.GetDepth() < 4, false);
		}
	}

ZUILittleArrows::HitPart ZUIRenderer_LittleArrows_Platinum::FindHitPart(ZUILittleArrows_Rendered* inLittleArrows, ZPoint inHitPoint, const ZRect& inBounds, bool inEnabled)
	{
	if (!inEnabled || !inBounds.Contains(inHitPoint))
		return ZUILittleArrows::hitPartNone;
	if (inHitPoint.v < (inBounds.top + inBounds.Height()/2))
		return ZUILittleArrows::hitPartUp;
	return ZUILittleArrows::hitPartDown;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ProgressBar_Platinum

ZUIRenderer_ProgressBar_Platinum::ZUIRenderer_ProgressBar_Platinum()
	{}

ZUIRenderer_ProgressBar_Platinum::~ZUIRenderer_ProgressBar_Platinum()
	{}

enum
	{
	kProgress_Width = 16,
	kProgress_PatternHeight = 16,
	kProgress_BarHeight = 14,
	kProgress_IndeterNumSteps = kProgress_Width / 2 // 8 "frames" before we repeat
	};

static const char* sPixvals_Progress_Indeterminate =
	{
	"5533333333555555"
	"7775555555577777"
	"AAAA77777777AAAA"
	"DDDDDAAAAAAAADDD"
	"______CCCCCCCC__"
	"DDDDDDDAAAAAAAAD"
	"BBBBBBBB77777777"
	"5999999995555555"
	"3377777777333333"
	"2225555555522222"
	"5533333333555555"
	"7775555555577777"
	"AAAA77777777AAAA"
	"DDDDDAAAAAAAADDD"
	"______CCCCCCCC__"
	"DDDDDDDAAAAAAAA2"
	};

static const char* sPixvals_Progress_IndeterminateDisabled =
	{
	"DDAAAAAAAADDDDDD"
	"DDDAAAAAAAADDDDD"
	"DDDDAAAAAAAADDDD"
	"DDDDDAAAAAAAADDD"
	"DDDDDDAAAAAAAADD"
	"DDDDDDDAAAAAAAAD"
	"DDDDDDDDAAAAAAAA"
	"ADDDDDDDDAAAAAAA"
	"AADDDDDDDDAAAAAA"
	"AAADDDDDDDDAAAAA"

	"AAAADDDDDDDDAAAA"
	"AAAAADDDDDDDDAAA"
	"AAAAAADDDDDDDDAA"
	"AAAAAAADDDDDDDDA"
	"AAAAAAAAAAADDDDD"
	"DAAAAAAAAAAADDDD"
	};

static const char* sPixvals_Progress_IndeterminateBW =
	{
	"__00000000______"
	"___00000000_____"
	"____00000000____"
	"_____00000000___"
	"______00000000__"
	"_______00000000_"
	"________00000000"
	"0________0000000"
	"00________000000"
	"000________00000"

	"0000________0000"
	"00000________000"
	"000000________00"
	"0000000________0"
	"00000000________"
	"_00000000_______"
	};

ZDCPixmap ZUIRenderer_ProgressBar_Platinum::sPixmapNormal(new ZDCPixmapRep(new ZDCPixmapRaster_StaticData(sPixvals_Progress_Indeterminate, kProgress_Width, kProgress_PatternHeight),
																					ZRect(kProgress_Width, kProgress_PatternHeight),
																					sPlatinumPixelDesc));

ZDCPixmap ZUIRenderer_ProgressBar_Platinum::sPixmapDisabled(new ZDCPixmapRep(new ZDCPixmapRaster_StaticData(sPixvals_Progress_IndeterminateDisabled, kProgress_Width, kProgress_PatternHeight),
																					ZRect(kProgress_Width, kProgress_PatternHeight),
																					sPlatinumPixelDesc));

ZDCPixmap ZUIRenderer_ProgressBar_Platinum::sPixmapMono(new ZDCPixmapRep(new ZDCPixmapRaster_StaticData(sPixvals_Progress_IndeterminateBW, kProgress_Width, kProgress_PatternHeight),
																					ZRect(kProgress_Width, kProgress_PatternHeight),
																					sPlatinumPixelDesc));

void ZUIRenderer_ProgressBar_Platinum::Activate(ZUIProgressBar_Rendered* inProgressBar)
	{ inProgressBar->Refresh(); }

long ZUIRenderer_ProgressBar_Platinum::GetFrameCount()
	{ return kProgress_IndeterNumSteps; }

ZCoord ZUIRenderer_ProgressBar_Platinum::GetHeight()
	{ return kProgress_BarHeight; }

void ZUIRenderer_ProgressBar_Platinum::DrawDeterminate(const ZDC& inDC, const ZRect& inBounds, bool enabled, bool inActive, double leftEdge, double rightEdge)
	{
// Draw the border
	ZDC localDC(inDC);
	ZRect theFrame = inBounds;
	theFrame.bottom = theFrame.top + kProgress_BarHeight;
	if (localDC.GetDepth() >= 4)
		{
		if (enabled && inActive)
			{
			localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			sDrawSide_TopLeft(localDC, theFrame, 0,0,1,1);
			localDC.SetInk(sLookupColor(kColorRamp_White));
			sDrawSide_BottomRight(localDC, theFrame, 1,1,0,0);
			}
		else
			{
			localDC.Frame(theFrame);
			}
		localDC.SetInk(sLookupColor(enabled && inActive ? kColorRamp_Black : kColorRamp_Gray_5));
		}
	else
		{
		localDC.Frame(theFrame);
		localDC.SetInk(ZRGBColor::sBlack);
		}

	localDC.Frame(theFrame.Inset(1,1));

	theFrame = theFrame.Inset(2,2);
// Clip down to the interior only
	localDC.SetClip(localDC.GetClip() & theFrame);

	ZCoord leftDivider = (ZCoord)((theFrame.Width())*leftEdge);
	ZCoord rightDivider = theFrame.Width() - ((ZCoord)(theFrame.Width()*rightEdge));
// Get the right edge *past* the end of the rect when we're at 100%
	if (rightEdge >= 1.0)
		rightDivider = -1;

	if (localDC.GetDepth() < 4)
		{
		localDC.SetInk(sLookupColor(kColorRamp_White));
// Left white background
		localDC.Fill(ZRect(theFrame.left, theFrame.top, theFrame.left + leftDivider, theFrame.bottom));
// Right white background
		localDC.Fill(ZRect(theFrame.left + rightDivider, theFrame.top, theFrame.right, theFrame.bottom));
		localDC.SetInk(sLookupColor(kColorRamp_Black));
// The bar itself
		localDC.Fill(ZRect(theFrame.left + leftDivider, theFrame.top, theFrame.left + rightDivider, theFrame.bottom));
		}
	else
		{
// Draw left empty area
		ZRect leftRect(theFrame.left, theFrame.top, theFrame.left + leftDivider, theFrame.bottom);
		ZCoord leftWidth(leftRect.Width());
		ZRect midRect(theFrame.left + leftDivider, theFrame.top, theFrame.right - rightDivider, theFrame.bottom);
		ZCoord midWidth(midRect.Width());
		ZRect rightRect(theFrame.right - rightDivider, theFrame.top, theFrame.right, theFrame.bottom);
		ZCoord rightWidth(rightRect.Width());
		if (enabled && inActive)
			{
			if (leftWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Black));
				localDC.Line(leftRect.right - 1, leftRect.top, leftRect.right - 1, leftRect.bottom - 1);
				}
			if (leftWidth >= 2)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(leftRect.left, leftRect.top, leftRect.left, leftRect.bottom - 1);
				}
			if (leftWidth >= 3)
				{
				localDC.Pixel(leftRect.right - 2, leftRect.top, sLookupColor(kColorRamp_Gray_B));
				localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
				localDC.Line(leftRect.right - 2, leftRect.top + 1, leftRect.right - 2, leftRect.bottom - 1);
				}
			if (leftWidth >=4)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
				localDC.Line(leftRect.left + 1, leftRect.top, leftRect.left + 1, leftRect.bottom - 1);
				}
			if (leftWidth >= 5)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
				localDC.Line(leftRect.left + 2, leftRect.bottom - 1, leftRect.right - 2, leftRect.bottom - 1);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
				localDC.Line(leftRect.left + 2, leftRect.top, leftRect.right - 2, leftRect.top);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
				localDC.Fill(ZRect(leftRect.left + 2, leftRect.top + 1, leftRect.right - 2, leftRect.bottom - 1));
				}
			}
		else
			{
			if (leftWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(leftRect.right - 1, leftRect.top, leftRect.right - 1, leftRect.bottom - 1);
				}
			if (leftWidth >= 2)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_C));
				localDC.Fill(ZRect(leftRect.left, leftRect.top, leftRect.right - 1, leftRect.bottom));
				}
			}
// Draw right empty area
		if (enabled && inActive)
			{
			if (rightWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(rightRect.left, rightRect.top, rightRect.left, rightRect.bottom - 1);
				}
			if (rightWidth >= 2)
				{
				localDC.Pixel(rightRect.right - 1, rightRect.top, sLookupColor(kColorRamp_Gray_B));
				localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
				localDC.Line(rightRect.right - 1, rightRect.top + 1, rightRect.right - 1, rightRect.bottom - 1);
				}
			if (rightWidth >=3)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
				localDC.Line(rightRect.left + 1, rightRect.top, rightRect.left + 1, rightRect.bottom - 1);
				}
			if (rightWidth >= 4)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_D));
				localDC.Line(rightRect.left + 2, rightRect.bottom - 1, rightRect.right - 1, rightRect.bottom - 1);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_8));
				localDC.Line(rightRect.left + 2, rightRect.top, rightRect.right - 2, rightRect.top);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
				localDC.Fill(ZRect(rightRect.left + 2, rightRect.top + 1, rightRect.right - 1, rightRect.bottom - 1));
				}
			}
		else
			{
			if (rightWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(rightRect.left, rightRect.top, rightRect.left, rightRect.bottom - 1);
				}
			if (rightWidth >= 2)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_C));
				localDC.Fill(ZRect(rightRect.left + 1, rightRect.top, rightRect.right, rightRect.bottom));
				}
			}
// And the chunk in the middle
		if (enabled && inActive)
			{
			if (midWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Black));
				localDC.Line(midRect.right - 1, midRect.top, midRect.right - 1, midRect.bottom - 1);
				}
			if (midWidth >= 2)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_7));
				localDC.Line(midRect.left, midRect.top, midRect.left, midRect.bottom - 1);
				}
			if (midWidth >= 3)
				{
				localDC.Pixel(midRect.right - 2, midRect.top, sLookupColor(kColorRamp_Gray_5));
				localDC.SetInk(sLookupColor(kColorRamp_Gray_2));
				localDC.Line(midRect.right - 2, midRect.top + 1, midRect.right - 2, midRect.bottom - 1);
				}
			if (midWidth >= 4)
				{
				localDC.Pixel(midRect.left + 1, midRect.top, sLookupColor(kColorRamp_Gray_7));
				localDC.Pixel(midRect.left + 1, midRect.top + 1, sLookupColor(kColorRamp_Gray_A));
				localDC.Pixel(midRect.left + 1, midRect.top + 2, sLookupColor(kColorRamp_Gray_C));
				localDC.Pixel(midRect.left + 1, midRect.top + 3, sLookupColor(kColorRamp_Gray_E));
				localDC.Pixel(midRect.left + 1, midRect.top + 4, sLookupColor(kColorRamp_Gray_E));
				localDC.Pixel(midRect.left + 1, midRect.top + 5, sLookupColor(kColorRamp_Gray_E));
				localDC.Pixel(midRect.left + 1, midRect.top + 6, sLookupColor(kColorRamp_Gray_C));
				localDC.Pixel(midRect.left + 1, midRect.top + 7, sLookupColor(kColorRamp_Gray_A));
				localDC.Pixel(midRect.left + 1, midRect.top + 8, sLookupColor(kColorRamp_Gray_7));
				localDC.Pixel(midRect.left + 1, midRect.top + 9, sLookupColor(kColorRamp_Gray_5));
				}
			if (midWidth >= 5)
				{
				localDC.Pixel(midRect.right - 3, midRect.top, sLookupColor(kColorRamp_Gray_5));
				localDC.Pixel(midRect.right - 3, midRect.top + 1, sLookupColor(kColorRamp_Gray_7));
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(midRect.right - 3, midRect.top + 2, midRect.right - 3, midRect.top + 8);
				localDC.Pixel(midRect.right - 3, midRect.top + 9, sLookupColor(kColorRamp_Gray_2));
				}
			if (midWidth >= 6)
				{
				localDC.Pixel(midRect.right - 4, midRect.top, sLookupColor(kColorRamp_Gray_5));
				localDC.Pixel(midRect.right - 4, midRect.top + 1, sLookupColor(kColorRamp_Gray_7));
				localDC.Pixel(midRect.right - 4, midRect.top + 2, sLookupColor(kColorRamp_Gray_A));
				localDC.Pixel(midRect.right - 4, midRect.top + 3, sLookupColor(kColorRamp_Gray_C));
				localDC.Pixel(midRect.right - 4, midRect.top + 4, sLookupColor(kColorRamp_Gray_C));
				localDC.Pixel(midRect.right - 4, midRect.top + 5, sLookupColor(kColorRamp_Gray_C));
				localDC.Pixel(midRect.right - 4, midRect.top + 6, sLookupColor(kColorRamp_Gray_A));
				localDC.Pixel(midRect.right - 4, midRect.top + 7, sLookupColor(kColorRamp_Gray_7));
				localDC.Pixel(midRect.right - 4, midRect.top + 8, sLookupColor(kColorRamp_Gray_5));
				localDC.Pixel(midRect.right - 4, midRect.top + 9, sLookupColor(kColorRamp_Gray_2));
				}
			if (midWidth >= 7)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(midRect.left + 2, midRect.top, midRect.right - 5, midRect.top);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_7));
				localDC.Line(midRect.left + 2, midRect.top + 1, midRect.right - 5, midRect.top + 1);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
				localDC.Line(midRect.left + 2, midRect.top + 2, midRect.right - 5, midRect.top + 2);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_C));
				localDC.Line(midRect.left + 2, midRect.top + 3, midRect.right - 5, midRect.top + 3);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_E));
				localDC.Line(midRect.left + 2, midRect.top + 4, midRect.right - 5, midRect.top + 4);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_C));
				localDC.Line(midRect.left + 2, midRect.top + 5, midRect.right - 5, midRect.top + 5);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
				localDC.Line(midRect.left + 2, midRect.top + 6, midRect.right - 5, midRect.top + 6);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_7));
				localDC.Line(midRect.left + 2, midRect.top + 7, midRect.right - 5, midRect.top + 7);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
				localDC.Line(midRect.left + 2, midRect.top + 8, midRect.right - 5, midRect.top + 8);
				localDC.SetInk(sLookupColor(kColorRamp_Gray_2));
				localDC.Line(midRect.left + 2, midRect.top + 9, midRect.right - 5, midRect.top + 9);
				}
			}
		else
			{
			if (midWidth >= 1)
				{
				localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
				localDC.Fill(ZRect(midRect.left, midRect.top, midRect.right, midRect.bottom));
				}
			}
		}
	}

void ZUIRenderer_ProgressBar_Platinum::DrawIndeterminate(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, long frame)
	{
	ZDC localDC(inDC);

// Draw the border
	ZRect theFrame = inBounds;
	theFrame.bottom = theFrame.top + kProgress_BarHeight;

	if (localDC.GetDepth() >= 4)
		{
		if (inEnabled && inActive)
			{
			localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
			sDrawSide_TopLeft(localDC, theFrame, 0,0,1,1);
			localDC.SetInk(sLookupColor(kColorRamp_White));
			sDrawSide_BottomRight(localDC, theFrame, 1,1,0,0);
			}
		else
			localDC.Frame(theFrame);
		localDC.SetInk(sLookupColor(inEnabled && inActive ? kColorRamp_Black : kColorRamp_Gray_5));
		}
	else
		{
		localDC.Frame(theFrame);
		localDC.SetInk(ZRGBColor::sBlack);
		}

	localDC.Frame(theFrame.Inset(1,1));

	theFrame = theFrame.Inset(2,2);

// Clip down to the interior only
	localDC.SetClip(localDC.GetClip() & theFrame);
// And image the barber pole
	if (localDC.GetDepth() < 4)
		localDC.SetInk(ZDCInk(sPixmapMono));
	else
		{
		if (inEnabled && inActive)
			localDC.SetInk(ZDCInk(sPixmapNormal));
		else
			localDC.SetInk(ZDCInk(sPixmapDisabled));
		}
	ZPoint currentOrigin = localDC.GetOrigin();
	localDC.SetPatternOrigin(ZPoint(-2, -2) - currentOrigin);
	localDC.OffsetPatternOrigin(ZPoint(-2*(frame % kProgress_IndeterNumSteps), 0));
	localDC.Fill(ZDCRgn(theFrame));
	}

bigtime_t ZUIRenderer_ProgressBar_Platinum::GetInterFrameDuration()
	{
// Go through all frames twice a second
	return ZTicks::sPerSecond() / (2 * kProgress_IndeterNumSteps);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_ChasingArrows_Platinum

ZUIRenderer_ChasingArrows_Platinum::ZUIRenderer_ChasingArrows_Platinum()
	{}

ZUIRenderer_ChasingArrows_Platinum::~ZUIRenderer_ChasingArrows_Platinum()
	{}

enum
	{
	kChasingArrows_NumFrames = 8 // 8 "frames" before we repeat
	};

enum
	{
	kChasingArrows_Width = 16,
	kChasingArrows_Height = 16
	};

static const char* sPixvals_ChasingArrows[kChasingArrows_NumFrames] =
	{
// frame 0
		"                "
		"    XX          "
		"     XXX        "
		"    XXX    X    "
		"   X  X     X   "
		"   X        X   "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"   X        X   "
		"   X     X  X   "
		"    X    XXX    "
		"        XXX     "
		"          XX    "
		"                "
		,
// frame 1
		"       X        "
		"       XX       "
		"      XXXX      "
		"    XX XX       "
		"   X   X        "
		"   X        X   "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"   X        X   "
		"        X   X   "
		"       XX XX    "
		"      XXXX      "
		"       XX       "
		"        X       "
		,

// frame 2
		"                "
		"         X      "
		"      XXXXX     "
		"    XX  XXXX    "
		"   X    X       "
		"   X            "
		"  X             "
		"  X          X  "
		"  X          X  "
		"             X  "
		"            X   "
		"       X    X   "
		"    XXXX  XX    "
		"     XXXXX      "
		"      X         "
		"                "
		,

// frame 3
		"                "
		"                "
		"      XXXX  X   "
		"    XX    XXX   "
		"   X      XXX   "
		"   X        X   "
		"  X             "
		"                "
		"                "
		"             X  "
		"   X        X   "
		"   XXX      X   "
		"   XXX    XX    "
		"   X  XXXX      "
		"                "
		"                "
		,

// frame 4
		"                "
		"                "
		"      XXXX      "
		"    XX    XX    "
		"   X        X X "
		"            XXX "
		"           XXX  "
		"             X  "
		"  X             "
		"  XXX           "
		" XXX            "
		" X X        X   "
		"    XX    XX    "
		"      XXXX      "
		"                "
		"                "
		,

// frame 5
		"                "
		"                "
		"      XXXX      "
		"     X    XX    "
		"            X   "
		"            X   "
		"  X          X  "
		" XXX       XXXX "
		"XXXXX       XXX "
		"  X          X  "
		"   X            "
		"   X            "
		"    XX    X     "
		"      XXXX      "
		"                "
		"                "
		,

// frame 6
		"                "
		"                "
		"       XXX      "
		"          XX    "
		"   X        X   "
		"  XX        X   "
		" XXX         X  "
		"  XXX        X  "
		"  X        XXX  "
		"  X         XXX "
		"   X        XX  "
		"   X        X   "
		"    XX          "
		"      XXX       "
		"                "
		"                "
		,

// frame 7
		"                "
		"                "
		"         X      "
		"  XXXX    XX    "
		"   XX       X   "
		"   XX       X   "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"  X          X  "
		"   X       XX   "
		"   X       XX   "
		"    XX    XXXX  "
		"      X         "
		"                "
		"                "
	};

void ZUIRenderer_ChasingArrows_Platinum::Activate(ZUIChasingArrows_Rendered* inChasingArrows)
	{ inChasingArrows->Refresh(); }

long ZUIRenderer_ChasingArrows_Platinum::GetFrameCount()
	{ return kChasingArrows_NumFrames; }

ZPoint ZUIRenderer_ChasingArrows_Platinum::GetSize()
	{ return ZPoint(kChasingArrows_Width, kChasingArrows_Height); }

void ZUIRenderer_ChasingArrows_Platinum::DrawFrame(const ZDC& inDC, const ZRect& inBounds, bool enabled, bool inActive, long frame)
	{
	ZRGBColorMap theColorMap;
	theColorMap.fPixval = 'X';
	if (inDC.GetDepth() >= 4)
		theColorMap.fColor = sLookupColor(enabled && inActive ? kColorRamp_Black : kColorRamp_Gray_5);
	else
		theColorMap.fColor = ZRGBColor::sBlack;

	inDC.Pixels(inBounds.left, inBounds.top, kChasingArrows_Width, kChasingArrows_Height, sPixvals_ChasingArrows[frame], &theColorMap, 1);
	}

bigtime_t ZUIRenderer_ChasingArrows_Platinum::GetInterFrameDuration()
	{ return ZTicks::sPerSecond() / 10; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_TabPane_Platinum

enum { kSmallTabImages, kLargeTabImages, kNumImageTypes };
enum { kTabNormalDeselectedL, kTabNormalDeselectedR,
		kTabNormalSelectedL, kTabNormalSelectedR,
		kTabPressedL, kTabPressedR,
		kTabDisabledDeselectedL, kTabDisabledDeselectedR,
		kTabDisabledSelectedL, kTabDisabledSelectedR,
		kNumTabImages };

static const ZCoord kSmallTabImageHeight = 19;
static const ZCoord kSmallTabImageWidth = 12;
static const ZCoord kLargeTabImageHeight = 24;
static const ZCoord kLargeTabImageWidth = 12;
static const ZCoord kPanelEndMargin = 6;

static const char* sPixvals_Tab_Small[] =
	{
// kTabNormalDeselectedL
		"          20"
		"        20AC"
		"       2ACDD"
		"      2ADDCC"
		"      0CDCCC"
		"     2CDCCCC"
		"     0CDCCCC"
		"    2ADCCCCC"
		"    0CDCCCCC"
		"   2ADCCCCCC"
		"   0CDCCCCCC"
		"  2ADCCCCCCC"
		"  0CDCCCCCCC"
		" 2ADCCCCCCCC"
		" 0CDCCCCCCCC"
		"2ADCCCCCCCCC"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabNormalDeselectedR
		"00          "
		"CC00        "
		"DDD80       "
		"CCCB42      "
		"CCCC80      "
		"CCCCB62     "
		"CCCCC80     "
		"CCCCCB62    "
		"CCCCCC80    "
		"CCCCCCB62   "
		"CCCCCCC80   "
		"CCCCCCCB62  "
		"CCCCCCCC80  "
		"CCCCCCCCB62 "
		"CCCCCCCCC80 "
		"CCCCCCCCCB62"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabNormalSelectedL
		"          20"
		"        20BC"
		"       2BC__"
		"      2B__EE"
		"      0C_EEE"
		"     2B_EEEE"
		"     0C_EEEE"
		"    2B_EEEEE"
		"    0C_EEEEE"
		"   2B_EEEEEE"
		"   0C_EEEEEE"
		"  2B_EEEEEEE"
		"  0C_EEEEEEE"
		" 2B_EEEEEEEE"
		" 0C_EEEEEEEE"
		"2B_EEEEEEEEE"
		"0C_EEEEEEEEE"
		"C_EEEEEEEEEE"
		"_EEEEEEEEEEE"
		,

// kTabNormalSelectedR
		"00          "
		"CB00        "
		"__D80       "
		"EEEB42      "
		"EEED80      "
		"EEEEB42     "
		"EEEED80     "
		"EEEEEB42    "
		"EEEEED80    "
		"EEEEEEB42   "
		"EEEEEED80   "
		"EEEEEEEB42  "
		"EEEEEEED80  "
		"EEEEEEEEC42 "
		"EEEEEEEEE80 "
		"EEEEEEEEED40"
		"EEEEEEEEEE80"
		"EEEEEEEEEECC"
		"EEEEEEEEEEE_"
		,

// kTabPressedL
		"          20"
		"        2044"
		"       24455"
		"      245566"
		"      045666"
		"     2456666"
		"     0456666"
		"    24566666"
		"    04566666"
		"   245666666"
		"   045666666"
		"  2456666666"
		"  0456666666"
		" 24566666666"
		" 04566666666"
		"245666666666"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabPressedR
		"00          "
		"4800        "
		"55870       "
		"666842      "
		"666670      "
		"6666842     "
		"6666670     "
		"66666842    "
		"66666670    "
		"666666842   "
		"666666670   "
		"6666666842  "
		"6666666670  "
		"66666666842 "
		"66666666670 "
		"666666666842"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabDisabledDeselectedL
		"          55"
		"        55DD"
		"       5DDDD"
		"      5DDDDD"
		"      5DDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		"5DDDDDDDDDDD"
		"555555555555"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledDeselectedR
		"55          "
		"DD55        "
		"DDDD5       "
		"DDDDD5      "
		"DDDDD5      "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDDD5"
		"555555555555"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledSelectedL
		"          55"
		"        55DD"
		"       5DDDD"
		"      5DDDDD"
		"      5DDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		"5DDDDDDDDDDD"
		"5DDDDDDDDDDD"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledSelectedR
		"55          "
		"DD55        "
		"DDDD5       "
		"DDDDD5      "
		"DDDDD5      "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDDD5"
		"DDDDDDDDDDD5"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"

	};

static const char* sPixvals_Tab_Large[] =
	{
// kTabNormalDeselectedL
		"          20"
		"        20AC"
		"       2ACDD"
		"      2ADDCC"
		"      0CDCCC"
		"     2ADCCCC"
		"     0CDCCCC"
		"     0CDCCCC"
		"    2ADCCCCC"
		"    0CDCCCCC"
		"    0CDCCCCC"
		"   2ADCCCCCC"
		"   0CDCCCCCC"
		"   0CDCCCCCC"
		"  2ADCCCCCCC"
		"  0CDCCCCCCC"
		"  0CDCCCCCCC"
		" 2ADCCCCCCCC"
		" 0CDCCCCCCCC"
		" 0CDCCCCCCCC"
		"2ADCCCCCCCCC"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabNormalDeselectedR
		"00          "
		"CC00        "
		"DDD80       "
		"CCCB42      "
		"CCCC80      "
		"CCCCB42     "
		"CCCCC80     "
		"CCCCCA0     "
		"CCCCCB42    "
		"CCCCCC80    "
		"CCCCCCA0    "
		"CCCCCCB42   "
		"CCCCCCC80   "
		"CCCCCCCA0   "
		"CCCCCCCB42  "
		"CCCCCCCC80  "
		"CCCCCCCCA0  "
		"CCCCCCCCB42 "
		"CCCCCCCCC80 "
		"CCCCCCCCCA0 "
		"CCCCCCCCCB62"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabNormalSelectedL
		"          20"
		"        20BC"
		"       2BC__"
		"      2BC_EE"
		"      0C_EEE"
		"     2B_EEEE"
		"     0C_EEEE"
		"     0C_EEEE"
		"    2B_EEEEE"
		"    0C_EEEEE"
		"    0C_EEEEE"
		"   2B_EEEEEE"
		"   0C_EEEEEE"
		"   0C_EEEEEE"
		"  2B_EEEEEEE"
		"  0C_EEEEEEE"
		"  0C_EEEEEEE"
		" 2B_EEEEEEEE"
		" 0C_EEEEEEEE"
		" 0C_EEEEEEEE"
		"2B_EEEEEEEEE"
		"0C_EEEEEEEEE"
		"C_EEEEEEEEEE"
		"_EEEEEEEEEEE"
		,

// kTabNormalSelectedR
		"00          "
		"CB00        "
		"__D80       "
		"EEEB42      "
		"EEED80      "
		"EEEEB42     "
		"EEEEC80     "
		"EEEED80     "
		"EEEEEB42    "
		"EEEEEC80    "
		"EEEEED80    "
		"EEEEEEB42   "
		"EEEEEEC80   "
		"EEEEEED80   "
		"EEEEEEEB42  "
		"EEEEEEEC80  "
		"EEEEEEED80  "
		"EEEEEEEEC42 "
		"EEEEEEEED80 "
		"EEEEEEEEE80 "
		"EEEEEEEEED40"
		"EEEEEEEEEE80"
		"EEEEEEEEEECC"
		"EEEEEEEEEEE_"
		,

// kTabPressedL
		"          20"
		"        2044"
		"       24455"
		"      244566"
		"      045666"
		"     2456666"
		"     0456666"
		"     0456666"
		"    24566666"
		"    04566666"
		"    04566666"
		"   245666666"
		"   045666666"
		"   045666666"
		"  2456666666"
		"  0456666666"
		"  0456666666"
		" 24566666666"
		" 04566666666"
		" 04566666666"
		"245666666666"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabPressedR
		"00          "
		"4800        "
		"55870       "
		"666842      "
		"666670      "
		"6666842     "
		"6666670     "
		"6666670     "
		"66666842    "
		"66666670    "
		"66666670    "
		"666666842   "
		"666666670   "
		"666666670   "
		"6666666842  "
		"6666666670  "
		"6666666670  "
		"66666666842 "
		"66666666670 "
		"66666666670 "
		"666666666842"
		"000000000000"
		"CCCCCCCCCCCC"
		"____________"
		,

// kTabDisabledDeselectedL
		"          55"
		"        55DD"
		"       5DDDD"
		"      5DDDDD"
		"      5DDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		"5DDDDDDDDDDD"
		"555555555555"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledDeselectedR
		"55          "
		"DD55        "
		"DDDD5       "
		"DDDDD5      "
		"DDDDD5      "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDDD5"
		"555555555555"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledSelectedL
		"          55"
		"        55DD"
		"       5DDDD"
		"      5DDDDD"
		"      5DDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"     5DDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"    5DDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"   5DDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		"  5DDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		" 5DDDDDDDDDD"
		"5DDDDDDDDDDD"
		"5DDDDDDDDDDD"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"
		,

// kTabDisabledSelectedR
		"55          "
		"DD55        "
		"DDDD5       "
		"DDDDD5      "
		"DDDDD5      "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDD5     "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDD5    "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDD5   "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDD5  "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDD5 "
		"DDDDDDDDDDD5"
		"DDDDDDDDDDD5"
		"DDDDDDDDDDDD"
		"DDDDDDDDDDDD"

	};

static const char* sPixvals_Tab_SmallBW[] =
	{
// kTabNormalDeselectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		"0___________"
		"000000000000"
		"____________"
		"____________"
		,

// kTabNormalDeselectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"___________0"
		"000000000000"
		"____________"
		"____________"
		,

// kTabNormalSelectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		"0___________"
		"0___________"
		"____________"
		"____________"
		,

// kTabNormalSelectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"___________0"
		"___________0"
		"____________"
		"____________"
		,

// kTabPressedL
		"          00"
		"        0000"
		"       00000"
		"      000000"
		"      000000"
		"     0000000"
		"     0000000"
		"    00000000"
		"    00000000"
		"   000000000"
		"   000000000"
		"  0000000000"
		"  0000000000"
		" 00000000000"
		" 00000000000"
		"000000000000"
		"000000000000"
		"____________"
		"____________"
		,

// kTabPressedR
		"00          "
		"0000        "
		"00000       "
		"000000      "
		"000000      "
		"0000000     "
		"0000000     "
		"00000000    "
		"00000000    "
		"000000000   "
		"000000000   "
		"0000000000  "
		"0000000000  "
		"00000000000 "
		"00000000000 "
		"000000000000"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledDeselectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		"0___________"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledDeselectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"___________0"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledSelectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		"0___________"
		"0___________"
		"____________"
		"____________"
		,

// kTabDisabledSelectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"___________0"
		"___________0"
		"____________"
		"____________"

	};

static const char* sPixvals_Tab_LargeBW[] =
	{
// kTabNormalDeselectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		" 0__________"
		"0___________"
		"000000000000"
		"____________"
		"____________"
		,

// kTabNormalDeselectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"__________0 "
		"___________0"
		"000000000000"
		"____________"
		"____________"
		,

// kTabNormalSelectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		" 0__________"
		"0___________"
		"0___________"
		"____________"
		"____________"
		,

// kTabNormalSelectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"__________0 "
		"___________0"
		"___________0"
		"____________"
		"____________"
		,

// kTabPressedL
		"          00"
		"        0000"
		"       00000"
		"      000000"
		"      000000"
		"     0000000"
		"     0000000"
		"     0000000"
		"    00000000"
		"    00000000"
		"    00000000"
		"   000000000"
		"   000000000"
		"   000000000"
		"  0000000000"
		"  0000000000"
		"  0000000000"
		" 00000000000"
		" 00000000000"
		" 00000000000"
		"000000000000"
		"000000000000"
		"____________"
		"____________"
		,

// kTabPressedR
		"00          "
		"0000        "
		"00000       "
		"000000      "
		"000000      "
		"0000000     "
		"0000000     "
		"0000000     "
		"00000000    "
		"00000000    "
		"00000000    "
		"000000000   "
		"000000000   "
		"000000000   "
		"0000000000  "
		"0000000000  "
		"0000000000  "
		"00000000000 "
		"00000000000 "
		"00000000000 "
		"000000000000"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledDeselectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		" 0__________"
		"0___________"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledDeselectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"__________0 "
		"___________0"
		"000000000000"
		"____________"
		"____________"
		,

// kTabDisabledSelectedL
		"          00"
		"        00__"
		"       0____"
		"      0_____"
		"      0_____"
		"     0______"
		"     0______"
		"     0______"
		"    0_______"
		"    0_______"
		"    0_______"
		"   0________"
		"   0________"
		"   0________"
		"  0_________"
		"  0_________"
		"  0_________"
		" 0__________"
		" 0__________"
		" 0__________"
		"0___________"
		"0___________"
		"____________"
		"____________"
		,

// kTabDisabledSelectedR
		"00          "
		"__00        "
		"____0       "
		"_____0      "
		"_____0      "
		"______0     "
		"______0     "
		"______0     "
		"_______0    "
		"_______0    "
		"_______0    "
		"________0   "
		"________0   "
		"________0   "
		"_________0  "
		"_________0  "
		"_________0  "
		"__________0 "
		"__________0 "
		"__________0 "
		"___________0"
		"___________0"
		"____________"
		"____________"

	};
#if 0
static const char* sPixvals_Tab_SmallMask[] =
	{
// Left
		"          00"
		"        0000"
		"       00000"
		"      000000"
		"      000000"
		"     0000000"
		"     0000000"
		"    00000000"
		"    00000000"
		"   000000000"
		"   000000000"
		"  0000000000"
		"  0000000000"
		" 00000000000"
		" 00000000000"
		"000000000000"
		"000000000000"
		"000000000000"
		"000000000000"
		,

// Right
		"00          "
		"0000        "
		"00000       "
		"000000      "
		"000000      "
		"0000000     "
		"0000000     "
		"00000000    "
		"00000000    "
		"000000000   "
		"000000000   "
		"0000000000  "
		"0000000000  "
		"00000000000 "
		"00000000000 "
		"000000000000"
		"000000000000"
		"000000000000"
		"000000000000"
};

static const char* sPixvals_Tab_LargeMask[] =
	{
// Left
		"          00"
		"        0000"
		"       00000"
		"      000000"
		"      000000"
		"     0000000"
		"     0000000"
		"     0000000"
		"    00000000"
		"    00000000"
		"    00000000"
		"   000000000"
		"   000000000"
		"   000000000"
		"  0000000000"
		"  0000000000"
		"  0000000000"
		" 00000000000"
		" 00000000000"
		" 00000000000"
		"000000000000"
		"000000000000"
		"000000000000"
		"000000000000"
		,

// Right
		"00          "
		"0000        "
		"00000       "
		"000000      "
		"000000      "
		"0000000     "
		"0000000     "
		"0000000     "
		"00000000    "
		"00000000    "
		"00000000    "
		"000000000   "
		"000000000   "
		"000000000   "
		"0000000000  "
		"0000000000  "
		"0000000000  "
		"00000000000 "
		"00000000000 "
		"00000000000 "
		"000000000000"
		"000000000000"
		"000000000000"
		"000000000000"
	};
#endif
ZUIRenderer_TabPane_Platinum::ZUIRenderer_TabPane_Platinum()
	{}

ZUIRenderer_TabPane_Platinum::~ZUIRenderer_TabPane_Platinum()
	{}

ZRect ZUIRenderer_TabPane_Platinum::GetContentArea(ZUITabPane_Rendered* inTabPane)
	{
	bool hasLargeTabs;
	if (!inTabPane->GetAttribute("ZUITabPane::LargeTab", nil, &hasLargeTabs))
		hasLargeTabs = true;

	ZPoint theSize = inTabPane->GetSize();
	ZRect theRect(3, (ZCoord)(hasLargeTabs ? kLargeTabImageHeight : kSmallTabImageHeight), theSize.h - 3, theSize.v - 3);
	return theRect;
	}

void ZUIRenderer_TabPane_Platinum::sRenderTab(const ZDC& inDC, ZPoint inLocation, ZCoord inMidWidth, bool inLarge, bool inPressed, EZTriState inHilite, bool inEnabled)
	{
	ZDC localDC(inDC);

	size_t imageIndex = kTabNormalDeselectedL;
	if (inEnabled)
		{
		if (inHilite != eZTriState_Off)
			imageIndex = kTabNormalSelectedL;
		else if (inPressed)
			imageIndex = kTabPressedL;
		}
	else
		{
		if (inHilite != eZTriState_Off)
			imageIndex = kTabDisabledSelectedL;
		else
			imageIndex = kTabDisabledDeselectedL;
		}

	const char* pixValsLeft;
	const char* pixValsRight;
	ZCoord imageWidth;
	ZCoord imageHeight;

	if (inLarge)
		{
		imageWidth = kLargeTabImageWidth;
		imageHeight = kLargeTabImageHeight;
		if (localDC.GetDepth() < 4)
			{
			pixValsLeft = sPixvals_Tab_LargeBW[imageIndex];
			pixValsRight = sPixvals_Tab_LargeBW[imageIndex + 1];
			}
		else
			{
			pixValsLeft = sPixvals_Tab_Large[imageIndex];
			pixValsRight = sPixvals_Tab_Large[imageIndex + 1];
			}
		}
	else
		{
		imageWidth = kSmallTabImageWidth;
		imageHeight = kSmallTabImageHeight;
		if (localDC.GetDepth() < 4)
			{
			pixValsLeft = sPixvals_Tab_SmallBW[imageIndex];
			pixValsRight = sPixvals_Tab_SmallBW[imageIndex + 1];
			}
		else
			{
			pixValsLeft = sPixvals_Tab_Small[imageIndex];
			pixValsRight = sPixvals_Tab_Small[imageIndex + 1];
			}
		}

// Left
	localDC.Pixels(inLocation.h, inLocation.v, imageWidth, imageHeight, pixValsLeft, sColorMap, sColorMapSize);
// Right
	localDC.Pixels(inLocation.h + imageWidth + inMidWidth, inLocation.v, imageWidth, imageHeight, pixValsRight, sColorMap, sColorMapSize);

	ZRect midSectionFrame(inLocation.h + imageWidth, inLocation.v, inLocation.h + imageWidth + inMidWidth, inLocation.v + imageHeight);

// Now draw the background and the other crud that's needed
	if (localDC.GetDepth() < 4)
		{
		ZRect localFrame = midSectionFrame;
		localFrame.bottom -= 3;
		localDC.PenNormal();
		if (inEnabled)
			{
			if (inPressed)
				localDC.Fill(localFrame);
			else
				{
				localDC.Line(localFrame.left, localFrame.top, localFrame.right - 1, localFrame.top);
				localDC.SetInk(ZRGBColor::sWhite);
				localDC.Fill(ZRect(localFrame.left, localFrame.top + 1, localFrame.right, localFrame.bottom));
				}
			}
		else
			{
// Our bitmaps are not grayed out, so we can't have a grayed line at top. That said, the bitmaps will probably look horrible
// even if they were grayed out (lots of 45 degree lines, which will either look solid or be completely empty)
//			localDC.SetInk(ZDCInk::sGray);
			localDC.Line(localFrame.left, localFrame.top, localFrame.right - 1, localFrame.top);
			localDC.SetInk(ZRGBColor::sWhite);
			localDC.Fill(ZRect(localFrame.left, localFrame.top + 1, localFrame.right, localFrame.bottom));
			}
		}
	else
		{
		enum { EdgeFrame, OuterEdge, InnerEdge, Fill, InteriorFrame, InteriorOuterEdge, InteriorInnerEdge, kNumTabColors };
		uint8 colorIndexes[kNumTabColors];
		if (inEnabled)
			{
			if (inHilite != eZTriState_Off)
				{
				colorIndexes[EdgeFrame] = kColorRamp_Black;
				colorIndexes[OuterEdge] = kColorRamp_Gray_C;
				colorIndexes[InnerEdge] = kColorRamp_White;
				colorIndexes[Fill] = kColorRamp_Gray_E;
				colorIndexes[InteriorFrame] = kColorRamp_Gray_E;
				colorIndexes[InteriorOuterEdge] = kColorRamp_Gray_E;
				colorIndexes[InteriorInnerEdge] = kColorRamp_Gray_E;
				}
			else if (inPressed)
				{
				colorIndexes[EdgeFrame] = kColorRamp_Black;
				colorIndexes[OuterEdge] = kColorRamp_Gray_4;
				colorIndexes[InnerEdge] = kColorRamp_Gray_5;
				colorIndexes[Fill] = kColorRamp_Gray_6;
				colorIndexes[InteriorFrame] = kColorRamp_Black;
				colorIndexes[InteriorOuterEdge] = kColorRamp_Gray_C;
				colorIndexes[InteriorInnerEdge] = kColorRamp_White;
				}
			else
				{
				colorIndexes[EdgeFrame] = kColorRamp_Black;
				colorIndexes[OuterEdge] = kColorRamp_Gray_C;
				colorIndexes[InnerEdge] = kColorRamp_Gray_D;
				colorIndexes[Fill] = kColorRamp_Gray_C;
				colorIndexes[InteriorFrame] = kColorRamp_Black;
				colorIndexes[InteriorOuterEdge] = kColorRamp_Gray_C;
				colorIndexes[InteriorInnerEdge] = kColorRamp_White;
				}
			}
		else
			{
			if (inHilite != eZTriState_Off)
				{
				colorIndexes[EdgeFrame] = kColorRamp_Gray_5;
				colorIndexes[OuterEdge] = kColorRamp_Gray_D;
				colorIndexes[InnerEdge] = kColorRamp_Gray_D;
				colorIndexes[Fill] = kColorRamp_Gray_D;
				colorIndexes[InteriorFrame] = kColorRamp_Gray_D;
				colorIndexes[InteriorOuterEdge] = kColorRamp_Gray_D;
				colorIndexes[InteriorInnerEdge] = kColorRamp_Gray_D;
				}
			else
				{
				colorIndexes[EdgeFrame] = kColorRamp_Gray_5;
				colorIndexes[OuterEdge] = kColorRamp_Gray_D;
				colorIndexes[InnerEdge] = kColorRamp_Gray_D;
				colorIndexes[Fill] = kColorRamp_Gray_D;
				colorIndexes[InteriorFrame] = kColorRamp_Gray_5;
				colorIndexes[InteriorOuterEdge] = kColorRamp_Gray_D;
				colorIndexes[InteriorInnerEdge] = kColorRamp_Gray_D;
				}
			}
		localDC.SetInk(sLookupColor(colorIndexes[EdgeFrame]));
		localDC.Line(midSectionFrame.left, midSectionFrame.top, midSectionFrame.right - 1, midSectionFrame.top);

		localDC.SetInk(sLookupColor(colorIndexes[OuterEdge]));
		localDC.Line(midSectionFrame.left, midSectionFrame.top + 1, midSectionFrame.right - 1, midSectionFrame.top + 1);

		localDC.SetInk(sLookupColor(colorIndexes[InnerEdge]));
		localDC.Line(midSectionFrame.left, midSectionFrame.top + 2, midSectionFrame.right - 1, midSectionFrame.top + 2);

		localDC.SetInk(sLookupColor(colorIndexes[InteriorInnerEdge]));
		localDC.Line(midSectionFrame.left, midSectionFrame.bottom - 1, midSectionFrame.right - 1, midSectionFrame.bottom - 1);

		localDC.SetInk(sLookupColor(colorIndexes[InteriorOuterEdge]));
		localDC.Line(midSectionFrame.left, midSectionFrame.bottom - 2, midSectionFrame.right - 1, midSectionFrame.bottom - 2);

		localDC.SetInk(sLookupColor(colorIndexes[InteriorFrame]));
		localDC.Line(midSectionFrame.left, midSectionFrame.bottom - 3, midSectionFrame.right - 1, midSectionFrame.bottom - 3);

		localDC.SetInk(sLookupColor(colorIndexes[Fill]));
		localDC.Fill(ZRect(midSectionFrame.left, midSectionFrame.top + 3, midSectionFrame.right, midSectionFrame.bottom - 3));
		}
	}

void ZUIRenderer_TabPane_Platinum::Activate(ZUITabPane_Rendered* inTabPane)
	{ inTabPane->Refresh(); }

bool ZUIRenderer_TabPane_Platinum::GetContentBackInk(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZDCInk& outInk)
	{
	outInk = sLookupColor(inTabPane->GetActive() && inTabPane->GetReallyEnabled() ? kColorRamp_Gray_E: kColorRamp_Gray_D);
	return true;
	}

void ZUIRenderer_TabPane_Platinum::RenderBackground(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, const ZRect& inBounds)
	{
	bool hasLargeTabs;
	if (!inTabPane->GetAttribute("ZUITabPane::LargeTab", nil, &hasLargeTabs))
		hasLargeTabs = true;

	bool enabled = inTabPane->GetReallyEnabled() && inTabPane->GetActive();

	ZDC localDC(inDC);

// First draw all the tabs
	ZRect topBounds = inBounds;
	topBounds.bottom = topBounds.top + hasLargeTabs ? kLargeTabImageHeight : kSmallTabImageHeight;
	ZCoord activeLeft = 0;
	ZCoord activeRight = 0;
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
				ZCoord tabWidth = captionSize.h + 2*(hasLargeTabs ? kLargeTabImageWidth : kSmallTabImageWidth);;
				if (*i == currentCaption)
					{
					activeLeft = leftEdge;
					activeRight = leftEdge + tabWidth;
					}
				this->DrawTab(inTabPane, localDC, *i, ZRect(leftEdge, inBounds.Top(), leftEdge + tabWidth, inBounds.Bottom()), ZUIDisplayState(false, *i == currentCaption ? eZTriState_On : eZTriState_Off, enabled, false, enabled));
				leftEdge += tabWidth;
				}
			}
		}

	ZRect borderBounds = inBounds;
	borderBounds.top += (hasLargeTabs ? kLargeTabImageHeight : kSmallTabImageHeight) - 3;

// Clip out the area occupied by the active caption
	localDC.SetClip(localDC.GetClip() - ZRect(activeLeft, borderBounds.top, activeRight, borderBounds.top + 3));

// Draw the border area
	if (localDC.GetDepth() < 4)
		{
		if (enabled)
			localDC.SetInk(sLookupColor(kColorRamp_Black));
		else
			localDC.SetInk(ZDCInk::sGray);
		}
	else
		localDC.SetInk(sLookupColor(enabled ? kColorRamp_Black : kColorRamp_Gray_5));
	localDC.Frame(borderBounds);

	if (localDC.GetDepth() < 4)
		localDC.SetInk(sLookupColor(kColorRamp_White));
	else
		localDC.SetInk(sLookupColor(enabled ? kColorRamp_Gray_E : kColorRamp_Gray_D));
	borderBounds.MakeInset(1,1);
	localDC.Fill(borderBounds);

	if (localDC.GetDepth() > 4 && enabled)
		{
		localDC.SetInk(sLookupColor(kColorRamp_Gray_C));
		sDrawSide_TopLeft(localDC, borderBounds, 0,0,1,1);

		localDC.SetInk(sLookupColor(kColorRamp_Gray_9));
		sDrawSide_BottomRight(localDC, borderBounds, 1,1,0,0);

		localDC.SetInk(sLookupColor(kColorRamp_White));
		sDrawSide_TopLeft(localDC, borderBounds, 1,1,2,2);

		localDC.SetInk(sLookupColor(kColorRamp_Gray_B));
		sDrawSide_BottomRight(localDC, borderBounds, 2,2,1,1);

		localDC.Pixel(borderBounds.left, borderBounds.bottom - 1, sLookupColor(kColorRamp_Gray_D));
		localDC.Pixel(borderBounds.right - 1, borderBounds.top, sLookupColor(kColorRamp_Gray_D));
		}
	}

ZRect ZUIRenderer_TabPane_Platinum::CalcTabRect(ZUITabPane_Rendered* inTabPane, ZRef<ZUICaption> inCaption)
	{
	bool hasLargeTabs;
	if (!inTabPane->GetAttribute("ZUITabPane::LargeTab", nil, &hasLargeTabs))
		hasLargeTabs = true;

	ZRect bounds = inTabPane->CalcBounds();

	ZRef<ZUICaption> currentCaption(inTabPane->GetCurrentTabCaption());
	const vector<ZRef<ZUICaption> >& theVector = inTabPane->GetCaptionVector();
	ZCoord leftEdge = kPanelEndMargin;
	for (vector<ZRef<ZUICaption> >::const_iterator i = theVector.begin(); i != theVector.end(); ++i)
		{
		if (*i)
			{
			ZPoint captionSize = (*i)->GetSize();
			ZCoord tabWidth = captionSize.h + 2*(hasLargeTabs ? kLargeTabImageWidth : kSmallTabImageWidth);
			if (*i == inCaption)
				return ZRect(leftEdge, bounds.Top(), leftEdge + tabWidth, bounds.Top() + (hasLargeTabs ? kLargeTabImageHeight : kSmallTabImageHeight));
			leftEdge += tabWidth;
			}
		}
	return ZRect::sZero;
	}

void ZUIRenderer_TabPane_Platinum::DrawTab(ZUITabPane_Rendered* inTabPane, const ZDC& inDC, ZRef<ZUICaption> inCaption, const ZRect& inTabRect, const ZUIDisplayState& inState)
	{
	bool hasLargeTabs;
	if (!inTabPane->GetAttribute("ZUITabPane::LargeTab", nil, &hasLargeTabs))
		hasLargeTabs = true;

	ZPoint location = inTabRect.TopLeft();
	ZCoord midWidth = inTabRect.Width() - 2 * (hasLargeTabs ? kLargeTabImageWidth : kSmallTabImageWidth);
	sRenderTab(inDC, location, midWidth, hasLargeTabs, inState.GetPressed(), inState.GetHilite(), inState.GetEnabled() && inState.GetActive());
	if (inCaption)
		{
		ZRect midSectionFrame;
		if (hasLargeTabs)
			midSectionFrame = ZRect(location.h + kLargeTabImageWidth, location.v, location.h + kLargeTabImageWidth + midWidth, location.v + kLargeTabImageHeight - 1);
		else
			midSectionFrame = ZRect(location.h + kSmallTabImageWidth, location.v, location.h + kSmallTabImageWidth + midWidth, location.v + kSmallTabImageHeight - 1);
		ZPoint captionSize = inCaption->GetSize();
		ZRect captionBounds(captionSize);
		captionBounds = captionBounds.Centered(midSectionFrame);
		inCaption->Draw(inDC, captionBounds.TopLeft(), inState, this);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Group_Platinum

ZUIRenderer_Group_Platinum::ZUIRenderer_Group_Platinum(bool isPrimary)
:	fIsPrimary(isPrimary)
	{}

ZUIRenderer_Group_Platinum::~ZUIRenderer_Group_Platinum()
	{}

void ZUIRenderer_Group_Platinum::Activate(ZUIGroup_Rendered* inGroup, ZPoint inTitleSize)
	{
	ZRect theBounds = inGroup->CalcBounds();
	if (inTitleSize.v)
		theBounds.top += inTitleSize.v - 4;
	inGroup->Invalidate(ZDCRgn(theBounds) - theBounds.Inset(2,2));
	}
void ZUIRenderer_Group_Platinum::RenderGroup(const ZDC& inDC, const ZRect& inBounds, bool inEnabled, bool inActive, ZPoint inTitleSize) 
	{
	ZDC localDC(inDC);
// Clip out the area that is occupied by the header pane, if any
	if (inTitleSize.h > 0)
		{
		ZRect clipRect(inBounds.left + 8, inBounds.top, inBounds.left + 12 + inTitleSize.h, inBounds.top + inTitleSize.v);
		localDC.SetClip(localDC.GetClip() - clipRect);
		}
	ZRect realBounds(inBounds);
	if (inTitleSize.v)
		realBounds.top += inTitleSize.v - 4;

	if (localDC.GetDepth() < 4)
		{
		localDC.SetInk(ZDCInk::sGray);
		localDC.Frame(realBounds);
		}
	else
		{
		if (fIsPrimary)
			{
			ZRGBColor borderDarkColor(sLookupColor(inEnabled && inActive ? kColorRamp_Gray_8 : kColorRamp_Gray_B));
			ZRGBColor borderLightColor(sLookupColor(inEnabled && inActive ? kColorRamp_White : kColorRamp_Gray_D));
			if (inEnabled && inActive)
				{
				localDC.SetInk(borderLightColor);
				realBounds.right -= 1;
				realBounds.bottom -= 1;
				localDC.Frame(realBounds + ZPoint(1,1));
				}
			localDC.SetInk(borderDarkColor);
			localDC.Frame(realBounds);
			if (inEnabled && inActive)
				{
				localDC.Pixel(realBounds.left, realBounds.bottom, borderLightColor);
				localDC.Pixel(realBounds.right, realBounds.top, borderLightColor);
				}
			}
		else
			{
			localDC.SetInk(sLookupColor(inEnabled && inActive ? kColorRamp_Gray_8 : kColorRamp_Gray_B));
			sDrawSide_TopLeft(localDC, realBounds, 0,0,1,1);
			localDC.SetInk(sLookupColor(inEnabled && inActive ? kColorRamp_White : kColorRamp_Gray_B));
			sDrawSide_BottomRight(localDC, realBounds, 1,1,0,0);
			}
		}
	}
ZCoord ZUIRenderer_Group_Platinum::GetTitlePaneHLocation()
	{
// This needs to be better -- we want to align the baseline of the pane with our border line
	return 10;
	}
ZDCInk ZUIRenderer_Group_Platinum::GetBackInk(const ZDC& inDC)
	{ return ZDCInk(); }

void ZUIRenderer_Group_Platinum::GetInsets(ZPoint inTitleSize, ZPoint& outTopLeft, ZPoint& outBottomRight)
	{
	outTopLeft = ZPoint(10, max(ZCoord(inTitleSize.v + 10), ZCoord(12)));
	outBottomRight = ZPoint( -10, -10);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_FocusFrame_Platinum

ZUIRenderer_FocusFrame_Platinum::ZUIRenderer_FocusFrame_Platinum()
	{}

ZUIRenderer_FocusFrame_Platinum::~ZUIRenderer_FocusFrame_Platinum()
	{}

void ZUIRenderer_FocusFrame_Platinum::Activate(ZUIFocusFrame_Rendered* inFocusFrame)
	{
	inFocusFrame->Invalidate(this->GetBorderRgn(inFocusFrame, inFocusFrame->CalcBounds()));
	}

void ZUIRenderer_FocusFrame_Platinum::Draw(ZUIFocusFrame_Rendered* inFocusFrame, const ZDC& inDC, const ZRect& inBounds, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	ZRect innerBounds;
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		innerBounds = inBounds.Inset(2, 2);
	else
		innerBounds = inBounds.Inset(1, 1);
	if (inState.GetEnabled() && inState.GetActive())
		{
		localDC.SetInk(ZRGBColor::sBlack);
		localDC.Frame(innerBounds);
		if (showFocusBorder && inState.GetFocused())
			{
			ZRGBColor theFocusRingColor(sLookupColor(kColorRamp_Gray_7));
			localDC.SetInk(theFocusRingColor);
			localDC.Frame(innerBounds.Inset( -1, -1));
// Bottom
			localDC.Line(innerBounds.left - 1, innerBounds.bottom + 1, innerBounds.right, innerBounds.bottom + 1);
// Right
			localDC.Line(innerBounds.right + 1, innerBounds.bottom, innerBounds.right + 1, innerBounds.top - 1);
// Left
			localDC.Line(innerBounds.left - 2, innerBounds.top - 1, innerBounds.left - 2, innerBounds.bottom);
// Top
			localDC.Line(innerBounds.left - 1, innerBounds.top - 2, innerBounds.right, innerBounds.top - 2);
			}
		else
			{
			localDC.SetInk(ZRGBColor::sWhite);
// Bottom
			localDC.Line(innerBounds.left, innerBounds.bottom, innerBounds.right, innerBounds.bottom);
// Right
			localDC.Line(innerBounds.right, innerBounds.bottom, innerBounds.right, innerBounds.top);

			localDC.SetInk(sLookupColor(kColorRamp_Gray_A));
// Left
			localDC.Line(innerBounds.left - 1, innerBounds.top - 1, innerBounds.left - 1, innerBounds.bottom - 1);
// Top
			localDC.Line(innerBounds.left - 1, innerBounds.top - 1, innerBounds.right - 1, innerBounds.top - 1);
			}
		}
	else
		{
		localDC.SetInk(sLookupColor(kColorRamp_Gray_5));
		localDC.Frame(innerBounds);
		}
	}

void ZUIRenderer_FocusFrame_Platinum::GetInsets(ZUIFocusFrame_Rendered* inFocusFrame, ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	bool showFocusBorder;
	if (!inFocusFrame->GetAttribute("ZUIFocusFrame::ShowFocusRing", nil, &showFocusBorder))
		showFocusBorder = true;
	if (showFocusBorder)
		{
		outTopLeftInset = ZPoint(3, 3);
		outBottomRightInset = ZPoint( -3, -3);
		}
	else
		{
		outTopLeftInset = ZPoint(2, 2);
		outBottomRightInset = ZPoint(-2, -2);
		}
	}

ZDCRgn ZUIRenderer_FocusFrame_Platinum::GetBorderRgn(ZUIFocusFrame_Rendered* inFocusFrame, const ZRect& inBounds)
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

bool ZUIRenderer_FocusFrame_Platinum::GetInternalBackInk(ZUIFocusFrame_Rendered* inFocusFrame, const ZUIDisplayState& inState, const ZDC& inDC, ZDCInk& outInk)
	{
	outInk = sLookupColor(kColorRamp_Gray_E);
	return true;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRenderer_Divider_Platinum

ZUIRenderer_Divider_Platinum::ZUIRenderer_Divider_Platinum()
	{}

ZUIRenderer_Divider_Platinum::~ZUIRenderer_Divider_Platinum()
	{}

void ZUIRenderer_Divider_Platinum::Activate(ZUIDivider_Rendered* inDivider)
	{ inDivider->Refresh(); }

ZCoord ZUIRenderer_Divider_Platinum::GetDividerThickness()
	{ return 2; }

void ZUIRenderer_Divider_Platinum::DrawDivider(const ZDC& inDC, ZPoint inLocation, ZCoord inLength, bool inHorizontal, const ZUIDisplayState& inState)
	{
	ZDC localDC(inDC);
	localDC.PenNormal();

	if (inHorizontal)
		{
		localDC.SetInk(sLookupColor(inState.GetEnabled() && inState.GetActive() ? kColorRamp_Gray_8 : kColorRamp_Gray_B));
		localDC.Line(inLocation.h, inLocation.v, inLocation.h + inLength - 1, inLocation.v);

		localDC.SetInk(sLookupColor(inState.GetEnabled() && inState.GetActive() ? kColorRamp_White : kColorRamp_Gray_D));
		localDC.Line(inLocation.h + 1, inLocation.v + 1, inLocation.h + inLength - 1, inLocation.v + 1);
		}
	else
		{
		localDC.SetInk(sLookupColor(inState.GetEnabled() && inState.GetActive() ? kColorRamp_Gray_8 : kColorRamp_Gray_B));
		localDC.Line(inLocation.h, inLocation.v, inLocation.h, inLocation.v + inLength - 1);

		localDC.SetInk(sLookupColor(inState.GetEnabled() && inState.GetActive() ? kColorRamp_White : kColorRamp_Gray_D));
		localDC.Line(inLocation.h + 1, inLocation.v + 1, inLocation.h, inLocation.v + inLength - 1);
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Platinum

ZUIRendererFactory_Platinum::ZUIRendererFactory_Platinum()
	{}
ZUIRendererFactory_Platinum::~ZUIRendererFactory_Platinum()
	{}

// From ZUIRendererFactory
string ZUIRendererFactory_Platinum::GetName()
	{ return "Platinum"; }

// From ZUIRendererFactory_Caching
ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Platinum::Make_Renderer_CaptionPane()
	{ return new ZUIRenderer_CaptionPane_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonPush()
	{ return new ZUIRenderer_ButtonPush_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonRadio()
	{ return new ZUIRenderer_ButtonRadio_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonCheckbox()
	{ return new ZUIRenderer_ButtonCheckbox_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonPopup()
	{ return new ZUIRenderer_ButtonPopup_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonBevel()
	{ return new ZUIRenderer_ButtonBevel_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonDisclosure()
	{ return new ZUIRenderer_ButtonDisclosure_Platinum; }

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Platinum::Make_Renderer_ButtonPlacard()
	{ return new ZUIRenderer_ButtonPlacard_Platinum; }

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Platinum::Make_Renderer_ScrollBar()
	{ return new ZUIRenderer_ScrollBar_Platinum; }

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Platinum::Make_Renderer_Slider()
	{ return new ZUIRenderer_Slider_Platinum; }

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Platinum::Make_Renderer_LittleArrows()
	{ return new ZUIRenderer_LittleArrows_Platinum; }

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Platinum::Make_Renderer_ProgressBar()
	{ return new ZUIRenderer_ProgressBar_Platinum; }

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Platinum::Make_Renderer_ChasingArrows()
	{ return new ZUIRenderer_ChasingArrows_Platinum; }

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Platinum::Make_Renderer_TabPane()
	{ return new ZUIRenderer_TabPane_Platinum; }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Platinum::Make_Renderer_GroupPrimary()
	{ return new ZUIRenderer_Group_Platinum(true); }

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Platinum::Make_Renderer_GroupSecondary()
	{ return new ZUIRenderer_Group_Platinum(false); }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Platinum::Make_Renderer_FocusFrameEditText()
	{ return new ZUIRenderer_FocusFrame_Platinum; }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Platinum::Make_Renderer_FocusFrameListBox()
	{ return new ZUIRenderer_FocusFrame_Platinum; }

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Platinum::Make_Renderer_FocusFrameDateControl()
	{ return new ZUIRenderer_FocusFrame_Platinum; }

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Platinum::Make_Renderer_Splitter()
	{ return new ZUIRenderer_Splitter_Standard; }

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Platinum::Make_Renderer_Divider()
	{ return new ZUIRenderer_Divider_Platinum; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Platinum

ZUIAttributeFactory_Platinum::ZUIAttributeFactory_Platinum()
	{}
ZUIAttributeFactory_Platinum::~ZUIAttributeFactory_Platinum()
	{}

string ZUIAttributeFactory_Platinum::GetName()
	{ return "Platinum"; }

ZRef<ZUIColor> ZUIAttributeFactory_Platinum::MakeColor(const string& inColorName)
	{
	if (inColorName == "Text")
		return new ZUIColor_Fixed(ZRGBColor::sBlack);

	if (inColorName == "TextHighlight")
		{
		#if ZCONFIG(OS, MacOS7)
			return new ZUIColor_MacHighlight(false);
		#else
			return new ZUIColor_Fixed(ZRGBColor::sWhite);
		#endif
		}

	return ZUIAttributeFactory_Caching::MakeColor(inColorName);
	}

ZRef<ZUIInk> ZUIAttributeFactory_Platinum::MakeInk(const string& inInkName)
	{
	if (inInkName == "WindowBackground_Document")
		return new ZUIInk_Fixed(ZRGBColor::sWhite);

	if (inInkName == "WindowBackground_Dialog")
		return new ZUIInk_Fixed(ZRGBColor(0xDDDD));

	if (inInkName == "Background_Highlight")
		{
		#if ZCONFIG(OS, MacOS7)
			return new ZUIInk_MacHighlight(true);
		#else
			return new ZUIInk_Fixed(ZRGBColor::sBlack);
		#endif
		}

	return ZUIAttributeFactory_Caching::MakeInk(inInkName);
	}

ZRef<ZUIFont> ZUIAttributeFactory_Platinum::MakeFont(const string& inFontName)
	{
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

	return ZUIAttributeFactory_Caching::MakeFont(inFontName);
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Platinum::MakeMetric(const string& inMetricName)
	{
// We'll just use the inherited versions for now
	return ZUIAttributeFactory_Caching::MakeMetric(inMetricName);
	}

// =================================================================================================
