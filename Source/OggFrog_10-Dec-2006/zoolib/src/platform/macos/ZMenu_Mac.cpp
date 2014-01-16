static const char rcsid[] = "@(#) $Id: ZMenu_Mac.cpp,v 1.5 2006/04/10 20:44:21 agreen Exp $";

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

#include "ZMenu_Mac.h"

// =================================================================================================
#if ZCONFIG(OS, MacOS7)

#include "ZString.h"
#include "ZUtil_Mac_HL.h" // For ZUtil_Mac_HL::sCIconHandleFromPixmapCombo

#include <Appearance.h> // For gestaltAppearanceAttr etc.
#include <Gestalt.h>
#include <LowMem.h>
#include <Menus.h>
#include <QuickDraw.h>
#include <Resources.h>

#include <map>

#ifndef MERCUTIO_ENABLED
#	define MERCUTIO_ENABLED 1
#endif

#if MERCUTIO_ENABLED
#	include <MercutioAPI.h>
#endif

#define EnableMenuItem EnableItem
#define DisableMenuItem DisableItem

static short sNextMenuID = 1000;
static short sNextMenuHierMenuID = 1;
static short sGetUnusedMenuID(bool hierarchical)
	{
	if (hierarchical)
		{
		short startValue = sNextMenuHierMenuID;
		while (true)
			{
			short currentValue = sNextMenuHierMenuID;
			MenuHandle existingMenu = ::GetMenuHandle(currentValue);
			++sNextMenuHierMenuID;
// wrap it around
			if (sNextMenuHierMenuID >= 256)
				sNextMenuHierMenuID = 1;
// If we didn't find a menu, then this ID is available
			if (existingMenu == nil)
				return currentValue;
// If we end up back where we started then we can't find a free ID
			if (sNextMenuHierMenuID == startValue)
				{
				ZDebugStopf(1, ("sGetUnusedMenuID, couldn't find a free hierarchical ID"));
				break;
				}
			}
		}
	else
		{
		short startValue = sNextMenuID;
		while (true)
			{
			short currentValue = sNextMenuID;
			MenuHandle existingMenu = ::GetMenuHandle(currentValue);
			++sNextMenuID;
// wrap it around
			if (sNextMenuID >= 2000)
				sNextMenuID = 1000;
// If we didn't find a menu, then this ID is available
			if (existingMenu == nil)
				return currentValue;
// If we end up back where we started then we can't find a free ID
			if (sNextMenuID == startValue)
				{
				ZDebugStopf(1, ("sGetUnusedMenuID, couldn't find a free non-hierarchical ID!!"));
				break;
				}
			}
		}
	return 0;
	}

static string sStripEmbeddedChars(const string& source)
	{
	string returnedString;
	for (size_t index = 0; index < source.length(); ++index)
		{
		if (source[index] == '&')
			continue;
		returnedString += source[index];
		}
	return returnedString;
	}


static bool sCheckedForAppearanceFlag = false;
static bool sHasAppearanceFlag = false;
static bool sHasAppearance()
	{
	if (!sCheckedForAppearanceFlag)
		{
		sCheckedForAppearanceFlag = true;
		long response;
		if (noErr == ::Gestalt(gestaltAppearanceAttr, &response))
			{
			if ((response & (1 << gestaltAppearanceExists)) != 0)
				sHasAppearanceFlag = true;
			}
		}
	return sHasAppearanceFlag;
	}

#if MERCUTIO_ENABLED
static bool sCheckedForMercutioFlag = false;
static bool sHasMercutioFlag = false;
static Handle sMercutioResource = nil;
static MenuPrefsRec sMenuPrefsRec;
static MenuHandle sMercutioHelperMenuHandle = nil;
static MercutioCallbackUPP sMercutioCallbackUPP = nil;
static map<pair<short, short>, ZRef<ZMenuItem> >* sCurrentMenuItemMap = nil;

