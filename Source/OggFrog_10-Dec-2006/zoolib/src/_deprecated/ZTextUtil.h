/*  @(#) $Id: ZTextUtil.h,v 1.3 2004/05/05 15:52:28 agreen Exp $ */

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

#ifndef __ZTextUtil__
#define __ZTextUtil__ 1
#include "zconfig.h"

#include "ZGeom.h"

#include <string>
#include <vector>

class ZDC;

void ZTextUtil_sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const char* inText, size_t inTextLength);

namespace ZTextUtil {

size_t sCalcNextLineBreak(const ZDC& inDC, ZCoord inWidth, const char* inText, size_t inTextLength, size_t inOffset);
size_t sCalcNextLineBreak(const ZDC& inDC, ZCoord inWidth, const std::string& inString, size_t inOffset);

ZPoint sCalcSize(const ZDC& inDC, ZCoord inWidth, const char* inText, size_t inTextLength);
ZPoint sCalcSize(const ZDC& inDC, ZCoord inWidth, const std::string& inString);

//void sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const char* inText, size_t inTextLength);
void sDraw(const ZDC& inDC, ZPoint inLocation, ZCoord inWidth, const std::string& inString);

// AG 2001-03-16. This entire suite will be revised soon to support multiple styles/scripts/directions. The LineBreak
// stuff will be deprecated, because it's generally more efficient and flexible to recast the calling code so
// that it walks the source material itself, rather than gathering a full set of linebreak offsets, oftentimes
// ignoring most of them.

class LineBreak;
void sCalcLineBreaks(const ZDC& inDC, ZCoord inWidth, const std::string& inString, std::vector<LineBreak>& outLineBreaks);
void sCalcLineBreaks(const ZDC& inDC, ZCoord inWidth1, ZCoord inWidth2, const std::string& inString, std::vector<LineBreak>& outLineBreaks);

} // namespace ZTextUtil

class ZTextUtil::LineBreak
	{
public:
	LineBreak();
	LineBreak(size_t inStart, size_t inSize);
	size_t Start() { return fStart;}
	size_t Size() { return fSize;}
private:
	size_t fStart;
	size_t fSize;
	};

#endif // __ZTextUtil__
