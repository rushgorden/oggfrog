static const char rcsid[] = "@(#) $Id: ZUISwitchable.cpp,v 1.4 2006/04/10 20:44:22 agreen Exp $";

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

#include "ZUISwitchable.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Switchable

ZUIRendererFactory_Switchable::ZUIRendererFactory_Switchable(ZRef<ZUIRendererFactory> realFactory)
:	fRealFactory(realFactory)
	{}
ZUIRendererFactory_Switchable::~ZUIRendererFactory_Switchable()
	{}

ZRef<ZUIRendererFactory> ZUIRendererFactory_Switchable::GetRealFactory()
	{ return fRealFactory; }

void ZUIRendererFactory_Switchable::SetRealFactory(ZRef<ZUIRendererFactory> realFactory)
	{
	ZMutexLocker locker(fMutex);

	if (fRealFactory == realFactory)
		return;

	fRealFactory = realFactory;

	if (fRenderer_CaptionPane)
		fRenderer_CaptionPane->SetRealRenderer(fRealFactory->Get_Renderer_CaptionPane());

	if (fRenderer_ButtonPush)
		fRenderer_ButtonPush->SetRealRenderer(fRealFactory->Get_Renderer_ButtonPush());
	if (fRenderer_ButtonRadio)
		fRenderer_ButtonRadio->SetRealRenderer(fRealFactory->Get_Renderer_ButtonRadio());
	if (fRenderer_ButtonCheckbox)
		fRenderer_ButtonCheckbox->SetRealRenderer(fRealFactory->Get_Renderer_ButtonCheckbox());
	if (fRenderer_ButtonPopup)
		fRenderer_ButtonPopup->SetRealRenderer(fRealFactory->Get_Renderer_ButtonPopup());
	if (fRenderer_ButtonBevel)
		fRenderer_ButtonBevel->SetRealRenderer(fRealFactory->Get_Renderer_ButtonBevel());
	if (fRenderer_ButtonDisclosure)
		fRenderer_ButtonDisclosure->SetRealRenderer(fRealFactory->Get_Renderer_ButtonDisclosure());
	if (fRenderer_ButtonPlacard)
		fRenderer_ButtonPlacard->SetRealRenderer(fRealFactory->Get_Renderer_ButtonPlacard());

	if (fRenderer_ScrollBar)
		fRenderer_ScrollBar->SetRealRenderer(fRealFactory->Get_Renderer_ScrollBar());
	if (fRenderer_Slider)
		fRenderer_Slider->SetRealRenderer(fRealFactory->Get_Renderer_Slider());
	if (fRenderer_LittleArrows)
		fRenderer_LittleArrows->SetRealRenderer(fRealFactory->Get_Renderer_LittleArrows());

	if (fRenderer_ProgressBar)
		fRenderer_ProgressBar->SetRealRenderer(fRealFactory->Get_Renderer_ProgressBar());
	if (fRenderer_ChasingArrows)
		fRenderer_ChasingArrows->SetRealRenderer(fRealFactory->Get_Renderer_ChasingArrows());

	if (fRenderer_TabPane)
		fRenderer_TabPane->SetRealRenderer(fRealFactory->Get_Renderer_TabPane());

	if (fRenderer_GroupPrimary)
		fRenderer_GroupPrimary->SetRealRenderer(fRealFactory->Get_Renderer_GroupPrimary());
	if (fRenderer_GroupSecondary)
		fRenderer_GroupSecondary->SetRealRenderer(fRealFactory->Get_Renderer_GroupSecondary());

	if (fRenderer_FocusFrameEditText)
		fRenderer_FocusFrameEditText->SetRealRenderer(fRealFactory->Get_Renderer_FocusFrameEditText());
	if (fRenderer_FocusFrameListBox)
		fRenderer_FocusFrameListBox->SetRealRenderer(fRealFactory->Get_Renderer_FocusFrameListBox());

	if (fRenderer_Splitter)
		fRenderer_Splitter->SetRealRenderer(fRealFactory->Get_Renderer_Splitter());

	if (fRenderer_Divider)
		fRenderer_Divider->SetRealRenderer(fRealFactory->Get_Renderer_Divider());
	}

string ZUIRendererFactory_Switchable::GetName()
	{ return fRealFactory->GetName(); }

ZRef<ZUIRenderer_CaptionPane> ZUIRendererFactory_Switchable::Get_Renderer_CaptionPane()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_CaptionPane)
		fRenderer_CaptionPane = new ZUIRenderer_CaptionPane_Indirect(fRealFactory->Get_Renderer_CaptionPane());
	return fRenderer_CaptionPane;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonPush()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonPush)
		fRenderer_ButtonPush = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonPush());
	return fRenderer_ButtonPush;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonRadio()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonRadio)
		fRenderer_ButtonRadio = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonRadio());
	return fRenderer_ButtonRadio;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonCheckbox()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonCheckbox)
		fRenderer_ButtonCheckbox = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonCheckbox());
	return fRenderer_ButtonCheckbox;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonPopup()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonPopup)
		fRenderer_ButtonPopup = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonPopup());
	return fRenderer_ButtonPopup;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonBevel()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonBevel)
		fRenderer_ButtonBevel = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonBevel());
	return fRenderer_ButtonBevel;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonDisclosure()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonDisclosure)
		fRenderer_ButtonDisclosure = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonDisclosure());
	return fRenderer_ButtonDisclosure;
	}

