/*  @(#) $Id: ZMain.h,v 1.8 2006/04/10 20:44:20 agreen Exp $ */

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

#ifndef __ZMain__
#define __ZMain__ 1
#include "zconfig.h"

extern int ZMain(int argc, char** argv);

namespace ZMainNS {

extern int sArgC;
extern char** sArgV;

// =================================================================================================

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

void sInvokeAsMainThread(void (*iProc)(void*), void* iParam);

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

void sDaemonize(bool iForceFDClose = false);

// =================================================================================================

} // namespace ZMain

#endif // __ZMain__
