static const char rcsid[] = "@(#) $Id: ZUIInk.cpp,v 1.4 2006/07/15 20:54:22 goingware Exp $";

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

#include "ZUIInk.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <LowMem.h> // For LMGetHiliteRGB
#endif

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/LowMem.h>
#endif

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk

ZDCInk ZUIInk::GetInk()
	{
	ZUnimplemented();
	return ZDCInk();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Fixed

ZUIInk_Fixed::ZUIInk_Fixed(const ZDCInk& theInk)
:	fInk(theInk)
	{}

ZDCInk ZUIInk_Fixed::GetInk()
	{ return fInk; }

void ZUIInk_Fixed::SetInk(const ZDCInk& theInk)
	{
	fInk = theInk;
	this->Changed();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Indirect

ZUIInk_Indirect::ZUIInk_Indirect(ZRef<ZUIInk> realInk)
:	fRealInk(realInk)
	{
	}

ZUIInk_Indirect::~ZUIInk_Indirect()
	{}

ZDCInk ZUIInk_Indirect::GetInk()
	{
	if (ZRef<ZUIInk> theRealInk = fRealInk)
		return theRealInk->GetInk();
	return ZUIInk::GetInk();
	}

void ZUIInk_Indirect::SetRealInk(ZRef<ZUIInk> realInk)
	{
	if (fRealInk == realInk)
		return;
	fRealInk = realInk;
	this->Changed();
	}

ZRef<ZUIInk> ZUIInk_Indirect::GetRealInk()
	{ return fRealInk; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Win32System

#if ZCONFIG(OS, Win32)

ZUIInk_Win32System::ZUIInk_Win32System(int systemColorIndex)
:	fSystemColorIndex(systemColorIndex)
	{}

ZDCInk ZUIInk_Win32System::GetInk()
	{
//	COLORREF theSysColor = ::GetSysColor(fSystemColorIndex);
//	COLORREF theNearestColor = ::GetNearestColor(nil, theSysColor);
//	return ZDCInk(ZRGBColor::sFromCOLORREF(theNearestColor));
	return ZDCInk(ZRGBColor::sFromCOLORREF(::GetSysColor(fSystemColorIndex)));
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_MacHighlight

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

ZUIInk_MacHighlight::ZUIInk_MacHighlight(bool returnHilight)
:	fReturnHilight(returnHilight)
	{}

/* AG 98-09-04. If fReturnHilight is true, we're going to return the hilight Ink
itself. Otherwise we look to see how bright the highlight is, and return black if it's
a light Ink, or white if it's a dark Ink. This should guarantee we get decent contrast. */
ZDCInk ZUIInk_MacHighlight::GetInk()
	{
	RGBColor tempRGB;
	LMGetHiliteRGB(&tempRGB); // LMGetHiliteRGB may be a macro, hence no :: scope
	if (fReturnHilight)
		return ZRGBColor(tempRGB);
// AG 99-01-19. If the luminance is less than 15% switch to white
	if (ZRGBColor(tempRGB).NTSCLuminance() < 0.15)
		return ZRGBColor::sWhite;
	return ZRGBColor::sBlack;
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================
