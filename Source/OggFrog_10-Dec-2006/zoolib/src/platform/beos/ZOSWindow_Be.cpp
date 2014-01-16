static const char rcsid[] = "@(#) $Id: ZOSWindow_Be.cpp,v 1.9 2006/04/10 20:44:20 agreen Exp $";

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

#include "ZOSWindow_Be.h"


#if ZCONFIG(API_OSWindow, Be)
#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

#include "ZDC_Be.h"
#include "ZMenu_Be.h"
#include "ZDragClip.h" // For ZDragInitiator
#include "ZThreadSimple.h"
#include "ZTicks.h"

#include <interface/Bitmap.h>
#include <interface/MenuBar.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>
#include <interface/Screen.h>

static int const sDebugBe = 3;

static ZAtomic_t sWindowsShown;

// =================================================================================================
#pragma mark -
#pragma mark Utility Methods

static void sSetCursor(const ZCursor& inCursor)
	{
	if (inCursor.IsCustom())
		{
		ZPoint theHotSpot;
		ZDCPixmap colorPixmap, monoPixmap, maskPixmap;
		inCursor.Get(colorPixmap, monoPixmap, maskPixmap, theHotSpot);

// Figure out what's going to be our source pixmap. Prefer the mono pixmap over the color one.
		ZDCPixmap sourcePixmap(monoPixmap ? monoPixmap : colorPixmap);

		unsigned char theCursorData[68];

		theCursorData[0] = 16; // Length of a side of the cursor's bounding box
		theCursorData[1] = 1; // Bit depth of the cursor data
		theCursorData[2] = theHotSpot.v;
		theCursorData[3] = theHotSpot.h;

		unsigned char* thePtr = &theCursorData[4];
		for (ZCoord y = 0; y < 16; ++y)
			{
			for (int count = 0; count < 2; ++count)
				{
				unsigned char theBitMask = 1 << 7;
				unsigned char theResult = 0;
				for (ZCoord x = 0; x < 8; ++x)
					{
					if (sourcePixmap.GetPixel(count*8 + x, y) == ZRGBColor::sBlack)
						theResult |= theBitMask;
					theBitMask = theBitMask >> 1;
					}
				*thePtr++ = theResult;
				}
			}
		for (ZCoord y = 0; y < 16; ++y)
			{
			for (int count = 0; count < 2; ++count)
				{
				unsigned char theBitMask = 1 << 7;
				unsigned char theResult = 0;
				for (ZCoord x = 0; x < 8; ++x)
					{
					if (maskPixmap.GetPixel(count*8 + x, y) == ZRGBColor::sWhite)
						theResult |= theBitMask;
	// Also mask out any bits that are set in the sourcePixmap (BeOS only draws set bits)
					if (sourcePixmap.GetPixel(count*8 + x, y) == ZRGBColor::sBlack)
						theResult |= theBitMask;
					theBitMask = theBitMask >> 1;
					}
				*thePtr++ = theResult;
				}
			}
		be_app->SetCursor(theCursorData);
		}
	else
		{
		const void* beCursor = B_HAND_CURSOR;
		switch (inCursor.GetCursorType())
			{
			case ZCursor::eCursorTypeIBeam: beCursor = B_I_BEAM_CURSOR; break;
//			case ZCursor::eCursorTypeCross: theName = IDC_CROSS; break;
//			case ZCursor::eCursorTypePlus: theName = IDC_CROSS; break;
//			case ZCursor::eCursorTypeWatch: theName = IDC_WAIT; break;
			case ZCursor::eCursorTypeArrow:
			default:
				beCursor = B_HAND_CURSOR;
				break;
			}
		be_app->SetCursor(beCursor);
		}
	}


static void sFromLayerAndLook(const ZOSWindow::CreationAttributes& inAttributes,
			window_look& outWindow_look, window_feel& outWindow_feel, uint32& outFlags)
	{
	outFlags = 0;

	switch (inAttributes.fLook)
		{
		case ZOSWindow::lookDocument:
			{
			if (inAttributes.fHasSizeBox)
				outWindow_look = B_DOCUMENT_WINDOW_LOOK;
			else
				outWindow_look = B_TITLED_WINDOW_LOOK;
			}
			break;
		case ZOSWindow::lookPalette:
			{
			outWindow_look = B_FLOATING_WINDOW_LOOK;
			}
			break;
		case ZOSWindow::lookAlert:
		case ZOSWindow::lookMovableAlert:
		case ZOSWindow::lookModal:
		case ZOSWindow::lookMovableModal:
			{
			outWindow_look = B_MODAL_WINDOW_LOOK;
			}
			break;
		case ZOSWindow::lookThinBorder:
		case ZOSWindow::lookMenu:
			{
			outWindow_look = B_BORDERED_WINDOW_LOOK;
			}
			break;
		default:
			ZUnimplemented();
		};

	switch (inAttributes.fLayer)
		{
		case ZOSWindow::layerSinker: outWindow_feel = B_NORMAL_WINDOW_FEEL; break;
		case ZOSWindow::layerDocument: outWindow_feel = B_NORMAL_WINDOW_FEEL; break;
		case ZOSWindow::layerFloater:
		case ZOSWindow::layerMenu:
			outWindow_feel = B_FLOATING_APP_WINDOW_FEEL;
			outFlags |= B_AVOID_FOCUS;
			break;
		case ZOSWindow::layerDialog: outWindow_feel = B_MODAL_APP_WINDOW_FEEL; break;
		default:
			ZUnimplemented();
		};

	if (!inAttributes.fResizable)
		outFlags |= B_NOT_RESIZABLE;
	if (!inAttributes.fHasCloseBox)
		outFlags |= B_NOT_CLOSABLE;
	if (!inAttributes.fHasZoomBox)
		outFlags |= B_NOT_ZOOMABLE;
	}

static void sDispatchMessageToOwnerNoThrow(ZOSWindow_Be* inOSWindow, const ZMessage& inMessage)
	{
	try
		{
		inOSWindow->DispatchMessageToOwner(inMessage);
		}
	catch (...)
		{}
	}

static void sDispatchMessageNoThrow(ZOSWindow_Be* inOSWindow, const ZMessage& inMessage)
	{
	try
		{
		inOSWindow->DispatchMessage(inMessage);
		}
	catch (...)
		{}
	}

static void sDispatchMessageToOwnerNoThrow(ZOSApp_Be* inOSApp, const ZMessage& inMessage)
	{
	try
		{
		inOSApp->DispatchMessageToOwner(inMessage);
		}
	catch (...)
		{}
	}

