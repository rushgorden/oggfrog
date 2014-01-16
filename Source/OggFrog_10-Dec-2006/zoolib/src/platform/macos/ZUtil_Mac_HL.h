/*  @(#) $Id: ZUtil_Mac_HL.h,v 1.8 2006/07/15 20:54:22 goingware Exp $ */

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

#ifndef __ZUtil_Mac_HL__
#define __ZUtil_Mac_HL__ 1
#include "zconfig.h"

#include "ZDCPixmap.h"

#if (ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)) && ZCONFIG(API_Graphics, QD)

#if ZCONFIG(OS, MacOSX)
#	include <HIToolbox/Dialogs.h>
#	include <HIToolbox/Events.h>
#else
#	include <Dialogs.h> // For DialogPtr
#	include <Events.h> // For EventRecord
#endif

#include <string>

class ZDCPattern;

namespace ZUtil_Mac_HL {

// =================================================================================================

bool sInteractWithUser();
void sPreDialog();
void sPostDialog();

pascal Boolean sModalFilter(DialogPtr theDialog, EventRecord* inOutEventRecord, short* inOutItemHit);

string sGetVersionString();

// =================================================================================================

void sPixmapsFromCIconHandle(CIconHandle inHandle, ZDCPixmap* outColorPixmap, ZDCPixmap* outMonoPixmap, ZDCPixmap* outMaskPixmap);
CIconHandle sCIconHandleFromPixmapCombo(const ZDCPixmapCombo& inPixmapCombo);
CIconHandle sCIconHandleFromPixmaps(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap);

IconFamilyHandle sNewIconFamily();
void sDisposeIconFamily(IconFamilyHandle inIconFamilyHandle);

enum IconSize { eHuge, eLarge, eSmall, eMini };
enum IconKind { eData1, eData4, eData8, eData32, eMask1, eMask8 };

void sAddPixmapToIconFamily(const ZDCPixmap& inPixmap, IconKind inAsIconKind, IconFamilyHandle inIconFamilyHandle);
ZDCPixmap sPixmapFromIconFamily(IconFamilyHandle inIconFamilyHandle, IconSize inIconSize, IconKind inIconKind);

ZDCPixmap sPixmapFromIconSuite(IconSuiteRef inIconSuite, IconSize inIconSize, IconKind inIconKind);

ZDCPixmap sPixmapFromIconDataHandle(Handle inIconDataHandle, IconSize inIconSize, IconKind inIconKind);

IconRef sIconRefFromPixmaps(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap);

// =================================================================================================

} // namespace ZUtil_Mac_HL

#endif // (ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)) && ZCONFIG(API_Graphics, QD)

// =================================================================================================

#if ZCONFIG(API_Graphics, QD)

class ZStreamR;

namespace ZUtil_Mac_HL {

ZDCPixmap sPixmapFromStreamPICT(const ZStreamR& inStream);

} // namespace ZUtil_Mac_HL

#endif // ZCONFIG(API_Graphics, QD)

// =================================================================================================

#endif // __ZUtil_Mac_HL__