ZRef<ZUIRenderer_Button> ZUIRendererFactory_Switchable::Get_Renderer_ButtonPlacard()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ButtonPlacard)
		fRenderer_ButtonPlacard = new ZUIRenderer_Button_Indirect(fRealFactory->Get_Renderer_ButtonPlacard());
	return fRenderer_ButtonPlacard;
	}

ZRef<ZUIRenderer_ScrollBar> ZUIRendererFactory_Switchable::Get_Renderer_ScrollBar()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ScrollBar)
		fRenderer_ScrollBar = new ZUIRenderer_ScrollBar_Indirect(fRealFactory->Get_Renderer_ScrollBar());
	return fRenderer_ScrollBar;
	}

ZRef<ZUIRenderer_Slider> ZUIRendererFactory_Switchable::Get_Renderer_Slider()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_Slider)
		fRenderer_Slider = new ZUIRenderer_Slider_Indirect(fRealFactory->Get_Renderer_Slider());
	return fRenderer_Slider;
	}

ZRef<ZUIRenderer_LittleArrows> ZUIRendererFactory_Switchable::Get_Renderer_LittleArrows()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_LittleArrows)
		fRenderer_LittleArrows = new ZUIRenderer_LittleArrows_Indirect(fRealFactory->Get_Renderer_LittleArrows());
	return fRenderer_LittleArrows;
	}

ZRef<ZUIRenderer_ProgressBar> ZUIRendererFactory_Switchable::Get_Renderer_ProgressBar()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ProgressBar)
		fRenderer_ProgressBar = new ZUIRenderer_ProgressBar_Indirect(fRealFactory->Get_Renderer_ProgressBar());
	return fRenderer_ProgressBar;
	}

ZRef<ZUIRenderer_ChasingArrows> ZUIRendererFactory_Switchable::Get_Renderer_ChasingArrows()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_ChasingArrows)
		fRenderer_ChasingArrows = new ZUIRenderer_ChasingArrows_Indirect(fRealFactory->Get_Renderer_ChasingArrows());
	return fRenderer_ChasingArrows;
	}

ZRef<ZUIRenderer_TabPane> ZUIRendererFactory_Switchable::Get_Renderer_TabPane()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_TabPane)
		fRenderer_TabPane = new ZUIRenderer_TabPane_Indirect(fRealFactory->Get_Renderer_TabPane());
	return fRenderer_TabPane;
	}

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Switchable::Get_Renderer_GroupPrimary()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_GroupPrimary)
		fRenderer_GroupPrimary = new ZUIRenderer_Group_Indirect(fRealFactory->Get_Renderer_GroupPrimary());
	return fRenderer_GroupPrimary;
	}

ZRef<ZUIRenderer_Group> ZUIRendererFactory_Switchable::Get_Renderer_GroupSecondary()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_GroupSecondary)
		fRenderer_GroupSecondary = new ZUIRenderer_Group_Indirect(fRealFactory->Get_Renderer_GroupSecondary());
	return fRenderer_GroupSecondary;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Switchable::Get_Renderer_FocusFrameEditText()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_FocusFrameEditText)
		fRenderer_FocusFrameEditText = new ZUIRenderer_FocusFrame_Indirect(fRealFactory->Get_Renderer_FocusFrameEditText());
	return fRenderer_FocusFrameEditText;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Switchable::Get_Renderer_FocusFrameListBox()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_FocusFrameListBox)
		fRenderer_FocusFrameListBox = new ZUIRenderer_FocusFrame_Indirect(fRealFactory->Get_Renderer_FocusFrameListBox());
	return fRenderer_FocusFrameListBox;
	}

ZRef<ZUIRenderer_FocusFrame> ZUIRendererFactory_Switchable::Get_Renderer_FocusFrameDateControl()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_FocusFrameDateControl)
		fRenderer_FocusFrameDateControl = new ZUIRenderer_FocusFrame_Indirect(fRealFactory->Get_Renderer_FocusFrameDateControl());
	return fRenderer_FocusFrameDateControl;
	}

ZRef<ZUIRenderer_Splitter> ZUIRendererFactory_Switchable::Get_Renderer_Splitter()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_Splitter)
		fRenderer_Splitter = new ZUIRenderer_Splitter_Indirect(fRealFactory->Get_Renderer_Splitter());
	return fRenderer_Splitter;
	}

