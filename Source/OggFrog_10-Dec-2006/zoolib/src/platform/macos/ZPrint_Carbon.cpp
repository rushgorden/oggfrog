static const char rcsid[] = "@(#) $Id: ZPrint_Carbon.cpp,v 1.5 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZPrint_Carbon.h"

#if ZCONFIG(OS, Carbon)

#include "ZDC_QD.h"
#include "ZUtil_Mac_HL.h"
#include "ZUtil_Mac_LL.h"

#include <PMCore.h>

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_Carbon

ZPrintPageSetup_Carbon::ZPrintPageSetup_Carbon(const ZPrintPageSetup_Carbon& iOther)
	{
	ZAssertStop(2, iOther.fPageFormat);
	fPageFormat = iOther.fPageFormat;
	::HandToHand((Handle*)&fPageFormat);
	}

ZPrintPageSetup_Carbon::ZPrintPageSetup_Carbon()
	{
	fPrintSettings = kPMNoPrintSettings;
	fPageFormat = kPMNoPageFormat;
	OSStatus status = ::PMCreatePageFormat(&fPageFormat);
	if (fPageFormat == kPMNoPageFormat)
		throw bad_alloc();

	PMPrintSession printSession;
	::PMCreateSession(&printSession);
	status = ::PMCreatePageFormat(&fPageFormat);
	::PMRelease(&printSession);
	Boolean changed;
	::PMSessionValidatePageFormat(printSession, fPageFormat, &changed);
	}

ZPrintPageSetup_Carbon::~ZPrintPageSetup_Carbon()
	{
	if (fPageFormat)
		::DisposeHandle((Handle)fPageFormat);
	}

ZRef<ZPrintPageSetup> ZPrintPageSetup_Carbon::Clone()
	{ return new ZPrintPageSetup_Carbon(*this); }

bool ZPrintPageSetup_Carbon::DoPageSetupDialog()
	{
//	PMPrintSession printSession;
//	::PMCreateSession(&printSession);
	OSStatus status;
	Boolean accepted;

	// Set up a valid PageFormat object.
	if (fPageFormat == kPMNoPageFormat)
		{
		status = ::PMCreatePageFormat(&fPageFormat);

		// Note that PMPageFormat is not session-specific, but calling
		// PMSessionDefaultPageFormat assigns values specific to the printer
		// associated with the current printing session.
		if ((status == noErr) && (fPageFormat != kPMNoPageFormat))
			status = PMSessionDefaultPageFormat(fPrintSession, fPageFormat);
		}
	else
		status = PMSessionValidatePageFormat(fPrintSession, fPageFormat, kPMDontWantBoolean);

	ZUtil_Mac_HL::sPreDialog();
	// Display the Page Setup dialog.
	if (status == noErr)
		{
		status = ::PMSessionPageSetupDialog(fPrintSession, fPageFormat, &accepted);
		if (!accepted)
			status = kPMCancel; // user clicked Cancel button
		} 

	ZUtil_Mac_HL::sPostDialog();
	::PMRelease(&fPrintSession);
	return accepted;
	}

ZRef<ZPrintJobSetup> ZPrintPageSetup_Carbon::CreateJobSetup()
	{ return new ZPrintJobSetup_Carbon(fPrintSettings, fPageFormat); }

ZPoint_T<double> ZPrintPageSetup_Carbon::GetPageSize()
	{
	PMRect pageRect;
	::PMGetAdjustedPageRect(fPageFormat, &pageRect);
	return ZPoint_T<double>(pageRect.right - pageRect.left, pageRect.bottom - pageRect.top);
	}

ZPoint_T<double> ZPrintPageSetup_Carbon::GetPageOrigin()
	{
	PMRect paperRect;
	::PMGetAdjustedPaperRect(fPageFormat, &paperRect);

	PMRect pageRect;
	::PMGetAdjustedPageRect(fPageFormat, &pageRect);

	return ZPoint_T<double>(pageRect.left - paperRect.left, pageRect.top - paperRect.top);
	}

ZPoint_T<double> ZPrintPageSetup_Carbon::GetPaperSize()
	{
	PMRect paperRect;
	::PMGetAdjustedPaperRect(fPageFormat, &paperRect);
	return ZPoint_T<double>(paperRect.right - paperRect.left, paperRect.bottom - paperRect.top);
	}

