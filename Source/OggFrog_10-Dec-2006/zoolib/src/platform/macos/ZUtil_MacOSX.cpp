static const char rcsid[] = "@(#) $Id: ZUtil_MacOSX.cpp,v 1.2 2006/08/02 21:40:42 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2006 Andrew Green and Learning in Motion, Inc.
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

#include "ZUtil_MacOSX.h"

#if ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)

#if ZCONFIG(OS, MacOSX)
#	include <CoreFoundation/CFString.h>
#else
#	include <CFString.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_MacOSX

static CFStringRef sEmptyCFString = CFSTR("");

CFStringRef ZUtil_MacOSX::sCreateCFStringUTF8(const string8& iString8)
	{
	if (CFIndex sourceSize = iString8.size())
		{
		return ::CFStringCreateWithBytes(nil,
			reinterpret_cast<const UInt8*>(iString8.data()), sourceSize,
			kCFStringEncodingUTF8, false);
		}

	::CFRetain(sEmptyCFString);
	return sEmptyCFString;
	}

CFStringRef ZUtil_MacOSX::sCreateCFStringUTF16(const string16& iString16)
	{
	if (CFIndex sourceSize = iString16.size())
		{
		return ::CFStringCreateWithCharacters(nil,
				reinterpret_cast<const UniChar*>(iString16.data()), sourceSize);
		}

	::CFRetain(sEmptyCFString);
	return sEmptyCFString;
	}

string8 ZUtil_MacOSX::sAsUTF8(const CFStringRef& iCFString)
	{
	if (const char *s = ::CFStringGetCStringPtr(iCFString, kCFStringEncodingUTF8))
		return string8(s);

	const CFIndex sourceCU = ::CFStringGetLength(iCFString);
	if (sourceCU == 0)
		return string8();

	// Worst case is six bytes per code unit.
	const size_t bufferSize = sourceCU * 6;
	string8 result(' ', bufferSize);

	UInt8* buffer = reinterpret_cast<UInt8*>(const_cast<char*>(result.data()));

	CFIndex bufferUsed;
	::CFStringGetBytes(iCFString, CFRangeMake(0, sourceCU),
		kCFStringEncodingUTF8, 1, false,
		buffer, bufferSize, &bufferUsed);

	result.resize(bufferUsed);

	return result;
	}

string16 ZUtil_MacOSX::sAsUTF16(const CFStringRef& iCFString)
	{
	const CFIndex sourceCU = ::CFStringGetLength(iCFString);
	if (sourceCU == 0)
		return string16();

	if (const UniChar* s = ::CFStringGetCharactersPtr(iCFString))
		return string16(reinterpret_cast<const UTF16*>(s), sourceCU);

	string16 result(' ', sourceCU);

	UniChar* buffer = reinterpret_cast<UniChar*>(const_cast<UTF16*>(result.data()));

	::CFStringGetCharacters(iCFString, CFRangeMake(0, sourceCU), buffer);
	return result;
	}

#endif // ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)