static pascal void sMercutioCallback(short inMenuID, short inPreviousModifiers, RichItemData* inOutItemData)
	{
	if (!sCurrentMenuItemMap)
		return;

	pair<short, short> thePair(inMenuID, inOutItemData->itemID);
	ZRef<ZMenuItem> theMenuItem = (*sCurrentMenuItemMap)[thePair];
	if (!theMenuItem)
		return;

	if (inOutItemData->cbMsg == cbBasicDataOnlyMsg || inOutItemData->cbMsg == cbGetLongestItemMsg)
		{
		ZDCFont::Style theStyle = theMenuItem->GetStyle();
		if (inOutItemData->textStyle.s != theStyle)
			{
			inOutItemData->textStyle.s = theStyle;
			inOutItemData->flags |= kChangedByCallback;
			inOutItemData->flags |= ksameAlternateAsLastTime;
			}
		inOutItemData->textStyle.s = theMenuItem->GetStyle();
		if (theMenuItem->GetPixmapCombo())
			{
			inOutItemData->flags |= kHasIcon;
			inOutItemData->flags |= ksameAlternateAsLastTime;
			inOutItemData->flags |= kChangedByCallback;
			}
		}

	if (inOutItemData->cbMsg == cbIconOnlyMsg || inOutItemData->cbMsg == cbGetLongestItemMsg)
		{
		if (theMenuItem->GetPixmapCombo())
			{
			if (inOutItemData->hIcon == nil)
				{
				inOutItemData->hIcon = (Handle)ZUtil_Mac_HL::sCIconHandleFromPixmapCombo(theMenuItem->GetPixmapCombo());
				inOutItemData->iconType = 'cicn';
				inOutItemData->flags |= ksameAlternateAsLastTime;
				inOutItemData->flags |= kChangedByCallback;
				}
			}
		}
	}

static bool sHasMercutio()
	{
	if (!sCheckedForMercutioFlag)
		{
		sCheckedForMercutioFlag = true;
		sMercutioResource = ::Get1Resource('MDEF', 19999);
		sMercutioCallbackUPP = NewMercutioCallback(sMercutioCallback);
		sMercutioHelperMenuHandle = ::NewMenu(20000, "\pMercutio Helper");
		sMercutioHelperMenuHandle[0]->menuProc = sMercutioResource;
		sMenuPrefsRec.optionKeyFlag.s = condense;
		sMenuPrefsRec.shiftKeyFlag.s = extend;
		sMenuPrefsRec.cmdKeyFlag.s = shadow;
		sMenuPrefsRec.controlKeyFlag.s = italic;
		sMenuPrefsRec.isDynamicFlag.s = 0;
		sMenuPrefsRec.forceNewGroupFlag.s = 0;
		sMenuPrefsRec.useCallbackFlag.s = underline;
		sMenuPrefsRec.requiredModifiers = 0;
		}
	return sMercutioResource != nil;
	}

static bool sIsMercutioMenu(MenuHandle inMenuHandle)
	{
	ZAssert(!sHasAppearance());
	if (::sHasMercutio() && inMenuHandle[0]->menuProc == sMercutioResource)
		return true;
	return false;
	}

#endif // MERCUTIO_ENABLED

static MenuHandle sNewMenuHierarchical()
	{
	MenuHandle newMenuHandle = ::NewMenu(::sGetUnusedMenuID(true), "\pHierarchical");
#if MERCUTIO_ENABLED
	if (!sHasAppearance() && ::sHasMercutio())
		{
		newMenuHandle[0]->menuProc = sMercutioResource;
		::MDEF_SetCallbackProc(newMenuHandle, sMercutioCallbackUPP);
		::MDEF_SetMenuPrefs(newMenuHandle, &sMenuPrefsRec);
		}
#endif // MERCUTIO_ENABLED
	::InsertMenu(newMenuHandle, -1);
	return newMenuHandle;
	}

static MenuHandle sNewMenuMain(short inBeforeID, const string& inTitle)
	{
	ZAssert(inBeforeID != -1);
	MenuHandle newMenuHandle = ::NewMenu(sGetUnusedMenuID(false), ZString::sAsPString(inTitle));
#if MERCUTIO_ENABLED
	if (!sHasAppearance() && ::sHasMercutio())
		{
		newMenuHandle[0]->menuProc = sMercutioResource;
		::MDEF_SetCallbackProc(newMenuHandle, sMercutioCallbackUPP);
		::MDEF_SetMenuPrefs(newMenuHandle, &sMenuPrefsRec);
		}
#endif // MERCUTIO_ENABLED
	::InsertMenu(newMenuHandle, inBeforeID);
	return newMenuHandle;
	}

// ==================================================

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif
struct MenuElement
	{
	MenuHandle menu;
	short leftEdge;
	};

