static const char rcsid[] = "@(#) $Id: ZDC_Be.cpp,v 1.12 2006/04/10 20:44:20 agreen Exp $";

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

#include "ZDC_Be.h"

#if ZCONFIG(API_Graphics, Be)
#include <app/Application.h>
#include <kernel/OS.h>
#include <interface/Bitmap.h>
#include <interface/View.h>
#include <interface/Window.h>

#include "ZByteSwap.h"
#include "ZDebug.h"

static drawing_mode sModeLookup[] = { B_OP_COPY, B_OP_OVER, B_OP_INVERT };

static void sGetBitmapEtc(const ZDCPixmap& inPixmap, ZRef<ZDCPixmapRaster_Be>& outRaster_Be, ZRect& outPixmapSourceRect);
static ZDCPixmapNS::PixelDesc sPixelDescFromColorSpace(color_space inColorSpace);

// ==================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_Be 2

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be::SetupView

ZDCCanvas_Be::SetupView::SetupView(ZDCCanvas_Be* inCanvas, ZDCState& ioState)
:	fCanvas(inCanvas)
	{
	if (fCanvas->fLooperToLock)
		fCanvas->fLooperToLock->Lock();
	if (ioState.fChangeCount_Clip != fCanvas->fChangeCount_Clip)
		{
		ioState.fChangeCount_Clip = ++fCanvas->fChangeCount_Clip;
		fCanvas->fBView->ConstrainClippingRegion((ioState.fClip + ioState.fClipOrigin).GetBRegion());
		}
	}

