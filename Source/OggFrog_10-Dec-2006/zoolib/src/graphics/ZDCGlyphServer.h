/*  @(#) $Id: ZDCGlyphServer.h,v 1.4 2006/04/26 22:31:48 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZDCGlyphServer__
#define __ZDCGlyphServer__ 1
#include "zconfig.h"

#include "ZDCFont.h"
#include "ZDCPixmap.h"
#include "ZUnicode.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDCGlyphServer

class ZDCGlyphServer : public ZRefCountedWithFinalization
	{
protected:
	ZDCGlyphServer();
	virtual ~ZDCGlyphServer();

public:
	virtual bool GetGlyph(const ZDCFont& iFont, UTF32 iCP,
		ZPoint& oOrigin, ZCoord& oEscapement, ZDCPixmap& oPixmap) = 0;

	virtual ZCoord GetEscapement(const ZDCFont& iFont, UTF32 iCP) = 0;

	virtual void GetFontInfo(const ZDCFont& iFont,
		ZCoord& oAscent, ZCoord& oDescent, ZCoord& oLeading) = 0;
	};

#endif // __ZDCGlyphServer__
