static const char rcsid[] = "@(#) $Id: ZAlert.cpp,v 1.9 2006/07/17 18:23:46 agreen Exp $";

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

#include "ZAlert.h"

#include "ZApp.h"
#include "ZDC_QD.h"
#include "ZThread.h"
#include "ZString.h"
#include "ZTicks.h"
#include "ZUIFactory.h"
#include "ZUnicode.h"
#include "ZUtil_Mac_HL.h"
#include "ZUtil_Win.h"
#include "ZWindow.h"

#define ZTr(a) a

// ==================================================

#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

namespace ZANONYMOUS {

class Caption_ICON : public ZUICaption
	{
public:
	Caption_ICON(short inIconID);
	virtual ~Caption_ICON();
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

protected:
	short fIconID;
	};

Caption_ICON::Caption_ICON(short inIconID)
:	fIconID(inIconID)
	{}

Caption_ICON::~Caption_ICON()
	{}

void Caption_ICON::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	ZDC localDC(inDC);
	localDC.PenNormal();
	ZDCSetupForQD theSetupForQD(localDC, true);

	Rect qdRect = ZRect(32, 32) + inLocation + theSetupForQD.GetOffset();

	if (CIconHandle colorIconHandle = ::GetCIcon(fIconID))
		{
		::PlotCIconHandle(&qdRect, atNone, inState.GetEnabled() && inState.GetActive() ? kTransformNone : kTransformDisabled, colorIconHandle);
		::DisposeCIcon(colorIconHandle);
		}
	else
		{
		if (Handle iconHandle = ::GetIcon(fIconID))
			::PlotIconHandle(&qdRect, atNone, inState.GetEnabled() && inState.GetActive() ? kTransformNone : kTransformDisabled, iconHandle);
		}

	}

ZDCRgn Caption_ICON::GetRgn()
	{ return ZRect(32, 32); }

class Caption_PICT : public ZUICaption
	{
public:
	Caption_PICT(short inPictureID);
	virtual ~Caption_PICT();
	virtual void Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer);
	virtual ZDCRgn GetRgn();

protected:
	short fPictureID;
	};


Caption_PICT::Caption_PICT(short inPictureID)
:	fPictureID(inPictureID)
	{}

Caption_PICT::~Caption_PICT()
	{}

void Caption_PICT::Draw(const ZDC& inDC, ZPoint inLocation, const ZUIDisplayState& inState, ZUICaptionRenderer* inCaptionRenderer)
	{
	PicHandle thePicHandle = ::GetPicture(fPictureID);
	if (thePicHandle)
		{
		ZDC localDC(inDC);
		localDC.PenNormal();
		ZDCSetupForQD theSetupForQD(inDC, true);
		ZRect theBounds = thePicHandle[0]->picFrame;
		Rect qdBounds = theBounds - theBounds.TopLeft() + inLocation + theSetupForQD.GetOffset();
		::DrawPicture(thePicHandle, &qdBounds);
		}
	}

ZDCRgn Caption_PICT::GetRgn()
	{
	if (PicHandle thePicHandle = ::GetPicture(fPictureID))
		{
		ZRect theBounds = thePicHandle[0]->picFrame;
		return theBounds - theBounds.TopLeft();
		}
	return ZDCRgn();
	}

} // anonymous namespace

#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)

// ==================================================

namespace ZAlert {

class AlertWindow;
class TextBoxWindow;

} // namespace ZAlert

class ZAlert::AlertWindow : public ZWindow,
							public ZUIButton::Responder,
							public ZPaneLocator
	{
public:
	static ZOSWindow* sCreateOSWindow(bool inHasCloseBox);

	AlertWindow(ZAlert::Level theLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
							const vector<ZRef<ZUICaption> >& inButtonCaptions, int32 inDefaultButtonIndex, int32 inEscapeButtonIndex);
	virtual ~AlertWindow();

// From ZEventHr via ZWindow
	virtual bool DoKeyDown(const ZEvent_Key& theEvent);

// From ZWindow
	virtual void HandleSetupMenus(ZMenuSetup& inMenuSetup, ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual bool WindowQuitRequested();
	virtual void WindowCloseByUser();
	virtual void WindowIdle();

// From ZUIButton::Responder
	virtual bool HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent);

// From ZPaneLocator
	virtual bool GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult);
	virtual bool GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation);

