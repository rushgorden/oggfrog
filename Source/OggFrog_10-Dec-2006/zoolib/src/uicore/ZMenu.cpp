static const char rcsid[] = "@(#) $Id: ZMenu.cpp,v 1.6 2006/07/12 19:41:08 agreen Exp $";

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

#include "ZMenu.h"

#include "ZMenuDef.h"
#include "ZString.h"
#include "ZMemory.h" // For ZBlockSet

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuAccel

#if ZCONFIG(OS, Win32)

string ZMenuAccel::GetAbbreviation() const
	{
	string abbreviation;
	if (fKeyCode != 0)
		{
		if ((fModifiers & eCommandKey) || (fModifiers & eControlKeyWin))
			abbreviation += "Ctrl+";
		if ((fModifiers & eShiftKey) || (fModifiers & eShiftKeyWin))
			abbreviation += "Shift+";
		if ((fModifiers & eOptionKey) || (fModifiers & eAltKeyWin))
			abbreviation += "Alt+";
		abbreviation += char(fKeyCode);
		}
	return abbreviation;
	}

#endif // ZCONFIG(OS, Win32)

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuItem

ZMenuItem::ZMenuItem()
	{
	fChangeCount = 0;

	fCommand = mcNoCommand;

	fEnabled_Default = false;
	fEnabled = true;

	fChecked_Default = false;
	fChecked = false;

	fStyle_Default = ZDCFont::normal;
	fStyle = ZDCFont::normal;
	}

ZMenuItem::ZMenuItem(int32 inMenuCommand, const string& inText)
	{
	fChangeCount = 0;

	fCommand = inMenuCommand;

	fEnabled_Default = false;
	fEnabled = true;

	fChecked_Default = false;
	fChecked = false;

	fText_Default = inText;
	fText = inText;

	fStyle_Default = ZDCFont::normal;
	fStyle = ZDCFont::normal;
	}

ZMenuItem::ZMenuItem(int32 inMenuCommand, const ZMessage& inMessage, const string& inText)
	{
	fChangeCount = 0;

	fCommand = inMenuCommand;

	fEnabled_Default = false;
	fEnabled = true;

	fChecked_Default = false;
	fChecked = false;

	fMessage_Default = inMessage;
	fMessage = inMessage;

	fText_Default = inText;
	fText = inText;

	fStyle_Default = ZDCFont::normal;
	fStyle = ZDCFont::normal;
	}

ZMenuItem::ZMenuItem(int32 inMenuCommand, const string& inText, const ZMenuAccel& inAccel)
	{
	fChangeCount = 0;

	fCommand = inMenuCommand;

	fEnabled_Default = false;
	fEnabled = true;

	fChecked_Default = false;
	fChecked = false;

	fText_Default = inText;
	fText = inText;

	fStyle_Default = ZDCFont::normal;
	fStyle = ZDCFont::normal;

	fAccel_Default = inAccel;
	fAccel = inAccel;
	}

ZMenuItem::ZMenuItem(int32 inMenuCommand, const ZMessage& inMessage, const string& inText, const ZMenuAccel& inAccel)
	{
	fChangeCount = 0;

	fCommand = inMenuCommand;

	fEnabled_Default = false;
	fEnabled = true;

	fChecked_Default = false;
	fChecked = false;

	fMessage_Default = inMessage;
	fMessage = inMessage;

	fText_Default = inText;
	fText = inText;

	fStyle_Default = ZDCFont::normal;
	fStyle = ZDCFont::normal;

	fAccel_Default = inAccel;
	fAccel = inAccel;
	}

void ZMenuItem::SetCommand(int32 inMenuCommand)
	{ fCommand = inMenuCommand; }

int32 ZMenuItem::GetCommand()
	{ return fCommand; }

void ZMenuItem::SetDefaultEnabled(bool inEnabled)
	{
	if (fEnabled_Default == inEnabled && fEnabled == inEnabled)
		return;
	++fChangeCount;
	fEnabled_Default = inEnabled;
	fEnabled = inEnabled;
	}

