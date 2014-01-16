static const char rcsid[] = "@(#) $Id: ZUIColor.cpp,v 1.4 2006/07/15 20:54:22 goingware Exp $";

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
#include "ZUIColor.h"

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
#pragma mark * ZUIColor

ZRGBColor ZUIColor::GetColor()
	{
	ZUnimplemented();
	return ZRGBColor::sBlack;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Fixed

ZUIColor_Fixed::ZUIColor_Fixed(const ZRGBColor& theColor)
:	fColor(theColor)
	{}

ZRGBColor ZUIColor_Fixed::GetColor()
	{ return fColor; }

void ZUIColor_Fixed::SetColor(const ZRGBColor& theColor)
	{
	fColor = theColor;
	this->Changed();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Indirect

ZUIColor_Indirect::ZUIColor_Indirect(ZRef<ZUIColor> realColor)
:	fRealColor(realColor)
	{}

ZUIColor_Indirect::~ZUIColor_Indirect()
	{}

ZRGBColor ZUIColor_Indirect::GetColor()
	{
	if (ZRef<ZUIColor> theRealColor = fRealColor)
		return theRealColor->GetColor();
	return ZUIColor::GetColor();
	}

void ZUIColor_Indirect::SetRealColor(ZRef<ZUIColor> realColor)
	{
	if (fRealColor == realColor)
		return;
	fRealColor = realColor;
	this->Changed();
	}

ZRef<ZUIColor> ZUIColor_Indirect::GetRealColor()
	{ return fRealColor; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Win32System

#if ZCONFIG(OS, Win32)

ZUIColor_Win32System::ZUIColor_Win32System(int systemColorIndex)
:	fSystemColorIndex(systemColorIndex)
	{}

ZRGBColor ZUIColor_Win32System::GetColor()
	{
	return ZRGBColor::sFromCOLORREF(::GetSysColor(fSystemColorIndex));
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Win32SystemLightened

#if ZCONFIG(OS, Win32)

ZUIColor_Win32SystemLightened::ZUIColor_Win32SystemLightened(int systemColorIndex)
:	fSystemColorIndex(systemColorIndex)
	{}

ZRGBColor ZUIColor_Win32SystemLightened::GetColor()
	{
	ZRGBColor theRGBColor = ZRGBColor::sFromCOLORREF(::GetSysColor(fSystemColorIndex));
	return ZRGBColor::sWhite - (ZRGBColor::sWhite - theRGBColor) / 2;
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_MacHighlight

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

ZUIColor_MacHighlight::ZUIColor_MacHighlight(bool returnHilight)
:	fReturnHilight(returnHilight)
	{}

/* AG 98-09-04. If fReturnHilight is true, we're going to return the hilight color
itself. Otherwise we look to see how bright the highlight is, and return black if it's
a light color, or white if it's a dark color. This should guarantee we get decent contrast. */
ZRGBColor ZUIColor_MacHighlight::GetColor()
	{
	RGBColor tempRGB;
	// LMGetHiliteRGB is a macro, hence no :: scope
	LMGetHiliteRGB(&tempRGB);

	ZRGBColor theRGBColor = tempRGB;
	if (fReturnHilight)
		return theRGBColor;

	// AG 99-01-19. If the luminance is less than 15% switch to white
	if (theRGBColor.NTSCLuminance() < 0.15)
		return ZRGBColor::sWhite;

	return ZRGBColor::sBlack;
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================
