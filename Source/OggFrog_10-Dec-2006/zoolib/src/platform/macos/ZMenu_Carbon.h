/*  @(#) $Id: ZMenu_Carbon.h,v 1.3 2006/10/13 20:32:32 agreen Exp $ */

/* ------------------------------------------------------------
Copyright (c) 2001 Andrew Green and Learning in Motion, Inc.
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

#ifndef __ZMenu_Carbon__
#define __ZMenu_Carbon__ 1
#include "zconfig.h"

// =================================================================================================
#if ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)
#include "ZMenu.h"

#if ZCONFIG(OS, MacOSX)
#	include <HIToolbox/Menus.h>
#else
#	include <Menus.h>
#endif

namespace ZMenu_Carbon {

void sInstallMenuBar(const ZMenuBar& iMenuBar);
void sSetupMenuBar(const ZMenuBar& iMenuBar);

ZRef<ZMenuItem> sGetMenuItem(MenuRef iMenuRef, MenuItemIndex iMenuItemIndex);

} // namespace ZMenu_Carbon

#endif // ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)
// =================================================================================================

#endif // __ZMenu_Carbon__