static void sDispatchMessageNoThrow(ZOSApp_Be* inOSApp, const ZMessage& inMessage)
	{
	try
		{
		inOSApp->DispatchMessage(inMessage);
		}
	catch (...)
		{}
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZBMessageHelper

class ZBMessageHelper
	{
public:
	ZBMessageHelper();
	~ZBMessageHelper();

	int32 QueueZMessage(const ZMessage& inMessage);
	void GetZMessageFromID(int32 inID, ZMessage& outMessage);

protected:
	int32 fNextID;
	deque<pair<int32, ZMessage> > fMessageQueue;
	};

ZBMessageHelper::ZBMessageHelper()
:	fNextID(0)
	{}

ZBMessageHelper::~ZBMessageHelper()
	{
	}

int32 ZBMessageHelper::QueueZMessage(const ZMessage& inMessage)
	{
	int32 theID = fNextID++;
	fMessageQueue.push_back(make_pair(theID, inMessage));
	return theID;
	}

void ZBMessageHelper::GetZMessageFromID(int32 inID, ZMessage& outMessage)
	{
	int32 dumpedCount = 0;
	bool foundOne = false;
	size_t index = 0;
	for (deque<pair<int32, ZMessage> >::iterator i = fMessageQueue.begin(); i != fMessageQueue.end(); ++i)
		{
		if ((*i).first == inID)
			{
			if (i != fMessageQueue.begin())
				ZDebugLogf(0, ("Out of order message, id %d", inID));
			outMessage = (*i).second;
			fMessageQueue.erase(i);
			return;
			}
		}
	ZDebugStopf(0, ("Did not find message matching id %d", inID));
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMutex_BLooper

ZMutex_BLooper::ZMutex_BLooper(BLooper* inLooper)
:	fLooper(inLooper)
	{}

ZMutex_BLooper::~ZMutex_BLooper()
	{}

ZThread::Error ZMutex_BLooper::Acquire()
	{
	if (fLooper->Lock())
		return ZThread::errorNone;
	return ZThread::errorDisposed;
	}

ZThread::Error ZMutex_BLooper::Acquire(bigtime_t inMicroseconds)
	{
	status_t result = fLooper->LockWithTimeout(inMicroseconds);
	if (result == B_OK)
		return ZThread::errorNone;
	if (result == B_TIMED_OUT)
		return ZThread::errorTimeout;
	return ZThread::errorDisposed;
	}

void ZMutex_BLooper::Release()
	{ fLooper->Unlock(); }

bool ZMutex_BLooper::IsLocked()
	{ return fLooper->IsLocked(); }

string* ZMutex_BLooper::CheckForDeadlockImp(ZThread* iAcquiringThread)
	{
	// We need to be able to convert from the owning thread_id to
	// the owning ZThread. For now I'm punting on this.
	return nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDragInitiator_Be

class ZDragInitiator_Be : public ZDragInitiator
	{
public:
	ZDragInitiator_Be(BView* inBView);
	virtual ~ZDragInitiator_Be();
protected:
	virtual void Internal_DoDrag(const ZTuple& inTuple, ZPoint inStartPoint, ZPoint inHotPoint, const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap, const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap, ZDragSource* inDragSource);
	BView* fBView;
	};

ZDragInitiator_Be::ZDragInitiator_Be(BView* inBView)
:	fBView(inBView)
	{}
ZDragInitiator_Be::~ZDragInitiator_Be()
	{}

static bool sMungeProc_SetAlphaFromMask(ZCoord hCoord, ZCoord vCoord, ZRGBColor& inOutColor, const void* inConstRefCon, void* inRefCon)
	{
	const ZDCPixmap* theMask = reinterpret_cast<const ZDCPixmap*>(inConstRefCon);
	inOutColor.alpha = (unsigned char)(theMask->GetPixel(hCoord, vCoord).NTSCLuminance() * 0x80U);
	return true;
	}

static bool sMungeProc_SetAlphaFromRgn(ZCoord hCoord, ZCoord vCoord, ZRGBColor& inOutColor, const void* inConstRefCon, void* inRefCon)
	{
	const ZDCRgn* theRgn = reinterpret_cast<const ZDCRgn*>(inConstRefCon);
	if (theRgn->Contains(ZPoint(hCoord, vCoord)))
		inOutColor.alpha = 0x80U;
	else
		inOutColor.alpha = 0x0U;
	return true;
	}

static bool sMungeProc_SetAlphaSimple(ZCoord hCoord, ZCoord vCoord, ZRGBColor& inOutColor, const void* inConstRefCon, void* inRefCon)
	{
	inOutColor.alpha = 0x80U;
	return true;
	}

void ZDragInitiator_Be::Internal_DoDrag(const ZTuple& inTuple, ZPoint inStartPoint, ZPoint inHotPoint, const ZDCRgn* inDragRgn, const ZDCPixmap* inDragPixmap, const ZDCRgn* inDragMaskRgn, const ZDCPixmap* inDragMaskPixmap, ZDragSource* inDragSource)
	{
	BMessage theMessage('ZLib');
	theMessage.AddString("Type", "Drag");

	if (inDragPixmap && *inDragPixmap)
		{
		ZDCPixmap tempPixmap(*inDragPixmap);
		if (inDragMaskPixmap && *inDragMaskPixmap)
			tempPixmap.Munge(sMungeProc_SetAlphaFromMask, inDragMaskPixmap, nil);
		else if (inDragMaskRgn)
			tempPixmap.Munge(sMungeProc_SetAlphaFromRgn, inDragMaskRgn, nil);
		else if (inDragRgn)
			tempPixmap.Munge(sMungeProc_SetAlphaFromRgn, inDragRgn, nil);
		else
			tempPixmap.Munge(sMungeProc_SetAlphaSimple, nil, nil);

		ZPoint theSize = tempPixmap.Size();
		BBitmap* theBitmap = new BBitmap(ZRect(theSize), B_RGBA32_LITTLE);
		ZDCPixmapNS::RasterDesc theRasterDesc(ZDCPixmapNS::PixvalDesc(false, 32), theBitmap->BytesPerRow(), theSize.v, false);
		tempPixmap.CopyTo(ZPoint(0, 0), theBitmap->Bits(), theRasterDesc, ZDCPixmapNS::PixelDesc(0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000), ZRect(theSize));
		fBView->DragMessage(&theMessage, theBitmap, B_OP_ALPHA, inHotPoint);
		}
	else
		{
		ZAssert(inDragRgn);
		fBView->DragMessage(&theMessage, inDragRgn->Bounds() - inHotPoint + fBView->ConvertFromScreen(inStartPoint));
		}
	delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Be::RealWindow

class ZOSWindow_Be::RealWindow : public BWindow
	{
public:
	RealWindow(ZOSWindow_Be* inOSWindow, BRect inInitialFrame, const char* inTitle, window_look inWindowLook, window_feel inWindowFeel,
			uint32 inFlags, uint32 inWorkSpace = B_CURRENT_WORKSPACE);
	virtual ~RealWindow();

	void OSWindowDisposed(ZOSWindow_Be* inOSWindow);

// From BLooper via BWindow
	virtual void DispatchMessage(BMessage* inMessage, BHandler* inTarget);
	virtual bool QuitRequested();

// From BHandler via BWindow
	virtual void MessageReceived(BMessage* inMessage);

// From BWindow
	virtual void MenusBeginning();
	virtual void Show();
	virtual void Hide();

// Our protocol
	ZOSWindow_Be* GetOSWindow()
		{ return fOSWindow; }

protected:
	ZOSWindow_Be* fOSWindow;
	};

ZOSWindow_Be::RealWindow::RealWindow(ZOSWindow_Be* inOSWindow, BRect inInitialFrame, const char* inTitle,
	window_look inWindowLook, window_feel inWindowFeel, uint32 inFlags, uint32 inWorkSpace = B_CURRENT_WORKSPACE)
:	BWindow(inInitialFrame, inTitle, inWindowLook, inWindowFeel, inFlags, inWorkSpace),
	fOSWindow(inOSWindow)
	{
	ZAssert(fOSWindow);
	}

ZOSWindow_Be::RealWindow::~RealWindow()
	{
	bool wasHidden = this->IsHidden();
	if (!wasHidden)
		{
		if (!this->IsFloating())
			ZAtomic_Add(&sWindowsShown, -1);
		}
	ZAssert(fOSWindow == nil);
	}

void ZOSWindow_Be::RealWindow::OSWindowDisposed(ZOSWindow_Be* inOSWindow)
	{
	ZAssert(fOSWindow == inOSWindow);
	fOSWindow = nil;
	this->Quit();
	}

// From BLooper via BWindow
void ZOSWindow_Be::RealWindow::DispatchMessage(BMessage* inMessage, BHandler* inTarget)
	{
	if (fOSWindow)
		fOSWindow->BWindow_DispatchMessage(inMessage, inTarget);
	else
		BWindow::DispatchMessage(inMessage, inTarget);
	}

bool ZOSWindow_Be::RealWindow::QuitRequested()
	{
	ZDebugStopf(1, ("ZOSWindow_Be::RealWindow::QuitRequested, this should not get called"));
	return false;
	}

// From BHandler via BWindow
void ZOSWindow_Be::RealWindow::MessageReceived(BMessage* inMessage)
	{
	if (fOSWindow)
		fOSWindow->BWindow_MessageReceived(inMessage);
	else
		BWindow::MessageReceived(inMessage);
	}

// From BWindow
void ZOSWindow_Be::RealWindow::MenusBeginning()
	{
	if (fOSWindow)
		fOSWindow->BWindow_MenusBeginning();
	}

void ZOSWindow_Be::RealWindow::Show()
	{
	bool wasHidden = this->IsHidden();
	BWindow::Show();
	bool isHidden = this->IsHidden();
	if (wasHidden != !isHidden)
		{
		if (!this->IsFloating())
			ZAtomic_Add(&sWindowsShown, 1);
		ZOSApp_Be::sGet()->Internal_RecordWindowRosterChange();
		}
	}

void ZOSWindow_Be::RealWindow::Hide()
	{
	bool wasHidden = this->IsHidden();
	BWindow::Hide();
	bool isHidden = this->IsHidden();
	if (!wasHidden && isHidden)
		{
		if (!this->IsFloating())
			ZAtomic_Add(&sWindowsShown, -1);
		ZOSApp_Be::sGet()->Internal_RecordWindowRosterChange();
		}
	}


// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Be::RealView

class ZOSWindow_Be::RealView : public BView
	{
public:
	RealView(ZOSWindow_Be* inOSWindow, BRect inFrame, const char* inName, uint32 inResizeMask, uint32 inFlags);
	virtual ~RealView();

// Our protocol
	void OSWindowDisposed(ZOSWindow_Be* inOSWindow);

// From BView
	virtual void Draw(BRect inUpdateRect);
	virtual void FrameMoved(BPoint inNewPosition);
	virtual void FrameResized(float inNewWidth, float inNewHeight);
	virtual void MouseDown(BPoint inPoint);
	virtual void MouseUp(BPoint inPoint);
	virtual void MouseMoved(BPoint inPoint, uint32 inTransit, const BMessage* inBMessage);
	virtual void KeyDown(const char* inBytes, int32 inNumBytes);
	virtual void WindowActivated(bool inActive);

protected:
	ZOSWindow_Be* fOSWindow;
	};

ZOSWindow_Be::RealView::RealView(ZOSWindow_Be* inOSWindow, BRect inFrame, const char* inName, uint32 inResizeMask, uint32 inFlags)
:	BView(inFrame, inName, inResizeMask, inFlags),
	fOSWindow(inOSWindow)
	{
	}

ZOSWindow_Be::RealView::~RealView()
	{
	}

// Our protocol
void ZOSWindow_Be::RealView::OSWindowDisposed(ZOSWindow_Be* inOSWindow)
	{
	ZAssert(fOSWindow == inOSWindow);
	fOSWindow = nil;
	}

// From BView
void ZOSWindow_Be::RealView::Draw(BRect inUpdateRect)
	{
	if (fOSWindow)
		fOSWindow->BView_Draw(inUpdateRect);
	}

void ZOSWindow_Be::RealView::FrameMoved(BPoint inNewPosition)
	{
	if (fOSWindow)
		fOSWindow->BView_FrameMoved(inNewPosition);
	}

void ZOSWindow_Be::RealView::FrameResized(float inNewWidth, float inNewHeight)
	{
	if (fOSWindow)
		fOSWindow->BView_FrameResized(inNewWidth, inNewHeight);
	}

void ZOSWindow_Be::RealView::MouseDown(BPoint inPoint)
	{
	if (fOSWindow)
		fOSWindow->BView_MouseDown(inPoint);
	}

void ZOSWindow_Be::RealView::MouseUp(BPoint inPoint)
	{
	if (fOSWindow)
		fOSWindow->BView_MouseUp(inPoint);
	}

void ZOSWindow_Be::RealView::MouseMoved(BPoint inPoint, uint32 inTransit, const BMessage* inBMessage)
	{
	if (fOSWindow)
		fOSWindow->BView_MouseMoved(inPoint, inTransit, inBMessage);
	}

void ZOSWindow_Be::RealView::KeyDown(const char* inBytes, int32 inNumBytes)
	{
	if (fOSWindow)
		fOSWindow->BView_KeyDown(inBytes, inNumBytes);
	}

void ZOSWindow_Be::RealView::WindowActivated(bool inActive)
	{
	if (fOSWindow)
		fOSWindow->BView_WindowActivated(inActive);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_Window

class ZDCCanvas_Be_Window : public ZDCCanvas_Be
	{
public:
	ZDCCanvas_Be_Window(ZOSWindow_Be* inWindow, BView* inBView);
	virtual ~ZDCCanvas_Be_Window();

// Our protocol
	void OSWindowDisposed();

// From ZRefCountedWithFinalization via ZDCCanvas_Be
	virtual void Finalize();

// From ZDCCanvas
	virtual void Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord inDeltaH, ZCoord inDeltaV);

protected:
	ZOSWindow_Be* fOSWindow;
	};

ZDCCanvas_Be_Window::ZDCCanvas_Be_Window(ZOSWindow_Be* inWindow, BView* inBView)
	{
	fOSWindow = inWindow;
	fBView = inBView;
	}

ZDCCanvas_Be_Window::~ZDCCanvas_Be_Window()
	{
	ZAssert(fBView == nil);
	fBView = nil;
	}

// Our protocol
void ZDCCanvas_Be_Window::OSWindowDisposed()
	{
	fOSWindow = nil;
	fBView = nil;
	}

// From ZRefCountedWithFinalization via ZDCCanvas_Be
void ZDCCanvas_Be_Window::Finalize()
	{
// If we're being finalized, then our window must be disposed already and will have
// called our OSWindowDisposed, which will have set fOSWindow to nil.
	ZAssertStop(2, fOSWindow == nil);
	this->FinalizationComplete();
	delete this;
	}

void ZDCCanvas_Be_Window::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fBView)
		return;
	if (hDelta == 0 && vDelta == 0)
		return;

	ZPoint offset(hDelta, vDelta);

	ZDCRgn destRgn = ((ioState.fClip + ioState.fClipOrigin) & (inRect + ioState.fOrigin));

// srcRgn is the set of pixels we're want and are able to copy from.
	ZDCRgn srcRgn = ((destRgn - offset) & (inRect + ioState.fOrigin));

// drawnRgn is the destination pixels that will be drawn by the CopyBits call, it's the srcRgn
// shifted back to the destination.
	ZDCRgn drawnRgn = srcRgn + offset;

// invalidRgn is the destination pixels we wanted to draw but could not because they were
// outside the visRgn, or were in the excludeRgn
	ZDCRgn invalidRgn = destRgn - drawnRgn;

// And set the clip (drawnRgn)
	fBView->ConstrainClippingRegion(drawnRgn.GetBRegion());
	++fChangeCount_Clip;

	ZRect drawnBounds = drawnRgn.Bounds();
	fBView->CopyBits(drawnBounds - offset, drawnBounds);

	fBView->Invalidate(invalidRgn.GetBRegion());
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Be

ZOSWindow_Be::ZOSWindow_Be(const ZOSWindow::CreationAttributes& inAttributes)
	{
	fMutex_Looper = nil;
	fMessageHelper = nil;
	fPendingIdle = false;
	fRealMenuBar = nil;
	fRealWindow = nil;
	fRealView = nil;
	fCapturedMouse = false;
	fContainsMouse = false;

	window_look theRealWindowLook;
	window_feel theRealWindowFeel;
	uint32 theFlags;
	sFromLayerAndLook(inAttributes, theRealWindowLook, theRealWindowFeel, theFlags);
	fRealWindow = new RealWindow(this, inAttributes.fFrame, "Untitled", theRealWindowLook, theRealWindowFeel, theFlags | B_WILL_ACCEPT_FIRST_CLICK);

	fMutex_Looper = new ZMutex_BLooper(fRealWindow);
	fMessageHelper = new ZBMessageHelper;

	if (inAttributes.fHasMenuBar)
		{
		fRealMenuBar = new BMenuBar(fRealWindow->Bounds(), "MenuBar");
		BMenu* aMenu = new BMenu("Test");
		aMenu->AddItem(new BMenuItem("Dummy", nil));
		fRealMenuBar->AddItem(aMenu);
		fRealMenuBar->ResizeToPreferred();
		fRealWindow->SetKeyMenuBar(fRealMenuBar);
		fRealWindow->AddChild(fRealMenuBar);
		}

	ZRect windowBounds = fRealWindow->Bounds();
	if (fRealMenuBar)
		windowBounds.top += ZRect(fRealMenuBar->Bounds()).Height();

	fRealView = new RealView(this, windowBounds, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	fRealView->SetViewColor(B_TRANSPARENT_32_BIT);
	fRealWindow->AddChild(fRealView);
	fRealView->MakeFocus(true);

// BeOS's model whereby saved states *accumulate* is a bit of a pain for our purposes, so we do a
// PushState here so that we always have a known clean state to return to, one where the clipping
// region is not constrained and thus we're to apply any clipping we like. 
	fRealView->PushState();

	fCanvas = new ZDCCanvas_Be_Window(this, fRealView);

	ZOSApp_Be::sGet()->AddOSWindow(this);

// At this point we have created our BWindow, and we own its lock, but its message loop has not started yet. Normally you call
// Show on the window, which both makes it visible *and* starts its message loop thread. But we have slightly different semantics
// defined for our windows: By the time a ZOSWindow has been created, its message loop thread has also been created and is running,
// but is blocked waiting for its lock to be released, which lock is held by the creating thread, ie the one that executed the
// ZOSWindow's constructor (*this* thread right here). There's two ways to subvert/trick BeOS to get the semantics we want. The
// first is to call Lock() on the window and then to directly call Run() on it, which relies on BWindow::Run() updating its internal
// flags so that a later Show doesn't try to spawn a second thread. The cleaner way is to call Lock(), then call Hide(), then call Show(),
// which is what we do.
#if 1
	fRealWindow->Lock();
	fRealWindow->Hide();
	fRealWindow->Show();
#else
	fRealWindow->Lock();
	fRealWindow->Run();
#endif
	}

ZOSWindow_Be::~ZOSWindow_Be()
	{
	ASSERTLOCKED();
	ZOSApp_Be::sGet()->RemoveOSWindow(this);
	fRealView->OSWindowDisposed(this);
	fRealWindow->OSWindowDisposed(this);
	fCanvas->OSWindowDisposed();

	delete fMutex_Looper;
	fMutex_Looper = nil;

	ZMutexLocker structureLocker(fMutex_Structure);
	delete fMessageHelper;
	fMessageHelper = nil;
	}

ZMutexBase& ZOSWindow_Be::GetLock()
	{ return *fMutex_Looper; }

void ZOSWindow_Be::QueueMessage(const ZMessage& inMessage)
	{
	if (fMutex_Structure.Acquire() != ZThread::errorNone)
		return;

	int32 theID = fMessageHelper->QueueZMessage(inMessage);
	BMessage theBMessage('ZLib');
	theBMessage.AddString("Type", "ZMessage");
	theBMessage.AddInt32("ID", theID);
	fRealWindow->PostMessage(&theBMessage);

	fMutex_Structure.Release();
	}

bool ZOSWindow_Be::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Idle")
			{
			fMutex_Structure.Acquire();
			fPendingIdle = false;
			fMutex_Structure.Release();
			this->DispatchMessageToOwner(inMessage);
			return true;
			}

		}
	return false;
	}

void ZOSWindow_Be::SetShownState(ZShownState inState)
	{
	ASSERTLOCKED();
	if (inState == eZShownStateShown)
		{
		if (fRealWindow->IsHidden())
			{
			fRealWindow->Show();
			return;
			}
		if (fRealWindow->IsMinimized())
			{
			fRealWindow->Minimize(false);
			return;
			}
		}
	else if (inState == eZShownStateMinimized)
		{
		if (fRealWindow->IsHidden())
			{
			fRealWindow->Show();
			fRealWindow->Minimize(true);
			return;
			}
		if (!fRealWindow->IsMinimized())
			{
			fRealWindow->Minimize(true);
			return;
			}
		return;
		}
	else if (inState == eZShownStateHidden)
		{
		if (fRealWindow->IsHidden())
			return;
		fRealWindow->Hide();
		}
	else
		{
		ZUnimplemented();
		}
	}

ZShownState ZOSWindow_Be::GetShownState()
	{
	ASSERTLOCKED();
	if (fRealWindow->IsHidden())
		return eZShownStateHidden;
	if (fRealWindow->IsMinimized())
		return eZShownStateMinimized;
	return eZShownStateShown;
	}

void ZOSWindow_Be::SetTitle(const string& inTitle)
	{
	ASSERTLOCKED();
	fRealWindow->SetTitle(inTitle.c_str());
	}

string ZOSWindow_Be::GetTitle()
	{
	ASSERTLOCKED();
	return fRealWindow->Title();
	}

void ZOSWindow_Be::SetCursor(const ZCursor& inCursor)
	{
	ASSERTLOCKED();
	fCursor = inCursor;
	if (fContainsMouse || fCapturedMouse)
		{
		ZDebugLogf(sDebugBe, ("setting cursor in set cursor"));
		::sSetCursor(fCursor);
		}
	}

void ZOSWindow_Be::SetLocation(ZPoint inLocation)
	{
	ASSERTLOCKED();
	ZPoint currentLocation = fRealView->ConvertToScreen(BPoint(0, 0));
	ZPoint offset = inLocation - currentLocation;
	fRealWindow->MoveBy(offset.h, offset.v);
	}

ZPoint ZOSWindow_Be::GetLocation()
	{
	ASSERTLOCKED();
	return fRealView->ConvertToScreen(BPoint(0, 0));
	}

void ZOSWindow_Be::SetSize(ZPoint inNewSize)
	{
	ASSERTLOCKED();
	ZPoint currentSize = ZRect(fRealView->Bounds()).Size();
	ZPoint delta = inNewSize - currentSize;
	fRealWindow->ResizeBy(delta.h, delta.v);
	}

ZPoint ZOSWindow_Be::GetSize()
	{
	ASSERTLOCKED();
	return ZRect(fRealView->Bounds()).Size();
	}

void ZOSWindow_Be::SetFrame(const ZRect& inFrame)
	{
	ASSERTLOCKED();
// Account for menu bar size
	ZRect realFrame(inFrame);
	if (fRealMenuBar)
		realFrame.top -= ZRect(fRealMenuBar->Bounds()).Height();
	fRealWindow->Zoom(realFrame.TopLeft(), realFrame.Width(), realFrame.Height());
	}

void ZOSWindow_Be::SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize)
	{
	ASSERTLOCKED();
	}

void ZOSWindow_Be::PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists)
	{
	ASSERTLOCKED();
	ZUnimplemented();
	}