void ZMenuItem::SetEnabled(bool inEnabled)
	{
	if (fEnabled == inEnabled)
		return;
	++fChangeCount;
	fEnabled = inEnabled;
	}

bool ZMenuItem::GetEnabled()
	{ return fEnabled; }

void ZMenuItem::SetDefaultChecked(bool inChecked)
	{
	if (fChecked_Default == inChecked && fChecked == inChecked)
		return;
	++fChangeCount;
	fChecked_Default = inChecked;
	fChecked = inChecked;
	}

void ZMenuItem::SetChecked(bool inChecked)
	{
	if (fChecked == inChecked)
		return;
	++fChangeCount;
	fChecked = inChecked;
	}

bool ZMenuItem::GetChecked()
	{ return fChecked; }

void ZMenuItem::SetDefaultMessage(const ZMessage& inMessage)
	{
//	if (fMessage_Default == fMessage && fMessage == inMenuMessage)
//		return;
	++fChangeCount;
	fMessage_Default = inMessage;
	fMessage = inMessage;
	}
void ZMenuItem::SetMessage(const ZMessage& inMessage)

	{
//	if (fMessage == inMessage)
//		return;
	++fChangeCount;
	fMessage = inMessage;
	}

ZMessage ZMenuItem::GetMessage()
	{ return fMessage; }

void ZMenuItem::SetDefaultText(const string& inText)
	{
	if (fText_Default == inText && fText == inText)
		return;
	++fChangeCount;
	fText_Default = inText;
	fText = inText;
	}

void ZMenuItem::SetText(const string& inText)
	{
	if (fText == inText)
		return;
	++fChangeCount;
	fText = inText;
	}

string ZMenuItem::GetText()
	{ return fText; }

void ZMenuItem::SetDefaultStyle(ZDCFont::Style inStyle)
	{
	if (fStyle_Default == inStyle && fStyle == inStyle)
		return;
	++fChangeCount;
	fStyle_Default = inStyle;
	fStyle = inStyle;
	}

void ZMenuItem::SetStyle(ZDCFont::Style inStyle)
	{
	if (fStyle == inStyle)
		return;
	++fChangeCount;
	fStyle = inStyle;
	}

ZDCFont::Style ZMenuItem::GetStyle()
	{ return fStyle; }

void ZMenuItem::SetDefaultAccel(const ZMenuAccel& inMenuAccel)
	{
	if (fAccel_Default == inMenuAccel && fAccel == inMenuAccel)
		return;
	++fChangeCount;
	fAccel_Default = inMenuAccel;
	fAccel = inMenuAccel;
	}

void ZMenuItem::SetAccel(const ZMenuAccel& inMenuAccel)
	{
	if (fAccel == inMenuAccel)
		return;
	++fChangeCount;
	fAccel = inMenuAccel;
	}

ZMenuAccel ZMenuItem::GetAccel()
	{ return fAccel; }

void ZMenuItem::SetDefaultPixmapCombo(const ZDCPixmapCombo& inPixmapCombo)
	{
	if ((fPixmapCombo_Default == inPixmapCombo) && (fPixmapCombo == inPixmapCombo))
		return;
	++fChangeCount;
	fPixmapCombo_Default = inPixmapCombo;
	fPixmapCombo = inPixmapCombo;
	}

void ZMenuItem::SetPixmapCombo(const ZDCPixmapCombo& inPixmapCombo)
	{
	if (fPixmapCombo == inPixmapCombo)
		return;
	++fChangeCount;
	fPixmapCombo = inPixmapCombo;
	}

ZDCPixmapCombo ZMenuItem::GetPixmapCombo()
	{ return fPixmapCombo; }

void ZMenuItem::SetDefaultSubMenu(const ZRef<ZMenu>& inMenu)
	{
	if (fSubMenu_Default == inMenu && fSubMenu == inMenu)
		return;
	++fChangeCount;
	fSubMenu_Default = inMenu;
	fSubMenu = inMenu;
	}

void ZMenuItem::SetSubMenu(const ZRef<ZMenu>& inMenu)
	{
	if (fSubMenu == inMenu)
		return;
	++fChangeCount;
	fSubMenu = inMenu;
	}

