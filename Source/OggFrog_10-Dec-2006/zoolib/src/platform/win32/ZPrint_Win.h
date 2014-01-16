/*  @(#) $Id: ZPrint_Win.h,v 1.5 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZPrint_Win__
#define __ZPrint_Win__ 1

#include "zconfig.h"

#if ZCONFIG(OS, Win32)

#include "ZPrint.h"
#include "ZUnicode.h"

#include "ZWinHeader.h"
#include <commdlg.h>

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_WinW

class ZPrintPageSetup_WinW : public ZPrintPageSetup
	{
private:
	ZPrintPageSetup_WinW(const ZPrintPageSetup_WinW& iOther);

public:
	ZPrintPageSetup_WinW();
	virtual ~ZPrintPageSetup_WinW();

// From ZPrintPageSetup
	virtual ZRef<ZPrintPageSetup> Clone();

	virtual bool DoPageSetupDialog();

	virtual ZRef<ZPrintJobSetup> CreateJobSetup();

	virtual ZPoint_T<double> GetPageSize();
	virtual ZPoint_T<double> GetPageOrigin();
	virtual ZPoint_T<double> GetPaperSize();
	virtual ZPoint_T<double> GetUnits();

// Our protocol
	PAGESETUPDLGW& GetPAGESETUPDLGW() { return fPAGESETUPDLGW; }

protected:
	PAGESETUPDLGW fPAGESETUPDLGW;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_WinW

class ZPrintJobSetup_WinW : public ZPrintJobSetup
	{
private:
	ZPrintJobSetup_WinW(const ZPrintJobSetup_WinW& iOther);

public:
	ZPrintJobSetup_WinW();
	virtual ~ZPrintJobSetup_WinW();

// From ZPrintJobSetup
	ZRef<ZPrintJobSetup> Clone();

	virtual void SetOptions(const Options& iOptions);
	virtual Options GetOptions();

	virtual ZRef<ZPrintJob> DoPrintDialogAndStartJob(const string& iJobName);

// Our protocol
	PRINTDLGW& GetPRINTDLGW() { return fPRINTDLGW; }

protected:
	PRINTDLGW fPRINTDLGW;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_WinW

class ZPrintJob_WinW : public ZPrintJob
	{
public:
	ZPrintJob_WinW(HDC iHDC, const string16& iJobName);
	virtual ~ZPrintJob_WinW();

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
	HDC fHDC;
	string16 fJobName;

	ZDC* fPageDC;
	ZDC* fPaperDC;

	DOCINFOW fDocInfoW;

	size_t fCurrentPageIndex;

	bool fDocOpen;
	bool fPageOpen;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup_WinA

class ZPrintPageSetup_WinA : public ZPrintPageSetup
	{
private:
	ZPrintPageSetup_WinA(const ZPrintPageSetup_WinA& iOther);

public:
	ZPrintPageSetup_WinA();
	virtual ~ZPrintPageSetup_WinA();

// From ZPrintPageSetup
	virtual ZRef<ZPrintPageSetup> Clone();

	virtual bool DoPageSetupDialog();

	virtual ZRef<ZPrintJobSetup> CreateJobSetup();

	virtual ZPoint_T<double> GetPageSize();
	virtual ZPoint_T<double> GetPageOrigin();
	virtual ZPoint_T<double> GetPaperSize();
	virtual ZPoint_T<double> GetUnits();

// Our protocol
	PAGESETUPDLGA& GetPAGESETUPDLGA() { return fPAGESETUPDLGA; }

protected:
	PAGESETUPDLGA fPAGESETUPDLGA;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup_WinA

class ZPrintJobSetup_WinA : public ZPrintJobSetup
	{
private:
	ZPrintJobSetup_WinA(const ZPrintJobSetup_WinA& iOther);

public:
	ZPrintJobSetup_WinA();
	virtual ~ZPrintJobSetup_WinA();

// From ZPrintJobSetup
	ZRef<ZPrintJobSetup> Clone();

	virtual void SetOptions(const Options& iOptions);
	virtual Options GetOptions();

	virtual ZRef<ZPrintJob> DoPrintDialogAndStartJob(const string& iJobName);

// Our protocol
	PRINTDLGA& GetPRINTDLGA() { return fPRINTDLGA; }

protected:
	PRINTDLGA fPRINTDLGA;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob_Win

class ZPrintJob_WinA : public ZPrintJob
	{
public:
	ZPrintJob_WinA(HDC iHDC, const string& iJobName);
	virtual ~ZPrintJob_WinA();

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
	HDC fHDC;
	string fJobName;

	ZDC* fPageDC;
	ZDC* fPaperDC;

	DOCINFOA fDocInfoA;

	size_t fCurrentPageIndex;

	bool fDocOpen;
	bool fPageOpen;
	};

#endif // ZCONFIG(OS, Win32)

#endif // __ZPrint_Win__