// Our Protocol
	int32 GetResult()
		{ return fResult; }

protected:
	vector<ZUIButton*> fButtons;
	int32 fDefaultButtonIndex;
	int32 fEscapeButtonIndex;

	ZUICaptionPane* fCaptionPane_Icon;
	ZUICaptionPane* fCaptionPane_Message;
	ZUICaptionPane* fCaptionPane_Verbiage;
	ZUICaptionPane* fCaptionPane_TimeOut;

	ZWindowPane* fWindowPane;

	ZRef<ZUIMetric> fMetric_Layout_ControlSpace_H;
	ZRef<ZUIMetric> fMetric_Layout_ControlSpace_V;

	int32 fResult;
	bigtime_t fTimeOutTime;
	string fLastTimeString;
	};

static const ZCoord kIdealDialogWidth = 374;
static const ZCoord kASpace = 13;
static const ZCoord kBSpace = 23;
static const ZCoord kAOuterSpace = kASpace - 4;
static const ZCoord kBOuterSpace = kBSpace - 4;
static const ZCoord kIdealTextWidth = kIdealDialogWidth - kBOuterSpace - 32 - kBSpace - kASpace;


ZOSWindow* ZAlert::AlertWindow::sCreateOSWindow(bool inHasCloseBox)
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0,0,100,100);
	theAttributes.fLook = ZOSWindow::lookMovableModal;
	theAttributes.fLayer = ZOSWindow::layerDialog;
	theAttributes.fResizable = false;
	theAttributes.fHasSizeBox = false;
	theAttributes.fHasCloseBox = inHasCloseBox;
	theAttributes.fHasZoomBox = false;
	theAttributes.fHasMenuBar = false;
	theAttributes.fHasTitleIcon = false;

	return ZApp::sGet()->CreateOSWindow(theAttributes);
	}

ZAlert::AlertWindow::AlertWindow(ZAlert::Level theLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
							const vector<ZRef<ZUICaption> >& inButtonCaptions, int32 inDefaultButtonIndex, int32 inEscapeButtonIndex)