ZDCCanvas_Be::SetupView::~SetupView()
	{
	if (fCanvas->fLooperToLock)
		fCanvas->fLooperToLock->Unlock();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be::SetupInk

class ZDCCanvas_Be::SetupInk
	{
public:
	SetupInk(ZDCCanvas_Be* inCanvas, ZDCState& ioState);
	~SetupInk();
	pattern GetPattern();
protected:
	ZDCCanvas_Be* fCanvas;
	ZPoint fRealPatternOrigin;
	ZRef<ZDCInk::Rep> fInkRep;
	};

ZDCCanvas_Be::SetupInk::SetupInk(ZDCCanvas_Be* inCanvas, ZDCState& ioState)
:	fCanvas(inCanvas), fInkRep(ioState.fInk.GetRep())
	{
	if (ioState.fChangeCount_Ink != fCanvas->fChangeCount_Ink)
		{
		ioState.fChangeCount_Ink = ++fCanvas->fChangeCount_Ink;
		ZAssertStop(kDebug_Be, fInkRep);
		if (fInkRep->fType == ZDCInk::eTypeSolidColor)
			fCanvas->fBView->SetHighColor(fInkRep->fAsSolidColor.fColor);
		else if (fInkRep->fType == ZDCInk::eTypeTwoColor)
			{
			fCanvas->fBView->SetHighColor(fInkRep->fAsTwoColor.fForeColor);
			fCanvas->fBView->SetLowColor(fInkRep->fAsTwoColor.fBackColor);
			fRealPatternOrigin = ioState.fPatternOrigin;
			}
		else if (fInkRep->fType == ZDCInk::eTypeMultiColor)
			{
// There are some methods that can't draw with multi color inks. When we try to do so we'll
// see magenta instead, which should be noticeable!
			fCanvas->fBView->SetHighColor(ZRGBColor::sMagenta);
			}
		else
			{
			ZDebugStopf(kDebug_Be, ("ZDCCanvas_Be::SetupInk, unknown ink type"));
			}
		fCanvas->fBView->SetPenSize(1);
		}

	if (ioState.fChangeCount_Mode != fCanvas->fChangeCount_Mode)
		{
		ioState.fChangeCount_Mode = ++fCanvas->fChangeCount_Mode;
		fCanvas->fBView->SetDrawingMode(sModeLookup[ioState.fMode]);
		}
	}

ZDCCanvas_Be::SetupInk::~SetupInk()
	{}

pattern ZDCCanvas_Be::SetupInk::GetPattern()
	{
	if (fInkRep->fType == ZDCInk::eTypeTwoColor)
		{
		pattern tempPattern;
		ZDCPattern::sOffset(fRealPatternOrigin.h, fRealPatternOrigin.v, fInkRep->fAsTwoColor.fPattern.pat, tempPattern.data);
		return tempPattern;
		}
	return B_SOLID_HIGH;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be

ZDCCanvas_Be::ZDCCanvas_Be()
	{
	fBView = nil;
	fLooperToLock = nil;

	fChangeCount_Ink = 1;
	fChangeCount_Mode = 1;
	fChangeCount_Clip = 1;
	fChangeCount_Font = 1;
	}

ZDCCanvas_Be::~ZDCCanvas_Be()
	{
	ZAssertStop(kDebug_Be, fBView == nil);
	ZAssertStop(kDebug_Be, fLooperToLock == nil);
	}

void ZDCCanvas_Be::Pixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, const ZRGBColor& inColor)
	{
	if (!fBView)
		return;
	inLocationH += ioState.fOrigin.h;
	inLocationV += ioState.fOrigin.v;

	SetupView theSetupView(this, ioState);
	fBView->SetHighColor(inColor);
	++fChangeCount_Ink;
	fBView->SetPenSize(1);
	fBView->SetDrawingMode(B_OP_COPY);
	++fChangeCount_Mode;
	fBView->StrokeLine(BPoint(inLocationH, inLocationV), BPoint(inLocationH, inLocationV), B_SOLID_HIGH);
	}

void ZDCCanvas_Be::Pixels(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, ZCoord inWidth, ZCoord inHeight,
								const char* inPixvals, const ZRGBColorMap* inColorMap, size_t inColorMapSize,
								const ZRGBColorPOD* inDitherColor)
	{
	if (!fBView)
		return;

	vector<ZRGBColorPOD> localColors(256);
	for (size_t x = 0; x < 256; ++x)
		localColors[x].red = localColors[x].green = localColors[x].blue = localColors[x].alpha = 0;

	for (size_t x = 0; x < inColorMapSize; ++x)
		localColors[uint8(inColorMap[x].fPixval)] = inColorMap[x].fColor;

	ZDCPixmapNS::PixelDesc theSourcePixelDesc(&localColors[0], 256);
	ZDCPixmapNS::RasterDesc theSourceRasterDesc(ZDCPixmapNS::PixvalDesc(false, 8), inWidth, inHeight, false);

	BBitmap* theBitmap = new BBitmap(ZRect(inWidth, inHeight), B_RGBA32_LITTLE);
	ZDCPixmapNS::PixelDesc theBitmapPixelDesc(0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	ZDCPixmapNS::RasterDesc theBitmapRasterDesc(ZDCPixmapNS::PixvalDesc(false, 32), theBitmap->BytesPerRow(), inHeight, false);

	ZRect theBitmapBounds(inWidth, inHeight);

	ZDCPixmapNS::sBlit(inPixvals, theSourceRasterDesc, theSourcePixelDesc,
						theBitmap->Bits(), theBitmapRasterDesc, theBitmapPixelDesc,
						theBitmapBounds, ZPoint(0, 0));

	if (inDitherColor)
		{
		uint32 thePixval = theBitmapPixelDesc.AsPixval(*inDitherColor);
		ZDCPixmapNS::PixvalAccessor theAccessor(theBitmapRasterDesc.fPixvalDesc);
		for (ZCoord y = 0; y < inHeight; ++y)
			{
			void* rowAddress = theBitmapRasterDesc.CalcRowAddress(theBitmap->Bits(), y);
			for (ZCoord x = 0; x < inWidth; ++x)
				{
				if (((x + y) & 1) != 0)
					theAccessor.SetPixval(rowAddress, x, thePixval);
				}
			}
		}

	SetupView theSetupView(this, ioState);
	fBView->SetDrawingMode(B_OP_ALPHA);
	++fChangeCount_Mode;

	fBView->DrawBitmap(theBitmap, theBitmapBounds, theBitmapBounds + ZPoint(inLocationH + ioState.fOrigin.h, inLocationV + ioState.fOrigin.v));
	delete theBitmap;
	}

static void sBuildPolygonForLine(ZCoord inStartH, ZCoord inStartV, ZCoord inEndH, ZCoord inEndV, ZCoord inPenWidth, BPoint* outPoints)
	{
// For notes on this see AG's log book, 97-03-05
	ZPoint leftMostPoint, rightMostPoint;
	if (inStartH < inEndH)
		{
		leftMostPoint.h = inStartH;
		leftMostPoint.v = inStartV;
		rightMostPoint.h = inEndH;
		rightMostPoint.v = inEndV;
		}
	else
		{
		leftMostPoint.h = inEndH;
		leftMostPoint.v = inEndV;
		rightMostPoint.h = inStartH;
		rightMostPoint.v = inStartV;
		}
// Two cases, from top left down to bottom right, or from bottom left up to top right
	inPenWidth -= 1;
	if (leftMostPoint.v < rightMostPoint.v)
		{
		outPoints[0].x = leftMostPoint.h;
		outPoints[0].y = leftMostPoint.v;

		outPoints[1].x = leftMostPoint.h + inPenWidth;
		outPoints[1].y = leftMostPoint.v;

		outPoints[2].x = rightMostPoint.h + inPenWidth;
		outPoints[2].y = rightMostPoint.v;

		outPoints[3].x = rightMostPoint.h + inPenWidth;
		outPoints[3].y = rightMostPoint.v + inPenWidth;

		outPoints[4].x = rightMostPoint.h;
		outPoints[4].y = rightMostPoint.v + inPenWidth;

		outPoints[5].x = leftMostPoint.h;
		outPoints[5].y = leftMostPoint.v + inPenWidth;
		}
	else
		{
		outPoints[0].x = leftMostPoint.h;
		outPoints[0].y = leftMostPoint.v;

		outPoints[1].x = rightMostPoint.h;
		outPoints[1].y = rightMostPoint.v;

		outPoints[2].x = rightMostPoint.h + inPenWidth;
		outPoints[2].y = rightMostPoint.v;

		outPoints[3].x = rightMostPoint.h + inPenWidth;
		outPoints[3].y = rightMostPoint.v + inPenWidth;

		outPoints[4].x = leftMostPoint.h + inPenWidth;
		outPoints[4].y = leftMostPoint.v + inPenWidth;

		outPoints[5].x = leftMostPoint.h;
		outPoints[5].y = leftMostPoint.v + inPenWidth;
		}
	}

void ZDCCanvas_Be::Line(ZDCState& ioState, ZCoord inStartH, ZCoord inStartV, ZCoord inEndH, ZCoord inEndV)
	{
	if (!fBView)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	inStartH += ioState.fOrigin.h;
	inStartV += ioState.fOrigin.v;
	inEndH += ioState.fOrigin.h;
	inEndV += ioState.fOrigin.v;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		if (inStartH == inEndH)
			{
// Vertical line
			ZRect theRect;
			theRect.left = inStartH;
			theRect.right = inEndH + ioState.fPenWidth;
			if (inStartV < inEndV)
				{
				theRect.top = inStartV;
				theRect.bottom = inEndV + ioState.fPenWidth;
				}
			else
				{
				theRect.top = inEndV;
				theRect.bottom = inStartV + ioState.fPenWidth;
				}
			this->Internal_TileRegion(theClip & theRect, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
			}
		else if (inStartV == inEndV)
			{
// Horizontal line
			ZRect theRect;
			theRect.top = inStartV;
			theRect.bottom = inStartV + ioState.fPenWidth;
			if (inStartH < inEndH)
				{
				theRect.left = inStartH;
				theRect.right = inEndH + ioState.fPenWidth;
				}
			else
				{
				theRect.left = inEndH;
				theRect.right = inStartH + ioState.fPenWidth;
				}
			this->Internal_TileRegion(theClip & theRect, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
			}
		else
			{
			ZDCRgn theRgn = ZDCRgn::sLine(inStartH, inStartV, inEndH, inEndV, ioState.fPenWidth);
			this->Internal_TileRegion(theClip & theRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
			}
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		if (ioState.fPenWidth == 1)
			fBView->StrokeLine(BPoint(inStartH, inStartV), BPoint(inEndH, inEndV), theSetupInk.GetPattern());
		else
			{
// We're going to fill a rectangle or a polygon,
// depending on whether the line is horizontal/vertical or not
			if (inStartH == inEndH)
				{
// Vertical line
				ZRect theRect;
				if (inStartV < inEndV)
					theRect = ZRect(inStartH, inStartV, inStartH + ioState.fPenWidth, inEndV + ioState.fPenWidth);
				else
					theRect = ZRect(inStartH, inEndV, inStartH + ioState.fPenWidth, inStartV + ioState.fPenWidth);
				fBView->FillRect(theRect, theSetupInk.GetPattern());
				}
			else if (inStartV == inEndV)
				{
// Horizontal line
				ZRect theRect;
				if (inStartH < inEndH)
					theRect = ZRect(inStartH, inStartV, inEndH + ioState.fPenWidth, inStartV + ioState.fPenWidth);
				else
					theRect = ZRect(inEndH, inStartV, inStartH + ioState.fPenWidth, inStartV + ioState.fPenWidth);
				fBView->FillRect(theRect, theSetupInk.GetPattern());
				}
			else
				{
// Okay, build a polygon which will emulate a normal square pen on the Mac
				BPoint thePoints[6];
				::sBuildPolygonForLine(inStartH, inStartV, inEndH, inEndV, ioState.fPenWidth, thePoints);
				fBView->FillPolygon(thePoints, 6, theSetupInk.GetPattern());
				}
			}
		}
	}

// Rectangle
void ZDCCanvas_Be::FrameRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fBView)
		return;

	if (inRect.IsEmpty())
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realRect = inRect + ioState.fOrigin;
	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		if (realRect.Width() <= ioState.fPenWidth * 2 || realRect.Height() <= ioState.fPenWidth * 2)
			this->Internal_TileRegion(theClip & realRect, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		else
			{
// T
			this->Internal_TileRegion(theClip & ZRect(realRect.left, realRect.top, realRect.right, realRect.top + ioState.fPenWidth), *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
// L
			this->Internal_TileRegion(theClip & ZRect(realRect.left, realRect.top + ioState.fPenWidth, realRect.left + ioState.fPenWidth, realRect.bottom - ioState.fPenWidth), *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
// B
			this->Internal_TileRegion(theClip & ZRect(realRect.left, realRect.bottom - ioState.fPenWidth, realRect.right, realRect.bottom), *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
// R
			this->Internal_TileRegion(theClip & ZRect(realRect.right - ioState.fPenWidth, realRect.top + ioState.fPenWidth, realRect.right, realRect.bottom - ioState.fPenWidth), *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
			}
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		if (ioState.fPenWidth == 1)
			fBView->StrokeRect(realRect, theSetupInk.GetPattern());
		else
			{
			if (realRect.Width() <= ioState.fPenWidth * 2 || realRect.Height() <= ioState.fPenWidth * 2)
				fBView->FillRect(BRect(realRect.left, realRect.top, realRect.right - 1, realRect.bottom - 1), theSetupInk.GetPattern());
			else
				{
				pattern thePattern = theSetupInk.GetPattern();
// T
				fBView->FillRect(BRect(realRect.left, realRect.top, realRect.right - 1, realRect.top + ioState.fPenWidth - 1), thePattern);
// L
				fBView->FillRect(BRect(realRect.left, realRect.top + ioState.fPenWidth, realRect.left + ioState.fPenWidth - 1, realRect.bottom - ioState.fPenWidth - 1), thePattern);
// B
				fBView->FillRect(BRect(realRect.left, realRect.bottom - ioState.fPenWidth, realRect.right - 1, realRect.bottom - 1), thePattern);
// R
				fBView->FillRect(BRect(realRect.right - ioState.fPenWidth, realRect.top + ioState.fPenWidth, realRect.right - 1, realRect.bottom - ioState.fPenWidth - 1), thePattern);
				}
			}
		}
	}

void ZDCCanvas_Be::FillRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fBView)
		return;

	if (inRect.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realRect = inRect + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + (ioState.fClipOrigin - ioState.fOriginComp);
		this->Internal_TileRegion(theClip & realRect, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin + ioState.fOriginComp);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->FillRect(BRect(realRect.left, realRect.top, realRect.right - 1, realRect.bottom - 1), theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::InvertRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fBView)
		return;

	if (inRect.IsEmpty())
		return;

	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_INVERT);
	++fChangeCount_Mode;

	ZRect realRect = inRect + ioState.fOrigin;
	fBView->FillRect(realRect, B_SOLID_HIGH);
	}

// Region
void ZDCCanvas_Be::FrameRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fBView)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZDCRgn realRgn = inRgn + ioState.fOrigin;
	realRgn -= realRgn.Inset(ioState.fPenWidth, ioState.fPenWidth);

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->FillRegion(realRgn.GetBRegion(), theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::FillRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fBView)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZDCRgn realRgn = inRgn + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->FillRegion(realRgn.GetBRegion(), theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::InvertRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fBView)
		return;

	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_INVERT);
	++fChangeCount_Mode;

	ZDCRgn realRgn = inRgn + ioState.fOrigin;
	fBView->FillRegion(realRgn.GetBRegion(), B_SOLID_HIGH);
	}

// Round cornered rect
void ZDCCanvas_Be::FrameRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize)
	{
	if (!fBView)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realRect = inRect + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		ZDCRgn realRgn = ZDCRgn::sRoundRect(realRect, inCornerSize);
		this->Internal_TileRegion(theClip & realRect, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->StrokeRoundRect(realRect.Inset(ioState.fPenWidth / 2, ioState.fPenWidth / 2), inCornerSize.h / 2, inCornerSize.v / 2, theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::FillRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize)
	{
	if (!fBView)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realRect = inRect + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		ZDCRgn realRgn = ZDCRgn::sRoundRect(realRect, inCornerSize);
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->FillRoundRect(BRect(realRect.left, realRect.top, realRect.right - 1, realRect.bottom - 1), inCornerSize.h / 2, inCornerSize.v / 2, theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::InvertRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& cornerSize)
	{
	if (!fBView)
		return;
	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_INVERT);
	++fChangeCount_Mode;

	ZRect realRect = inRect + ioState.fOrigin;
	fBView->FillRoundRect(realRect, cornerSize.h, cornerSize.v, B_SOLID_HIGH);
	}

// Ellipse
void ZDCCanvas_Be::FrameEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fBView)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	if (inBounds.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realBounds = inBounds + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		ZDCRgn realRgn = ZDCRgn::sEllipse(realBounds) - ZDCRgn::sEllipse(realBounds.Inset(ioState.fPenWidth, ioState.fPenWidth));
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		if (ioState.fPenWidth == 1)
			fBView->StrokeEllipse(realBounds, theSetupInk.GetPattern());
		else
			{
			ZDCRgn realRgn = ZDCRgn::sEllipse(realBounds) - ZDCRgn::sEllipse(realBounds.Inset(ioState.fPenWidth, ioState.fPenWidth));
			fBView->FillRegion(realRgn.GetBRegion(), theSetupInk.GetPattern());
			}
		}
	}