ZRef<ZMenu> ZMenuItem::GetSubMenu()
	{ return fSubMenu; }

long ZMenuItem::GetChangeCount()
	{ return fChangeCount; }

void ZMenuItem::Reset()
	{
	bool changed = false;
	changed |= (fEnabled != fEnabled_Default);
	changed |= (fChecked != fChecked_Default);
	changed |= (fMessage != fMessage_Default);
	changed |= (fText != fText_Default);
	changed |= (fStyle != fStyle_Default);
	changed |= (fPixmapCombo != fPixmapCombo_Default);
	changed |= (fAccel != fAccel_Default);
	changed |= (fSubMenu != fSubMenu_Default);

	if (changed)
		++fChangeCount;

	fEnabled = fEnabled_Default;
	fChecked = fChecked_Default;
	fMessage = fMessage_Default;
	fText = fText_Default;
	fStyle = fStyle_Default;
	fPixmapCombo = fPixmapCombo_Default;
	fAccel = fAccel_Default;
	fSubMenu = fSubMenu_Default;
	if (fSubMenu)
		fSubMenu->Reset();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu

ZMenu::ZMenu()
	{}

ZMenu::~ZMenu()
	{}

ZRef<ZMenuItem> ZMenu::Append(const ZRef<ZMenuItem>& inMenuItem)
	{
	fItemVector.push_back(inMenuItem);
	return inMenuItem;
	}

ZRef<ZMenuItem> ZMenu::Append(int32 inMenuCommand, const string& inText)
	{
	ZRef<ZMenuItem> theMenuItem = new ZMenuItem(inMenuCommand, inText);
	fItemVector.push_back(theMenuItem);
	return theMenuItem;
	}

ZRef<ZMenuItem> ZMenu::Append(int32 inMenuCommand, const string& inText, const ZMenuAccel& inAccel)
	{
	ZRef<ZMenuItem> theMenuItem = new ZMenuItem(inMenuCommand, inText, inAccel);
	fItemVector.push_back(theMenuItem);
	return theMenuItem;
	}

ZRef<ZMenuItem> ZMenu::AppendSeparator()
	{
	ZRef<ZMenuItem> theMenuItem = new ZMenuItem;
	fItemVector.push_back(theMenuItem);
	return theMenuItem;
	}

ZRef<ZMenuItem> ZMenu::Append(const string& inText, const ZRef<ZMenu>& inMenu)
	{
	ZRef<ZMenuItem> theMenuItem = new ZMenuItem;
	theMenuItem->SetDefaultText(inText);
	theMenuItem->SetDefaultSubMenu(inMenu);
	fItemVector.push_back(theMenuItem);
	return theMenuItem;
	}

ZRef<ZMenuItem> ZMenu::Append(int32 inMenuCommand, const string& inText, const ZRef<ZMenu>& inMenu)
	{
	ZRef<ZMenuItem> theMenuItem = new ZMenuItem;
	theMenuItem->SetCommand(inMenuCommand);
	theMenuItem->SetDefaultText(inText);
	theMenuItem->SetDefaultSubMenu(inMenu);
	fItemVector.push_back(theMenuItem);
	return theMenuItem;
	}

size_t ZMenu::Count()
	{ return fItemVector.size(); }

ZRef<ZMenuItem> ZMenu::At(size_t inIndex)
	{ return fItemVector[inIndex]; }

void ZMenu::Remove(size_t inIndex)
	{ fItemVector.erase(fItemVector.begin()+inIndex); }

void ZMenu::Remove(const ZRef<ZMenuItem>& inMenuItem)
	{
	vector<ZRef<ZMenuItem> >::iterator theIter = find(fItemVector.begin(), fItemVector.end(), inMenuItem);
	if (theIter != fItemVector.end())
		fItemVector.erase(theIter);
	}

void ZMenu::RemoveAll()
	{ fItemVector.erase(fItemVector.begin(), fItemVector.end()); }

size_t ZMenu::IndexOf(const ZRef<ZMenuItem>& inMenuItem)
	{
// If we don't find inMenuItem, we'll return the index of one past the end of our vector 
	vector<ZRef<ZMenuItem> >::iterator theIter = find(fItemVector.begin(), fItemVector.end(), inMenuItem);
	return theIter - fItemVector.begin();
	}

void ZMenu::InsertAt(size_t inIndex, const ZRef<ZMenuItem>& inMenuItem)
	{ fItemVector.insert(fItemVector.begin()+inIndex, inMenuItem); }

ZRef<ZMenuItem> ZMenu::GetMenuItem(int32 inMenuCommand)
	{
	for (vector<ZRef<ZMenuItem> >::iterator i = fItemVector.begin(); i != fItemVector.end(); ++i)
		{
		if ((*i)->GetCommand() == inMenuCommand)
			return *i;
		if (ZRef<ZMenu> theSubMenu = (*i)->GetSubMenu())
			{
			if (ZRef<ZMenuItem> theItem = theSubMenu->GetMenuItem(inMenuCommand))
				return theItem;
			}
		}
	return ZRef<ZMenuItem>();
	}

void ZMenu::GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector)
	{
	for (vector<ZRef<ZMenuItem> >::iterator i = fItemVector.begin(); i != fItemVector.end(); ++i)
		{
		if ((*i)->GetCommand() == inMenuCommand)
			outVector.push_back(*i);
		if (ZRef<ZMenu> theSubMenu = (*i)->GetSubMenu())
			theSubMenu->GetMenuItems(inMenuCommand, outVector);
		}
	}