:	ZWindow(ZApp::sGet(), sCreateOSWindow(inEscapeButtonIndex >= 0)), ZPaneLocator(nil)
	{
	ZAssertStopf(1, inButtonCaptions.size() > 0, ("We need at least one button"));
	ZAssertStop(1, (inDefaultButtonIndex < int32(inButtonCaptions.size())) && (inEscapeButtonIndex < int32(inButtonCaptions.size())));

	fResult = -1;
	fTimeOutTime = 0;
	if (inTimeOut > 0)
		fTimeOutTime = ZTicks::sNow() + inTimeOut;

	fDefaultButtonIndex = inDefaultButtonIndex;
	fEscapeButtonIndex = inEscapeButtonIndex;

	fCaptionPane_Verbiage = nil;
	fCaptionPane_TimeOut = nil;
	fCaptionPane_Icon = nil;

	ZRef<ZUIFactory> theUIFactory = ZUIFactory::sGet();
	ZRef<ZUIAttributeFactory> theUIAttributeFactory = ZUIAttributeFactory::sGet();

	this->SetTitle(inTitle);
	this->SetBackInk(theUIAttributeFactory->GetInk_WindowBackground_Dialog());

	fMetric_Layout_ControlSpace_H = theUIAttributeFactory->GetMetric("Layout:ControlSpace:H");
	fMetric_Layout_ControlSpace_V = theUIAttributeFactory->GetMetric("Layout:ControlSpace:V");

	fWindowPane = new ZWindowPane(this, nil);

// Create the alert's icon
	ZRef<ZUICaption> theCaptionIcon;
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	if (theLevel == ZAlert::eLevelNote)
		theCaptionIcon = new Caption_ICON(noteIcon);
	else if (theLevel == ZAlert::eLevelCaution)
		theCaptionIcon = new Caption_ICON(cautionIcon);
	else if (theLevel == ZAlert::eLevelStop)
		theCaptionIcon = new Caption_ICON(stopIcon);
#elif ZCONFIG(OS, Win32)
	HICON theHICON = nil;
	if (theLevel == ZAlert::eLevelNote)
		theHICON = ZUtil_Win::sLoadIconID(false, IDI_QUESTION);
	else if (theLevel == ZAlert::eLevelCaution)
		theHICON = ZUtil_Win::sLoadIconID(false, IDI_EXCLAMATION);
	else if (theLevel == ZAlert::eLevelStop)
		theHICON = ZUtil_Win::sLoadIconID(false, IDI_HAND);
	if (theHICON)
		{
		ZDCPixmap thePixmapColor, thePixmapMono, thePixmapMask;
		ZUtil_Win::sPixmapsFromHICON(theHICON, &thePixmapColor, &thePixmapMono, &thePixmapMask);
		theCaptionIcon = new ZUICaption_Pix(thePixmapColor, thePixmapMono, thePixmapMask);
		::DestroyIcon(theHICON);
		}
#endif
	if (theCaptionIcon)
		fCaptionPane_Icon = theUIFactory->Make_CaptionPane(fWindowPane, this, theCaptionIcon);

// Build the message caption pane

	ZRef<ZUICaption> messageCaption = new ZUICaption_Text(inMessage, theUIAttributeFactory->GetFont_SystemLarge(), kIdealTextWidth);
	fCaptionPane_Message = theUIFactory->Make_CaptionPane(fWindowPane, this, messageCaption);

// And the verbiage caption pane (if any)
	if (inVerbiage.size())
		{
		ZRef<ZUICaption> verbiageCaption = new ZUICaption_Text(inVerbiage, theUIAttributeFactory->GetFont_SystemSmall(), kIdealTextWidth);
		fCaptionPane_Verbiage = theUIFactory->Make_CaptionPane(fWindowPane, this, verbiageCaption);
		}

// And the timeout pane
	fCaptionPane_TimeOut = theUIFactory->Make_CaptionPane(fWindowPane, this, ZRef<ZUICaption>());

	ZRect messageFrame = fCaptionPane_Message->CalcFrame();
	ZCoord rightEdge = messageFrame.right;
	ZCoord leftEdge = messageFrame.left;
	ZCoord bottomEdge = messageFrame.bottom;
	if (fCaptionPane_Verbiage)
		{
		ZRect verbiageFrame = fCaptionPane_Verbiage->CalcFrame();
		rightEdge = max(rightEdge, verbiageFrame.right);
		bottomEdge = verbiageFrame.bottom;
		}
// If our text did not push us past the bottom of the icon, then use the icon's bottom
	if (fCaptionPane_Icon)
		bottomEdge = max(bottomEdge, fCaptionPane_Icon->CalcFrame().bottom);


// Build the buttons -- they run from right to left, remember
	ZCoord buttonWidth = 0;
	ZCoord maxButtonHeight = 0;
	for (size_t x = 0; x < inButtonCaptions.size(); ++x)
		{
		ZUIButton* aButton = theUIFactory->Make_ButtonPush(fWindowPane, this, this, inButtonCaptions[x]);
		fButtons.push_back(aButton);
		ZPoint buttonSize = aButton->GetSize();
		buttonWidth += buttonSize.h;
		if (x != 0)
			buttonWidth += fMetric_Layout_ControlSpace_H->GetMetric();
		maxButtonHeight = max(maxButtonHeight, buttonSize.v);
		}

	ZPoint windowSize(max(ZCoord(leftEdge+buttonWidth), rightEdge) + kBOuterSpace, bottomEdge + maxButtonHeight + kASpace + kAOuterSpace);

// Resize the window to have just enough space for the buttons and text
	this->SetSize(windowSize);
	}

ZAlert::AlertWindow::~AlertWindow()
	{
	}

bool ZAlert::AlertWindow::DoKeyDown(const ZEvent_Key& theEvent)
	{
	switch (theEvent.GetCharCode())
		{
		case ZKeyboard::ccReturn:
		case ZKeyboard::ccEnter:
			{
			if (fDefaultButtonIndex >= 0)
				{
				fButtons[fDefaultButtonIndex]->Flash();
				return true;
				}
			}
			break;
		case ZKeyboard::ccEscape:
			{
			if (fEscapeButtonIndex >= 0)
				{
				fButtons[fEscapeButtonIndex]->Flash();
				return true;
				}
			}
			break;
		}

	return ZWindow::DoKeyDown(theEvent);
	}

