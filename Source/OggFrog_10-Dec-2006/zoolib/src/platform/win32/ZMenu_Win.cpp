static const char rcsid[] = "@(#) $Id: ZMenu_Win.cpp,v 1.11 2006/07/12 19:39:41 agreen Exp $";

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

#include "ZMenu_Win.h"

// =================================================================================================
#if ZCONFIG(OS, Win32)

#include "ZDC_GDI.h" // For ZDC_NativeGDI
#include "ZMemory.h" // For ZBlockSet
#include "ZOSWindow_Win.h" // For ZOSApp_Win::InvokeFunctionInMainThread
#include "ZUtil_Win.h"

static string sStripEmbeddedChars(const string& source)
	{
#if 1
	return source;
#else
	string returnedString;
	for (size_t index = 0; index < source.length(); ++index)
		{
		if (source[index] == 0xC9)
			{
			// It's a mac ellipsis
			returnedString += "...";
			continue;
			}
		returnedString += source[index];
		}
	return returnedString;
#endif
	}

static bool sSetupMenuW(ZRef<ZMenu> inMenu, HMENU inHMENU, WORD& inOutRollingID);
static bool sSetupItemW(const ZRef<ZMenuItem>& inMenuItem, HMENU inHMENU, int inPos, WORD& inOutRollingID)
	{
	MENUITEMINFOW theInfo;
	ZBlockZero(&theInfo, sizeof(theInfo));
	theInfo.cbSize = sizeof(theInfo);
	// Here we make sure to see what pre-existing sub menu may already be installed, other pre-existing
	// attributes we don't care about for the time being.
	theInfo.fMask = MIIM_SUBMENU;
	::GetMenuItemInfoW(inHMENU, inPos, true, &theInfo);

	bool isEnabled = inMenuItem->GetEnabled();
	string theItemText = inMenuItem->GetText();
	ZDCPixmapCombo thePixmapCombo = inMenuItem->GetPixmapCombo();

	if (theItemText.empty() && !thePixmapCombo)
		isEnabled = false;

	theInfo.fType = MFT_OWNERDRAW;
	theInfo.dwItemData = reinterpret_cast<DWORD>(inMenuItem.GetObject());

	theInfo.wID = inOutRollingID++;

	if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
		{
		if (theInfo.hSubMenu == nil)
			theInfo.hSubMenu = ::CreatePopupMenu();
		isEnabled = ::sSetupMenuW(theSubMenu, theInfo.hSubMenu, inOutRollingID);
		}
	else
		{
		if (theInfo.hSubMenu)
			::DestroyMenu(theInfo.hSubMenu);
		theInfo.hSubMenu = nil;
		}

	theInfo.fState = 0;
	if (!isEnabled)
		theInfo.fState |= MFS_GRAYED;
	if (inMenuItem->GetChecked())
		theInfo.fState |= MFS_CHECKED;
	theInfo.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
	::SetMenuItemInfoW(inHMENU, inPos, true, &theInfo);

	return isEnabled;
	}

static bool sSetupMenuA(ZRef<ZMenu> inMenu, HMENU inHMENU, WORD& inOutRollingID);
static bool sSetupItemA(const ZRef<ZMenuItem>& inMenuItem, HMENU inHMENU, int inPos, WORD& inOutRollingID)
	{
	MENUITEMINFOA theInfo;
	ZBlockZero(&theInfo, sizeof(theInfo));
	theInfo.cbSize = sizeof(theInfo);
	// Here we make sure to see what pre-existing sub menu may already be installed, other pre-existing
	// attributes we don't care about for the time being.
	theInfo.fMask = MIIM_SUBMENU;
	::GetMenuItemInfoA(inHMENU, inPos, true, &theInfo);

	bool isEnabled = inMenuItem->GetEnabled();
	string theItemText = inMenuItem->GetText();
	ZDCPixmapCombo thePixmapCombo = inMenuItem->GetPixmapCombo();

	if (theItemText.empty() && !thePixmapCombo)
		isEnabled = false;

	theInfo.fType = MFT_OWNERDRAW;
	theInfo.dwItemData = reinterpret_cast<DWORD>(inMenuItem.GetObject());

	theInfo.wID = inOutRollingID++;

	if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
		{
		if (theInfo.hSubMenu == nil)
			theInfo.hSubMenu = ::CreatePopupMenu();
		isEnabled = ::sSetupMenuA(theSubMenu, theInfo.hSubMenu, inOutRollingID);
		}
	else
		{
		if (theInfo.hSubMenu)
			::DestroyMenu(theInfo.hSubMenu);
		theInfo.hSubMenu = nil;
		}

	theInfo.fState = 0;
	if (!isEnabled)
		theInfo.fState |= MFS_GRAYED;
	if (inMenuItem->GetChecked())
		theInfo.fState |= MFS_CHECKED;
	theInfo.fMask = MIIM_SUBMENU | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
	::SetMenuItemInfoA(inHMENU, inPos, true, &theInfo);

	return isEnabled;
	}