void ZDCCanvas_Be::FillEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fBView)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	ZRect realBounds = inBounds + ioState.fOrigin;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		ZDCRgn realRgn = ZDCRgn::sEllipse(realBounds);
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);
		fBView->FillEllipse(realBounds, theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::InvertEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fBView)
		return;

	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_INVERT);
	++fChangeCount_Mode;

	ZRect realRect = inBounds + ioState.fOrigin;
	fBView->FillEllipse(realRect, B_SOLID_HIGH);
	}

// Poly
void ZDCCanvas_Be::FillPoly(ZDCState& ioState, const ZDCPoly& inPoly)
	{
	if (!fBView)
		return;

	ZRef<ZDCInk::Rep> theRep = ioState.fInk.GetRep();
	if (!theRep)
		return;

	if (theRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZDCRgn theClip = ioState.fClip + ioState.fClipOrigin;
		ZDCRgn realRgn = ZDCRgn::sPoly(inPoly) + ioState.fOrigin;
		this->Internal_TileRegion(theClip & realRgn, *theRep->fAsMultiColor.fPixmap, ioState.fPatternOrigin);
		}
	else
		{
		SetupView theSetupView(this, ioState);
		SetupInk theSetupInk(this, ioState);

		size_t theCount;
		BPoint* theBPoints;
		inPoly.GetBPoints(ioState.fOrigin, theBPoints, theCount);
		fBView->FillPolygon(theBPoints, (int32)theCount, theSetupInk.GetPattern());
		}
	}