ZPoint_T<double> ZPrintPageSetup_Carbon::GetUnits()
	{
	PMResolution res;
	::PMGetResolution(fPageFormat, &res);
	return ZPoint_T<double>(res.hRes, res.vRes);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_Carbon

ZPrintJobSetup_Carbon::ZPrintJobSetup_Carbon(const ZPrintJobSetup_Carbon& iOther)
	{
	ZAssertStop(2, iOther.fPrintSettings);
	fPrintSettings = iOther.fPrintSettings;
	::HandToHand((Handle*)&fPrintSettings);
	if (fPrintSettings == nil)
		throw bad_alloc();
	fOptions = iOther.fOptions;
	}

ZPrintJobSetup_Carbon::ZPrintJobSetup_Carbon(PMPrintSettings iPrintSettings, PMPageFormat iPageFormat)
:	fPrintSettings(iPrintSettings)
	{
	ZAssert(fPrintSettings);
	}

ZPrintJobSetup_Carbon::~ZPrintJobSetup_Carbon()
	{
//	if (fPrintSettings != kPMNoPrintSettings)
//		::PMRelease(&fPrintSession);
	}

ZRef<ZPrintJobSetup> ZPrintJobSetup_Carbon::Clone()
	{ return new ZPrintJobSetup_Carbon(*this); }

void ZPrintJobSetup_Carbon::SetOptions(const Options& iOptions)
	{
	fOptions = iOptions;
	::PMSetFirstPage(fPrintSettings, fOptions.fFirstPage, true);
	::PMSetLastPage(fPrintSettings, fOptions.fLastPage, true);
	}

ZPrintJobSetup::Options ZPrintJobSetup_Carbon::GetOptions()
	{
	Options theOptions = fOptions;
	UInt32 firstPage, lastPage;
	::PMGetFirstPage(fPrintSettings, &firstPage);
	::PMGetLastPage(fPrintSettings, &lastPage);
	long localFirstPage = min(firstPage, lastPage);
	long localLastPage = max(firstPage, lastPage);
	if (localFirstPage == 1 && localLastPage == 9999)
		theOptions.fPageRange = Options::ePageRangeAll;
	else
		theOptions.fPageRange = Options::ePageRangeExplicit;
	theOptions.fFirstPage = localFirstPage;
	theOptions.fLastPage = localLastPage;
	return theOptions;
	}

ZRef<ZPrintJob> ZPrintJobSetup_Carbon::DoPrintDialogAndStartJob(const string& iJobName)
	{
	PMPrintSession printSession;
	fPageFormat = kPMNoPageFormat;
	fPrintSettings = kPMNoPrintSettings;
	UInt32 minPage = 1, maxPage = 9999;

	OSStatus status = ::PMCreateSession(&printSession);
	if (status != kPMNoError)
		return ZRef<ZPrintJob>(); // no sense continuing. -ec 01.12.12

	if (fPrintSettings == kPMNoPrintSettings)
		{
		status = PMCreatePrintSettings(&fPrintSettings);

		// Note that PMPrintSettings is not session-specific, but calling
		// PMSessionDefaultPrintSettings assigns values specific to the printer
		// associated with the current printing session.
		if ((status == kPMNoError) && (fPrintSettings != kPMNoPrintSettings))
			status = ::PMSessionDefaultPrintSettings(fPrintSession, fPrintSettings);
		}
	else
		{
		status = PMSessionValidatePrintSettings(fPrintSession, fPrintSettings, kPMDontWantBoolean);
		}

	// Set a valid page range before displaying the Print dialog
	if (status == kPMNoError)
		status = PMSetPageRange(fPrintSettings, minPage, maxPage);

	ZUtil_Mac_HL::sPreDialog();

	// Display the Print dialog.
	Boolean accepted = false;
	if (status == noErr)
		{
		status = PMSessionPrintDialog(fPrintSession, fPrintSettings, fPageFormat, &accepted);
		if (!accepted)
			status = kPMCancel; // user clicked Cancel button
		}

	ZUtil_Mac_HL::sPostDialog();
	::PMRelease(&printSession);

	if (accepted)
		{
		//THPrint theTHPrint = fTHPrint;
		//::HandToHand((Handle*)&theTHPrint);
		//if (theTHPrint == nil)
		//	throw bad_alloc();
		return new ZPrintJob_Carbon(fPrintSettings, iJobName);
		}
	return ZRef<ZPrintJob>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_Carbon

ZPrintJob_Carbon::ZPrintJob_Carbon(PMPrintSettings iPrintSettings, const string& iJobName)
:	fPrintSettings(iPrintSettings),
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
	::PMSetFirstPage(fPrintSettings, 1L, true);
	::PMSetLastPage(fPrintSettings, 9999L, true);
	}

ZPrintJob_Carbon::~ZPrintJob_Carbon()
	{
	try
		{ this->Finish(); }
	catch (...)
		{}
	if (fPrintSettings != kPMNoPrintSettings)
		::PMRelease(&fPrintSettings);
	}

ZDC ZPrintJob_Carbon::GetPageDC()
	{
	if (fPageDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);

		// Set up the origin comp so that (0,0) is in the top left corner of the *page*
		if (fPaperDC)
			fPageDC = new ZDC(fPaperDC->GetCanvas(), ZPoint::sZero);
		else
			fPageDC = new ZDC(new ZDCCanvas_QD_Print((CGrafPtr)GetQDGlobalsThePort()), ZPoint::sZero);

		PMRect pageRect; 
		::PMGetAdjustedPageRect(fPageFormat, &pageRect);

		fPageDC->SetClip(ZRect(pageRect.left, pageRect.top, pageRect.right, pageRect.bottom));
		}

	return *fPageDC;
	}

