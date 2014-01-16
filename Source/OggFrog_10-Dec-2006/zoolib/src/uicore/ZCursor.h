/*  @(#) $Id: ZCursor.h,v 1.1.1.1 2002/02/17 18:41:07 agreen Exp $ */

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

#ifndef __ZCursor__
#define __ZCursor__ 1
#include "zconfig.h"

#include "ZDCPixmap.h"
#include "ZGeom.h"

class ZCursor
	{
public:
	enum CursorType
		{
		eCursorTypeCustom,
		eCursorTypeArrow,
		eCursorTypeIBeam,
		eCursorTypeCross,
		eCursorTypePlus,
		eCursorTypeWatch,
		eCursorTypeSplitH,
		eCursorTypeSplitV
		};

	ZCursor();
	ZCursor(const ZCursor& inOther);
	ZCursor(CursorType inCursorType);
	ZCursor(const ZDCPixmapCombo& inPixmapCombo, const ZPoint& inHotSpot);
	ZCursor(const ZDCPixmap& inPixmapColor, const ZDCPixmap& inPixmapMono, const ZDCPixmap& inPixmapMask, const ZPoint& inHotSpot);

	ZCursor& operator=(const ZCursor& inOther);

	~ZCursor() {};

	bool IsCustom() const;

	void Set(CursorType inCursorType);
	void Set(const ZDCPixmapCombo& inPixmapCombo, const ZPoint& inHotSpot);
	void Set(const ZDCPixmap& inPixmapColor, const ZDCPixmap& inPixmapMono, const ZDCPixmap& inPixmapMask, const ZPoint& inHotSpot);

	CursorType GetCursorType() const;
	bool Get(ZDCPixmapCombo& outPixmapCombo, ZPoint& outHotSpot) const;
	bool Get(ZDCPixmap& outPixmapColor, ZDCPixmap& outPixmapMono, ZDCPixmap& outPixmapMask, ZPoint& outHotSpot) const;

private:
	CursorType fCursorType;

	ZDCPixmap fPixmapColor;
	ZDCPixmap fPixmapMono;
	ZDCPixmap fPixmapMask;

	ZPoint fHotSpot;
	};


#endif // __ZCursor__
