static const char rcsid[] = "@(#) $Id: ZRegionAdorner.cpp,v 1.2 2003/09/29 01:10:50 agreen Exp $";

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

#include "ZRegionAdorner.h"

#include "ZUtil_Graphics.h"

// ==================================================

ZRegionAdorner::ZRegionAdorner()
	{}

ZRegionAdorner::ZRegionAdorner(const ZDCRgn& inRgn, const ZDCPattern& inPattern)
:	fRgn(inRgn), fPattern(inPattern)
	{}

ZRegionAdorner::ZRegionAdorner(const ZDCPattern& inPattern)
:	fPattern(inPattern)
	{}

void ZRegionAdorner::AdornPane(const ZDC& inDC, ZSubPane* inPane, const ZDCRgn& inPaneBoundsRgn)
	{
	ZDC localDC(inDC);
	localDC.SetMode(ZDC::modeXor);
	localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, fPattern));
	localDC.Fill(fRgn);
	}

void ZRegionAdorner::SetRgnPattern(const ZDC& inDC, const ZDCRgn& inRgn, const ZDCPattern& inPattern)
	{
// This is fun! We basically need to difference the rgns, and the patterns, and
// use xor mode to do minimal redraws. First used in Marrakech, and subsequently in MiM
	ZDCRgn oldRgnToPaint;
	ZDCRgn newRgnToPaint;
	ZDCRgn diffRgnToPaint;
	ZUtil_Graphics::sCalculateRgnDifferences(fRgn, inRgn, &oldRgnToPaint, &newRgnToPaint, &diffRgnToPaint);

	ZDCPattern diffPattern;
	bool patternChanged;
	ZUtil_Graphics::sCalculatePatternDifference(fPattern, inPattern, &diffPattern, &patternChanged);

	ZDC localDC(inDC);
	localDC.SetMode(ZDC::modeXor);
	if (oldRgnToPaint)
		{
		localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, fPattern));
		localDC.Fill(oldRgnToPaint);
		}

	if (patternChanged && diffRgnToPaint)
		{
		localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, diffPattern));
		localDC.Fill(diffRgnToPaint);
		}

	if (newRgnToPaint)
		{
		localDC.SetInk(ZDCInk(ZRGBColor::sBlack, ZRGBColor::sWhite, inPattern));
		localDC.Fill(newRgnToPaint);
		}

	fPattern = inPattern;
	fRgn = inRgn;
	}


void ZRegionAdorner::SetPattern(const ZDC& inDC, const ZDCPattern& inPattern)
	{ this->SetRgnPattern(inDC, fRgn, inPattern); }

void ZRegionAdorner::SetRgn(const ZDC& inDC, const ZDCRgn& inRgn)
	{ this->SetRgnPattern(inDC, inRgn, fPattern); }

