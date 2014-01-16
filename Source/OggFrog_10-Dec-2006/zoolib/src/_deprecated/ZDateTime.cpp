static const char rcsid[] = "@(#) $Id: ZDateTime.cpp,v 1.23 2006/07/15 20:54:22 goingware Exp $";

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

#include "ZDateTime.h"
#include "ZDebug.h"
#include "ZStream.h"
//#include "ZTime.h" // For ZTime::sMutex

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <OSUtils.h>
#endif

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/OSUtils.h>
#endif

#if ZCONFIG(OS, Win32)
#	include "ZWinHeader.h"
#endif

#if ZCONFIG(OS, POSIX)
#	include <sys/time.h>
#endif

#include "ZThread.h"

#include <cstdio>

using std::string;

// Check to see if this system defines _TBIAS, being the offset between
// Jan 1 1970 & Jan 1 1900. If it does, then we need to do the offsetting
// thing. But I haven't come across a system which actually defines this, so
// trip a compilation error when we find such a system, and make sure that
// _TBIAS has the right value. For the time being we check the value we get
// and compensate if it appears to be way out of wack.
#ifdef _TBIAS
#	error _TBIAS is defined
#endif

ZDateTime ZDateTime::sZero = 0;

static bool sSystemEpochIs1970()
	{
	static bool sChecked = false;
	static bool sIs1970 = false;
	if (!sChecked)
		{
		if (time(nil) < 50 * 365 * 24 * 60 * 60)
			sIs1970 = true;
		sChecked = true;
		}
	return sIs1970;
	}

static time_t sToSystemEpoch(time_t iTime_t)
	{
	if (sSystemEpochIs1970())
		return iTime_t - ZTime::kEpochDelta_1900_To_1970;
	else
		return iTime_t;
	}

static time_t sFromSystemEpoch(time_t iTime_t)
	{
	if (sSystemEpochIs1970())
		return iTime_t + ZTime::kEpochDelta_1900_To_1970;
	else
		return iTime_t;
	}

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
		// Is a multiple of 4, and not a multiple of 400, and is a multiple of 100 --> is not a leap year.
		return 0;
		}

	// Is a multiple of 4, and not a multiple of 400 and not a multiple of 100 --> is a leap year.
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

		uint32 days = uint32(iTime_t) / kSecondsPerDay;
		uint32 seconds = uint32(iTime_t) % kSecondsPerDay;
	
		// January 1, 1900 was a Monday.
		oTM.tm_wday = (days + 1) % 7;
	
		uint32 years = 0;
	
		for (;;)
			{
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

		time_t theTimeT = sToSystemEpoch(iTime_t);
//##		ZMutexLocker locker(ZTime::sMutex);
		struct tm* theTM = ::gmtime(&theTimeT);
		oTM = *theTM;

	#endif
	}

