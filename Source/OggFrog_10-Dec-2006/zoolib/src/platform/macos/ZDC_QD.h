/*  @(#) $Id: ZDC_QD.h,v 1.10 2006/07/15 20:54:22 goingware Exp $ */

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

#ifndef __ZDC_QD__
#define __ZDC_QD__ 1
#include "zconfig.h"

#include "ZDC.h"

#if ZCONFIG(API_Graphics, QD)

#include "ZUtil_Mac_LL.h"

#if ZCONFIG(OS, MacOSX)
#	include <QD/QDOffscreen.h>
#else
#	include <QDOffscreen.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD

class ZDCCanvas_QD_OffScreen;
class ZDCSetupForQD;
class ZDCPixmapRep_QD;

class ZDCCanvas_QD : public ZDCCanvas
	{
public:
	static ZRef<ZDCCanvas_QD> sFindCanvasOrCreateNative(CGrafPtr inGrafPtr);

protected:
// Linked list of all canvases
	void Internal_Link(CGrafPtr inGrafPtr);
	void Internal_Unlink();
	static ZMutex sMutex_List;
	static ZDCCanvas_QD* sCanvas_Head;
	ZDCCanvas_QD* fCanvas_Prev;
	ZDCCanvas_QD* fCanvas_Next;

protected:
	ZDCCanvas_QD();
	virtual ~ZDCCanvas_QD();

public:
// Force subclasses to implement Finalize
	virtual void Finalize() = 0;


// From ZDCCanvas
// Simple pixel and lines
	virtual void Pixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, const ZRGBColor& inColor);

	virtual void Line(ZDCState& ioState, ZCoord inStartH, ZCoord inStartV, ZCoord inEndH, ZCoord inEndV);

// Rectangle
	virtual void FrameRect(ZDCState& ioState, const ZRect& inRect);
	virtual void FillRect(ZDCState& ioState, const ZRect& inRect);
	virtual void InvertRect(ZDCState& ioState, const ZRect& inRect);
	virtual void HiliteRect(ZDCState& ioState, const ZRect& inRect, const ZRGBColor& inBackColor);

// Region
	virtual void FrameRegion(ZDCState& ioState, const ZDCRgn& inRgn);
	virtual void FillRegion(ZDCState& ioState, const ZDCRgn& inRgn);
	virtual void InvertRegion(ZDCState& ioState, const ZDCRgn& inRgn);
	virtual void HiliteRegion(ZDCState& ioState, const ZDCRgn& inRgn, const ZRGBColor& inBackColor);

// Round cornered rect
	virtual void FrameRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);
	virtual void FillRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);
	virtual void InvertRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize);
	virtual void HiliteRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize, const ZRGBColor& inBackColor);

// Ellipse
	virtual void FrameEllipse(ZDCState& ioState, const ZRect& inBounds);
	virtual void FillEllipse(ZDCState& ioState, const ZRect& inBounds);
	virtual void InvertEllipse(ZDCState& ioState, const ZRect& inBounds);
	virtual void HiliteEllipse(ZDCState& ioState, const ZRect& inBounds, const ZRGBColor& inBackColor);

// Poly
	virtual void FillPoly(ZDCState& ioState, const ZDCPoly& inPoly);
	virtual void InvertPoly(ZDCState& ioState, const ZDCPoly& inPoly);
	virtual void HilitePoly(ZDCState& ioState, const ZDCPoly& inPoly, const ZRGBColor& inBackColor);

// Pixmap
	virtual void FillPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inPixmap);
	virtual void DrawPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inSourcePixmap, const ZDCPixmap* inMaskPixmap);

// Flood Fill
	virtual void FloodFill(ZDCState& ioState, const ZPoint& inSeedLocation);

// Text
	virtual void DrawText(ZDCState& ioState, const ZPoint& theLocation, const char* theText, size_t textLength);

	virtual ZCoord GetTextWidth(ZDCState& ioState, const char* theText, size_t textLength);
	virtual void GetFontInfo(ZDCState& ioState, ZCoord& outAscent, ZCoord& outDescent, ZCoord& outLeading);
	virtual ZCoord GetLineHeight(ZDCState& ioState);
	virtual void BreakLine(ZDCState& ioState, const char* inLineStart, size_t inRemainingTextLength, ZCoord inTargetWidth, size_t& outNextLineDelta);

	virtual void InvalCache();

// Syncing
	virtual void Sync();
	virtual void Flush();

// Info
	virtual short GetDepth();
	virtual bool IsOffScreen();
	virtual bool IsPrinting();

// OffScreen
	virtual ZRef<ZDCCanvas> CreateOffScreen(const ZRect& inCanvasRect);
	virtual ZRef<ZDCCanvas> CreateOffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat);

