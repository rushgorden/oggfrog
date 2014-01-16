static const char rcsid[] = "@(#) $Id: ZUIFactory.cpp,v 1.5 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUIFactory.h"

// =================================================================================================
// =================================================================================================
#pragma mark-
#pragma mark * ZUIFactory

ZRef<ZUIFactory> ZUIFactory::sUIFactory;

ZUIFactory::ZUIFactory()
	{
// AG 98-10-11. Call ZUIFactory::sSet to set the UIFactory you want returned from sGet.
	}
ZUIFactory::~ZUIFactory()
	{}

void ZUIFactory::sSet(ZRef<ZUIFactory> inFactory)
	{ sUIFactory = inFactory; }

ZRef<ZUIFactory> ZUIFactory::sGet()
	{
// If sUIFactory is nil then app initialization code has not created a factory, which is now an error. Use
// ZUIUtil::sInitializeUIFactories
	ZAssertStop(2, sUIFactory);
	return sUIFactory;
	}

static ZRef<ZUICaption> sMake_CaptionText(const string& inText)
	{ return new ZUICaption_Text(inText, ZUIAttributeFactory::sGet()->GetFont_SystemLarge(), 0); }

ZUICaptionPane* ZUIFactory::Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const string& inLabel)
	{ return this->Make_CaptionPane(inSuperPane, inLocator, sMake_CaptionText(inLabel)); }

ZSubPane* ZUIFactory::Make_StaticText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, const string& inLabel)
	{ return this->Make_CaptionPane(inSuperPane, inLocator, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonPush(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonRadio(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonCheckbox(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonPopup(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonBevel(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUIButton* ZUIFactory::Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, const string& inLabel)
	{ return this->Make_ButtonPlacard(inSuperPane, inLocator, inResponder, sMake_CaptionText(inLabel)); }

ZUICaptionPane* ZUIFactory::Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption)
	{ return new ZUICaptionPane(inSuperPane, inLocator, inCaption); }

#include "ZUITextEngine_Mac.h"

//#define UseZUITextEngine_Z 1

#ifndef UseZUITextEngine_Z
//	#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
//		#define UseZUITextEngine_Z 0
//	#else
		#define UseZUITextEngine_Z 1
//	#endif
#endif // UseZUITextEngine_Z

#if UseZUITextEngine_Z
#include "ZUITextEngine_Z.h"
#endif // UseZUITextEngine_Z

ZUITextPane* ZUIFactory::Make_TextPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
#if UseZUITextEngine_Z
	return new ZUITextPane_TextEngine(inSuperPane, inLocator, new ZUITextEngine_Z(ZDCFont()));
#elif ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	return new ZUITextPane_TextEngine(inSuperPane, inLocator, new ZUITextEngine_TextEdit(ZDCFont()));
#elif ZCONFIG(OS, Win32)
//	return new ZUITextPane_Win_EditControl(inSuperPane, inLocator);
#else
	ZUnimplemented();
	return nil;
#endif
	}

// =================================================================================================
// =================================================================================================
#pragma mark-
#pragma mark * ZUIFactory_Indirect

ZUIFactory_Indirect::ZUIFactory_Indirect(ZRef<ZUIFactory> realFactory)
:	fRealFactory(realFactory)
	{}

ZUIFactory_Indirect::~ZUIFactory_Indirect()
	{}

ZRef<ZUIFactory> ZUIFactory_Indirect::GetRealFactory()
	{ return fRealFactory; }

void ZUIFactory_Indirect::SetRealFactory(ZRef<ZUIFactory> realFactory)
	{ fRealFactory = realFactory; }

ZUICaptionPane* ZUIFactory_Indirect::Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_CaptionPane(inSuperPane, inLocator, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonPush(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonRadio(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonCheckbox(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonPopup(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonBevel(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonDisclosure(inSuperPane, inLocator, inResponder);
	return nil;
	}

ZUIButton* ZUIFactory_Indirect::Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ButtonPlacard(inSuperPane, inLocator, inResponder, inCaption);
	return nil;
	}

ZUITabPane* ZUIFactory_Indirect::Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_TabPane(inSuperPane, inLocator);
	return nil;
	}

ZUIScrollBar* ZUIFactory_Indirect::Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool withBorder)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ScrollBar(inSuperPane, inLocator, withBorder);
	return nil;
	}

ZUILittleArrows* ZUIFactory_Indirect::Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_LittleArrows(inSuperPane, inLocator);
	return nil;
	}

ZUISlider* ZUIFactory_Indirect::Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_Slider(inSuperPane, inLocator);
	return nil;
	}

ZUIProgressBar* ZUIFactory_Indirect::Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ProgressBar(inSuperPane, inLocator);
	return nil;
	}

ZUIChasingArrows* ZUIFactory_Indirect::Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_ChasingArrows(inSuperPane, inLocator);
	return nil;
	}

ZUIGroup* ZUIFactory_Indirect::Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_GroupPrimary(inSuperPane, inLocator);
	return nil;
	}

