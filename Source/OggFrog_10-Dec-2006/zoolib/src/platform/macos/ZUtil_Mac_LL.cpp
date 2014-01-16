static const char rcsid[] = "@(#) $Id: ZUtil_Mac_LL.cpp,v 1.13 2006/07/23 22:07:26 agreen Exp $";

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

#include "ZUtil_Mac_LL.h"
#include "ZDC_QD.h"
#include "ZDebug.h"
#include "ZMacOSX.h"
#include "ZMemory.h"

#include <new> // for bad_alloc

#define kDebug_Mac 3

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/Resources.h>
#endif

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#	include <Resources.h>
#endif

// =================================================================================================

#if ZCONFIG(API_Graphics, QD)

void ZUtil_Mac_LL::sSetupPixMapColor(ZPoint inSize, int32 inDepth, PixMap& oPixMap)
	{
	ZAssert(inDepth == 16 || inDepth == 24 || inDepth == 32);

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
	ZAssert(inDepth != 24);
#endif

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = (((inSize.h * inDepth + 31) / 32) * 4) | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = inSize.h;
	oPixMap.bounds.bottom = inSize.v;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = RGBDirect;
	oPixMap.cmpCount = 3;
	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandle(sizeof(ColorTable)));
//	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandle(sizeof(ColorTable) - sizeof(CSpecArray)));
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
	oPixMap.pmTable[0]->ctFlags = 0;
	oPixMap.pmTable[0]->ctSize = 0;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	switch (inDepth)
		{
		case 16:
			{
			oPixMap.cmpSize = 5;
			oPixMap.pmTable[0]->ctSeed = 15;
			break;
			}
		case 24:
			{
			oPixMap.cmpSize = 8;
			oPixMap.pmTable[0]->ctSeed = 24;
			break;
			}
		case 32:
			{
			oPixMap.cmpSize = 8;
			oPixMap.pmTable[0]->ctSeed = 32;
			break;
			}
		}
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetupPixMapColor(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap)
	{
	ZAssert(inDepth == 16 || inDepth == 24 || inDepth == 32);

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
	ZAssert(inDepth != 24);
#endif

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = inRowBytes | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = (inRowBytes * 8) / inDepth;
	oPixMap.bounds.bottom = inHeight;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = RGBDirect;
	oPixMap.cmpCount = 3;
	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandle(sizeof(ColorTable)));