void ZDCCanvas_Be::InvertPoly(ZDCState& ioState, const ZDCPoly& inPoly)
	{
	if (!fBView)
		return;

	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_INVERT);
	++fChangeCount_Mode;

	size_t theCount;
	BPoint* theBPoints;
	inPoly.GetBPoints(ioState.fOrigin, theBPoints, theCount);
	fBView->FillPolygon(theBPoints, (int32)theCount, B_SOLID_HIGH);
	}

// Pixmap
static bool sMungeProc_ApplyAlpha(ZCoord hCoord, ZCoord vCoord, ZRGBColor& inOutColor, const void* inConstRefCon, void* inRefCon)
	{
	const ZDCPixmap* theMask = reinterpret_cast<const ZDCPixmap*>(inConstRefCon);
	inOutColor.alpha = uint16(theMask->GetPixel(hCoord, vCoord).NTSCLuminance() * 0xFFFFU);
	return true;
	}

void ZDCCanvas_Be::DrawPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inSourcePixmap, const ZDCPixmap* inMaskPixmap)
	{
	if (!fBView)
		return;

	if (!inSourcePixmap)
		return;

	if (inMaskPixmap)
		{
		if (*inMaskPixmap)
			{
			// We need to ensure that we've got a source pixmap with an alpha channel,
			// so when we munge the alpha values there's somewhere to put them.
			ZDCPixmap localSourcePixmap;
			if (inSourcePixmap.GetPixelDesc().HasAlpha())
				{
				localSourcePixmap = inSourcePixmap;
				}
			else
				{
				localSourcePixmap = ZDCPixmap(inSourcePixmap.Size(), ZDCPixmapNS::eFormatEfficient_Color_32);
				localSourcePixmap.CopyFrom(ZPoint(0,0), inSourcePixmap, inSourcePixmap.Size());
				}

			localSourcePixmap.Munge(false, sMungeProc_ApplyAlpha, inMaskPixmap, nil);
	
			ZRef<ZDCPixmapRaster_Be> theRaster_Be;
			ZRect pixmapSourceRect;
			::sGetBitmapEtc(localSourcePixmap, theRaster_Be, pixmapSourceRect);
			if (!theRaster_Be)
				return;
	
			BBitmap* sourceBitmap = theRaster_Be->GetBitmap();
			ZAssertStop(kDebug_Be, sourceBitmap);
			SetupView theSetupView(this, ioState);
			fBView->SetDrawingMode(B_OP_ALPHA);
			++fChangeCount_Mode;
	
			fBView->DrawBitmap(sourceBitmap, pixmapSourceRect, pixmapSourceRect + (inLocation + ioState.fOrigin));
			}
		}
	else
		{
		ZRef<ZDCPixmapRaster_Be> theRaster_Be;
		ZRect pixmapSourceRect;
		::sGetBitmapEtc(inSourcePixmap, theRaster_Be, pixmapSourceRect);
		if (!theRaster_Be)
			return;

		BBitmap* sourceBitmap = theRaster_Be->GetBitmap();
		ZAssertStop(kDebug_Be, sourceBitmap);
		SetupView theSetupView(this, ioState);
		fBView->SetDrawingMode(B_OP_COPY);
		++fChangeCount_Mode;

		fBView->DrawBitmap(sourceBitmap, pixmapSourceRect, pixmapSourceRect + (inLocation + ioState.fOrigin));
		}
	}

