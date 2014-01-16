static const char rcsid[] = "@(#) $Id: ZPrintDroid.cpp,v 1.3 2002/10/01 19:04:00 agreen Exp $";

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

#include "ZPrintDroid.h"


// ==================================================
// headerFooter will be used to do .... headers & footers!
ZPrintDroid::ZPrintDroid(ZRef<ZPrintJob> thePrintJob, 
						const ZPrintJobSetup::Options& theJobOptions,
						bool usePageDC, HeaderFooter* headerFooter)
:	fPrintJob(thePrintJob), fJobOptions(theJobOptions),
	fUsePageDC(usePageDC), fHeaderFooter(headerFooter),
	fCurrentPage(1), fCurrentOffset(0),
	fGotFreshPage(false), fFirstCall(true), fImagedHeaderFooter(false), 
	fCalculatedHeaderFooter(false)
	{
	}


bool ZPrintDroid::ImageThis(Printable* thePrintable)
	{
// Loop around until we've imaged the whole of the Printable (mostly we'll only
// run through the loop once, but this handles the general case of Printables that are
// taller than one page, or run over a page boundary
	while (true)
		{
// We're going to need a *real* page available at all times, even when
// we're just passing over non printing pages. If we pass a dummy DC then
// metrics calculated by non drawing Printables may be off and
// page breaks in the wrong places. We don't set up the page in our
// constructor, cause we may never have ImageThis called and we don't want
// a blank page being spat out if nothing gets printed.

// We'll also need to start up a new page if we've moved on to the top
// of a new page when we're really printing. fGotFreshPage indicates
// that we've done a NewPage but haven't done any drawing yet
		if (fFirstCall || fCurrentOffset <= 0)
			{
// We were initialized with fCurrentPage == 1, when we should have
// it == to zero. But if we do that, then the result of GetCurrentPage
// will be zero until we've called ImageThis. Hence this special case
			if (!fFirstCall)
				fCurrentPage++;
// If the current page is beyond the end of the range requested by the user then bail
// This only works when we have an explicit page range, otherwise we just keep
// going so long as we're being fed Printables
			if ((fJobOptions.fPageRange == ZPrintJobSetup::Options::ePageRangeExplicit) &&
					(fCurrentPage > fJobOptions.fLastPage))
				break;

			if (fFirstCall || !fGotFreshPage)
				{
				fPrintJob->NewPage();
				fGotFreshPage = true;
				fCalculatedHeaderFooter = false;
				fImagedHeaderFooter = false;
				}
			}

// We're drawing if we don't have an explicit page range or we do and we're past the first page
		bool reallyDraw = ((fJobOptions.fPageRange != ZPrintJobSetup::Options::ePageRangeExplicit) ||
											(fCurrentPage >= fJobOptions.fFirstPage));

// Deal with the headers and footers. Again, these are a little fiddly. We
// need to have their sizes so pages break correctly, but they also
// have to be actually drawn when we've got a new page. However, we
// can't just draw them whenever we do a new page, because do a new page
// when we first get called, even if we're not inside the printing range yet

		if (fHeaderFooter && fGotFreshPage && (!fCalculatedHeaderFooter || !fImagedHeaderFooter))
			{
			if ((reallyDraw && !fImagedHeaderFooter) || !fCalculatedHeaderFooter)
				{
				ZDC headerFooterDC(fUsePageDC ? fPrintJob->GetPageDC() : fPrintJob->GetPaperDC());
				ZDCRgn availableRgn = fHeaderFooter->ImageHeaderFooter(this, headerFooterDC, reallyDraw);
				fNonHFArea = availableRgn.Bounds();
				ZAssert(!fNonHFArea.IsEmpty());
				fCalculatedHeaderFooter = true;
				fImagedHeaderFooter = reallyDraw;
				}
			}

		fFirstCall = false;

		ZDC localDC(fUsePageDC ? fPrintJob->GetPageDC() : fPrintJob->GetPaperDC());
		if (fHeaderFooter)
			localDC.SetClip(fNonHFArea);

		ZRect pageRect(localDC.GetClip().Bounds());

		ZCoord spaceConsumed = thePrintable->ImageYourself(this, localDC, pageRect.Size(), reallyDraw, ZPoint(0, fCurrentOffset + pageRect.top));
// Move down the page
		fCurrentOffset += spaceConsumed;
// And mark the page as dirtied iff we consumed space, and we were really drawing
		if (spaceConsumed > 0 && reallyDraw)
			fGotFreshPage = false;

// If we're still on the same page, then this Printable has been imaged
// completely and we can bail from this loop
		if (fCurrentOffset < pageRect.Height())
			break;
		if (fCurrentOffset == pageRect.Height())
			{
			fCurrentOffset = 0;
			break;
			}

// Move the current offset to be <= 0 on the *next* page (we actually move on to the
// next page at the beginning of this loop)
		fCurrentOffset -= pageRect.Height();
		fCurrentOffset -= spaceConsumed;
		}
// Returns true if we've imaged all the pages that were requested, false otherwise
	return ((fJobOptions.fPageRange == ZPrintJobSetup::Options::ePageRangeExplicit) &&
										(fCurrentPage > fJobOptions.fLastPage));
	}
