/*  @(#) $Id: ZUtil_UI.h,v 1.4 2003/04/20 21:40:35 agreen Exp $ */

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

#ifndef __ZUtil_UI__
#define __ZUtil_UI__ 1
#include "zconfig.h"

#include "ZAsset.h"
#include "ZCursor.h"
#include "ZDCPixmap.h"

namespace ZUtil_UI {

void sInitializeUIFactories();
void sTearDownUIFactories();

ZDCPixmapCombo sGetPixmapCombo(const ZAsset& inAsset);
ZDCPixmap sGetPixmap(const ZAsset& inAsset);
ZCursor sGetCursor(const ZAsset& inAsset);

string sGetString(const ZAsset& inAsset);
string sGetString(const ZAsset& inAsset, const char* inChildName);

string sGetString(const ZAsset& inAsset, size_t inIndex);

} // namsepace ZUtil_UI

#endif // __ZUtil_UI__
