static const char rcsid[] = "@(#) $Id: ZDC_QD.cpp,v 1.23 2006/10/13 20:32:01 agreen Exp $";

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

#include "ZDC_QD.h"

#if ZCONFIG(API_Graphics, QD)

#include "ZDCInk.h"
#include "ZMacOSX.h"
#include "ZStream.h"
#include "ZUtil_Mac_LL.h"

static short sModeLookup[] = { srcCopy, srcOr, srcXor, srcBic};

// If we're using the old headers then MacLineTo won't be defined
#if !defined(TARGET_OS_MAC) || (TARGET_OS_MAC && !defined(MacLineTo))
#	define MacLineTo LineTo
#	define MacFrameRect FrameRect
#	define MacInvertRect InvertRect
#	define MacFrameRgn FrameRgn
#	define MacInvertRgn InvertRgn
#	define MacPaintRgn PaintRgn
#	define MacDrawText DrawText
#	define MacOffsetRgn OffsetRgn
#endif

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/LowMem.h>
#	include <CarbonCore/FixMath.h>
#	include <CarbonCore/TextUtils.h>
#else
#	include <LowMem.h> // For LMSetHiliteMode
#	include <FixMath.h> // For Long2Fix
#	include <TextUtils.h> // For StyledLineBreak stuff
#endif

static inline void sUseHiliteColor()
	{
	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
		::LMSetHiliteMode(0);
	#endif
	}

static inline const BitMap* sGetPortBitMapForCopyBits(CGrafPtr iGrafPtr)
	{
	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		return ::GetPortBitMapForCopyBits(iGrafPtr);
	#else
		return &reinterpret_cast<GrafPtr>(iGrafPtr)->portBits;
	#endif
	}

static inline ZPoint sGetPortOrigin(CGrafPtr inGrafPtr)
	{
	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		Rect tempRect;
		::GetPortBounds(inGrafPtr, &tempRect);
		return ZPoint(tempRect.left, tempRect.top);
	#else
		return ZPoint(inGrafPtr->portRect.left, inGrafPtr->portRect.top);
	#endif
	}

static inline bool sIsPortOffscreen(CGrafPtr iGrafPtr)
	{
	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		return ::IsPortOffscreen(iGrafPtr);
	#else
		return reinterpret_cast<CGrafPort*>(iGrafPtr)->portVersion & 0x0001;
	#endif
	}

// Forward declarations of utility methods
static ZRef<ZDCPixmapRep_QD> sGetPixmapRep_QDIfComplex(const ZRef<ZDCInk::Rep>& inInkRep);

// =================================================================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_QD 2

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD::SetupPort

class ZDCCanvas_QD::SetupPort
	{
public:
	SetupPort(ZDCCanvas_QD* inCanvas, ZDCState& ioState);
	~SetupPort();

protected:
#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort fPCP;
#endif
	ZDCCanvas_QD* fCanvas;
	};

ZDCCanvas_QD::SetupPort::SetupPort(ZDCCanvas_QD* inCanvas, ZDCState& ioState)
	{
	fCanvas = inCanvas;
	ZAssertStop(kDebug_QD, fCanvas->fMutexToCheck == nil || fCanvas->fMutexToCheck->IsLocked());
	fCanvas->fMutexToLock->Acquire();

	::SetGWorld(fCanvas->fGrafPtr, nil);
	if (ioState.fChangeCount_Origin != inCanvas->fChangeCount_Origin ||
		ioState.fChangeCount_PatternOrigin != inCanvas->fChangeCount_PatternOrigin ||
		ioState.fChangeCount_Clip != inCanvas->fChangeCount_Clip)
		inCanvas->Internal_SetupPortReal(ioState, true);
	}

ZDCCanvas_QD::SetupPort::~SetupPort()
	{
	fCanvas->fMutexToLock->Release();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD::SetupInk

class ZDCCanvas_QD::SetupInk
	{
public:
	SetupInk(ZDCCanvas_QD* inCanvas, ZDCState& ioState);
	~SetupInk();
	};

ZDCCanvas_QD::SetupInk::SetupInk(ZDCCanvas_QD* inCanvas, ZDCState& ioState)
	{
	inCanvas->Internal_SetupInk(ioState);
	}

ZDCCanvas_QD::SetupInk::~SetupInk()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD::SetupText

class ZDCCanvas_QD::SetupText
	{
public:
	SetupText(ZDCCanvas_QD* inCanvas, ZDCState& ioState);
	~SetupText();
	};

ZDCCanvas_QD::SetupText::SetupText(ZDCCanvas_QD* inCanvas, ZDCState& ioState)
	{
	inCanvas->Internal_SetupText(ioState);
	}

ZDCCanvas_QD::SetupText::~SetupText()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_Native

class ZDCCanvas_QD_Native : public ZDCCanvas_QD_NonWindow
	{
public:
	ZDCCanvas_QD_Native(CGrafPtr inGrafPtr);
	virtual ~ZDCCanvas_QD_Native();
	};

ZDCCanvas_QD_Native::ZDCCanvas_QD_Native(CGrafPtr inGrafPtr)
	{
	ZAssertLocked(kDebug_QD, sMutex_List);
	this->Internal_Link(inGrafPtr);
	}

ZDCCanvas_QD_Native::~ZDCCanvas_QD_Native()
	{
	fGrafPtr = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD

ZMutex ZDCCanvas_QD::sMutex_List;
ZDCCanvas_QD* ZDCCanvas_QD::sCanvas_Head;

ZRef<ZDCCanvas_QD> ZDCCanvas_QD::sFindCanvasOrCreateNative(CGrafPtr inGrafPtr)
	{
	ZAssertStop(kDebug_QD, inGrafPtr);
	ZMutexLocker locker(sMutex_List);
	ZDCCanvas_QD* theCanvas = sCanvas_Head;
	while (theCanvas)
		{
		if (theCanvas->fGrafPtr == inGrafPtr)
			return theCanvas;
		theCanvas = theCanvas->fCanvas_Next;
		}
	theCanvas = new ZDCCanvas_QD_Native(inGrafPtr);
	return theCanvas;
	}

void ZDCCanvas_QD::Internal_Link(CGrafPtr inGrafPtr)
	{
	ZAssertStop(kDebug_QD, fGrafPtr == nil);
	ZMutexLocker locker(sMutex_List);

	fGrafPtr = inGrafPtr;

	fCanvas_Prev = nil;
	if (sCanvas_Head)
		sCanvas_Head->fCanvas_Prev = this;
	fCanvas_Next = sCanvas_Head;
	sCanvas_Head = this;
	}

void ZDCCanvas_QD::Internal_Unlink()
	{
	ZAssertLocked(kDebug_QD, sMutex_List);
	if (fCanvas_Prev)
		fCanvas_Prev->fCanvas_Next = fCanvas_Next;
	if (fCanvas_Next)
		fCanvas_Next->fCanvas_Prev = fCanvas_Prev;
	if (sCanvas_Head == this)
		sCanvas_Head = fCanvas_Next;
	fCanvas_Prev = nil;
	fCanvas_Next = nil;
	}

// =================================================================================================

ZDCCanvas_QD::ZDCCanvas_QD()
	{
	fCanvas_Prev = nil;
	fCanvas_Next = nil;

	fChangeCount_Origin = 1;
	fChangeCount_PatternOrigin = 1;
	fChangeCount_Ink = 1;
	fChangeCount_PenWidth = 1;
	fChangeCount_Mode = 1;
	fChangeCount_Clip = 1;
	fChangeCount_Font = 1;

	fGrafPtr = nil;

	fMutexToLock = nil;
	fMutexToCheck = nil;

	fPixPatHandle = nil;
	}

ZDCCanvas_QD::~ZDCCanvas_QD()
	{
// Subclasses must unlink themselves and detach/destroy grafports before this destructor is called.
	ZAssertStop(kDebug_QD, fCanvas_Prev == nil);
	ZAssertStop(kDebug_QD, fCanvas_Next == nil);
	ZAssertStop(kDebug_QD, fGrafPtr == nil);
	ZAssertStop(kDebug_QD, fMutexToLock == nil);
	ZAssertStop(kDebug_QD, fMutexToCheck == nil);
	if (fPixPatHandle)
		::DisposePixPat(fPixPatHandle);
	}

// =================================================================================================

void ZDCCanvas_QD::Pixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV, const ZRGBColor& inColor)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor tempColor = inColor;
	::SetCPixel(inLocationH + ioState.fOrigin.h + ioState.fPatternOrigin.h, inLocationV + ioState.fOrigin.v + ioState.fPatternOrigin.v, &tempColor);
	}

void ZDCCanvas_QD::Line(ZDCState& ioState, ZCoord inStartH, ZCoord inStartV, ZCoord inEndH, ZCoord inEndV)
	{
	if (!fGrafPtr)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
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
			this->Internal_TileRegion(ioState, theRect, thePixmapRep);
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
			this->Internal_TileRegion(ioState, theRect, thePixmapRep);
			}
		else
			{
			ZDCRgn theRgn = ZDCRgn::sLine(inStartH, inStartV, inEndH, inEndV, ioState.fPenWidth);
			this->Internal_TileRegion(ioState, theRgn, thePixmapRep);
			}
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		ZPoint offset = ioState.fOrigin + ioState.fPatternOrigin;
		::MoveTo(inStartH + offset.h, inStartV + offset.v);
		::MacLineTo(inEndH + offset.h, inEndV + offset.v);
		}
	}

// Rectangle
void ZDCCanvas_QD::FrameRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fGrafPtr)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	if (inRect.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		if (inRect.Width() <= ioState.fPenWidth * 2 || inRect.Height() <= ioState.fPenWidth * 2)
			this->Internal_TileRegion(ioState, inRect, thePixmapRep);
		else
			{
			// T
			this->Internal_TileRegion(ioState, ZRect(inRect.left, inRect.top, inRect.right, inRect.top + ioState.fPenWidth), thePixmapRep);
			// L
			this->Internal_TileRegion(ioState, ZRect(inRect.left, inRect.top + ioState.fPenWidth, inRect.left + ioState.fPenWidth, inRect.bottom - ioState.fPenWidth), thePixmapRep);
			// R
			this->Internal_TileRegion(ioState, ZRect(inRect.right - ioState.fPenWidth, inRect.top + ioState.fPenWidth, inRect.right, inRect.bottom - ioState.fPenWidth), thePixmapRep);
			// B
			this->Internal_TileRegion(ioState, ZRect(inRect.left, inRect.bottom - ioState.fPenWidth, inRect.right, inRect.bottom), thePixmapRep);
			}
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		Rect tempRect = inRect + (ioState.fOrigin + ioState.fPatternOrigin);
		::MacFrameRect(&tempRect);
		}
	}

