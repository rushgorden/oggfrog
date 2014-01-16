static const char rcsid[] = "@(#) $Id: ZPrint.cpp,v 1.8 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZPrint.h"
#include "ZPrint_Carbon.h"
#include "ZPrint_MacClassic.h"
#include "ZPrint_Win.h"
#include "ZUtil_Win.h"

ZRef<ZPrintPageSetup> ZPrint::sCreatePageSetup()
	{
#if ZCONFIG(OS, MacOS7)
	return new ZPrintPageSetup_MacClassic;
#elif ZCONFIG(OS, Carbon)
	return new ZPrintPageSetup_Carbon;
#elif ZCONFIG(OS, Win32)
	if (ZUtil_Win::sUseWAPI())
		return new ZPrintPageSetup_WinW;
	else
		return new ZPrintPageSetup_WinA;
#endif
	return ZRef<ZPrintPageSetup>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup

ZPrintPageSetup::ZPrintPageSetup()
	{}

ZPrintPageSetup::ZPrintPageSetup(const ZPrintPageSetup&)
	{}

ZPrintPageSetup::~ZPrintPageSetup()
	{}

void ZPrintPageSetup::ToStream(const ZStreamW& inStream)
	{}

void ZPrintPageSetup::FromStream(const ZStreamR& inStream)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup::Options

ZPrintJobSetup::Options::Options()
	{
	fPageRange = ePageRangeAll;
	fFirstPage = 0;
	fLastPage = 0;
	fAvailableKnown = availableUnknown;
	fMinPage = 0;
	fMaxPage = 0;
	fCollate = false;
	fNumberOfCopies = 1;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup

ZPrintJobSetup::ZPrintJobSetup()
	{}

ZPrintJobSetup::ZPrintJobSetup(const ZPrintJobSetup&)
	{}

ZPrintJobSetup::~ZPrintJobSetup()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob

ZPrintJob::ZPrintJob()
	{}

ZPrintJob::~ZPrintJob()
	{}