//	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandle(sizeof(ColorTable) - sizeof(CSpecArray)));
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
	oPixMap.pmTable[0]->ctFlags = 0;
	oPixMap.pmTable[0]->ctSize = 0;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	switch (inDepth)
		{
		case 16:
			{
			oPixMap.cmpSize = 5;
			oPixMap.pmTable[0]->ctSeed = 15;
			break;
			}
		case 24:
			{
			oPixMap.cmpSize = 8;
			oPixMap.pmTable[0]->ctSeed = 24;
			break;
			}
		case 32:
			{
			oPixMap.cmpSize = 8;
			oPixMap.pmTable[0]->ctSeed = 32;
			break;
			}
		}
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetupPixMapGray(ZPoint inSize, int32 inDepth, PixMap& oPixMap)
	{
	ZAssert(inDepth == 1 || inDepth == 2 || inDepth == 4 || inDepth == 8);

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = (((inSize.h * inDepth + 31) / 32) * 4) | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = inSize.h;
	oPixMap.bounds.bottom = inSize.v;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = 0;
	oPixMap.cmpCount = 1;
	oPixMap.cmpSize = inDepth;
	oPixMap.pmTable = ::GetCTable(64 + inDepth);
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
	oPixMap.pmTable[0]->ctFlags = 0;
//	oPixMap.pmTable[0]->ctSize = 0;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetupPixMapGray(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap)
	{
	ZAssert(inDepth == 1 || inDepth == 2 || inDepth == 4 || inDepth == 8);

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = inRowBytes | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = (inRowBytes * 8) / inDepth;
	oPixMap.bounds.bottom = inHeight;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = 0;
	oPixMap.cmpCount = 1;
	oPixMap.cmpSize = inDepth;
	oPixMap.pmTable = ::GetCTable(64 + inDepth);
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
// AG 2001-12-12. Not Okay? Needed? --> oPixMap.pmTable[0]->ctFlags = 0;
//	oPixMap.pmTable[0]->ctSize = 0;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetupPixMapIndexed(ZPoint inSize, int32 inDepth, PixMap& oPixMap)
	{
	ZAssert(inDepth == 1 || inDepth == 2 || inDepth == 4 || inDepth == 8);

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = (((inSize.h * inDepth + 31) / 32) * 4) | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = inSize.h;
	oPixMap.bounds.bottom = inSize.v;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = 0;
	oPixMap.cmpCount = 1;
	oPixMap.cmpSize = inDepth;

	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandleClear(sizeof(ColorTable) + ((1 << inDepth) - 1) * sizeof(CSpecArray)));
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
	oPixMap.pmTable[0]->ctSeed = ::GetCTSeed();
	oPixMap.pmTable[0]->ctFlags = 0;
	oPixMap.pmTable[0]->ctSize = (1 << inDepth) - 1;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetupPixMapIndexed(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap)
	{
	ZAssert(inDepth == 1 || inDepth == 2 || inDepth == 4 || inDepth == 8);

	oPixMap.baseAddr = nil;
	oPixMap.pmTable = nil;

	oPixMap.rowBytes = inRowBytes | 0x8000;

	oPixMap.bounds.left = 0;
	oPixMap.bounds.top = 0;
	oPixMap.bounds.right = (inRowBytes * 8) / inDepth;
	oPixMap.bounds.bottom = inHeight;
	oPixMap.pmVersion = baseAddr32;
	oPixMap.packType = 0;
	oPixMap.packSize = 0;
	oPixMap.hRes = 72<<16; // Fixed point
	oPixMap.vRes = 72<<16; // Fixed point
	oPixMap.pixelSize = inDepth;
	oPixMap.pixelType = 0;
	oPixMap.cmpCount = 1;
	oPixMap.cmpSize = inDepth;

	oPixMap.pmTable = reinterpret_cast<CTabHandle>(::NewHandleClear(sizeof(ColorTable) + ((1 << inDepth) - 1) * sizeof(CSpecArray)));
	if (oPixMap.pmTable == nil)
		throw bad_alloc();
	oPixMap.pmTable[0]->ctSeed = ::GetCTSeed();
	oPixMap.pmTable[0]->ctFlags = 0;
	oPixMap.pmTable[0]->ctSize = (1 << inDepth) - 1;

#if OLDPIXMAPSTRUCT
	oPixMap.planeBytes = 0;
	oPixMap.pmReserved = 0;
#else // OLDPIXMAPSTRUCT
	oPixMap.pixelFormat = inDepth;
	oPixMap.pmExt = nil;
#endif // OLDPIXMAPSTRUCT
	}

// =================================================================================================

void ZUtil_Mac_LL::sPixmapFromPixPatHandle(PixPatHandle inHandle, ZDCPixmap* outColorPixmap, ZDCPattern* outPattern)
	{
	ZUtil_Mac_LL::HandleLocker locker1((Handle)inHandle);
	if (outColorPixmap)
		{
		ZUtil_Mac_LL::HandleLocker locker2((Handle)inHandle[0]->patMap);
		ZUtil_Mac_LL::HandleLocker locker3((Handle)inHandle[0]->patData);
		*outColorPixmap = ZDCPixmap(new ZDCPixmapRep_QD(inHandle[0]->patMap[0], *inHandle[0]->patData, inHandle[0]->patMap[0]->bounds));
		}
	if (outPattern)
		{
		*outPattern = *reinterpret_cast<ZDCPattern*>(&inHandle[0]->pat1Data);
		}
	}

// =================================================================================================

static short sClosestPowerOfTwo(short inValue)
	{
	ZAssertStop(kDebug_Mac, inValue > 0 && inValue <= 0x4000);

	short thePower = 1;
	while (thePower <= inValue)
		thePower <<= 1;
	return thePower >> 1;
	}

PixPatHandle ZUtil_Mac_LL::sPixPatHandleFromPixmap(const ZDCPixmap& inPixmap)
	{
	ZRef<ZDCPixmapRep_QD> thePixmapRep_QD = ZDCPixmapRep_QD::sAsPixmapRep_QD(inPixmap.GetRep());
	const PixMap& sourcePixMap = thePixmapRep_QD->GetPixMap();

	PixPatHandle thePixPatHandle = ::NewPixPat();

	thePixPatHandle[0]->patMap[0]->rowBytes = sourcePixMap.rowBytes;
	thePixPatHandle[0]->patMap[0]->bounds = sourcePixMap.bounds;
	thePixPatHandle[0]->patMap[0]->pmVersion = sourcePixMap.pmVersion;
	thePixPatHandle[0]->patMap[0]->packType = sourcePixMap.packType;
	thePixPatHandle[0]->patMap[0]->packSize = sourcePixMap.packSize;
	thePixPatHandle[0]->patMap[0]->hRes = sourcePixMap.hRes;
	thePixPatHandle[0]->patMap[0]->vRes = sourcePixMap.vRes;
	thePixPatHandle[0]->patMap[0]->pixelType = sourcePixMap.pixelType;
	thePixPatHandle[0]->patMap[0]->pixelSize = sourcePixMap.pixelSize;
	thePixPatHandle[0]->patMap[0]->cmpCount = sourcePixMap.cmpCount;
	thePixPatHandle[0]->patMap[0]->cmpSize = sourcePixMap.cmpSize;
	#if OLDPIXMAPSTRUCT
	thePixPatHandle[0]->patMap[0]->planeBytes = sourcePixMap.planeBytes;
	thePixPatHandle[0]->patMap[0]->pmReserved = sourcePixMap.pmReserved;
	#else
	thePixPatHandle[0]->patMap[0]->pixelFormat = sourcePixMap.pixelFormat;
	thePixPatHandle[0]->patMap[0]->pmExt = sourcePixMap.pmExt;
	#endif

	// Copy the color table
	size_t cTabSize = ::GetHandleSize((Handle)sourcePixMap.pmTable);
	::SetHandleSize((Handle)thePixPatHandle[0]->patMap[0]->pmTable, cTabSize);
	ZBlockCopy(sourcePixMap.pmTable[0], thePixPatHandle[0]->patMap[0]->pmTable[0], cTabSize);

	// And all the pixel data
	size_t pixelDataSize = (sourcePixMap.rowBytes & 0x3FFF) * sourcePixMap.bounds.bottom;
	::SetHandleSize((Handle)thePixPatHandle[0]->patData, pixelDataSize);
	ZBlockCopy(sourcePixMap.baseAddr, thePixPatHandle[0]->patData[0], pixelDataSize);

	::PixPatChanged(thePixPatHandle);

	return thePixPatHandle;
	}

// =================================================================================================

ZDCPixmapNS::PixelDesc ZUtil_Mac_LL::sCTabHandleToPixelDesc(CTabHandle inCTabHandle)
	{
	ZUtil_Mac_LL::HandleLocker locker1((Handle)inCTabHandle);
	ColorSpec* curColorSpec = &inCTabHandle[0]->ctTable[0];
	vector<ZRGBColorPOD> theColors(inCTabHandle[0]->ctSize + 1);
	for (vector<ZRGBColorPOD>::iterator i = theColors.begin(); i != theColors.end(); ++i, ++curColorSpec)
		{
		(*i).red = curColorSpec->rgb.red;
		(*i).green = curColorSpec->rgb.green;
		(*i).blue = curColorSpec->rgb.blue;
		(*i).alpha = 0xFFFFU;
		}
	return ZDCPixmapNS::PixelDesc(&theColors[0], theColors.size());
	}

// =================================================================================================

void ZUtil_Mac_LL::sSetWindowManagerPort()
	{
	#if ZCONFIG(OS, MacOS7)

		CGrafPtr windowManagerPort;
		::GetCWMgrPort(&windowManagerPort);
		::SetGWorld(windowManagerPort, nil);

	#elif ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

		if (ZMacOSX::sIsMacOSX())
			{
			::SetPort(nil);
			}
		else
			{
			typedef pascal void (*GetCWMgrPortProcPtr) (CGrafPtr * wMgrCPort);

			static GetCWMgrPortProcPtr sGetCWMgrPortProcPtr = nil;
			static bool sChecked = false;

			if (!sChecked)
				{
				CFragConnectionID connID = kInvalidID;
				if (noErr == ::GetSharedLibrary("\pInterfaceLib", kCompiledCFragArch, kReferenceCFrag, &connID, nil, nil))
					::FindSymbol(connID, "\pGetCWMgrPort", reinterpret_cast<Ptr*>(&sGetCWMgrPortProcPtr), nil);
				sChecked = true;
				}

			if (sGetCWMgrPortProcPtr)
				{
				CGrafPtr windowManagerPort;
				sGetCWMgrPortProcPtr(&windowManagerPort);
				::SetGWorld(windowManagerPort, nil);
				}
			}

	#endif
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::RefCountedCTabHandle

ZUtil_Mac_LL::RefCountedCTabHandle::~RefCountedCTabHandle()
	{
	if (fCTabHandle)
		::DisposeHandle((Handle)fCTabHandle);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveRestorePort

ZUtil_Mac_LL::SaveRestorePort::SaveRestorePort()
	{
	::GetGWorld(&fPreConstruct_GrafPtr, &fPreConstruct_GDHandle);
	}

ZUtil_Mac_LL::SaveRestorePort::~SaveRestorePort()
	{
	::SetGWorld(fPreConstruct_GrafPtr, fPreConstruct_GDHandle);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveSetBlackWhite

ZUtil_Mac_LL::SaveSetBlackWhite::SaveSetBlackWhite()
	{
	::GetForeColor(&fPreConstruct_ForeColor);
	::GetBackColor(&fPreConstruct_BackColor);
	::ForeColor(blackColor);
	::BackColor(whiteColor);
	}

ZUtil_Mac_LL::SaveSetBlackWhite::~SaveSetBlackWhite()
	{
	::RGBForeColor(&fPreConstruct_ForeColor);
	::RGBBackColor(&fPreConstruct_BackColor);
	}

#endif // ZCONFIG(API_Graphics, QD)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::PreserveCurrentPort

#if ZCONFIG(API_Thread, Mac)

ZUtil_Mac_LL::PreserveCurrentPort::PreserveCurrentPort()
	{
	fPreConstruct_AllowTimeSlice = ::ZThreadTM_SetCurrentAllowTimeSlice(false);

	fSwitchInfo.fSwitchProc = sSwitchProc;
	fSwitchInfo.fPrev = nil;
	fSwitchInfo.fNext = nil;
	::ZThreadTM_InstallSwitchCallback(&fSwitchInfo);
	}

ZUtil_Mac_LL::PreserveCurrentPort::~PreserveCurrentPort()
	{
	::ZThreadTM_RemoveSwitchCallback(&fSwitchInfo);

	::ZThreadTM_SetCurrentAllowTimeSlice(fPreConstruct_AllowTimeSlice);
	}

void ZUtil_Mac_LL::PreserveCurrentPort::sSwitchProc(bool inSwitchingIn, ZThreadTM_SwitchInfo* inSwitchInfo)
	{
	SwitchInfo* ourSwitchInfo = static_cast<SwitchInfo*>(inSwitchInfo);
	if (inSwitchingIn)
		::SetGWorld(ourSwitchInfo->fPreSwitch_GrafPtr, ourSwitchInfo->fPreSwitch_GDHandle);
	else
		::GetGWorld(&ourSwitchInfo->fPreSwitch_GrafPtr, &ourSwitchInfo->fPreSwitch_GDHandle);
	}

#endif // ZCONFIG(API_Thread, Mac)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::PreserveResFile

#if ZCONFIG(API_Thread, Mac)

ZUtil_Mac_LL::PreserveResFile::PreserveResFile()
	{
	if (ZMacOSX::sIsMacOSX())
		return;

	fPreConstruct_AllowTimeSlice = ::ZThreadTM_SetCurrentAllowTimeSlice(false);

	fSwitchInfo.fSwitchProc = sSwitchProc;
	fSwitchInfo.fPrev = nil;
	fSwitchInfo.fNext = nil;
	::ZThreadTM_InstallSwitchCallback(&fSwitchInfo);
	}

ZUtil_Mac_LL::PreserveResFile::~PreserveResFile()
	{
	if (ZMacOSX::sIsMacOSX())
		return;

	::ZThreadTM_RemoveSwitchCallback(&fSwitchInfo);

	::ZThreadTM_SetCurrentAllowTimeSlice(fPreConstruct_AllowTimeSlice);
	}

void ZUtil_Mac_LL::PreserveResFile::sSwitchProc(bool inSwitchingIn, ZThreadTM_SwitchInfo* inSwitchInfo)
	{
	SwitchInfo* ourSwitchInfo = static_cast<SwitchInfo*>(inSwitchInfo);
	if (inSwitchingIn)
		::UseResFile(ourSwitchInfo->fPreSwitch_ResFile);
	else
		ourSwitchInfo->fPreSwitch_ResFile = ::CurResFile();
	}

#endif // ZCONFIG(API_Thread, Mac)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveRestoreResFile

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

ZUtil_Mac_LL::SaveRestoreResFile::SaveRestoreResFile()
	{
	fPreConstruct_ResFile = ::CurResFile();
	}

ZUtil_Mac_LL::SaveRestoreResFile::SaveRestoreResFile(short iResFile)
	{
	fPreConstruct_ResFile = ::CurResFile();
	if (iResFile != -1)
		::UseResFile(iResFile);
	}

ZUtil_Mac_LL::SaveRestoreResFile::~SaveRestoreResFile()
	{
	if (fPreConstruct_ResFile != -1)
		::UseResFile(fPreConstruct_ResFile);
	}

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
