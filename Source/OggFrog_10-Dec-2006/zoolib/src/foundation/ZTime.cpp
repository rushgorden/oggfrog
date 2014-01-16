static const char rcsid[] = "@(#) $Id: ZTime.cpp,v 1.13 2006/07/15 20:54:22 goingware Exp $";

/* ------------------------------------------------------------
Copyright (c) 2003 Andrew Green and Learning in Motion, Inc.
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

#include "ZTime.h"
#include "ZThread.h"

#include <math.h> // For NAN and isnan -- cmath doesn't make isnan visible on gcc/MacOSX

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <DriverServices.h>
#endif

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/UTCUtils.h>
#	include <CarbonCore/DriverServices.h>
#endif

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

#if ZCONFIG(OS, POSIX) || ZCONFIG(OS, Be)
#	include <sys/time.h> // For timeval
#endif

#if ZCONFIG(OS, Be)
#	include <kernel/OS.h> // for system_time
#endif

ZMutex ZTime::sMutex;

// =================================================================================================
#pragma mark -
#pragma mark * ZTime

ZTime::ZTime()
:	fVal(NAN)
	{}

ZTime::operator operator_bool_type() const
	{ return operator_bool_generator_type::translate(!isnan(fVal)); }

bool ZTime::operator<(const ZTime& iOther) const
	{
	if (isnan(fVal))
		{
		if (isnan(iOther.fVal))
			return false;
		return true;
		}

	if (isnan(iOther.fVal))
		return false;

	return fVal < iOther.fVal;
	}

bool ZTime::operator==(const ZTime& iOther) const
	{
	if (isnan(fVal))
		{
		if (isnan(iOther.fVal))
			return true;
		return false;
		}

	if (isnan(iOther.fVal))
		return false;

	return fVal == iOther.fVal;
	}

int ZTime::Compare(const ZTime& iOther) const
	{
	if (isnan(fVal))
		{
		if (isnan(iOther.fVal))
			return 0;
		return -1;
		}

	if (isnan(iOther.fVal))
		return 1;

	if (fVal < iOther.fVal)
		return -1;

	if (fVal > iOther.fVal)
		return 1;

	return 0;	
	}

ZTime ZTime::sNow()
	{
#if ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	UTCDateTime theUTCDateTime;
	::GetUTCDateTime(&theUTCDateTime, kUTCDefaultOptions);

	double result = double(theUTCDateTime.highSeconds) + double(theUTCDateTime.lowSeconds);
	result -= kEpochDelta_1904_To_1970;
	result += double(theUTCDateTime.fraction) / 65536.0;
	return result;

#elif ZCONFIG(OS, MacOS7)

	// This has a resolution of one second, not a lot of use really.
	unsigned long theDateTime;
	::GetDateTime(&theDateTime);
	return theDateTime - kEpochDelta_1904_To_1970;

#elif ZCONFIG(OS, Win32)

	// FILETIME is a 64 bit count of the number of 100-nanosecond (1e-7)
	// periods since January 1, 1601 (UTC).
	FILETIME ft;
	::GetSystemTimeAsFileTime(&ft);

	// 100 nanoseconds is one ten millionth of a second, so divide the
	// count of 100 nanoseconds by ten million to get the count of seconds.
	double result = (*reinterpret_cast<int64*>(&ft)) / 1e7;

	// Subtract the number of seconds between 1601 and 1970.
	result -= kEpochDelta_1601_To_1970;
    return result;

#elif ZCONFIG(OS, POSIX) || ZCONFIG(OS, Be)

	timeval theTimeVal;
	::gettimeofday(&theTimeVal, nil);
	return theTimeVal.tv_sec + double(theTimeVal.tv_usec) / 1e6;

#else

	#error Unsupported platform

#endif	
	}

ZTime ZTime::sSystem()
	{
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

	Nanoseconds theNanoseconds = AbsoluteToNanoseconds(UpTime());
	return double(*reinterpret_cast<uint64*>(&theNanoseconds)) / 1e9;

#elif ZCONFIG(OS, Win32)

	uint64 frequency;
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&frequency));
	if (!frequency)
		{
		// A zero frequency indicates there's no performance counter support,
		// so we fall back to GetTickCount.
		return double(::GetTickCount()) / 1e3;
		}

	uint64 time;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
	return double(time) / double(frequency);

#elif ZCONFIG(OS, POSIX)

	/* AG 2003-10-26.
	I've spent all day trying to find a monotonic clock for unix. Mozilla's
	PR_GetInterval has sensible implementations for classic MacOS, Irix and
	Solaris. But the unix implementation just uses gettimeofday.
	The POSIX 'clock_gettime' using CLOCK_MONOTONIC looked promising, even
	being supported on Linux (in librt). However on Linux it simply tracks
	gettimeofday and is thus not monotonic, at least across process invocations
	N.B. I didn't try changing the clock whilst a process was running.

	For the moment we're guaranteeing a monotonic clock by tracking the last
	value returned by gettimeofday and if it goes backwards we accumulate a
	delta that is applied to every result.
	*/

	static volatile double sLast = 0;
	static volatile double sDelta = 0;

	timeval theTimeVal;
	::gettimeofday(&theTimeVal, nil);
	double result = theTimeVal.tv_sec + double(theTimeVal.tv_usec) / 1e6;

	// I know that this is not threadsafe. However, we'd need to have two
	// threads at the right (wrong) place at the same time when the clock
	// has just changed, leading to sDelta getting incremented twice. But
	// we'll still see a monotonically increasing clock.
	if (sLast > result)
		sDelta += sLast - result;

	sLast = result;
	return result + sDelta;

#elif ZCONFIG(OS, Be)

	return double(::system_time()) / 1e6;

#else

	#error Unsupported platform

#endif
	}
