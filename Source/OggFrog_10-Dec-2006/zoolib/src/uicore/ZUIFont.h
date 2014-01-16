/*  @(#) $Id: ZUIFont.h,v 1.4 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZUIFont__
#define __ZUIFont__
#include "zconfig.h"

#include "ZUIBase.h"
#include "ZDCFont.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont

class ZUIFont : public ZUIBase
	{
public:
	ZUIFont() {}
	virtual ~ZUIFont() {}

	virtual ZDCFont GetFont() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Fixed

class ZUIFont_Fixed : public ZUIFont
	{
public:
	ZUIFont_Fixed(const ZDCFont& theFont);

	virtual ZDCFont GetFont();

	void SetFont(const ZDCFont& theFont);
private:
	ZDCFont fFont;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Indirect

class ZUIFont_Indirect : public ZUIFont
	{
public:
	ZUIFont_Indirect(ZRef<ZUIFont> realFont);
	virtual ~ZUIFont_Indirect();

// From ZUIInk
	virtual ZDCFont GetFont();

	void SetRealFont(ZRef<ZUIFont> realFont);
	ZRef<ZUIFont> GetRealFont();
private:
	ZRef<ZUIFont> fRealFont;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFont_Win32System

#if ZCONFIG(OS, Win32)

class ZUIFont_Win32System : public ZUIFont
	{
public:
	enum WhichFont { eWhichFontCaption, eWhichFontSmallCaption, eWhichFontMenu, eWhichFontStatus, eWhichFontMessage };
	ZUIFont_Win32System(WhichFont inWhichFont);

	virtual ZDCFont GetFont();
protected:
	WhichFont fWhichFont;
	};

#endif // ZCONFIG(OS, Win32)

#endif // __ZUIFont__
