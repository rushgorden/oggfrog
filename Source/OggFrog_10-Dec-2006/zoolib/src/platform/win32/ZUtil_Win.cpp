static const char rcsid[] = "@(#) $Id: ZUtil_Win.cpp,v 1.11 2005/12/14 21:22:19 agreen Exp $";

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

#include "ZUtil_Win.h"

#if ZCONFIG(OS, Win32)

#include "ZDC_GDI.h"
#include "ZDCPixmap.h"
#include "ZDebug.h"

#define kDebug_Win 2

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& ioColor, void* iRefcon)
	{
	ioColor = ZRGBColor::sWhite - ioColor;
	return true;
	}

void ZUtil_Win::sPixmapsFromHICON(HICON iHICON, ZDCPixmap* oColorPixmap, ZDCPixmap* oMonoPixmap, ZDCPixmap* oMaskPixmap)
	{
	ZAssertStop(2, iHICON);
	ICONINFO theICONINFO;
	::GetIconInfo(iHICON, &theICONINFO);

	HDC dummyHDC = ::GetDC(nil);

	ZDCPixmap monoPixmap(new ZDCPixmapRep_DIB(dummyHDC, theICONINFO.hbmMask));

	if (theICONINFO.hbmColor)
		{
		if (oColorPixmap)
			*oColorPixmap = ZDCPixmap(new ZDCPixmapRep_DIB(dummyHDC, theICONINFO.hbmColor));
		if (oMaskPixmap)
			*oMaskPixmap = monoPixmap;
		}
	else
		{
		// theICONINFO.hbmColor is nil, so theICONINFO.hbmMask (and thus monoPixmap) contains
		// the mono pixmap and the mask stacked on top of each other.
		ZPoint monoSize = monoPixmap.Size();
		if (oMonoPixmap)
			*oMonoPixmap = ZDCPixmap(monoPixmap, ZRect(0, 0, monoSize.h, monoSize.v / 2));
		if (oMaskPixmap)
			*oMaskPixmap = ZDCPixmap(monoPixmap, ZRect(0, monoSize.v / 2, monoSize.h, monoSize.v));
		}

	if (oMaskPixmap)
		oMaskPixmap->Munge(sMungeProc_Invert, nil);

	if (theICONINFO.hbmMask)
		::DeleteObject(theICONINFO.hbmMask);
	if (theICONINFO.hbmMask)
		::DeleteObject(theICONINFO.hbmColor);

	::ReleaseDC(nil, dummyHDC);
	}

static HINSTANCE sGetAppModuleHandle()
	{
	if (ZUtil_Win::sUseWAPI())
		return ::GetModuleHandleW(0);
	else
		return ::GetModuleHandleA(0);
	}

HBITMAP ZUtil_Win::sLoadBitmapID(bool iFromApp, int iResourceID)
	{
	ZAssertStop(kDebug_Win, (iResourceID & 0xFFFF0000) == 0);
	HINSTANCE theHINSTANCE = nil;
	if (iFromApp)
		theHINSTANCE = sGetAppModuleHandle();

	if (ZUtil_Win::sUseWAPI())
		return ::LoadBitmapW(theHINSTANCE, MAKEINTRESOURCEW(iResourceID));
	else
		return ::LoadBitmapA(theHINSTANCE, MAKEINTRESOURCEA(iResourceID));
	}

HICON ZUtil_Win::sLoadIconID(bool iFromApp, int iResourceID)
	{
	ZAssertStop(kDebug_Win, (iResourceID & 0xFFFF0000) == 0);

	HINSTANCE theHINSTANCE = nil;
	if (iFromApp)
		theHINSTANCE = sGetAppModuleHandle();

	if (ZUtil_Win::sUseWAPI())
		return ::LoadIconW(theHINSTANCE, MAKEINTRESOURCEW(iResourceID));
	else
		return ::LoadIconA(theHINSTANCE, MAKEINTRESOURCEA(iResourceID));
	}

bool ZUtil_Win::sDragFullWindows()
	{
	bool dragFullWindows = false;
	HKEY keyDragFullWindows;
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_QUERY_VALUE, &keyDragFullWindows))
		{
		BYTE buffer[100];
		DWORD bufferSize = 100;
		if (ERROR_SUCCESS == ::RegQueryValueEx(keyDragFullWindows, "DragFullWindows", NULL, NULL, buffer, &bufferSize))
			{
			if (buffer[0] == '1')
				dragFullWindows = true;
			}
		::RegCloseKey(keyDragFullWindows);
		}
	return dragFullWindows;
	}

// From Whisper 1.3
static bool sIsWinNT_Inited;
static bool sIsWinNT_Result;
bool ZUtil_Win::sIsWinNT()
	{
	// We unconditionally use the 'A' API because we don't yet know
	// if we're on NT, which is what this method is trying to determine.
	// We could use the W entry point, and a failure used to indicate
	// that we know we're not on NT.
	if (!sIsWinNT_Inited)
		{
		OSVERSIONINFOA info;
		info.dwOSVersionInfoSize = sizeof(info);

		if (::GetVersionExA(&info))
			sIsWinNT_Result = info.dwPlatformId == VER_PLATFORM_WIN32_NT;

		sIsWinNT_Inited = true;
		}
	return sIsWinNT_Result;
	}

static bool sIsWin95OSR2_Inited;
static bool sIsWin95OSR2_Result;
bool ZUtil_Win::sIsWin95OSR2()
	{
	if (!sIsWin95OSR2_Inited)
		{
		OSVERSIONINFOA info;
		info.dwOSVersionInfoSize = sizeof(info);

		if (::GetVersionExA(&info))
			{
			if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
				{
				if (info.dwMinorVersion >= 10)
					sIsWin95OSR2_Result = true;
				else if (LOWORD(info.dwBuildNumber) > 1080)
					sIsWin95OSR2_Result = true;
				}
			}

		sIsWin95OSR2_Inited = true;
		}
	return sIsWin95OSR2_Result;
	}

static bool sFlag_DisallowWAPI;
bool ZUtil_Win::sUseWAPI()
	{
	if (sFlag_DisallowWAPI)
		return false;

	return sIsWinNT();
	}

void ZUtil_Win::sDisallowWAPI()
	{ sFlag_DisallowWAPI = true; }

#endif // ZCONFIG(OS, Win32)