void ZOSWindow_Be::EndPoseModal()
	{
	ZUnimplemented();
	}

bool ZOSWindow_Be::WaitingForPoseModal()
	{
//	ZUnimplemented();
	return false;
	}

void ZOSWindow_Be::Zoom()
	{
	ASSERTLOCKED();
	fRealWindow->Zoom();
	}

void ZOSWindow_Be::BringFront(bool inMakeVisible)
	{
	ASSERTLOCKED();
	if (fRealWindow->IsHidden())
		{
		if (inMakeVisible)
			{
			fRealWindow->Show();
			fRealWindow->Activate(true);
			}
		}
	else
		{
		fRealWindow->Activate(true);
		}
	}

bool ZOSWindow_Be::GetActive()
	{
	ASSERTLOCKED();
// Floating windows are considered to be always active
	if (fRealWindow->Look() == B_FLOATING_WINDOW_LOOK)
		return true;
	return fRealWindow->IsActive();
	}

void ZOSWindow_Be::GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	ASSERTLOCKED();
	switch (fRealWindow->Look())
		{
		case B_DOCUMENT_WINDOW_LOOK:
		case B_TITLED_WINDOW_LOOK:
			outTopLeftInset = ZPoint(5, 24);
			outBottomRightInset = ZPoint(-5, -5);
			break;
		case B_BORDERED_WINDOW_LOOK:
			outTopLeftInset = ZPoint(1, 1);
			outBottomRightInset = ZPoint(-1, -1);
			break;
		case B_MODAL_WINDOW_LOOK:
			outTopLeftInset = ZPoint(5, 5);
			outBottomRightInset = ZPoint(-5, -5);
			break;
		case B_FLOATING_WINDOW_LOOK:
			outTopLeftInset = ZPoint(3, 18);
			outBottomRightInset = ZPoint(-3, -3);
			break;
		}
	if (fRealMenuBar)
		outTopLeftInset.v += ZRect(fRealMenuBar->Bounds()).Height();
	}

