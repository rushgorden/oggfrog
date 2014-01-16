static const char rcsid[] = "@(#) $Id: ZMenu_Carbon.cpp,v 1.6 2006/10/13 20:32:32 agreen Exp $";

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

#include "ZMenu_Carbon.h"

// =================================================================================================
#if ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)

#include "ZString.h"
#include "ZUtil_Mac_HL.h" // For ZUtil_Mac_HL::sCIconHandleFromPixmapCombo

#if ZCONFIG(OS, MacOSX)
#	include <CarbonCore/Gestalt.h>
#	include <CarbonCore/LowMem.h>
#	include <HIToolbox/Menus.h>
#	include <QD/QuickDraw.h>
#	include <CarbonCore/Resources.h>
#else
#	include <Gestalt.h>
#	include <LowMem.h>
#	include <Menus.h>
#	include <QuickDraw.h>
#	include <Resources.h>
#endif

#include <map>

static MenuID sNextMenuID = 1000;
static MenuID sNextMenuHierMenuID = 1;
static MenuID sGetUnusedMenuID(bool hierarchical)
	{
	if (hierarchical)
		{
		MenuID startValue = sNextMenuHierMenuID;
		while (true)
			{
			MenuID currentValue = sNextMenuHierMenuID;
			MenuRef existingMenu = ::GetMenuRef(currentValue);
			++sNextMenuHierMenuID;

			if (sNextMenuHierMenuID >= 256)
				{
				// wrap it around
				sNextMenuHierMenuID = 1;
				}

			if (existingMenu == nil)
				{
				// We didn't find a menu, then this ID is available
				return currentValue;
				}

			if (sNextMenuHierMenuID == startValue)
				{
				// If we end up back where we started then we can't find a free ID
				ZDebugStopf(1, ("sGetUnusedMenuID, couldn't find a free hierarchical ID"));
				break;
				}
			}
		}
	else
		{
		MenuID startValue = sNextMenuID;
		while (true)
			{
			MenuID currentValue = sNextMenuID;
			MenuRef existingMenu = ::GetMenuRef(currentValue);
			++sNextMenuID;

			if (sNextMenuID >= 2000)
				{
				// wrap it around
				sNextMenuID = 1000;
				}

			if (existingMenu == nil)
				{
				// We didn't find a menu, then this ID is available
				return currentValue;
				}

			if (sNextMenuID == startValue)
				{
				// If we end up back where we started then we can't find a free ID
				ZDebugStopf(1, ("sGetUnusedMenuID, couldn't find a free non-hierarchical ID!!"));
				break;
				}
			}
		}
	return 0;
	}

static string sStripEmbeddedChars(const string& iSource)
	{
	string result;
	for (string::const_iterator i = iSource.begin(); i != iSource.end(); ++i)
		{
		if ('&' == *i)
			continue;
		result += *i;
		}
	return result;
	}

static MenuRef sNewMenuHierarchical()
	{
	MenuRef newMenuRef = ::NewMenu(::sGetUnusedMenuID(true), "\pHierarchical");
	::MacInsertMenu(newMenuRef, hierMenu);
	return newMenuRef;
	}

static MenuRef sNewMenuMain(MenuID iBeforeID, const string& iTitle)
	{
	ZAssert(iBeforeID != hierMenu);
	MenuRef newMenuRef = ::NewMenu(sGetUnusedMenuID(false), ZString::sAsPString(iTitle));
	::MacInsertMenu(newMenuRef, iBeforeID);
	return newMenuRef;
	}

static void sDeleteMenuRecursive(MenuID iMenuID);
static void sDeleteMenuItem(MenuRef iMenuRef, MenuItemIndex iMenuItemIndex)
	{
	Handle oldIconHandle;
	UInt8 oldIconType;
	::GetMenuItemIconHandle(iMenuRef, iMenuItemIndex, &oldIconType, &oldIconHandle);
	if (oldIconType != kMenuNoIcon)
		{
		ZAssert(oldIconType == kMenuColorIconType);
		::DisposeCIcon((CIconHandle)oldIconHandle);
		}

	SInt16 theSubMenuID;
	if (noErr != ::GetMenuItemHierarchicalID(iMenuRef, iMenuItemIndex, &theSubMenuID))
		theSubMenuID = 0;

	if (theSubMenuID != 0)
		{
		::SetMenuItemHierarchicalID(iMenuRef, iMenuItemIndex, 0);
		::sDeleteMenuRecursive(theSubMenuID);
		}

	::DeleteMenuItem(iMenuRef, iMenuItemIndex);
	}