void ZDCCanvas_QD::FillRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, inRect, thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		Rect tempRect = inRect + (ioState.fOrigin + ioState.fPatternOrigin);
		::PaintRect(&tempRect);
		}
	}

void ZDCCanvas_QD::InvertRect(ZDCState& ioState, const ZRect& inRect)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	Rect tempRect(inRect + (ioState.fOrigin + ioState.fPatternOrigin));
	::MacInvertRect(&tempRect);
	}

void ZDCCanvas_QD::HiliteRect(ZDCState& ioState, const ZRect& inRect, const ZRGBColor& inBackColor)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor oldBackColor;
	::GetBackColor(&oldBackColor);
	RGBColor tempColor(inBackColor);
	::RGBBackColor(&tempColor);
	sUseHiliteColor();
	Rect tempRect = inRect + (ioState.fOrigin + ioState.fPatternOrigin);
	::MacInvertRect(&tempRect);
	::RGBBackColor(&oldBackColor);
	}

// Region
void ZDCCanvas_QD::FrameRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fGrafPtr)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, inRgn - inRgn.Inset(ioState.fPenWidth, ioState.fPenWidth), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		ZPoint offset = ioState.fOrigin + ioState.fPatternOrigin;
		::MacFrameRgn((inRgn + offset).GetRgnHandle());
		}
	}

void ZDCCanvas_QD::FillRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fGrafPtr)
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, inRgn, thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		::MacPaintRgn((inRgn + (ioState.fOrigin + ioState.fPatternOrigin)).GetRgnHandle());
		}
	}

void ZDCCanvas_QD::InvertRegion(ZDCState& ioState, const ZDCRgn& inRgn)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);

	::MacInvertRgn((inRgn + (ioState.fOrigin + ioState.fPatternOrigin)).GetRgnHandle());
	}

void ZDCCanvas_QD::HiliteRegion(ZDCState& ioState, const ZDCRgn& inRgn, const ZRGBColor& inBackColor)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor oldBackColor;
	::GetBackColor(&oldBackColor);
	RGBColor tempColor(inBackColor);
	::RGBBackColor(&tempColor);
	sUseHiliteColor();
	::MacInvertRgn((inRgn + (ioState.fOrigin + ioState.fPatternOrigin)).GetRgnHandle());
	::RGBBackColor(&oldBackColor);
	}

// Round cornered rect
void ZDCCanvas_QD::FrameRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize)
	{
	if (!fGrafPtr)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	if (inRect.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		ZDCRgn outerRgn = ZDCRgn::sRoundRect(inRect, inCornerSize);
		ZDCRgn innerRgn = ZDCRgn::sRoundRect(inRect.left + ioState.fPenWidth, inRect.top + ioState.fPenWidth,
							inRect.right - ioState.fPenWidth, inRect.bottom - ioState.fPenWidth,
							inCornerSize.h - ioState.fPenWidth, inCornerSize.v - ioState.fPenWidth);
		this->Internal_TileRegion(ioState, (outerRgn - innerRgn), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		ZRect realBounds = inRect + (ioState.fOrigin + ioState.fPatternOrigin);
		ZDCRgn outerRgn = ZDCRgn::sRoundRect(realBounds, inCornerSize);
		ZDCRgn innerRgn = ZDCRgn::sRoundRect(realBounds.left + ioState.fPenWidth, realBounds.top + ioState.fPenWidth,
							realBounds.right - ioState.fPenWidth, realBounds.bottom - ioState.fPenWidth,
							inCornerSize.h - ioState.fPenWidth, inCornerSize.v - ioState.fPenWidth);

		::MacPaintRgn((outerRgn - innerRgn).GetRgnHandle());
		}
	}

void ZDCCanvas_QD::FillRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, ZDCRgn::sRoundRect(inRect, inCornerSize), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		::MacPaintRgn(ZDCRgn::sRoundRect(inRect + (ioState.fOrigin + ioState.fPatternOrigin), inCornerSize).GetRgnHandle());
		}
	}

void ZDCCanvas_QD::InvertRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	::MacInvertRgn(ZDCRgn::sRoundRect(inRect + (ioState.fOrigin + ioState.fPatternOrigin), inCornerSize).GetRgnHandle());
	}

void ZDCCanvas_QD::HiliteRoundRect(ZDCState& ioState, const ZRect& inRect, const ZPoint& inCornerSize, const ZRGBColor& inBackColor)
	{
	if (!fGrafPtr)
		return;

	if (inRect.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor oldBackColor;
	::GetBackColor(&oldBackColor);
	RGBColor tempColor(inBackColor);
	::RGBBackColor(&tempColor);
	sUseHiliteColor();
	::MacInvertRgn(ZDCRgn::sRoundRect(inRect + (ioState.fOrigin + ioState.fPatternOrigin), inCornerSize).GetRgnHandle());
	::RGBBackColor(&oldBackColor);
	}

// Ellipse
void ZDCCanvas_QD::FrameEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fGrafPtr)
		return;

	if (ioState.fPenWidth <= 0)
		return;

	if (inBounds.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		ZDCRgn outerRgn = ZDCRgn::sEllipse(inBounds);
		ZDCRgn innerRgn = ZDCRgn::sEllipse(inBounds.Inset(ioState.fPenWidth, ioState.fPenWidth));

		this->Internal_TileRegion(ioState, (outerRgn - innerRgn), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		Rect tempRect = inBounds + (ioState.fOrigin + ioState.fPatternOrigin);
		if (ioState.fPenWidth * 2 >= inBounds.Width() || ioState.fPenWidth * 2 >= inBounds.Height())
			::PaintOval(&tempRect);
		else
			::FrameOval(&tempRect);
		}
	}

void ZDCCanvas_QD::FillEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fGrafPtr)
		return;

	if (inBounds.IsEmpty())
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, ZDCRgn::sEllipse(inBounds), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		Rect tempRect = inBounds + (ioState.fOrigin + ioState.fPatternOrigin);
		::PaintOval(&tempRect);
		}
	}

void ZDCCanvas_QD::InvertEllipse(ZDCState& ioState, const ZRect& inBounds)
	{
	if (!fGrafPtr)
		return;

	if (inBounds.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	Rect tempRect(inBounds + (ioState.fOrigin + ioState.fPatternOrigin));
	::InvertOval(&tempRect);
	}

void ZDCCanvas_QD::HiliteEllipse(ZDCState& ioState, const ZRect& inBounds, const ZRGBColor& inBackColor)
	{
	if (!fGrafPtr)
		return;

	if (inBounds.IsEmpty())
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor oldBackColor;
	::GetBackColor(&oldBackColor);
	RGBColor tempColor(inBackColor);
	::RGBBackColor(&tempColor);
	sUseHiliteColor();
	Rect tempRect(inBounds + (ioState.fOrigin + ioState.fPatternOrigin));
	::InvertOval(&tempRect);
	::RGBBackColor(&oldBackColor);
	}

// Poly
void ZDCCanvas_QD::FillPoly(ZDCState& ioState, const ZDCPoly& inPoly)
	{
	if (!fGrafPtr)
		return;

	ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
	if (!theInkRep)
		return;

	if (ZRef<ZDCPixmapRep_QD> thePixmapRep = ::sGetPixmapRep_QDIfComplex(theInkRep))
		{
		this->Internal_TileRegion(ioState, ZDCRgn::sPoly(inPoly), thePixmapRep);
		}
	else
		{
		SetupPort theSetupPort(this, ioState);
		SetupInk theSetupInk(this, ioState);

		PolyHandle thePolyHandle = inPoly.GetPolyHandle(ioState.fOrigin + ioState.fPatternOrigin);
		::PaintPoly(thePolyHandle);
		}
	}

void ZDCCanvas_QD::InvertPoly(ZDCState& ioState, const ZDCPoly& inPoly)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);

	PolyHandle thePolyHandle = inPoly.GetPolyHandle(ioState.fOrigin + ioState.fPatternOrigin);
	::InvertPoly(thePolyHandle);
	}

void ZDCCanvas_QD::HilitePoly(ZDCState& ioState, const ZDCPoly& inPoly, const ZRGBColor& inBackColor)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);

	RGBColor oldBackColor;
	::GetBackColor(&oldBackColor);
	RGBColor tempColor(inBackColor);
	::RGBBackColor(&tempColor);
	sUseHiliteColor();
	PolyHandle thePolyHandle = inPoly.GetPolyHandle(ioState.fOrigin + ioState.fPatternOrigin);
	::InvertPoly(thePolyHandle);
	::RGBBackColor(&oldBackColor);
	}

// Pixmap
void ZDCCanvas_QD::FillPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inPixmap)
	{
	// Placeholder for code to tile the current ink using inPixmap as a mask
	ZUnimplemented();
	}

void ZDCCanvas_QD::DrawPixmap(ZDCState& ioState, const ZPoint& inLocation, const ZDCPixmap& inSourcePixmap, const ZDCPixmap* inMaskPixmap)
	{
	using namespace ZDCPixmapNS;

	if (!fGrafPtr)
		return;

	if (!inSourcePixmap)
		return;

	SetupPort theSetupPort(this, ioState);

	ZPoint realLocation = inLocation + ioState.fOrigin + ioState.fPatternOrigin;

	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// QuickDraw bottlenecks don't catch CopyMask calls, so if we're printing don't use inMaskPixmap
	if (inMaskPixmap && *inMaskPixmap && !this->IsPrinting())
		{
		// Grab the source PixMapHandle
		ZRef<ZDCPixmapRep_QD> sourceRep = ZDCPixmapRep_QD::sAsPixmapRep_QD(inSourcePixmap.GetRep());
		const PixMap& sourcePixMap = sourceRep->GetPixMap();

		ZRect localSourceBounds = inSourcePixmap.Size();

		// The Mac convention for the representation and use of masks is *wrong*. In almost every other context
		// white pixels indicate that the source should be used and black the destination. On the Mac this is reversed.
		// So we make a temporary inverted 1 bit copy of the mask pixmap and use that as the mask.
		// There is also a bug in CopyMask and CopyDeepMask -- neither of them is happy with using a > 1 bit deep
		// PixMapHandle with only black and white in the color table. We end up with the leftmost destination column
		// replicated one pixel to the right. It's not always obvious, but does mess up some images. So taking this
		// 1 bit copy removes that problem as well.
		RasterDesc maskRasterDesc;
		maskRasterDesc.fPixvalDesc.fDepth = 1;
		maskRasterDesc.fPixvalDesc.fBigEndian = true;
		maskRasterDesc.fRowBytes = ((localSourceBounds.Width() + 31) / 32) * 4;
		maskRasterDesc.fRowCount = localSourceBounds.Height();
		maskRasterDesc.fFlipped = false;
		vector<char> maskTempVector(maskRasterDesc.fRowBytes * localSourceBounds.Height());
		inMaskPixmap->CopyTo(ZPoint(0, 0),
							&maskTempVector[0],
							maskRasterDesc,
							PixelDesc(1, 0),
							localSourceBounds.Size());

		BitMap tempMaskBitmap;
		tempMaskBitmap.bounds = ZRect(localSourceBounds.Size());
		tempMaskBitmap.rowBytes = maskRasterDesc.fRowBytes;
		tempMaskBitmap.baseAddr = &maskTempVector[0];

		// Determine the bounding rectangle to use
		Rect qdSourceRect = localSourceBounds + sourceRep->GetBounds().TopLeft();
		Rect qdDestRest = localSourceBounds - localSourceBounds.TopLeft() + realLocation;

		// And do the blit
		::CopyMask((BitMap*)&sourcePixMap, &tempMaskBitmap, sGetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &tempMaskBitmap.bounds, &qdDestRest);
		}
	else
		{
		ZRef<ZDCPixmapRep_QD> sourceRep = ZDCPixmapRep_QD::sAsPixmapRep_QD(inSourcePixmap.GetRep());
		const PixMap& sourcePixMap = sourceRep->GetPixMap();

		ZRect localSourceBounds = inSourcePixmap.GetBounds();

		Rect qdSourceRect = localSourceBounds;
		Rect qdDestRect = localSourceBounds - localSourceBounds.TopLeft() + realLocation;

		::CopyBits((BitMap*)&sourcePixMap, sGetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);
		}
	}