static bool sSetupMenuW(ZRef<ZMenu> inMenu, HMENU inHMENU, WORD& inOutRollingID)
	{
	bool anyEnabled = false;
	short currentCount = ::GetMenuItemCount(inHMENU);

	while (inMenu->Count() > currentCount)
		{
		::AppendMenuW(inHMENU, MF_STRING, 0, L"");
		++currentCount;
		}
	while (inMenu->Count() < currentCount)
		{
		::DeleteMenu(inHMENU, currentCount, MF_BYPOSITION);
		--currentCount;
		}

	for (size_t x = 0; x < inMenu->Count(); ++x)
		{
		if (::sSetupItemW(inMenu->At(x), inHMENU, x, inOutRollingID))
			anyEnabled = true;
		}
	return anyEnabled;
	}

static bool sSetupMenuA(ZRef<ZMenu> inMenu, HMENU inHMENU, WORD& inOutRollingID)
	{
	bool anyEnabled = false;
	short currentCount = ::GetMenuItemCount(inHMENU);

	while (inMenu->Count() > currentCount)
		{
		::AppendMenuA(inHMENU, MF_STRING, 0, "");
		++currentCount;
		}
	while (inMenu->Count() < currentCount)
		{
		::DeleteMenu(inHMENU, currentCount, MF_BYPOSITION);
		--currentCount;
		}

	for (size_t x = 0; x < inMenu->Count(); ++x)
		{
		if (::sSetupItemA(inMenu->At(x), inHMENU, x, inOutRollingID))
			anyEnabled = true;
		}
	return anyEnabled;
	}

