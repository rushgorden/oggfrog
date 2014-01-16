static const char rcsid[] = "@(#) $Id: ZFontMenus.cpp,v 1.9 2006/07/12 19:47:32 agreen Exp $";

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

#include "ZFontMenus.h"
#include "ZString.h"
#include "ZUtil_Win.h"

#define ZTr(a) a

#include <set>

static void sMakeFontNamesProc(size_t inCurrentIndex, const string& inFontName, void* inRefCon)
	{
	vector<string>* theVector = reinterpret_cast<vector<string>*>(inRefCon);
	theVector->push_back(inFontName);
	}

void ZFontMenus::sMakeFontNames(vector<string>& outFontNames)
	{
	outFontNames.clear();
	sEnumerateFontNames(sMakeFontNamesProc, &outFontNames);
	}


static void sMakeFontItemsProc(size_t inCurrentIndex, const string& inFontName, void* inRefCon)
	{
	vector<ZRef<ZMenuItem> >* theVector = reinterpret_cast<vector<ZRef<ZMenuItem> >*>(inRefCon);
	ZMessage theMessage;
	theMessage.SetString("fontName", inFontName);
	theVector->push_back(new ZMenuItem(mcFontItem, theMessage, inFontName));
	}

void ZFontMenus::sMakeFontItems(vector<ZRef<ZMenuItem> >& outMenuItems)
	{
	outMenuItems.clear();
	sEnumerateFontNames(sMakeFontItemsProc, &outMenuItems);
	}


static void sAppendFontsProc(size_t inCurrentIndex, const string& inFontName, void* inRefCon)
	{
	ZRef<ZMenu>* theMenu = reinterpret_cast<ZRef<ZMenu>*>(inRefCon);
	ZMessage theMessage;
	theMessage.SetString("fontName", inFontName);
	(*theMenu)->Append(new ZMenuItem(mcFontItem, theMessage, inFontName));
	}

void ZFontMenus::sAppendFonts(ZRef<ZMenu>& inMenu)
	{
	sEnumerateFontNames(sAppendFontsProc, &inMenu);
	}


static void sMakeFontMenuProc(size_t inCurrentIndex, const string& inFontName, void* inRefCon)
	{
	ZRef<ZMenu>* theMenu = reinterpret_cast<ZRef<ZMenu>*>(inRefCon);
	ZMessage theMessage;
	theMessage.SetString("fontName", inFontName);
	(*theMenu)->Append(new ZMenuItem(mcFontItem, theMessage, inFontName));
	}

ZRef<ZMenu> ZFontMenus::sMakeFontMenu()
	{
	ZRef<ZMenu> theMenu = new ZMenu;
	sEnumerateFontNames(sMakeFontMenuProc, &theMenu);
	return theMenu;
	}

ZRef<ZMenu> ZFontMenus::sMakeSizeMenu()
	{
	ZRef<ZMenu> theMenu = new ZMenu;
	ZMessage theMessage;
	theMessage.SetInt32("fontSize", 9);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "9"));
	theMessage.SetInt32("fontSize", 10);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "10"));
	theMessage.SetInt32("fontSize", 12);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "12"));
	theMessage.SetInt32("fontSize", 14);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "14"));
	theMessage.SetInt32("fontSize", 18);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "18"));
	theMessage.SetInt32("fontSize", 24);
	theMenu->Append(new ZMenuItem(mcSizeItem, theMessage, "24"));
	return theMenu;
	}