// Flood Fill
void ZDCCanvas_QD::FloodFill(ZDCState& ioState, const ZPoint& inSeedLocation)
	{
	if (!fGrafPtr)
		return;

	ZRect theBounds = ioState.fClip.Bounds() + (ioState.fClipOrigin - ioState.fOrigin);
	if (theBounds.IsEmpty())
		return;
	ZPoint realSeedLocation = inSeedLocation + (ioState.fOrigin + ioState.fPatternOrigin);
	if (!theBounds.Contains(realSeedLocation))
		return;

	SetupPort theSetupPort(this, ioState);
	// Create a temporary bitmap into which SeedCFill will dump its results;
	BitMap tempBitmap;
	tempBitmap.bounds = theBounds;
	tempBitmap.rowBytes = (((theBounds.Width()) + 31) / 32) * 4;
	vector<char> tempMaskVector(tempBitmap.rowBytes * theBounds.Height());
	tempBitmap.baseAddr = &tempMaskVector[0];

	Rect qdRect = theBounds;
	::SeedCFill(sGetPortBitMapForCopyBits(fGrafPtr), &tempBitmap, &qdRect, &qdRect, realSeedLocation.h, realSeedLocation.v, nil, 0);
	ZDCRgn theFillRgn;
	::BitMapToRegion(theFillRgn.GetRgnHandle(), &tempBitmap);
	theFillRgn -= (ioState.fOrigin + ioState.fPatternOrigin);
	this->FillRegion(ioState, theFillRgn);
	}

// Text
void ZDCCanvas_QD::DrawText(ZDCState& ioState, const ZPoint& inLocation, const char* inText, size_t inTextLength)
	{
	if (!fGrafPtr)
		return;

	SetupPort theSetupPort(this, ioState);
	SetupText theSetupText(this, ioState);

	::MoveTo(inLocation.h + ioState.fOrigin.h + ioState.fPatternOrigin.h, inLocation.v + ioState.fOrigin.v + ioState.fPatternOrigin.v);

	RGBColor oldForeColor;
	::GetForeColor(&oldForeColor);
	RGBColor tempColor(ioState.fTextColor);
	::RGBForeColor(&tempColor);

	::MacDrawText(inText, 0, inTextLength);

	::RGBForeColor(&oldForeColor);
	}

ZCoord ZDCCanvas_QD::GetTextWidth(ZDCState& ioState, const char* inText, size_t inTextLength)
	{
	if (!fGrafPtr)
		return 0;

	ZUtil_Mac_LL::SaveRestorePort theSRP;
	SetupPort theSetupPort(this, ioState);
	SetupText theSetupText(this, ioState);

	return ::TextWidth(inText, 0, inTextLength);
	}

void ZDCCanvas_QD::GetFontInfo(ZDCState& ioState, ZCoord& outAscent, ZCoord& outDescent, ZCoord& outLeading)
	{
	if (!fGrafPtr)
		{
		outAscent = 0;
		outDescent = 0;
		outLeading = 0;
		return;
		}

	ZUtil_Mac_LL::SaveRestorePort theSRP;
	SetupPort theSetupPort(this, ioState);
	SetupText theSetupText(this, ioState);

	FontInfo theFontInfo;
	::GetFontInfo(&theFontInfo);
//	outAscent = theFontInfo.ascent - theFontInfo.descent; // <-- Just experimenting
	outAscent = theFontInfo.ascent;
	outDescent = theFontInfo.descent;
	outLeading = theFontInfo.leading;
	}

ZCoord ZDCCanvas_QD::GetLineHeight(ZDCState& ioState)
	{
	if (!fGrafPtr)
		return 0;

	ZUtil_Mac_LL::SaveRestorePort theSRP;
	SetupPort theSetupPort(this, ioState);
	SetupText theSetupText(this, ioState);

	FontInfo theFontInfo;
	::GetFontInfo(&theFontInfo);
	return theFontInfo.ascent + theFontInfo.leading;
//	return theFontInfo.ascent + theFontInfo.descent + theFontInfo.leading;
	}

void ZDCCanvas_QD::BreakLine(ZDCState& ioState, const char* inLineStart, size_t inRemainingTextLength, ZCoord inTargetWidth, size_t& outNextLineDelta)
	{
	if (!fGrafPtr)
		{
		outNextLineDelta = inRemainingTextLength;
		return;
		}

	ZAssertStop(kDebug_QD, inTargetWidth >= 0);
	if (inRemainingTextLength == 0)
		{
		outNextLineDelta = 0;
		return;
		}

	ZUtil_Mac_LL::SaveRestorePort theSRP;

	SetupPort theSetupPort(this, ioState);
	SetupText theSetupText(this, ioState);

	long lineBytes = 1;
	Fixed wrapWidth = ::Long2Fix((long)inTargetWidth);
	StyledLineBreakCode theCode = ::StyledLineBreak(const_cast<char*>(inLineStart), inRemainingTextLength, 0, inRemainingTextLength, 0, &wrapWidth, &lineBytes);
	ZAssertStop(kDebug_QD, lineBytes > 0);
	outNextLineDelta = lineBytes;

	// Try to find any '\r' characters (which don't work as CRs on Mac)
	for (size_t x = 0; x < outNextLineDelta; ++x)
		{
		char theChar = inLineStart[x];
		if (theChar == '\r' || theChar == '\n')
			{
			// Found one. Use its position as the break length.
			outNextLineDelta = x + 1;
			return;
			}
		}
	}

void ZDCCanvas_QD::InvalCache()
	{
	if (!fGrafPtr)
		return;

	ZMutexLocker locker(*fMutexToLock);

	++fChangeCount_Origin;
	++fChangeCount_PatternOrigin;
	++fChangeCount_Ink;
	++fChangeCount_PenWidth;
	++fChangeCount_Mode;
	++fChangeCount_Clip;
	++fChangeCount_Font;
	}

void ZDCCanvas_QD::Sync()
	{
	if (!fGrafPtr)
		return;

	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());
	ZMutexLocker locker(*fMutexToLock);

	#if TARGET_API_MAC_CARBON
		::QDFlushPortBuffer(fGrafPtr, nil);
	#endif
	}

void ZDCCanvas_QD::Flush()
	{
	if (!fGrafPtr)
		return;

	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());
	ZMutexLocker locker(*fMutexToLock);

	#if TARGET_API_MAC_CARBON
		::QDFlushPortBuffer(fGrafPtr, nil);
	#endif
	}

static short sGetDepth(CGrafPtr inGrafPtr)
	{
	::SetGWorld(inGrafPtr, nil);
	CGrafPtr ourGrafPtr;
	GDHandle ourGDHandle;
	::GetGWorld(&ourGrafPtr, &ourGDHandle);
	short result = 0;
	if (ourGDHandle)
		result = GETPIXMAPPIXELFORMAT(ourGDHandle[0]->gdPMap[0]);
	return result;
	}

short ZDCCanvas_QD::GetDepth()
	{
	if (!fGrafPtr)
		return 0;

	ZMutexLocker locker(*fMutexToLock);
	return ::sGetDepth(fGrafPtr);
	}

bool ZDCCanvas_QD::IsOffScreen()
	{
	return ::sIsPortOffscreen(fGrafPtr);
	}

bool ZDCCanvas_QD::IsPrinting()
	{ return false; }

ZRef<ZDCCanvas> ZDCCanvas_QD::CreateOffScreen(const ZRect& inCanvasRect)
	{
	return this->CreateOffScreen(inCanvasRect.Size(), ZDCPixmapNS::eFormatEfficient_Color_32);
	}

ZRef<ZDCCanvas> ZDCCanvas_QD::CreateOffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat)
	{
	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());
	ZMutexLocker locker(*fMutexToLock);

	return new ZDCCanvas_QD_OffScreen(inDimensions, iFormat);
	}

// =================================================================================================
// Non inlined internal stuff

ZPoint ZDCCanvas_QD::Internal_QDStart(ZDCState& ioState, bool inUsingPatterns)
	{
	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());
	ZAssertLocked(kDebug_QD, *fMutexToLock);

	// Make sure we get our location and clip setup correctly, as we're using the QD stuff
	++fChangeCount_Origin;
	++fChangeCount_Clip;
	::SetGWorld(fGrafPtr, nil);
	this->Internal_SetupPortReal(ioState, inUsingPatterns);
	this->Internal_SetupInk(ioState);
	this->Internal_SetupText(ioState);
	if (inUsingPatterns)
		return ioState.fOrigin + ioState.fPatternOrigin;
	return ZPoint::sZero;
	}

void ZDCCanvas_QD::Internal_QDEnd()
	{
	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());
	ZAssertLocked(kDebug_QD, *fMutexToLock);
	this->InvalCache();
	}

ZDCRgn ZDCCanvas_QD::Internal_GetExcludeRgn()
	{ return ZDCRgn(); }

