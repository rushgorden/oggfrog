static const char rcsid[] = "@(#) $Id: ZPrint_MacClassic.cpp,v 1.5 2006/07/12 19:38:02 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZPrint_MacClassic.h"

#if ZCONFIG(OS, MacOS7)

#include "ZDC_QD.h"
#include "ZString.h"
#include "ZUtil_Mac_HL.h"
#include "ZUtil_Mac_LL.h"

#include <Resources.h>


// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_MacClassic

ZPrintPageSetup_MacClassic::ZPrintPageSetup_MacClassic(const ZPrintPageSetup_MacClassic& iOther)
	{
	ZAssertStop(2, iOther.fTHPrint);
	fTHPrint = iOther.fTHPrint;
	::HandToHand((Handle*)&fTHPrint);
	}

ZPrintPageSetup_MacClassic::ZPrintPageSetup_MacClassic()
	{
	fTHPrint = (THPrint)::NewHandleClear(sizeof(TPrint));
	if (fTHPrint == nil)
		throw bad_alloc();
	::PrOpen();
	::PrintDefault(fTHPrint);
	::PrClose();
	::PrValidate(fTHPrint);
	}

ZPrintPageSetup_MacClassic::~ZPrintPageSetup_MacClassic()
	{
	if (fTHPrint)
		::DisposeHandle((Handle)fTHPrint);
	}

ZRef<ZPrintPageSetup> ZPrintPageSetup_MacClassic::Clone()
	{ return new ZPrintPageSetup_MacClassic(*this); }

bool ZPrintPageSetup_MacClassic::DoPageSetupDialog()
	{
	::PrOpen();
	ZUtil_Mac_HL::sPreDialog();
	bool result = false;
	if (::PrStlDialog(fTHPrint))
		result = true;
	ZUtil_Mac_HL::sPostDialog();
	::PrClose();
	return result;
	}

ZRef<ZPrintJobSetup> ZPrintPageSetup_MacClassic::CreateJobSetup()
	{
	THPrint theTHPrint = fTHPrint;
	::HandToHand(&((Handle)theTHPrint));
	if (theTHPrint == nil)
		throw bad_alloc();
	return new ZPrintJobSetup_MacClassic(theTHPrint);
	}

ZPoint_T<double> ZPrintPageSetup_MacClassic::GetPageSize()
	{ return ZRect_T<double>(fTHPrint[0]->prInfo.rPage).Size(); }
	
ZPoint_T<double> ZPrintPageSetup_MacClassic::GetPageOrigin()
	{
	ZRect_T<double> paperRect(fTHPrint[0]->rPaper);
	ZRect_T<double> pageRect(fTHPrint[0]->prInfo.rPage);
	return pageRect.TopLeft() - paperRect.TopLeft();
	}

ZPoint_T<double> ZPrintPageSetup_MacClassic::GetPaperSize()
	{ return ZRect_T<double>(fTHPrint[0]->rPaper).Size(); }

ZPoint_T<double> ZPrintPageSetup_MacClassic::GetUnits()
	{ return ZPoint_T<double>(fTHPrint[0]->prInfo.iHRes, fTHPrint[0]->prInfo.iVRes); }

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_MacClassic

ZPrintJobSetup_MacClassic::ZPrintJobSetup_MacClassic(const ZPrintJobSetup_MacClassic& iOther)
	{
	ZAssertStop(2, iOther.fTHPrint);
	fTHPrint = iOther.fTHPrint;
	::HandToHand((Handle*)&fTHPrint);
	if (fTHPrint == nil)
		throw bad_alloc();
	fOptions = iOther.fOptions;
	}

ZPrintJobSetup_MacClassic::ZPrintJobSetup_MacClassic(THPrint iTHPrint)
:	fTHPrint(iTHPrint)
	{}

ZPrintJobSetup_MacClassic::~ZPrintJobSetup_MacClassic()
	{
	if (fTHPrint)
		::DisposeHandle((Handle)fTHPrint);
	}

ZRef<ZPrintJobSetup> ZPrintJobSetup_MacClassic::Clone()
	{ return new ZPrintJobSetup_MacClassic(*this); }

void ZPrintJobSetup_MacClassic::SetOptions(const Options& iOptions)
	{
	fOptions = iOptions;
	fTHPrint[0]->prJob.iFstPage = fOptions.fFirstPage;
	fTHPrint[0]->prJob.iLstPage = fOptions.fLastPage;
	}