ZRef<ZMenu> ZFontMenus::sMakeStyleMenu()
	{
	ZRef<ZMenu> theMenu = new ZMenu;

	ZRef<ZMenuItem> theMenuItem;
	ZMessage theMessage;

	theMessage.SetInt32("fontStyle", ZDCFont::normal);
	theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Normal"), 'T');
	theMenu->Append(theMenuItem);

	theMessage.SetInt32("fontStyle", ZDCFont::bold);
	theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Bold"));
	theMenuItem->SetDefaultStyle(ZDCFont::bold);
	theMenu->Append(theMenuItem);

	theMessage.SetInt32("fontStyle", ZDCFont::italic);
	theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Italic"));
	theMenuItem->SetDefaultStyle(ZDCFont::italic);
	theMenu->Append(theMenuItem);

	theMessage.SetInt32("fontStyle", ZDCFont::underline);
	theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Underline"));
	theMenuItem->SetDefaultStyle(ZDCFont::underline);
	theMenu->Append(theMenuItem);

	if (ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon))
		{
		theMessage.SetInt32("fontStyle", ZDCFont::outline);
		theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Outline"));
		theMenuItem->SetDefaultStyle(ZDCFont::outline);
		theMenu->Append(theMenuItem);
	
		theMessage.SetInt32("fontStyle", ZDCFont::shadow);
		theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Shadow"));
		theMenuItem->SetDefaultStyle(ZDCFont::shadow);
		theMenu->Append(theMenuItem);
	
		theMessage.SetInt32("fontStyle", ZDCFont::condense);
		theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Condense"));
		theMenuItem->SetDefaultStyle(ZDCFont::condense);
		theMenu->Append(theMenuItem);
	
		theMessage.SetInt32("fontStyle", ZDCFont::extend);
		theMenuItem = new ZMenuItem(mcStyleItem, theMessage, ZTr("Extend"));
		theMenuItem->SetDefaultStyle(ZDCFont::extend);
		theMenu->Append(theMenuItem);
		}
	return theMenu;
	}

#if ZCONFIG(OS, Win32)
static int CALLBACK sCallbackW(const ENUMLOGFONTEXW* inENUMLOGFONTEXW, const NEWTEXTMETRICEXW* inNEWTEXTMETRICEXW, int inFontType, LPARAM inLPARAM)
	{
	// Filter out raster fonts
	if (inFontType & RASTER_FONTTYPE)
		return 1;

	// Filter out any char set except western or symbol
	if (inENUMLOGFONTEXW->elfLogFont.lfCharSet != ANSI_CHARSET && inENUMLOGFONTEXW->elfLogFont.lfCharSet != SYMBOL_CHARSET)
		return 1;

	vector<string>* theStringVector = reinterpret_cast<vector<string>*>(inLPARAM);
	theStringVector->push_back(ZUnicode::sAsUTF8(inENUMLOGFONTEXW->elfLogFont.lfFaceName));
	return 1;
	}

static int CALLBACK sCallbackA(const ENUMLOGFONTEXA* inENUMLOGFONTEXA, const NEWTEXTMETRICEXA* inNEWTEXTMETRICEXA, int inFontType, LPARAM inLPARAM)
	{
	// Filter out raster fonts
	if (inFontType & RASTER_FONTTYPE)
		return 1;

	// Filter out any char set except western or symbol
	if (inENUMLOGFONTEXA->elfLogFont.lfCharSet != ANSI_CHARSET &&inENUMLOGFONTEXA->elfLogFont.lfCharSet != SYMBOL_CHARSET)
		return 1;

	vector<string>* theStringVector = reinterpret_cast<vector<string>*>(inLPARAM);
	theStringVector->push_back(string(inENUMLOGFONTEXA->elfLogFont.lfFaceName));
	return 1;
	}
#endif // ZCONFIG(OS, Win32)

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
#include <Menus.h>
static vector<string> sCachedFontNames;
static bool sGeneratedFontNamesCache = false;
static ZMutex sMutex_FontNameCache;
#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

void ZFontMenus::sEnumerateFontNames(CallbackProc inCallbackProc, void* inRefCon)
	{
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	if (!sGeneratedFontNamesCache)
		{
		ZMutexLocker locker(sMutex_FontNameCache);
		if (!sGeneratedFontNamesCache)
			{
			if (MenuHandle fontMenuHandle = ::NewMenu(1, "\pFonts"))
				{
				::AppendResMenu(fontMenuHandle, 'FONT');
				short count = ::CountMenuItems(fontMenuHandle);
				for (short x = 1; x <= count; ++x)
					{
					Str255 itemText;
					::GetMenuItemText(fontMenuHandle, x, itemText);
					sCachedFontNames.push_back(ZString::sFromPString(itemText));
					}
				::DisposeMenu(fontMenuHandle);
				}
			sGeneratedFontNamesCache = true;
			}
		}
	for (size_t x = 0; x < sCachedFontNames.size(); ++x)
		inCallbackProc(x, sCachedFontNames[x], inRefCon);

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

#if ZCONFIG(OS, Win32)
	// We accumulate the font names into a vector so that we can sort them before passing them to our caller.
	vector<string> theFontNames;
	if (ZUtil_Win::sUseWAPI())
		{
		LOGFONTW theLOGFONTW;
		theLOGFONTW.lfCharSet = DEFAULT_CHARSET;
		theLOGFONTW.lfFaceName[0] = 0;
		theLOGFONTW.lfPitchAndFamily = 0;
		HDC anIC = ::CreateICW(L"DISPLAY", nil, nil, nil);
		::EnumFontFamiliesExW(anIC, &theLOGFONTW, (FONTENUMPROCW)sCallbackW, reinterpret_cast<LPARAM>(&theFontNames), 0);
		::DeleteDC(anIC);
		}
	else
		{
		LOGFONTA theLOGFONTA;
		theLOGFONTA.lfCharSet = DEFAULT_CHARSET;
		theLOGFONTA.lfFaceName[0] = 0;
		theLOGFONTA.lfPitchAndFamily = 0;
		HDC anIC = ::CreateICA("DISPLAY", nil, nil, nil);
		::EnumFontFamiliesExA(anIC, &theLOGFONTA, (FONTENUMPROCA)sCallbackA, reinterpret_cast<LPARAM>(&theFontNames), 0);
		::DeleteDC(anIC);
		}

	sort(theFontNames.begin(), theFontNames.end());
	for (size_t x = 0; x < theFontNames.size(); ++x)
		inCallbackProc(x, theFontNames[x], inRefCon);
#endif // ZCONFIG(OS, Win32)
	}