void ZMenu::Reset()
	{
	for (vector<ZRef<ZMenuItem> >::iterator i = fItemVector.begin(); i != fItemVector.end(); ++i)
		(*i)->Reset();
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuSetup

ZMenuSetup::ZMenuSetup()
	{}

ZMenuSetup::ZMenuSetup(const ZMenuSetup& inOther)
	{}

ZMenuSetup::~ZMenuSetup()
	{}

ZMenuSetup& ZMenuSetup::operator=(const ZMenuSetup& inOther)
	{ return *this; }

void ZMenuSetup::EnableDisableCheckMenuItem(int32 inMenuCommand, bool inEnable, bool inCheckIt)
	{
	if (ZRef<ZMenuItem> theMenuItem = this->GetMenuItem(inMenuCommand))
		{
		theMenuItem->SetEnabled(inEnable);
		theMenuItem->SetChecked(inCheckIt);
		}
	}

void ZMenuSetup::SetItemText(int32 inMenuCommand, const string& inText)
	{
	if (ZRef<ZMenuItem> theMenuItem = this->GetMenuItem(inMenuCommand))
		theMenuItem->SetText(inText);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuInstall

ZMenuInstall::ZMenuInstall()
	{}

ZMenuInstall::ZMenuInstall(const ZMenuInstall& inOther)
	{}

ZMenuInstall::~ZMenuInstall()
	{}

ZMenuInstall& ZMenuInstall::operator=(const ZMenuInstall& inOther)
	{ return *this; }

ZRef<ZMenu> ZMenuInstall::GetAppleMenu() const
	{ return ZRef<ZMenu>(); }

ZRef<ZMenu> ZMenuInstall::GetHelpMenu() const
	{ return ZRef<ZMenu>(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuPopup

ZMenuPopup::ZMenuPopup()
	{ fMenu = new ZMenu; }

ZMenuPopup::ZMenuPopup(const ZRef<ZMenu>& inMenu)
:	fMenu(inMenu)
	{}

ZMenuPopup::~ZMenuPopup()
	{}

ZRef<ZMenuItem> ZMenuPopup::GetMenuItem(int32 inMenuCommand)
	{ return fMenu->GetMenuItem(inMenuCommand); }

void ZMenuPopup::GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector)
	{ fMenu->GetMenuItems(inMenuCommand, outVector); }

void ZMenuPopup::Reset()
	{ fMenu->Reset(); }

ZRef<ZMenuItem> ZMenuPopup::Append(const ZRef<ZMenuItem>& inMenuItem)
	{ return fMenu->Append(inMenuItem); }

ZRef<ZMenuItem> ZMenuPopup::AppendSeparator()
	{ return fMenu->AppendSeparator(); }

ZRef<ZMenu> ZMenuPopup::GetMenu()
	{ return fMenu; }

ZRef<ZMenuItem> ZMenuPopup::DoPopup(const ZRef<ZMenuItem>& inAlignedItem, const ZPoint& inGlobalLocation)
	{
	int32 alignedItem = fMenu->IndexOf(inAlignedItem);
	if (alignedItem >= fMenu->Count())
		alignedItem = -1;
	return this->DoPopup(inGlobalLocation, alignedItem);
	}

// ZMenuPopup::DoPopup is defined in ZMenu_Mac, ZMenu_Win, ZMenu_Be or other platform-specific file as appropriate

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuBar

ZMenuBar::ZMenuBar()
	{
	fAppleMenu = new ZMenu;
	fHelpMenu = new ZMenu;
	}

ZMenuBar::~ZMenuBar()
	{}

ZRef<ZMenuItem> ZMenuBar::GetMenuItem(int32 inMenuCommand)
	{
	ZRef<ZMenuItem> theMenuItem = fAppleMenu->GetMenuItem(inMenuCommand);
	if (!theMenuItem)
		theMenuItem = fHelpMenu->GetMenuItem(inMenuCommand);
	if (!theMenuItem)
		{
		for (vector<ZRef<ZMenu> >::iterator i = fMenuVector.begin(); i != fMenuVector.end() && !theMenuItem; ++i)
			theMenuItem = (*i)->GetMenuItem(inMenuCommand);
		}
	return theMenuItem;
	}

void ZMenuBar::GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector)
	{
	fAppleMenu->GetMenuItems(inMenuCommand, outVector);
	fHelpMenu->GetMenuItems(inMenuCommand, outVector);
	for (vector<ZRef<ZMenu> >::iterator i = fMenuVector.begin(); i != fMenuVector.end(); ++i)
		(*i)->GetMenuItems(inMenuCommand, outVector);
	}

void ZMenuBar::Append(const string& inText, const ZRef<ZMenu>& inMenu)
	{
	fTitleVector.push_back(inText);
	fMenuVector.push_back(inMenu);
	}

size_t ZMenuBar::Count()
	{ return fMenuVector.size(); }

ZRef<ZMenu> ZMenuBar::At(size_t inIndex)
	{ return fMenuVector[inIndex]; }

void ZMenuBar::Remove(size_t inIndex)
	{
	fTitleVector.erase(fTitleVector.begin() + inIndex);
	fMenuVector.erase(fMenuVector.begin() + inIndex);
	}

void ZMenuBar::Remove(const ZRef<ZMenu>& inMenu)
	{ this->Remove(this->IndexOf(inMenu)); }

size_t ZMenuBar::IndexOf(const ZRef<ZMenu>& inMenu)
	{ return find(fMenuVector.begin(), fMenuVector.end(), inMenu) - fMenuVector.begin(); }

void ZMenuBar::InsertAt(size_t inIndex, const string& inText, const ZRef<ZMenu>& inMenu)
	{
	fTitleVector.insert(fTitleVector.begin()+inIndex, inText);
	fMenuVector.insert(fMenuVector.begin()+inIndex, inMenu);
	}

ZRef<ZMenu> ZMenuBar::GetAppleMenu() const
	{ return fAppleMenu; }

ZRef<ZMenu> ZMenuBar::GetHelpMenu() const
	{ return fHelpMenu; }

void ZMenuBar::Reset()
	{
	fAppleMenu->Reset();
	fHelpMenu->Reset();
	for (vector<ZRef<ZMenu> >::iterator i = fMenuVector.begin(); i != fMenuVector.end(); ++i)
		(*i)->Reset();
	}

const vector<ZRef<ZMenu> >& ZMenuBar::GetMenuVector() const
	{ return fMenuVector; }

const vector<string>& ZMenuBar::GetTitleVector() const
	{ return fTitleVector; }

// =================================================================================================