static ZRef<ZMenuItem> sFindItemByAccel(const ZRef<ZMenu>& inMenu, DWORD inVirtualKey, bool inControlDown, bool inShiftDown, bool inAltDown)
	{
	for (size_t x = 0; x < inMenu->Count(); ++x)
		{
		ZRef<ZMenuItem> currentItem = inMenu->At(x);
		ZMenuAccel theAccel = currentItem->GetAccel();
		if (theAccel.fKeyCode == inVirtualKey)
			{
			bool accelHasControl = (0 != (theAccel.fModifiers & (ZMenuAccel::eCommandKey | ZMenuAccel::eControlKeyWin)));
			bool accelHasShift = (0 != (theAccel.fModifiers & (ZMenuAccel::eShiftKey | ZMenuAccel::eShiftKeyWin)));
			bool accelHasAlt = (0 != (theAccel.fModifiers & (ZMenuAccel::eOptionKey | ZMenuAccel::eAltKeyWin)));
			if (inControlDown == accelHasControl && inShiftDown == accelHasShift && inAltDown == accelHasAlt)
				return currentItem;
			}
		if (ZRef<ZMenu> theSubMenu = currentItem->GetSubMenu())
			{
			if (ZRef<ZMenuItem> theMenuItem = sFindItemByAccel(theSubMenu, inVirtualKey, inControlDown, inShiftDown, inAltDown))
				return theMenuItem;
			}
		}
	return ZRef<ZMenuItem>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu_Win

HMENU ZMenu_Win::sCreateHMENU(const ZMenuBar& inMenuBar)
	{
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	const vector<string>& titleVector = inMenuBar.GetTitleVector();

	ZAssertLogf(0, menuVector.size() == titleVector.size(), ("menuVector.size() == %d, titleVector.size() == %d", menuVector.size(), titleVector.size()));

	HMENU newHMENU = ::CreateMenu();
	if (ZUtil_Win::sUseWAPI())
		{
		for (size_t x = 0; x < titleVector.size(); ++x)
			::AppendMenuW(newHMENU, MF_STRING | MF_POPUP, (UINT)::CreatePopupMenu(), ZUnicode::sAsUTF16(titleVector[x]).c_str());
		}
	else
		{
		for (size_t x = 0; x < titleVector.size(); ++x)
			::AppendMenuA(newHMENU, MF_STRING | MF_POPUP, (UINT)::CreatePopupMenu(), titleVector[x].c_str());
		}
	return newHMENU;
	}

void ZMenu_Win::sPostSetup(const ZMenuBar& inMenuBar, HMENU inHMENU)
	{
// Check to see if the last item in the HMENU has ID SC_RESTORE or SC_CLOSE, if so then the HMENU has
// been augmented with the MDI widgets, and our first menu item is not at index 0 but at index 1.
	size_t mdiFixup = 0;
	UINT lastID = ::GetMenuItemID(inHMENU, ::GetMenuItemCount(inHMENU) - 1);
	if (lastID == SC_RESTORE || lastID == SC_CLOSE)
		mdiFixup = 1;

	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();

	WORD theRollingID = 1;
	for (size_t x = 0; x < menuVector.size(); ++x)
		{
		HMENU theSubMenu = ::GetSubMenu(inHMENU, x + mdiFixup);
		if (theSubMenu)
			{
			if (ZUtil_Win::sUseWAPI())
				::sSetupMenuW(menuVector[x], theSubMenu, theRollingID);
			else
				::sSetupMenuA(menuVector[x], theSubMenu, theRollingID);
			}
		}
	}

ZRef<ZMenuItem> ZMenu_Win::sFindItemByCommand(HMENU inHMENU, WORD inSearchedID)
	{
	if (ZUtil_Win::sUseWAPI())
		{
		MENUITEMINFOW theInfo;
		ZBlockZero(&theInfo, sizeof(theInfo));
		theInfo.cbSize = sizeof(theInfo);
		theInfo.fMask = MIIM_DATA | MIIM_ID;
		if (!::GetMenuItemInfoW(inHMENU, inSearchedID, false, &theInfo))
			return ZRef<ZMenuItem>();
		ZAssert(theInfo.wID == inSearchedID);
		return reinterpret_cast<ZMenuItem*>(theInfo.dwItemData);
		}
	else
		{
		MENUITEMINFOA theInfo;
		ZBlockZero(&theInfo, sizeof(theInfo));
		theInfo.cbSize = sizeof(theInfo);
		theInfo.fMask = MIIM_DATA | MIIM_ID;
		if (!::GetMenuItemInfoA(inHMENU, inSearchedID, false, &theInfo))
			return ZRef<ZMenuItem>();
		ZAssert(theInfo.wID == inSearchedID);
		return reinterpret_cast<ZMenuItem*>(theInfo.dwItemData);
		}
	}


ZRef<ZMenuItem> ZMenu_Win::sFindItemByAccel(const ZMenuBar& inMenuBar, DWORD inVirtualKey, bool inControlDown, bool inShiftDown, bool inAltDown)
	{
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	for (size_t x = 0; x < menuVector.size(); ++x)
		{
		if (ZRef<ZMenuItem> theMenuItem = ::sFindItemByAccel(menuVector[x], inVirtualKey, inControlDown, inShiftDown, inAltDown))
			{
			if (theMenuItem->GetEnabled())
				return theMenuItem;
			return ZRef<ZMenuItem>();
			}
		}
	return ZRef<ZMenuItem>();
	}

// ==================================================

struct DoPopup_t
	{
	ZRef<ZMenu> fMenu;
	int32 fAlignWithItem;
	ZRef<ZMenuItem> fResult;
	ZPoint fGlobalLocation;
	ZOSWindow_Std* fBlockedWindow;
	bool fBlockedWindowRunFlag;
	};

static void* sCall_PopupMenu(void* inParam)
	{
	DoPopup_t* theStruct = reinterpret_cast<DoPopup_t*>(inParam);

	HMENU theHMENU = ::CreatePopupMenu();
	WORD theRollingID = 1;
	if (ZUtil_Win::sUseWAPI())
		::sSetupMenuW(theStruct->fMenu, theHMENU, theRollingID);
	else
		::sSetupMenuA(theStruct->fMenu, theHMENU, theRollingID);

	::SetMenuDefaultItem(theHMENU, theStruct->fAlignWithItem, true);

	DWORD result = ::TrackPopupMenu(theHMENU, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
		theStruct->fGlobalLocation.h, theStruct->fGlobalLocation.v, 0, ZOSApp_Win::sGet()->GetHWND_Utility(), nil);

	if (result != 0)
		theStruct->fResult = ZMenu_Win::sFindItemByCommand(theHMENU, result);

	::DestroyMenu(theHMENU);

/*	if (theStruct->fBlockedWindow)
		{
		theStruct->fBlockedWindow->fMutex_Structure.Acquire();
		*theStruct->fBlockedWindowRunFlag = false;
		theStruct->fBlockedWindow->fCondition_Structure.Broadcast();
		theStruct->fBlockedWindow->fMutex_Structure.Release();
		}*/

	return nil;
	}


ZRef<ZMenuItem> ZMenuPopup::DoPopup(const ZPoint& inGlobalLocation, int32 inAlignedItem)
	{
	if (inAlignedItem < 0 || inAlignedItem >= fMenu->Count())
		{
		// The item is out of range, use -1
		inAlignedItem = -1;
		}

	DoPopup_t theStruct;
	theStruct.fMenu = fMenu;
	theStruct.fAlignWithItem = inAlignedItem;
	theStruct.fGlobalLocation = inGlobalLocation;
	theStruct.fBlockedWindow = nil; // ZOSWindow_Std::sWindowForCurrentThread();
	theStruct.fBlockedWindowRunFlag = true;

	if (theStruct.fBlockedWindow)
		{
		ZUnimplemented();
//		ZOSApp_Win::sGet()->InvokeFunctionFromMessagePumpNoWait(sCall_PopupMenu, reinterpret_cast<int32>(&theStruct));
//		theStruct.fBlockedWindow->Internal_MessageLoop(&theStruct.fBlockedWindowRunFlag);
		}
	else
		{
		ZOSApp_Win::sGet()->InvokeFunctionFromMessagePump(sCall_PopupMenu, &theStruct);
		}

	return theStruct.fResult;
	}

// ==================================================

static ZDCFont sGetMenuFont()
	{
	if (ZUtil_Win::sUseWAPI())
		{
		NONCLIENTMETRICSW ncm;
		ncm.cbSize = sizeof(ncm);
		::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		LOGFONTW theLOGFONT = ncm.lfMenuFont;
		ZDCFont::Style theStyle = ZDCFont::normal;
		if (theLOGFONT.lfItalic)
			theStyle |= ZDCFont::italic;
		if (theLOGFONT.lfUnderline)
			theStyle |= ZDCFont::underline;
		if (theLOGFONT.lfWeight > FW_NORMAL)
			theStyle |= ZDCFont::bold;
		return ZDCFont(theLOGFONT.lfFaceName, theStyle, -theLOGFONT.lfHeight);
		}
	else
		{
		NONCLIENTMETRICSA ncm;
		ncm.cbSize = sizeof(ncm);
		::SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
		LOGFONTA theLOGFONT = ncm.lfMenuFont;
		ZDCFont::Style theStyle = ZDCFont::normal;
		if (theLOGFONT.lfItalic)
			theStyle |= ZDCFont::italic;
		if (theLOGFONT.lfUnderline)
			theStyle |= ZDCFont::underline;
		if (theLOGFONT.lfWeight > FW_NORMAL)
			theStyle |= ZDCFont::bold;
		return ZDCFont(theLOGFONT.lfFaceName, theStyle, -theLOGFONT.lfHeight);
		}
	}

static bool sMungeProc_ColorizeBlack(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
// Anything that is black gets turned into what was passed in inRefCon. Otherwise it is left alone.
	if (inOutColor == ZRGBColor::sBlack)
		{
		inOutColor = *static_cast<ZRGBColorPOD*>(inRefCon);
		return true;
		}
	return false;
	}

static bool sMungeProc_ColorizeNonBlack(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
// Anything that is *not* black gets turned into what was passed in inRefCon. Otherwise it is left alone.
	if (inOutColor != ZRGBColor::sBlack)
		{
		inOutColor = *static_cast<ZRGBColorPOD*>(inRefCon);
		return true;
		}
	return false;
	}

static bool sMungeProc_Invert(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = ZRGBColor::sWhite - inOutColor;
	return true;
	}

static bool sMungeProc_Tint(ZCoord hCoord, ZCoord vCoord, ZRGBColorPOD& inOutColor, void* inRefCon)
	{
	inOutColor = inOutColor / 2 + *static_cast<ZRGBColorPOD*>(inRefCon);
	return true;
	}

static void sDrawPixmap(ZDC& inDC, ZPoint inLocation, const ZDCPixmapCombo& inPixmapCombo,
		const ZRGBColor& inColorHilite1, const ZRGBColor& inColorShadow1, const ZRGBColor& inColorHilight, bool inEnabled, bool inSelected)
	{
	if (inEnabled)
		{
// Prefer theColorPixmap
		ZDCPixmap theSourcePixmap(inPixmapCombo.GetColor() ? inPixmapCombo.GetColor() : inPixmapCombo.GetMono());
		if (inSelected)
			{
			ZRGBColor theColor = inColorHilight / 2;
			theSourcePixmap.Munge(sMungeProc_Tint, &theColor);
			}
		inDC.Draw(inLocation, theSourcePixmap, inPixmapCombo.GetMask());
		}
	else
		{
		ZDCPixmap disabledMask(inPixmapCombo.GetMono());
		if (!disabledMask)
			{
			disabledMask = inPixmapCombo.GetColor();
			disabledMask.Munge(sMungeProc_ColorizeNonBlack, const_cast<ZRGBColorPOD*>(&ZRGBColor::sWhite));
			}
// Invert disabledMask
		disabledMask.Munge(sMungeProc_Invert, nil);

// Prefer theMonoPixmap
		ZDCPixmap theSourcePixmap(inPixmapCombo.GetMono() ? inPixmapCombo.GetMono() : inPixmapCombo.GetColor());
		ZRGBColor tempColor;
		if (!inSelected)
			{
			ZDCPixmap hilitePixmap = theSourcePixmap;
			tempColor = inColorHilite1;
			hilitePixmap.Munge(sMungeProc_ColorizeBlack, &tempColor);
			inDC.Draw(inLocation + ZPoint(1,1), hilitePixmap, disabledMask);
			}
		ZDCPixmap shadowPixmap = theSourcePixmap;
		tempColor = inColorShadow1;
		shadowPixmap.Munge(sMungeProc_ColorizeBlack, &tempColor);
		inDC.Draw(inLocation, shadowPixmap, disabledMask);
		}
	}

// AG 99-06-05. Some of this is derived from Paul DiLascia's Cool Menu article, Microsoft Systems Journal,
// January 1998, Vol 13 No 1, pp31-52.
// Other parts are derived from the source for WINE-990328/windows/menu.c
// And the rest is all my own work :)

static ZCoord sGetTextWidth(const ZDC& inDC, const string& inText)
	{
	ZDC underlineDC(inDC);
	ZDCFont underlineFont = underlineDC.GetFont();
	underlineFont.SetStyle(underlineFont.GetStyle() | ZDCFont::underline);
	underlineDC.SetFont(underlineFont);

	ZCoord currWidth = 0;
	string remainingText = inText;
	while (remainingText.size() > 0)
		{
		size_t nextAmpersand = remainingText.find('&');
		if (nextAmpersand == string::npos)
			nextAmpersand = remainingText.size();
		string substring = remainingText.substr(0, nextAmpersand);
		currWidth += inDC.GetTextWidth(substring);

		if (nextAmpersand > remainingText.size() - 1)
			break;

		substring = inText.substr(nextAmpersand + 1, 1);
		currWidth += underlineDC.GetTextWidth(substring);
		remainingText = remainingText.substr(nextAmpersand + 2, remainingText.size() - nextAmpersand + 2);
		}
	return currWidth;
	}

static void sDrawText(const ZDC& inDC, ZPoint inLocation, const string& inText)
	{
	ZDC underlineDC(inDC);
	ZDCFont underlineFont = underlineDC.GetFont();
	underlineFont.SetStyle(underlineFont.GetStyle() | ZDCFont::underline);
	underlineDC.SetFont(underlineFont);

	string remainingText = inText;
	while (remainingText.size() > 0)
		{
		size_t nextAmpersand = remainingText.find('&');
		if (nextAmpersand == string::npos)
			nextAmpersand = remainingText.size();
		string substring = remainingText.substr(0, nextAmpersand);
		inDC.DrawText(inLocation, substring);
		inLocation.h += inDC.GetTextWidth(substring);

		if (nextAmpersand > remainingText.size() - 1)
			break;

		substring = inText.substr(nextAmpersand + 1, 1);
		underlineDC.DrawText(inLocation, substring);
		inLocation.h += underlineDC.GetTextWidth(substring);
		remainingText = remainingText.substr(nextAmpersand + 2, remainingText.size() - nextAmpersand + 2);
		}
	}

void ZMenu_Win::sHandle_MEASUREITEM(MEASUREITEMSTRUCT* inMIS)
	{
	ZPoint checkDimensions(::GetSystemMetrics(SM_CXMENUCHECK), ::GetSystemMetrics(SM_CYMENUCHECK));

	ZRef<ZMenuItem> theMenuItem = reinterpret_cast<ZMenuItem*>(inMIS->itemData);
	ZDCPixmapCombo thePixmapCombo = theMenuItem->GetPixmapCombo();
	string theItemText = ::sStripEmbeddedChars(theMenuItem->GetText());
	ZDC theDC(ZDCScratch::sGet());
	ZDCFont menuFont = ::sGetMenuFont();
	menuFont.SetStyle(theMenuItem->GetStyle());
	theDC.SetFont(menuFont);

// Left edge + check width + right edge
	if (theItemText.empty() && !thePixmapCombo)
		{
// It's a divider -- use half height and zero width
		inMIS->itemHeight = ::GetSystemMetrics(SM_CYMENU) / 2;
		inMIS->itemWidth = 0;
		}
	else
		{
		inMIS->itemWidth = 1 + checkDimensions.h + 4;
		inMIS->itemHeight = ::GetSystemMetrics(SM_CYMENU) - 1;

		if (thePixmapCombo)
			{
			ZPoint thePixmapSize = thePixmapCombo.Size();
			inMIS->itemWidth += (4 + thePixmapSize.h);
			inMIS->itemHeight = max(ZCoord(inMIS->itemHeight), ZCoord(thePixmapSize.v + 2));
			}

		if (theItemText.size())
			{
			ZCoord stringWidth = ::sGetTextWidth(theDC, theItemText);
			ZCoord textHeight = theDC.GetLineHeight();
			// Find the embedded tab, if any
			inMIS->itemWidth += (4 + stringWidth);
			inMIS->itemHeight = max(ZCoord(inMIS->itemHeight), ZCoord(textHeight));
			}

		string right;
		ZMenuAccel theAccel = theMenuItem->GetAccel();
		if (theAccel.fKeyCode != 0)
			inMIS->itemWidth += 8 + theDC.GetTextWidth(theAccel.GetAbbreviation());
		}
	}


#ifndef OBM_CHECK
#	define OBM_CHECK 32760
#endif

void ZMenu_Win::sHandle_DRAWITEM(DRAWITEMSTRUCT* inDIS)
	{
	ZRGBColor colorGrayText = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_GRAYTEXT));
	ZRGBColor colorHiliteText = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	ZRGBColor colorMenuText = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_MENUTEXT));
	ZRGBColor colorMenu = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_MENU));
	ZRGBColor colorHilight = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_HIGHLIGHT));
	ZRGBColor colorHilite1 = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_3DHILIGHT));
	ZRGBColor colorShadow1 = ZRGBColor::sFromCOLORREF(::GetSysColor(COLOR_3DSHADOW));

	ZRef<ZMenuItem> theMenuItem = reinterpret_cast<ZMenuItem*>(inDIS->itemData);
	ZDCPixmapCombo thePixmapCombo = theMenuItem->GetPixmapCombo();
	string theItemText = ::sStripEmbeddedChars(theMenuItem->GetText());

	ZDC_NativeGDI theDC(inDIS->hDC);
	ZRect theBounds = inDIS->rcItem;

	if (theItemText.empty() && !thePixmapCombo)
		{
// It's a divider
		ZCoord theVCoord = theBounds.Top() + theBounds.Height() / 2;
		theDC.SetInk(colorShadow1);
		theDC.Line(theBounds.left, theVCoord, theBounds.right - 1, theVCoord);
		theDC.SetInk(colorHilite1);
		theDC.Line(theBounds.left, theVCoord + 1, theBounds.right - 1, theVCoord + 1);
		}
	else
		{
		bool isEnabled = ((inDIS->itemState & ODS_GRAYED) == 0);
		bool isSelected = ((inDIS->itemState & ODS_SELECTED) != 0);
		bool isChecked = ((inDIS->itemState & ODS_CHECKED) != 0);

		ZPoint checkDimensions(::GetSystemMetrics(SM_CXMENUCHECK), ::GetSystemMetrics(SM_CYMENUCHECK));
		if (isSelected)
			theDC.SetInk(colorHilight);
		else
			theDC.SetInk(colorMenu);
		theDC.Fill(theBounds);

		ZDCFont menuFont = ::sGetMenuFont();
		menuFont.SetStyle(theMenuItem->GetStyle());
		theDC.SetFont(menuFont);

		if (isChecked)
			{
			HBITMAP theHBITMAP = ZUtil_Win::sLoadBitmapID(false, OBM_CHECK);
			ZDCPixmap checkPixmap(new ZDCPixmapRep_DIB(inDIS->hDC, theHBITMAP, true));
			::DeleteObject(theHBITMAP);
			ZDCPixmap checkPixmapMask = checkPixmap;
			checkPixmapMask.Munge(sMungeProc_Invert, nil);
			ZPoint checkPixmapOrigin(theBounds.Left() + 1, theBounds.Top() + (theBounds.Height() - checkPixmap.Height())/2);
			if (isEnabled)
				{
				if (isSelected)
					{
					ZRGBColor tempColor = colorHiliteText;
					checkPixmap.Munge(sMungeProc_ColorizeBlack, &tempColor);
					}
				theDC.Draw(checkPixmapOrigin, checkPixmap, checkPixmapMask);
				}
			else
				{
				ZRGBColor tempColor;
				if (!isSelected)
					{
					ZDCPixmap tempPixmap = checkPixmap;
					tempColor = colorHilite1;
					tempPixmap.Munge(sMungeProc_ColorizeBlack, &tempColor);
					theDC.Draw(checkPixmapOrigin + ZPoint(1, 1), tempPixmap, checkPixmapMask);
					}
				tempColor = colorGrayText;
				checkPixmap.Munge(sMungeProc_ColorizeBlack, &tempColor);
				theDC.Draw(checkPixmapOrigin, checkPixmap, checkPixmapMask);
				}
			}

		ZCoord theHLocation = theBounds.Left() + 1 + checkDimensions.h + 4;

		if (thePixmapCombo)
			{
			ZPoint pixmapSize = thePixmapCombo.Size();
			::sDrawPixmap(theDC, ZPoint(theHLocation, theBounds.Top() + (theBounds.Height() - pixmapSize.v)/2), thePixmapCombo,
				colorHilite1, colorShadow1, colorHilight, isEnabled, isSelected);
			theHLocation += pixmapSize.h + 4;
			}

		ZCoord ascent, descent, leading;
		theDC.GetFontInfo(ascent, descent, leading);
		ZCoord lineHeight = ascent + descent + leading;
		ZCoord theVLocation = theBounds.Top() + (theBounds.Height() - lineHeight) / 2 + ascent;

		if (theItemText.size() > 0)
			{
			if (isEnabled)
				{
				if (isSelected)
					theDC.SetTextColor(colorHiliteText);
				else
					theDC.SetTextColor(colorMenuText);
				::sDrawText(theDC, ZPoint(theHLocation, theVLocation), theItemText);
				}
			else
				{
				if (!isSelected)
					{
					theDC.SetTextColor(colorHilite1);
					::sDrawText(theDC, ZPoint(theHLocation, theVLocation) + ZPoint(1, 1), theItemText);
					}
				theDC.SetTextColor(colorGrayText);
				::sDrawText(theDC, ZPoint(theHLocation, theVLocation), theItemText);
				}
			}

		ZMenuAccel theAccel = theMenuItem->GetAccel();
		if (theAccel.fKeyCode != 0)
			{
			string theAbbrev = theAccel.GetAbbreviation();
			ZCoord abbrevWidth = theDC.GetTextWidth(theAbbrev);
			ZCoord abbrevHLoc = theBounds.Right() - 4 - abbrevWidth;
			if (isEnabled)
				{
				if (isSelected)
					theDC.SetTextColor(colorHiliteText);
				else
					theDC.SetTextColor(colorMenuText);
				theDC.DrawText(ZPoint(abbrevHLoc, theVLocation), theAbbrev);
				}
			else
				{
				if (!isSelected)
					{
					theDC.SetTextColor(colorHilite1);
					theDC.DrawText(ZPoint(abbrevHLoc, theVLocation) + ZPoint(1, 1), theAbbrev);
					}
				theDC.SetTextColor(colorGrayText);
				theDC.DrawText(ZPoint(abbrevHLoc, theVLocation), theAbbrev);
				}
			}
		}
	}