bool ZOSWindow_Be::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();
	ZUnimplemented();
	return false;
	}

ZDCRgn ZOSWindow_Be::GetVisibleContentRgn()
	{
	ASSERTLOCKED();
	ZRect myBounds(this->GetSize());
	ZDCRgn contentRgn(myBounds);
// If we have a resize box then clip out that rect
	if (fRealWindow->Look() == B_DOCUMENT_WINDOW_LOOK)
		{
		myBounds.left = myBounds.right - 14;
		myBounds.top = myBounds.bottom - 14;
		contentRgn -= myBounds;
		}
	return contentRgn;
	}

ZPoint ZOSWindow_Be::ToGlobal(const ZPoint& inWindowPoint)
	{
	ASSERTLOCKED();
	return fRealView->ConvertToScreen(inWindowPoint);
	}

void ZOSWindow_Be::InvalidateRegion(const ZDCRgn& inBadRgn)
	{
	ASSERTLOCKED();
	if (!fRealWindow->IsHidden())
		{
		ZDCRgn localRgn(inBadRgn);
		fRealView->Invalidate(localRgn.GetBRegion());
		}
	}

void ZOSWindow_Be::UpdateNow()
	{
	ASSERTLOCKED();
// So, what the hell's happening with the pop and push states? Well, UpdateIfNeeded calls back in to our view to do its drawing, but I reckon
// it does a push/pop pair, and because of how nested states *accumulate* clipping regions etc. means that we can never get our clip blown back
// up to what we need. This pop/push will restore the view to whatever condition it was in when its extant canvas was created. This
// will break if there is no canvas already in use etc. I need to think more about all this before finishing everything up.
	if (fRealWindow->NeedsUpdate())
		{
		fRealView->PopState();
		fRealWindow->UpdateIfNeeded();
		fRealView->PushState();
		}
	}

