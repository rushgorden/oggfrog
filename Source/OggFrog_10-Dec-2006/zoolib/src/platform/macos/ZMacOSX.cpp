static const char rcsid[] = "@(#) $Id: ZMacOSX.cpp,v 1.6 2006/07/23 22:07:26 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZMacOSX.h"

#define kDebug_MacOSX 1

static bool sIsMacOSX_Checked;
static bool sIsMacOSX_Value;

bool ZMacOSX::sIsMacOSX()
	{
	#if ZCONFIG(OS, MacOSX)

		return true;

	#elif ZCONFIG(OS, Carbon)

		if (!sIsMacOSX_Checked)
			{
			UInt32 systemVersion;
			OSErr err = ::Gestalt(gestaltSystemVersion, (SInt32*)&systemVersion);
			if (err == noErr && systemVersion >= 0x01000)
				sIsMacOSX_Value = true;
			sIsMacOSX_Checked = true;
			}
		return sIsMacOSX_Value;

	#else

		return false;

	#endif
	}
