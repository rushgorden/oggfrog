/*  @(#) $Id: ZPrint.h,v 1.7 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZPrint__
#define __ZPrint__ 1
#include "zconfig.h"

#include "ZDC.h"
#include <string>

// =================================================================================================
#pragma mark -
#pragma mark * ZPrint

class ZPrintJob;
class ZPrintJobSetup;
class ZPrintPageSetup;

namespace ZPrint {

ZRef<ZPrintPageSetup> sCreatePageSetup();

} // namespace ZPrint

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintPageSetup

class ZPrintPageSetup : public ZRefCounted
	{
private:
	ZPrintPageSetup& operator=(const ZPrintPageSetup&); // Not implemented

protected:
	ZPrintPageSetup();
	ZPrintPageSetup(const ZPrintPageSetup&);

public:
	virtual ~ZPrintPageSetup();

	virtual ZRef<ZPrintPageSetup> Clone() = 0;

	virtual bool DoPageSetupDialog() = 0;

	virtual ZRef<ZPrintJobSetup> CreateJobSetup() = 0;

	virtual ZPoint_T<double> GetPageSize() = 0;
	virtual ZPoint_T<double> GetPageOrigin() = 0;
	virtual ZPoint_T<double> GetPaperSize() = 0;
	virtual ZPoint_T<double> GetUnits() = 0;

	virtual void ToStream(const ZStreamW& inStream);
	virtual void FromStream(const ZStreamR& inStream);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJobSetup

class ZPrintJobSetup : public ZRefCounted
	{
private:
	ZPrintJobSetup& operator=(const ZPrintPageSetup&); // Not implemented

protected:
	ZPrintJobSetup();
	ZPrintJobSetup(const ZPrintJobSetup&);

public:
	virtual ~ZPrintJobSetup();

	virtual ZRef<ZPrintJobSetup> Clone() = 0;

	struct Options
		{
		Options();
		enum EPageRange { ePageRangeAll, ePageRangeSelection, ePageRangeExplicit };
		enum { availableKnown = true, availableUnknown = false };

		EPageRange fPageRange;
		size_t fFirstPage;
		size_t fLastPage;

		bool fAvailableKnown;
		size_t fMinPage;
		size_t fMaxPage;

		bool fCollate;
		size_t fNumberOfCopies;
		};

	virtual void SetOptions(const Options& iOptions) = 0;
	virtual Options GetOptions() = 0;

	virtual ZRef<ZPrintJob> DoPrintDialogAndStartJob(const string& iJobName) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZPrintJob

class ZPrintJob : public ZRefCounted
	{
private:
	ZPrintJob(ZPrintJob&); // Not implemented
	ZPrintJob& operator=(const ZPrintJob&); // Not implemented

protected:
	ZPrintJob();

public:
	virtual ~ZPrintJob();

	virtual ZDC GetPageDC() = 0;
	virtual ZDC GetPaperDC() = 0;

	virtual void NewPage() = 0;
	virtual void Finish() = 0;
	virtual void Abort() = 0;

	virtual size_t GetCurrentPageIndex() = 0;

	virtual ZPoint_T<double> GetPageSize() = 0;
	virtual ZPoint_T<double> GetPageOrigin() = 0;
	virtual ZPoint_T<double> GetPaperSize() = 0;
	virtual ZPoint_T<double> GetUnits() = 0;

private:
	};

// =================================================================================================

#endif // __ZPrint__
