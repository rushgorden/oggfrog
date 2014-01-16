/*  @(#) $Id: ZRegionAdorner.h,v 1.2 2002/03/15 19:08:01 agreen Exp $ */

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

#ifndef __ZRegionAdorner__
#define __ZRegionAdorner__ 1
#include "zconfig.h"

#include "ZPane.h"

class ZRegionAdorner : public ZPaneAdorner
	{
public:
	ZRegionAdorner();
	ZRegionAdorner(const ZDCRgn& inRgn, const ZDCPattern& inPattern);
	ZRegionAdorner(const ZDCPattern& inPattern);

	virtual void AdornPane(const ZDC& inDC, ZSubPane* inPane, const ZDCRgn& inPaneBoundsRgn);

	void SetRgn(const ZDC& inDC, const ZDCRgn& inRgn);
	void SetPattern(const ZDC& inDC, const ZDCPattern& inPattern);
	void SetRgnPattern(const ZDC& inDC, const ZDCRgn& inRgn, const ZDCPattern& inPattern);
	const ZDCRgn& GetRgn() const
		{ return fRgn; }
	const ZDCPattern& GetPattern() const
		{ return fPattern; }
private:
	ZDCRgn fRgn;
	ZDCPattern fPattern;
	};


#endif // __ZRegionAdorner__
