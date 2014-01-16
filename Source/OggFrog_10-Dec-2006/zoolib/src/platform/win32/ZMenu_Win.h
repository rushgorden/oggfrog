/*  @(#) $Id: ZMenu_Win.h,v 1.2 2006/07/12 19:39:41 agreen Exp $ */

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

#ifndef __ZMenu_Win__
#define __ZMenu_Win__ 1
#include "zconfig.h"

#if ZCONFIG(OS, Win32)
#include "ZMenu.h"

namespace ZMenu_Win {

void sHandle_MEASUREITEM(MEASUREITEMSTRUCT* inMIS);
void sHandle_DRAWITEM(DRAWITEMSTRUCT* inDIS);
LRESULT sHandle_MENUCHAR(UINT inChar, UINT inFlags, HMENU inHMENU);

HMENU sCreateHMENU(const ZMenuBar& inMenuBar);
void sPostSetup(const ZMenuBar& inMenuBar, HMENU inHMENU);
ZRef<ZMenuItem> sFindItemByCommand(HMENU inHMENU, WORD inSearchedID);
ZRef<ZMenuItem> sFindItemByAccel(const ZMenuBar& inMenuBar, DWORD inVirtualKey, bool inControlDown, bool inShiftDown, bool inAltDown);

} // namespace ZMenu_Win


#endif // ZCONFIG(OS, Win32)


#endif // __ZMenu_Win__