ZRef<ZDCCanvas> ZOSWindow_Be::GetCanvas()
	{
	ASSERTLOCKED();
	return fCanvas;
	}

void ZOSWindow_Be::AcquireCapture()
	{
	ASSERTLOCKED();
	if (fCapturedMouse)
		return;
	fCapturedMouse = true;
	fRealView->SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
	}

void ZOSWindow_Be::ReleaseCapture()
	{
	ASSERTLOCKED();
	if (!fCapturedMouse)
		return;
	fCapturedMouse = false;
	fRealView->SetEventMask(0);
	}

void ZOSWindow_Be::GetNative(void* outNative)
	{
	ZUnimplemented();
// Have not decided what should be returned
	}

void ZOSWindow_Be::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	if (fRealMenuBar)
		{
		ZMenu_Be::sInstallMenus(inMenuBar, fRealMenuBar);
		fMenuBar = inMenuBar;
		}
	}

ZDragInitiator* ZOSWindow_Be::CreateDragInitiator()
	{
	return new ZDragInitiator_Be(fRealView);
	}

ZRect ZOSWindow_Be::GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly)
	{
	ZRect theRect(ZRect::sZero);
	if (inScreenIndex == 0)
		theRect = BScreen(B_MAIN_SCREEN_ID).Frame();
	return theRect;
	}

void ZOSWindow_Be::Internal_RecordIdle()
	{
	if (fMutex_Structure.Acquire() != ZThread::errorNone)
		return;

	if (!fPendingIdle)
		{
		fPendingIdle = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Idle");
		this->QueueMessage(theMessage);
		}
	fMutex_Structure.Release();
	}

ZOSWindow_Be* ZOSWindow_Be::sFromBWindow(BWindow* inBWindow)
	{
	if (RealWindow* theRealWindow = dynamic_cast<RealWindow*>(inBWindow))
		return theRealWindow->GetOSWindow();
	return nil;
	}

BWindow* ZOSWindow_Be::GetBWindow()
	{ return fRealWindow; }

// ==================================================

void ZOSWindow_Be::BWindow_DispatchMessage(BMessage* inMessage, BHandler* inTarget)
	{
	switch (inMessage->what)
		{
		case '_UPD':
			{
			fRealView->PopState();
			fRealView->PushState();
			fRealWindow->BWindow::DispatchMessage(inMessage, inTarget);
			break;
			}
		case 'ZLib':
			{
			const char* theType;
			if (inMessage->FindString("Type", &theType) == B_OK)
				{
				if (::strcmp(theType, "ZMessage") == 0)
					{
					int32 theID;
					if (inMessage->FindInt32("ID", &theID) == B_OK)
						{
						fMutex_Structure.Acquire();
						ZMessage theZMessage;
						fMessageHelper->GetZMessageFromID(theID, theZMessage);
						fMutex_Structure.Release();
						::sDispatchMessageNoThrow(this, theZMessage);
						}
					}
				else if (::strcmp(theType, "ZMenuItem") == 0)
					{
					void* theVoidPtr;
					if (inMessage->FindPointer("ZMenuItem", &theVoidPtr) == B_OK)
						{
						if (ZRef<ZMenuItem> theMenuItem = static_cast<ZMenuItem*>(theVoidPtr))
							{
							ZMessage theMessage = theMenuItem->GetMessage();
							theMessage.SetString("what", "zoolib:Menu");
							theMessage.SetInt32("menuCommand", theMenuItem->GetCommand());
							::sDispatchMessageToOwnerNoThrow(this, theMessage);
							}
						}
					}
				}
			break;
			}
		case B_QUIT_REQUESTED:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:Close");
			::sDispatchMessageToOwnerNoThrow(this, theMessage);
			break;
			}
		case B_ZOOM:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:Zoom");
			::sDispatchMessageToOwnerNoThrow(this, theMessage);
			break;
			}
		default:
			{
			fRealWindow->BWindow::DispatchMessage(inMessage, inTarget);
			break;
			}
		}
	}

void ZOSWindow_Be::BWindow_MessageReceived(BMessage* inMessage)
	{
	fRealWindow->BWindow::MessageReceived(inMessage);
	}