struct MenusList
	{
	short itemsOffset; // offset to last menu item.
	short rightEdge; // right edge of the menu.
	short resId; // resource id of MBDF.
	MenuElement elements[]; // array of menu items.
	};

typedef MenusList** MenusListHandle;
#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

// ==================================================

static void sDeleteMenuRecursive(short inMenuID);
static void sDeleteMenuItem(MenuHandle inMenuHandle, short inMenuItemIndex)
	{
	if (sHasAppearance())
		{
		Handle oldIconHandle;
		UInt8 oldIconType;
		::GetMenuItemIconHandle(inMenuHandle, inMenuItemIndex, &oldIconType, &oldIconHandle);
		if (oldIconType != kMenuNoIcon)
			{
			ZAssert(oldIconType == kMenuColorIconType);
			::DisposeCIcon((CIconHandle)oldIconHandle);
			}
		SInt16 theSubMenuID;
		if (noErr != ::GetMenuItemHierarchicalID(inMenuHandle, inMenuItemIndex, &theSubMenuID))
			theSubMenuID = 0;
		if (theSubMenuID != 0)
			{
			::SetMenuItemHierarchicalID(inMenuHandle, inMenuItemIndex, 0);
			::sDeleteMenuRecursive(theSubMenuID);
			}
		::DeleteMenuItem(inMenuHandle, inMenuItemIndex);
		}
	else
		{
		CharParameter theItemCmd;
		::GetItemCmd(inMenuHandle, inMenuItemIndex, &theItemCmd);
		if (theItemCmd == hMenuCmd)
			{
			CharParameter theItemMark;
			::GetItemMark(inMenuHandle, inMenuItemIndex, &theItemMark);
			sDeleteMenuRecursive(theItemMark);
			}
		::DeleteMenuItem(inMenuHandle, inMenuItemIndex);
		}
	}

#define GetMenuID(theMenuHandle) (theMenuHandle[0]->menuID)

static void sDeleteMenuRecursive(MenuHandle inMenuHandle)
	{
	if (inMenuHandle)
		{
		short currentCount = ::CountMenuItems(inMenuHandle);
		for (short x = 1; x <= currentCount; ++x)
			::sDeleteMenuItem(inMenuHandle, 1);
		::DeleteMenu(GetMenuID(inMenuHandle));
		::DisposeMenu(inMenuHandle);
		}
	}

static void sDeleteMenuRecursive(short inMenuID)
	{
	MenuHandle theMenuHandle = ::GetMenuHandle(inMenuID);
	::sDeleteMenuRecursive(theMenuHandle);
	}

