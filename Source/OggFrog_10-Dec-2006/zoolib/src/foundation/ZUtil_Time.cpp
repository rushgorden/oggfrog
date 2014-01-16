static const char rcsid[] = "@(#) $Id: ZUtil_Time.cpp,v 1.12 2006/04/26 22:31:27 agreen Exp $";

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

#include "ZUtil_Time.h"
#include "ZDebug.h"
#include "ZStream.h"

#include <ctime>

#if ZCONFIG(OS, MacOS7)
#	include <OSUtils.h>
#endif

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

#if ZCONFIG(OS, POSIX) || ZCONFIG(OS, Be)
#	include <sys/time.h>
#endif

#include "ZThread.h"

#include <cstdio>

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Time

static ZMutex sTimeMutex;

#if _MSL_USING_MW_C_HEADERS
// Return 1 if iYear (1900-based) is a leap year. 0 Otherwise
static int sLeapYear(int iYear)
	{
	if (iYear % 4)
		{
		// It's not a multiple of 4 --> is not a leap year
		return 0;
		}

	if (((iYear + 1900) % 400) == 0)
		{
		// Is a multiple of 4 and is a multiple of 400 --> is a leap year.
		return 1;
		}

	if ((iYear % 100) == 0)
		{
		// Is a multiple of 4, and not a multiple of 400,
		// and is a multiple of 100 --> is not a leap year.
		return 0;
		}

	// Is a multiple 4, and not a multiple of 400 and not a multiple of 100 --> is a leap year.
	return 1;
	}

static const uint32 kSecondsPerMinute = 60;
static const uint32 kSecondsPerHour = 60 * kSecondsPerMinute;
static const uint32 kSecondsPerDay = 24 * kSecondsPerHour;
static const uint32 sMonthToDays[2][13] =
	{
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
	};
#endif // _MSL_USING_MW_C_HEADERS

static void sTimeToTM(time_t iTime_t, struct tm& oTM)
	{
	#if _MSL_USING_MW_C_HEADERS

		// MSL assumes time_t values are in the local timezone, so it applies
		// the local-->UTC transform when gmtime is called. As MSL doesn't
		// support the timezone name field in struct tm it's simpler if we
		// just do the work ourselves.

		// iTime_t is 1970-based, so convert to 1900-based.
		uint32 timeSince1900 = iTime_t + ZTime::kEpochDelta_1900_To_1970;

		uint32 days = uint32(timeSince1900) / kSecondsPerDay;
		uint32 seconds = uint32(timeSince1900) % kSecondsPerDay;
	
		// January 1, 1900 was a Monday.
		oTM.tm_wday = (days + 1) % 7;
	
		uint32 years = 0;
	
		for (;;)
			{
			// AG 2003-10-28. This is taken from MSL, and it seems
			// incredibly ugly to have to iterate (as of 2005) 105 times
			// to figure out the number of years/number of days represented
			// by iTime_t.
			uint32 daysThisYear = sLeapYear(years) ? 366 : 365;
	
			if (days < daysThisYear)
				break;
	
			days -= daysThisYear;
			++years;
			}
	
		oTM.tm_year = years;
		oTM.tm_yday = days;
	
		uint32 months = 0;
	
		uint32 isLeapYear = sLeapYear(years);
	
		for (;;)
			{
			uint32 daysThruThisMonth = sMonthToDays[isLeapYear][months + 1];
	
			if (days < daysThruThisMonth)
				{
				days -= sMonthToDays[isLeapYear][months];
				break;
				}
	
			++months;
			}
	
		oTM.tm_mon = months;
		oTM.tm_mday = days + 1;
	
		oTM.tm_hour = seconds / kSecondsPerHour;
	
		seconds = seconds % kSecondsPerHour;
	
		oTM.tm_min = seconds / kSecondsPerMinute;
		oTM.tm_sec = seconds % kSecondsPerMinute;

		oTM.tm_isdst = 0;

	#else

		ZMutexLocker locker(sTimeMutex);
		struct tm* theTM = gmtime(&iTime_t);
		oTM = *theTM;

	#endif
	}

#if _MSL_USING_MW_C_HEADERS
// MSL's strftime, in CW 8 and CW 9, does not handle timezone fields very well.
// On POSIX you call gmtime or localtime and end up with a tm structure
// containing a timezone value and a pointer to the textual name of that
// timezone, for use by %z and %Z respectively. On MSL you just get the
// current timezone value for %z, and usually an empty string for %Z.
// So we handle the %z and %Z fields ourselves.

static string sFormatTimeUTC(const struct tm& iTM, const string& iFormat)
	{	
	string realFormat;
	realFormat.reserve(iFormat.size());

	for (string::const_iterator i = iFormat.begin(); i != iFormat.end(); ++i)
		{
		if (*i == '%')
			{
			++i;
			if (i == iFormat.end())
				break;
			if (*i == 'z')
				{
				realFormat+= "+0000";
				}
			else if (*i == 'Z')
				{
				realFormat += "UTC";
				}
			else
				{
				realFormat += '%';
				realFormat += *i;
				}
			}
		else
			{
			realFormat += *i;
			}
		}

	char buf[256];
	strftime(buf, 255, realFormat.c_str(), const_cast<struct tm*>(&iTM));
	return buf;
	}