ZDC ZPrintJob_Carbon::GetPaperDC()
	{
	if (fPaperDC == nil)
		{
		// We can't get the DC unless a page is open
		ZAssertStop(2, fPageOpen);

		// Set up the origin comp so that (0,0) is in the top left
		// corner of the paper, regardless of how margins are set up
		PMRect paperRect;
		::PMGetAdjustedPaperRect(fPageFormat, &paperRect);
		ZRect paperZRect(paperRect.left, paperRect.top, paperRect.right, paperRect.bottom);
		if (fPageDC)
			fPaperDC = new ZDC(fPageDC->GetCanvas(), -paperZRect.TopLeft());
		else
			fPaperDC = new ZDC(new ZDCCanvas_QD_Print((CGrafPtr)GetQDGlobalsThePort()), -paperZRect.TopLeft());

		// Set up clipping to encompass the whole page area. We can't image it all, but
		// the clip carries some information to anyone using the paperDC. If a client
		// has kept a DC referencing our underlying PrintPortWrapper then the clipping
		// (and other settings) for that DC will *not* be affected
		fPaperDC->SetClip(ZRect(paperZRect.Size()));
		}

	return *fPaperDC;
	}

void ZPrintJob_Carbon::NewPage()
	{
	ZUtil_Mac_LL::PreserveResFile preserveResFile;
	ZUtil_Mac_LL::SaveRestoreResFile saveRestoreResFile(fPrinterResFile);

	try
		{
		OSStatus status = kPMNoError;
		if (!fPrinterOpen)
			{
			fPrinterOpen = true;
		    PMPrintSession  printSession;
			status = ::PMCreateSession(&printSession);
			if (status != kPMNoError)
				throw runtime_error("PMCreateSession Failed");
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
			status = ::PMSessionEndPage(fPrintSession);
			if (status != kPMNoError)
				throw runtime_error("PMEndPage Failed");
			}

// Get the doc open, if needed
		if (!fDocOpen)
			{
			fDocOpen = true;
			Boolean result;
			::PMSessionValidatePrintSettings(fPrintSession, fPrintSettings, &result);
			::PMSessionValidatePageFormat(fPrintSession, fPageFormat, &result);
			status = ::PMSessionBeginDocument(fPrintSession, fPrintSettings, fPageFormat);

			if (status != kPMNoError)
				throw runtime_error("PMBeginDocument failed");
			}

		if (!fPageOpen)
			{
			fPageOpen = true;
			status = ::PMSessionBeginPage(fPrintSession, fPageFormat, nil);
			if (status != kPMNoError)
				throw runtime_error("PMSessionBeginPage Failed");
			}

		++fCurrentPageIndex;
		}
	catch (...)
		{
		this->Finish();
		throw;
		}
	}

void ZPrintJob_Carbon::Finish()
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
		::PMSessionEndPage(fPrintSession);
		}

	if (fDocOpen)
		{
		fDocOpen = false;
		::PMSessionEndDocument(fPrintSession);
		}

	if (fPrinterOpen)
		{
		fPrinterOpen = false;
		::PMRelease(fPrintSession);
		fPrinterResFile = -1;
		}
	}

void ZPrintJob_Carbon::Abort()
	{}

size_t ZPrintJob_Carbon::GetCurrentPageIndex()
	{ return fCurrentPageIndex; }

ZPoint_T<double> ZPrintJob_Carbon::GetPageSize()
	{
	PMRect pageRect;
	::PMGetAdjustedPageRect(fPageFormat, &pageRect);
	return ZPoint_T<double>(pageRect.right - pageRect.left, pageRect.bottom - pageRect.top);
	}

ZPoint_T<double> ZPrintJob_Carbon::GetPageOrigin()
	{
	PMRect paperRect;
	::PMGetAdjustedPaperRect(fPageFormat, &paperRect);

	PMRect pageRect;
	::PMGetAdjustedPageRect(fPageFormat, &pageRect);

	return ZPoint_T<double>(pageRect.left - paperRect.left, pageRect.top - paperRect.top);
	}

ZPoint_T<double> ZPrintJob_Carbon::GetPaperSize()
	{
	PMRect paperRect;
	::PMGetAdjustedPaperRect(fPageFormat, &paperRect);
	return ZPoint_T<double>(paperRect.right - paperRect.left, paperRect.bottom - paperRect.top);
	}

ZPoint_T<double> ZPrintJob_Carbon::GetUnits()
	{
	PMResolution res;
	::PMGetResolution(fPageFormat, &res);
	return ZPoint_T<double>(res.hRes, res.vRes);
	}

#endif // ZCONFIG(OS, Carbon)