ZUIGroup* ZUIFactory_Indirect::Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_GroupSecondary(inSuperPane, inLocator);
	return nil;
	}

ZUIFocusFrame* ZUIFactory_Indirect::Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_FocusFrameEditText(inSuperPane, inLocator);
	return nil;
	}

ZUIFocusFrame* ZUIFactory_Indirect::Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_FocusFrameListBox(inSuperPane, inLocator);
	return nil;
	}

ZUIFocusFrame* ZUIFactory_Indirect::Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_FocusFrameDateControl(inSuperPane, inLocator);
	return nil;
	}

ZUISplitter* ZUIFactory_Indirect::Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_Splitter(inSuperPane, inLocator);
	return nil;
	}

ZUIDivider* ZUIFactory_Indirect::Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	if (ZRef<ZUIFactory> theRealFactory = fRealFactory)
		return theRealFactory->Make_Divider(inSuperPane, inLocator);
	return nil;
	}

// =================================================================================================
// =================================================================================================
#pragma mark-
#pragma mark * ZUIFactory_FallBack

ZUIFactory_FallBack::ZUIFactory_FallBack(const vector<ZRef<ZUIFactory> >& inFactories)
:	fFactories(inFactories)
	{}

ZUIFactory_FallBack::~ZUIFactory_FallBack()
	{}