// ==================================================

void ZFontMenus::sSetupMenus(ZMenuSetup* inMenuSetup, const vector<ZDCFont>& inFonts)
	{
	// Build the lists of fonts, styles & sizes that apply
	set<string> fontNames;
	set<int32> fontSizes;
	set<int32> fontStyles;
	for (size_t x = 0; x < inFonts.size(); ++x)
		{
		fontNames.insert(inFonts[x].GetName());
		fontSizes.insert(inFonts[x].GetSize());
		fontStyles.insert(inFonts[x].GetStyle());
		}

	vector<ZRef<ZMenuItem> > styleMenuItems;
	inMenuSetup->GetMenuItems(mcStyleItem, styleMenuItems);
	for (size_t x = 0; x < styleMenuItems.size(); ++x)
		{
		int32 currentStyle = styleMenuItems[x]->GetMessage().GetInt32("fontStyle");
		styleMenuItems[x]->SetEnabled(true);
		if (fontStyles.size() > 0)
			{
			if (fontStyles.size() == 1)
				{
				if (currentStyle == ZDCFont::normal)
					{
					if (*fontStyles.begin() == currentStyle)
						styleMenuItems[x]->SetChecked(true);
					}
				else
					{
					if ((*fontStyles.begin() & currentStyle) != 0)
						styleMenuItems[x]->SetChecked(true);
					}
				}
			else
				{
				for (set<int32>::iterator i = fontStyles.begin(); i != fontStyles.end(); ++i)
					{
					if (((*i) & currentStyle) != 0)
						{
// Mark the item with a dash
//						styleMenuItems[x]->SetChecked(true);
						}
					}
				}
			}
		}

	vector<ZRef<ZMenuItem> > sizeMenuItems;
	inMenuSetup->GetMenuItems(mcSizeItem, sizeMenuItems);
	for (size_t x = 0; x < sizeMenuItems.size(); ++x)
		{
		int32 currentSize = sizeMenuItems[x]->GetMessage().GetInt32("fontSize");
		sizeMenuItems[x]->SetEnabled(true);
		if (fontSizes.size() > 0)
			{
			if (fontSizes.size() == 1)
				{
				if (*fontSizes.begin() == currentSize)
					sizeMenuItems[x]->SetChecked(true);
				}
			else
				{
				if (fontSizes.find(currentSize) != fontSizes.end())
					{
					// Mark the item with a dash
//						sizeMenuItems[x]->SetChecked(true);
					}
				}
			}
		}

	vector<ZRef<ZMenuItem> > nameMenuItems;
	inMenuSetup->GetMenuItems(mcFontItem, nameMenuItems);
	for (size_t x = 0; x < nameMenuItems.size(); ++x)
		{
		string currentName = nameMenuItems[x]->GetMessage().GetString("fontName");
		nameMenuItems[x]->SetEnabled(true);
		if (fontNames.size() > 0)
			{
			if (fontNames.size() == 1)
				{
				if (*fontNames.begin() == currentName)
					nameMenuItems[x]->SetChecked(true);
				}
			else
				{
				if (fontNames.find(currentName) != fontNames.end())
					{
					// Mark the item with a dash
//						nameMenuItems[x]->SetChecked(true);
					}
				}
			}
		}
	}