static bool sSetupMenu(ZRef<ZMenu> inMenu, MenuHandle inMenuHandle, map<pair<short, short>, ZRef<ZMenuItem> >& inOutMenuItemMap, bool& inOutSubMenuParentEnabled);
static bool sSetupItem(const ZRef<ZMenuItem>& inMenuItem, MenuHandle inMenuHandle, short inMenuItemIndex, map<pair<short, short>, ZRef<ZMenuItem> >& inOutMenuItemMap, bool& inOutSubMenuParentEnabled)
	{
	inOutMenuItemMap[make_pair(GetMenuID(inMenuHandle), inMenuItemIndex)] = inMenuItem;
	bool itemIsEnabled = inMenuItem->GetEnabled();
	string theItemText = ::sStripEmbeddedChars(inMenuItem->GetText());

// ==================================================
	if (sHasAppearance())
		{
		ZDCPixmapCombo thePixmapCombo = inMenuItem->GetPixmapCombo();
		bool isDivider = (theItemText.empty() && !thePixmapCombo);

		UInt32 theRefCon1;
		::GetMenuItemRefCon(inMenuHandle, inMenuItemIndex, &theRefCon1);
		UInt32 theRefCon2;
		::GetMenuItemRefCon2(inMenuHandle, inMenuItemIndex, &theRefCon2);
		if (inMenuItem != reinterpret_cast<ZMenuItem*>(theRefCon1) || inMenuItem->GetChangeCount() != theRefCon2)
			{
// Get the refcons set up
			::SetMenuItemRefCon(inMenuHandle, inMenuItemIndex, reinterpret_cast<UInt32>(inMenuItem.GetObject()));
			::SetMenuItemRefCon2(inMenuHandle, inMenuItemIndex, inMenuItem->GetChangeCount());

// Set up the CIconHandle, removing the old one and installing the new
			Handle oldIconHandle;
			UInt8 oldIconType;
			::GetMenuItemIconHandle(inMenuHandle, inMenuItemIndex, &oldIconType, &oldIconHandle);

			if (thePixmapCombo)
				{
				if (Handle newIconHandle = (Handle)ZUtil_Mac_HL::sCIconHandleFromPixmapCombo(thePixmapCombo))
					::SetMenuItemIconHandle(inMenuHandle, inMenuItemIndex, kMenuColorIconType, newIconHandle);
				else
					::SetMenuItemIconHandle(inMenuHandle, inMenuItemIndex, kMenuNoIcon, nil);
				}
			else
				{
				::SetMenuItemIconHandle(inMenuHandle, inMenuItemIndex, kMenuNoIcon, nil);
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
				::SetMenuItemText(inMenuHandle, inMenuItemIndex, "\p-");
				}
			else
				{
				if (theItemText.size() == 0)
					::SetMenuItemText(inMenuHandle, inMenuItemIndex, "\p ");
				else
					::SetMenuItemText(inMenuHandle, inMenuItemIndex, ZString::sAsPString(theItemText));
				::SetItemStyle(inMenuHandle, inMenuItemIndex, inMenuItem->GetStyle());
				}

// Set up the accelerator
			ZMenuAccel theAccel = inMenuItem->GetAccel();
			if (!isDivider && theAccel.fKeyCode != 0)
				{
				::SetItemCmd(inMenuHandle, inMenuItemIndex, theAccel.fKeyCode);
				UInt8 modifiers = 0;
				if ((theAccel.fModifiers & ZMenuAccel::eCommandKey) == 0 && (theAccel.fModifiers & ZMenuAccel::eCommandKeyMac) == 0)
					modifiers |= kMenuNoCommandModifier;
				if ((theAccel.fModifiers & ZMenuAccel::eShiftKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eShiftKeyMac) != 0)
					modifiers |= kMenuShiftModifier;
				if ((theAccel.fModifiers & ZMenuAccel::eOptionKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eOptionKeyMac) != 0)
					modifiers |= kMenuOptionModifier;
				if ((theAccel.fModifiers & ZMenuAccel::eControlKeyMac) != 0)
					modifiers |= kMenuControlModifier;
				::SetMenuItemModifiers(inMenuHandle, inMenuItemIndex, modifiers);
				}
			else
				::SetItemCmd(inMenuHandle, inMenuItemIndex, 0);
			::MacCheckMenuItem(inMenuHandle, inMenuItemIndex, !isDivider && inMenuItem->GetChecked());
			}

// Set up (or remove) the sub menu
		SInt16 theSubMenuID;
		if (noErr != ::GetMenuItemHierarchicalID(inMenuHandle, inMenuItemIndex, &theSubMenuID))
			theSubMenuID = 0;
		if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
			{
			MenuHandle theSubMenuHandle = nil;
			if (theSubMenuID != 0)
				{
				theSubMenuHandle = ::GetMenuHandle(theSubMenuID);
				ZAssert(theSubMenuHandle);
				}
			else
				{
				theSubMenuHandle = ::sNewMenuHierarchical();
				::SetMenuItemHierarchicalID(inMenuHandle, inMenuItemIndex, GetMenuID(theSubMenuHandle));
				}
			if (itemIsEnabled)
				inOutSubMenuParentEnabled = true;
			if (::sSetupMenu(theSubMenu, theSubMenuHandle, inOutMenuItemMap, inOutSubMenuParentEnabled))
				itemIsEnabled = true;
			}
		else
			{
			if (theSubMenuID != 0)
				{
				::SetMenuItemHierarchicalID(inMenuHandle, inMenuItemIndex, 0);
				::sDeleteMenuRecursive(theSubMenuID);
				}
			}
		if (isDivider || !itemIsEnabled)
			::DisableMenuItem(inMenuHandle, inMenuItemIndex);
		else
			::MacEnableMenuItem(inMenuHandle, inMenuItemIndex);
		}
// ==================================================
#if MERCUTIO_ENABLED
	else if (sIsMercutioMenu(inMenuHandle))
		{
		bool isDivider = theItemText.empty();
		if (isDivider)
			{
			itemIsEnabled = false;
			::SetMenuItemText(inMenuHandle, inMenuItemIndex, "\p-");
			}
		else
			{
			::SetMenuItemText(inMenuHandle, inMenuItemIndex, ZString::sAsPString(theItemText));
			}

// Force every item to be dynamic
		Style theStyle = underline; // Marked as dynamic

		if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
			{
			CharParameter theItemCmd;
			::GetItemCmd(inMenuHandle, inMenuItemIndex, &theItemCmd);
			CharParameter theItemMark;
			::GetItemMark(inMenuHandle, inMenuItemIndex, &theItemMark);
			MenuHandle theSubMenuHandle = nil;
			if (theItemCmd == hMenuCmd)
				{
				theSubMenuHandle = ::GetMenuHandle(theItemMark);
				}
			else
				{
				theSubMenuHandle = ::sNewMenuHierarchical();
				::SetItemCmd(inMenuHandle, inMenuItemIndex, hMenuCmd);
				::SetItemMark(inMenuHandle, inMenuItemIndex, GetMenuID(theSubMenuHandle));
				}
			if (itemIsEnabled)
				inOutSubMenuParentEnabled = true;
			if (::sSetupMenu(theSubMenu, theSubMenuHandle, inOutMenuItemMap, inOutSubMenuParentEnabled))
				itemIsEnabled = true;
			}
		else
			{
			CharParameter theItemCmd;
			::GetItemCmd(inMenuHandle, inMenuItemIndex, &theItemCmd);
			CharParameter theItemMark;
			::GetItemMark(inMenuHandle, inMenuItemIndex, &theItemMark);
			if (theItemCmd == hMenuCmd)
				::sDeleteMenuRecursive(theItemMark);

			ZMenuAccel theAccel = inMenuItem->GetAccel();
			if (!isDivider && theAccel.fKeyCode != 0)
				{
				::SetItemCmd(inMenuHandle, inMenuItemIndex, theAccel.fKeyCode);
				UInt8 modifiers = 0;
				if ((theAccel.fModifiers & ZMenuAccel::eCommandKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eCommandKeyMac) != 0)
					theStyle |= shadow;
				if ((theAccel.fModifiers & ZMenuAccel::eShiftKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eShiftKeyMac) != 0)
					theStyle |= extend;
				if ((theAccel.fModifiers & ZMenuAccel::eOptionKey) != 0 || (theAccel.fModifiers & ZMenuAccel::eOptionKeyMac) != 0)
					theStyle |= condense;
				if ((theAccel.fModifiers & ZMenuAccel::eControlKeyMac) != 0)
					theStyle |= italic;
				::SetItemCmd(inMenuHandle, inMenuItemIndex, theAccel.fKeyCode);
				}
			else
				{
				::SetItemCmd(inMenuHandle, inMenuItemIndex, 0);
				}
			::MacCheckMenuItem(inMenuHandle, inMenuItemIndex, !isDivider && inMenuItem->GetChecked());
			}

		::SetItemStyle(inMenuHandle, inMenuItemIndex, theStyle);

		if (isDivider || !itemIsEnabled)
			::DisableMenuItem(inMenuHandle, inMenuItemIndex);
		else
			::MacEnableMenuItem(inMenuHandle, inMenuItemIndex);
		}
#endif // MERCUTIO_ENABLED
// ==================================================
	else
		{
		bool isDivider = (theItemText.size() == 0);
		if (isDivider)
			{
			itemIsEnabled = false;
			::SetMenuItemText(inMenuHandle, inMenuItemIndex, "\p-");
			}
		else
			{
			::SetMenuItemText(inMenuHandle, inMenuItemIndex, ZString::sAsPString(theItemText));
			}

		::SetItemStyle(inMenuHandle, inMenuItemIndex, inMenuItem->GetStyle());

		if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
			{
			CharParameter theItemCmd;
			::GetItemCmd(inMenuHandle, inMenuItemIndex, &theItemCmd);
			CharParameter theItemMark;
			::GetItemMark(inMenuHandle, inMenuItemIndex, &theItemMark);
			MenuHandle theSubMenuHandle = nil;
			if (theItemCmd == hMenuCmd)
				{
				theSubMenuHandle = ::GetMenuHandle(theItemMark);
				}
			else
				{
				theSubMenuHandle = ::sNewMenuHierarchical();
				::SetItemCmd(inMenuHandle, inMenuItemIndex, hMenuCmd);
				::SetItemMark(inMenuHandle, inMenuItemIndex, GetMenuID(theSubMenuHandle));
				}
			if (itemIsEnabled)
				inOutSubMenuParentEnabled = true;
			if (::sSetupMenu(theSubMenu, theSubMenuHandle, inOutMenuItemMap, inOutSubMenuParentEnabled))
				itemIsEnabled = true;
			}
		else
			{
			CharParameter theItemCmd;
			::GetItemCmd(inMenuHandle, inMenuItemIndex, &theItemCmd);
			CharParameter theItemMark;
			::GetItemMark(inMenuHandle, inMenuItemIndex, &theItemMark);
			if (theItemCmd == hMenuCmd)
				::sDeleteMenuRecursive(theItemMark);

			ZMenuAccel theAccel = inMenuItem->GetAccel();
			if (!isDivider && theAccel.fKeyCode != 0)
				{
// With this non-Mercutio, non-Appearance menu manager we support only the command key.
//				ZAssert(theAccel.fModifiers == ZMenuAccel::eCommandKey);
				::SetItemCmd(inMenuHandle, inMenuItemIndex, theAccel.fKeyCode);
				}
			else
				{
				::SetItemCmd(inMenuHandle, inMenuItemIndex, 0);
				}
			::MacCheckMenuItem(inMenuHandle, inMenuItemIndex, !isDivider && inMenuItem->GetChecked());
			}

		if (isDivider || !itemIsEnabled)
			::DisableMenuItem(inMenuHandle, inMenuItemIndex);
		else
			::MacEnableMenuItem(inMenuHandle, inMenuItemIndex);
		}
	return itemIsEnabled;
	}