void ZDCCanvas_QD::Internal_SetupPortReal(ZDCState& ioState, bool inUsingPatterns)
	{
	// If inUsingPatterns is true then we put the QD origin at a location governed by fPatternOrigin, so
	// that QD patterns can be made to line up correctly, or are shifted when scrolled (ie the whole
	// point of the patternOrigin concept). If inUsingPatterns is false then we put the QD origin at
	// fOrigin, so that (0,0) in the ZDC and (0,0) in QuickDraw are aligned. In this mode patterns won't
	// tile when drawn with different values of fOrigin, but it does mean that (for example) toolbox
	// controls can be always drawn at (0,0) rather than having to offset their bounds each time they're drawn.

	bool differentPatternOrigin = (ioState.fChangeCount_PatternOrigin != fChangeCount_PatternOrigin);
	bool differentOrigin = (ioState.fChangeCount_Origin != fChangeCount_Origin);
	bool differentClip = (ioState.fChangeCount_Clip != fChangeCount_Clip);

	if (differentOrigin || differentPatternOrigin)
		{
		if (inUsingPatterns)
			::SetOrigin(ioState.fPatternOrigin.h, ioState.fPatternOrigin.v);
		else
			::SetOrigin(-ioState.fOrigin.h, -ioState.fOrigin.v);
		}

	if (differentClip || differentPatternOrigin)
		{
		ZDCRgn myClip = this->Internal_CalcClipRgn(ioState);
		if (inUsingPatterns)
			::SetClip((myClip + ioState.fPatternOrigin).GetRgnHandle());
		else
			::SetClip((myClip - ioState.fOrigin).GetRgnHandle());
		}

	if (differentPatternOrigin)
		ioState.fChangeCount_PatternOrigin = ++fChangeCount_PatternOrigin;
	if (differentOrigin)
		ioState.fChangeCount_Origin = ++fChangeCount_Origin;
	if (differentClip)
		ioState.fChangeCount_Clip = ++fChangeCount_Clip;
	}

void ZDCCanvas_QD::Internal_SetupInk(ZDCState& ioState)
	{
	if (ioState.fChangeCount_Ink != fChangeCount_Ink)
		{
		ioState.fChangeCount_Ink = ++fChangeCount_Ink;

		PixPatHandle oldPixPatHandle = fPixPatHandle;
		fPixPatHandle = nil;

		ZRef<ZDCInk::Rep> theInkRep = ioState.fInk.GetRep();
		if (theInkRep->fType == ZDCInk::eTypeSolidColor)
			{
			RGBColor tempColor = theInkRep->fAsSolidColor.fColor;
			// If we're on a shallow screen QD does *not* interpret the RGB values of the colors it's asked to draw.
			// A solid black pattern is drawn solid black regardless of the RGB value assigned to foreColor. So on
			// a shallow screen we manually force a white pattern for lighter colors and a black pattern for darker ones.
			CGrafPtr ourPort;
			GDHandle ourGDevice;
			::GetGWorld(&ourPort, &ourGDevice);
			if (ourGDevice == nil || GETPIXMAPPIXELFORMAT(ourGDevice[0]->gdPMap[0]) < 4)
				{
				if (theInkRep->fAsSolidColor.fColor.Luminance() < (0xFFFFL/2))
					{
					::PenPat(reinterpret_cast<const Pattern*>(&ZDCPattern::sBlack));
					::RGBBackColor(&tempColor);
					}
				else
					{
					::PenPat(reinterpret_cast<const Pattern*>(&ZDCPattern::sWhite));
					::RGBForeColor(&tempColor);
					}
				}
			else
				{
				::PenPat(reinterpret_cast<const Pattern*>(&ZDCPattern::sForeOnly));
				::RGBForeColor(&tempColor);
				}
			// Leave the back color?
			}
		else if (theInkRep->fType == ZDCInk::eTypeTwoColor)
			{
			::PenPat(reinterpret_cast<const Pattern*>(&theInkRep->fAsTwoColor.fPattern));
			RGBColor tempColor = theInkRep->fAsTwoColor.fForeColor;
			::RGBForeColor(&tempColor);
			tempColor = theInkRep->fAsTwoColor.fBackColor;
			::RGBBackColor(&tempColor);
			}
		else if (theInkRep->fType == ZDCInk::eTypeMultiColor)
			{
			if (!ZMacOSX::sIsMacOSX())
				{
				fPixPatHandle = ZUtil_Mac_LL::sPixPatHandleFromPixmap(*theInkRep->fAsMultiColor.fPixmap);
				::PenPixPat(fPixPatHandle);
				}
			}
		else
			{
			ZDebugStopf(1, ("ZDCCanvas_QD::Internal_SetupInk, unknown ink type"));
			}

		if (oldPixPatHandle)
			::DisposePixPat(oldPixPatHandle);
		}

	if (ioState.fChangeCount_Mode != fChangeCount_Mode)
		{
		ioState.fChangeCount_Mode = ++fChangeCount_Mode;
		::PenMode(sModeLookup[ioState.fMode]);
		}

	if (ioState.fChangeCount_PenWidth != fChangeCount_PenWidth)
		{
		ioState.fChangeCount_PenWidth = ++fChangeCount_PenWidth;
		::PenSize(ioState.fPenWidth, ioState.fPenWidth);
		}
	}

void ZDCCanvas_QD::Internal_SetupText(ZDCState& ioState)
	{
	if (ioState.fChangeCount_Font == fChangeCount_Font)
		return;
	ioState.fChangeCount_Font = ++fChangeCount_Font;

	TextStyle theStyle = ioState.fFont.GetTextStyle();
	::TextFont(theStyle.tsFont);
	::TextSize(theStyle.tsSize);
	::TextFace(theStyle.tsFace);

	::TextMode(srcOr);
	}

struct TileRegion_t
	{
	CGrafPtr fGrafPtr;
	const PixMap* fPixMap;
	Rect fQDSourceRect;
	Point fRealPatternOrigin;
	Point fLocalToGlobalOffset;
	ZDCRgn fRgn;
	};

static int sDummy = 0;
static PixMap sPixMap;

static DEFINE_API(void) sTileRegion(short inDepth, short inDeviceFlags, GDHandle inTargetDevice, long inUserData)
	{
	TileRegion_t* theData = reinterpret_cast<TileRegion_t*>(inUserData);

	ZDCRgn realRgn;

	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		::GetPortVisibleRegion(theData->fGrafPtr, realRgn.GetRgnHandle());
		realRgn &= theData->fRgn;
	#else
		realRgn = theData->fRgn;
		::SectRgn(theData->fGrafPtr->visRgn, realRgn.GetRgnHandle(), realRgn.GetRgnHandle());
	#endif

	ZPoint pixmapSize(theData->fQDSourceRect.right - theData->fQDSourceRect.left, theData->fQDSourceRect.bottom - theData->fQDSourceRect.top);

	// destBounds is the rectangle enclosing the region to be tiled.
	ZRect destBounds = realRgn.Bounds();
	// drawnBounds is the same rectangle, but with its top and left coordinate
	// rounded down to a multiple of the pixmap size, taking into account the pattern origin.
	ZRect drawnBounds = destBounds;
	drawnBounds.left = drawnBounds.left - ((((drawnBounds.left + theData->fRealPatternOrigin.h) % pixmapSize.h) + pixmapSize.h) % pixmapSize.h);
	drawnBounds.top = drawnBounds.top - ((((drawnBounds.top + theData->fRealPatternOrigin.v) % pixmapSize.v) + pixmapSize.v) % pixmapSize.v);

	// sourceBitMap will hold a pointer to either the actual PixMap,
	// or to a temporary GWorld.
	const BitMap* sourceBitMap = nil;
	GWorldPtr theGWorldPtr = nil;

	if (drawnBounds.Width() > pixmapSize.h || drawnBounds.Height() > pixmapSize.v)
		{
		// We're going to be doing more than one blit.
		if (((inDeviceFlags & (1 << screenDevice))
			&& GETPIXMAPPIXELFORMAT(theData->fPixMap) != GETPIXMAPPIXELFORMAT(inTargetDevice[0]->gdPMap[0])))
			{
			// We're targetting a screen device whose pixel format differs from our source's.

			// AG 2001-12-07. This has been here for ages, but I've only just written this comment.
			// Copy the source data into a temporary GWorld that's the same depth/alignment as our
			// destination iff we're changing formats from source to dest, and if we're going to do
			// more than one blit. Note however that we only do this when our target device is a screen
			// device because NewGWorld can only create a screen-compatible GWorld, to do so for
			// arbitrary destinations would require that we extract the info ourselves.
			ZUtil_Mac_LL::SaveRestorePort theSRP;
			Rect tempRect = destBounds + theData->fLocalToGlobalOffset;
			tempRect.right = tempRect.left + pixmapSize.h;
			tempRect.bottom = tempRect.top + pixmapSize.v;

			QDErr theQDErr = ::NewGWorld(&theGWorldPtr, 0, &tempRect, nil, nil, 0);
			if (theQDErr != noErr || theGWorldPtr == nil)
				theQDErr= ::NewGWorld(&theGWorldPtr, 0, &tempRect, nil, nil, useTempMem);
			if (theQDErr == noErr && theGWorldPtr != nil)
				{
				::LockPixels(::GetGWorldPixMap(theGWorldPtr));
				SetGWorld(theGWorldPtr, nil);
				sourceBitMap = sGetPortBitMapForCopyBits(theGWorldPtr);
				tempRect.left = 0;
				tempRect.top = 0;
				tempRect.right = pixmapSize.h;
				tempRect.bottom = pixmapSize.v;
				::CopyBits((BitMap*)theData->fPixMap, sourceBitMap, &theData->fQDSourceRect, &tempRect, srcCopy, nil);
				}
			}
		}

	if (sourceBitMap == nil)
		{
		// We didn't need to set up a temporary GWorld, so the
		// source is the PixMap itself.
		sourceBitMap = (BitMap*)theData->fPixMap;
		}

	const BitMap* destBitMap = sGetPortBitMapForCopyBits(theData->fGrafPtr);

	if (drawnBounds.Width() > pixmapSize.h || drawnBounds.Height() > pixmapSize.v)
		{
		if (pixmapSize.h >= 64 || pixmapSize.v >= 64)
			++sDummy;
		}

	for (ZCoord y = drawnBounds.top; y < drawnBounds.bottom; y += pixmapSize.v)
		{
		Rect qdDestRect;
		qdDestRect.top = y;
		qdDestRect.bottom = y + pixmapSize.v;
		if (ZDCRgn currentStrip = realRgn & ZRect(drawnBounds.left, y, drawnBounds.right, y + pixmapSize.v))
			{
			// AG 1999-10-20. What's going on here? Well, when CopyBits is called it does a bunch of
			// preflighting, which is noticably slower when the clip region is non-rectangular. If
			// we're just blitting a single thing, then there's nothing to be done -- the clip region
			// is what it is and we have to live with it. When we're tiling though, we're making
			// multiple calls, and the likelihood is that at least some of the tiles could be drawn
			// without any clipping and still get the effect we want. So we do our best to help out,
			// and calculate the intersection of the required clip region with each row of tiles,
			// which for many rows will end up being rectangular. If the resultant clip is not
			// rectangular, we calculate the intersection of the clip region for each tile in the
			// row, and that's the best we can do.
			if (currentStrip.IsSimpleRect())
				{
				::SetClip(currentStrip.GetRgnHandle());
				for (ZCoord x = drawnBounds.left; x < drawnBounds.right; x += pixmapSize.h)
					{
					qdDestRect.left = x;
					qdDestRect.right = x + pixmapSize.h;
					::CopyBits(sourceBitMap, destBitMap, &theData->fQDSourceRect, &qdDestRect, srcCopy, nil);
					}
				}
			else
				{
				for (ZCoord x = drawnBounds.left; x < drawnBounds.right; x += pixmapSize.h)
					{
					qdDestRect.left = x;
					qdDestRect.right = x + pixmapSize.h;
					ZDCRgn currentRect = currentStrip & ZRect(qdDestRect);
					if (!currentRect.IsEmpty())
						{
						::SetClip(currentRect.GetRgnHandle());
						::CopyBits(sourceBitMap, destBitMap, &theData->fQDSourceRect, &qdDestRect, srcCopy, nil);
						}
					}
				}
			}
		}

	if (theGWorldPtr)
		::DisposeGWorld(theGWorldPtr);
	}

