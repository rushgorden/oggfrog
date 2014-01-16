static const char rcsid[] = "@(#) $Id: ZPrint_Win.cpp,v 1.6 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZPrint_Win.h"

#if ZCONFIG(OS, Win32)

#include "ZDC_GDI.h"
#include "ZMemory.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_GDI_Print

class ZDCCanvas_GDI_Print : public ZDCCanvas_GDI_NonWindow
	{
public:
	ZDCCanvas_GDI_Print(HDC theHDC);
	virtual ~ZDCCanvas_GDI_Print();

	virtual bool IsPrinting();
	};

ZDCCanvas_GDI_Print::ZDCCanvas_GDI_Print(HDC theHDC)
	{
	fHDC = theHDC;
	}

ZDCCanvas_GDI_Print::~ZDCCanvas_GDI_Print()
	{
	fHDC = nil;
	}

bool ZDCCanvas_GDI_Print::IsPrinting()
	{ return true; }

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_WinW

ZPrintPageSetup_WinW::ZPrintPageSetup_WinW(const ZPrintPageSetup_WinW& iOther)
	{
	ZAssertStop(2, iOther.fPAGESETUPDLGW.lStructSize == sizeof(iOther.fPAGESETUPDLGW));
	fPAGESETUPDLGW = iOther.fPAGESETUPDLGW;
	}

ZPrintPageSetup_WinW::ZPrintPageSetup_WinW()
	{
	ZBlockZero(&fPAGESETUPDLGW, sizeof(fPAGESETUPDLGW));
	fPAGESETUPDLGW.lStructSize = sizeof(fPAGESETUPDLGW);
	fPAGESETUPDLGW.Flags |= PSD_DEFAULTMINMARGINS;
	fPAGESETUPDLGW.Flags |= PSD_RETURNDEFAULT;

// disable for now since this constructor is called in intialization and ::PageSetupDlg displays
// a dialog. Just use defaults for now. 27Sept1999, robin.
//	::PageSetupDlg(&fPAGESETUPDLG);
//	fPAGESETUPDLG.Flags &= ~PSD_RETURNDEFAULT;
	}

ZPrintPageSetup_WinW::~ZPrintPageSetup_WinW()
	{}

ZRef<ZPrintPageSetup> ZPrintPageSetup_WinW::Clone()
	{ return new ZPrintPageSetup_WinW(*this); }

bool ZPrintPageSetup_WinW::DoPageSetupDialog()
	{
	if (::PageSetupDlgW(&fPAGESETUPDLGW))
		return true;
	return false;
	}

ZRef<ZPrintJobSetup> ZPrintPageSetup_WinW::CreateJobSetup()
	{ return new ZPrintJobSetup_WinW; }

ZPoint_T<double> ZPrintPageSetup_WinW::GetPageSize()
	{
	ZRect_T<double> pageMargins(fPAGESETUPDLGW.rtMargin);
	ZPoint_T<double> paperSize(fPAGESETUPDLGW.ptPaperSize);
	paperSize -= pageMargins.TopLeft() + pageMargins.BottomRight();
	return paperSize;
	}

ZPoint_T<double> ZPrintPageSetup_WinW::GetPageOrigin()
	{ return ZRect_T<double>(fPAGESETUPDLGW.rtMargin).TopLeft(); }

ZPoint_T<double> ZPrintPageSetup_WinW::GetPaperSize()
	{ return ZPoint_T<double>(fPAGESETUPDLGW.ptPaperSize); }