void ZOSWindow_Be::BWindow_MenusBeginning()
	{
	if (fRealMenuBar && fOwner)
		{
		fMenuBar.Reset();
		fOwner->OwnerSetupMenus(fMenuBar);
		ZMenu_Be::sPostSetup(fMenuBar, fRealMenuBar);
		}
	fRealWindow->BWindow::MenusBeginning();
	}

// ==================================================

void ZOSWindow_Be::BView_Draw(BRect inUpdateRect)
	{
	ZDCRgn updateClip;
	fRealView->GetClippingRegion(updateClip.GetBRegion());

	ZDC theDC(this->GetCanvas(), ZPoint::sZero);
	theDC.SetClip(this->GetVisibleContentRgn() & updateClip);

	theDC.SetInk(ZRGBColor::sWhite);

	if (fOwner)
		fOwner->OwnerUpdate(theDC);
	}

void ZOSWindow_Be::BView_FrameMoved(BPoint inNewPosition)
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:FrameChange");
	this->DispatchMessageToOwner(theMessage);
	}

void ZOSWindow_Be::BView_FrameResized(float inNewWidth, float inNewHeight)
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:FrameChange");
	this->DispatchMessageToOwner(theMessage);
	}

static ZMessage sMouseBMessageToZMessage(BMessage* inBMessage, BPoint inWhere, int32 inButtons, int32 inModifiers)
	{
	ZMessage theMessage;
	theMessage.SetPoint("where", ZPoint(inWhere));

	bigtime_t theWhen;
	if (B_OK == inBMessage->FindInt64("when", &theWhen))
		theMessage.SetInt64("when", theWhen);

	int32 theClicks;
	if (B_OK == inBMessage->FindInt32("clicks", &theClicks))
		theMessage.SetInt32("clicks", theClicks);

	int32 theButtons;
	if (B_OK != inBMessage->FindInt32("buttons", &theButtons))
		theButtons = inButtons;
	theMessage.SetBool("buttonLeft", (theButtons & B_PRIMARY_MOUSE_BUTTON) != 0);
	theMessage.SetBool("buttonMiddle", (theButtons & B_SECONDARY_MOUSE_BUTTON) != 0);
	theMessage.SetBool("buttonRight", (theButtons & B_TERTIARY_MOUSE_BUTTON) != 0);
	theMessage.SetBool("button4", false);
	theMessage.SetBool("button5", false);

	int32 theModifiers;
	if (B_OK != inBMessage->FindInt32("modifiers", &theModifiers))
		theModifiers = inModifiers;
	theMessage.SetBool("command", (theModifiers & B_COMMAND_KEY) != 0);
	theMessage.SetBool("option", (theModifiers & B_OPTION_KEY) != 0);
	theMessage.SetBool("shift", (theModifiers & B_SHIFT_KEY) != 0);
	theMessage.SetBool("control", (theModifiers & B_CONTROL_KEY) != 0);
	theMessage.SetBool("capsLock", (theModifiers & B_CAPS_LOCK) != 0);
	return theMessage;
	}

void ZOSWindow_Be::BView_MouseDown(BPoint inPoint)
	{
	ZDebugLogf(sDebugBe, ("ZOSWindow_Be::BView_MouseDown"));
	if (fCapturedMouse)
		{
		if (fContainsMouse)
			{
			ZDebugLogf(sDebugBe, ("ZOSWindow_Be::RealView::MouseDown SetEventMask(B_POINTER_EVENTS"));
			fRealView->SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			}
		else
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:CaptureCancelled");
			this->DispatchMessageToOwner(theMessage);

			this->ReleaseCapture();
			}
		}

	BMessage* currentBMessage = fRealWindow->CurrentMessage();
	ZMessage theMessage = sMouseBMessageToZMessage(currentBMessage, inPoint, 0, 0);
	theMessage.SetString("what", "zoolib:MouseDown");
	theMessage.SetBool("contained", fRealView->Bounds().Contains(inPoint));

	this->DispatchMessageToOwner(theMessage);
	}

void ZOSWindow_Be::BView_MouseUp(BPoint inPoint)
	{
	ZDebugLogf(sDebugBe, ("ZOSWindow_Be::RealView::MouseUp"));
	if (fCapturedMouse)
		{
		ZDebugLogf(sDebugBe, ("ZOSWindow_Be::RealView::MouseUp SetEventMask(B_POINTER_EVENTS"));
		fRealView->SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
		}

	BMessage* currentBMessage = fRealWindow->CurrentMessage();
	ZMessage theMessage = sMouseBMessageToZMessage(currentBMessage, inPoint, 0, 0);
	theMessage.SetString("what", "zoolib:MouseUp");
	theMessage.SetBool("contained", fRealView->Bounds().Contains(inPoint));

	this->DispatchMessageToOwner(theMessage);
	}

void ZOSWindow_Be::BView_MouseMoved(BPoint inPoint, uint32 inTransit, const BMessage* inBMessage)
	{
	ZDebugLogf(sDebugBe, ("ZOSWindow_Be::RealView::MouseMoved"));
	if (inBMessage)
		{
/*		ZDrag theDrag;

		if (inTransit == B_ENTERED_VIEW)
			fOSWindow->fOwner->OwnerDrag(inPoint, ZMouse::eMotionEnter, theDrag);
		else if (inTransit == B_EXITED_VIEW)
			fOSWindow->fOwner->OwnerDrag(inPoint, ZMouse::eMotionLeave, theDrag);
		else if (inTransit == B_INSIDE_VIEW)
			fOSWindow->fOwner->OwnerDrag(inPoint, ZMouse::eMotionMove, theDrag);*/
		}
	else
		{
		bool wasCaptured = fCapturedMouse;
		bool wasContained = fContainsMouse;
// The BeOS does not rigorously send enter, inside, exit, outside messages. If we rely on getting entry
// and exit messages to drive things then we go awry.
		if (inTransit == B_ENTERED_VIEW)
			fContainsMouse = true;
		else if (inTransit == B_EXITED_VIEW)
			fContainsMouse = false;
		else if (inTransit == B_INSIDE_VIEW)
			fContainsMouse = true;
		else if (inTransit == B_OUTSIDE_VIEW)
			fContainsMouse = false;

		if (fContainsMouse != wasContained)
			{
			if (fContainsMouse)
				{
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:MouseEnter");
				theMessage.SetPoint("where", ZPoint(inPoint));
				this->DispatchMessageToOwner(theMessage);
				}
			else
				{
				ZMessage theMessage;
				theMessage.SetString("what", "zoolib:MouseExit");
				theMessage.SetPoint("where", ZPoint(inPoint));
				this->DispatchMessageToOwner(theMessage);
				}
			}

		if (inTransit == B_INSIDE_VIEW || inTransit == B_OUTSIDE_VIEW)
			{
			BMessage* currentBMessage = fRealWindow->CurrentMessage();
			ZMessage theMessage = sMouseBMessageToZMessage(currentBMessage, inPoint, 0, ::modifiers());
			theMessage.SetString("what", "zoolib:MouseChange");
			theMessage.SetBool("contained", fContainsMouse);
			this->DispatchMessageToOwner(theMessage);
			}

		if ((fCapturedMouse && !wasCaptured) || (fContainsMouse && !wasContained))
			{
			ZDebugLogf(sDebugBe, ("setting cursor in mouse move"));
			::sSetCursor(fCursor);
			}

//		if (fCapturedMouse)
//			{
//			BMessage theBMessage('ZLib');
//			theBMessage.SetString("Type", "ZOSWindow_Be::RealView::CheckMouse");
//			fRealWindow->PostMessage(&theBMessage, this);
//			}
		}
	}

void ZOSWindow_Be::BView_KeyDown(const char* inBytes, int32 inNumBytes)
	{
//##	if (fOSWindow->fOwner)
//##		fOwner->OwnerKeyDown(ZEvent_Key(this->Window()->CurrentMessage()));
	}