void ZAlert::AlertWindow::HandleSetupMenus(ZMenuSetup& inMenuSetup, ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	// Basically, we completely disable the menu bar by just absorbing this call.
	}

bool ZAlert::AlertWindow::WindowQuitRequested()
	{
	return false;
	}

void ZAlert::AlertWindow::WindowCloseByUser()
	{
	if (fEscapeButtonIndex >= 0)
		fButtons[fEscapeButtonIndex]->Flash();
	}

void ZAlert::AlertWindow::WindowIdle()
	{
// If we have no timeout, or no default button, then just bail
	if (fTimeOutTime == 0 || fDefaultButtonIndex < 0)
		return;
// If we've reached the time at which our timeout expires, flash the default button, which will
// ultimately call our HandleUIButton method, which will call EndModal, and the caller will be able
// to continue execution, retrieving our result from our GetResult method.
	if (ZTicks::sNow() >= fTimeOutTime)
		{
// Set our timeout to zero, to prevent the button getting flashed over and over again.
		fTimeOutTime = 0;
		fButtons[fDefaultButtonIndex]->Flash();
		}
	else
		{
// Convert the delta into a string, and if it's different from the last string we set, stuff a new
// caption into our timeout caption pane.
		bigtime_t theDelta = (fTimeOutTime - ZTicks::sNow() + (0.75 * ZTicks::sPerSecond())) / ZTicks::sPerSecond();
		string newTimeString = ZString::sFormat("%lld", theDelta);
		if (newTimeString != fLastTimeString)
			{
			fLastTimeString = newTimeString;
			ZRef<ZUICaption> timeOutCaption = new ZUICaption_Text(newTimeString, ZUIAttributeFactory::sGet()->GetFont_SystemLarge(), 0);
			fCaptionPane_TimeOut->SetCaption(timeOutCaption, true);
			}
		}
	}

bool ZAlert::AlertWindow::HandleUIButtonEvent(ZUIButton* inButton, ZUIButton::Event inButtonEvent)
	{
	if (inButtonEvent != ZUIButton::ButtonReleasedWhileDown)
		return Responder::HandleUIButtonEvent(inButton, inButtonEvent);

	for (size_t x = 0; x < fButtons.size(); ++x)
		{
		if (fButtons[x] == inButton)
			{
			fResult = x;
			this->Show(false);
			this->EndPoseModal();
			return true;
			}
		}
	return false;
	}

bool ZAlert::AlertWindow::GetPaneAttribute(ZSubPane* inPane, const string& inAttribute, void* inParam, void* outResult)
	{
	if (fDefaultButtonIndex >= 0 && inPane == fButtons[fDefaultButtonIndex])
		{
		if (inAttribute == "ZUIButton::IsDefault")
			{
			*((bool*)outResult) = true;
			return true;
			}
		}
	return ZPaneLocator::GetPaneAttribute(inPane, inAttribute, inParam, outResult);
	}

