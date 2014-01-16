static const char rcsid[] = "@(#) $Id: ZUIFont.cpp,v 1.4 2006/07/12 19:41:08 agreen Exp $";

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

#include "ZUIFont.h"

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#	include "ZUtil_Win.h"
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont

ZDCFont ZUIFont::GetFont()
	{
	ZUnimplemented();
	return ZDCFont();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Fixed

ZUIFont_Fixed::ZUIFont_Fixed(const ZDCFont& theFont)
:	fFont(theFont)
	{}

ZDCFont ZUIFont_Fixed::GetFont()
	{ return fFont; }

void ZUIFont_Fixed::SetFont(const ZDCFont& theFont)
	{
	fFont = theFont;
	this->Changed();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Indirect

ZUIFont_Indirect::ZUIFont_Indirect(ZRef<ZUIFont> realFont)
:	fRealFont(realFont)
	{}

ZUIFont_Indirect::~ZUIFont_Indirect()
	{}

ZDCFont ZUIFont_Indirect::GetFont()
	{
	if (ZRef<ZUIFont> theRealFont = fRealFont)
		return theRealFont->GetFont();
	return ZUIFont::GetFont();
	}

void ZUIFont_Indirect::SetRealFont(ZRef<ZUIFont> realFont)
	{
	if (fRealFont == realFont)
		return;
	fRealFont = realFont;
	this->Changed();
	}

ZRef<ZUIFont> ZUIFont_Indirect::GetRealFont()
	{ return fRealFont; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Win32System

#if ZCONFIG(OS, Win32)

ZUIFont_Win32System::ZUIFont_Win32System(WhichFont inWhichFont)
:	fWhichFont(inWhichFont)
	{}

ZDCFont ZUIFont_Win32System::GetFont()
	{
	if (ZUtil_Win::sUseWAPI())
		{
		NONCLIENTMETRICSW ncm;
		ncm.cbSize = sizeof(ncm);
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		LOGFONTW* theLOGFONT = nil;
		switch (fWhichFont)
			{
			case eWhichFontCaption: theLOGFONT = &ncm.lfCaptionFont; break;
			case eWhichFontSmallCaption: theLOGFONT = &ncm.lfSmCaptionFont; break;
			case eWhichFontMenu: theLOGFONT = &ncm.lfMenuFont; break;
			case eWhichFontStatus: theLOGFONT = &ncm.lfStatusFont; break;
			case eWhichFontMessage: theLOGFONT = &ncm.lfMessageFont; break;
			default:
				ZUnimplemented();
			}
		ZDCFont::Style theStyle = ZDCFont::normal;
		if (theLOGFONT->lfItalic)
			theStyle |= ZDCFont::italic;
		if (theLOGFONT->lfUnderline)
			theStyle |= ZDCFont::underline;
		if (theLOGFONT->lfWeight > FW_NORMAL)
			theStyle |= ZDCFont::bold;
		return ZDCFont(theLOGFONT->lfFaceName, theStyle, -theLOGFONT->lfHeight);
		}
	else
		{
		NONCLIENTMETRICSA ncm;
		ncm.cbSize = sizeof(ncm);
		::SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		LOGFONTA* theLOGFONT = nil;
		switch (fWhichFont)
			{
			case eWhichFontCaption: theLOGFONT = &ncm.lfCaptionFont; break;
			case eWhichFontSmallCaption: theLOGFONT = &ncm.lfSmCaptionFont; break;
			case eWhichFontMenu: theLOGFONT = &ncm.lfMenuFont; break;
			case eWhichFontStatus: theLOGFONT = &ncm.lfStatusFont; break;
			case eWhichFontMessage: theLOGFONT = &ncm.lfMessageFont; break;
			default:
				ZUnimplemented();
			}
		ZAssert(theLOGFONT);
		ZDCFont::Style theStyle = ZDCFont::normal;
		if (theLOGFONT->lfItalic)
			theStyle |= ZDCFont::italic;
		if (theLOGFONT->lfUnderline)
			theStyle |= ZDCFont::underline;
		if (theLOGFONT->lfWeight > FW_NORMAL)
			theStyle |= ZDCFont::bold;
		return ZDCFont(theLOGFONT->lfFaceName, theStyle, -theLOGFONT->lfHeight);
		}
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
