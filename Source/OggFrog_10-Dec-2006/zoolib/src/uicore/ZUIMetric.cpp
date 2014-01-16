static const char rcsid[] = "@(#) $Id: ZUIMetric.cpp,v 1.3 2006/07/12 19:41:08 agreen Exp $";

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

#include "ZUIMetric.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric

ZCoord ZUIMetric::GetMetric()
	{
	ZUnimplemented();
	return 0;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Fixed

ZCoord ZUIMetric_Fixed::GetMetric()
	{ return fMetric; }

ZUIMetric_Fixed::ZUIMetric_Fixed(ZCoord theMetric)
:	fMetric(theMetric)
	{}

void ZUIMetric_Fixed::SetMetric(ZCoord theMetric)
	{
	fMetric = theMetric;
	this->Changed();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Indirect

ZUIMetric_Indirect::ZUIMetric_Indirect(ZRef<ZUIMetric> realMetric)
:	fRealMetric(realMetric)
	{}

ZUIMetric_Indirect::~ZUIMetric_Indirect()
	{}

ZCoord ZUIMetric_Indirect::GetMetric()
	{
	if (ZRef<ZUIMetric> theRealMetric = fRealMetric)
		return theRealMetric->GetMetric();
	return ZUIMetric::GetMetric();
	}

void ZUIMetric_Indirect::SetRealMetric(ZRef<ZUIMetric> realMetric)
	{
	if (fRealMetric == realMetric)
		return;
	fRealMetric = realMetric;
	this->Changed();
	}

ZRef<ZUIMetric> ZUIMetric_Indirect::GetRealMetric()
	{ return fRealMetric; }

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Win32System

#if ZCONFIG(OS, Win32)

ZUIMetric_Win32System::ZUIMetric_Win32System(int systemMetricIndex)
:	fSystemMetricIndex(systemMetricIndex), fCompensation(0)
	{}

ZUIMetric_Win32System::ZUIMetric_Win32System(int systemMetricIndex, ZCoord inCompensation)
:	fSystemMetricIndex(systemMetricIndex), fCompensation(inCompensation)
	{}

ZCoord ZUIMetric_Win32System::GetMetric()
	{ return ::GetSystemMetrics(fSystemMetricIndex) + fCompensation; }

#endif // ZCONFIG(OS, Win32)
