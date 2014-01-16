/*  @(#) $Id: ZUIMetric.h,v 1.2 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZUIMetric__
#define __ZUIMetric__
#include "zconfig.h"

#include "ZUIBase.h"
#include "ZRGBColor.h"
#include "ZGeom.h" // For ZCoord

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric

class ZUIMetric : public ZUIBase
	{
public:
	ZUIMetric() {}
	virtual ~ZUIMetric() {}

	virtual ZCoord GetMetric() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Fixed

class ZUIMetric_Fixed : public ZUIMetric
	{
public:
	ZUIMetric_Fixed(ZCoord theMetric);

	virtual ZCoord GetMetric();

	void SetMetric(ZCoord theMetric);
private:
	ZCoord fMetric;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Indirect

class ZUIMetric_Indirect : public ZUIMetric
	{
public:
	ZUIMetric_Indirect(ZRef<ZUIMetric> realMetric);
	virtual ~ZUIMetric_Indirect();

// From ZUIMetric
	virtual ZCoord GetMetric();

	void SetRealMetric(ZRef<ZUIMetric> realMetric);
	ZRef<ZUIMetric> GetRealMetric();

private:
	ZRef<ZUIMetric> fRealMetric;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIMetric_Win32System

#if ZCONFIG(OS, Win32)

class ZUIMetric_Win32System : public ZUIMetric
	{
public:
	ZUIMetric_Win32System(int systemMetricIndex);
	ZUIMetric_Win32System(int systemMetricIndex, ZCoord inCompensation);

	virtual ZCoord GetMetric();
protected:
	int fSystemMetricIndex;
	ZCoord fCompensation;
	};

#endif // ZCONFIG(OS, Win32)

#endif // __ZUIMetric__