ZPoint_T<double> ZPrintPageSetup_WinW::GetUnits()
	{
	if (fPAGESETUPDLGW.Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
		return ZPoint_T<double>(2540, 2540);

	// Make sure it really is in inches
	ZAssertStop(2, fPAGESETUPDLGW.Flags & PSD_INTHOUSANDTHSOFINCHES);
	return ZPoint_T<double>(1000,1000);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_WinW

ZPrintJobSetup_WinW::ZPrintJobSetup_WinW(const ZPrintJobSetup_WinW& iOther)
	{
	ZAssertStop(2, iOther.fPRINTDLGW.lStructSize == sizeof(iOther.fPRINTDLGW));
	fPRINTDLGW = iOther.fPRINTDLGW;
	}

ZPrintJobSetup_WinW::ZPrintJobSetup_WinW()
	{
	ZBlockZero(&fPRINTDLGW, sizeof(fPRINTDLGW));
	fPRINTDLGW.lStructSize = sizeof(fPRINTDLGW);
	fPRINTDLGW.Flags |= PD_RETURNDEFAULT;
	::PrintDlgW(&fPRINTDLGW);
	fPRINTDLGW.Flags &= ~PD_RETURNDEFAULT;

	// Setup with 1 copy by default
	fPRINTDLGW.nCopies = 1;
	}

ZPrintJobSetup_WinW::~ZPrintJobSetup_WinW()
	{}

ZRef<ZPrintJobSetup> ZPrintJobSetup_WinW::Clone()
	{ return new ZPrintJobSetup_WinW(*this); }

void ZPrintJobSetup_WinW::SetOptions(const Options& iOptions)
	{
// Setup whether we know what the range is to start off
	if (iOptions.fAvailableKnown == Options::availableKnown)
		{
		ZAssert(iOptions.fMinPage <= iOptions.fMaxPage);
		fPRINTDLGW.nMinPage = iOptions.fMinPage;
		fPRINTDLGW.nMaxPage = iOptions.fMaxPage;
		}
	else
		{
		fPRINTDLGW.nMinPage = 0;
		fPRINTDLGW.nMaxPage = 0;
		}

	// Sept271999 rvb: don't set from and to page #'s since we dont' know last page, it looks funny
	// to have last page 999. It is better to leave it blank.
	fPRINTDLGW.nFromPage = fPRINTDLGW.nMinPage;
	fPRINTDLGW.nToPage = fPRINTDLGW.nMaxPage;

	// Clear out the selection/pagenums flags
	fPRINTDLGW.Flags &= ~(PD_SELECTION | PD_PAGENUMS);
	if (iOptions.fPageRange == Options::ePageRangeSelection)
		{
		fPRINTDLGW.Flags |= PD_SELECTION;
		}
	else if (iOptions.fPageRange == Options::ePageRangeExplicit)
		{
		fPRINTDLGW.Flags |= PD_PAGENUMS;
		fPRINTDLGW.nFromPage = iOptions.fFirstPage;
		fPRINTDLGW.nToPage = iOptions.fLastPage;
		}
	else
		{
		//to make allpages, unset selected and pagerange
		fPRINTDLGW.Flags &= ~(PD_SELECTION| PD_PAGENUMS);
		ZAssert(iOptions.fPageRange == Options::ePageRangeAll);
		}

	if (iOptions.fCollate)
		fPRINTDLGW.Flags |= PD_COLLATE;
	else
		fPRINTDLGW.Flags &= ~PD_COLLATE;

	ZAssertStop(2, iOptions.fNumberOfCopies >= 1);

	fPRINTDLGW.nCopies = iOptions.fNumberOfCopies;
	}

ZPrintJobSetup::Options ZPrintJobSetup_WinW::GetOptions()
	{
	Options theOptions;
	if (fPRINTDLGW.nMinPage == 0 && fPRINTDLGW.nMaxPage == 0)
		{
		theOptions.fAvailableKnown = Options::availableUnknown;
		theOptions.fMinPage = 0;
		theOptions.fMaxPage = 0;
		}
	else
		{
		theOptions.fAvailableKnown = Options::availableKnown;
		theOptions.fMinPage = fPRINTDLGW.nMinPage;
		theOptions.fMaxPage = fPRINTDLGW.nMaxPage;
		}

	theOptions.fFirstPage = theOptions.fMinPage;
	theOptions.fLastPage = theOptions.fMaxPage;
	switch (fPRINTDLGW.Flags & (PD_SELECTION | PD_PAGENUMS))
		{
		case PD_ALLPAGES:
			{
			theOptions.fPageRange = Options::ePageRangeAll;
			}
			break;
		case PD_SELECTION:
			{
			theOptions.fPageRange = Options::ePageRangeSelection;
			}
			break;
		case PD_PAGENUMS:
			{
			theOptions.fPageRange = Options::ePageRangeExplicit;
			theOptions.fFirstPage = fPRINTDLGW.nFromPage;
			theOptions.fLastPage = fPRINTDLGW.nToPage;
			break;
			}
		default:
			ZUnimplemented();
		}
	if (fPRINTDLGW.Flags & PD_COLLATE)
		theOptions.fCollate = true;
	else
		theOptions.fCollate = false;

	theOptions.fNumberOfCopies = fPRINTDLGW.nCopies;
	return theOptions;
	}

ZRef<ZPrintJob> ZPrintJobSetup_WinW::DoPrintDialogAndStartJob(const string& iJobName)
	{
	fPRINTDLGW.Flags |= PD_RETURNDC;
	bool result = ::PrintDlgW(&fPRINTDLGW);
	fPRINTDLGW.Flags &= ~PD_RETURNDC;
	HDC theHDC = fPRINTDLGW.hDC;
	fPRINTDLGW.hDC = nil;
	if (theHDC)
		{
		if (result)
			return new ZPrintJob_WinW(theHDC, ZUnicode::sAsUTF16(iJobName));
		::DeleteDC(theHDC);
		}
	return ZRef<ZPrintJob>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_WinW

ZPrintJob_WinW::ZPrintJob_WinW(HDC iHDC, const string16& iJobName)
:	fHDC(iHDC),
	fJobName(iJobName),
	fPageDC(nil),
	fPaperDC(nil),
	fCurrentPageIndex(0),
	fDocOpen(false),
	fPageOpen(false)
	{}

ZPrintJob_WinW::~ZPrintJob_WinW()
	{
	try
		{ this->Finish(); }
	catch (...)
		{}

	if (fHDC)
		::DeleteDC(fHDC);
	}

// From ZPrintJob
ZDC ZPrintJob_WinW::GetPageDC()
	{
	if (fPageDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		if (fPaperDC)
			fPageDC = new ZDC(fPaperDC->GetCanvas(), ZPoint::sZero);
		else
			fPageDC = new ZDC(new ZDCCanvas_GDI_Print(fHDC), ZPoint::sZero);
		double pageWidth = ::GetDeviceCaps(fHDC, HORZRES);
		double pageHeight = ::GetDeviceCaps(fHDC, VERTRES);
		double hPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
		double vPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);

		// rvb 8Oct99, the size of the page on windows is too big, this is a hack to fix it,
		// picked 6 to subtract from the length since it made the footer now appear properly
		ZPoint_T<double> pixelSize(pageWidth/hPixelRes*72, pageHeight/vPixelRes*72- 6);
		fPageDC->SetClip(ZRect(pixelSize));
		}
	return *fPageDC;
	}

ZDC ZPrintJob_WinW::GetPaperDC()
	{
	if (fPaperDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		double physicalOffsetX = ::GetDeviceCaps(fHDC, PHYSICALOFFSETX);
		double physicalOffsetY = ::GetDeviceCaps(fHDC, PHYSICALOFFSETY);
		double hPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
		double vPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);
		ZPoint originComp(physicalOffsetX/hPixelRes*72, physicalOffsetY/vPixelRes*72);
		if (fPageDC)
			fPaperDC = new ZDC(fPageDC->GetCanvas(), originComp);
		else
			fPaperDC = new ZDC(new ZDCCanvas_GDI_Print(fHDC), originComp);

		double physicalWidth = ::GetDeviceCaps(fHDC, PHYSICALWIDTH);
		double physicalHeight = ::GetDeviceCaps(fHDC, PHYSICALHEIGHT);

		ZPoint physicalSize(physicalWidth/hPixelRes*72, physicalHeight/vPixelRes*72);
		fPaperDC->SetClip(ZRect(physicalSize));
		}
	return *fPaperDC;
	}

void ZPrintJob_WinW::NewPage()
	{
	try
		{
		// If we still have a page open, then close it (sending it to the printer or spooler)
		if (fPageOpen)
			{
			// Lose our printing DC's caches. If anyone has kept a reference to the
			// underlying ZDCCanvas_GDI_Print then that will still be valid, and the
			// settings in their DC will be used. Really though, users should acquire a
			// fresh reference to the DC after each page
			delete fPageDC;
			fPageDC = nil;

			delete fPaperDC;
			fPaperDC = nil;

			fPageOpen = false;
			if (::EndPage(fHDC) <= 0)
				throw runtime_error("EndPage failed");
			}

		// Get the doc open, if needed
		if (!fDocOpen)
			{
			fDocOpen = true;
			ZBlockSet(&fDocInfoW, sizeof(fDocInfoW), 0);
			fDocInfoW.cbSize = sizeof(fDocInfoW);
			fDocInfoW.lpszDocName = fJobName.c_str();
			if (::StartDocW(fHDC, &fDocInfoW) <= 0)
				throw runtime_error("StartDoc failed");
			}

		if (!fPageOpen)
			{
			fPageOpen = true;
			if (::StartPage(fHDC) <= 0)
				throw runtime_error("StartPage failed");
			}
		++fCurrentPageIndex;
		}
	catch (...)
		{
		this->Finish();
		throw;
		}
	}

void ZPrintJob_WinW::Finish()
	{
	delete fPageDC;
	fPageDC = nil;

	delete fPaperDC;
	fPaperDC = nil;

	if (fPageOpen)
		{
		fPageOpen = false;
		if (::EndPage(fHDC) <= 0)
			throw runtime_error("EndPage failed");
		}

	if (fDocOpen)
		{
		::EndDoc(fHDC);
		fDocOpen = false;
		}
	}

void ZPrintJob_WinW::Abort()
	{}

size_t ZPrintJob_WinW::GetCurrentPageIndex()
	{ return fCurrentPageIndex; }

ZPoint_T<double> ZPrintJob_WinW::GetPageSize()
	{
	int pageWidth = ::GetDeviceCaps(fHDC, HORZRES);
	int pageHeight = ::GetDeviceCaps(fHDC, VERTRES);
	return ZPoint_T<double>(pageWidth, pageHeight);
	}

ZPoint_T<double> ZPrintJob_WinW::GetPageOrigin()
	{
	int physicalOffsetX = ::GetDeviceCaps(fHDC, PHYSICALOFFSETX);
	int physicalOffsetY = ::GetDeviceCaps(fHDC, PHYSICALOFFSETY);
	return ZPoint_T<double>(physicalOffsetX, physicalOffsetY);
	}

ZPoint_T<double> ZPrintJob_WinW::GetPaperSize()
	{
	// We can't get the paper size from the HDC
	int paperWidth = ::GetDeviceCaps(fHDC, PHYSICALWIDTH);
	int paperHeight = ::GetDeviceCaps(fHDC, PHYSICALHEIGHT);
	return ZPoint_T<double>(paperWidth, paperHeight);
	}

ZPoint_T<double> ZPrintJob_WinW::GetUnits()
	{
	int hPixels = ::GetDeviceCaps(fHDC, LOGPIXELSX);
	int vPixels = ::GetDeviceCaps(fHDC, LOGPIXELSY);
	return ZPoint_T<double>(hPixels, vPixels);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_WinA

ZPrintPageSetup_WinA::ZPrintPageSetup_WinA(const ZPrintPageSetup_WinA& iOther)
	{
	ZAssertStop(2, iOther.fPAGESETUPDLGA.lStructSize == sizeof(iOther.fPAGESETUPDLGA));
	fPAGESETUPDLGA = iOther.fPAGESETUPDLGA;
	}

ZPrintPageSetup_WinA::ZPrintPageSetup_WinA()
	{
	ZBlockZero(&fPAGESETUPDLGA, sizeof(fPAGESETUPDLGA));
	fPAGESETUPDLGA.lStructSize = sizeof(fPAGESETUPDLGA);
	fPAGESETUPDLGA.Flags |= PSD_DEFAULTMINMARGINS;
	fPAGESETUPDLGA.Flags |= PSD_RETURNDEFAULT;

// disable for now since this constructor is called in intialization and ::PageSetupDlg displays
// a dialog. Just use defaults for now. 27Sept1999, robin.
//	::PageSetupDlg(&fPAGESETUPDLG);
//	fPAGESETUPDLG.Flags &= ~PSD_RETURNDEFAULT;
	}

ZPrintPageSetup_WinA::~ZPrintPageSetup_WinA()
	{}

ZRef<ZPrintPageSetup> ZPrintPageSetup_WinA::Clone()
	{ return new ZPrintPageSetup_WinA(*this); }

bool ZPrintPageSetup_WinA::DoPageSetupDialog()
	{
	if (::PageSetupDlgA(&fPAGESETUPDLGA))
		return true;
	return false;
	}

ZRef<ZPrintJobSetup> ZPrintPageSetup_WinA::CreateJobSetup()
	{ return new ZPrintJobSetup_WinA; }

ZPoint_T<double> ZPrintPageSetup_WinA::GetPageSize()
	{
	ZRect_T<double> pageMargins(fPAGESETUPDLGA.rtMargin);
	ZPoint_T<double> paperSize(fPAGESETUPDLGA.ptPaperSize);
	paperSize -= pageMargins.TopLeft() + pageMargins.BottomRight();
	return paperSize;
	}

ZPoint_T<double> ZPrintPageSetup_WinA::GetPageOrigin()
	{ return ZRect_T<double>(fPAGESETUPDLGA.rtMargin).TopLeft(); }

ZPoint_T<double> ZPrintPageSetup_WinA::GetPaperSize()
	{ return ZPoint_T<double>(fPAGESETUPDLGA.ptPaperSize); }

ZPoint_T<double> ZPrintPageSetup_WinA::GetUnits()
	{
	if (fPAGESETUPDLGA.Flags & PSD_INHUNDREDTHSOFMILLIMETERS)
		return ZPoint_T<double>(2540, 2540);

	// Make sure it really is in inches
	ZAssertStop(2, fPAGESETUPDLGA.Flags & PSD_INTHOUSANDTHSOFINCHES);
	return ZPoint_T<double>(1000,1000);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_WinA

ZPrintJobSetup_WinA::ZPrintJobSetup_WinA(const ZPrintJobSetup_WinA& iOther)
	{
	ZAssertStop(2, iOther.fPRINTDLGA.lStructSize == sizeof(iOther.fPRINTDLGA));
	fPRINTDLGA = iOther.fPRINTDLGA;
	}

ZPrintJobSetup_WinA::ZPrintJobSetup_WinA()
	{
	ZBlockZero(&fPRINTDLGA, sizeof(fPRINTDLGA));
	fPRINTDLGA.lStructSize = sizeof(fPRINTDLGA);
	fPRINTDLGA.Flags |= PD_RETURNDEFAULT;
	::PrintDlgA(&fPRINTDLGA);
	fPRINTDLGA.Flags &= ~PD_RETURNDEFAULT;

	// Setup with 1 copy by default
	fPRINTDLGA.nCopies = 1;
	}

ZPrintJobSetup_WinA::~ZPrintJobSetup_WinA()
	{}

ZRef<ZPrintJobSetup> ZPrintJobSetup_WinA::Clone()
	{ return new ZPrintJobSetup_WinA(*this); }

void ZPrintJobSetup_WinA::SetOptions(const Options& iOptions)
	{
// Setup whether we know what the range is to start off
	if (iOptions.fAvailableKnown == Options::availableKnown)
		{
		ZAssert(iOptions.fMinPage <= iOptions.fMaxPage);
		fPRINTDLGA.nMinPage = iOptions.fMinPage;
		fPRINTDLGA.nMaxPage = iOptions.fMaxPage;
		}
	else
		{
		fPRINTDLGA.nMinPage = 0;
		fPRINTDLGA.nMaxPage = 0;
		}

	// Sept271999 rvb: don't set from and to page #'s since we dont' know last page, it looks funny
	// to have last page 999. It is better to leave it blank.
	fPRINTDLGA.nFromPage = fPRINTDLGA.nMinPage;
	fPRINTDLGA.nToPage = fPRINTDLGA.nMaxPage;

	// Clear out the selection/pagenums flags
	fPRINTDLGA.Flags &= ~(PD_SELECTION | PD_PAGENUMS);
	if (iOptions.fPageRange == Options::ePageRangeSelection)
		{
		fPRINTDLGA.Flags |= PD_SELECTION;
		}
	else if (iOptions.fPageRange == Options::ePageRangeExplicit)
		{
		fPRINTDLGA.Flags |= PD_PAGENUMS;
		fPRINTDLGA.nFromPage = iOptions.fFirstPage;
		fPRINTDLGA.nToPage = iOptions.fLastPage;
		}
	else
		{
		//to make allpages, unset selected and pagerange
		fPRINTDLGA.Flags &= ~(PD_SELECTION| PD_PAGENUMS);
		ZAssert(iOptions.fPageRange == Options::ePageRangeAll);
		}

	if (iOptions.fCollate)
		fPRINTDLGA.Flags |= PD_COLLATE;
	else
		fPRINTDLGA.Flags &= ~PD_COLLATE;

	ZAssertStop(2, iOptions.fNumberOfCopies >= 1);

	fPRINTDLGA.nCopies = iOptions.fNumberOfCopies;
	}

ZPrintJobSetup::Options ZPrintJobSetup_WinA::GetOptions()
	{
	Options theOptions;
	if (fPRINTDLGA.nMinPage == 0 && fPRINTDLGA.nMaxPage == 0)
		{
		theOptions.fAvailableKnown = Options::availableUnknown;
		theOptions.fMinPage = 0;
		theOptions.fMaxPage = 0;
		}
	else
		{
		theOptions.fAvailableKnown = Options::availableKnown;
		theOptions.fMinPage = fPRINTDLGA.nMinPage;
		theOptions.fMaxPage = fPRINTDLGA.nMaxPage;
		}

	theOptions.fFirstPage = theOptions.fMinPage;
	theOptions.fLastPage = theOptions.fMaxPage;
	switch (fPRINTDLGA.Flags & (PD_SELECTION | PD_PAGENUMS))
		{
		case PD_ALLPAGES:
			{
			theOptions.fPageRange = Options::ePageRangeAll;
			}
			break;
		case PD_SELECTION:
			{
			theOptions.fPageRange = Options::ePageRangeSelection;
			}
			break;
		case PD_PAGENUMS:
			{
			theOptions.fPageRange = Options::ePageRangeExplicit;
			theOptions.fFirstPage = fPRINTDLGA.nFromPage;
			theOptions.fLastPage = fPRINTDLGA.nToPage;
			break;
			}
		default:
			ZUnimplemented();
		}
	if (fPRINTDLGA.Flags & PD_COLLATE)
		theOptions.fCollate = true;
	else
		theOptions.fCollate = false;

	theOptions.fNumberOfCopies = fPRINTDLGA.nCopies;
	return theOptions;
	}

ZRef<ZPrintJob> ZPrintJobSetup_WinA::DoPrintDialogAndStartJob(const string& iJobName)
	{
	fPRINTDLGA.Flags |= PD_RETURNDC;
	bool result = ::PrintDlgA(&fPRINTDLGA);
	fPRINTDLGA.Flags &= ~PD_RETURNDC;
	HDC theHDC = fPRINTDLGA.hDC;
	fPRINTDLGA.hDC = nil;
	if (theHDC)
		{
		if (result)
			return new ZPrintJob_WinA(theHDC, iJobName);
		::DeleteDC(theHDC);
		}
	return ZRef<ZPrintJob>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_WinA

ZPrintJob_WinA::ZPrintJob_WinA(HDC iHDC, const string& iJobName)
:	fHDC(iHDC),
	fJobName(iJobName),
	fPageDC(nil),
	fPaperDC(nil),
	fCurrentPageIndex(0),
	fDocOpen(false),
	fPageOpen(false)
	{}

ZPrintJob_WinA::~ZPrintJob_WinA()
	{
	try
		{ this->Finish(); }
	catch (...)
		{}

	if (fHDC)
		::DeleteDC(fHDC);
	}

// From ZPrintJob
ZDC ZPrintJob_WinA::GetPageDC()
	{
	if (fPageDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		if (fPaperDC)
			fPageDC = new ZDC(fPaperDC->GetCanvas(), ZPoint::sZero);
		else
			fPageDC = new ZDC(new ZDCCanvas_GDI_Print(fHDC), ZPoint::sZero);
		double pageWidth = ::GetDeviceCaps(fHDC, HORZRES);
		double pageHeight = ::GetDeviceCaps(fHDC, VERTRES);
		double hPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
		double vPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);

		// rvb 8Oct99, the size of the page on windows is too big, this is a hack to fix it,
		// picked 6 to subtract from the length since it made the footer now appear properly
		ZPoint_T<double> pixelSize(pageWidth/hPixelRes*72, pageHeight/vPixelRes*72- 6);
		fPageDC->SetClip(ZRect(pixelSize));
		}
	return *fPageDC;
	}

ZDC ZPrintJob_WinA::GetPaperDC()
	{
	if (fPaperDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);
		double physicalOffsetX = ::GetDeviceCaps(fHDC, PHYSICALOFFSETX);
		double physicalOffsetY = ::GetDeviceCaps(fHDC, PHYSICALOFFSETY);
		double hPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSX);
		double vPixelRes = ::GetDeviceCaps(fHDC, LOGPIXELSY);
		ZPoint originComp(physicalOffsetX/hPixelRes*72, physicalOffsetY/vPixelRes*72);
		if (fPageDC)
			fPaperDC = new ZDC(fPageDC->GetCanvas(), originComp);
		else
			fPaperDC = new ZDC(new ZDCCanvas_GDI_Print(fHDC), originComp);

		double physicalWidth = ::GetDeviceCaps(fHDC, PHYSICALWIDTH);
		double physicalHeight = ::GetDeviceCaps(fHDC, PHYSICALHEIGHT);

		ZPoint physicalSize(physicalWidth/hPixelRes*72, physicalHeight/vPixelRes*72);
		fPaperDC->SetClip(ZRect(physicalSize));
		}
	return *fPaperDC;
	}

void ZPrintJob_WinA::NewPage()
	{
	try
		{
		// If we still have a page open, then close it (sending it to the printer or spooler)
		if (fPageOpen)
			{
			// Lose our printing DC's caches. If anyone has kept a reference to the
			// underlying ZDCCanvas_GDI_Print then that will still be valid, and the
			// settings in their DC will be used. Really though, users should acquire a
			// fresh reference to the DC after each page
			delete fPageDC;
			fPageDC = nil;

			delete fPaperDC;
			fPaperDC = nil;

			fPageOpen = false;
			if (::EndPage(fHDC) <= 0)
				throw runtime_error("EndPage failed");
			}

		// Get the doc open, if needed
		if (!fDocOpen)
			{
			fDocOpen = true;
			ZBlockSet(&fDocInfoA, sizeof(fDocInfoA), 0);
			fDocInfoA.cbSize = sizeof(fDocInfoA);
			fDocInfoA.lpszDocName = fJobName.c_str();
			if (::StartDocA(fHDC, &fDocInfoA) <= 0)
				throw runtime_error("StartDoc failed");
			}

		if (!fPageOpen)
			{
			fPageOpen = true;
			if (::StartPage(fHDC) <= 0)
				throw runtime_error("StartPage failed");
			}
		++fCurrentPageIndex;
		}
	catch (...)
		{
		this->Finish();
		throw;
		}
	}

void ZPrintJob_WinA::Finish()
	{
	delete fPageDC;
	fPageDC = nil;

	delete fPaperDC;
	fPaperDC = nil;

	if (fPageOpen)
		{
		fPageOpen = false;
		if (::EndPage(fHDC) <= 0)
			throw runtime_error("EndPage failed");
		}

	if (fDocOpen)
		{
		::EndDoc(fHDC);
		fDocOpen = false;
		}
	}

void ZPrintJob_WinA::Abort()
	{}

size_t ZPrintJob_WinA::GetCurrentPageIndex()
	{ return fCurrentPageIndex; }

ZPoint_T<double> ZPrintJob_WinA::GetPageSize()
	{
	int pageWidth = ::GetDeviceCaps(fHDC, HORZRES);
	int pageHeight = ::GetDeviceCaps(fHDC, VERTRES);
	return ZPoint_T<double>(pageWidth, pageHeight);
	}

ZPoint_T<double> ZPrintJob_WinA::GetPageOrigin()
	{
	int physicalOffsetX = ::GetDeviceCaps(fHDC, PHYSICALOFFSETX);
	int physicalOffsetY = ::GetDeviceCaps(fHDC, PHYSICALOFFSETY);
	return ZPoint_T<double>(physicalOffsetX, physicalOffsetY);
	}

ZPoint_T<double> ZPrintJob_WinA::GetPaperSize()
	{
	// We can't get the paper size from the HDC
	int paperWidth = ::GetDeviceCaps(fHDC, PHYSICALWIDTH);
	int paperHeight = ::GetDeviceCaps(fHDC, PHYSICALHEIGHT);
	return ZPoint_T<double>(paperWidth, paperHeight);
	}

ZPoint_T<double> ZPrintJob_WinA::GetUnits()
	{
	int hPixels = ::GetDeviceCaps(fHDC, LOGPIXELSX);
	int vPixels = ::GetDeviceCaps(fHDC, LOGPIXELSY);
	return ZPoint_T<double>(hPixels, vPixels);
	}

#endif // ZCONFIG(OS, Win32)