void ZOSWindow_Be::BView_WindowActivated(bool inActive)
	{
// Any activation change cancels the mouse capture
	if (fCapturedMouse)
		{
		fCapturedMouse = false;
		fRealView->SetEventMask(0);
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:CaptureCancelled");
		this->DispatchMessageToOwner(theMessage);
		}
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:Activate");
	theMessage.SetBool("active", inActive);
	this->DispatchMessageToOwner(theMessage);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Be::RealApp

class ZOSApp_Be::RealApp : public BApplication
	{
public:
	RealApp(const char* inSignature, status_t* outError, ZOSApp_Be* inOSApp);
	virtual ~RealApp();

	void OSAppDisposed(ZOSApp_Be* inOSApp);

// From BLooper via BApplication
	virtual void DispatchMessage(BMessage* inMessage, BHandler* inTarget);

protected:
	friend class ZOSApp_Be;
	ZOSApp_Be* fOSApp;
	};

ZOSApp_Be::RealApp::RealApp(const char* inSignature, status_t* outError, ZOSApp_Be* inOSApp)
:	BApplication(inSignature, outError),
	fOSApp(inOSApp)
	{
	ZAssert(fOSApp);
	}

ZOSApp_Be::RealApp::~RealApp()
	{
	ZAssert(fOSApp == nil);
	}

void ZOSApp_Be::RealApp::OSAppDisposed(ZOSApp_Be* inOSApp)
	{
	ZAssert(fOSApp == inOSApp);
	fOSApp = nil;
	delete this;
	}

// From BLooper via BApplication
void ZOSApp_Be::RealApp::DispatchMessage(BMessage* inMessage, BHandler* inTarget)
	{
	ZAssert(fOSApp);
	fOSApp->BApp_DispatchMessage(inMessage, inTarget);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindowRoster_Be

class ZOSWindowRoster_Be : public ZOSWindowRoster
	{
public:
	ZOSWindowRoster_Be(ZOSApp_Be* inOSApp);
	virtual ~ZOSWindowRoster_Be();

	virtual void Reset();
	virtual bool GetNextOSWindow(bigtime_t inTimeout, ZOSWindow*& outOSWindow);
	virtual void DropCurrentOSWindow();
	virtual void UnlockCurrentOSWindow();

	void OSWindowRemoved(ZOSWindow_Be* inOSWindow);

protected:
	ZOSApp_Be* fOSApp;
	vector<ZOSWindow_Be*> fVector_OSWindowsVisited;
	vector<ZOSWindow_Be*> fVector_OSWindowsDropped;
	ZOSWindow_Be* fCurrentOSWindow;
	ZOSWindowRoster_Be* fNext;
	ZOSWindowRoster_Be* fPrev;
	friend class ZOSApp_Be;
	};

ZOSWindowRoster_Be::ZOSWindowRoster_Be(ZOSApp_Be* inOSApp)
	{
	fOSApp = inOSApp;
	fCurrentOSWindow = nil;

	if (fOSApp->fOSWindowRoster_Head)
		fOSApp->fOSWindowRoster_Head->fPrev = this;
	fNext = fOSApp->fOSWindowRoster_Head;
	fPrev = nil;
	fOSApp->fOSWindowRoster_Head = this;
	}

ZOSWindowRoster_Be::~ZOSWindowRoster_Be()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fNext)
				fNext->fPrev = fPrev;
			if (fPrev)
				fPrev->fNext = fNext;
			if (fOSApp->fOSWindowRoster_Head == this)
				fOSApp->fOSWindowRoster_Head = fNext;
			fNext = nil;
			fPrev = nil;
			}
		}
	}

void ZOSWindowRoster_Be::Reset()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			fVector_OSWindowsVisited.clear();
		}
	}

bool ZOSWindowRoster_Be::GetNextOSWindow(bigtime_t inTimeout, ZOSWindow*& outOSWindow)
	{
	outOSWindow = nil;
	if (fOSApp)
		{
		if (inTimeout < 0)
			inTimeout = 0;
		bigtime_t endTime = ZTicks::sNow() + inTimeout;
		while (true)
			{
			bool anyToAsk = false;
			ZMutexLocker locker(fOSApp->fMutex_Structure, false);
			if (locker.IsOkay())
				{
				for (int32 index = 0; index < fOSApp->fRealApp->CountWindows(); ++index)
					{
					if (ZOSWindow_Be* currentWindow = ZOSWindow_Be::sFromBWindow(fOSApp->fRealApp->WindowAt(index)))
						{
						vector<ZOSWindow_Be*>::const_iterator theIter = find(fVector_OSWindowsVisited.begin(), fVector_OSWindowsVisited.end(), currentWindow);
						if (theIter == fVector_OSWindowsVisited.end())
							{
							theIter = find(fVector_OSWindowsDropped.begin(), fVector_OSWindowsDropped.end(), currentWindow);
							if (theIter == fVector_OSWindowsDropped.end())
								{
								anyToAsk = true;
								status_t result = currentWindow->GetBWindow()->LockWithTimeout(10000);
								if (result == B_OK)
									{
									fVector_OSWindowsVisited.push_back(currentWindow);
									fCurrentOSWindow = currentWindow;
									outOSWindow = currentWindow;
									return true;
									}
								break;
								}
							}
						}
					}
				}
			if (!anyToAsk || ZTicks::sNow() >= endTime)
				return !anyToAsk;
			}
		}
	return true;
	}

void ZOSWindowRoster_Be::DropCurrentOSWindow()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fCurrentOSWindow)
				{
				vector<ZOSWindow_Be*>::iterator theIter = find(fVector_OSWindowsDropped.begin(), fVector_OSWindowsDropped.end(), fCurrentOSWindow);
// Make sure we're not double droppping a window
				ZAssertStop(1, theIter == fVector_OSWindowsDropped.end());
				fVector_OSWindowsDropped.push_back(fCurrentOSWindow);
				}
			}
		}
	}

void ZOSWindowRoster_Be::UnlockCurrentOSWindow()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fCurrentOSWindow)
				fCurrentOSWindow->GetLock().Release();
			}
		}
	}

