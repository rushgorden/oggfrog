/*  @(#) $Id: ZUISwitchable.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZUISwitchable__
#define __ZUISwitchable__ 1
#include "zconfig.h"

#include "ZUIRendered.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZUIRendererFactory_Switchable

class ZUIRendererFactory_Switchable : public ZUIRendererFactory
	{
public:
	ZUIRendererFactory_Switchable(ZRef<ZUIRendererFactory> realFactory);
	virtual ~ZUIRendererFactory_Switchable();

	ZRef<ZUIRendererFactory> GetRealFactory();
	void SetRealFactory(ZRef<ZUIRendererFactory> realFactory);

// From ZUIRendererFactory
	virtual string GetName();

	virtual ZRef<ZUIRenderer_CaptionPane> Get_Renderer_CaptionPane();

	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPush();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonRadio();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonCheckbox();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPopup();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonBevel();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonDisclosure();
	virtual ZRef<ZUIRenderer_Button> Get_Renderer_ButtonPlacard();

	virtual ZRef<ZUIRenderer_ScrollBar> Get_Renderer_ScrollBar();
	virtual ZRef<ZUIRenderer_Slider> Get_Renderer_Slider();
	virtual ZRef<ZUIRenderer_LittleArrows> Get_Renderer_LittleArrows();

	virtual ZRef<ZUIRenderer_ProgressBar> Get_Renderer_ProgressBar();
	virtual ZRef<ZUIRenderer_ChasingArrows> Get_Renderer_ChasingArrows();

	virtual ZRef<ZUIRenderer_TabPane> Get_Renderer_TabPane();

	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupPrimary();
	virtual ZRef<ZUIRenderer_Group> Get_Renderer_GroupSecondary();

	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameEditText();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameListBox();
	virtual ZRef<ZUIRenderer_FocusFrame> Get_Renderer_FocusFrameDateControl();

	virtual ZRef<ZUIRenderer_Splitter> Get_Renderer_Splitter();

	virtual ZRef<ZUIRenderer_Divider> Get_Renderer_Divider();

protected:
	ZRef<ZUIRendererFactory> fRealFactory;

	ZMutex fMutex;

	ZRef<ZUIRenderer_CaptionPane_Indirect> fRenderer_CaptionPane;

	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonPush;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonRadio;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonCheckbox;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonPopup;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonBevel;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonDisclosure;
	ZRef<ZUIRenderer_Button_Indirect> fRenderer_ButtonPlacard;

	ZRef<ZUIRenderer_ScrollBar_Indirect> fRenderer_ScrollBar;
	ZRef<ZUIRenderer_Slider_Indirect> fRenderer_Slider;
	ZRef<ZUIRenderer_LittleArrows_Indirect> fRenderer_LittleArrows;

	ZRef<ZUIRenderer_ProgressBar_Indirect> fRenderer_ProgressBar;
	ZRef<ZUIRenderer_ChasingArrows_Indirect> fRenderer_ChasingArrows;

	ZRef<ZUIRenderer_TabPane_Indirect> fRenderer_TabPane;

	ZRef<ZUIRenderer_Group_Indirect> fRenderer_GroupPrimary;
	ZRef<ZUIRenderer_Group_Indirect> fRenderer_GroupSecondary;

	ZRef<ZUIRenderer_FocusFrame_Indirect> fRenderer_FocusFrameEditText;
	ZRef<ZUIRenderer_FocusFrame_Indirect> fRenderer_FocusFrameListBox;
	ZRef<ZUIRenderer_FocusFrame_Indirect> fRenderer_FocusFrameDateControl;

	ZRef<ZUIRenderer_Splitter_Indirect> fRenderer_Splitter;

	ZRef<ZUIRenderer_Divider_Indirect> fRenderer_Divider;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZUIAttributeFactory_Switchable

class ZUIAttributeFactory_Switchable : public ZUIAttributeFactory
	{
public:
	ZUIAttributeFactory_Switchable(ZRef<ZUIAttributeFactory> realFactory);
	virtual ~ZUIAttributeFactory_Switchable();

	ZRef<ZUIAttributeFactory> GetRealFactory();
	void SetRealFactory(ZRef<ZUIAttributeFactory> realFactory);

	virtual string GetName();

	virtual bool GetAttribute(const string& theAttributeName, void* theAttributeResult);

	virtual ZRef<ZUIIconRenderer> GetIconRenderer();

	virtual ZRef<ZUIColor> GetColor(const string& theColorName);
	virtual ZRef<ZUIInk> GetInk(const string& theInkName);
	virtual ZRef<ZUIFont> GetFont(const string& theFontName);
	virtual ZRef<ZUIMetric> GetMetric(const string& theMetricName);
protected:
	ZRef<ZUIAttributeFactory> fRealFactory;

	ZMutex fMutex;

	ZRef<ZUIIconRenderer_Indirect> fIconRenderer;

	map<string, ZRef<ZUIColor_Indirect> > fColorMap;
	map<string, ZRef<ZUIInk_Indirect> > fInkMap;
	map<string, ZRef<ZUIFont_Indirect> > fFontMap;
	map<string, ZRef<ZUIMetric_Indirect> > fMetricMap;
	};

// =================================================================================================

#endif // __ZUISwitchable__