static DeviceLoopDrawingUPP sTileRegionUPP = NewDeviceLoopDrawingUPP(sTileRegion);

void ZDCCanvas_QD::Internal_TileRegion(ZDCState& ioState, const ZDCRgn& inRgn, ZRef<ZDCPixmapRep_QD> inPixmapRep)
	{
	ZMutexLocker locker(*fMutexToLock);

	// Preflight -- switch to our grafPort, set fore and back colors to black and white.

	#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
	#endif

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// Get the current QD origin
	ZPoint currentQDOrigin = sGetPortOrigin(fGrafPtr);

	// Offset inRgn to be in ZDC coords, intersect it with our clip rgn (also in ZDC coords).
	ZDCRgn realRgn = (inRgn + ioState.fOrigin) & this->Internal_CalcClipRgn(ioState);

	// And shift it so it's in the same coordinate system as whatever has been left in the port.
	realRgn += currentQDOrigin;

	TileRegion_t theData;
	theData.fGrafPtr = fGrafPtr;
	theData.fPixMap = &inPixmapRep->GetPixMap();
	theData.fQDSourceRect = inPixmapRep->GetBounds();
	theData.fRgn = realRgn;
	// Account for the origin already set in fGrafPtr
	theData.fRealPatternOrigin = ioState.fPatternOrigin - currentQDOrigin;
	theData.fLocalToGlobalOffset.h = 0;
	theData.fLocalToGlobalOffset.v = 0;
	if (1 || ::sIsPortOffscreen(fGrafPtr))
		{
		::sTileRegion(0, 0, 0, reinterpret_cast<long>(&theData));
		}
	else
		{
		::LocalToGlobal(&theData.fLocalToGlobalOffset);
		::DeviceLoop(realRgn.GetRgnHandle(), sTileRegionUPP, reinterpret_cast<long>(&theData), 0);
		}

	++fChangeCount_Clip;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_NonWindow

ZDCCanvas_QD_NonWindow::ZDCCanvas_QD_NonWindow()
	{
	fMutexToLock = &fMutex;
	}

ZDCCanvas_QD_NonWindow::~ZDCCanvas_QD_NonWindow()
	{
	ZAssertStop(kDebug_QD, fGrafPtr == nil);
	fMutexToLock = nil;
	}

void ZDCCanvas_QD_NonWindow::Finalize()
	{
	fMutexToLock->Acquire();

	ZMutexLocker lockerList(sMutex_List);
	if (this->GetRefCount() > 1)
		{
		this->FinalizationComplete();
		fMutexToLock->Release();
		}
	else
		{
		this->Internal_Unlink();
		this->FinalizationComplete();
		fMutexToLock->Release();
		delete this;
		}
	}

// Moving blocks of pixels
void ZDCCanvas_QD_NonWindow::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fGrafPtr)
		return;

	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());

	ZMutexLocker locker(*fMutexToLock);

	// Mark our clip as having been changed, because it's going to be physically changed to do the scroll
	// or we're going to call Internal_RecordInvalidation, which will also invalidate our clip.
	++fChangeCount_Clip;

	// We don't bother ensuring that our grafPort's origin is at (0,0), we just compensate for whatever
	// value it has by adding in currentQDOrigin as needed.
	ZPoint currentQDOrigin = sGetPortOrigin(fGrafPtr);
	ZPoint offset(hDelta, vDelta);

	// Work out effectiveVisRgn, the set of pixels we're able/allowed to touch. It's made
	// up of our port's real visRgn, intersected with inRect, and with our window's exclude rgn removed
	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		ZDCRgn effectiveVisRgn;
		::GetPortVisibleRegion(fGrafPtr, effectiveVisRgn.GetRgnHandle());
	#else
		ZDCRgn effectiveVisRgn(fGrafPtr->visRgn, false);
	#endif

	// As visRgn is kept in port coords we have shift it to our window coordinate system.
	effectiveVisRgn -= currentQDOrigin;
	effectiveVisRgn &= inRect + ioState.fOrigin;

	// destRgn is the pixels we want and are able to draw to.
	ZDCRgn destRgn = (ioState.fClip + ioState.fClipOrigin) & effectiveVisRgn;

	// srcRgn is the set of pixels we're want and are able to copy from.
	ZDCRgn srcRgn = (destRgn - offset) & effectiveVisRgn;

	// drawnRgn is the destination pixels that will be drawn by the CopyBits call, it's the srcRgn
	// shifted back to the destination.
	ZDCRgn drawnRgn = srcRgn + offset;

	// Now do the jiggery-pokery to actually scroll:
	#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
	#endif

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	// And set the clip (drawnRgn)
	::SetClip((drawnRgn + currentQDOrigin).GetRgnHandle());

	Rect qdSourceRect = drawnRgn.Bounds() + currentQDOrigin - offset;
	Rect qdDestRect = drawnRgn.Bounds() + currentQDOrigin;
	::CopyBits(sGetPortBitMapForCopyBits(fGrafPtr), sGetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);
	}

void ZDCCanvas_QD_NonWindow::CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
	ZRef<ZDCCanvas_QD> sourceCanvasQD = ZRefDynamicCast<ZDCCanvas_QD>(inSourceCanvas);
	ZAssertStop(kDebug_QD, sourceCanvasQD);

	CGrafPtr sourceGrafPtr = sourceCanvasQD->Internal_GetGrafPtr();
	if (fGrafPtr == nil || sourceGrafPtr == nil)
		return;

	ZAssertStop(kDebug_QD, fMutexToCheck == nil || fMutexToCheck->IsLocked());

	ZMutexLocker locker(*fMutexToLock);

	++fChangeCount_Clip;

	#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort thePCP;
	#endif

	::SetGWorld(fGrafPtr, nil);
	ZUtil_Mac_LL::SaveSetBlackWhite theSSBW;

	ZPoint currentDestQDOrigin = sGetPortOrigin(fGrafPtr);
	ZPoint currentSourceQDOrigin = sGetPortOrigin(sourceGrafPtr);

	::SetClip((ioState.fClip + (ioState.fClipOrigin + currentDestQDOrigin)).GetRgnHandle());

	Rect qdSourceRect = inSourceRect + (inSourceState.fOrigin + currentSourceQDOrigin);
	Rect qdDestRect = inSourceRect + (inDestLocation - inSourceRect.TopLeft() + ioState.fOrigin + currentDestQDOrigin);
	::CopyBits(sGetPortBitMapForCopyBits(sourceGrafPtr), sGetPortBitMapForCopyBits(fGrafPtr), &qdSourceRect, &qdDestRect, srcCopy, nil);
	}

