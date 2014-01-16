/*  @(#) $Id: ZUIInk.h,v 1.4 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZUIInk__
#define __ZUIInk__ 1
#include "zconfig.h"

#include "ZUIBase.h"
#include "ZDCInk.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk

class ZUIInk : public ZUIBase
	{
public:
	ZUIInk() {}
	virtual ~ZUIInk() {}

	virtual ZDCInk GetInk() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Fixed

class ZUIInk_Fixed : public ZUIInk
	{
public:
	ZUIInk_Fixed(const ZDCInk& theInk);

	virtual ZDCInk GetInk();

	void SetInk(const ZDCInk& theInk);
private:
	ZDCInk fInk;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Indirect

class ZUIInk_Indirect : public ZUIInk
	{
public:
	ZUIInk_Indirect(ZRef<ZUIInk> realInk);
	virtual ~ZUIInk_Indirect();

// From ZUIInk
	virtual ZDCInk GetInk();

	void SetRealInk(ZRef<ZUIInk> realInk);
	ZRef<ZUIInk> GetRealInk();
private:
	ZRef<ZUIInk> fRealInk;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_Win32System

#if ZCONFIG(OS, Win32)

class ZUIInk_Win32System : public ZUIInk
	{
public:
	ZUIInk_Win32System(int systemColorIndex);

	virtual ZDCInk GetInk();
protected:
	int fSystemColorIndex;
	};

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZUIInk_MacHighlight

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

class ZUIInk_MacHighlight : public ZUIInk
	{
public:
	ZUIInk_MacHighlight(bool returnHilight);

	virtual ZDCInk GetInk();
protected:
	bool fReturnHilight;
	};

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================

#endif // __ZUIInk__