// Moving blocks of pixels
void ZDCCanvas_Be::CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
// AG 98-08-06. We can't copy from an arbitrary canvas -- we can copybits within a single
// BView, and we can copy from an offscreen by calling through to it and using
// DrawBitmap. For now this will do to let MiM's windoid moving code operate.

	if (inSourceCanvas == this)
		{
		ZRect destRect = inSourceRect + (ioState.fOrigin + inDestLocation - inSourceRect.TopLeft());
		ZRect sourceRect = inSourceRect + inSourceState.fOrigin;

// We have to manage the clip ourselves. BeOS generates updates for any part of a CopyBits that is sourced outside the
// current clip, so we have to arrange things so that the destination rect intersected with the clip translated back to
// the source is unioned into the clip rgn.
		ZDCRgn destRgn = (ioState.fClip + ioState.fClipOrigin) & destRect;
		destRgn |= destRgn - (destRect.TopLeft() - sourceRect.TopLeft());
		fBView->ConstrainClippingRegion(destRgn.GetBRegion());
		++fChangeCount_Clip;

		fBView->CopyBits(sourceRect, destRect);

		if (fLooperToLock)
			fLooperToLock->Unlock();
		}
	else
		{
		if (ZRef<ZDCCanvas_Be_OffScreen> theOffScreen = ZRefDynamicCast<ZDCCanvas_Be_OffScreen>(inSourceCanvas))
			{
			SetupView theSetupView(this, ioState);
			fBView->SetDrawingMode(B_OP_COPY);
			++fChangeCount_Mode;

			theOffScreen->DrawHere(fBView, inSourceRect + inSourceState.fOrigin, inDestLocation + ioState.fOrigin);
			}
		}
	}

// Text
void ZDCCanvas_Be::DrawText(ZDCState& ioState, const ZPoint& inLocation, const char* inText, size_t textLength)
	{
	if (!fBView)
		return;
	if (textLength == 0)
		return;

	SetupView theSetupView(this, ioState);

	fBView->SetDrawingMode(B_OP_OVER);

	if (ioState.fChangeCount_Font != fChangeCount_Font)
		{
		BFont theBFont;
		ioState.fFont.GetBFont(theBFont);
		fBView->SetFont(&theBFont);
		ioState.fChangeCount_Font = ++fChangeCount_Font;
		}

	fBView->SetHighColor(ioState.fTextColor);
	++fChangeCount_Ink;
	++fChangeCount_Mode;

	ZPoint realLocation = inLocation + ioState.fOrigin;
	fBView->DrawString(inText, textLength, realLocation);
	}

ZCoord ZDCCanvas_Be::GetTextWidth(ZDCState& ioState, const char* inText, size_t textLength)
	{
	if (textLength == 0)
		return 0 ;

	BFont theBFont;
	ioState.fFont.GetBFont(theBFont);
	return ZCoord(theBFont.StringWidth(inText, textLength));
	}

void ZDCCanvas_Be::GetFontInfo(ZDCState& ioState, ZCoord& ascent, ZCoord& descent, ZCoord& leading)
	{
	font_height the_font_height;
	BFont theBFont;
	ioState.fFont.GetBFont(theBFont);
	theBFont.GetHeight(&the_font_height);
	ascent = ZCoord(the_font_height.ascent);
	descent = ZCoord(the_font_height.descent);
	leading = ZCoord(the_font_height.leading);
	}

ZCoord ZDCCanvas_Be::GetLineHeight(ZDCState& ioState)
	{
	BFont theBFont;
	ioState.fFont.GetBFont(theBFont);
	font_height the_font_height;
	theBFont.GetHeight(&the_font_height);
	return ZCoord(the_font_height.ascent + the_font_height.descent);
	}

void ZDCCanvas_Be::BreakLine(ZDCState& ioState, const char* inLineStart, size_t inRemainingTextLength, ZCoord inTargetWidth, size_t& outNextLineDelta)
	{
	if (inRemainingTextLength == 0)
		{
		outNextLineDelta = 0;
		return;
		}
	ZAssertStop(kDebug_Be, inTargetWidth >= 0);

// Get the list of escapements for the string
	float* theFloatVector = new float[inRemainingTextLength];
	BFont theBFont;
	ioState.fFont.GetBFont(theBFont);

	theBFont.GetEscapements(inLineStart, inRemainingTextLength, theFloatVector);
	float accumWidth = 0.0;
	outNextLineDelta = 0;
	while (outNextLineDelta < inRemainingTextLength)
		{
		if (inLineStart[outNextLineDelta] == '\n' || inLineStart[outNextLineDelta] == '\r')
			{
			outNextLineDelta += 1;
			delete theFloatVector;
			return;
			}
		accumWidth += theFloatVector[outNextLineDelta] * theBFont.Size();
		if (accumWidth > inTargetWidth)
			break;
		outNextLineDelta += 1;
		}
	if (accumWidth > inTargetWidth)
		{
		for (size_t x = outNextLineDelta; x > 0; --x)
			{
			char theChar = inLineStart[x - 1];
			if (isspace(theChar) || ispunct(theChar))
				{
// Found one. Use its position as the break length.
				outNextLineDelta = x;
				break;
				}
			}
		}

	if (outNextLineDelta == 0)
		outNextLineDelta = 1;
	delete theFloatVector;
	}