static void sDeleteMenuRecursive(MenuRef iMenuRef)
	{
	if (iMenuRef)
		{
		short currentCount = ::CountMenuItems(iMenuRef);
		for (short x = 1; x <= currentCount; ++x)
			::sDeleteMenuItem(iMenuRef, 1);
		::DeleteMenu(GetMenuID(iMenuRef));
		::DisposeMenu(iMenuRef);
		}
	}

static void sDeleteMenuRecursive(MenuID iMenuID)
	{
	MenuRef theMenuRef = ::GetMenuRef(iMenuID);
	::sDeleteMenuRecursive(theMenuRef);
	}

namespace ZANONYMOUS {
struct MenuItemInfo
	{
	ZMenuItem* fMenuItem;
	int32 fChangeCount;
	};
} // anonymous namespace

static bool sSetupMenu(ZRef<ZMenu> iMenu, MenuRef iMenuRef, map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >& ioMenuItemMap, bool& ioSubMenuParentEnabled);
static bool sSetupItem(const ZRef<ZMenuItem>& iMenuItem, MenuRef iMenuRef, MenuItemIndex iMenuItemIndex, map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >& ioMenuItemMap, bool& ioSubMenuParentEnabled)
	{
	ioMenuItemMap[make_pair(GetMenuID(iMenuRef), iMenuItemIndex)] = iMenuItem;
	bool itemIsEnabled = iMenuItem->GetEnabled();
	string theItemText = ::sStripEmbeddedChars(iMenuItem->GetText());

	ZDCPixmapCombo thePixmapCombo = iMenuItem->GetPixmapCombo();
	bool isDivider = (theItemText.size() == 0 && !thePixmapCombo);

	MenuItemInfo currentMenuItemInfo;
	currentMenuItemInfo.fMenuItem = nil;
	currentMenuItemInfo.fChangeCount = 0;
	OSErr err = ::GetMenuItemProperty(iMenuRef, iMenuItemIndex, 'zlib', 'info', sizeof(MenuItemInfo), nil, &currentMenuItemInfo);

	if (err != noErr || currentMenuItemInfo.fMenuItem != iMenuItem.GetObject() || currentMenuItemInfo.fChangeCount != iMenuItem->GetChangeCount())
		{
		currentMenuItemInfo.fMenuItem = iMenuItem.GetObject();
		currentMenuItemInfo.fChangeCount = iMenuItem->GetChangeCount();
		::SetMenuItemProperty(iMenuRef, iMenuItemIndex, 'zlib', 'info', sizeof(MenuItemInfo), &currentMenuItemInfo);

		// Set up the CIconHandle, removing the old one and installing the new
		Handle oldIconHandle;
		UInt8 oldIconType;
		::GetMenuItemIconHandle(iMenuRef, iMenuItemIndex, &oldIconType, &oldIconHandle);

		if (thePixmapCombo)
			{
			if (Handle newIconHandle = (Handle)ZUtil_Mac_HL::sCIconHandleFromPixmapCombo(thePixmapCombo))
				::SetMenuItemIconHandle(iMenuRef, iMenuItemIndex, kMenuColorIconType, newIconHandle);
			else
				::SetMenuItemIconHandle(iMenuRef, iMenuItemIndex, kMenuNoIcon, nil);
			}
		else
			{
			::SetMenuItemIconHandle(iMenuRef, iMenuItemIndex, kMenuNoIcon, nil);
			}

		if (oldIconType != kMenuNoIcon)
			{
			ZAssertStop(1, oldIconType == kMenuColorIconType && oldIconHandle != nil);
			::DisposeCIcon((CIconHandle)oldIconHandle);
			}

		// Set up the item text
		if (isDivider)
			{
			itemIsEnabled = false;
			::SetMenuItemText(iMenuRef, iMenuItemIndex, "\p-");
			}
		else
			{
			if (theItemText.size() == 0)
				::SetMenuItemText(iMenuRef, iMenuItemIndex, "\p ");
			else
				::SetMenuItemText(iMenuRef, iMenuItemIndex, ZString::sAsPString(theItemText));
			::SetItemStyle(iMenuRef, iMenuItemIndex, iMenuItem->GetStyle());
			}

		// Set up the accelerator
		ZMenuAccel theAccel = iMenuItem->GetAccel();
		if (!isDivider && theAccel.fKeyCode != 0)
			{
			::SetItemCmd(iMenuRef, iMenuItemIndex, theAccel.fKeyCode);
			SInt16 modifiers = 0;
			if ((theAccel.fModifiers & ZMenuAccel::eCommandKey) == 0 && (theAccel.fModifiers & ZMenuAccel::eCommandKeyMac) == 0)
				modifiers |= kMenuNoCommandModifier;
			if ((theAccel.fModifiers & ZMenuAccel::eShiftKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eShiftKeyMac) != 0)
				modifiers |= kMenuShiftModifier;
			if ((theAccel.fModifiers & ZMenuAccel::eOptionKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eOptionKeyMac) != 0)
				modifiers |= kMenuOptionModifier;
			if ((theAccel.fModifiers & ZMenuAccel::eControlKeyMac) != 0)
				modifiers |= kMenuControlModifier;
			::SetMenuItemModifiers(iMenuRef, iMenuItemIndex, modifiers);
			}
		else
			::SetItemCmd(iMenuRef, iMenuItemIndex, 0);
		::MacCheckMenuItem(iMenuRef, iMenuItemIndex, !isDivider && iMenuItem->GetChecked());
		}

	// Set up (or remove) the sub menu
	MenuID theSubMenuID;
	if (noErr != ::GetMenuItemHierarchicalID(iMenuRef, iMenuItemIndex, &theSubMenuID))
		theSubMenuID = 0;
	if (ZRef<ZMenu> theSubMenu = iMenuItem->GetSubMenu())
		{
		MenuRef theSubMenuRef = nil;
		if (theSubMenuID != 0)
			{
			theSubMenuRef = ::GetMenuRef(theSubMenuID);
			ZAssert(theSubMenuRef);
			}
		else
			{
			theSubMenuRef = ::sNewMenuHierarchical();
			::SetMenuItemHierarchicalID(iMenuRef, iMenuItemIndex, GetMenuID(theSubMenuRef));
			}
		if (itemIsEnabled)
			ioSubMenuParentEnabled = true;

		// If the item was already enabled, mark it as being choosable.
		if (itemIsEnabled)
			::ChangeMenuItemAttributes(iMenuRef, iMenuItemIndex, kMenuItemAttrSubmenuParentChoosable, 0);
		else
			::ChangeMenuItemAttributes(iMenuRef, iMenuItemIndex, 0, kMenuItemAttrSubmenuParentChoosable);

		if (::sSetupMenu(theSubMenu, theSubMenuRef, ioMenuItemMap, ioSubMenuParentEnabled))
			itemIsEnabled = true;
		}
	else
		{
		if (theSubMenuID != 0)
			{
			::SetMenuItemHierarchicalID(iMenuRef, iMenuItemIndex, 0);
			::sDeleteMenuRecursive(theSubMenuID);
			}
		}
	if (isDivider || !itemIsEnabled)
		::DisableMenuItem(iMenuRef, iMenuItemIndex);
	else
		::MacEnableMenuItem(iMenuRef, iMenuItemIndex);

	return itemIsEnabled;
	}