static time_t sTMToTime(const struct tm& iTM)
	{
	time_t result;

	#if _MSL_USING_MW_C_HEADERS

		// MSL's mktime does no timezone conversion.
		result = mktime(const_cast<struct tm*>(&iTM));

	#else

		// timegm assumes iTM is in UTC.
		result = timegm(const_cast<struct tm*>(&iTM));

	#endif

	return sFromSystemEpoch(result);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDateTime

ZDateTime::ZDateTime()
:	fTime_t(0)
	{}

ZDateTime::ZDateTime(const ZDateTime& other)
:	fTime_t(other.fTime_t)
	{}

ZDateTime::ZDateTime(time_t iTime_t)
:	fTime_t(iTime_t)
	{}

ZDateTime::ZDateTime(const ZTime& iTime)
:	fTime_t(time_t(iTime.fVal) + ZTime::kEpochDelta_1900_To_1970)
	{}

ZDateTime& ZDateTime::operator=(const ZDateTime& other)
	{
	fTime_t = other.fTime_t;
	return *this;
	}

ZDateTime ZDateTime::operator+(const ZDateTimeDelta& inDateTimeDelta) const
	{
	ZDateTime newTime(*this);
	return newTime += inDateTimeDelta;
	}

ZDateTime& ZDateTime::operator+=(const ZDateTimeDelta& inDateTimeDelta)
	{
	fTime_t += inDateTimeDelta.fDelta;
	return *this;
	}

ZDateTimeDelta ZDateTime::operator-(const ZDateTime& other) const
	{
	if (fTime_t > other.fTime_t)
		return ZDateTimeDelta(fTime_t - other.fTime_t);
	return ZDateTimeDelta(-((other.fTime_t - fTime_t)));
	}

ZDateTime ZDateTime::operator-(const ZDateTimeDelta& inDateTimeDelta) const
	{
	ZDateTime newTime(*this);
	return newTime -= inDateTimeDelta;
	}

ZDateTime& ZDateTime::operator-=(const ZDateTimeDelta& inDateTimeDelta)
	{
	fTime_t -= inDateTimeDelta.fDelta;
	return *this;
	}

ZTime ZDateTime::AsTime() const
	{ return ZTime(fTime_t - ZTime::kEpochDelta_1900_To_1970); }

ZDateTime ZDateTime::AsUTC() const
	{ return *this - sUTCToLocalDelta(); }

ZDateTime ZDateTime::AsLocal() const
	{ return *this + sUTCToLocalDelta(); }

ZDateTime ZDateTime::sNowUTC()
	{
	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

		unsigned long theDateTime;
		::GetDateTime(&theDateTime);
		// theDateTime is seconds since 00:00 January 1 1904, local time. There
		// were no leap days *between* 1900 and 1904 (1904 was a leap year,
		// but the extra day happened after January 1), so to convert to
		// our epoch (1900) we simply add 4 years worth of seconds.
		theDateTime += ZTime::kEpochDelta_1900_To_1904;

		// Return the value, converted to UTC
		return ZDateTime(theDateTime).AsUTC();

	#elif ZCONFIG(OS, POSIX)

		// POSIX returns time since system epoch start, defined as 00:00 January 1 1970 UTC.
		// This has not always been the case on Linux, but certainly is true now.
		// As we use 1900 as our epoch start we have to convert.
		struct timeval theTimeVal;

		if (::gettimeofday(&theTimeVal, 0) == -1)
			return 0;

		return ZDateTime(sFromSystemEpoch(theTimeVal.tv_sec));

	#elif ZCONFIG(OS, Win32)

		// MSL's time() calls GetLocalTime. Rather than mucking about with timezone conversions,
		// I'm just manually doing the conversion from SYSTEMTIME to 1900-based count of seconds.
		SYSTEMTIME systemTime;
		::GetSystemTime(&systemTime);
		int years = systemTime.wYear - 1900;

		int leapDays;
		if (systemTime.wMonth > 2)
			leapDays = years / 4;
		else
			leapDays = (years - 1) / 4;

		int days = (years * 365) + leapDays;

		// systemTime.wMonth is 1-based.
		static const int daysPriorToMonth[] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
		days += daysPriorToMonth[systemTime.wMonth];

		// systemTime.wDay is 1-based.
		days += systemTime.wDay - 1;

		return ZDateTime((((((days * 24) + systemTime.wHour) * 60) + systemTime.wMinute) * 60) + systemTime.wSecond);

	#else

		#error Unsupported platform

	#endif
	}

ZDateTimeDelta ZDateTime::sUTCToLocalDelta()
	{
	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

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
		return ZDateTimeDelta(delta);

	#elif ZCONFIG(OS, POSIX)

		struct timezone zone;

		if (::gettimeofday(NULL, &zone) == -1)
			return 0;

		// tz_minuteswest is the number of minutes to be added to local
		// time to get UTC. sUTCToLocalDelta is the number of seconds to be added to UTC
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

void ZDateTime::FromStream(const ZStreamR& inStream)
	{ fTime_t = inStream.ReadInt32(); }

void ZDateTime::ToStream(const ZStreamW& inStream) const
	{ inStream.WriteInt32(fTime_t); }

int ZDateTime::GetYear() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	// tm_year is 1900-based
	return theTM.tm_year + 1900;
	}

// inYear must be a full 4-digit specification
void ZDateTime::SetYear(int inYear) 
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	// tm_year is 1900-based
	theTM.tm_year = inYear - 1900; 
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetMonth() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_mon + 1;
	}

