static const char rcsid[] = "@(#) $Id: ZMenu_Be.cpp,v 1.4 2006/04/10 20:44:20 agreen Exp $";

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

#include "ZMenu_Be.h"

// =================================================================================================
#if ZCONFIG(OS, Be)
#include "ZMenu.h"

#include <interface/MenuItem.h>
#include <interface/MenuBar.h>
#include <interface/PopUpMenu.h>

// ==================================================

static string sStripEmbeddedChars(const string& source)
	{
	string returnedString;
	for (size_t index = 0; index < source.length(); ++index)
		{
		if (source[index] == '&')
			continue;
		if (source[index] == 0xC9)
			{
			// It's a mac ellipsis.
			returnedString += "...";
			continue;
			}
		returnedString += source[index];
		}
	return returnedString;
	}

class PopupMenuItem : public BMenuItem
	{
public:
	PopupMenuItem(BMenu* inBMenu, const ZRef<ZMenuItem>& inMenuItem);
	virtual ~ PopupMenuItem();

	ZRef<ZMenuItem> GetMenuItem();
protected:
	ZRef<ZMenuItem> fMenuItem;
	};

static bool sSetupMenu(ZRef<ZMenu> inMenu, BMenu* inBMenu);
static bool sSetupItem(const ZRef<ZMenuItem>& inMenuItem, BMenu* inBMenu, size_t inMenuItemIndex)
	{
	string theText = sStripEmbeddedChars(inMenuItem->GetText());

	BMenuItem* theBMenuItem = inBMenu->ItemAt(inMenuItemIndex);

	bool isEnabled = inMenuItem->GetEnabled();
	bool isText = (theText.size() > 0);
	if (isText)
		{
		if (ZRef<ZMenu> theSubMenu = inMenuItem->GetSubMenu())
			{
			BMenu* theSubBMenu = theBMenuItem->Submenu();
			if (theSubBMenu == nil)
				{
				theSubBMenu = new BMenu("dummy");
				inBMenu->RemoveItem(inMenuItemIndex);
				delete theBMenuItem;
				theBMenuItem = new BMenuItem(theSubBMenu, nil);
				inBMenu->AddItem(theBMenuItem, inMenuItemIndex);
				}
			isEnabled = sSetupMenu(theSubMenu, theSubBMenu);
			}
		else
			{
			if (theBMenuItem->Submenu() != nil || dynamic_cast<BSeparatorItem*>(theBMenuItem) != nil)
				{
				inBMenu->RemoveItem(inMenuItemIndex);
				delete theBMenuItem;
				theBMenuItem = new BMenuItem("dummy", nil);
				inBMenu->AddItem(theBMenuItem, inMenuItemIndex);
				}
			}
		BMessage* theMessage = new BMessage('ZLib');
		theMessage->AddString("Type", "ZMenuItem");
		theMessage->AddPointer("ZMenuItem", static_cast<void*>(inMenuItem.GetObject()));
		theBMenuItem->SetMessage(theMessage);
		ZMenuAccel theAccel = inMenuItem->GetAccel();

		if (theAccel.fKeyCode == 0)
			theBMenuItem->SetShortcut(0, 0);
		else
			{
			uint32 theModifiers = 0;
			if ((theAccel.fModifiers & ZMenuAccel::eCommandKey) != 0)
				theModifiers |= B_COMMAND_KEY;
			if ((theAccel.fModifiers & ZMenuAccel::eShiftKey) != 0)
				theModifiers |= B_SHIFT_KEY;
			if ((theAccel.fModifiers & ZMenuAccel::eOptionKey) != 0)
				theModifiers |= B_OPTION_KEY;
// We haven't defined a generic control key modifier yet
//			if ((theAccel.fModifiers & ZMenuAccel::eControlKey) != 0)
//				theModifiers |= B_CONTROL_KEY;
			theBMenuItem->SetShortcut(theAccel.fKeyCode, theModifiers);
			}

		theBMenuItem->SetLabel(theText.c_str());
		theBMenuItem->SetEnabled(isEnabled);
		theBMenuItem->SetMarked(inMenuItem->GetChecked());
		}
	else
		{
		isEnabled = false;
		if (dynamic_cast<BSeparatorItem*>(theBMenuItem) == nil)
			{
			inBMenu->RemoveItem(inMenuItemIndex);
			delete theBMenuItem;
			theBMenuItem = new BSeparatorItem;
			inBMenu->AddItem(theBMenuItem, inMenuItemIndex);
			}
		theBMenuItem->SetMessage(nil);
		}
	return isEnabled;
	}

