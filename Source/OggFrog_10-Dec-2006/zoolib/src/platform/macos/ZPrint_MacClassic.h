/*  @(#) $Id: ZPrint_MacClassic.h,v 1.4 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZPrint_MacClassic__
#define __ZPrint_MacClassic__ 1
#include "zconfig.h"

#if ZCONFIG(OS, MacOS7)

#include "ZPrint.h"

#include <Printing.h>

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_MacClassic

class ZPrintPageSetup_MacClassic : public ZPrintPageSetup
	{
protected:
	ZPrintPageSetup_MacClassic(const ZPrintPageSetup_MacClassic& iOther);

public:
	ZPrintPageSetup_MacClassic();
	virtual ~ZPrintPageSetup_MacClassic();

// From ZPrintPageSetup
	virtual ZRef<ZPrintPageSetup> Clone();

	virtual bool DoPageSetupDialog();

	virtual ZRef<ZPrintJobSetup> CreateJobSetup();

	virtual ZPoint_T<double> GetPageSize();
	virtual ZPoint_T<double> GetPageOrigin();
	virtual ZPoint_T<double> GetPaperSize();
	virtual ZPoint_T<double> GetUnits();

// Our protocol
	THPrint& GetTHPrint() { return fTHPrint; }

protected:
	THPrint fTHPrint;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_MacClassic

class ZPrintJobSetup_MacClassic : public ZPrintJobSetup
	{
protected:
	ZPrintJobSetup_MacClassic(const ZPrintJobSetup_MacClassic& iOther);

public:
	ZPrintJobSetup_MacClassic(THPrint iTHPrint);
	virtual ~ZPrintJobSetup_MacClassic();

// From ZPrintJobSetup
	ZRef<ZPrintJobSetup> Clone();

	virtual void SetOptions(const Options& iOptions);
	virtual Options GetOptions();

	virtual ZRef<ZPrintJob> DoPrintDialogAndStartJob(const string& iJobName);

// Our protocol
	THPrint& GetTHPrint() { return fTHPrint; }

protected:
	Options fOptions;
	THPrint fTHPrint;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_MacClassic

class ZPrintJob_MacClassic : public ZPrintJob
	{
public:
	ZPrintJob_MacClassic(THPrint iTHPrint, const string& iJobName);
	virtual ~ZPrintJob_MacClassic();

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
	TPPrPort fTPPrPort;
	THPrint fTHPrint;
	ZDC* fPageDC;
	ZDC* fPaperDC;
	size_t fCurrentPageIndex;
	string fJobName;
	bool fPrinterOpen;
	bool fDocOpen;
	bool fPageOpen;
	};

#endif // ZCONFIG(OS, MacOS7)

#endif // __ZPrint_MacClassic__
