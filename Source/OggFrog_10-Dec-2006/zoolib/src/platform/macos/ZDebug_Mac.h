/*  @(#) $Id: ZDebug_Mac.h,v 1.2 2006/07/12 19:38:02 agreen Exp $ */

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

#ifndef __ZDebug_Mac__
#define __ZDebug_Mac__ 1
#include "zconfig.h"

#include "ZDebug.h"

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
// This flag should be set when code is executing such that it would be
// unsafe to invoke a high level debugger, ie at deferred task or interrupt
// time. For example, ZFiber sets this when executing deferred task threads.
extern bool sDebug_InvokeLowLevelDebugger;


#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

#endif // __ZDebug_Mac__