static bool sSetupMenu(ZRef<ZMenu> inMenu, BMenu* inBMenu)
	{
	ZAssertStop(0, inMenu);
	ZAssertStop(0, inBMenu);

	bool anyEnabled = false;

	while (inMenu->Count() > inBMenu->CountItems())
		{
		inBMenu->AddItem(new BMenuItem("dummy", nil));
		}
	while (inMenu->Count() < inBMenu->CountItems())
		{
		delete inBMenu->RemoveItem(inBMenu->CountItems()-1);
		}
	for (size_t x = 0; x < inMenu->Count(); ++x)
		{
		if (::sSetupItem(inMenu->At(x), inBMenu, x))
			anyEnabled = true;
		}
	return anyEnabled;
	}

// ==================================================

void ZMenu_Be::sInstallMenus(const ZMenuBar& inMenuBar, BMenuBar* inBMenuBar)
	{
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	const vector<string>& titleVector = inMenuBar.GetTitleVector();

	for (size_t x = 0; x < titleVector.size(); ++x)
		{
		string theTitle = sStripEmbeddedChars(titleVector[x]);
		if (x >= inBMenuBar->CountItems())
			{
// If we have more titles than menu handles installed, then append a menu handle
			inBMenuBar->AddItem(new BMenu(theTitle.c_str()));
			}
		else
			{
			inBMenuBar->ItemAt(x)->SetLabel(theTitle.c_str());
			}
		}

	while (inBMenuBar->CountItems() > titleVector.size())
		delete inBMenuBar->RemoveItem(inBMenuBar->CountItems() - 1);
	}

void ZMenu_Be::sPostSetup(const ZMenuBar& inMenuBar, BMenuBar* inBMenuBar)
	{
	const vector<ZRef<ZMenu> >& menuVector = inMenuBar.GetMenuVector();
	for (size_t x = 0; x < menuVector.size(); ++x)
		::sSetupMenu(menuVector[x], inBMenuBar->SubmenuAt(x));
	}

// ==================================================

ZRef<ZMenuItem> ZMenuPopup::DoPopup(const ZPoint& inGlobalLocation, int32 inAlignedItem)
	{
	BPopUpMenu theBPopupMenu("ZMenuPopup", false, false);
//	theBPopupMenu.SetFont(be_bold_font);
	sSetupMenu(fMenu, &theBPopupMenu);
	BMenuItem* theBMenuItem = theBPopupMenu.Go(inGlobalLocation, false, true, false);
	if (theBMenuItem)
		{
		if (BMessage* theMessage = theBMenuItem->Message())
			{
			if (theMessage->what == 'ZLib')
				{
				const char* theType;
				if (theMessage->FindString("Type", &theType) == B_OK)
					{
					if (::strcmp(theType, "ZMenuItem") == 0)
						{
						void* theVoidPtr;
						if (theMessage->FindPointer("ZMenuItem", &theVoidPtr) == B_OK)
							{
							if (ZRef<ZMenuItem> theMenuItem = static_cast<ZMenuItem*>(theVoidPtr))
								return theMenuItem;
							}
						}
					}
				}
			}
		}
	return nil;
	}

// ==================================================

#endif // ZCONFIG(OS, Be)
// =================================================================================================