ZPrintJobSetup::Options ZPrintJobSetup_MacClassic::GetOptions()
	{
	Options theOptions = fOptions;
	long localFirstPage = min(fTHPrint[0]->prJob.iFstPage, fTHPrint[0]->prJob.iLstPage);
	long localLastPage = max(fTHPrint[0]->prJob.iFstPage, fTHPrint[0]->prJob.iLstPage);
	if (localFirstPage == 1 && localLastPage == 9999)
		theOptions.fPageRange = Options::ePageRangeAll;
	else
		theOptions.fPageRange = Options::ePageRangeExplicit;
	theOptions.fFirstPage = localFirstPage;
	theOptions.fLastPage = localLastPage;
	return theOptions;
	}

ZRef<ZPrintJob> ZPrintJobSetup_MacClassic::DoPrintDialogAndStartJob(const string& iJobName)
	{
	::PrOpen();
	ZUtil_Mac_HL::sPreDialog();
	bool result = ::PrJobDialog(fTHPrint);
	ZUtil_Mac_HL::sPostDialog();
	::PrClose();
	if (result)
		{
		THPrint theTHPrint = fTHPrint;
		::HandToHand((Handle*)&theTHPrint);
		if (theTHPrint == nil)
			throw bad_alloc();
		return new ZPrintJob_MacClassic(theTHPrint, iJobName);
		}
	return ZRef<ZPrintJob>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_MacClassic

ZPrintJob_MacClassic::ZPrintJob_MacClassic(THPrint iTHPrint, const string& iJobName)
:	fTHPrint(iTHPrint),
	fTPPrPort(nil),
	fPrinterResFile(-1),
	fPageDC(nil),
	fPaperDC(nil),
	fCurrentPageIndex(0),
	fJobName(iJobName),
	fDocOpen(false),
	fPageOpen(false)
	{
	// We have to reset these, otherwise the printer driver will discard
	// all pages up to but not including whatever was in iFstPage
	fTHPrint[0]->prJob.iFstPage = 1;
	fTHPrint[0]->prJob.iLstPage = 9999;
	}

ZPrintJob_MacClassic::~ZPrintJob_MacClassic()
	{
	try
		{ this->Finish(); }
	catch (...)
		{}

	if (fTHPrint)
		::DisposeHandle((Handle)fTHPrint);
	}

ZDC ZPrintJob_MacClassic::GetPageDC()
	{
	if (fPageDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		// Set up the origin comp so that (0,0) is in the top left corner of the *page*
		if (fPaperDC)
			fPageDC = new ZDC(fPaperDC->GetCanvas(), ZPoint::sZero);
		else
			fPageDC = new ZDC(new ZDCCanvas_QD_Print(&fTPPrPort->gPort), ZPoint::sZero);

		ZRect pageRect = fTHPrint[0]->prInfo.rPage;
		fPageDC->SetClip(pageRect);
		}

	return *fPageDC;
	}

ZDC ZPrintJob_MacClassic::GetPaperDC()
	{
	if (fPaperDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		// Set up the origin comp so that (0,0) is in the top left
		// corner of the paper, regardless of how margins are set up
		ZRect paperRect = fTHPrint[0]->rPaper;
		if (fPageDC)
			fPaperDC = new ZDC(fPageDC->GetCanvas(), -paperRect.TopLeft());
		else
			fPaperDC = new ZDC(new ZDCCanvas_QD_Print(&fTPPrPort->gPort), -paperRect.TopLeft());
		// Set up clipping to encompass the whole page area. We can't image it all, but
		// the clip carries some information to anyone using the paperDC. If a client
		// has kept a DC referencing our underlying PrintPortWrapper then the clipping
		// (and other settings) for that DC will *not* be affected
		fPaperDC->SetClip(ZRect(paperRect.Size()));
		}

	return *fPaperDC;
	}

void ZPrintJob_MacClassic::NewPage()
	{
	ZUtil_Mac_LL::PreserveResFile preserveResFile;
	ZUtil_Mac_LL::SaveRestoreResFile saveRestoreResFile(fPrinterResFile);
	try
		{
		if (!fPrinterOpen)
			{
			fPrinterOpen = true;
			::PrOpen();
			if (::PrError() != noErr)
				throw runtime_error("PrOpen Failed");
			fPrinterResFile = ::CurResFile();
			}

		// If we still have a page open, then close it (sending it to the printer or spooler)
		if (fPageOpen)
			{
			// Lose our printing DC's caches. If anyone has kept a reference to the
			// underlying PrintPortWrappers then that will still be valid, and the
			// settings in their DC will be used. Really though, users should acquire a
			// fresh reference to the DC after each page
			delete fPageDC;
			fPageDC = nil;

			delete fPaperDC;
			fPaperDC = nil;

			fPageOpen = false;
			::PrClosePage(fTPPrPort);
			if (::PrError() != noErr)
				throw runtime_error("PrClosePage Failed");

			// Check to see if we've hit iPFMaxPgs
			if (fCurrentPageIndex != 0 && ((fCurrentPageIndex % iPFMaxPgs) == 0))
				{
				// If so, then we have to close the document
				fDocOpen = false;
				::PrCloseDoc(fTPPrPort);
				if (::PrError() != noErr)
					throw runtime_error("PrCloseDoc Failed");
				// And trigger the spool, I guess this only happens when we're not doing background printing
				if (fTHPrint[0]->prJob.bJDocLoop == bSpoolLoop)
					{
					TPrStatus theTPrStatus;
					::PrPicFile(fTHPrint, nil, nil, nil, &theTPrStatus);
					}
				}
			}

		// Get the doc open, if needed
		if (!fDocOpen)
			{
			fDocOpen = true;
			// Create a window with fJobName as its title, placed physically offscreen so it's not
			// visible to the user. The printer driver pulls the title from the frontmost document
			// window in PrValidate() (ugly, no?)
			ZRect grayBounds((::GetGrayRgn())[0]->rgnBBox);
			Rect dummyWindowBounds = ZRect(-5,-5,-1,-1)+grayBounds.TopLeft();
			WindowRef dummyWindow = ::NewCWindow(nil, &dummyWindowBounds, ZString::sAsPString(fJobName), true, plainDBox, (WindowRef)-1, false,0);
			::PrValidate(fTHPrint);
			::DisposeWindow(dummyWindow);
			fTPPrPort = ::PrOpenDoc(fTHPrint, nil, nil);

			if (::PrError() != noErr)
				throw runtime_error("PrOpenDoc failed");
			}

		if (!fPageOpen)
			{
			fPageOpen = true;
			::PrOpenPage(fTPPrPort, nil);
			if (::PrError() != noErr)
				throw runtime_error("PrOpenPage Failed");
			}
		++fCurrentPageIndex;
		}
	catch (...)
		{
		this->Finish();
		throw;
		}
	}

void ZPrintJob_MacClassic::Finish()
	{
	delete fPageDC;
	fPageDC = nil;

	delete fPaperDC;
	fPaperDC = nil;

	ZUtil_Mac_LL::PreserveResFile preserveResFile;
	ZUtil_Mac_LL::SaveRestoreResFile saveRestoreResFile(fPrinterResFile);

	if (fPageOpen)
		{
		fPageOpen = false;
		::PrClosePage(fTPPrPort);
		}

	if (fDocOpen)
		{
		fDocOpen = false;
		::PrCloseDoc(fTPPrPort);
		// And trigger the spool, I guess this only happens when we're not doing background printing
		if (fTHPrint[0]->prJob.bJDocLoop == bSpoolLoop)
			{
			TPrStatus theTPrStatus;
			::PrPicFile(fTHPrint, nil, nil, nil, &theTPrStatus);
			}
		}

	fTPPrPort = nil;

	if (fPrinterOpen)
		{
		fPrinterOpen = false;
		::PrClose();
		fPrinterResFile = -1;
		}
	}

void ZPrintJob_MacClassic::Abort()
	{}

size_t ZPrintJob_MacClassic::GetCurrentPageIndex()
	{ return fCurrentPageIndex; }

ZPoint_T<double> ZPrintJob_MacClassic::GetPageSize()
	{ return ZRect_T<double>(fTHPrint[0]->prInfo.rPage).Size(); }

ZPoint_T<double> ZPrintJob_MacClassic::GetPageOrigin()
	{
	ZRect_T<double> paperRect(fTHPrint[0]->rPaper);
	ZRect_T<double> pageRect(fTHPrint[0]->prInfo.rPage);
	return pageRect.TopLeft()-paperRect.TopLeft();
	}

ZPoint_T<double> ZPrintJob_MacClassic::GetPaperSize()
	{ return ZRect_T<double>(fTHPrint[0]->rPaper).Size(); }

ZPoint_T<double> ZPrintJob_MacClassic::GetUnits()
	{ return ZPoint_T<double>(fTHPrint[0]->prInfo.iHRes, fTHPrint[0]->prInfo.iVRes);}

#endif // ZCONFIG(OS, MacOS7)
