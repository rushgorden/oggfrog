/*  @(#) $Id: ZPrint_Carbon.h,v 1.4 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZPrint_Carbon__
#define __ZPrint_Carbon__ 1

#include "zconfig.h"

#if ZCONFIG(OS, Carbon)

#include "ZPrint.h"

#include <PMApplication.h>

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_Carbon

class ZPrintPageSetup_Carbon : public ZPrintPageSetup
	{
protected:
	ZPrintPageSetup_Carbon(const ZPrintPageSetup_Carbon& iOther);

public:
	ZPrintPageSetup_Carbon();
	virtual ~ZPrintPageSetup_Carbon();

// From ZPrintPageSetup
	virtual ZRef<ZPrintPageSetup> Clone();

	virtual bool DoPageSetupDialog();

	virtual ZRef<ZPrintJobSetup> CreateJobSetup();

	virtual ZPoint_T<double> GetPageSize();
	virtual ZPoint_T<double> GetPageOrigin();
	virtual ZPoint_T<double> GetPaperSize();
	virtual ZPoint_T<double> GetUnits();

// Our protocol
	PMPageFormat& GetPageFormat() { return fPageFormat; }

protected:
	PMPrintSession fPrintSession;
	PMPageFormat fPageFormat;
	PMPrintSettings fPrintSettings;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_Carbon

class ZPrintJobSetup_Carbon : public ZPrintJobSetup
	{
protected:
	ZPrintJobSetup_Carbon(const ZPrintJobSetup_Carbon& iOther);

public:
	ZPrintJobSetup_Carbon(PMPrintSettings iPrintSettings, PMPageFormat iPageFormat);
	virtual ~ZPrintJobSetup_Carbon();

// From ZPrintJobSetup
	ZRef<ZPrintJobSetup> Clone();

	virtual void SetOptions(const Options& iOptions);
	virtual Options GetOptions();

	virtual ZRef<ZPrintJob> DoPrintDialogAndStartJob(const string& iJobName);

// Our protocol
	PMPrintSettings& GetPMPrintSettings() { return fPrintSettings; }

protected:
	Options fOptions;
	PMPrintSession fPrintSession;
	PMPrintSettings fPrintSettings;
	PMPageFormat fPageFormat;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_Carbon

class ZPrintJob_Carbon : public ZPrintJob
	{
public:
	ZPrintJob_Carbon(PMPrintSettings iPrintSettings, const string& iJobName);
	virtual ~ZPrintJob_Carbon();

// From ZPrintJob
	virtual ZDC GetPageDC();
	virtual ZDC GetPaperDC();

	virtual void NewPage();
	virtual void Finish();
	virtual void Abort();

	virtual size_t GetCurrentPageIndex();

	virtual ZPoint_T<double> GetPageSize();
	virtual ZPoint_T<double> GetPageOrigin();
	virtual ZPoint_T<double> GetPaperSize();
	virtual ZPoint_T<double> GetUnits();

protected:
	short fPrinterResFile;
	PMPrintSession fPrintSession;
	PMPrintSettings fPrintSettings;
	PMPageFormat fPageFormat;
	ZDC* fPageDC;
	ZDC* fPaperDC;
	size_t fCurrentPageIndex;
	string fJobName;
	bool fPrinterOpen;
	bool fDocOpen;
	bool fPageOpen;
	};

#endif // ZCONFIG(OS, Carbon)

#endif // __ZPrint_Carbon__