LRESULT ZMenu_Win::sHandle_MENUCHAR(UINT inChar, UINT inFlags, HMENU inHMENU)
	{
	// Walk through inHMENU finding items which are ours
	size_t selectedIndex = 0xFFFFFFFFU;
	vector<size_t> possibleItems;

	if (ZUtil_Win::sUseWAPI())
		{
		size_t menuItemCount = ::GetMenuItemCount(inHMENU);
		MENUITEMINFOW theInfo;
		ZBlockZero(&theInfo, sizeof(theInfo));
		theInfo.cbSize = sizeof(theInfo);
		for (size_t x = 0; x < menuItemCount; ++x)
			{
			// Here we make sure to see what pre-existing sub menu may already be installed, other pre-existing
			// attributes we don't care about for the time being.
			theInfo.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
			::GetMenuItemInfoW(inHMENU, x, true, &theInfo);
			if (theInfo.fType & MFT_OWNERDRAW || theInfo.fType & MFT_STRING)
				{
				string theMenuItemText;
				if (theInfo.fType & MFT_STRING)
					theMenuItemText = reinterpret_cast<char*>(theInfo.dwTypeData);

				if (theInfo.fType & MFT_OWNERDRAW)
					theMenuItemText = reinterpret_cast<ZMenuItem*>(theInfo.dwItemData)->GetText();

				size_t theAmpersandOffset = theMenuItemText.find('&');
				if (theAmpersandOffset != string::npos)
					{
					if (theAmpersandOffset < theMenuItemText.size() - 1)
						{
						if (::toupper(theMenuItemText[theAmpersandOffset + 1]) == ::toupper(inChar))
							possibleItems.push_back(x);
						}
					}
				}
			if (theInfo.fState & MFS_HILITE)
				selectedIndex = x;
			}
		}
	else
		{
		size_t menuItemCount = ::GetMenuItemCount(inHMENU);
		MENUITEMINFOA theInfo;
		ZBlockZero(&theInfo, sizeof(theInfo));
		theInfo.cbSize = sizeof(theInfo);
		for (size_t x = 0; x < menuItemCount; ++x)
			{
			// Here we make sure to see what pre-existing sub menu may already be installed, other pre-existing
			// attributes we don't care about for the time being.
			theInfo.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
			::GetMenuItemInfoA(inHMENU, x, true, &theInfo);
			if (theInfo.fType & MFT_OWNERDRAW || theInfo.fType & MFT_STRING)
				{
				string theMenuItemText;
				if (theInfo.fType & MFT_STRING)
					theMenuItemText = reinterpret_cast<char*>(theInfo.dwTypeData);

				if (theInfo.fType & MFT_OWNERDRAW)
					theMenuItemText = reinterpret_cast<ZMenuItem*>(theInfo.dwItemData)->GetText();

				size_t theAmpersandOffset = theMenuItemText.find('&');
				if (theAmpersandOffset != string::npos)
					{
					if (theAmpersandOffset < theMenuItemText.size() - 1)
						{
						if (::toupper(theMenuItemText[theAmpersandOffset + 1]) == ::toupper(inChar))
							possibleItems.push_back(x);
						}
					}
				}
			if (theInfo.fState & MFS_HILITE)
				selectedIndex = x;
			}
		}

	if (possibleItems.size() == 0)
		return 0;

	if (possibleItems.size() == 1)
		return MAKELONG(possibleItems[0], MNC_EXECUTE);

	for (size_t x = 0; x < possibleItems.size(); ++x)
		{
		if (possibleItems[x] > selectedIndex)
			return MAKELONG(possibleItems[x], MNC_SELECT);
		}

	return MAKELONG(possibleItems[0], MNC_SELECT);
	}

#endif // ZCONFIG(OS, Win32)
// =================================================================================================
