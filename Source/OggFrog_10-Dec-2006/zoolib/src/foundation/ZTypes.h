/*  @(#) $Id: ZTypes.h,v 1.19 2006/04/26 22:31:27 agreen Exp $ */

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

#ifndef __ZTypes__
#define __ZTypes__

#include "zconfig.h"

#include <cstdio> // Pull in definition of size_t, which is heavily used by zoolib

// ==================================================
// The usual suite of types with known sizes and signedness.

#if ZCONFIG(OS, Be)

#	include<support/SupportDefs.h>

#elif ZCONFIG(Compiler, MSVC)

	typedef signed char int8;
	typedef unsigned char uint8;

	typedef signed short int16;
	typedef unsigned short uint16;

	typedef signed int int32;
	typedef unsigned int uint32;

	typedef __int64 int64;
	typedef unsigned __int64 uint64;

	typedef int64 bigtime_t;

#elif ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

	#if ZCONFIG(Compiler, GCC)
		#include <Carbon/Carbon.h>
	#else
		#include <MacTypes.h>
	#endif

	typedef SInt8 int8;
	typedef UInt8 uint8;

	typedef SInt16 int16;
	typedef UInt16 uint16;

	typedef SInt32 int32;
	typedef UInt32 uint32;

	typedef SInt64 int64;
	typedef UInt64 uint64;

	typedef int64 bigtime_t;
	
#else

	#include <stdint.h>

	typedef int8_t int8;
	typedef uint8_t uint8;

	typedef int16_t int16;
	typedef uint16_t uint16;

	typedef int32_t int32;
	typedef uint32_t uint32;

	typedef int64_t int64;
	typedef uint64_t uint64;

	typedef int64 bigtime_t;

#endif

// ==================================================

// Use the ZFourCC inline if possible.
inline uint32 ZFourCC(uint8 a, uint8 b, uint8 c, uint8 d)
	{ return uint32((a << 24) | (b << 16) | (c << 8) | d); }

// And the macro if a compile-time constant is needed (case statements).
#define ZFOURCC(a,b,c,d) \
	((uint32)((((uint8)a) << 24) | (((uint8)b) << 16) | (((uint8)c) << 8) | (((uint8)d))))

// ==================================================
// Macros to help harmonize function signatures between 68K and PPC.

#if ZCONFIG(Processor, 68K)
#	define ZPARAM_D0 : __D0
#	define ZPARAM_A0 : __A0
#	define ZPARAM_A1 : __A1
#else
#	define ZPARAM_D0
#	define ZPARAM_A0
#	define ZPARAM_A1
#endif

// ==================================================

struct ZPointPOD
	{
	int32 h;
	int32 v;
	};

struct ZRectPOD
	{
	int32 left;
	int32 top;
	int32 right;
	int32 bottom;
	};

// ==================================================

enum ZType
	{
	eZType_Null,
	eZType_String, eZType_CString,
	eZType_Int8, eZType_Int16, eZType_Int32, eZType_Int64,
	eZType_Float, eZType_Double,
	eZType_Bool,
	eZType_Pointer,
	eZType_Raw,
	eZType_Tuple,
	eZType_RefCounted,
	eZType_Rect,
	eZType_Point,
	eZType_Region,
	eZType_ID,
	eZType_Vector,
	eZType_Type,
	eZType_Time
	};

const char* ZTypeAsString(ZType iType);

// ==================================================
namespace ZooLib {

// There are several places where we need a buffer for some other code
// to dump data into, the content of which we don't care about. Rather
// than having multiple static buffers, or requiring wasteful use of
// stack space (a problem on MacOS 9) we instead have a shared garbage
// buffer. The key thing to remember in using it is that it must never
// be read from -- there's no way to know what other code is also using it.

extern char sGarbageBuffer[4096];

// In many places we need a stack-based buffer. Ideally they'd be quite large
// but on MacOS 9 we don't want to blow the 24K - 32K that's normally available.
// This constant is 4096 on most platforms, 256 on MacOS 9.

#if ZCONFIG(OS, MacOS7) || (ZCONFIG(OS, Carbon) && !__MACH__)
	static const size_t sStackBufferSize = 256;
#else
	static const size_t sStackBufferSize = 4096;
#endif

} // namespace ZooLib

// ==================================================
// For a discussion of the implementation of countof See section 14.3 of
// "Imperfect C++" by Matthew Wilson, published by Addison Wesley.
namespace ZooLib {
template<typename T, int N>
uint8 (&byte_array_of_same_dimension_as(T(&)[N]))[N];
} // namespace ZooLib

#define countof(x) sizeof(ZooLib::byte_array_of_same_dimension_as((x)))

// ==================================================
// AG 2000-08-16. ZMouse being here is a temporary stopgap. We have to lose the file ZMouse.h,
// because it conflicts with Microsoft's file of the same name, and I haven't decided where to
// go with the ZMouse enums.

namespace ZMouse {

enum Motion { eMotionEnter, eMotionMove, eMotionLinger, eMotionLeave };
enum Button { eButtonLeft, eButtonMiddle, eButtonRight,
				buttonLeft = eButtonLeft,
				buttonMiddle = eButtonMiddle,
				buttonRight = eButtonRight };
} // namespace ZMouse

#endif // __ZTypes__
