/*  @(#) $Id: ZUtil_Mac_LL.h,v 1.10 2006/07/23 22:07:26 agreen Exp $ */

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

#ifndef __ZUtil_Mac_LL__
#define __ZUtil_Mac_LL__ 1
#include "zconfig.h"

#if ZCONFIG(API_Graphics, QD) || ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

#include "ZDCPixmapNS.h" 
#include "ZGeom.h"
#include "ZRefCount.h"

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/MacMemory.h>
#	include <QD/QDOffscreen.h>
#else
#	include <MacMemory.h>
#	include <QDOffscreen.h>
#endif

#endif // ZCONFIG(API_Graphics, QD) || ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)


#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

#include "ZThreadTM.h"
#include "ZTypes.h"

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

class ZDCPixmap;
class ZDCPattern;

namespace ZUtil_Mac_LL {

// =================================================================================================

#if ZCONFIG(API_Graphics, QD)

void sSetupPixMapColor(ZPoint inSize, int32 inDepth, PixMap& oPixMap);
void sSetupPixMapColor(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap);

void sSetupPixMapGray(ZPoint inSize, int32 inDepth, PixMap& oPixMap);
void sSetupPixMapGray(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap);

void sSetupPixMapIndexed(ZPoint inSize, int32 inDepth, PixMap& oPixMap);
void sSetupPixMapIndexed(size_t inRowBytes, int32 inDepth, int32 inHeight, PixMap& oPixMap);

void sPixmapFromPixPatHandle(PixPatHandle inHandle, ZDCPixmap* outColorPixmap, ZDCPattern* outPattern);

PixPatHandle sPixPatHandleFromPixmap(const ZDCPixmap& inPixmap);

ZDCPixmapNS::PixelDesc sCTabHandleToPixelDesc(CTabHandle inCTabHandle);

void sSetWindowManagerPort();

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::RefCountedCTabHandle

class RefCountedCTabHandle : public ZRefCounted
	{
public:
	RefCountedCTabHandle(CTabHandle inCTabHandle) : fCTabHandle(inCTabHandle) {}
	virtual ~RefCountedCTabHandle();

	CTabHandle GetCTabHandle() { return fCTabHandle; }

private:
	CTabHandle fCTabHandle;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveRestorePort

class SaveRestorePort
	{
public:
	SaveRestorePort();
	~SaveRestorePort();

protected:
	CGrafPtr fPreConstruct_GrafPtr;
	GDHandle fPreConstruct_GDHandle;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveSetBlackWhite

class SaveSetBlackWhite
	{
public:
	SaveSetBlackWhite();
	~SaveSetBlackWhite();

protected:
	RGBColor fPreConstruct_ForeColor;
	RGBColor fPreConstruct_BackColor;
	};

#endif // ZCONFIG(API_Graphics, QD)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::PreserveCurrentPort

#if ZCONFIG(API_Thread, Mac)

class PreserveCurrentPort
	{
public:
	PreserveCurrentPort();
	~PreserveCurrentPort();

protected:
	static void sSwitchProc(bool inSwitchingIn, ZThreadTM_SwitchInfo* inSwitchInfo);
	struct SwitchInfo : ZThreadTM_SwitchInfo
		{
		CGrafPtr fPreSwitch_GrafPtr;
		GDHandle fPreSwitch_GDHandle;
		};
	SwitchInfo fSwitchInfo;
	bool fPreConstruct_AllowTimeSlice;
	};

#endif // ZCONFIG(API_Thread, Mac)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::PreserveResFile

#if ZCONFIG(API_Thread, Mac)

class PreserveResFile
	{
public:
	PreserveResFile();
	~PreserveResFile();

protected:
	static void sSwitchProc(bool inSwitchingIn, ZThreadTM_SwitchInfo* inSwitchInfo);
	struct SwitchInfo : ZThreadTM_SwitchInfo
		{
		short fPreSwitch_ResFile;
		};
	SwitchInfo fSwitchInfo;
	bool fPreConstruct_AllowTimeSlice;
	};

#endif // ZCONFIG(API_Thread, Mac)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::SaveRestoreResFile

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

class SaveRestoreResFile
	{
public:
	SaveRestoreResFile();
	SaveRestoreResFile(short iResFile);
	~SaveRestoreResFile();

protected:
	short fPreConstruct_ResFile;
	};

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================
#pragma mark -
#pragma mark * ZUtil_Mac_LL::HandleLocker

#if ZCONFIG(API_Graphics, QD) || ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

class HandleLocker
	{
public:
	HandleLocker(Handle inHandle) : fHandle(inHandle) { this->Internal_Adopt(); }
	~HandleLocker() { this->Internal_Orphan(); }

	void Adopt(Handle inHandle) { this->Internal_Orphan(); fHandle = inHandle; this->Internal_Adopt(); }
	void Orphan() { this->Internal_Orphan(); }

protected:
	void Internal_Adopt()
		{
		if (fHandle)
			{
			fOldState = ::HGetState(fHandle);
			::HLock(fHandle);
			}
		}

	void Internal_Orphan()
		{
		if (fHandle)
			::HSetState(fHandle, fOldState);
		fHandle = nil;
		}

	Handle fHandle;
	SignedByte fOldState;
	};

#endif // ZCONFIG(API_Graphics, QD) || ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)

// =================================================================================================

} // namespace ZUtil_Mac_LL

#endif // __ZUtil_Mac_LL__