ZDCRgn ZDCCanvas_QD_NonWindow::Internal_CalcClipRgn(const ZDCState& inState)
	{
	return inState.fClip + inState.fClipOrigin;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_OffScreen

ZDCCanvas_QD_OffScreen::ZDCCanvas_QD_OffScreen(const ZRect& inGlobalRect)
	{
	fGWorldPtr = nil;

	Rect tempRect = inGlobalRect;

	QDErr theQDErr = ::NewGWorld(&fGWorldPtr, 0, &tempRect, nil, nil, 0);
	if (theQDErr != noErr || fGWorldPtr == nil)
		theQDErr= ::NewGWorld(&fGWorldPtr, 0, &tempRect, nil, nil, useTempMem);
	if (theQDErr != noErr || fGWorldPtr == nil)
		throw bad_alloc();
	::LockPixels(::GetGWorldPixMap(fGWorldPtr));

	this->Internal_Link(fGWorldPtr);
	}

static short sFormatEfficientToDepth(ZDCPixmapNS::EFormatEfficient iFormat)
	{
	using namespace ZDCPixmapNS;
	switch (iFormat)
		{
//		case eFormatEfficient_Indexed_1: return 1;
//		case eFormatEfficient_Indexed_8: return 8;

		case eFormatEfficient_Gray_1: return 1;
		case eFormatEfficient_Gray_8: return 8;

		case eFormatEfficient_Color_16: return 16;
		case eFormatEfficient_Color_24: return 32;
		case eFormatEfficient_Color_32: return 32;
		}
	ZUnimplemented();
	return 0;
	}

ZDCCanvas_QD_OffScreen::ZDCCanvas_QD_OffScreen(ZPoint inDimensions, ZDCPixmapNS::EFormatEfficient iFormat)
	{
	ZAssertStop(kDebug_QD, inDimensions.h > 0 && inDimensions.v > 0);

	fGWorldPtr = nil;
	Rect bounds = ZRect(inDimensions);

	QDErr theQDErr;
	switch (iFormat)
		{
		case ZDCPixmapNS::eFormatEfficient_Gray_1:
			{
			theQDErr = ::NewGWorld(&fGWorldPtr, 1, &bounds, ::GetCTable(33), nil, 0);
			break;
			}
		case ZDCPixmapNS::eFormatEfficient_Gray_8:
			{
			theQDErr = ::NewGWorld(&fGWorldPtr, 8, &bounds, ::GetCTable(40), nil, 0);
			break;
			}
		case ZDCPixmapNS::eFormatEfficient_Color_16:
			{
			theQDErr = ::NewGWorld(&fGWorldPtr, 16, &bounds, nil, nil, 0);
			break;
			}
		case ZDCPixmapNS::eFormatEfficient_Color_24:
			#if !ZCONFIG(OS, MacOS7) && !ZCONFIG(OS, Carbon)
			// Mac doesn't support 24 bit, although QD does on Windows
			{
			theQDErr = ::NewGWorld(&fGWorldPtr, 24, &bounds, nil, nil, 0);
			break;
			}
			#endif // !ZCONFIG(OS, MacOS7) && !ZCONFIG(OS, Carbon)
		case ZDCPixmapNS::eFormatEfficient_Color_32:
			{
			theQDErr = ::NewGWorld(&fGWorldPtr, 32, &bounds, nil, nil, 0);
			break;
			}
		default:
			ZUnimplemented();
		}
			
	if (theQDErr != noErr || !fGWorldPtr)
		throw bad_alloc();
	::LockPixels(::GetGWorldPixMap(fGWorldPtr));

	this->Internal_Link(fGWorldPtr);
	}

ZDCCanvas_QD_OffScreen::~ZDCCanvas_QD_OffScreen()
	{
	if (fGWorldPtr)
		{
		// AG 98-12-14. Don't leave our grafPtr set as the current port. Major crash in QD
		// will occasionally result (makes sense, and it seems odd this doesn't happen more often)
		CGrafPtr currentPort;
		GDHandle currentDevice;
		::GetGWorld(&currentPort, &currentDevice);
		if (fGWorldPtr == currentPort)
			ZUtil_Mac_LL::sSetWindowManagerPort();
		::DisposeGWorld(fGWorldPtr);
		fGWorldPtr = nil;
		}
	fGrafPtr = nil;
	}

ZRGBColor ZDCCanvas_QD_OffScreen::GetPixel(ZDCState& ioState, ZCoord inLocationH, ZCoord inLocationV)
	{
	SetupPort theSetupPort(this, ioState);

	inLocationH += ioState.fOrigin.h + ioState.fPatternOrigin.h;
	inLocationV += ioState.fOrigin.v + ioState.fPatternOrigin.v;

	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		Rect tempRect;
		::GetPortBounds(fGWorldPtr, &tempRect);
		if (inLocationH < tempRect.left || inLocationH >= tempRect.right ||
			inLocationV < tempRect.top || inLocationV >= tempRect.bottom)
			return ZRGBColor::sBlack;
	#else
		if (inLocationH < fGWorldPtr->portRect.left || inLocationH >= fGWorldPtr->portRect.right ||
			inLocationV < fGWorldPtr->portRect.top || inLocationV >= fGWorldPtr->portRect.bottom)
			return ZRGBColor::sBlack;
	#endif

	RGBColor theColor;
	::GetCPixel(inLocationH, inLocationV, &theColor);
	return theColor;
	}

ZDCPixmap ZDCCanvas_QD_OffScreen::GetPixmap(ZDCState& ioState, const ZRect& inBounds)
	{
	if (fGrafPtr == nil || inBounds.IsEmpty())
		return ZDCPixmap();

	PixMapHandle thePixMapHandle = ::GetGWorldPixMap(fGWorldPtr);
	ZUtil_Mac_LL::HandleLocker locker1((Handle)thePixMapHandle);

	ZRef<ZDCPixmapRep> theRep_QD = new ZDCPixmapRep_QD(thePixMapHandle[0],
					::GetPixBaseAddr(thePixMapHandle),
					inBounds + (ioState.fOrigin));
	return ZDCPixmap(theRep_QD);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_Print

ZDCCanvas_QD_Print::ZDCCanvas_QD_Print(GrafPtr inGrafPtr)
	{
	this->Internal_Link((CGrafPtr)inGrafPtr);
	}

ZDCCanvas_QD_Print::~ZDCCanvas_QD_Print()
	{
	fGrafPtr = nil;
	}

bool ZDCCanvas_QD_Print::IsPrinting()
	{ return true; }

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_QD_PICT

class ZDCCanvas_QD_PICT : public ZDCCanvas_QD_NonWindow
	{
public:
	ZDCCanvas_QD_PICT(const ZRect& inBounds, const ZStreamW& inStream);
	virtual ~ZDCCanvas_QD_PICT();

protected:
	void PutPicProc(const void* dataPtr, short byteCount);
	static DEFINE_API(void) sPutPicProc(const void* dataPtr, short byteCount);
	static QDPutPicUPP sPutPicProcUPP;

	const ZStreamW& fStream;
	bool fStreamOkay;

	GWorldPtr fGWorldPtr;
	CQDProcs* fCQDProcs_Old;
	CQDProcs fCQDProcs;

	PicHandle fPicHandle;

	ZDCCanvas_QD_PICT* fPrev;
	ZDCCanvas_QD_PICT* fNext;
	static ZDCCanvas_QD_PICT* sHead;
	};

ZDCCanvas_QD_PICT::ZDCCanvas_QD_PICT(const ZRect& inBounds, const ZStreamW& inStream)
:	fStream(inStream)
	{
	fCQDProcs_Old = nil;
	fPicHandle = nil;
	fGWorldPtr = nil;
	fStreamOkay = true;

	// Write out the picture header
	try
		{
		fStream.WriteInt16(0); // picSize -- never used for PICT version 2
		fStream.WriteInt16(inBounds.top); // picFrame.top
		fStream.WriteInt16(inBounds.left); // picFrame.left
		fStream.WriteInt16(inBounds.bottom); // picFrame.bottom
		fStream.WriteInt16(inBounds.right); // picFrame.right
		}
	catch (...)
		{
		}

	#if ZCONFIG(API_Thread, Mac)
	ZUtil_Mac_LL::PreserveCurrentPort fPCP;
	#endif

	// Set up the dummy GWorld
	Rect bounds = { 0, 0, 4, 4 };
	QDErr theQDErr= ::NewGWorld(&fGWorldPtr, 32, &bounds, nil, nil, 0);
	if (theQDErr != noErr || fGWorldPtr == nil)
		theQDErr= ::NewGWorld(&fGWorldPtr, 32, &bounds, nil, nil, useTempMem);
	if (theQDErr != noErr || fGWorldPtr == nil)
		throw bad_alloc();

	::LockPixels(::GetGWorldPixMap(fGWorldPtr));

	this->Internal_Link(fGWorldPtr);

	// Put ourselves on the list of PICT canvases
	sMutex_List.Acquire();
	fPrev = nil;
	fNext = sHead;
	if (sHead)
		sHead->fPrev = this;
	sHead = this;
	sMutex_List.Release();

	// Attach our pic save proc
	::SetGWorld(fGWorldPtr, nil);
	::SetStdCProcs(&fCQDProcs);
	fCQDProcs.putPicProc = sPutPicProcUPP;

	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		fCQDProcs_Old = ::GetPortGrafProcs(fGWorldPtr);
		::SetPortGrafProcs(fGWorldPtr, &fCQDProcs);
	#else
		fCQDProcs_Old = fGWorldPtr->grafProcs;
		fGWorldPtr->grafProcs = &fCQDProcs;
	#endif

	// Do the open picture thing
	OpenCPicParams theOCPParams;
	theOCPParams.srcRect = inBounds;
	theOCPParams.hRes = 0x00480000;
	theOCPParams.vRes = 0x00480000;
	theOCPParams.version = -2;
	theOCPParams.reserved1 = 0;
	theOCPParams.reserved2 = 0;
	fPicHandle = ::OpenCPicture(&theOCPParams);

	// And make sure we start with a sensible clip (ie not wide-open)
	::ClipRect(&theOCPParams.srcRect);
	}

ZDCCanvas_QD_PICT::~ZDCCanvas_QD_PICT()
	{
	if (fGWorldPtr)
		{
		::SetGWorld(fGWorldPtr, nil);
		::ClosePicture();

		#if ACCESSOR_CALLS_ARE_FUNCTIONS
			::SetPortGrafProcs(fGWorldPtr, fCQDProcs_Old);
		#else
			fGWorldPtr->grafProcs = fCQDProcs_Old;
		#endif

		ZUtil_Mac_LL::sSetWindowManagerPort();

		sMutex_List.Acquire();
		if (fPrev)
			fPrev->fNext = fNext;
		if (fNext)
			fNext->fPrev = fPrev;
		if (sHead == this)
			sHead = fNext;
		sMutex_List.Release();

		::DisposeGWorld(fGWorldPtr);
		fGWorldPtr = nil;

		::DisposeHandle((Handle)fPicHandle);
		}
	fGrafPtr = nil;
	}

void ZDCCanvas_QD_PICT::PutPicProc(const void* dataPtr, short byteCount)
	{
	try
		{
		if (fStreamOkay)
			fStream.Write(dataPtr, byteCount);
		// QD relies on the value in picSize to determine whether it is
		// on a word boundary or not, and hence whether it needs to pad. So we have
		// to keep this value up to date. Although it wraps at 32767 the low bit
		// will have the correct value at all times. Note that this method gets
		// called during the OpenCPicture call, before we've been told what fPicHandle is,
		// hence its use has to be wrapped in a conditional.
		if (fPicHandle)
			fPicHandle[0]->picSize += byteCount;
		}
	catch (...)
		{
		#if ZCONFIG(OS, Carbon)
			::SetQDError(cNoMemErr);
		#elif ZCONFIG(OS, MacOS7)
			*((QDErr*)(0x0D6E)) = cNoMemErr;
		#endif
		fStreamOkay = false;
		}
	}

DEFINE_API(void) ZDCCanvas_QD_PICT::sPutPicProc(const void* dataPtr, short byteCount)
	{
	CGrafPtr currentPort;
	GDHandle currentGDHandle;
	::GetGWorld(&currentPort, &currentGDHandle);
	sMutex_List.Acquire();
	ZDCCanvas_QD_PICT* current = sHead;
	while (current && current->fGWorldPtr != currentPort)
		current = current->fNext;
	sMutex_List.Release();
	ZAssertStop(0, current != nil);
	current->PutPicProc(dataPtr, byteCount);
	}

QDPutPicUPP ZDCCanvas_QD_PICT::sPutPicProcUPP = NewQDPutPicUPP(sPutPicProc);
ZDCCanvas_QD_PICT* ZDCCanvas_QD_PICT::sHead = nil;

// =================================================================================================
#pragma mark -
#pragma mark * ZDC_PICT

ZDC_PICT::ZDC_PICT(const ZRect& inBounds, const ZStreamW& inStream)
:	ZDC(new ZDCCanvas_QD_PICT(inBounds, inStream), ZPoint::sZero)
	{
	fState.fClip = inBounds;
	}

ZDC_PICT::~ZDC_PICT()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZDC_NativeQD

ZDC_NativeQD::ZDC_NativeQD(CGrafPtr inGrafPtr)
	{
	::GetGWorld(&fSavedGrafPtr, &fSavedGDHandle);
	if (inGrafPtr == nil)
		fGrafPtr = fSavedGrafPtr;
	else
		fGrafPtr = inGrafPtr;

	fCanvas = ZDCCanvas_QD::sFindCanvasOrCreateNative(fGrafPtr);

	::SetGWorld(fGrafPtr, nil);
	::GetClip(fOldClip.GetRgnHandle());

	Rect portBounds;

	#if ACCESSOR_CALLS_ARE_FUNCTIONS
		::GetPortBounds(fGrafPtr, &portBounds);
		fOldTextMode = ::GetPortTextMode(fGrafPtr);
		fOldTextSize = ::GetPortTextSize(fGrafPtr);
		fOldTextFont = ::GetPortTextFont(fGrafPtr);
		fOldTextFace = ::GetPortTextFace(fGrafPtr);
	#else
		portBounds = fGrafPtr->portRect;
		fOldTextMode = fGrafPtr->txMode;
		fOldTextSize = fGrafPtr->txSize;
		fOldTextFont = fGrafPtr->txFont;
		fOldTextFace = fGrafPtr->txFace;
	#endif

	fOldOrigin.h = portBounds.left;
	fOldOrigin.v = portBounds.top;

	::GetForeColor(&fOldForeColor);
	::GetBackColor(&fOldBackColor);
	::GetPenState(&fOldState);

	this->SetFont(ZDCFont(fOldTextFont, fOldTextFace, fOldTextSize));

	// Now stuff our ZDC settings with the origin and clipping pulled from the grafPtr
	this->SetOrigin(-fOldOrigin);

	// Check for a wide open clip -- it'll screw us up anytime we switch origins around.
	ZRect clipBounds = fOldClip.Bounds();
	if (clipBounds.left == -32767 && clipBounds.top == -32767
		&& clipBounds.right == 32767 && clipBounds.bottom == 32767)
		{
		this->SetClip(ZRect(portBounds));
		}
	else
		{
		this->SetClip(fOldClip);
		}
	}

ZDC_NativeQD::~ZDC_NativeQD()
	{
	// Ensure that the canvas's settings are invalidated
	fCanvas->InvalCache();

	// Copy back the state we saved in our constructor
	::SetGWorld(fGrafPtr, nil);
	::RGBForeColor(&fOldForeColor);
	::RGBBackColor(&fOldBackColor);
	::SetPenState(&fOldState);
	::TextMode(fOldTextMode);
	::TextSize(fOldTextSize);
	::TextFont(fOldTextFont);
	::TextFace(fOldTextFace);

	::SetOrigin(fOldOrigin.h, fOldOrigin.v);
	::SetClip(fOldClip.GetRgnHandle());

	// And restore which port was active at the time.
	::SetGWorld(fSavedGrafPtr, fSavedGDHandle);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDC::SetupForQD

ZDCSetupForQD::ZDCSetupForQD(const ZDC& inDC, bool inUsingPatterns)
:	fCanvas(ZRefDynamicCast<ZDCCanvas_QD>(inDC.GetCanvas()))
	{
	ZAssertStop(kDebug_QD, fCanvas->fMutexToCheck == nil || fCanvas->fMutexToCheck->IsLocked());
	ZMutexLocker locker(*fCanvas->fMutexToLock);

	fOffset = fCanvas->Internal_QDStart(inDC.GetState(), inUsingPatterns);
	}

ZDCSetupForQD::~ZDCSetupForQD()
	{
	ZAssertStop(kDebug_QD, fCanvas->fMutexToCheck == nil || fCanvas->fMutexToCheck->IsLocked());
	ZMutexLocker locker(*fCanvas->fMutexToLock);

	fCanvas->Internal_QDEnd();
	}

ZPoint ZDCSetupForQD::GetOffset()
	{ return fOffset; }

GrafPtr ZDCSetupForQD::GetGrafPtr()
	{ return (GrafPtr)fCanvas->fGrafPtr; }

CGrafPtr ZDCSetupForQD::GetCGrafPtr()
	{ return fCanvas->fGrafPtr; }

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapRep_QD

static bool sCheckDesc(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZRect& inBounds,
							const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	using namespace ZDCPixmapNS;

	if (inRasterDesc.fFlipped || ((inRasterDesc.fRowBytes % 4) != 0))
		return false;

	bool isOkay = false;

	if (PixelDescRep_Color* thePixelDescRep_Color = ZRefDynamicCast<PixelDescRep_Color>(inPixelDesc.GetRep()))
		{
		uint32 maskRed, maskGreen, maskBlue, maskAlpha;
		thePixelDescRep_Color->GetMasks(maskRed, maskGreen, maskBlue, maskAlpha);
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 16:
				{
				if (inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0x7C00 && maskGreen == 0x03E0 && maskBlue == 0x001F)
					{
					isOkay = true;
					}
				break;
				}
			#if !ZCONFIG(OS, MacOS7) && !ZCONFIG(OS, Carbon) && !ZCONFIG(OS, MacOSX)
			case 24:
				{
				if ((inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0xFF0000 && maskGreen == 0x00FF00 && maskBlue == 0x0000FF)
					|| (inRasterDesc.fPixvalDesc.fBigEndian == false && maskRed == 0x0000FF && maskGreen == 0x00FF00 && maskBlue == 0xFF0000))
					{
					isOkay = true;
					}
				break;
				}
			#endif
			case 32:
				{
				if ((inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0x00FF0000 && maskGreen == 0x0000FF00 && maskBlue == 0x000000FF)
					|| (inRasterDesc.fPixvalDesc.fBigEndian == false && maskRed == 0x0000FF00 && maskGreen == 0x00FF0000 && maskBlue == 0xFF000000))
					{
					isOkay = true;
					}
				break;
				}
			}
		}
	else if (PixelDescRep_Gray* thePixelDescRep_Gray = ZRefDynamicCast<PixelDescRep_Gray>(inPixelDesc.GetRep()))
		{
		uint32 maskGray, maskAlpha;
		thePixelDescRep_Gray->GetMasks(maskGray, maskAlpha);
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				isOkay = (maskGray == ((1 << inRasterDesc.fPixvalDesc.fDepth) - 1));
				}
			}
		}
	else if (PixelDescRep_Indexed* thePixelDescRep_Indexed = ZRefDynamicCast<PixelDescRep_Indexed>(inPixelDesc.GetRep()))
		{
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				isOkay = true;
				}
			}
		}

	return isOkay;
	}