ZRef<ZUIRenderer_Divider> ZUIRendererFactory_Switchable::Get_Renderer_Divider()
	{
	ZMutexLocker locker(fMutex);
	if (!fRenderer_Divider)
		fRenderer_Divider = new ZUIRenderer_Divider_Indirect(fRealFactory->Get_Renderer_Divider());
	return fRenderer_Divider;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Switchable

ZUIAttributeFactory_Switchable::ZUIAttributeFactory_Switchable(ZRef<ZUIAttributeFactory> realFactory)
:	fRealFactory(realFactory)
	{}

ZUIAttributeFactory_Switchable::~ZUIAttributeFactory_Switchable()
	{}

ZRef<ZUIAttributeFactory> ZUIAttributeFactory_Switchable::GetRealFactory()
	{ return fRealFactory; }

void ZUIAttributeFactory_Switchable::SetRealFactory(ZRef<ZUIAttributeFactory> realFactory)
	{
	ZMutexLocker locker(fMutex);

	if (fRealFactory == realFactory)
		return;

	fRealFactory = realFactory;

	if (fIconRenderer)
		fIconRenderer->SetRealRenderer(fRealFactory->GetIconRenderer());

	for (map<string, ZRef<ZUIColor_Indirect> >::iterator i = fColorMap.begin(); i != fColorMap.end(); ++i)
		(*i).second->SetRealColor(fRealFactory->GetColor((*i).first));

	for (map<string, ZRef<ZUIInk_Indirect> >::iterator i = fInkMap.begin(); i != fInkMap.end(); ++i)
		(*i).second->SetRealInk(fRealFactory->GetInk((*i).first));

	for (map<string, ZRef<ZUIFont_Indirect> >::iterator i = fFontMap.begin(); i != fFontMap.end(); ++i)
		(*i).second->SetRealFont(fRealFactory->GetFont((*i).first));

	for (map<string, ZRef<ZUIMetric_Indirect> >::iterator i = fMetricMap.begin(); i != fMetricMap.end(); ++i)
		(*i).second->SetRealMetric(fRealFactory->GetMetric((*i).first));
	}

string ZUIAttributeFactory_Switchable::GetName()
	{
	ZMutexLocker locker(fMutex);
	return fRealFactory->GetName();
	}

bool ZUIAttributeFactory_Switchable::GetAttribute(const string& theAttributeName, void* theAttributeResult)
	{ return fRealFactory->GetAttribute(theAttributeName, theAttributeResult); }

ZRef<ZUIIconRenderer> ZUIAttributeFactory_Switchable::GetIconRenderer()
	{
	ZMutexLocker locker(fMutex);
	if (!fIconRenderer)
		fIconRenderer = new ZUIIconRenderer_Indirect(fRealFactory->GetIconRenderer());
	return fIconRenderer;
	}

ZRef<ZUIColor> ZUIAttributeFactory_Switchable::GetColor(const string& theColorName)
	{
	ZMutexLocker locker(fMutex);
	map<string, ZRef<ZUIColor_Indirect> >::iterator theIter = fColorMap.find(theColorName);
	if (theIter == fColorMap.end())
		theIter = fColorMap.insert(map<string, ZRef<ZUIColor_Indirect> >::value_type(theColorName, new ZUIColor_Indirect(fRealFactory->GetColor(theColorName)))).first;
	return (*theIter).second;
	}

ZRef<ZUIInk> ZUIAttributeFactory_Switchable::GetInk(const string& theInkName)
	{
	ZMutexLocker locker(fMutex);
	map<string, ZRef<ZUIInk_Indirect> >::iterator theIter = fInkMap.find(theInkName);
	if (theIter == fInkMap.end())
		theIter = fInkMap.insert(map<string, ZRef<ZUIInk_Indirect> >::value_type(theInkName, new ZUIInk_Indirect(fRealFactory->GetInk(theInkName)))).first;
	return (*theIter).second;
	}

ZRef<ZUIFont> ZUIAttributeFactory_Switchable::GetFont(const string& theFontName)
	{
	ZMutexLocker locker(fMutex);
	map<string, ZRef<ZUIFont_Indirect> >::iterator theIter = fFontMap.find(theFontName);
	if (theIter == fFontMap.end())
		theIter = fFontMap.insert(map<string, ZRef<ZUIFont_Indirect> >::value_type(theFontName, new ZUIFont_Indirect(fRealFactory->GetFont(theFontName)))).first;
	return (*theIter).second;
	}

ZRef<ZUIMetric> ZUIAttributeFactory_Switchable::GetMetric(const string& theMetricName)
	{
	ZMutexLocker locker(fMutex);
	map<string, ZRef<ZUIMetric_Indirect> >::iterator theIter = fMetricMap.find(theMetricName);
	if (theIter == fMetricMap.end())
		theIter = fMetricMap.insert(map<string, ZRef<ZUIMetric_Indirect> >::value_type(theMetricName, new ZUIMetric_Indirect(fRealFactory->GetMetric(theMetricName)))).first;
	return (*theIter).second;
	}

// =================================================================================================