void ZDCCanvas_Be::InvalCache()
	{
	++fChangeCount_Ink;
	++fChangeCount_Mode;
	++fChangeCount_Clip;
	++fChangeCount_Font;
	}

void ZDCCanvas_Be::Sync()
	{
	if (fBView)
		{
		if (fLooperToLock)
			fLooperToLock->Lock();
		fBView->Sync();
		if (fLooperToLock)
			fLooperToLock->Unlock();
		}
	}

void ZDCCanvas_Be::Flush()
	{
	if (fBView)
		{
		if (fLooperToLock)
			fLooperToLock->Lock();
		fBView->Flush();
		if (fLooperToLock)
			fLooperToLock->Unlock();
		}
	}

short ZDCCanvas_Be::GetDepth()
	{
// Dummy for now
	return 8;
	}

bool ZDCCanvas_Be::IsOffScreen()
	{ return false; }

bool ZDCCanvas_Be::IsPrinting()
	{ return false; }

ZRef<ZDCCanvas> ZDCCanvas_Be::CreateOffScreen(const ZRect& inCanvasRect)
	{
	return this->CreateOffScreen(inCanvasRect.Size(), ZDCPixmapNS::eFormatEfficient_Color_32);
	}

ZRef<ZDCCanvas> ZDCCanvas_Be::CreateOffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat)
	{
// No good till we have CopyFrom implemented
	return new ZDCCanvas_Be_OffScreen(inDimensions, iFormat);
	}

void ZDCCanvas_Be::Internal_TileRegion(const ZDCRgn& inRgn, const ZDCPixmap& inPixmap, ZPoint inRealPatternOrigin)
	{
	ZRef<ZDCPixmapRaster_Be> theRaster_Be;
	ZRect pixmapSourceRect;
	::sGetBitmapEtc(inPixmap, theRaster_Be, pixmapSourceRect);
	if (!theRaster_Be)
		return;

	BBitmap* theBitmap = theRaster_Be->GetBitmap();
	ZAssertStop(kDebug_Be, theBitmap);

	if (fLooperToLock)
		fLooperToLock->Lock();

	ZDCRgn localClipRgn(inRgn);
	fBView->ConstrainClippingRegion(localClipRgn.GetBRegion());
	fBView->SetDrawingMode(B_OP_COPY);
	++fChangeCount_Clip;
	++fChangeCount_Mode;

	ZPoint pixmapSize = pixmapSourceRect.Size();
	ZRect drawnBounds = inRgn.Bounds();
	ZCoord drawnOriginH = drawnBounds.left - ((((drawnBounds.left + inRealPatternOrigin.h) % pixmapSize.h) + pixmapSize.h) % pixmapSize.h);
	ZCoord drawnOriginV = drawnBounds.top - ((((drawnBounds.top + inRealPatternOrigin.v) % pixmapSize.v) + pixmapSize.v) % pixmapSize.v);

	for (ZCoord y = drawnOriginV; y < drawnBounds.bottom; y += pixmapSize.v)
		{
		for (ZCoord x = drawnOriginH; x < drawnBounds.right; x += pixmapSize.h)
			{
			ZRect destBounds(x, y, x + pixmapSize.h, y + pixmapSize.v);
			fBView->DrawBitmapAsync(theBitmap, pixmapSourceRect, destBounds);
			}
		}
	fBView->Sync();

	if (fLooperToLock)
		fLooperToLock->Unlock();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_NonWindow

ZDCCanvas_Be_NonWindow::ZDCCanvas_Be_NonWindow()
	{}

ZDCCanvas_Be_NonWindow::~ZDCCanvas_Be_NonWindow()
	{}

void ZDCCanvas_Be_NonWindow::Finalize()
	{
	if (fLooperToLock)
		fLooperToLock->Lock();

	if (this->GetRefCount() > 1)
		{
		this->FinalizationComplete();
		if (fLooperToLock)
			fLooperToLock->Unlock();
		}
	else
		{
		this->FinalizationComplete();
		if (fLooperToLock)
			fLooperToLock->Unlock();
		delete this;
		}
	}

void ZDCCanvas_Be_NonWindow::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fBView)
		return;
	if (hDelta == 0 && vDelta == 0)
		return;

	ZPoint offset(hDelta, vDelta);

	ZDCRgn destRgn = ((ioState.fClip + ioState.fClipOrigin) & (inRect + ioState.fOrigin));

// srcRgn is the set of pixels we're want and are able to copy from.
	ZDCRgn srcRgn = ((destRgn - offset) & (inRect + ioState.fOrigin));

// drawnRgn is the destination pixels that will be drawn by the CopyBits call, it's the srcRgn
// shifted back to the destination.
	ZDCRgn drawnRgn = srcRgn + offset;

// And set the clip (drawnRgn)
	fBView->ConstrainClippingRegion(drawnRgn.GetBRegion());
	++fChangeCount_Clip;

	ZRect drawnBounds = drawnRgn.Bounds();
	fBView->CopyBits(drawnBounds - offset, drawnBounds);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_OffScreen

struct
	{
	color_space fColorSpace;
	uint32 fMaskRed;
	uint32 fMaskGreen;
	uint32 fMaskBlue;
	uint32 fMaskAlpha;
	int32 fDepth;
	bool fBigEndian;
	}
sColorSpaceToDescColor[] =
	{
	{ B_RGB15_BIG, 0x7C00, 0x03E0, 0x001F, 0x0000, 16, true},
	{ B_RGB15_LITTLE, 0x7C00, 0x03E0, 0x001F, 0x0000, 16, false},
	{ B_RGBA15_BIG, 0x7C00, 0x03E0, 0x001F, 0x8000, 16, true},
	{ B_RGBA15_LITTLE, 0x7C00, 0x03E0, 0x001F, 0x8000, 16, false},
	{ B_RGB16_BIG, 0xF800, 0x07E0, 0x001F, 0x0000, 16, true},
	{ B_RGB16_LITTLE, 0xF800, 0x07E0, 0x001F, 0x0000, 16, false},
	{ B_RGB24_BIG, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 24, true},
	{ B_RGB24_LITTLE, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 24, false},
	{ B_RGB32_BIG, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 32, true},
	{ B_RGB32_LITTLE, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 32, false},
	{ B_RGBA32_BIG, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 32, true},
	{ B_RGBA32_LITTLE, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 32, false}
	};

