static const char rcsid[] = "@(#) $Id: ZCursor.cpp,v 1.2 2003/09/29 01:10:03 agreen Exp $";

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

#include "ZCursor.h"

ZCursor::ZCursor()
:	fCursorType(eCursorTypeArrow)
	{}

ZCursor::ZCursor(const ZCursor& inOther)
:	fCursorType(inOther.fCursorType),
	fPixmapColor(inOther.fPixmapColor),
	fPixmapMono(inOther.fPixmapMono),
	fPixmapMask(inOther.fPixmapMask),
	fHotSpot(inOther.fHotSpot)
	{
	}

ZCursor::ZCursor(CursorType inCursorType)
:	fCursorType(inCursorType)
	{
	if (fCursorType == eCursorTypeCustom)
		fCursorType = eCursorTypeArrow;
	}

ZCursor::ZCursor(const ZDCPixmapCombo& inPixmapCombo, const ZPoint& inHotSpot)
:	fCursorType(eCursorTypeCustom),
	fHotSpot(inHotSpot)
	{
	inPixmapCombo.GetPixmaps(fPixmapColor, fPixmapMono, fPixmapMask);
	if ((!fPixmapColor && !fPixmapMono) || !fPixmapMask)
		fCursorType = eCursorTypeArrow;
	}

ZCursor::ZCursor(const ZDCPixmap& inPixmapColor, const ZDCPixmap& inPixmapMono, const ZDCPixmap& inPixmapMask, const ZPoint& inHotSpot)
:	fCursorType(eCursorTypeCustom),
	fPixmapColor(inPixmapColor),
	fPixmapMono(inPixmapMono),
	fPixmapMask(inPixmapMask),
	fHotSpot(inHotSpot)
	{
	if ((!fPixmapColor && !fPixmapMono) || !fPixmapMask)
		fCursorType = eCursorTypeArrow;
	}

ZCursor& ZCursor::operator=(const ZCursor& inOther)
	{
	fCursorType = inOther.fCursorType;
	fPixmapColor = inOther.fPixmapColor;
	fPixmapMono = inOther.fPixmapMono;
	fPixmapMask = inOther.fPixmapMask;
	fHotSpot = inOther.fHotSpot;
	return *this;
	}

bool ZCursor::IsCustom() const
	{ return fCursorType == eCursorTypeCustom; }

void ZCursor::Set(CursorType inCursorType)
	{
	if (inCursorType == fCursorType)
		return;

	if (inCursorType == eCursorTypeCustom)
		inCursorType = eCursorTypeArrow;

	fCursorType = inCursorType;
	fPixmapColor = ZDCPixmap();
	fPixmapMono = ZDCPixmap();
	fPixmapMask = ZDCPixmap();
	}

void ZCursor::Set(const ZDCPixmapCombo& inPixmapCombo, const ZPoint& inHotSpot)
	{
	fCursorType = eCursorTypeCustom;
	inPixmapCombo.GetPixmaps(fPixmapColor, fPixmapMono, fPixmapMask);
	fHotSpot = inHotSpot;

	if ((!fPixmapColor && !fPixmapMono) || !fPixmapMask)
		fCursorType = eCursorTypeArrow;
	}

void ZCursor::Set(const ZDCPixmap& inPixmapColor, const ZDCPixmap& inPixmapMono, const ZDCPixmap& inPixmapMask, const ZPoint& inHotSpot)
	{
	fCursorType = eCursorTypeCustom;
	fPixmapColor = inPixmapColor;
	fPixmapMono = inPixmapMono;
	fPixmapMask = inPixmapMask;
	fHotSpot = inHotSpot;
	if ((!fPixmapColor && !fPixmapMono) || !fPixmapMask)
		fCursorType = eCursorTypeArrow;
	}

ZCursor::CursorType ZCursor::GetCursorType() const
	{ return fCursorType; }

bool ZCursor::Get(ZDCPixmapCombo& outPixmapCombo, ZPoint& outHotSpot) const
	{
	if (fCursorType != eCursorTypeCustom)
		return false;
	outPixmapCombo.SetPixmaps(fPixmapColor, fPixmapMono, fPixmapMask);
	outHotSpot = fHotSpot;
	return true;
	}

bool ZCursor::Get(ZDCPixmap& outPixmapColor, ZDCPixmap& outPixmapMono, ZDCPixmap& outPixmapMask, ZPoint& outHotSpot) const
	{
	if (fCursorType != eCursorTypeCustom)
		return false;
	outPixmapColor = fPixmapColor;
	outPixmapMono = fPixmapMono;
	outPixmapMask = fPixmapMask;
	outHotSpot = fHotSpot;
	return true;
	}