bool ZAlert::AlertWindow::GetPaneLocation(ZSubPane* inPane, ZPoint& outLocation)
	{
	if (inPane == fWindowPane)
		{
// Although fWindowPane defaults to zero, we'll have to run through all the buttons before
// determining that inPane is not one of them if we rely on the default behavior
		outLocation = ZPoint::sZero;
		return true;
		}

	if (inPane == fCaptionPane_Icon)
		{
		outLocation.h = kBOuterSpace;
		outLocation.v = kAOuterSpace;
		return true;
		}

	if (inPane == fCaptionPane_Message)
		{
		if (fCaptionPane_Icon)
			outLocation.h = fCaptionPane_Icon->CalcFrame().right + kBSpace;
		else
			outLocation.h = kBSpace;
		outLocation.v = kAOuterSpace;
		return true;
		}

	if (inPane == fCaptionPane_Verbiage)
		{
		ZRect messageFrame = fCaptionPane_Message->CalcFrame();
		outLocation.h = messageFrame.left;
		outLocation.v = messageFrame.bottom + kASpace;
		return true;
		}

// Get the right edge of whichever is wider, the message or verbiage
	ZRect messageFrame = fCaptionPane_Message->CalcFrame();
	ZCoord rightEdge = messageFrame.right;
	ZCoord bottomEdge = messageFrame.bottom;
	if (fCaptionPane_Verbiage)
		{
		ZRect verbiageFrame = fCaptionPane_Verbiage->CalcFrame();
		rightEdge = max(rightEdge, verbiageFrame.right);
		bottomEdge = verbiageFrame.bottom;
		}
// If our text did not push us past the bottom of the icon, then use the icon's bottom
	if (fCaptionPane_Icon)
		bottomEdge = max(bottomEdge, fCaptionPane_Icon->CalcFrame().bottom);

// Force the right edge to be in from the edge of the window frame
	rightEdge = fWindowPane->GetSize().h - kBOuterSpace;

	for (size_t x = 0; x < fButtons.size(); ++x)
		{
		ZPoint buttonSize = fButtons[x]->GetSize();
		ZPoint topLeftInset, bottomRightInset;
		fButtons[x]->GetInsets(topLeftInset, bottomRightInset);
		if (inPane == fButtons[x])
			{
			outLocation.v = bottomEdge + kASpace - topLeftInset.v;
			outLocation.h = rightEdge - buttonSize.h - bottomRightInset.h;
			return true;
			}
		rightEdge -= buttonSize.h - (topLeftInset.h - bottomRightInset.h) + fMetric_Layout_ControlSpace_H->GetMetric();
		}

	if (inPane == fCaptionPane_TimeOut)
		{
		ZPoint thePaneSize = inPane->GetSize();
		ZRect leftmostButtonFrame = fButtons[fButtons.size() - 1]->CalcFrame();
		outLocation.h = leftmostButtonFrame.left - fMetric_Layout_ControlSpace_H->GetMetric() - thePaneSize.h;
		outLocation.v = leftmostButtonFrame.top + (leftmostButtonFrame.Height() - thePaneSize.v) / 2;
		return true;
		}

	return ZPaneLocator::GetPaneLocation(inPane, outLocation);
	}

// ==================================================

class ZAlert::TextBoxWindow : public ZWindow
	{
public:
	static ZOSWindow* sCreateOSWindow();
	TextBoxWindow(const string& inMessage);
	virtual ~TextBoxWindow();

// From ZWindow
	virtual bool WindowQuitRequested();
	};

ZOSWindow* ZAlert::TextBoxWindow::sCreateOSWindow()
	{
	ZOSWindow::CreationAttributes theAttributes;
	theAttributes.fFrame = ZRect(0,0,100,100);
	theAttributes.fLook = ZOSWindow::lookMovableModal;
	theAttributes.fLayer = ZOSWindow::layerDialog;
	theAttributes.fResizable = false;
	theAttributes.fHasSizeBox = false;
	theAttributes.fHasCloseBox = false;
	theAttributes.fHasZoomBox = false;
	theAttributes.fHasMenuBar = false;
	theAttributes.fHasTitleIcon = false;

	return ZApp::sGet()->CreateOSWindow(theAttributes);
	}

ZAlert::TextBoxWindow::TextBoxWindow(const string& inMessage)
:	ZWindow(ZApp::sGet(), sCreateOSWindow())
	{
	this->SetBackInk(ZUIAttributeFactory::sGet()->GetInk_WindowBackground_Dialog());

	ZWindowPane* myWindowPane = new ZWindowPane(this, nil);

	ZRef<ZUICaption> messageCaption = new ZUICaption_Text(inMessage, ZUIAttributeFactory::sGet()->GetFont_SystemLarge(), kIdealTextWidth);
	ZPoint messageSize = messageCaption->GetSize();

// Calculate how much space is needed to display the message and verbiage
	ZPoint windowSize = messageSize;

	windowSize.h += kAOuterSpace + kAOuterSpace;
	windowSize.v += kAOuterSpace + kAOuterSpace;

// Resize the window to have just enough space for the buttons and text
	this->SetSize(windowSize);

	ZUICaptionPane* theMessageCaptionPane = new ZUICaptionPane(myWindowPane, new ZPaneLocatorFixed(kAOuterSpace, kAOuterSpace), messageCaption);
	}

