/*  @(#) $Id: ZUIColor.h,v 1.4 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZUIColor__
#define __ZUIColor__
#include "zconfig.h"

#include "ZUIBase.h"
#include "ZRGBColor.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor

class ZUIColor : public ZUIBase
	{
public:
	ZUIColor() {}
	virtual ~ZUIColor() {}

	virtual ZRGBColor GetColor() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Fixed

class ZUIColor_Fixed : public ZUIColor
	{
public:
	ZUIColor_Fixed(const ZRGBColor& theColor);

	virtual ZRGBColor GetColor();

	void SetColor(const ZRGBColor& theColor);
private:
	ZRGBColor fColor;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Indirect

class ZUIColor_Indirect : public ZUIColor
	{
public:
	ZUIColor_Indirect(ZRef<ZUIColor> realColor);
	virtual ~ZUIColor_Indirect();

// From ZUColor
	virtual ZRGBColor GetColor();

	void SetRealColor(ZRef<ZUIColor> realColor);
	ZRef<ZUIColor> GetRealColor();

private:
	ZRef<ZUIColor> fRealColor;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Win32System

#if ZCONFIG(OS, Win32)

class ZUIColor_Win32System : public ZUIColor
	{
public:
	ZUIColor_Win32System(int systemColorIndex);

	virtual ZRGBColor GetColor();

protected:
	int fSystemColorIndex;
	};

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_Win32SystemLightened

#if ZCONFIG(OS, Win32)

class ZUIColor_Win32SystemLightened : public ZUIColor
	{
public:
	ZUIColor_Win32SystemLightened(int systemColorIndex);

	virtual ZRGBColor GetColor();
protected:
	int fSystemColorIndex;
	};

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIColor_MacHighlight

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

class ZUIColor_MacHighlight : public ZUIColor
	{
public:
	ZUIColor_MacHighlight(bool returnHilight);

	virtual ZRGBColor GetColor();

protected:
	bool fReturnHilight;
	};


#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================

#endif // __ZUIColor__