static string sFormatTimeLocal(const struct tm& iTM, const string& iFormat, int iDeltaSeconds)
	{
	string realFormat;
	realFormat.reserve(iFormat.size());

	for (string::const_iterator i = iFormat.begin(); i != iFormat.end(); ++i)
		{
		if (*i == '%')
			{
			++i;
			if (i == iFormat.end())
				break;
			if (*i == 'z')
				{
				int deltaMinutes = iDeltaSeconds / 60;
				char buf[6];
				if (deltaMinutes < 0)
					sprintf(buf, "-%02d%02d", (-deltaMinutes) / 60, (-deltaMinutes) % 60);
				else
					sprintf(buf, "+%02d%02d", deltaMinutes / 60, deltaMinutes % 60);
				realFormat += buf;
				}
			else if (*i == 'Z')
				{
				// Ignore the Z field, just like MSL.
				}
			else
				{
				realFormat += '%';
				realFormat += *i;
				}
			}
		else
			{
			realFormat += *i;
			}
		}

	char buf[256];
	strftime(buf, 255, realFormat.c_str(), const_cast<struct tm*>(&iTM));
	return buf;
	}
#endif // _MSL_USING_MW_C_HEADERS

/** iTime is assumed to be UTC-based, return a textual version
of the date/time in UTC. */
string ZUtil_Time::sAsStringUTC(ZTime iTime, const string& iFormat)
	{
	if (!iTime)
		return "invalid";

	struct tm theTM;

	#if _MSL_USING_MW_C_HEADERS

		sTimeToTM(iTime.fVal, theTM);
		return sFormatTimeUTC(theTM, iFormat);

	#else

		char buf[256];
		sTimeToTM(time_t(iTime.fVal), theTM);
		strftime(buf, 255, iFormat.c_str(), &theTM);
		return buf;

	#endif
	}

#if _MSL_USING_MW_C_HEADERS
static int sUTCToLocalDelta()
	{
	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

		MachineLocation loc;
		int delta = 0;

		::ReadLocation(&loc);

		// Don't bother if the user has not set the location
		if (loc.latitude != 0 && loc.longitude != 0 && loc.u.gmtDelta != 0)
			{
			delta = loc.u.gmtDelta & 0x00FFFFFF;

			if (delta & 0x00800000)
				delta |= 0xFF000000;
			}
		return delta;

	#elif ZCONFIG(OS, POSIX) || ZCONFIG(OS, Be)

		struct timezone zone;

		if (::gettimeofday(NULL, &zone) == -1)
			return 0;

		// tz_minuteswest is the number of minutes to be added to local
		// time to get UTC. sUTCDelta is the number of seconds to be added to UTC
		// to get the local time. So we multiply by negative 60.
		if (zone.tz_dsttime)
			{
			// tz_minuteswest does not itself account for DST.
			return -60 * (zone.tz_minuteswest - 60);
			}
		else
			{
			return -60 * zone.tz_minuteswest;
			}

	#elif ZCONFIG(OS, Win32)

		TIME_ZONE_INFORMATION tzi;
		int delta = 0;
		switch (::GetTimeZoneInformation(&tzi))
			{
			case TIME_ZONE_ID_UNKNOWN:
				{
				delta = tzi.Bias;
				break;
				}
			case TIME_ZONE_ID_STANDARD:
				{
				delta = tzi.Bias + tzi.StandardBias;
				break;
				}
			case TIME_ZONE_ID_DAYLIGHT:
				{
				delta = tzi.Bias + tzi.DaylightBias;
				break;
				}
			}
		return -60 * delta;

	#else

		#error Unsupported platform

	#endif
	}
#endif // _MSL_USING_MW_C_HEADERS

/** iTime is assumed to be UTC-based, return a textual version
of the date/time in the local time zone. */
string ZUtil_Time::sAsStringLocal(ZTime iTime, const string& iFormat)
	{
	if (!iTime)
		return "invalid";

	struct tm theTM;

	#if _MSL_USING_MW_C_HEADERS

		int delta = sUTCToLocalDelta();
		time_t localTime = iTime.fVal + delta;
		sTimeToTM(localTime, theTM);
		return sFormatTimeLocal(theTM, iFormat, delta);

	#else

		time_t theTimeT = time_t(iTime.fVal);
	
		ZMutexLocker locker(sTimeMutex);
		theTM = *::localtime(&theTimeT);
		locker.Release();
	
		char buf[256];
		strftime(buf, 255, iFormat.c_str(), &theTM);
		return buf;
	#endif
	}