static bool sSetupMenu(ZRef<ZMenu> inMenu, MenuHandle inMenuHandle, map<pair<short, short>, ZRef<ZMenuItem> >& inOutMenuItemMap, bool& inOutSubMenuParentEnabled)
	{
	bool anyEnabled = false;
	short currentCount = ::CountMenuItems(inMenuHandle);
	while (inMenu->Count() > currentCount)
		{
		::AppendMenu(inMenuHandle, "\p ");
		++currentCount;
		}
	while (inMenu->Count() < currentCount)
		{
		::sDeleteMenuItem(inMenuHandle, currentCount);
		--currentCount;
		}
	for (size_t x = 0; x < inMenu->Count(); ++x)
		{
		if (::sSetupItem(inMenu->At(x), inMenuHandle, x + 1, inOutMenuItemMap, inOutSubMenuParentEnabled))
			anyEnabled = true;
		}
	return anyEnabled;
	}

static void sPostSetup(const ZMenuBar& inMenuBar, size_t& inOutAppleMenuItemCount, map<pair<short, short>, ZRef<ZMenuItem> >& inOutMenuItemMap, bool& inOutSubMenuParentEnabled)
	{
	MenusListHandle theMenusListHandle = (MenusListHandle)::LMGetMenuList();
	short menuCount = (theMenusListHandle[0]->itemsOffset)/sizeof(MenuElement);
	ZAssert(menuCount > 0);

	MenuHandle appleMenuHandle = theMenusListHandle[0]->elements[0].menu;

// Set up the apple menu
	ZRef<ZMenu> appleMenu = inMenuBar.GetAppleMenu();
	while (appleMenu->Count() > inOutAppleMenuItemCount)
		{
		::MacInsertMenuItem(appleMenuHandle, "\p ", 0);
		++inOutAppleMenuItemCount;
		}
	while (appleMenu->Count() < inOutAppleMenuItemCount)
		{
		::DeleteMenuItem(appleMenuHandle, 1);
		--inOutAppleMenuItemCount;
		}
	for (size_t x = 0; x < appleMenu->Count(); ++x)
		::sSetupItem(appleMenu->At(x), appleMenuHandle, x + 1, inOutMenuItemMap, inOutSubMenuParentEnabled);

// Now set up all the other menus
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	for (size_t x = 0; x < menuVector.size(); ++x)
		{
		ZAssertStop(1, theMenusListHandle[0]->elements[x + 1].menu[0]->menuID > 0);
		::sSetupMenu(menuVector[x], theMenusListHandle[0]->elements[x + 1].menu, inOutMenuItemMap, inOutSubMenuParentEnabled);
		}
	}