static ZRef<ZDCPixmapRep_QD> sCreateRepForDesc(ZRef<ZDCPixmapRaster> inRaster, const ZRect& inBounds,
												const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	if (::sCheckDesc(inRaster->GetRasterDesc(), inBounds, inPixelDesc))
		return new ZDCPixmapRep_QD(inRaster, inBounds, inPixelDesc);

	return ZRef<ZDCPixmapRep_QD>();
	}

static bool sSetupPixMap(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZDCPixmapNS::PixelDesc& inPixelDesc, PixMap& outPixMap)
	{
	using namespace ZDCPixmapNS;

	if (inRasterDesc.fFlipped || ((inRasterDesc.fRowBytes % 4) != 0))
		return false;

	ZRef<PixelDescRep> thePixelDescRep = inPixelDesc.GetRep();
	if (PixelDescRep_Color* thePixelDescRep_Color = ZRefDynamicCast<PixelDescRep_Color>(thePixelDescRep))
		{
		uint32 maskRed, maskGreen, maskBlue, maskAlpha;
		thePixelDescRep_Color->GetMasks(maskRed, maskGreen, maskBlue, maskAlpha);
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 16:
				{
				if (inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0x7C00 && maskGreen == 0x03E0 && maskBlue == 0x001F)
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 16, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			case 24:
				{
				if ((inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0xFF0000 && maskGreen == 0x00FF00 && maskBlue == 0x0000FF)
					|| (inRasterDesc.fPixvalDesc.fBigEndian == false && maskRed == 0x0000FF && maskGreen == 0x00FF00 && maskBlue == 0xFF0000))
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 24, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			case 32:
				{
				if ((inRasterDesc.fPixvalDesc.fBigEndian == true && maskRed == 0x00FF0000 && maskGreen == 0x0000FF00 && maskBlue == 0x000000FF)
					|| (inRasterDesc.fPixvalDesc.fBigEndian == false && maskRed == 0x0000FF00 && maskGreen == 0x00FF0000 && maskBlue == 0xFF000000))
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 32, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			}
		}
	else if (PixelDescRep_Gray* thePixelDescRep_Gray = ZRefDynamicCast<PixelDescRep_Gray>(thePixelDescRep))
		{
		uint32 maskGray, maskAlpha;
		thePixelDescRep_Gray->GetMasks(maskGray, maskAlpha);
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				if (maskGray == ((1 << inRasterDesc.fPixvalDesc.fDepth) - 1))
					{
					ZUtil_Mac_LL::sSetupPixMapGray(inRasterDesc.fRowBytes, inRasterDesc.fPixvalDesc.fDepth, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				}
			}
		}
	else if (PixelDescRep_Indexed* thePixelDescRep_Indexed = ZRefDynamicCast<PixelDescRep_Indexed>(thePixelDescRep))
		{
		switch (inRasterDesc.fPixvalDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				ZUtil_Mac_LL::sSetupPixMapIndexed(inRasterDesc.fRowBytes, inRasterDesc.fPixvalDesc.fDepth, inRasterDesc.fRowCount, outPixMap);
				return true;
				}
			}
		}

	return false;
	}

ZDCPixmapRep_QD::ZDCPixmapRep_QD(ZRef<ZDCPixmapRaster> inRaster,
								const ZRect& inBounds,
								const ZDCPixmapNS::PixelDesc& inPixelDesc)
:	ZDCPixmapRep(inRaster, inBounds, inPixelDesc)
	{
	bool result = ::sSetupPixMap(fRaster->GetRasterDesc(), fPixelDesc, fPixMap);
	ZAssertStop(kDebug_QD, result);
	fPixMap.baseAddr = reinterpret_cast<Ptr>(fRaster->GetBaseAddress());
	fChangeCount = fPixelDesc.GetChangeCount() - 1;
	}

ZDCPixmapRep_QD::ZDCPixmapRep_QD(const PixMap* inSourcePixMap, const void* inSourceData, const ZRect& inSourceBounds)
	{
	using namespace ZDCPixmapNS;

	fBounds = inSourceBounds.Size();

	short theDepth = GETPIXMAPPIXELFORMAT(inSourcePixMap);

	switch (theDepth)
		{
		case 1:
		case 2:
		case 4:
		case 8:
			{
			if (theDepth + 32 == inSourcePixMap->pmTable[0]->ctSeed)
				{
				// It's a standard gray scale ctable, so we can use a grayscale PixelDesc
				fPixelDesc = PixelDesc((1 << theDepth) - 1, 0);
				}
			else
				{
				fPixelDesc = ZUtil_Mac_LL::sCTabHandleToPixelDesc(inSourcePixMap->pmTable);
				}
			break;
			}
		case 16:
			{
			fPixelDesc = PixelDesc(0x7C00, 0x03E0, 0x001F, 0x0000);
			break;
			}
		case 24:
			{
			fPixelDesc = PixelDesc(0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
			break;
			}
		default:
		case 32:
			{
			fPixelDesc = PixelDesc(0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000);
			break;
			}
		}

	RasterDesc sourceRasterDesc(PixvalDesc(theDepth, true), inSourcePixMap->rowBytes & 0x3FFF, inSourcePixMap->bounds.bottom, false);

	fRaster = new ZDCPixmapRaster_Simple(RasterDesc(PixvalDesc(theDepth, true), ((inSourceBounds.Width() * theDepth + 31) / 32) * 4, inSourceBounds.Height(), false));

	this->CopyFrom(ZPoint(0, 0),
					inSourceData ? inSourceData : inSourcePixMap->baseAddr,
					sourceRasterDesc,
					fPixelDesc,
					inSourceBounds);

	bool result = ::sSetupPixMap(fRaster->GetRasterDesc(), fPixelDesc, fPixMap);
	ZAssertStop(kDebug_QD, result);
	fPixMap.baseAddr = reinterpret_cast<Ptr>(fRaster->GetBaseAddress());
	}

ZDCPixmapRep_QD::ZDCPixmapRep_QD(const BitMap* inSourceBitMap, const void* inSourceData, const ZRect& inSourceBounds, bool inInvert)
	{
	ZUnimplemented();
	}

ZDCPixmapRep_QD::~ZDCPixmapRep_QD()
	{
	if (fPixMap.pmTable)
		::DisposeHandle(reinterpret_cast<Handle>(fPixMap.pmTable));
	}

const PixMap& ZDCPixmapRep_QD::GetPixMap()
	{
	using namespace ZDCPixmapNS;

	if (fChangeCount != fPixelDesc.GetChangeCount())
		{
		if (PixelDescRep_Indexed* thePixelDesc = ZRefDynamicCast<PixelDescRep_Indexed>(fPixelDesc.GetRep()))
			{
			const ZRGBColorPOD* theColors;
			size_t theSize;
			thePixelDesc->GetColors(theColors, theSize);
			for (size_t x = 0; x < theSize; ++x)
				{
				fPixMap.pmTable[0]->ctTable[x].rgb.red = theColors[x].red;
				fPixMap.pmTable[0]->ctTable[x].rgb.green = theColors[x].green;
				fPixMap.pmTable[0]->ctTable[x].rgb.blue = theColors[x].blue;
				}
			}
		fChangeCount = fPixelDesc.GetChangeCount();
		}
	return fPixMap;
	}

ZRef<ZDCPixmapRep_QD> ZDCPixmapRep_QD::sAsPixmapRep_QD(ZRef<ZDCPixmapRep> inRep)
	{
	using namespace ZDCPixmapNS;

	// See if it's a QD-compatible rep already.
	ZRef<ZDCPixmapRep_QD> theRep_QD = ZRefDynamicCast<ZDCPixmapRep_QD>(inRep);
	if (!theRep_QD)
		{
		ZRef<ZDCPixmapRaster> theRaster = inRep->GetRaster();
		ZRect theBounds = inRep->GetBounds();
		PixelDesc thePixelDesc = inRep->GetPixelDesc();
		theRep_QD = ::sCreateRepForDesc(theRaster, theBounds, thePixelDesc);
		if (!theRep_QD)
			{
			EFormatStandard fallbackFormat = eFormatStandard_xRGB_32;
			if (ZRefDynamicCast<PixelDescRep_Gray>(thePixelDesc.GetRep()))
				fallbackFormat = eFormatStandard_Gray_8;

			RasterDesc newRasterDesc(theBounds.Size(), fallbackFormat);
			PixelDesc newPixelDesc(fallbackFormat);
			theRep_QD = new ZDCPixmapRep_QD(new ZDCPixmapRaster_Simple(newRasterDesc), theBounds.Size(), newPixelDesc);

			theRep_QD->CopyFrom(ZPoint(0, 0), inRep, theBounds);
			}
		}
	return theRep_QD;
	}

// =================================================================================================
#pragma mark -
#pragma mark * Rep conversion/coercion utilities

static ZRef<ZDCPixmapRep_QD> sGetPixmapRep_QDIfComplex(const ZRef<ZDCInk::Rep>& inInkRep)
	{
	ZRef<ZDCPixmapRep_QD> thePixmapRep_QD;
	if (inInkRep->fType == ZDCInk::eTypeMultiColor)
		{
		ZRef<ZDCPixmapRep> thePixmapRep = inInkRep->fAsMultiColor.fPixmap->GetRep();
		ZPoint size = thePixmapRep->GetSize();

		// Our PixMapHandle is considered 'complex' if its width or height are not a power of two <= 64
		bool isComplex = false;

		if (ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX))
			{
			// We force the pixmap to be considered complex so as not to get into difficulties
			// with MacOS X's problems with the manually created PixPatHandles we'll use if
			// we were to say that the pixmap is not complex. The bug needs diagnosing, but
			// this workaround is okay so far.
			if (ZMacOSX::sIsMacOSX())
				isComplex = true;
			}

		switch (size.h)
			{
			case 1: case 2: case 4: case 8: case 16: case 32: case 64:
				break;
			default:
				isComplex = true;
			}

		switch (size.v)
			{
			case 1: case 2: case 4: case 8: case 16: case 32: case 64:
				break;
			default:
				isComplex = true;
			}

		if (isComplex)
			thePixmapRep_QD = ZDCPixmapRep_QD::sAsPixmapRep_QD(thePixmapRep);
		}

	return thePixmapRep_QD;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCPixmapFactory_QD

class ZDCPixmapFactory_QD : public ZDCPixmapFactory
	{
public:
	virtual ZRef<ZDCPixmapRep> CreateRep(const ZRef<ZDCPixmapRaster>& inRaster, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc);
	virtual ZRef<ZDCPixmapRep> CreateRep(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc);
	virtual ZDCPixmapNS::EFormatStandard MapEfficientToStandard(ZDCPixmapNS::EFormatEfficient inFormat);
	};

static ZDCPixmapFactory_QD sZDCPixmapFactory_QD;

ZRef<ZDCPixmapRep> ZDCPixmapFactory_QD::CreateRep(const ZRef<ZDCPixmapRaster>& inRaster, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	return ::sCreateRepForDesc(inRaster, inBounds, inPixelDesc);
	}

ZRef<ZDCPixmapRep> ZDCPixmapFactory_QD::CreateRep(const ZDCPixmapNS::RasterDesc& inRasterDesc, const ZRect& inBounds, const ZDCPixmapNS::PixelDesc& inPixelDesc)
	{
	if (::sCheckDesc(inRasterDesc, inBounds, inPixelDesc))
		return new ZDCPixmapRep_QD(new ZDCPixmapRaster_Simple(inRasterDesc), inBounds, inPixelDesc);
	return ZRef<ZDCPixmapRep>();
	}

ZDCPixmapNS::EFormatStandard ZDCPixmapFactory_QD::MapEfficientToStandard(ZDCPixmapNS::EFormatEfficient inFormat)
	{
	using namespace ZDCPixmapNS;
	EFormatStandard standardFormat = eFormatStandard_Invalid;
	switch (inFormat)
		{
//		case eFormatEfficient_Indexed_1: standardFormat = eFormatStandard_Indexed_1; break;
//		case eFormatEfficient_Indexed_8: standardFormat = eFormatStandard_Indexed_8; break;

		case eFormatEfficient_Gray_1: standardFormat = eFormatStandard_Gray_1; break;
		case eFormatEfficient_Gray_8: standardFormat = eFormatStandard_Gray_8; break;

		case eFormatEfficient_Color_16: standardFormat = eFormatStandard_xRGB_16_BE; break;
		case eFormatEfficient_Color_24:
		case eFormatEfficient_Color_32: standardFormat = eFormatStandard_xRGB_32; break;
		}

	return standardFormat;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvasFactory_QD

class ZDCCanvasFactory_QD : public ZDCCanvasFactory
	{
public:
	virtual ZRef<ZDCCanvas> CreateCanvas(ZPoint iSize, bool iBigEndian, const ZDCPixmapNS::PixelDesc& iPixelDesc);
	};

static ZDCCanvasFactory_QD sZDCCanvasFactory_QD;

ZRef<ZDCCanvas> ZDCCanvasFactory_QD::CreateCanvas(ZPoint iSize, bool iBigEndian, const ZDCPixmapNS::PixelDesc& iPixelDesc)
	{
#if 0
	ZRef<ZDCPixmapNS::PixelDescRep> thePixelDescRep = inPixelDesc.GetRep();
	if (ZDCPixmapNS::PixelDescRep_Color* thePixelDescRep_Color = dynamic_cast<ZDCPixmapNS::PixelDescRep_Color*>(thePixelDescRep.GetObject()))
		{
		uint32 maskRed, maskGreen, maskBlue, maskAlpha;
		thePixelDescRep_Color->GetMasks(maskRed, maskGreen, maskBlue, maskAlpha);
		switch (inRasterDesc.fDepth)
			{
			case 16:
				{
				if (iBigEndian == true && maskRed == 0x7C00 && maskGreen == 0x03E0 && maskBlue == 0x001F)
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 16, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			case 24:
				{
				if ((iBigEndian == true && maskRed == 0xFF0000 && maskGreen == 0x00FF00 && maskBlue == 0x0000FF)
					|| (iBigEndian == false && maskRed == 0x0000FF && maskGreen == 0x00FF00 && maskBlue == 0xFF0000))
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 24, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			case 32:
				{
				if ((iBigEndian == true && maskRed == 0x00FF0000 && maskGreen == 0x0000FF00 && maskBlue == 0x000000FF)
					|| (iBigEndian == false && maskRed == 0x0000FF00 && maskGreen == 0x00FF0000 && maskBlue == 0xFF000000))
					{
					ZUtil_Mac_LL::sSetupPixMapColor(inRasterDesc.fRowBytes, 32, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				break;
				}
			}
		}
	else if (ZDCPixmapNS::PixelDescRep_Gray* thePixelDescRep_Gray = dynamic_cast<ZDCPixmapNS::PixelDescRep_Gray*>(thePixelDescRep.GetObject()))
		{
		uint32 maskGray, maskAlpha;
		thePixelDescRep_Gray->GetMasks(maskGray, maskAlpha);
		switch (inRasterDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				if (maskGray == ((1 << inRasterDesc.fDepth) - 1))
					{
					ZUtil_Mac_LL::sSetupPixMapGray(inRasterDesc.fRowBytes, inRasterDesc.fDepth, inRasterDesc.fRowCount, outPixMap);
					return true;
					}
				}
			}
		}
	else if (ZDCPixmapNS::PixelDescRep_Indexed* thePixelDescRep_Indexed = dynamic_cast<ZDCPixmapNS::PixelDescRep_Indexed*>(thePixelDescRep.GetObject()))
		{
		switch (inRasterDesc.fDepth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				{
				ZUtil_Mac_LL::sSetupPixMapIndexed(inRasterDesc.fRowBytes, inRasterDesc.fDepth, inRasterDesc.fRowCount, outPixMap);
				return true;
				}
			}
		}
#endif
	return ZRef<ZDCCanvas>();
	}

#endif // ZCONFIG(API_Graphics, QD)
