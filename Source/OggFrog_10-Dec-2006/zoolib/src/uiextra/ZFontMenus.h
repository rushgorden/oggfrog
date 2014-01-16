/*  @(#) $Id: ZFontMenus.h,v 1.1.1.1 2002/02/17 18:39:53 agreen Exp $ */

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

#ifndef __ZFontMenus__
#define __ZFontMenus__ 1
#include "zconfig.h"

#include "ZMenu.h"
#include "ZDCFont.h"

namespace ZFontMenus {

void sMakeFontNames(vector<string>& outFontNames);
void sMakeFontItems(vector<ZRef<ZMenuItem> >& outMenuItems);
void sAppendFonts(ZRef<ZMenu>& inMenu);

ZRef<ZMenu> sMakeFontMenu();
ZRef<ZMenu> sMakeSizeMenu();
ZRef<ZMenu> sMakeStyleMenu();

typedef void (*CallbackProc)(size_t inCurrentIndex, const string& inFontName, void* inRefCon);
void sEnumerateFontNames(CallbackProc inCallbackProc, void* inRefCon);

void sSetupMenus(ZMenuSetup* inMenuSetup, const vector<ZDCFont>& inFonts);

} // namespace ZFontMenus

#endif // __ZFontMenus__