ZDCCanvas_Be_OffScreen::ZDCCanvas_Be_OffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat)
	{
// Always use 32 bits for now
	fBitmap = new BBitmap(ZRect(inDimensions), B_RGB32, true, false);
	fBView = new BView(ZRect(inDimensions), "OffScreen View", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fBitmap->AddChild(fBView);
	ZRef<ZDCPixmapRaster_Be> theRaster = new ZDCPixmapRaster_Be(fBitmap);
	fAsPixmap = new ZDCPixmap(new ZDCPixmapRep(theRaster, ZRect(inDimensions), sPixelDescFromColorSpace(fBitmap->ColorSpace())));
	fLooperToLock = fBView->Window();
	}

ZDCCanvas_Be_OffScreen::~ZDCCanvas_Be_OffScreen()
	{
// Deleting the pixmap will also delete our bitmap, and our view
	delete fAsPixmap;
	fAsPixmap = nil;
	fBitmap = nil;
	fBView = nil;
	fLooperToLock = nil;
	}

ZRGBColor ZDCCanvas_Be_OffScreen::GetPixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV)
	{ return fAsPixmap->GetPixel(inLocationH + ioState.fOrigin.h, inLocationV + ioState.fOrigin.v); }

ZDCPixmap ZDCCanvas_Be_OffScreen::GetPixmap(ZDCState& ioState, const ZRect& inBounds)
	{
	if (inBounds.IsEmpty())
		return ZDCPixmap();

	return ZDCPixmap(*fAsPixmap, inBounds + ioState.fOrigin);
	}

short ZDCCanvas_Be_OffScreen::GetDepth()
	{ return 32; }

bool ZDCCanvas_Be_OffScreen::IsOffScreen()
	{ return true; }

