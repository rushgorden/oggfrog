/*  @(#) $Id: ZMenu.h,v 1.5 2006/07/12 19:41:08 agreen Exp $ */

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

#ifndef __ZMenu__
#define __ZMenu__ 1
#include "zconfig.h"

#include "ZRefCount.h"
#include "ZDCFont.h"
#include "ZDCPixmap.h"
#include "ZMessage.h"
#include "ZMenuDef.h"

#include <vector>

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuAccel

class ZMenuAccel
	{
public:
	// We have different range of modifers on Mac and Windows. This
	// set of enums attempts to model that.
	enum
		{
		eCommandKey = 1, // Cmd on Mac, control on Windows
		eShiftKey = 2, // Shift on both
		eOptionKey = 4 // Option on Mac, Alt on Windows

		// Now the platform specific *explicit* modifiers. Using these will give you exactly
		// these modifiers, but you must use conditionally compiled code to access them.
		#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon) || ZCONFIG(OS, MacOSX)
			,eCommandKeyMac = 8,
			eShiftKeyMac = 16,
			eOptionKeyMac = 32,
			eControlKeyMac = 64
		#elif ZCONFIG(OS, Win32)
			,eControlKeyWin = 8,
			eShiftKeyWin = 16,
			eAltKeyWin = 32
		#endif // ZCONFIG_OS
		};

	ZMenuAccel() : fKeyCode(0), fModifiers(0) {}
	ZMenuAccel(short keyCode) : fKeyCode(keyCode), fModifiers(eCommandKey) {}
	ZMenuAccel(short keyCode, short modifiers) : fKeyCode(keyCode), fModifiers(modifiers) {}

	bool operator==(const ZMenuAccel& other) const
		{ return fKeyCode == other.fKeyCode && fModifiers == other.fModifiers; }
	bool operator!=(const ZMenuAccel& other) const
		{ return ! (*this == other); }
	bool operator<(const ZMenuAccel& other) const
		{ return (fKeyCode < other.fKeyCode) || ((fKeyCode == other.fKeyCode) && (fModifiers < other.fModifiers)); }

	#if ZCONFIG(OS, Win32)
		string GetAbbreviation() const;
	#endif

	short fKeyCode;
	short fModifiers;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuItem

class ZMenu;
class ZMenuItem : public ZRefCounted
	{
public:
	ZMenuItem();
	ZMenuItem(int32 inMenuCommand, const string& inText);
	ZMenuItem(int32 inMenuCommand, const ZMessage& inMessage, const string& inText);
	ZMenuItem(int32 inMenuCommand, const string& inText, const ZMenuAccel& inAccel);
	ZMenuItem(int32 inMenuCommand, const ZMessage& inMessage, const string& inText, const ZMenuAccel& inAccel);

	void SetCommand(int32 inMenuCommand);
	int32 GetCommand();

	void SetDefaultEnabled(bool inEnabled);
	void SetEnabled(bool inEnabled);
	bool GetEnabled();

	void SetDefaultChecked(bool inChecked);
	void SetChecked(bool inChecked);
	bool GetChecked();

	void SetDefaultMessage(const ZMessage& inMessage);
	void SetMessage(const ZMessage& inMessage);
	ZMessage GetMessage();

	void SetDefaultText(const string& inText);
	void SetText(const string& inText);
	string GetText();

	void SetDefaultStyle(ZDCFont::Style inStyle);
	void SetStyle(ZDCFont::Style inStyle);
	ZDCFont::Style GetStyle();

	void SetDefaultAccel(const ZMenuAccel& inMenuAccel);
	void SetAccel(const ZMenuAccel& inMenuAccel);
	ZMenuAccel GetAccel();

	void SetDefaultPixmapCombo(const ZDCPixmapCombo& inPixmapCombo);
	void SetPixmapCombo(const ZDCPixmapCombo& inPixmapCombo);
	ZDCPixmapCombo GetPixmapCombo();

	void SetDefaultSubMenu(const ZRef<ZMenu>& inMenu);
	void SetSubMenu(const ZRef<ZMenu>& inMenu);
	ZRef<ZMenu> GetSubMenu();

	long GetChangeCount();
	void Reset();

protected:
	long fChangeCount;
	int32 fCommand;

	bool fEnabled_Default;
	bool fEnabled;

	bool fChecked_Default;
	bool fChecked;

	ZMessage fMessage_Default;
	ZMessage fMessage;

	string fText_Default;
	string fText;

	ZDCFont::Style fStyle_Default;
	ZDCFont::Style fStyle;

	ZDCPixmapCombo fPixmapCombo_Default;
	ZDCPixmapCombo fPixmapCombo;

	ZMenuAccel fAccel_Default;
	ZMenuAccel fAccel;

	ZRef<ZMenu> fSubMenu_Default;
	ZRef<ZMenu> fSubMenu;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenu

class ZMenu : public ZRefCounted
	{
public:
	ZMenu();
	virtual ~ZMenu();

	ZRef<ZMenuItem> Append(const ZRef<ZMenuItem>& inMenuItem);
	ZRef<ZMenuItem> Append(int32 inMenuCommand, const string& inText);
	ZRef<ZMenuItem> Append(int32 inMenuCommand, const string& inText, const ZMenuAccel& inAccel);
	ZRef<ZMenuItem> AppendSeparator();
	ZRef<ZMenuItem> Append(const string& inText, const ZRef<ZMenu>& inMenu);
	ZRef<ZMenuItem> Append(int32 inMenuCommand, const string& inText, const ZRef<ZMenu>& inMenu);

	size_t Count();
	ZRef<ZMenuItem> At(size_t inIndex);
	void Remove(size_t inIndex);
	void Remove(const ZRef<ZMenuItem>& inMenuItem);
	void RemoveAll();
	size_t IndexOf(const ZRef<ZMenuItem>& inMenuItem);

	void InsertAt(size_t inIndex, const ZRef<ZMenuItem>& inMenuItem);

	ZRef<ZMenuItem> GetMenuItem(int32 inMenuCommand);
	void GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector);

	void Reset();

protected:
	vector<ZRef<ZMenuItem> > fItemVector;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuSetup

class ZMenuSetup
	{
protected:
	ZMenuSetup();
	ZMenuSetup(const ZMenuSetup& inOther);
	~ZMenuSetup();
	ZMenuSetup& operator=(const ZMenuSetup& inOther);

public:
	virtual ZRef<ZMenuItem> GetMenuItem(int32 inMenuCommand) = 0;
	virtual void GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector) = 0;
// Old, and deprecated, API. Left in place to support existing source
	void EnableItem(int32 inMenuCommand)
		{ this->EnableDisableCheckMenuItem(inMenuCommand, true, false); }
	void EnableCheckItem(int32 inMenuCommand)
		{ this->EnableDisableCheckMenuItem(inMenuCommand, true, true); }

	void EnableDisableMenuItem(int32 inMenuCommand, bool inEnabled)
		{ this->EnableDisableCheckMenuItem(inMenuCommand, inEnabled, false); }
	void EnableDisableCheckMenuItem(int32 inMenuCommand, bool inEnabled, bool inCheckIt);

	void SetItemText(int32 menuCommand, const string& theText);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuInstall

class ZMenuInstall
	{
protected:
	ZMenuInstall();
	ZMenuInstall(const ZMenuInstall& inOther);
	~ZMenuInstall();
	ZMenuInstall& operator=(const ZMenuInstall& inOther);

public:
	virtual void Append(const string& inText, const ZRef<ZMenu>& inMenu) = 0;
	virtual size_t Count() = 0;
	virtual ZRef<ZMenu> At(size_t inIndex) = 0;
	virtual void Remove(size_t inIndex) = 0;
	virtual void Remove(const ZRef<ZMenu>& inMenu) = 0;
	virtual size_t IndexOf(const ZRef<ZMenu>& inMenu) = 0;

	virtual void InsertAt(size_t inIndex, const string& inText, const ZRef<ZMenu>& inMenu) = 0;

	virtual ZRef<ZMenu> GetAppleMenu() const;
	virtual ZRef<ZMenu> GetHelpMenu() const;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuPopup

class ZMenuPopup : public ZMenuSetup
	{
public:
	ZMenuPopup();
	ZMenuPopup(const ZRef<ZMenu>& inMenu);
	virtual ~ZMenuPopup();

// From ZMenuSetup.
	virtual ZRef<ZMenuItem> GetMenuItem(int32 inMenuCommand);
	virtual void GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector);

// Our protocol
	void Reset();
	ZRef<ZMenuItem> Append(const ZRef<ZMenuItem>& inMenuItem);
	ZRef<ZMenuItem> AppendSeparator();
	ZRef<ZMenu> GetMenu();
	ZRef<ZMenuItem> DoPopup(const ZPoint& inGlobalLocation, int32 inAlignedItem = -1);
	ZRef<ZMenuItem> DoPopup(const ZRef<ZMenuItem>& inAlignedItem, const ZPoint& inGlobalLocation);
protected:
	ZRef<ZMenu> fMenu;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMenuBar

/* ZMenuBar both holds onto a list of menus and their titles, and implements the ZMenuSetup and
ZMenuInstall interfaces. */

class ZMenuBar : public ZMenuSetup, public ZMenuInstall
	{
public:
	ZMenuBar();
	~ZMenuBar();

// From ZMenuSetup
	virtual ZRef<ZMenuItem> GetMenuItem(int32 inMenuCommand);
	virtual void GetMenuItems(int32 inMenuCommand, vector<ZRef<ZMenuItem> >& outVector);

// From ZMenuInstall
	virtual void Append(const string& inText, const ZRef<ZMenu>& inMenu);
	virtual size_t Count();
	virtual ZRef<ZMenu> At(size_t inIndex);
	virtual void Remove(size_t inIndex);
	virtual void Remove(const ZRef<ZMenu>& inMenu);
	virtual size_t IndexOf(const ZRef<ZMenu>& inMenu);

	virtual void InsertAt(size_t inIndex, const string& inText, const ZRef<ZMenu>& inMenu);

	virtual ZRef<ZMenu> GetAppleMenu() const;
	virtual ZRef<ZMenu> GetHelpMenu() const;

// Our protocol
	void Reset();
	const vector<ZRef<ZMenu> >& GetMenuVector() const;
	const vector<string>& GetTitleVector() const;

protected:
	ZRef<ZMenu> fAppleMenu;
	ZRef<ZMenu> fHelpMenu;
	vector<ZRef<ZMenu> > fMenuVector;
	vector<string> fTitleVector;
	};

// =================================================================================================

#endif // __ZMenu__
