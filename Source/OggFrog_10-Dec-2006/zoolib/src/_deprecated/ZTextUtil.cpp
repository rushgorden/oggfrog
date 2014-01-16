static const char rcsid[] = "@(#) $Id: ZTextUtil.cpp,v 1.3 2006/07/23 22:03:22 agreen Exp $";

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

#include "ZTextUtil.h"
#include "ZDC.h"

using std::min;
using std::string;
using std::vector;

ZTextUtil::LineBreak::LineBreak()
:	fStart(0), fSize(0)
	{}

ZTextUtil::LineBreak::LineBreak(size_t inStart, size_t inSize)
:	fStart(inStart), fSize(inSize)
	{}

void ZTextUtil::sCalcLineBreaks(const ZDC& inDC, ZCoord inWidth, const string& inString, vector<ZTextUtil::LineBreak>& outLineBreaks)
	{
	if (inString.size() == 0)
		return;

	//ZCoord ascent, descent, leading;
	//inDC.GetFontInfo(ascent, descent, leading);

	const char* textStart = &inString.at(0);
	size_t currentLineOffset = 0;
	size_t remainingTextLength = inString.size();

	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
		inDC.BreakLine(textStart + currentLineOffset, remainingTextLength, inWidth, nextLineDelta);
		outLineBreaks.push_back(LineBreak(currentLineOffset, nextLineDelta));
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}
	}

// This version takes a width for the first line and another for subsequent lines. This is useful when the display of a string 
// will start on one that has already been partially "consumed" by some other element(s). -ec 99.09.13
void ZTextUtil::sCalcLineBreaks(const ZDC& inDC, ZCoord inWidth1, ZCoord inWidth2, const string& inString, vector<ZTextUtil::LineBreak>& outLineBreaks)
	{
	if (inString.size() == 0)
		return;

	//ZCoord ascent, descent, leading;
	//inDC.GetFontInfo(ascent, descent, leading);
	ZCoord availableWidth = inWidth1;

	const char* textStart = &inString.at(0);
	size_t currentLineOffset = 0;
	size_t remainingTextLength = inString.size();

	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
#if 1 // rough fix. -ec 99.11.18
		if (availableWidth < 0)
			availableWidth = 0;
#else
		ZAssert(availableWidth >= 0);
#endif
		inDC.BreakLine(textStart + currentLineOffset, remainingTextLength, availableWidth, nextLineDelta);
		outLineBreaks.push_back(LineBreak(currentLineOffset, nextLineDelta));
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		availableWidth = inWidth2;
		}
	}

//## This lets some calling routine do the while-loop over the text.
// AG 2000-04-01. Of course, it would be better to recast the calling code to manage the issue itself,
// however we live in an imperfect universe. :) This will be going away soon.
size_t ZTextUtil::sCalcNextLineBreak(const ZDC& inDC, ZCoord inWidth, const char* inText, size_t inTextLength, size_t inOffset)
	{
#if 1 // the alternative couldn't be right! -ec 01.10.17
	if (inTextLength == 0)
		return 0;
#else
	if (inTextLength)
		return 0;
#endif

	size_t remainingTextLength = inTextLength - inOffset;

// this guarantees that we won't get overflows, and crashes with strings longer than 32767 bytes. -ec 00.02.24
	remainingTextLength = min(remainingTextLength, size_t(32767));

	size_t nextLineDelta;
	inDC.BreakLine(inText + inOffset, remainingTextLength, inWidth, nextLineDelta);
	return nextLineDelta;
	}

size_t ZTextUtil::sCalcNextLineBreak(const ZDC& inDC, ZCoord inWidth, const string& inString, size_t inOffset)
	{ return sCalcNextLineBreak(inDC, inWidth, inString.data(), inString.size(), inOffset); }

ZPoint ZTextUtil::sCalcSize(const ZDC& inDC, ZCoord inWidth, const char* inText, size_t inTextLength)
	{
	ZPoint stringDimensions = ZPoint::sZero;
	if (inTextLength == 0) 
		return stringDimensions;

	ZCoord ascent, descent, leading;
	inDC.GetFontInfo(ascent, descent, leading);

	size_t currentLineOffset = 0;
	size_t remainingTextLength = inTextLength;

	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
		inDC.BreakLine(inText + currentLineOffset, remainingTextLength, inWidth, nextLineDelta);
		ZCoord brokenWidth = inDC.GetTextWidth(inText + currentLineOffset, nextLineDelta);
		if (stringDimensions.h < brokenWidth)
			stringDimensions.h = brokenWidth;
		stringDimensions.v += ascent + descent;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}

	return stringDimensions;
	}

ZPoint ZTextUtil::sCalcSize(const ZDC& inDC, ZCoord inWidth, const string& inString)
	{ return sCalcSize(inDC, inWidth, inString.data(), inString.size()); }

//void ZTextUtil::sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const char* inText, size_t inTextLength)
void ZTextUtil_sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const char* inText, size_t inTextLength)
	{
	if (inTextLength == 0) 
		return;

	ZCoord ascent, descent, leading;
	inDC.GetFontInfo(ascent, descent, leading);
	inLocation.v += ascent;

	size_t currentLineOffset = 0;
	size_t remainingTextLength = inTextLength;

	while (remainingTextLength > 0)
		{
		size_t nextLineDelta;
		inDC.BreakLine(inText + currentLineOffset, remainingTextLength, inWidth, nextLineDelta);
		inDC.DrawText(inLocation, inText + currentLineOffset, nextLineDelta);
		inLocation.v += ascent + descent;
		currentLineOffset += nextLineDelta;
		remainingTextLength -= nextLineDelta;
		}
	}

void ZTextUtil::sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const string& inString)
	{
	if (size_t theSize = inString.size())
		ZTextUtil_sDraw(inDC, inLocation, inWidth, inString.data(), theSize);
	}