ZAlert::TextBoxWindow::~TextBoxWindow()
	{}

bool ZAlert::TextBoxWindow::WindowQuitRequested()
	{
	return true;
	}

// ==================================================

int32 ZAlert::sAlert(Level inLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
								const vector<ZRef<ZUICaption> >& inButtonCaptions, int32 inDefaultButton, int32 inEscapeButton)
	{
	AlertWindow theAlertWindow(inLevel, inTitle, inMessage, inVerbiage, inTimeOut, inButtonCaptions, inDefaultButton, inEscapeButton);
#if ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	ZUtil_Mac_HL::sInteractWithUser();
#endif // ZCONFIG(OS, MacOS7) || ZCONFIG(OS, Carbon)
	theAlertWindow.CenterDialog();
	theAlertWindow.BringFront(true);
	bool callerExists, calleeExists;
	theAlertWindow.PoseModal(true, &callerExists, &calleeExists);
	ZAssertStop(1, callerExists && calleeExists);
	return theAlertWindow.GetResult();
	}

int32 ZAlert::sAlert(Level inLevel, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut,
								const vector<string>& inButtonTitles, int32 inDefaultButton, int32 inEscapeButton)
	{
	vector<ZRef<ZUICaption> > theCaptions;
	for (size_t x = 0; x < inButtonTitles.size(); ++x)
		theCaptions.push_back(new ZUICaption_Text(inButtonTitles[x], ZUIAttributeFactory::sGet()->GetFont_SystemLarge(), 0));
	return sAlert(inLevel, inTitle, inMessage, inVerbiage, inTimeOut, theCaptions, inDefaultButton, inEscapeButton);
	}

// ==================================================

static ZAlert::Result sInternal_Alert(ZAlert::Level inLevel, ZAlert::Buttons inButtons, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut);

ZAlert::Result ZAlert::sNoteAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelNote, eButtonsOK, inTitle, inMessage, string(), inTimeOut); }
ZAlert::Result ZAlert::sNoteAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelNote, eButtonsOK, inTitle, inMessage, inVerbiage, inTimeOut); }

ZAlert::Result ZAlert::sCautionAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelCaution, eButtonsOKCancel_OKDefault, inTitle, inMessage, string(), inTimeOut); }
ZAlert::Result ZAlert::sCautionAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelCaution, eButtonsOKCancel_OKDefault, inTitle, inMessage, inVerbiage, inTimeOut); }

ZAlert::Result ZAlert::sStopAlert(const string& inTitle, const string& inMessage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelStop, eButtonsOK, inTitle, inMessage, string(), inTimeOut); }
ZAlert::Result ZAlert::sStopAlert(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelStop, eButtonsOK, inTitle, inMessage, inVerbiage, inTimeOut); }

ZAlert::Result ZAlert::sSaveChanges(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelStop, eButtonsSaveChanges, inTitle, inMessage, inVerbiage, inTimeOut); }

