/*  @(#) $Id: ZUtil_MacOSX.h,v 1.2 2006/08/02 21:40:42 agreen Exp $ */

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

#ifndef __ZUtil_MacOSX__
#define __ZUtil_MacOSX__ 1
#include "zconfig.h"


#if ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)

#include "ZUnicode.h"

#if ZCONFIG(OS, MacOSX)
#	include <CoreFoundation/CFBase.h>
#else
#	include <CFBase.h>
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_MacOSX

namespace ZUtil_MacOSX {

CFStringRef sCreateCFStringUTF8(const string8& iString8);
CFStringRef sCreateCFStringUTF16(const string16& iString16);

string8 sAsUTF8(const CFStringRef& iCFString);
string16 sAsUTF16(const CFStringRef& iCFString);

} // namespace ZUtil_MacOSX

#endif // ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)

#endif // __ZUtil_MacOSX__