void ZDCCanvas_Be_OffScreen::DrawHere(BView* inBView, const ZRect& inSourceRect, const ZPoint& inDestPoint)
	{
	ZRect realDest(inSourceRect.Size());
	realDest += inDestPoint;
	fBitmap->Lock();
	fBView->Sync();
	fBitmap->Unlock();
	inBView->DrawBitmap(fBitmap, inSourceRect, realDest);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRaster_Be

ZDCPixmapRaster_Be::ZDCPixmapRaster_Be(BBitmap* inBitmap)
	{
	fBitmap = inBitmap;
	fBaseAddress = fBitmap->Bits();
	fRasterDesc.fRowBytes = fBitmap->BytesPerRow();
	fRasterDesc.fRowCount = size_t(fBitmap->Bounds().bottom + 1);
	fRasterDesc.fFlipped = false;
	color_space theColorSpace = fBitmap->ColorSpace();
	for (size_t x = 0; x < countof(sColorSpaceToDescColor); ++x)
		{
		if (theColorSpace == sColorSpaceToDescColor[x].fColorSpace)
			{
			fRasterDesc.fPixvalDesc.fDepth = sColorSpaceToDescColor[x].fDepth;
			fRasterDesc.fPixvalDesc.fBigEndian = sColorSpaceToDescColor[x].fBigEndian;
			return;
			}
		}
	ZDebugStopf(0, ("Unsupported color space: %d", theColorSpace));
	}

ZDCPixmapRaster_Be::~ZDCPixmapRaster_Be()
	{
// Don't delete the bitmap if we have no app -- there's no app_server connection in place
	if (be_app)
		delete fBitmap;
	fBaseAddress = nil;
	}

BBitmap* ZDCPixmapRaster_Be::GetBitmap()
	{
	return fBitmap;
	}

// =================================================================================================
#pragma mark -
#pragma mark * Rep conversion/coercion utilities

static ZRef<ZDCPixmapRaster_Be> sCreateRasterForDesc(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZDCPixmapNS::PixelDesc& inPixelDesc, ZDCPixmapNS::PixelDesc& outPixelDesc)
	{
	using namespace ZDCPixmapNS;

	if (inRasterDesc.fFlipped || ((inRasterDesc.fRowBytes % 4) != 0))
		return nil;

	color_space theColorSpace = B_NO_COLOR_SPACE;

	if (PixelDescRep_Color* thePixelDescRep_Color = ZRefDynamicCast<PixelDescRep_Color>(inPixelDesc.GetRep()))
		{
		uint32 maskRed, maskGreen, maskBlue, maskAlpha;
		thePixelDescRep_Color->GetMasks(maskRed, maskGreen, maskBlue, maskAlpha);

		for (size_t x = 0; x < countof(sColorSpaceToDescColor); ++x)
			{
			if (inRasterDesc.fPixvalDesc.fDepth == sColorSpaceToDescColor[x].fDepth)
				{
				if (inRasterDesc.fPixvalDesc.fBigEndian == sColorSpaceToDescColor[x].fBigEndian
						&& maskRed == sColorSpaceToDescColor[x].fMaskRed
						&& maskGreen == sColorSpaceToDescColor[x].fMaskGreen
						&& maskBlue == sColorSpaceToDescColor[x].fMaskBlue
						&& maskAlpha == sColorSpaceToDescColor[x].fMaskAlpha)
					{
					theColorSpace = sColorSpaceToDescColor[x].fColorSpace;
					break;
					}
				if (inRasterDesc.fPixvalDesc.fBigEndian != sColorSpaceToDescColor[x].fBigEndian
						&& maskRed == ZByteSwap_32(sColorSpaceToDescColor[x].fMaskRed)
						&& maskGreen == ZByteSwap_32(sColorSpaceToDescColor[x].fMaskGreen)
						&& maskBlue == ZByteSwap_32(sColorSpaceToDescColor[x].fMaskBlue)
						&& maskAlpha == ZByteSwap_32(sColorSpaceToDescColor[x].fMaskAlpha))
					{
					theColorSpace = sColorSpaceToDescColor[x].fColorSpace;
					break;
					}
				}
			}
		}
	else if (PixelDescRep_Gray* thePixelDescRep_Gray = ZRefDynamicCast<PixelDescRep_Gray>(inPixelDesc.GetRep()))
		{
		uint32 maskGray, maskAlpha;
		thePixelDescRep_Gray->GetMasks(maskGray, maskAlpha);
		if (inRasterDesc.fPixvalDesc.fDepth == 1 && inRasterDesc.fPixvalDesc.fBigEndian && maskGray == 0x01)
			theColorSpace = B_GRAY1;
		}

	if (theColorSpace != B_NO_COLOR_SPACE)
		{
		ZPoint theBitmapSize;
		theBitmapSize.v = inRasterDesc.fRowCount;
		if (inRasterDesc.fPixvalDesc.fDepth < 8)
			theBitmapSize.h = inRasterDesc.fRowBytes * (8 / inRasterDesc.fPixvalDesc.fDepth);
		else
			theBitmapSize.h = inRasterDesc.fRowBytes / (inRasterDesc.fPixvalDesc.fDepth / 8);
		BBitmap* theBitmap = new BBitmap(ZRect(theBitmapSize), theColorSpace);
		outPixelDesc = sPixelDescFromColorSpace(theBitmap->ColorSpace());
		return new ZDCPixmapRaster_Be(theBitmap);
		}

	return ZRef<ZDCPixmapRaster_Be>();
	}

static ZDCPixmapNS::PixelDesc sPixelDescFromColorSpace(color_space inColorSpace)
	{
	for (size_t x = 0; x < countof(sColorSpaceToDescColor); ++x)
		{
		if (sColorSpaceToDescColor[x].fColorSpace == inColorSpace)
			return ZDCPixmapNS::PixelDesc(sColorSpaceToDescColor[x].fMaskRed,
											sColorSpaceToDescColor[x].fMaskGreen,
											sColorSpaceToDescColor[x].fMaskBlue,
											sColorSpaceToDescColor[x].fMaskAlpha);
		}
	ZDebugStopf(0, ("Unsupported color space: %d", inColorSpace));
	return ZDCPixmapNS::PixelDesc();
	}

static void sGetBitmapEtc(const ZDCPixmap& inPixmap, ZRef<ZDCPixmapRaster_Be>& outRaster_Be, ZRect& outPixmapSourceRect)
	{
	if (outRaster_Be = ZRefDynamicCast<ZDCPixmapRaster_Be>(inPixmap.GetRaster()))
		{
		outPixmapSourceRect = inPixmap.GetRep()->GetBounds();
		}
	else
		{
		ZPoint theSize = inPixmap.Size();
		outRaster_Be = new ZDCPixmapRaster_Be(new BBitmap(ZRect(theSize), B_RGBA32_LITTLE));
		inPixmap.CopyTo(ZPoint(0, 0), outRaster_Be->GetBaseAddress(), outRaster_Be->GetRasterDesc(), sPixelDescFromColorSpace(B_RGBA32_LITTLE), ZRect(theSize));
		outPixmapSourceRect = theSize;
		}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapFactory_Be

class ZDCPixmapFactory_Be : public ZDCPixmapFactory
	{
public:
	virtual ZRef<ZDCPixmapRep> CreateRep(const ZRef<ZDCPixmapRaster>& inRaster, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc);
	virtual ZRef<ZDCPixmapRep> CreateRep(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc);
	virtual ZDCPixmapNS::EFormatStandard MapEfficientToStandard(ZDCPixmapNS::EFormatEfficient inFormat);
	};

static ZDCPixmapFactory_Be sZDCPixmapFactory_Be;

ZRef<ZDCPixmapRep> ZDCPixmapFactory_Be::CreateRep(const ZRef<ZDCPixmapRaster>& inRaster, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	return new ZDCPixmapRep(inRaster, inBounds, inPixelDesc);
	}

ZRef<ZDCPixmapRep> ZDCPixmapFactory_Be::CreateRep(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	ZDCPixmapNS::PixelDesc realPixelDesc;
	if (ZRef<ZDCPixmapRaster> theRaster = ::sCreateRasterForDesc(inRasterDesc, inPixelDesc, realPixelDesc))
		return new ZDCPixmapRep(theRaster, inBounds, realPixelDesc);

	return new ZDCPixmapRep(new ZDCPixmapRaster_Simple(inRasterDesc), inBounds, inPixelDesc);
	}

ZDCPixmapNS::EFormatStandard ZDCPixmapFactory_Be::MapEfficientToStandard(ZDCPixmapNS::EFormatEfficient inFormat)
	{
	using namespace ZDCPixmapNS;

	EFormatStandard standardFormat = eFormatStandard_Invalid;
	switch (inFormat)
		{
//		case eFormatEfficient_Indexed_1: standardFormat = eFormatStandard_Indexed_1; break;

		case eFormatEfficient_Gray_1: standardFormat = eFormatStandard_Gray_1; break;

		#if ZCONFIG(Endian, Big)

		case eFormatEfficient_Color_16: standardFormat = eFormatStandard_ARGB_16_BE; break;
		case eFormatEfficient_Color_24: standardFormat = eFormatStandard_RGB_24; break;
		case eFormatEfficient_Color_32: standardFormat = eFormatStandard_ARGB_32; break;

		#else

		case eFormatEfficient_Color_16: standardFormat = eFormatStandard_ARGB_16_LE; break;
		case eFormatEfficient_Color_24: //standardFormat = eFormatStandard_BGR_24; break;
		case eFormatEfficient_Color_32: standardFormat = eFormatStandard_BGRA_32; break;

		#endif
		}

	return standardFormat;
	}

#endif // ZCONFIG(API_Graphics, Be)