ZAlert::Result ZAlert::sOKNoCancel(const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{ return sInternal_Alert(eLevelStop, eButtonsOKNoCancel, inTitle, inMessage, inVerbiage, inTimeOut); }

#if ZCONFIG(OS, Win32)
#include "ZOSWindow_Win.h" // For ZOSApp_Win

struct MessageBox_t
	{
	ZAlert::Level fLevel;
	ZAlert::Buttons fButtons;
	string fTitle;
	string fMessage;
	string fVerbiage;
	bigtime_t fTimeOut;
	};

static int32 sCall_MessageBox(int32 inParam)
	{
	MessageBox_t* theStruct = reinterpret_cast<MessageBox_t*>(inParam);
	UINT boxStyle = MB_TASKMODAL | MB_SETFOREGROUND;

	switch (theStruct->fLevel)
		{
		case ZAlert::eLevelNote: boxStyle |= MB_ICONINFORMATION; break;
		case ZAlert::eLevelCaution: boxStyle |= MB_ICONWARNING; break;
		case ZAlert::eLevelStop: boxStyle |= MB_ICONERROR; break;
		}

	switch (theStruct->fButtons)
		{
		case ZAlert::eButtonsOK:
			boxStyle |= MB_OK | MB_DEFBUTTON1;
			break;
		case ZAlert::eButtonsOKCancel_OKDefault:
			boxStyle |= MB_OKCANCEL | MB_DEFBUTTON1;
			break;
		case ZAlert::eButtonsOKCancel_CancelDefault:
			boxStyle |= MB_OKCANCEL | MB_DEFBUTTON2;
				break;
		case ZAlert::eButtonsSaveChanges:
		case ZAlert::eButtonsOKNoCancel:
			boxStyle |= MB_YESNOCANCEL | MB_DEFBUTTON1;
			break;
		}
	if (ZUtil_Win::sUseWAPI())
		return ::MessageBoxW(nil, ZUnicode::sAsUTF16(theStruct->fMessage).c_str(), ZUnicode::sAsUTF16(theStruct->fTitle).c_str(), boxStyle);
	else
		return ::MessageBoxA(nil, theStruct->fMessage.c_str(), theStruct->fTitle.c_str(), boxStyle);
	}
#endif // ZCONFIG(OS, Win32)

static ZAlert::Result sInternal_Alert(ZAlert::Level inLevel, ZAlert::Buttons inButtons, const string& inTitle, const string& inMessage, const string& inVerbiage, bigtime_t inTimeOut)
	{
#if 0//ZCONFIG(OS, Win32)
	MessageBox_t theStruct;
	theStruct.fLevel = inLevel;
	theStruct.fButtons = inButtons;
	theStruct.fTitle = inTitle;
	theStruct.fMessage = inMessage;
	theStruct.fVerbiage = inVerbiage;
	theStruct.fTimeOut = inTimeOut;
	int32 result = ZOSApp_Win::sGet()->InvokeFunctionInUtilityThread(sCall_MessageBox, reinterpret_cast<int32>(&theStruct));
	if (result == IDOK || result == IDYES)
		return eResultOK;
	else if (result == IDCANCEL)
		return eResultCancel;
	return eResultNo;
#else // ZCONFIG(OS, Win32)
	vector<string> theButtonTitles;
	int32 defaultButton = -1;
	int32 escapeButton = -1;
	int32 cancelButton = 1;
	switch (inButtons)
		{
		case ZAlert::eButtonsOK:
			theButtonTitles.push_back(ZTr("OK"));
			defaultButton = 0;
			break;
		case ZAlert::eButtonsOKCancel_OKDefault:
			theButtonTitles.push_back(ZTr("OK"));
			theButtonTitles.push_back(ZTr("Cancel"));
			defaultButton = 0;
			escapeButton = 1;
			break;
		case ZAlert::eButtonsOKCancel_CancelDefault:
			theButtonTitles.push_back(ZTr("OK"));
			theButtonTitles.push_back(ZTr("Cancel"));
			defaultButton = 1;
			escapeButton = 0;
			break;
		case ZAlert::eButtonsSaveChanges:
			theButtonTitles.push_back(ZTr("Save"));
			theButtonTitles.push_back(ZTr("Don't Save"));
			theButtonTitles.push_back(ZTr("Cancel"));
			defaultButton = 0;
			cancelButton = 2;
			escapeButton = 2;
			break;
		case ZAlert::eButtonsOKNoCancel:
			theButtonTitles.push_back(ZTr("OK"));
			theButtonTitles.push_back(ZTr("No"));
			theButtonTitles.push_back(ZTr("Cancel"));
			defaultButton = 0;
			cancelButton = 2;
			escapeButton = 2;
			break;
		}
	int32 result = ZAlert::sAlert(inLevel, inTitle, inMessage, inVerbiage, inTimeOut, theButtonTitles, defaultButton, escapeButton);
	if (result == 0)
		return ZAlert::eResultOK;
	else if (result == cancelButton)
		return ZAlert::eResultCancel;
	return ZAlert::eResultNo;
#endif
	}


ZWindow* ZAlert::sTextBox(const string& inMessage)
	{
	ZWindow* theWindow = new TextBoxWindow(inMessage);
	theWindow->CenterDialog();
	theWindow->BringFront(true);
	return theWindow;
	}