// ----------
// Internal or non-virtual implementation stuff
	ZPoint Internal_QDStart(ZDCState& ioState, bool inUsingPatterns);
	void Internal_QDEnd();

	virtual ZDCRgn Internal_CalcClipRgn(const ZDCState& inState) = 0;
	virtual ZDCRgn Internal_GetExcludeRgn();

	CGrafPtr Internal_GetGrafPtr() { return fGrafPtr; }

	void Internal_SetupPortReal(ZDCState& ioState, bool usingPatterns);
	void Internal_SetupInk(ZDCState& ioState);
	void Internal_SetupText(ZDCState& ioState);

	void Internal_TileRegion(ZDCState& ioState, const ZDCRgn& inRgn, ZRef<ZDCPixmapRep_QD> inPixmapRep);

protected:
	long fChangeCount_Origin;
	long fChangeCount_PatternOrigin;
	long fChangeCount_Ink;
	long fChangeCount_PenWidth;
	long fChangeCount_Mode;
	long fChangeCount_Clip;
	long fChangeCount_Font;

	CGrafPtr fGrafPtr;

	ZMutex* fMutexToLock;
	ZMutex* fMutexToCheck;

	PixPatHandle fPixPatHandle;

	class SetupPort;
	friend class SetupPort;
	class SetupInk;
	friend class SetupInk;
	class SetupText;
	friend class SetupText;

	friend class ZDCSetupForQD;
	friend class ZDCCanvas_QD_OffScreen;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_NonWindow

class ZDCCanvas_QD_NonWindow : public ZDCCanvas_QD
	{
public:
	ZDCCanvas_QD_NonWindow();
	virtual ~ZDCCanvas_QD_NonWindow();

	virtual void Finalize();
	virtual void Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta);
	virtual void CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect);
	virtual ZDCRgn Internal_CalcClipRgn(const ZDCState& inState);

protected:
	ZMutex fMutex;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_OffScreen

class ZDCCanvas_QD_OffScreen : public ZDCCanvas_QD_NonWindow
	{
public:
	ZDCCanvas_QD_OffScreen(const ZRect& inGlobalRect);
	ZDCCanvas_QD_OffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat);
	virtual ~ZDCCanvas_QD_OffScreen();

	virtual ZRGBColor GetPixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV);

	virtual ZDCPixmap GetPixmap(ZDCState& ioState, const ZRect& inBounds);

protected:
	GWorldPtr fGWorldPtr;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_Print

class ZDCCanvas_QD_Print : public ZDCCanvas_QD_NonWindow
	{
public:
	ZDCCanvas_QD_Print(GrafPtr inGrafPtr);
	virtual ~ZDCCanvas_QD_Print();

	virtual bool IsPrinting();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDC_PICT

class ZStreamW;
class ZDC_PICT : public ZDC
	{
public:
	ZDC_PICT(const ZRect& inBounds, const ZStreamW& inStream);
	~ZDC_PICT();
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDC_NativeQD

class ZDC_NativeQD : public ZDC
	{
public:
	ZDC_NativeQD(CGrafPtr inGrafPtr);
	~ZDC_NativeQD();

protected:
	CGrafPtr fSavedGrafPtr;
	GDHandle fSavedGDHandle;

	CGrafPtr fGrafPtr;

	ZDCRgn fOldClip;
	ZPoint fOldOrigin;
	RGBColor fOldForeColor;
	RGBColor fOldBackColor;
	PenState fOldState;
	short fOldTextMode;
	short fOldTextSize;
	short fOldTextFont;
	short fOldTextFace;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCSetupForQD

class ZDCSetupForQD
	{
public:
	ZDCSetupForQD(const ZDC& inDC, bool inUsingPatterns);
	~ZDCSetupForQD();
	ZPoint GetOffset();
	GrafPtr GetGrafPtr();
	CGrafPtr GetCGrafPtr();

protected:
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	ZUtil_Mac_LL::PreserveCurrentPort fPCP;
#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	ZRef<ZDCCanvas_QD> fCanvas;
	ZPoint fOffset;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRep_QD

class ZDCPixmapRep_QD : public ZDCPixmapRep
	{
public:
	ZDCPixmapRep_QD(ZRef<ZDCPixmapRaster> inRaster,
								const ZRect& inBounds,
								const ZDCPixmapNS::PixelDesc& inPixelDesc);

	ZDCPixmapRep_QD(const PixMap* inSourcePixMap, const void* inSourceData, const ZRect& inSourceBounds);
	ZDCPixmapRep_QD(const BitMap* inSourceBitMap, const void* inSourceData, const ZRect& inSourceBounds, bool inInvert);

	virtual ~ZDCPixmapRep_QD();

	const PixMap& GetPixMap();

	static ZRef<ZDCPixmapRep_QD> sAsPixmapRep_QD(ZRef<ZDCPixmapRep> inRep);

protected:
	PixMap fPixMap;
	int fChangeCount;
	};

// =================================================================================================

#endif // ZCONFIG(API_Graphics, QD)

#endif // __ZDC_QD__
