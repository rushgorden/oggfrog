/*  @(#) $Id: ZUIFactory.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUIFactory__
#define __ZUIFactory__ 1
#include "zconfig.h"

#include "ZUI.h"
#include <map>

// AG 98-06-21. ZUIFactory is now a base class and only creates controls. ZUIAttributeFactory
// hands out fonts, colors etc. It too is an abstract base class. You will need to instantiate a
// ZUIFactory and a ZUIAttributeFactory in your app.
// AG 98-08-30. After some thought, I've decided that attribute factories should be queried
// by passing the textual name of the property you're after. I've left the accessors that did
// exist in place, and they now just call through with the appropriate property name.

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFactory

class ZUIFactory : public ZRefCounted
	{
protected:
	ZUIFactory();

public:
	virtual ~ZUIFactory();

	static void sSet(ZRef<ZUIFactory> inFactory);
	static ZRef<ZUIFactory> sGet();

// Controls
// Version that takes a string -- default implementation calls Make_CaptionText, and caption version of same routine
	virtual ZUICaptionPane* Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const string& inLabel);
	virtual ZSubPane* Make_StaticText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const string& inLabel);

	virtual ZUIButton* Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);
	virtual ZUIButton* Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);
	virtual ZUIButton* Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);
	virtual ZUIButton* Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);
	virtual ZUIButton* Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);
	virtual ZUIButton* Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel);

// Controls
// Version that takes a caption. Generally you must provide an appropriate override.
	virtual ZUICaptionPane* Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption);

	virtual ZUIButton* Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;
	virtual ZUIButton* Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;
	virtual ZUIButton* Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;
	virtual ZUIButton* Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;
	virtual ZUIButton* Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;
	virtual ZUIButton* Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder) = 0;
	virtual ZUIButton* Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption) = 0;

	virtual ZUIScrollBar* Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool inWithBorder = false) = 0;
	virtual ZUILittleArrows* Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;
	virtual ZUISlider* Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUIProgressBar* Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;
	virtual ZUIChasingArrows* Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUITabPane* Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUIGroup* Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;
	virtual ZUIGroup* Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUITextPane* Make_TextPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIFocusFrame* Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;
	virtual ZUIFocusFrame* Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;
	virtual ZUIFocusFrame* Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUISplitter* Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

	virtual ZUIDivider* Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator) = 0;

protected:
	static ZRef<ZUIFactory> sUIFactory;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFactory_Indirect

class ZUIFactory_Indirect : public ZUIFactory
	{
public:
	ZUIFactory_Indirect(ZRef<ZUIFactory> realFactory);
	virtual ~ZUIFactory_Indirect();

	ZRef<ZUIFactory> GetRealFactory();
	void SetRealFactory(ZRef<ZUIFactory> realFactory);

// Controls
	virtual ZUICaptionPane* Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption);

	virtual ZUIButton* Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder);
	virtual ZUIButton* Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);

	virtual ZUITabPane* Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIScrollBar* Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool inWithBorder);
	virtual ZUILittleArrows* Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUISlider* Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIProgressBar* Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIChasingArrows* Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIGroup* Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIGroup* Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIFocusFrame* Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUISplitter* Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIDivider* Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

protected:
	ZRefSafe<ZUIFactory> fRealFactory;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIFactory_FallBack

class ZUIFactory_FallBack : public ZUIFactory
	{
public:
	ZUIFactory_FallBack(const vector<ZRef<ZUIFactory> >& inFactories);
	virtual ~ZUIFactory_FallBack();

// Controls
	virtual ZUICaptionPane* Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption);

	virtual ZUIButton* Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);
	virtual ZUIButton* Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder);
	virtual ZUIButton* Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption);

	virtual ZUITabPane* Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIScrollBar* Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool inWithBorder);
	virtual ZUILittleArrows* Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUISlider* Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIProgressBar* Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIChasingArrows* Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIGroup* Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIGroup* Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIFocusFrame* Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);
	virtual ZUIFocusFrame* Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUISplitter* Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

	virtual ZUIDivider* Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator);

protected:
	vector<ZRef<ZUIFactory> > fFactories;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory

class ZUIAttributeFactory : public ZRefCounted
	{
protected:
	ZUIAttributeFactory();

public:
	virtual ~ZUIAttributeFactory();

	static void sSet(ZRef<ZUIAttributeFactory> inFactory);
	static ZRef<ZUIAttributeFactory> sGet();

	virtual string GetName() = 0;

	virtual ZRef<ZUIColor> GetColor(const string& inColorName);
	virtual ZRef<ZUIInk> GetInk(const string& inInkName);
	virtual ZRef<ZUIFont> GetFont(const string& inFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& inMetricName);
	virtual bool GetAttribute(const string& inAttributeName, void* outAttributeResult);

	virtual ZRef<ZUIIconRenderer> GetIconRenderer();

// Old calls for backward compatibility
	ZRef<ZUIInk> GetInk_WindowBackground_Document();
	ZRef<ZUIInk> GetInk_WindowBackground_Dialog();
	ZRef<ZUIInk> GetInk_BackgroundHighlight();
	ZRef<ZUIInk> GetInk_List_Background();
	ZRef<ZUIInk> GetInk_List_Separator();

	ZRef<ZUIColor> GetColor_Text();
	ZRef<ZUIColor> GetColor_TextHighlight();

	ZRef<ZUIFont> GetFont_SystemLarge();
	ZRef<ZUIFont> GetFont_SystemSmall();
	ZRef<ZUIFont> GetFont_SystemSmallEmphasized();
	ZRef<ZUIFont> GetFont_Application();
	ZRef<ZUIFont> GetFont_Monospaced();

protected:
	static ZRef<ZUIAttributeFactory> sUIAttributeFactory;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Caching

class ZUIAttributeFactory_Caching : public ZUIAttributeFactory
	{
protected:
	ZUIAttributeFactory_Caching();

public:
	virtual ~ZUIAttributeFactory_Caching();

	virtual ZRef<ZUIColor> GetColor(const string& inColorName);
	virtual ZRef<ZUIInk> GetInk(const string& inInkName);
	virtual ZRef<ZUIFont> GetFont(const string& inFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& inMetricName);

	virtual ZRef<ZUIColor> MakeColor(const string& inColorName);
	virtual ZRef<ZUIInk> MakeInk(const string& inInkName);
	virtual ZRef<ZUIFont> MakeFont(const string& inFontName);
	virtual ZRef<ZUIMetric> MakeMetric(const string& inMetricName);

protected:
	ZMutex fMutex;
	map<string, void*, less<string> > fMap_Color;
	map<string, void*, less<string> > fMap_Ink;
	map<string, void*, less<string> > fMap_Font;
	map<string, void*, less<string> > fMap_Metric;
	};


// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Indirect

class ZUIAttributeFactory_Indirect : public ZUIAttributeFactory
	{
public:
	ZUIAttributeFactory_Indirect(ZRef<ZUIAttributeFactory> inRealFactory);
	virtual ~ZUIAttributeFactory_Indirect();

	ZRef<ZUIAttributeFactory> GetRealFactory();
	void SetRealFactory(ZRef<ZUIAttributeFactory> inRealFactory);

	virtual ZRef<ZUIColor> GetColor(const string& inColorName);
	virtual ZRef<ZUIInk> GetInk(const string& inInkName);
	virtual ZRef<ZUIFont> GetFont(const string& inFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& inMetricName);
	virtual bool GetAttribute(const string& inAttributeName, void* outAttributeResult);

	virtual ZRef<ZUIIconRenderer> GetIconRenderer();

protected:
	ZRefSafe<ZUIAttributeFactory> fRealFactory;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_FallBack

class ZUIAttributeFactory_FallBack : public ZUIAttributeFactory
	{
public:
	ZUIAttributeFactory_FallBack(const vector<ZRef<ZUIAttributeFactory> > inFactories);
	virtual ~ZUIAttributeFactory_FallBack();

	virtual ZRef<ZUIColor> GetColor(const string& inColorName);
	virtual ZRef<ZUIInk> GetInk(const string& inInkName);
	virtual ZRef<ZUIFont> GetFont(const string& inFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& inMetricName);
	virtual bool GetAttribute(const string& inAttributeName, void* outAttributeResult);

	virtual ZRef<ZUIIconRenderer> GetIconRenderer();

protected:
	vector<ZRef<ZUIAttributeFactory> > fFactories;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_FallBack

class ZUIAssetFactory : public ZRefCounted
	{
protected:
	ZUIAssetFactory();

public:
	virtual ~ZUIAssetFactory();

	static void sSet(ZRef<ZUIAssetFactory> inFactory);
	static ZRef<ZUIAssetFactory> sGet();

	};

// =================================================================================================

#endif // __ZUIFactory__