ZUICaptionPane* ZUIFactory_FallBack::Make_CaptionPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZRef<ZUICaption> inCaption)
	{
	ZUICaptionPane* theCaptionPane = nil;
	for (size_t x = 0; x < fFactories.size() && theCaptionPane == nil; ++x)
		theCaptionPane = fFactories[x]->Make_CaptionPane(inSuperPane, inLocator, inCaption);
	return theCaptionPane;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonPush(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonPush(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonRadio(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonRadio(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonCheckbox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonCheckbox(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonPopup(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonPopup(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonBevel(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonBevel(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonDisclosure(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonDisclosure(inSuperPane, inLocator, inResponder);
	return theUIButton;
	}

ZUIButton* ZUIFactory_FallBack::Make_ButtonPlacard(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, ZUIButton::Responder* inResponder, ZRef<ZUICaption> inCaption)
	{
	ZUIButton* theUIButton = nil;
	for (size_t x = 0; x < fFactories.size() && theUIButton == nil; ++x)
		theUIButton = fFactories[x]->Make_ButtonPlacard(inSuperPane, inLocator, inResponder, inCaption);
	return theUIButton;
	}

ZUITabPane* ZUIFactory_FallBack::Make_TabPane(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUITabPane* theTabPane = nil;
	for (size_t x = 0; x < fFactories.size() && theTabPane == nil; ++x)
		theTabPane = fFactories[x]->Make_TabPane(inSuperPane, inLocator);
	return theTabPane;
	}

ZUIScrollBar* ZUIFactory_FallBack::Make_ScrollBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator, bool inWithBorder)
	{
	ZUIScrollBar* theUIScrollBar = nil;
	for (size_t x = 0; x < fFactories.size() && theUIScrollBar == nil; ++x)
		theUIScrollBar = fFactories[x]->Make_ScrollBar(inSuperPane, inLocator, inWithBorder);
	return theUIScrollBar;
	}

ZUILittleArrows* ZUIFactory_FallBack::Make_LittleArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUILittleArrows* theUILittleArrows = nil;
	for (size_t x = 0; x < fFactories.size() && theUILittleArrows == nil; ++x)
		theUILittleArrows = fFactories[x]->Make_LittleArrows(inSuperPane, inLocator);
	return theUILittleArrows;
	}

ZUISlider* ZUIFactory_FallBack::Make_Slider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUISlider* theUISlider = nil;
	for (size_t x = 0; x < fFactories.size() && theUISlider == nil; ++x)
		theUISlider = fFactories[x]->Make_Slider(inSuperPane, inLocator);
	return theUISlider;
	}

ZUIProgressBar* ZUIFactory_FallBack::Make_ProgressBar(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIProgressBar* theUIProgressBar = nil;
	for (size_t x = 0; x < fFactories.size() && theUIProgressBar == nil; ++x)
		theUIProgressBar = fFactories[x]->Make_ProgressBar(inSuperPane, inLocator);
	return theUIProgressBar;
	}

ZUIChasingArrows* ZUIFactory_FallBack::Make_ChasingArrows(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIChasingArrows* theUIChasingArrows = nil;
	for (size_t x = 0; x < fFactories.size() && theUIChasingArrows == nil; ++x)
		theUIChasingArrows = fFactories[x]->Make_ChasingArrows(inSuperPane, inLocator);
	return theUIChasingArrows;
	}

ZUIGroup* ZUIFactory_FallBack::Make_GroupPrimary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIGroup* theUIGroup = nil;
	for (size_t x = 0; x < fFactories.size() && theUIGroup == nil; ++x)
		theUIGroup = fFactories[x]->Make_GroupPrimary(inSuperPane, inLocator);
	return theUIGroup;
	}

ZUIGroup* ZUIFactory_FallBack::Make_GroupSecondary(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIGroup* theUIGroup = nil;
	for (size_t x = 0; x < fFactories.size() && theUIGroup == nil; ++x)
		theUIGroup = fFactories[x]->Make_GroupSecondary(inSuperPane, inLocator);
	return theUIGroup;
	}

ZUIFocusFrame* ZUIFactory_FallBack::Make_FocusFrameEditText(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIFocusFrame* theUIFocusFrame = nil;
	for (size_t x = 0; x < fFactories.size() && theUIFocusFrame == nil; ++x)
		theUIFocusFrame = fFactories[x]->Make_FocusFrameEditText(inSuperPane, inLocator);
	return theUIFocusFrame;
	}

ZUIFocusFrame* ZUIFactory_FallBack::Make_FocusFrameListBox(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIFocusFrame* theUIFocusFrame = nil;
	for (size_t x = 0; x < fFactories.size() && theUIFocusFrame == nil; ++x)
		theUIFocusFrame = fFactories[x]->Make_FocusFrameListBox(inSuperPane, inLocator);
	return theUIFocusFrame;
	}

ZUIFocusFrame* ZUIFactory_FallBack::Make_FocusFrameDateControl(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIFocusFrame* theUIFocusFrame = nil;
	for (size_t x = 0; x < fFactories.size() && theUIFocusFrame == nil; ++x)
		theUIFocusFrame = fFactories[x]->Make_FocusFrameDateControl(inSuperPane, inLocator);
	return theUIFocusFrame;
	}

ZUISplitter* ZUIFactory_FallBack::Make_Splitter(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUISplitter* theUISplitter = nil;
	for (size_t x = 0; x < fFactories.size() && theUISplitter == nil; ++x)
		theUISplitter = fFactories[x]->Make_Splitter(inSuperPane, inLocator);
	return theUISplitter;
	}

ZUIDivider* ZUIFactory_FallBack::Make_Divider(ZSuperPane* inSuperPane, ZPaneLocator* inLocator)
	{
	ZUIDivider* theUIDivider = nil;
	for (size_t x = 0; x < fFactories.size() && theUIDivider == nil; ++x)
		theUIDivider = fFactories[x]->Make_Divider(inSuperPane, inLocator);
	return theUIDivider;
	}

// =================================================================================================
// =================================================================================================
#pragma mark-
#pragma mark * ZUIInk_UIColor

class ZUIInk_UIColor : public ZUIInk
	{
public:
	ZUIInk_UIColor(const ZRef<ZUIColor>& inUIColor);
	virtual ~ZUIInk_UIColor();

	virtual ZDCInk GetInk();
protected:
	ZRef<ZUIColor> fUIColor;
	};

ZUIInk_UIColor::ZUIInk_UIColor(const ZRef<ZUIColor>& inUIColor)
:	fUIColor(inUIColor)
	{
	}

ZUIInk_UIColor::~ZUIInk_UIColor()
	{}

ZDCInk ZUIInk_UIColor::GetInk()
	{
	if (fUIColor)
		return ZDCInk(fUIColor->GetColor());
	return ZDCInk();
	}

// =================================================================================================
// =================================================================================================
#pragma mark-
#pragma mark * ZUIAttributeFactory

ZRef<ZUIAttributeFactory> ZUIAttributeFactory::sUIAttributeFactory;
ZUIAttributeFactory::ZUIAttributeFactory()
	{
// AG 98-10-11. Call ZUIAttributeFactory::sSet to set the UIFactory you want returned from sGet
	}

ZUIAttributeFactory::~ZUIAttributeFactory()
	{}

void ZUIAttributeFactory::sSet(ZRef<ZUIAttributeFactory> inFactory)
	{ sUIAttributeFactory = inFactory; }

ZRef<ZUIAttributeFactory> ZUIAttributeFactory::sGet()
	{
	ZAssertStop(2, sUIAttributeFactory);
	return sUIAttributeFactory;
	}

ZRef<ZUIColor> ZUIAttributeFactory::GetColor(const string& inColorName)
	{
	if (inColorName == "List_Background")
		return new ZUIColor_Fixed(ZRGBColor::sWhite);
	if (inColorName == "List_Separator")
		return new ZUIColor_Fixed(ZRGBColor::sWhite);
	return ZRef<ZUIColor>();
	}

ZRef<ZUIInk> ZUIAttributeFactory::GetInk(const string& inInkName)
	{
// If we could not get an ink with the requested name, try grabbing a color instead
	if (ZRef<ZUIColor> theUIColor = this->GetColor(inInkName))
		return new ZUIInk_UIColor(theUIColor);
	return ZRef<ZUIInk>();
	}

ZRef<ZUIFont> ZUIAttributeFactory::GetFont(const string& inFontName)
	{ return ZRef<ZUIFont>(); }

ZRef<ZUIMetric> ZUIAttributeFactory::GetMetric(const string& inMetricName)
	{
// These are from Apple's Platinum Appearance guidelines. They may need to be overridden
// in the Win32 or other factories.

// Distance from inside edge of a dialog to controls it contains
	if (inMetricName == "Layout:DialogBorder:L")
		return new ZUIMetric_Fixed(12);
	if (inMetricName == "Layout:DialogBorder:T")
		return new ZUIMetric_Fixed(12);
	if (inMetricName == "Layout:DialogBorder:R")
		return new ZUIMetric_Fixed(12);
	if (inMetricName == "Layout:DialogBorder:B")
		return new ZUIMetric_Fixed(12);

// Distance from inside edge of a group box to controls it contains
	if (inMetricName == "Layout:GroupBorder:L")
		return new ZUIMetric_Fixed(10);
	if (inMetricName == "Layout:GroupBorder:T")
		return new ZUIMetric_Fixed(12);
	if (inMetricName == "Layout:GroupBorder:R")
		return new ZUIMetric_Fixed(10);
	if (inMetricName == "Layout:GroupBorder:B")
		return new ZUIMetric_Fixed(10);

// Distance between a label and the control it applies to
	if (inMetricName == "Layout:LabelSpace:H")
		return new ZUIMetric_Fixed(5);
	if (inMetricName == "Layout:LabelSpace:V")
		return new ZUIMetric_Fixed(5);

// DIstance between horizontally arranged controls
	if (inMetricName == "Layout:ControlSpace:H")
		return new ZUIMetric_Fixed(12);
// DIstance between vertically arranged controls
	if (inMetricName == "Layout:ControlSpace:V")
		return new ZUIMetric_Fixed(10);
// DIstance between vertically arranged related controls (like a stack of radio buttons)
	if (inMetricName == "Layout:ControlSpace:V:Related")
		return new ZUIMetric_Fixed(6);

	return ZRef<ZUIMetric>();
	}

bool ZUIAttributeFactory::GetAttribute(const string& inAttributeName, void* inAttributeResult)
	{ return false; }

ZRef<ZUIIconRenderer> ZUIAttributeFactory::GetIconRenderer()
	{ return new ZUIIconRenderer; }

ZRef<ZUIInk> ZUIAttributeFactory::GetInk_WindowBackground_Document()
	{ return this->GetInk("WindowBackground_Document"); }

ZRef<ZUIInk> ZUIAttributeFactory::GetInk_WindowBackground_Dialog()
	{ return this->GetInk("WindowBackground_Dialog"); }

ZRef<ZUIInk> ZUIAttributeFactory::GetInk_BackgroundHighlight()
	{ return this->GetInk("Background_Highlight"); }

ZRef<ZUIInk> ZUIAttributeFactory::GetInk_List_Background()
	{ return this->GetInk("List_Background"); }

ZRef<ZUIInk> ZUIAttributeFactory::GetInk_List_Separator()
	{ return this->GetInk("List_Separator"); }

ZRef<ZUIColor> ZUIAttributeFactory::GetColor_Text()
	{ return this->GetColor("Text"); }

ZRef<ZUIColor> ZUIAttributeFactory::GetColor_TextHighlight()
	{ return this->GetColor("TextHighlight"); }

ZRef<ZUIFont> ZUIAttributeFactory::GetFont_SystemLarge()
	{ return this->GetFont("SystemLarge"); }

ZRef<ZUIFont> ZUIAttributeFactory::GetFont_SystemSmall()
	{ return this->GetFont("SystemSmall"); }

ZRef<ZUIFont> ZUIAttributeFactory::GetFont_SystemSmallEmphasized()
	{ return this->GetFont("SystemSmallEmphasized"); }

ZRef<ZUIFont> ZUIAttributeFactory::GetFont_Application()
	{ return this->GetFont("Application"); }

ZRef<ZUIFont> ZUIAttributeFactory::GetFont_Monospaced()
	{ return this->GetFont("Monospaced"); }

// =================================================================================================
#pragma mark-
#pragma mark * ZUIAttributeFactory_Caching

// AG 2000-02-08. Storing void* pointing to heap allocated ZRef<> instances generates *massively* less code,
// cutting object code size and also making it more likely that this file will actually compile.

ZUIAttributeFactory_Caching::ZUIAttributeFactory_Caching()
	{}

ZUIAttributeFactory_Caching::~ZUIAttributeFactory_Caching()
	{
	fMutex.Acquire();
	for (map<string, void*, less<string> >::iterator theIter = fMap_Color.begin(); theIter != fMap_Color.end(); ++theIter)
		delete static_cast<ZRef<ZUIColor>*>((*theIter).second);
	for (map<string, void*, less<string> >::iterator theIter = fMap_Ink.begin(); theIter != fMap_Ink.end(); ++theIter)
		delete static_cast<ZRef<ZUIInk>*>((*theIter).second);
	for (map<string, void*, less<string> >::iterator theIter = fMap_Font.begin(); theIter != fMap_Font.end(); ++theIter)
		delete static_cast<ZRef<ZUIFont>*>((*theIter).second);
	for (map<string, void*, less<string> >::iterator theIter = fMap_Metric.begin(); theIter != fMap_Metric.end(); ++theIter)
		delete static_cast<ZRef<ZUIMetric>*>((*theIter).second);
	}

ZRef<ZUIColor> ZUIAttributeFactory_Caching::GetColor(const string& inColorName)
	{
	ZMutexLocker locker(fMutex);
	ZRef<ZUIColor> theUIColor;
	map<string, void*, less<string> >::iterator theIter = fMap_Color.find(inColorName);
	if (theIter == fMap_Color.end())
		{
		theUIColor = this->MakeColor(inColorName);
		if (!theUIColor)
			theUIColor = ZUIAttributeFactory::GetColor(inColorName);
		if (theUIColor)
			fMap_Color.insert(map<string, void*, less<string> >::value_type(inColorName, new ZRef<ZUIColor>(theUIColor)));
		}
	else
		{
		theUIColor = *static_cast<ZRef<ZUIColor>*>((*theIter).second);
		}
	return theUIColor;
	}

ZRef<ZUIInk> ZUIAttributeFactory_Caching::GetInk(const string& inInkName)
	{
	ZMutexLocker locker(fMutex);
	ZRef<ZUIInk> theUIInk;
	map<string, void*, less<string> >::iterator theIter = fMap_Ink.find(inInkName);
	if (theIter == fMap_Ink.end())
		{
		theUIInk = this->MakeInk(inInkName);
		if (!theUIInk)
			theUIInk = ZUIAttributeFactory::GetInk(inInkName);
		if (theUIInk)
			fMap_Ink.insert(map<string, void*, less<string> >::value_type(inInkName, new ZRef<ZUIInk>(theUIInk)));
		}
	else
		{
		theUIInk = *static_cast<ZRef<ZUIInk>*>((*theIter).second);
		}
	return theUIInk;
	}

ZRef<ZUIFont> ZUIAttributeFactory_Caching::GetFont(const string& inFontName)
	{
	ZMutexLocker locker(fMutex);
	ZRef<ZUIFont> theUIFont;
	map<string, void*, less<string> >::iterator theIter = fMap_Font.find(inFontName);
	if (theIter == fMap_Font.end())
		{
		theUIFont = this->MakeFont(inFontName);
		if (!theUIFont)
			theUIFont = ZUIAttributeFactory::GetFont(inFontName);
		if (theUIFont)
			fMap_Font.insert(map<string, void*, less<string> >::value_type(inFontName, new ZRef<ZUIFont>(theUIFont)));
		}
	else
		{
		theUIFont = *static_cast<ZRef<ZUIFont>*>((*theIter).second);
		}
	return theUIFont;
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Caching::GetMetric(const string& inMetricName)
	{
	ZMutexLocker locker(fMutex);
	ZRef<ZUIMetric> theUIMetric;
	map<string, void*, less<string> >::iterator theIter = fMap_Metric.find(inMetricName);
	if (theIter == fMap_Metric.end())
		{
		theUIMetric = this->MakeMetric(inMetricName);
		if (!theUIMetric)
			theUIMetric = ZUIAttributeFactory::GetMetric(inMetricName);
		if (theUIMetric)
			fMap_Metric.insert(map<string, void*, less<string> >::value_type(inMetricName, new ZRef<ZUIMetric>(theUIMetric)));
		}
	else
		{
		theUIMetric = *static_cast<ZRef<ZUIMetric>*>((*theIter).second);
		}
	return theUIMetric;
	}

ZRef<ZUIColor> ZUIAttributeFactory_Caching::MakeColor(const string& inColorName)
	{ return ZRef<ZUIColor>(); }

ZRef<ZUIInk> ZUIAttributeFactory_Caching::MakeInk(const string& inInkName)
	{ return ZRef<ZUIInk>(); }

ZRef<ZUIFont> ZUIAttributeFactory_Caching::MakeFont(const string& inFontName)
	{ return ZRef<ZUIFont>(); }

ZRef<ZUIMetric> ZUIAttributeFactory_Caching::MakeMetric(const string& inMetricName)
	{ return ZRef<ZUIMetric>(); }

// =================================================================================================
#pragma mark-
#pragma mark * ZUIAttributeFactory_Indirect

ZUIAttributeFactory_Indirect::ZUIAttributeFactory_Indirect(ZRef<ZUIAttributeFactory> inRealFactory)
:	fRealFactory(inRealFactory)
	{}

ZUIAttributeFactory_Indirect::~ZUIAttributeFactory_Indirect()
	{}

ZRef<ZUIAttributeFactory> ZUIAttributeFactory_Indirect::GetRealFactory()
	{ return fRealFactory; }

void ZUIAttributeFactory_Indirect::SetRealFactory(ZRef<ZUIAttributeFactory> inRealFactory)
	{ fRealFactory = inRealFactory; }

ZRef<ZUIColor> ZUIAttributeFactory_Indirect::GetColor(const string& inColorName)
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetColor(inColorName);
	return ZUIAttributeFactory::GetColor(inColorName);
	}

ZRef<ZUIInk> ZUIAttributeFactory_Indirect::GetInk(const string& inInkName)
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetInk(inInkName);
	return ZUIAttributeFactory::GetInk(inInkName);
	}

ZRef<ZUIFont> ZUIAttributeFactory_Indirect::GetFont(const string& inFontName)
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetFont(inFontName);
	return ZUIAttributeFactory::GetFont(inFontName);
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Indirect::GetMetric(const string& inMetricName)
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetMetric(inMetricName);
	return ZUIAttributeFactory::GetMetric(inMetricName);
	}

bool ZUIAttributeFactory_Indirect::GetAttribute(const string& inAttributeName, void* outAttributeResult)
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetAttribute(inAttributeName, outAttributeResult);
	return ZUIAttributeFactory::GetAttribute(inAttributeName, outAttributeResult);
	}

ZRef<ZUIIconRenderer> ZUIAttributeFactory_Indirect::GetIconRenderer()
	{
	if (ZRef<ZUIAttributeFactory> theRealFactory = fRealFactory)
		return theRealFactory->GetIconRenderer();
	return ZUIAttributeFactory::GetIconRenderer();
	}

// =================================================================================================
#pragma mark-
#pragma mark * ZUIAttributeFactory_FallBack

ZUIAttributeFactory_FallBack::ZUIAttributeFactory_FallBack(const vector<ZRef<ZUIAttributeFactory> > inFactories)
:	fFactories(inFactories)
	{}

ZUIAttributeFactory_FallBack::~ZUIAttributeFactory_FallBack()
	{}

ZRef<ZUIColor> ZUIAttributeFactory_FallBack::GetColor(const string& inColorName)
	{
	ZRef<ZUIColor> theUIColor;
	for (size_t x = 0; x < fFactories.size() && !theUIColor; ++x)
		theUIColor = fFactories[x]->GetColor(inColorName);
	if (!theUIColor)
		theUIColor = ZUIAttributeFactory::GetColor(inColorName);
	return theUIColor;
	}

ZRef<ZUIInk> ZUIAttributeFactory_FallBack::GetInk(const string& inInkName)
	{
	ZRef<ZUIInk> theUIInk;
	for (size_t x = 0; x < fFactories.size() && !theUIInk; ++x)
		theUIInk = fFactories[x]->GetInk(inInkName);
	if (!theUIInk)
		theUIInk = ZUIAttributeFactory::GetInk(inInkName);
	return theUIInk;
	}

ZRef<ZUIFont> ZUIAttributeFactory_FallBack::GetFont(const string& inFontName)
	{
	ZRef<ZUIFont> theUIFont;
	for (size_t x = 0; x < fFactories.size() && !theUIFont; ++x)
		theUIFont = fFactories[x]->GetFont(inFontName);
	if (!theUIFont)
		theUIFont = ZUIAttributeFactory::GetFont(inFontName);
	return theUIFont;
	}

ZRef<ZUIMetric> ZUIAttributeFactory_FallBack::GetMetric(const string& inMetricName)
	{
	ZRef<ZUIMetric> theUIMetric;
	for (size_t x = 0; x < fFactories.size() && !theUIMetric; ++x)
		theUIMetric = fFactories[x]->GetMetric(inMetricName);
	if (!theUIMetric)
		theUIMetric = ZUIAttributeFactory::GetMetric(inMetricName);
	return theUIMetric;
	}

bool ZUIAttributeFactory_FallBack::GetAttribute(const string& inAttributeName, void* outAttributeResult)
	{
	bool gotResult = false;
	for (size_t x = 0; x < fFactories.size() && !gotResult; ++x)
		gotResult = fFactories[x]->GetAttribute(inAttributeName, outAttributeResult);
	return gotResult;
	}

ZRef<ZUIIconRenderer> ZUIAttributeFactory_FallBack::GetIconRenderer()
	{
	ZRef<ZUIIconRenderer> theUIIconRenderer;
	for (size_t x = 0; x < fFactories.size() && !theUIIconRenderer; ++x)
		theUIIconRenderer = fFactories[x]->GetIconRenderer();
	return theUIIconRenderer;
	}

// =================================================================================================