void ZOSWindowRoster_Be::OSWindowRemoved(ZOSWindow_Be* inOSWindow)
	{
	vector<ZOSWindow_Be*>::iterator theIter;

	theIter = find(fVector_OSWindowsVisited.begin(), fVector_OSWindowsVisited.end(), inOSWindow);
	if (theIter != fVector_OSWindowsVisited.end())
		fVector_OSWindowsVisited.erase(theIter);

	theIter = find(fVector_OSWindowsDropped.begin(), fVector_OSWindowsDropped.end(), inOSWindow);
	if (theIter != fVector_OSWindowsDropped.end())
		fVector_OSWindowsDropped.erase(theIter);

	if (inOSWindow == fCurrentOSWindow)
		fCurrentOSWindow = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_Be_Scratch

class ZDCCanvas_Be_Scratch : public ZDCCanvas_Be_NonWindow
	{
public:
	ZDCCanvas_Be_Scratch();
	virtual ~ZDCCanvas_Be_Scratch();
	};

ZDCCanvas_Be_Scratch::ZDCCanvas_Be_Scratch()
	{}

ZDCCanvas_Be_Scratch::~ZDCCanvas_Be_Scratch()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Be

ZOSApp_Be* ZOSApp_Be::sOSApp_Be = nil;

ZOSApp_Be* ZOSApp_Be::sGet()
	{ return sOSApp_Be; }


ZOSApp_Be::ZOSApp_Be(const char* inSignature)
	{
	ZAssert(sOSApp_Be == nil);

	ZDCScratch::sSet(new ZDCCanvas_Be_Scratch);

	fMutex_Looper = nil;
	fMessageHelper = nil;

	fOSWindowRoster_Head = nil;

	fIsRunning = false;

	fPendingIdle = false;

	fPendingWindowRosterChange = false;

	fSemaphore_IdleExited = nil;

	if (inSignature == nil)
		inSignature = "application/x-vnd.zlib.unknown";

	status_t theError;
	fRealApp = new RealApp(inSignature, &theError, this);

	fMutex_Looper = new ZMutex_BLooper(fRealApp);
	fMessageHelper = new ZBMessageHelper;

	sOSApp_Be = this;

// Fire off our idle thread, which will tickle all windows and the app itself
	(new ZThreadSimple<ZOSApp_Be*>(sIdleThread, this))->Start();
	}

ZOSApp_Be::~ZOSApp_Be()
	{
	ZAssertStop(1, this->GetLock().IsLocked());
	ZAssert(sOSApp_Be == this);

// Tell our idle thread to exit, and wait for it to do so.
	ZSemaphore idleExited;
	fSemaphore_IdleExited = &idleExited;
	idleExited.Wait(1);

// We're being destroyed, kill all remaining windows, with extreme prejudice.
	ZMutexLocker structureLocker(fMutex_Structure);
	ZMessage theDieMessage;
	theDieMessage.SetString("what", "zoolib:DieDieDie");

	while (fVector_Windows.size() != 0)
		{
		fVector_Windows[0]->QueueMessage(theDieMessage);
		fCondition_Structure.Wait(fMutex_Structure);
		}

	while (fOSWindowRoster_Head)
		{
		fOSWindowRoster_Head->fOSApp = nil;
		fOSWindowRoster_Head = fOSWindowRoster_Head->fNext;
		}

	structureLocker.Release();

	ZDCScratch::sSet(ZRef<ZDCCanvas>());

	sOSApp_Be = nil;

	fRealApp->OSAppDisposed(this);

	delete fMutex_Looper;
	fMutex_Looper = nil;
	structureLocker.Acquire();
	delete fMessageHelper;
	fMessageHelper = nil;
	}

// From ZOSApp
ZMutexBase& ZOSApp_Be::GetLock()
	{ return *fMutex_Looper; }

void ZOSApp_Be::QueueMessage(const ZMessage& inMessage)
	{
	if (fMutex_Structure.Acquire() != ZThread::errorNone)
		return;

	int32 theID = fMessageHelper->QueueZMessage(inMessage);
	BMessage theBMessage('ZLib');
	theBMessage.AddString("Type", "ZMessage");
	theBMessage.AddInt32("ID", theID);
	fRealApp->PostMessage(&theBMessage);

	fMutex_Structure.Release();
	}

bool ZOSApp_Be::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSApp::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Idle")
			{
			fMutex_Structure.Acquire();
			fPendingIdle = false;
			fMutex_Structure.Release();
			this->DispatchMessageToOwner(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:WindowRosterChange")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingWindowRosterChange)
				{
				fPendingWindowRosterChange = false;
				if (fIsRunning)
					{
					locker.Release();
					this->DispatchMessageToOwner(inMessage);
					}
				}
			return true;
			}

		}
	return false;
	}

void ZOSApp_Be::Run()
	{
	fIsRunning = true;
	fRealApp->Run();
	fIsRunning = false;
// After returning from Run, we're unlocked. To preserve the semantics we
// supported everywhere else, we acquire our lock.
	fRealApp->Lock();
	}

bool ZOSApp_Be::IsRunning()
	{
	return fIsRunning;
	}

void ZOSApp_Be::ExitRun()
	{
	ZAssertStop(1, this->GetLock().IsLocked());
	fRealApp->Quit();
	}

string ZOSApp_Be::GetAppName()
	{
	return "Dummy";
	}

bool ZOSApp_Be::HasUserAccessibleWindows()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	return ZAtomic_Get(&sWindowsShown) != 0;
	}

bool ZOSApp_Be::HasGlobalMenuBar()
	{ return false; }

void ZOSApp_Be::BroadcastMessageToAllWindows(const ZMessage& inMessage)
	{
	ZAssertStop(1, this->GetLock().IsLocked());
	ZMutexLocker locker(fMutex_Structure);
	for (int32 index = 0; index < fRealApp->CountWindows(); ++index)
		{
		if (ZOSWindow_Be* currentWindow = ZOSWindow_Be::sFromBWindow(fRealApp->WindowAt(index)))
			currentWindow->QueueMessage(inMessage);
		}
	this->QueueMessage(inMessage);
	}

ZOSWindow* ZOSApp_Be::CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes)
	{
	ZMutexLocker structureLocker(fMutex_Structure, false);
	if (!structureLocker.IsOkay())
		return nil;
	return new ZOSWindow_Be(inAttributes);
	}

ZOSWindowRoster* ZOSApp_Be::CreateOSWindowRoster()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	return new ZOSWindowRoster_Be(this);
	}

void ZOSApp_Be::AddOSWindow(ZOSWindow_Be* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	fVector_Windows.push_back(inOSWindow);
	fCondition_Structure.Broadcast();
	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_Be::RemoveOSWindow(ZOSWindow_Be* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	ZOSWindowRoster_Be* theRoster = fOSWindowRoster_Head;
	while (theRoster)
		{
		theRoster->OSWindowRemoved(inOSWindow);
		theRoster = theRoster->fNext;
		}

	fVector_Windows.erase(find(fVector_Windows.begin(), fVector_Windows.end(), inOSWindow));

	fCondition_Structure.Broadcast();

	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_Be::Internal_RecordIdle()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingIdle)
		{
		fPendingIdle = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Idle");
		this->QueueMessage(theMessage);
		}
	}

void ZOSApp_Be::Internal_RecordWindowRosterChange()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingWindowRosterChange && fIsRunning)
		{
		fPendingWindowRosterChange = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:WindowRosterChange");
		this->QueueMessage(theMessage);
		}
	}
// ==================================================

void ZOSApp_Be::BApp_DispatchMessage(BMessage* inMessage, BHandler* inTarget)
	{
	switch (inMessage->what)
		{
		case 'ZLib':
			{
			const char* theType;
			if (inMessage->FindString("Type", &theType) == B_OK)
				{
				if (::strcmp(theType, "ZMessage") == 0)
					{
					int32 theID;
					if (inMessage->FindInt32("ID", &theID) == B_OK)
						{
						fMutex_Structure.Acquire();
						ZMessage theZMessage;
						fMessageHelper->GetZMessageFromID(theID, theZMessage);
						fMutex_Structure.Release();
						::sDispatchMessageNoThrow(this, theZMessage);
						}
					}
				}
			break;
			}
		case B_READY_TO_RUN:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RunStarted");
			::sDispatchMessageToOwnerNoThrow(this, theMessage);
			break;
			}
		case B_QUIT_REQUESTED:
			{
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:RequestQuit");
			::sDispatchMessageToOwnerNoThrow(this, theMessage);
			break;
			}
		default:
			{
			fRealApp->BApplication::DispatchMessage(inMessage, inTarget);
			break;
			}
		}
	}

// ==================================================

void ZOSApp_Be::IdleThread()
	{
	while (!fSemaphore_IdleExited)
		{
		ZMutexLocker structureLocker(fMutex_Structure);
		for (size_t x = 0; x < fVector_Windows.size(); ++x)
			fVector_Windows[x]->Internal_RecordIdle();

		this->Internal_RecordIdle();

		structureLocker.Release();

		ZThread::sSleep(50);
		}
	fSemaphore_IdleExited->Signal(1);
	}

void ZOSApp_Be::sIdleThread(ZOSApp_Be* inOSApp)
	{ inOSApp->IdleThread(); }

#endif // ZCONFIG(API_OSWindow, Be)
