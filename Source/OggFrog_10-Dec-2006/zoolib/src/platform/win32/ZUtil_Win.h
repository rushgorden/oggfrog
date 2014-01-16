/*  @(#) $Id: ZUtil_Win.h,v 1.6 2005/11/15 19:23:10 agreen Exp $ */

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

#ifndef __ZUtil_Win__
#define __ZUtil_Win__
#include "zconfig.h"

#if ZCONFIG(OS, Win32)

#include "ZWinHeader.h"

class ZDCPixmap;

namespace ZUtil_Win {

void sPixmapsFromHICON(HICON iHICON, ZDCPixmap* oColorPixmap, ZDCPixmap* oMonoPixmap, ZDCPixmap* oMaskPixmap);
bool sDragFullWindows();

HBITMAP sLoadBitmapID(bool iFromApp, int iResourceID);
HICON sLoadIconID(bool iFromApp, int iResourceID);

bool sIsWinNT();
bool sIsWin95OSR2();
bool sUseWAPI();

void sDisallowWAPI();
} // namespace ZUtil_Win

#endif //  ZCONFIG(OS, Win32)

#endif // __ZUtil_Win__
