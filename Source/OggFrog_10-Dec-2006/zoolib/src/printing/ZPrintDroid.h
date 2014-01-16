/*  @(#) $Id: ZPrintDroid.h,v 1.3 2002/10/01 19:04:01 agreen Exp $ */

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

#ifndef __ZPrintDroid__
#define __ZPrintDroid__ 1
#include "zconfig.h"

#include "ZGeom.h"
#include "ZPrint.h"


// Okay, here's a sketch of what we talked about. I didn't put much thought into class
// names and details of organization, so butcher this all you like. Two main sections:
// the ZPrintDroid stuff, and the KBNotePrinter stuff. The first has the logic for
// laying out a bunch of stuff down a page, the second has the logic for fetching stuff
// from the server in one thread, whilst actually printing it from the main thread


// Eric -- Printable here is not necessarily the same as ZPrint::Printable, so
// I'm putting a declaration of Printable in here with the changed ImageYourself
// signature just for exposition purposes. Possibly Printable here should be
// ZPrintDroid::Printable, just to be clear. In fact, that's what I'm gonna do


class ZPrintDroid
	{
public:
	class Printable;
	class HeaderFooter;
	ZPrintDroid(ZRef<ZPrintJob> thePrintJob, 
				const ZPrintJobSetup::Options& theJobOptions,
				bool usePageDC = true, HeaderFooter* headerFooter = nil);

	bool ImageThis(Printable* thePrintable);

	long GetCurrentPage() const
		{ return fCurrentPage; }
	ZRef<ZPrintJob> GetPrintJob() const
		{ return fPrintJob; }
private:
	ZPrintJobSetup::Options fJobOptions;
	ZRef<ZPrintJob> fPrintJob;
	long fCurrentPage;
	ZCoord fCurrentOffset;
	bool fGotFreshPage;
	bool fFirstCall;
	bool fImagedHeaderFooter;
	bool fCalculatedHeaderFooter;
	bool fUsePageDC;
	HeaderFooter* fHeaderFooter;
	ZRect fNonHFArea;
	};

class ZPrintDroid::Printable
	{
public:
	virtual ~Printable() {}
// Return the amount of vertical space consumed by the imaging process (even
// if it doesn't actually draw (very important, otherwise pagination won't work)
// We pass in the DC to draw on, the width of the page, a flag to say whether to
// actually draw, and the origin point at which to start drawing
	virtual ZCoord ImageYourself(const ZPrintDroid* thePrintDroid, 
													ZDC& theDC, ZPoint pageSize, 
													bool reallyDraw, ZPoint location) = 0;
	};

class ZPrintDroid::HeaderFooter
	{
public:
	virtual ~HeaderFooter() {}
// Return a rgn specifying the area of the page *not* used by the header & footer
	virtual ZDCRgn ImageHeaderFooter(const ZPrintDroid* thePrintDroid, 
															const ZDC& theDC, bool reallyDraw) = 0;
	};

#endif // __ZPrintDroid__