void ZDateTime::SetMonth(int inMonth) 
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	// n.b. 1 <= inMonth <= 12 , but 0 <= tm_mon <= 11
	theTM.tm_mon = inMonth - 1;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetDayOfMonth() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_mday;
	}

void ZDateTime::SetDayOfMonth(int inDayOfMonth) 
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	theTM.tm_mday = inDayOfMonth;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetHours() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_hour;
	}

void ZDateTime::SetHours(int inHours)
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	theTM.tm_hour = inHours;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetMinutes() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_min;
	}

void ZDateTime::SetMinutes(int inMinutes)
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	theTM.tm_min = inMinutes;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetSeconds() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_sec;
	}

void ZDateTime::SetSeconds(int inSeconds)
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	theTM.tm_sec = inSeconds;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetDayOfYear() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_yday;
	}

void ZDateTime::SetDayOfYear(int inDayOfYear)
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	theTM.tm_yday = inDayOfYear;
	fTime_t = sTMToTime(theTM);
	}

int ZDateTime::GetDayOfWeek() const
	{
	struct tm theTM;
	sTimeToTM(fTime_t, theTM);
	return theTM.tm_wday;
	}

#if _MSL_USING_MW_C_HEADERS
static string sFormatTime(const struct tm& iTM, const string& iFormat, bool iIsUTC)
	{
	// MSL's strftime, in CW 8 and CW 9, does not handle timezone fields very well.
	// On POSIX you call gmtime or localtime and end up with a tm structure
	// containing a timezone value and a pointer to the textual name of that
	// timezone, for use by %z and %Z respectively. On MSL you just get the
	// current timezone value for %z, and usually an empty string for %Z.
	// So we handle the %z and %Z fields ourselves.
	
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
				if (iIsUTC)
					{
					realFormat+= "+0000";
					}
				else
					{
					int deltaMinutes = ZDateTime::sUTCToLocalDelta().AsSeconds() / 60;
					char buf[6];
					if (deltaMinutes < 0)
						sprintf(buf, "-%02d%02d", (-deltaMinutes) / 60, (-deltaMinutes) % 60);
					else
						sprintf(buf, "+%02d%02d", deltaMinutes / 60, deltaMinutes % 60);
					realFormat += buf;
					}
				}
			else if (*i == 'Z')
				{
				if (iIsUTC)
					{
					realFormat += "UTC";
					}
				else
					{
					// Ignore the Z field, just like MSL.
					}
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

/** The instance is assumed to be UTC-based, return a textual version
of the date/time in UTC. */
string ZDateTime::AsStringUTC(const string& iFormat) const
	{
	struct tm theTM;

	#if _MSL_USING_MW_C_HEADERS

		sTimeToTM(fTime_t, theTM);
		return sFormatTime(theTM, iFormat, true);

	#else

		char buf[256];
		sTimeToTM(fTime_t, theTM);
		::strftime(buf, 255, iFormat.c_str(), &theTM);
		return buf;

	#endif
	}

/** The instance is assumed to be UTC-based, return a textual version
of the date/time in the local time zone. */
string ZDateTime::AsStringLocal(const string& iFormat) const
	{
	struct tm theTM;

	#if _MSL_USING_MW_C_HEADERS

		ZDateTime thisInLocalTimeZone = this->AsLocal();
		sTimeToTM(thisInLocalTimeZone.fTime_t, theTM);
		return sFormatTime(theTM, iFormat, false);

	#else

		time_t theTimeT = sToSystemEpoch(fTime_t);
	
//##		ZMutexLocker locker(ZTime::sMutex);
		theTM = *::localtime(&theTimeT);
//##		locker.Release();
	
		char buf[256];
		::strftime(buf, 255, iFormat.c_str(), &theTM);
		return buf;
	#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDateTimeDelta

ZDateTimeDelta::ZDateTimeDelta()
:	fDelta(0)
	{}

ZDateTimeDelta::ZDateTimeDelta(const ZDateTimeDelta& other)
:	fDelta(other.fDelta)
	{}

ZDateTimeDelta::ZDateTimeDelta(int theDelta)
:	fDelta(theDelta)
	{}

ZDateTimeDelta& ZDateTimeDelta::operator=(const ZDateTimeDelta& other)
	{
	fDelta = other.fDelta;
	return *this;
	}

ZDateTimeDelta ZDateTimeDelta::operator*(int inScalar) const
	{
	ZDateTimeDelta theDelta(*this);
	return theDelta *= inScalar;
	}
ZDateTimeDelta& ZDateTimeDelta::operator*=(int inScalar)
	{
	fDelta *= inScalar;
	return *this;
	}

ZDateTimeDelta ZDateTimeDelta::operator/(int inScalar) const
	{
	ZDateTimeDelta theDelta(*this);
	return theDelta /= inScalar;
	}

ZDateTimeDelta& ZDateTimeDelta::operator/=(int inScalar)
	{
	fDelta /= inScalar;
	return *this;
	}

ZDateTimeDelta ZDateTimeDelta::operator+(const ZDateTimeDelta& other) const
	{
	ZDateTimeDelta theDelta(*this);
	return theDelta += other;
	}

ZDateTimeDelta& ZDateTimeDelta::operator+=(const ZDateTimeDelta& other)
	{
	fDelta += other.fDelta;
	return *this;
	}

ZDateTimeDelta ZDateTimeDelta::operator-(const ZDateTimeDelta& other) const
	{
	ZDateTimeDelta theDelta(*this);
	return theDelta -= other;
	}

ZDateTimeDelta& ZDateTimeDelta::operator-=(const ZDateTimeDelta& other)
	{
	fDelta -= other.fDelta;
	return *this;
	}

ZDateTimeDelta ZDateTimeDelta::Absolute() const
	{
	if (fDelta < 0)
		return ZDateTimeDelta(-fDelta);
	return ZDateTimeDelta(*this);
	}

string ZDateTimeDelta::AsString() const
	{
	// Returns a string representation of the duration, ddd:hh:mm:ss
	string theString;
	int seconds = fDelta % 60;
	int minutes = (fDelta / 60) % 60;
	int hours = (fDelta / (60*60)) % 24;
	int days = (fDelta / (60*60*24));

	char temp[256];
	if (days > 0)
		{
		sprintf(temp, "%d", days);
		theString += temp;
		theString += ':';
		}

	if (days > 0)
		{
		sprintf(temp, "%.2d", hours);
		theString += temp;
		theString += ':';
		}
	else if (hours > 0)
		{
		sprintf(temp, "%d", hours);
		theString += temp;
		theString += ':';
		}

	if (days > 0 || hours > 0)
		{
		sprintf(temp, "%.2d", minutes);
		theString += temp;
		theString += ':';
		}
	else if (minutes > 0)
		{
		sprintf(temp, "%d", minutes);
		theString += temp;
		theString += ':';
		}

	if (days > 0 || hours > 0 || minutes > 0)
		{
		sprintf(temp, "%.2d", seconds);
		theString += temp;
		}
	else
		{
		sprintf(temp, "%d", seconds);
		theString += temp;
		}

	return theString;
	}