static ZRef<ZMenuItem> sFindItemForResult(short inMenuID, short inMenuItemIndex, size_t inAppleMenuItemCount, const map<pair<short, short>, ZRef<ZMenuItem> >& inMenuItemMap)
	{
	MenusListHandle theMenusListHandle = (MenusListHandle)::LMGetMenuList();
	MenuHandle appleMenuHandle = theMenusListHandle[0]->elements[0].menu;
	if (inMenuID == appleMenuHandle[0]->menuID && inMenuItemIndex > inAppleMenuItemCount)
		{
		Str255 theDAText;
		::GetMenuItemText(appleMenuHandle, inMenuItemIndex, theDAText);
		short daRefNum = ::OpenDeskAcc(theDAText);
		return ZRef<ZMenuItem>();
		}

	pair<short, short> thePair(inMenuID, inMenuItemIndex);
	map<pair<short, short>, ZRef<ZMenuItem> >::const_iterator theIter = inMenuItemMap.find(thePair);
	if (theIter != inMenuItemMap.end())
		return theIter->second;
	return ZRef<ZMenuItem>();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu_Mac

void ZMenu_Mac::sInstallMenuBar(const ZMenuBar& inMenuBar)
	{
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	const vector<string>& titleVector = inMenuBar.GetTitleVector();

	ZAssertLogf(0, menuVector.size() == titleVector.size(), ("menuVector.size() == %d, titleVector.size() == %d", menuVector.size(), titleVector.size()));

	MenusListHandle theMenusListHandle = (MenusListHandle)::LMGetMenuList();
	short menuCount = (theMenusListHandle[0]->itemsOffset)/sizeof(MenuElement);
	ZAssert(menuCount > 0);

// Figure out how many menus are really application menus (ID > 0)
	size_t realCount = 0;
	for (short x = 1; x < menuCount; ++x)
		{
		MenuHandle currentMenuHandle = theMenusListHandle[0]->elements[x].menu;
		if (theMenusListHandle[0]->elements[x].menu[0]->menuID < 0)
			break;
		++realCount;
		}

// Walk through our list of titles
	for (size_t x = 0; x < titleVector.size(); ++x)
		{
		if (x >= realCount)
			{
// If we have more titles than menu handles installed, then append a menu handle
			MenuHandle newMenuHandle = ::sNewMenuMain(0, ::sStripEmbeddedChars(titleVector[x]));
			++realCount;
			}
		else
			{
// Get the menu handle corresponding to the current title
			MenuHandle currentMenuHandle = theMenusListHandle[0]->elements[x + 1].menu;
			SignedByte oldState = ::HGetState((Handle)currentMenuHandle);
			::HLock((Handle)currentMenuHandle);
			string currentTitle = ZString::sFromPString(currentMenuHandle[0]->menuData);
			::HSetState((Handle)currentMenuHandle, oldState);
			string wantedTitle = ::sStripEmbeddedChars(titleVector[x]);
			if (currentTitle != wantedTitle)
				{
// If the title does not match what we want, insert a new menu handle prior to the current one
				MenuHandle newMenuHandle = ::sNewMenuMain(GetMenuID(currentMenuHandle), wantedTitle);
// And delete the current menu handle (plus any sub menus it references)
				::sDeleteMenuRecursive(GetMenuID(currentMenuHandle));
				}
			}
		}
	ZAssert(titleVector.size() <= realCount);
// We may have more menu handles installed than menus, in which case (recursively) delete the extras
	while (realCount > titleVector.size())
		{
		::sDeleteMenuRecursive(theMenusListHandle[0]->elements[realCount].menu[0]->menuID);
		--realCount;
		}

	::DrawMenuBar();
	}

ZRef<ZMenuItem> ZMenu_Mac::sPostSetupAndHandleMenuBarClick(const ZMenuBar& inMenuBar, ZPoint whereClicked, size_t& inOutAppleMenuItemCount)
	{
	ZMenu_Mac::sInstallMenuBar(inMenuBar);

	map<pair<short, short>, ZRef<ZMenuItem> > theMenuMap;
	bool anySubMenuParentEnabled = false;
	sPostSetup(inMenuBar, inOutAppleMenuItemCount, theMenuMap, anySubMenuParentEnabled);

#if MERCUTIO_ENABLED
	sCurrentMenuItemMap = &theMenuMap;
#endif // MERCUTIO_ENABLED

// If any item with a sub menu was explicitly enabled (as  opposed to just enabled because any of its sub menu's
// items were enabled) then use the LMSetMenuDisable hack to allow the user to choose that item. Normally
// such items are not choosable. cf  Hit-Developers Digest, 1999-12-23, vol 1 no. 98.
	if (anySubMenuParentEnabled)
		LMSetMenuDisable(0L);

	long theResult = ::MenuSelect(whereClicked);

	if (anySubMenuParentEnabled)
		theResult = LMGetMenuDisable();

#if MERCUTIO_ENABLED
	sCurrentMenuItemMap = nil;
#endif // MERCUTIO_ENABLED

	short resultMenuID = (((theResult) >> 16) & 0xFFFF);
	short resultItemIndex = ((theResult) & 0xFFFF);

	if (ZRef<ZMenuItem> theItem = sFindItemForResult(resultMenuID, resultItemIndex, inOutAppleMenuItemCount, theMenuMap))
		{
		if (theItem->GetEnabled())
			return theItem;
		}
	return ZRef<ZMenuItem>();
	}

ZRef<ZMenuItem> ZMenu_Mac::sPostSetupAndHandleCommandKey(const ZMenuBar& inMenuBar, const EventRecord& inEventRecord, size_t& inOutAppleMenuItemCount)
	{
	ZMenu_Mac::sInstallMenuBar(inMenuBar);

	map<pair<short, short>, ZRef<ZMenuItem> > theMenuMap;
	bool dummy = false;
	sPostSetup(inMenuBar, inOutAppleMenuItemCount, theMenuMap, dummy);
	long theResult;
	if (sHasAppearance())
		theResult = ::MenuEvent(&inEventRecord);
#if MERCUTIO_ENABLED
	else if (sHasMercutio())
		theResult = ::MDEF_MenuKey(inEventRecord.message, inEventRecord.modifiers, sMercutioHelperMenuHandle);
#endif // MERCUTIO_ENABLED
	else
		theResult = ::MenuKey(inEventRecord.message & charCodeMask);

	short resultMenuID = (((theResult) >> 16) & 0xFFFF);
	short resultItemIndex = ((theResult) & 0xFFFF);
	return sFindItemForResult(resultMenuID, resultItemIndex, inOutAppleMenuItemCount, theMenuMap);
	}

// ==================================================

ZRef<ZMenuItem> ZMenuPopup::DoPopup(const ZPoint& inGlobalLocation, int32 inAlignedItem)
	{
	MenuHandle theMenuHandle = ::sNewMenuHierarchical();

	if (inAlignedItem < 0 || inAlignedItem >= fMenu->Count())
// If the item is out of range, use zero
		inAlignedItem = 0;
	else
// Otherwise use the value (converted to a one based index)
		inAlignedItem += 1;

	map<pair<short, short>, ZRef<ZMenuItem> > theMenuMap;
	bool anySubMenuParentEnabled = false;
	::sSetupMenu(fMenu, theMenuHandle, theMenuMap, anySubMenuParentEnabled);

#if MERCUTIO_ENABLED
	sCurrentMenuItemMap = &theMenuMap;
#endif // MERCUTIO_ENABLED

	if (anySubMenuParentEnabled)
		LMSetMenuDisable(0L);

	long theResult = ::PopUpMenuSelect(theMenuHandle, inGlobalLocation.v, inGlobalLocation.h, inAlignedItem);

	if (anySubMenuParentEnabled)
		theResult = LMGetMenuDisable();

#if MERCUTIO_ENABLED
	sCurrentMenuItemMap = nil;
#endif // MERCUTIO_ENABLED

	short resultMenuID = (((theResult) >> 16) & 0xFFFF);
	short resultItem = ((theResult) & 0xFFFF);

	ZRef<ZMenuItem> foundItem;

	map<pair<short, short>, ZRef<ZMenuItem> >::const_iterator theIter = theMenuMap.find(make_pair(resultMenuID, resultItem));
	if (theIter != theMenuMap.end())
		{
		if (theIter->second->GetEnabled())
			foundItem = theIter->second;
		}
	::sDeleteMenuRecursive(theMenuHandle);
	return foundItem;
	}
// ==================================================

#endif // ZCONFIG(OS, MacOS7)
// =================================================================================================
