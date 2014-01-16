/*  @(#) $Id: ZDateTime.h,v 1.11 2006/04/10 20:44:19 agreen Exp $ */

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

#ifndef __ZDateTime__
#define __ZDateTime__ 1
#include "zconfig.h"

#include "ZTime.h"

#include <ctime>
#include <string>

#include "ZCompat_operator_bool.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZDateTimeDelta

class ZDateTime;

class ZDateTimeDelta
	{
public:
	ZDateTimeDelta();
	ZDateTimeDelta(const ZDateTimeDelta& other);
	ZDateTimeDelta(int theDelta);
	ZDateTimeDelta& operator=(const ZDateTimeDelta& other);

	bool operator==(const ZDateTimeDelta& other) const
		{ return fDelta == other.fDelta; }
	bool operator<(const ZDateTimeDelta& other) const
		{ return fDelta < other.fDelta; }

	bool operator<=(const ZDateTimeDelta& other) const
		{ return !(other < *this); }
	bool operator>(const ZDateTimeDelta& other) const
		{ return other < *this; }
	bool operator>=(const ZDateTimeDelta& other) const
		{ return !(*this < other); }
	bool operator!=(const ZDateTimeDelta& other) const
		{ return ! (*this == other); }


	ZDateTimeDelta operator*(int inScalar) const;
	ZDateTimeDelta& operator*=(int inScalar);

	ZDateTimeDelta operator/(int inScalar) const;
	ZDateTimeDelta& operator/=(int inScalar);

	ZDateTimeDelta operator+(const ZDateTimeDelta& other) const;
	ZDateTimeDelta& operator+=(const ZDateTimeDelta& other);


	ZDateTimeDelta operator-(const ZDateTimeDelta& other) const;
	ZDateTimeDelta& operator-=(const ZDateTimeDelta& other);

	ZDateTimeDelta Absolute() const;

	std::string AsString() const;

	int AsSeconds() const
		{ return fDelta; }

private:
	int fDelta;

	friend class ZDateTime;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZDateTime

class ZStreamR;
class ZStreamW;

class ZDateTime
	{
    ZOOLIB_DEFINE_OPERATOR_BOOL_TYPES(ZDateTime, operator_bool_generator_type, operator_bool_type);
public:
	ZDateTime();
	ZDateTime(const ZDateTime& other);
	ZDateTime(time_t iTime_t);
	ZDateTime(const ZTime& iTime);
	ZDateTime& operator=(const ZDateTime& other);

	long AsLong() const
		{ return fTime_t; }

	ZTime AsTime() const;

	std::string AsStringUTC(const std::string& inFormat) const;
	std::string AsStringLocal(const std::string& inFormat) const;

	void FromStream(const ZStreamR& inStream);
	void ToStream(const ZStreamW& inStream) const;

	operator operator_bool_type() const
		{ return operator_bool_generator_type::translate(fTime_t); }

	bool operator<(const ZDateTime& other) const
		{ return fTime_t < other.fTime_t; }
	bool operator==(const ZDateTime& other) const
		{ return fTime_t == other.fTime_t; }

	bool operator<=(const ZDateTime& other) const
		{ return !(other < *this); }
	bool operator>(const ZDateTime& other) const
		{ return other < *this; }
	bool operator>=(const ZDateTime& other) const
		{ return !(*this < other); }
	bool operator!=(const ZDateTime& other) const
		{ return ! (*this == other); }

	ZDateTime operator+(const ZDateTimeDelta& inDateTimeDelta) const;
	ZDateTime& operator+=(const ZDateTimeDelta& inDateTimeDelta);


	ZDateTimeDelta operator-(const ZDateTime& other) const;
	ZDateTime operator-(const ZDateTimeDelta& inDateTimeDelta) const;
	ZDateTime& operator-=(const ZDateTimeDelta& inDateTimeDelta);

	ZDateTime AsUTC() const;
	ZDateTime AsLocal() const;

	int GetYear() const;
	int GetMonth() const;
	int GetDayOfMonth() const;

	void SetYear(int inYear);
	void SetMonth(int inMonth);
	void SetDayOfMonth(int inDayOfMonth);

	int GetHours() const;
	int GetMinutes() const;
	int GetSeconds() const;

	void SetHours(int inHours);
	void SetMinutes(int inMinutes);
	void SetSeconds(int inSeconds);

	int GetDayOfYear() const;
	void SetDayOfYear(int inDayOfYear);

	int GetDayOfWeek() const;

	static ZDateTime sNowUTC();
	static ZDateTimeDelta sUTCToLocalDelta();
	static ZDateTime sZero;

protected:
	time_t fTime_t;

	friend class ZDateTimeDelta;
	};

#endif // __ZDateTime__
