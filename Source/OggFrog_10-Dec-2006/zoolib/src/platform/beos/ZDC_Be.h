/*  @(#) $Id: ZDC_Be.h,v 1.8 2006/04/10 20:44:20 agreen Exp $ */

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

#ifndef __ZDC_Be__
#define __ZDC_Be__ 1
#include "zconfig.h"

#if ZCONFIG(API_Graphics, Be)

#include "ZDC.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be

class ZDCCanvas_Be : public ZDCCanvas
	{
protected:
	ZDCCanvas_Be();
	virtual ~ZDCCanvas_Be();

// Force subclasses to implement Finalize
	virtual void Finalize() = 0;

// From ZDCCanvas
// Simple pixel and lines
	virtual void Pixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, const ZRGBColor& inColor);

	virtual void Pixels(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, ZCoord inWidth, ZCoord inHeight,
									const char* inPixvals, const ZRGBColorMap* inColorMap, size_t inColorMapSize,
									const ZRGBColorPOD* inDitherColor);

	virtual void Line(ZDCState& ioState, ZCoord inStartH, ZCoord inStartV, ZCoord inEndH, ZCoord inEndV);

// Rectangle
	virtual void FrameRect(ZDCState& ioState, const ZRect& inRect);
	virtual void FillRect(ZDCState& ioState, const ZRect& inRect);
	virtual void InvertRect(ZDCState& ioState, const ZRect& inRect);

// Region
	virtual void FrameRegion(ZDCState& ioState, const ZDCRgn& inRgn);
	virtual void FillRegion(ZDCState& ioState, const ZDCRgn& inRgn);
	virtual void InvertRegion(ZDCState& ioState, const ZDCRgn& inRgn);

// Round cornered rect
	virtual void FrameRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);
	virtual void FillRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);
	virtual void InvertRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);

// Ellipse
	virtual void FrameEllipse(ZDCState& ioState, const ZRect& inBounds);
	virtual void FillEllipse(ZDCState& ioState, const ZRect& inBounds);
	virtual void InvertEllipse(ZDCState& ioState, const ZRect& inBounds);

// Poly
	virtual void FillPoly(ZDCState& ioState, const ZDCPoly& inPoly);
	virtual void InvertPoly(ZDCState& ioState, const ZDCPoly& inPoly);

// Pixmap
	virtual void DrawPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inSourcePixmap, const ZDCPixmap* inMaskPixmap);

// Moving blocks of pixels
	virtual void CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect);

// Text
	virtual void DrawText(ZDCState& ioState, const ZPoint& inLocation, const char* inText, size_t inTextLength);

	virtual ZCoord GetTextWidth(ZDCState& ioState, const char* inText, size_t inTextLength);
	virtual void GetFontInfo(ZDCState& ioState, ZCoord& outAscent, ZCoord& outDescent, ZCoord& outLeading);
	virtual ZCoord GetLineHeight(ZDCState& ioState);
	virtual void BreakLine(ZDCState& ioState, const char* inLineStart, size_t inRemainingTextLength, ZCoord inTargetWidth, size_t& outNextLineDelta);

// Escape hatch
	virtual void InvalCache();

// Syncing
	virtual void Sync();
	virtual void Flush();

	virtual short GetDepth();
	virtual bool IsOffScreen();
	virtual bool IsPrinting();

// OffScreen
	virtual ZRef<ZDCCanvas> CreateOffScreen(const ZRect& inCanvasRect);
	virtual ZRef<ZDCCanvas> CreateOffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat);

// ----------
// Internal or non-virtual implementation stuff
	void Internal_TileRegion(const ZDCRgn& inRgn, const ZDCPixmap& inPixmap, ZPoint inPatternOrigin);

	long fChangeCount_Ink;
	long fChangeCount_Mode;
	long fChangeCount_Clip;
	long fChangeCount_Font;

	BView* fBView;
	BLooper* fLooperToLock;

	class SetupView;
	friend SetupView;
	class SetupInk;
	friend SetupInk;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_NonWindow

class ZDCCanvas_Be_NonWindow : public ZDCCanvas_Be
	{
public:
	ZDCCanvas_Be_NonWindow();
	virtual ~ZDCCanvas_Be_NonWindow();

	virtual void Finalize();

	virtual void Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_OffScreen

class ZDCCanvas_Be_OffScreen : public ZDCCanvas_Be_NonWindow
	{
public:
	ZDCCanvas_Be_OffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat);
	virtual ~ZDCCanvas_Be_OffScreen();

	virtual ZRGBColor GetPixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV);
	virtual ZDCPixmap GetPixmap(ZDCState& ioState, const ZRect& inBounds);
	virtual short GetDepth();
	virtual bool IsOffScreen();

	void DrawHere(BView* inBView, const ZRect& inSourceRect, const ZPoint& inDestPoint);

protected:
	BBitmap* fBitmap;
	ZDCPixmap* fAsPixmap;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be::SetupView

class ZDCCanvas_Be::SetupView
	{
public:
	SetupView(ZDCCanvas_Be* inCanvas, ZDCState& ioState);
	~SetupView();

protected:
	ZDCCanvas_Be* fCanvas;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRaster_Be

class ZDCPixmapRaster_Be : public ZDCPixmapRaster
	{
public:
	ZDCPixmapRaster_Be(BBitmap* inBitmap);
	virtual ~ZDCPixmapRaster_Be();

	BBitmap* GetBitmap();
private:
	BBitmap* fBitmap;
	};

// =================================================================================================

#endif // ZCONFIG(API_Graphics, Be)
#endif // __ZDC_Be__