static bool sSetupMenu(ZRef<ZMenu> iMenu, MenuRef iMenuRef, map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >& ioMenuItemMap, bool& ioSubMenuParentEnabled)
	{
	bool anyEnabled = false;
	short currentCount = ::CountMenuItems(iMenuRef);
	while (iMenu->Count() > currentCount)
		{
		::AppendMenu(iMenuRef, "\p ");
		++currentCount;
		}
	while (iMenu->Count() < currentCount)
		{
		::sDeleteMenuItem(iMenuRef, currentCount);
		--currentCount;
		}
	for (size_t x = 0; x < iMenu->Count(); ++x)
		{
		if (::sSetupItem(iMenu->At(x), iMenuRef, x + 1, ioMenuItemMap, ioSubMenuParentEnabled))
			anyEnabled = true;
		}
	return anyEnabled;
	}

static ZRef<ZMenuItem> sFindItemForResult(MenuID iMenuID, MenuItemIndex iMenuItemIndex, const map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >& iMenuItemMap)
	{
	pair<MenuID, MenuItemIndex> thePair(iMenuID, iMenuItemIndex);
	map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >::const_iterator theIter = iMenuItemMap.find(thePair);
	if (theIter != iMenuItemMap.end())
		return theIter->second;
	return ZRef<ZMenuItem>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu_Carbon

void ZMenu_Carbon::sInstallMenuBar(const ZMenuBar& iMenuBar)
	{
	const vector<ZRef<ZMenu> >& menuVector = iMenuBar.GetMenuVector();
	const vector<string>& titleVector = iMenuBar.GetTitleVector();

	ZAssertLogf(0, menuVector.size() == titleVector.size(), ("menuVector.size() == %d, titleVector.size() == %d", menuVector.size(), titleVector.size()));

	MenuRef theRootMenuRef = ::AcquireRootMenu();
	short menuCount = ::CountMenuItems(theRootMenuRef);
	ZAssertStop(1, menuCount >= 0);

	// Figure out how many menus are really application menus (ID > 0)
	size_t realCount = 0;
	for (short x = 0; x < menuCount; ++x)
		{
		MenuRef currentMenu;
		if (noErr == ::GetMenuItemHierarchicalMenu(theRootMenuRef, x + 1,  &currentMenu))
			{
			if (::GetMenuID(currentMenu) < 0)
				break;
			}
		++realCount;
		}

	// Walk through our list of titles
	for (size_t x = 0; x < titleVector.size(); ++x)
		{
		string wantedTitle = ::sStripEmbeddedChars(titleVector[x]);
		if (x >= realCount)
			{
			// If we have more titles than menus, then append a menu.
			MenuRef newMenuRef = ::sNewMenuMain(0, wantedTitle);
			++realCount;
			}
		else
			{
			// Get the menu corresponding to the current title
			MenuRef currentMenu;
			if (noErr == ::GetMenuItemHierarchicalMenu(theRootMenuRef, x + 1,  &currentMenu))
				{
				Str255 theStr255;
				::GetMenuTitle(currentMenu, theStr255);
				string currentTitle = ZString::sFromPString(theStr255);
				if (currentTitle != wantedTitle)
					{
					ZString::sToPString(wantedTitle, theStr255, 255);
					::SetMenuTitle(currentMenu, theStr255);
					}
				}
			}
		}
	ZAssert(titleVector.size() <= realCount);

	// We may have more menu handles installed than menus, in which case (recursively) delete the extras
	while (realCount > titleVector.size())
		{
		MenuRef currentMenu;
		if (noErr == ::GetMenuItemHierarchicalMenu(theRootMenuRef, realCount,  &currentMenu))
			{
			::sDeleteMenuItem(theRootMenuRef, realCount);
			::sDeleteMenuRecursive(::GetMenuID(currentMenu));
			}
		--realCount;
		}

	::ReleaseMenu(theRootMenuRef);
	::DrawMenuBar();
	}

void ZMenu_Carbon::sSetupMenuBar(const ZMenuBar& iMenuBar)
	{
	ZMenu_Carbon::sInstallMenuBar(iMenuBar);

	map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> > theMenuItemMap;
	bool theSubMenuParentEnabled;

	MenuRef theRootMenuRef = ::AcquireRootMenu();
	const vector<ZRef<ZMenu> >& menuVector = iMenuBar.GetMenuVector();
	for (size_t x = 0; x < menuVector.size(); ++x)
		{
		MenuRef currentMenu;
		if (noErr == ::GetMenuItemHierarchicalMenu(theRootMenuRef, x + 1,  &currentMenu))
			::sSetupMenu(menuVector[x], currentMenu, theMenuItemMap, theSubMenuParentEnabled);
		}
	::ReleaseMenu(theRootMenuRef);
	}

ZRef<ZMenuItem> ZMenu_Carbon::sGetMenuItem(MenuRef iMenuRef, MenuItemIndex iMenuItemIndex)
	{
	MenuItemInfo currentMenuItemInfo;
	if (noErr == ::GetMenuItemProperty(iMenuRef, iMenuItemIndex,
		'zlib', 'info',
		sizeof(MenuItemInfo), nil, &currentMenuItemInfo))
		{
		return currentMenuItemInfo.fMenuItem;
		}
	return ZRef<ZMenuItem>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu_Popup::DoPopup

ZRef<ZMenuItem> ZMenuPopup::DoPopup(const ZPoint& iGlobalLocation, int32 iAlignedItem)
	{
	MenuRef theMenuRef = ::sNewMenuHierarchical();

	if (iAlignedItem < 0 || iAlignedItem >= fMenu->Count())
		{
		// The item is out of range, use zero
		iAlignedItem = 0;
		}
	else
		{
		// Otherwise use the value (converted to a one based index)
		iAlignedItem += 1;
		}

	map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> > theMenuMap;
	bool anySubMenuParentEnabled = false;
	::sSetupMenu(fMenu, theMenuRef, theMenuMap, anySubMenuParentEnabled);

	long theResult = ::PopUpMenuSelect(theMenuRef, iGlobalLocation.v, iGlobalLocation.h, iAlignedItem);

	MenuID resultMenuID = (((theResult) >> 16) & 0xFFFF);
	MenuItemIndex resultItem = ((theResult) & 0xFFFF);

	ZRef<ZMenuItem> foundItem;

	map<pair<MenuID, MenuItemIndex>, ZRef<ZMenuItem> >::const_iterator theIter = theMenuMap.find(make_pair(resultMenuID, resultItem));
	if (theIter != theMenuMap.end())
		{
		if (theIter->second->GetEnabled())
			foundItem = theIter->second;
		}
	::sDeleteMenuRecursive(theMenuRef);
	return foundItem;
	}

// =================================================================================================
#endif // ZCONFIG(OS, MacOSX) || ZCONFIG(OS, Carbon)
