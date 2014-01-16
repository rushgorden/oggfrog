static const char rcsid[] = "@(#) $Id: ZOSWindow_X.cpp,v 1.20 2006/07/17 18:32:41 agreen Exp $";

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

#include "ZOSWindow_X.h"

#if ZCONFIG(API_OSWindow, X)

#include "ZDC_X.h"
#include "ZThreadSimple.h"
#include "ZTicks.h"
#include "ZUtil_STL.h"
#include "ZXLib.h"

#include <sys/time.h> // for timeval

#include <X11/cursorfont.h>

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// =================================================================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_X 1

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_X_Window

class ZDCCanvas_X_Window : public ZDCCanvas_X
	{
public:
	ZDCCanvas_X_Window(ZOSWindow_X* inOSWindow, ZRef<ZXServer> inXServer, Drawable inDrawable, ZMutex* inMutexToCheck, ZMutex* inMutexToLock);
	virtual ~ZDCCanvas_X_Window();

// From ZRefCountedWithFinalization via ZDCCanvas_X
	virtual void Finalize();

// From ZDCCanvas via ZDCCanvas_X
	virtual void Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta);
	virtual void CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect);

// From ZDCCanvas_X
	virtual ZDCRgn Internal_CalcClipRgn(const ZDCState& inState);

// Our protocol
	void InvalClip();
	void OSWindowDisposed();

protected:
	ZOSWindow_X* fOSWindow;
	};

ZDCCanvas_X_Window::ZDCCanvas_X_Window(ZOSWindow_X* inOSWindow, ZRef<ZXServer> inXServer, Drawable inDrawable, ZMutex* inMutexToCheck, ZMutex* inMutexToLock)
	{
	fOSWindow = inOSWindow;
	fXServer = inXServer;
	fDrawable = inDrawable;
	fMutexToCheck = inMutexToCheck;
	fMutexToLock = inMutexToLock;

	XGCValues values;
	values.graphics_exposures = 1;
	fGC = fXServer->CreateGC(fDrawable, GCGraphicsExposures, &values);
	}

ZDCCanvas_X_Window::~ZDCCanvas_X_Window()
	{
	if (fGC)
		{
		fXServer->FreeGC(fGC);
		fGC = nil;
		fDrawable = 0;
		fMutexToCheck = nil;
		fMutexToLock = nil;
		}
	}

void ZDCCanvas_X_Window::Finalize()
	{
	// If we're being finalized, then our window must be disposed already and will have
	// called our OSWindowDisposed, which will have set fOSWindow to nil.
	ZAssertStop(kDebug_X, fOSWindow == nil);
	this->FinalizationComplete();
	delete this;
	}

void ZDCCanvas_X_Window::Scroll(ZDCState& ioState, const ZRect& inRect, ZCoord hDelta, ZCoord vDelta)
	{
	if (!fDrawable)
		return;

	SetupLock theSetupLock(this);

	ZPoint offset(hDelta, vDelta);

	ZDCRgn excludeRgn = fOSWindow->Internal_GetExcludeRgn();

	// destRgn is the pixels we want and are able to draw to.
	ZDCRgn destRgn = ((ioState.fClip + ioState.fClipOrigin) & (inRect + ioState.fOrigin)) - excludeRgn;

	// srcRgn is the set of pixels we're want and are able to copy from.
	ZDCRgn srcRgn = ((destRgn - offset) & (inRect + ioState.fOrigin)) - excludeRgn;

	// drawnRgn is the destination pixels that will be drawn by the CopyBits call, it's the srcRgn
	// shifted back to the destination.
	ZDCRgn drawnRgn = srcRgn + offset;

	// invalidRgn is the destination pixels we wanted to draw but could not because they were
	// outside the visRgn, or were in the excludeRgn
	ZDCRgn invalidRgn = destRgn - drawnRgn;

	// And set the clip (drawnRgn)
	fXServer->SetRegion(fGC, drawnRgn.GetRegion());
	++fChangeCount_Clip;

	fXServer->SetFunction(fGC, GXcopy);
	++fChangeCount_Mode;

	ZRect drawnBounds = drawnRgn.Bounds();

	fXServer->CopyArea(fDrawable, fDrawable, fGC,
				drawnBounds.Left() - offset.h, drawnBounds.Top() - offset.v,
				drawnBounds.Width(), drawnBounds.Height(),
				drawnBounds.Left(), drawnBounds.Top());

	// Finally, let our window know what additional pixels are invalid
	fOSWindow->Internal_RecordInvalidation(invalidRgn);
	}

void ZDCCanvas_X_Window::CopyFrom(ZDCState& ioState, const ZPoint& inDestLocation, ZRef<ZDCCanvas> inSourceCanvas, const ZDCState& inSourceState, const ZRect& inSourceRect)
	{
	ZRef<ZDCCanvas_X> sourceCanvasX = ZRefDynamicCast<ZDCCanvas_X>(inSourceCanvas);
	ZAssertStop(kDebug_X, sourceCanvasX != nil);

	if (!fDrawable || !sourceCanvasX->Internal_GetDrawable())
		return;

	SetupLock theSetupLock(this);

	// We can (currently) only copy from one drawable to another if they're on the same display
	//ZAssertStop(kDebug_X, fXServer == sourceCanvasX->fXServer);

	ZRect destRect = inSourceRect + (ioState.fOrigin + inDestLocation - inSourceRect.TopLeft());
	ZRect sourceRect = inSourceRect + inSourceState.fOrigin;

	if (sourceCanvasX == this)
		{
		// Get the region that we know cannot be copied from, because it's marked as invalid or is
		// part of the window's pseudo-structure (size box).
		ZDCRgn excludeRgn = fOSWindow->Internal_GetExcludeRgn();

		// destRgn is the set of pixels we want and are allowed to draw to: the intersection of the
		// current clip and the destRect less the exclude region.
		ZDCRgn destRgn = ((ioState.fClip + ioState.fClipOrigin) & destRect) - excludeRgn;

		// sourceRgn is destRgn shifted back to the source location, with the exclude region removed (again).
		ZDCRgn sourceRgn = (destRgn + (sourceRect.TopLeft() - destRect.TopLeft())) - excludeRgn;

		// drawnRgn is the subset of destRgn that has accessible corresponding pixels in the source.
		ZDCRgn drawnRgn = destRgn & (sourceRgn + (destRect.TopLeft() - sourceRect.TopLeft()));

		// invalidRgn is therefore the subset of destRgn that does not have accessible pixels in the source.
		ZDCRgn invalidRgn = destRgn - drawnRgn;

		fXServer->SetRegion(fGC, drawnRgn.GetRegion());
		++fChangeCount_Clip;

		fXServer->SetFunction(fGC, GXcopy);
		++fChangeCount_Mode;

		fXServer->CopyArea(fDrawable, fDrawable, fGC,
					sourceRect.Left(), sourceRect.Top(),
					sourceRect.Width(), sourceRect.Height(),
					destRect.Left(), destRect.Top());
		fXServer->Sync(false);

		// Finally, let our window know what additional pixels are invalid
		fOSWindow->Internal_RecordInvalidation(invalidRgn);
		}
	else
		{
		ZDCRgn realClip = this->Internal_CalcClipRgn(ioState);

		fXServer->SetRegion(fGC, realClip.GetRegion());
		++fChangeCount_Clip;

		fXServer->SetFunction(fGC, GXcopy);
		++fChangeCount_Mode;

		fXServer->CopyArea(sourceCanvasX->Internal_GetDrawable(), fDrawable, fGC,
					sourceRect.Left(), sourceRect.Top(),
					sourceRect.Width(), sourceRect.Height(),
					destRect.Left(), destRect.Top());
		}
	}

ZDCRgn ZDCCanvas_X_Window::Internal_CalcClipRgn(const ZDCState& inState)
	{
	return (inState.fClip + inState.fClipOrigin) - fOSWindow->Internal_GetExcludeRgn();
	}

// Our protocol
void ZDCCanvas_X_Window::InvalClip()
	{
	ZAssertStop(kDebug_X, fMutexToLock->IsLocked());
	++fChangeCount_Clip;
	}

void ZDCCanvas_X_Window::OSWindowDisposed()
	{
	fXServer->FreeGC(fGC);
	fGC = nil;
	fDrawable = 0;
	fOSWindow = nil;
	fMutexToCheck = nil;
	fMutexToLock = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_X

ZOSWindow_X::ZOSWindow_X(ZOSApp_X* inApp, ZRef<ZXServer> inXServer, const ZOSWindow::CreationAttributes& inAttributes)
	{
	fShown = false;
	fApp = inApp;
	fXCursor = None;
	fCaptured = false;

	Display* theDisplay = inXServer->GetDisplay();
	int theScreenNum = DefaultScreen(theDisplay);

	XSetWindowAttributes theXSWA;
	theXSWA.background_pixmap = None;
	theXSWA.background_pixel = WhitePixel(theDisplay, theScreenNum);
	theXSWA.border_pixmap = None;
	theXSWA.border_pixel = BlackPixel(theDisplay, theScreenNum);
	theXSWA.bit_gravity = NorthWestGravity;
	theXSWA.win_gravity = NorthWestGravity;
	theXSWA.backing_store = NotUseful;
	theXSWA.backing_planes = 0;
	theXSWA.backing_pixel = 0;
	theXSWA.save_under = false;
	theXSWA.event_mask = KeyPressMask | KeyReleaseMask
						| ButtonPressMask | ButtonReleaseMask
						| EnterWindowMask | LeaveWindowMask | PointerMotionMask
//						| EnterWindowMask | LeaveWindowMask | PointerMotionMask | PointerMotionHintMask
						| Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask
						| ButtonMotionMask
						| KeymapStateMask
						| ExposureMask | VisibilityChangeMask
						| StructureNotifyMask 
//						| ResizeRedirectMask | SubstructureNotifyMask | SubstructureRedirectMask
						| FocusChangeMask | PropertyChangeMask
						| ColormapChangeMask
						| OwnerGrabButtonMask;
	theXSWA.do_not_propagate_mask = 0;
	theXSWA.override_redirect = false;
	theXSWA.colormap = None;
//	theXSWA.colormap = DefaultColormap(theDisplay, theScreenNum);
	theXSWA.cursor = None;
	unsigned long theValueMask = CWBorderPixmap | CWBorderPixel
						| CWBitGravity | CWWinGravity | CWBackingStore | CWBackingPlanes | CWBackingPixel
						| CWOverrideRedirect | CWSaveUnder | CWEventMask | CWDontPropagate | CWColormap | CWCursor;
//	unsigned long theValueMask = CWBorderPixmap | CWBorderPixel
//						| CWBitGravity | CWWinGravity | CWBackingStore | CWBackingPlanes | CWBackingPixel
//						| CWOverrideRedirect | CWSaveUnder | CWEventMask | CWDontPropagate | CWCursor;
// Although we set the background_pixmap and background_pixel fields of theXSWA, we're not passing CWBackPixmap | CWBackPixel in theValueMask,
// so the window will not have its background painted when the window is resized, and we get reduced flicker. At some point it might be possible to
// set up protocol whereby a window's user can say that she wants a constant background color (which we used to have), but for now all windows
// are responsible for erasing their background, and thus all windows can have funky backgrounds without fighting the OS (except on MacOS).

	Window theWindow = inXServer->CreateWindow(RootWindow(theDisplay, theScreenNum),
					inAttributes.fFrame.left, inAttributes.fFrame.top, inAttributes.fFrame.Width(), inAttributes.fFrame.Height(), 0,
					DisplayPlanes(theDisplay, theScreenNum), InputOutput, DefaultVisual(theDisplay, theScreenNum), theValueMask, &theXSWA);

	this->AttachToWindow(inXServer, theWindow);

	XSizeHints* theSizeHints = ZXLib::AllocSizeHints();
	theSizeHints->x = inAttributes.fFrame.Left();
	theSizeHints->y = inAttributes.fFrame.Top();
	theSizeHints->width = inAttributes.fFrame.Width();
	theSizeHints->height = inAttributes.fFrame.Height();
	if (inAttributes.fResizable)
		{
		theSizeHints->min_width = 20;
		theSizeHints->min_height = 20;
		theSizeHints->max_width = 2000;
		theSizeHints->max_height = 2000;
		}
	else
		{
		theSizeHints->min_width = inAttributes.fFrame.Width();
		theSizeHints->min_height = inAttributes.fFrame.Height();
		theSizeHints->max_width = inAttributes.fFrame.Width();
		theSizeHints->max_height = inAttributes.fFrame.Height();
		}
	theSizeHints->base_width = 10;
	theSizeHints->base_height = 10;
	theSizeHints->width_inc = 1;
	theSizeHints->height_inc = 1;
	theSizeHints->win_gravity = NorthWestGravity;
	theSizeHints->flags = PPosition | PSize | PMinSize | PMaxSize | PResizeInc | PBaseSize | PWinGravity;

	XWMHints* theWMHints = ZXLib::AllocWMHints();
	theWMHints->input = true;
	theWMHints->initial_state = NormalState;
	theWMHints->flags = InputHint | StateHint;

	XClassHint* theClassHint = ZXLib::AllocClassHint();

	XTextProperty theWindowName;
	const char* dummyName = "untitled zoolib window";
	ZXLib::StringListToTextProperty(&dummyName, 1, &theWindowName);

	fXServer->SetWMProperties(fWindow, &theWindowName, &theWindowName, nil, 0, theSizeHints, theWMHints, theClassHint);
	ZXLib::Free(theClassHint);
	ZXLib::Free(theWMHints);
	ZXLib::Free(theSizeHints);

	vector<Atom> theProtocolAtoms;
	theProtocolAtoms.push_back(fXServer->InternAtom("WM_DELETE_WINDOW"));
//##	theProtocolAtoms.push_back(fXServer->InternAtom("WM_TAKE_FOCUS"));
	fXServer->SetWMProtocols(fWindow, &theProtocolAtoms[0], theProtocolAtoms.size());

	fCanvas = new ZDCCanvas_X_Window(this, fXServer, fWindow, &fMutex_MessageDispatch, &fMutex_Structure);

	this->Internal_RecordFrameChange(inAttributes.fFrame);
	this->Internal_ReportFrameChange();

	this->Internal_RecordActiveChange(true);

	fApp->AddOSWindow(this);
	}

ZOSWindow_X::~ZOSWindow_X()
	{
	ASSERTLOCKED();

	fCanvas->OSWindowDisposed();
	fXServer->DestroyWindow(fWindow);
	if (fXCursor != None)
		fXServer->FreeCursor(fXCursor);
	this->DetachFromWindow();
	fApp->RemoveOSWindow(this);
	}

void ZOSWindow_X::SetShownState(ZShownState inState)
	{
	ASSERTLOCKED();
	bool isShown = (inState != eZShownStateHidden);

	if (fShown == isShown)
		return;
	fShown = isShown;
	this->Internal_RecordShownStateChange(inState);
	if (fShown)
		{
		fXServer->MapWindow(fWindow);
		}
	else
		{
		fXServer->UnmapWindow(fWindow);
		Display* theDisplay = fXServer->GetDisplay();
		int theScreenNum = DefaultScreen(theDisplay);
		XEvent theXEvent;
		theXEvent.xunmap.type = UnmapNotify;
		theXEvent.xunmap.serial = 0;
		theXEvent.xunmap.send_event = 0;
		theXEvent.xunmap.display = nil;
		theXEvent.xunmap.event = RootWindow(theDisplay, theScreenNum);
		theXEvent.xunmap.window = fWindow;
		theXEvent.xunmap.from_configure = false;
		fXServer->SendEvent(RootWindow(theDisplay, theScreenNum), false, SubstructureRedirectMask | SubstructureNotifyMask, theXEvent);
		fXServer->Flush();
		}
	this->Internal_ReportShownStateChange();
	fApp->Internal_RecordWindowRosterChange();
	}

void ZOSWindow_X::SetTitle(const string& inTitle)
	{
	ASSERTLOCKED();
	if (fTitle == inTitle)
		return;
	fTitle = inTitle;

	const char* theString = fTitle.size() ? inTitle.c_str() : "";
	XTextProperty theWindowName;
	ZXLib::StringListToTextProperty(&theString, 1, &theWindowName);

	fXServer->SetWMName(fWindow, &theWindowName);
	fXServer->SetWMIconName(fWindow, &theWindowName);
	}

string ZOSWindow_X::GetTitle()
	{
	ASSERTLOCKED();
	return fTitle;
	}

void ZOSWindow_X::SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap)
	{
	ASSERTLOCKED();
	}

ZCoord ZOSWindow_X::GetTitleIconPreferredHeight()
	{
	ASSERTLOCKED();
	return 0;
	}

void ZOSWindow_X::SetCursor(const ZCursor& inCursor)
	{
	ASSERTLOCKED();
	if (inCursor.IsCustom())
		{
		Cursor newXCursor = None;
		ZDCPixmap pixmapColor, pixmapMono, pixmapMask;
		ZPoint hotPoint;
		if (inCursor.Get(pixmapColor, pixmapMono, pixmapMask, hotPoint))
			{
			if (!pixmapMono)
				pixmapMono = pixmapColor;
			ZAssertStop(kDebug_X, pixmapMono && pixmapMask);
			newXCursor = fXServer->CreateCursorFromDCPixmaps(fWindow, pixmapMono, pixmapMask, hotPoint);
			}
		fXServer->DefineCursor(fWindow, newXCursor);
		if (fXCursor != None)
			fXServer->FreeCursor(fXCursor);
		fXCursor = newXCursor;
		}
	else
		{
		int theCursorShape = -1;
		switch (inCursor.GetCursorType())
			{
			case ZCursor::eCursorTypeIBeam: theCursorShape = XC_xterm; break;
			case ZCursor::eCursorTypeCross: theCursorShape = XC_crosshair; break;
			case ZCursor::eCursorTypePlus: theCursorShape = XC_plus; break;
			case ZCursor::eCursorTypeWatch: theCursorShape = XC_watch; break;
			case ZCursor::eCursorTypeArrow:
			default:
				theCursorShape = XC_X_cursor; break;
			}
		Cursor newXCursor = None;
		if (theCursorShape > 0)
			newXCursor = fXServer->CreateFontCursor(theCursorShape);
		fXServer->DefineCursor(fWindow, newXCursor);
		if (fXCursor != None)
			fXServer->FreeCursor(fXCursor);
		fXCursor = newXCursor;
		}
	}

void ZOSWindow_X::SetLocation(ZPoint inLocation)
	{
	ASSERTLOCKED();
	fXServer->MoveWindow(fWindow, inLocation.h, inLocation.v);
	this->Internal_RecordLocationChange(inLocation);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_X::SetSize(ZPoint inSize)
	{
	ASSERTLOCKED();
	fXServer->ResizeWindow(fWindow, inSize.h, inSize.v);
	this->Internal_RecordSizeChange(inSize);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_X::SetFrame(const ZRect& inFrame)
	{
	ASSERTLOCKED();
	fXServer->MoveResizeWindow(fWindow, inFrame.left, inFrame.top, inFrame.Width(), inFrame.Height());
	this->Internal_RecordFrameChange(inFrame);
	this->Internal_ReportFrameChange();
	}

void ZOSWindow_X::SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize)
	{
	ASSERTLOCKED();

	}

void ZOSWindow_X::BringFront(bool inMakeVisible)
	{
	ASSERTLOCKED();
	if (inMakeVisible)
		this->SetShownState(eZShownStateShown);
//	fApp->BringWindowFront(this, inMakeVisible);
//	this->Internal_ReportActiveChange();
	}

void ZOSWindow_X::GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset)
	{
	ASSERTLOCKED();
	outTopLeftInset = ZPoint::sZero;
	outBottomRightInset = ZPoint::sZero;
	}

bool ZOSWindow_X::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();
	return false;
	}

ZDCRgn ZOSWindow_X::GetVisibleContentRgn()
	{
	ASSERTLOCKED();
	return ZDCRgn(this->GetSize());
	}

void ZOSWindow_X::InvalidateRegion(const ZDCRgn& inBadRgn)
	{
	ASSERTLOCKED();
	this->Internal_RecordInvalidation(inBadRgn);
	}

void ZOSWindow_X::UpdateNow()
	{
	ASSERTLOCKED();
	this->Internal_ReportInvalidation();
	}

ZRef<ZDCCanvas> ZOSWindow_X::GetCanvas()
	{
	ASSERTLOCKED();
	return fCanvas;
	}

void ZOSWindow_X::AcquireCapture()
	{
	ASSERTLOCKED();
	fCaptured = true;

	unsigned int theEventMask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask
							| EnterWindowMask | LeaveWindowMask;
	fXServer->GrabPointer(fWindow, false, theEventMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime); 
	}

void ZOSWindow_X::ReleaseCapture()
	{
	ASSERTLOCKED();
	fXServer->UngrabPointer(CurrentTime);

	fCaptured = false;

	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:CaptureReleased");
	this->QueueMessageForOwner(theMessage);
	}

void ZOSWindow_X::SetMenuBar(const ZMenuBar& inMenuBar)
	{
	ASSERTLOCKED();
	}

void ZOSWindow_X::GetNative(void* outNative)
	{
	ASSERTLOCKED();
	*reinterpret_cast<Window*>(outNative) = fWindow;
	}

ZDragInitiator* ZOSWindow_X::CreateDragInitiator()
	{
	return nil;
	}

ZRect ZOSWindow_X::GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly)
	{
	Display* theDisplay = fXServer->GetDisplay();
	int theScreenNum = DefaultScreen(theDisplay);
	ZRect theRect;
	theRect.left = 0;
	theRect.top = 0;
	theRect.right = DisplayWidth(theDisplay, theScreenNum);
	theRect.bottom = DisplayHeight(theDisplay, theScreenNum);
	return theRect;
	}

ZDCRgn ZOSWindow_X::Internal_GetExcludeRgn()
	{
	ZAssertLocked(kDebug_X, fMutex_Structure);
	return this->Internal_GetInvalidRgn();
	}

static void sStuffMouseState(Window inWindow, const XMotionEvent& inEvent, bool iCaptured, ZOSWindow_Std::MouseState& outState)
	{
	ZDebugLogf(kDebug_X, ("XMotionEvent %d %d", inEvent.x, inEvent.y));
	outState.fLocation.h = inEvent.x;
	outState.fLocation.v = inEvent.y;
	outState.fContained = (inEvent.subwindow == inWindow);
	outState.fCaptured = iCaptured;

	outState.fButtons[0] = (inEvent.state & Button1Mask) != 0;
	outState.fButtons[1] = (inEvent.state & Button2Mask) != 0;
	outState.fButtons[2] = (inEvent.state & Button3Mask) != 0;
	outState.fButtons[3] = (inEvent.state & Button4Mask) != 0;
	outState.fButtons[4] = (inEvent.state & Button5Mask) != 0;

	outState.fModifier_Command = false;
	outState.fModifier_Option = false;
	outState.fModifier_Shift = (inEvent.state & ShiftMask) != 0;
	outState.fModifier_Control = (inEvent.state & ControlMask) != 0;
	outState.fModifier_CapsLock = (inEvent.state & LockMask) != 0;
	}

static void sStuffMouseState(const XCrossingEvent& inEvent, bool isContained, bool iCaptured, ZOSWindow_Std::MouseState& outState)
	{
	ZDebugLogf(kDebug_X, ("XCrossingEvent %d %d", inEvent.x, inEvent.y));
	outState.fLocation.h = inEvent.x;
	outState.fLocation.v = inEvent.y;
	outState.fContained = isContained;
	outState.fCaptured = iCaptured;

	outState.fButtons[0] = (inEvent.state & Button1Mask) != 0;
	outState.fButtons[1] = (inEvent.state & Button2Mask) != 0;
	outState.fButtons[2] = (inEvent.state & Button3Mask) != 0;
	outState.fButtons[3] = (inEvent.state & Button4Mask) != 0;
	outState.fButtons[4] = (inEvent.state & Button5Mask) != 0;

	outState.fModifier_Command = false;
	outState.fModifier_Option = false;
	outState.fModifier_Shift = (inEvent.state & ShiftMask) != 0;
	outState.fModifier_Control = (inEvent.state & ControlMask) != 0;
	outState.fModifier_CapsLock = (inEvent.state & LockMask) != 0;
	}

static void sStuffMouseState(Window inWindow, const XButtonEvent& inEvent, bool iCaptured, ZOSWindow_Std::MouseState& outState, int& outPressedButtonsCount)
	{
	ZDebugLogf(kDebug_X, ("XButtonEvent %d %d", inEvent.x, inEvent.y));
	outState.fLocation.h = inEvent.x;
	outState.fLocation.v = inEvent.y;
	outState.fContained = (inEvent.subwindow == inWindow);
	outState.fCaptured = iCaptured;

	outState.fButtons[0] = (inEvent.state & Button1Mask) != 0;
	outState.fButtons[1] = (inEvent.state & Button2Mask) != 0;
	outState.fButtons[2] = (inEvent.state & Button3Mask) != 0;
	outState.fButtons[3] = (inEvent.state & Button4Mask) != 0;
	outState.fButtons[4] = (inEvent.state & Button5Mask) != 0;
	outPressedButtonsCount = 0;
	for (int x = 0; x < 5; ++x)
		{
		if (outState.fButtons[x])
			++outPressedButtonsCount;
		}

	outState.fModifier_Command = false;
	outState.fModifier_Option = false;
	outState.fModifier_Shift = (inEvent.state & ShiftMask) != 0;
	outState.fModifier_Control = (inEvent.state & ControlMask) != 0;
	outState.fModifier_CapsLock = (inEvent.state & LockMask) != 0;
	}

void ZOSWindow_X::HandleXEvent(const XEvent& inEvent)
	{
	switch (inEvent.type)
		{
		case ButtonPress:
			{
			MouseState theState;
			int pressedButtonsCount;
			::sStuffMouseState(fWindow, inEvent.xbutton, fCaptured, theState, pressedButtonsCount);
			// Add in the pressed button to the list that are down
			theState.fButtons[inEvent.xbutton.button - 1] = true;

			// Distinguish initial press from subsequent ones, so we can record this as a mouse
			// down or just a change in the mouse state.
			if (pressedButtonsCount == 0)
				this->Internal_RecordMouseDown(inEvent.xbutton.button - 1, theState, ZTicks::sNow());
			else
				this->Internal_RecordMouseState(theState, ZTicks::sNow());
			break;
			}
		case ButtonRelease:
			{
			MouseState theState;
			int pressedButtonsCount;
			::sStuffMouseState(fWindow, inEvent.xbutton, fCaptured, theState, pressedButtonsCount);

			// Remove the released button from the list that are down
			theState.fButtons[inEvent.xbutton.button - 1] = false;
			if (pressedButtonsCount == 1)
				this->Internal_RecordMouseUp(theState, ZTicks::sNow());
			else
				this->Internal_RecordMouseState(theState, ZTicks::sNow());
			break;
			}
		case MotionNotify:
			{
			MouseState theState;
			::sStuffMouseState(fWindow, inEvent.xmotion, fCaptured, theState);
			this->Internal_RecordMouseState(theState, ZTicks::sNow());
			break;
			}
		case EnterNotify:
			{
			MouseState theState;
			::sStuffMouseState(inEvent.xcrossing, true, fCaptured, theState);
			this->Internal_RecordMouseState(theState, ZTicks::sNow());
			break;
			}
		case LeaveNotify:
			{
			MouseState theState;
			::sStuffMouseState(inEvent.xcrossing, false, fCaptured, theState);
			this->Internal_RecordMouseState(theState, ZTicks::sNow());
			break;
			}
		case FocusIn:
			{
			this->Internal_RecordActiveChange(true);
			break;
			}
		case FocusOut:
			{
			this->Internal_RecordActiveChange(false);
			break;
			}
		case Expose:
		case GraphicsExpose:
			{
			XEvent tempEvent = inEvent;
			while (true)
				{
				ZRect invalRect(tempEvent.xexpose.width, tempEvent.xexpose.height);
				invalRect += ZPoint(tempEvent.xexpose.x, tempEvent.xexpose.y);
				fPartialInvalRgn |= invalRect;
				if (tempEvent.xexpose.count == 0)
					{
					ZDCRgn theRgn = fPartialInvalRgn;
					fPartialInvalRgn.Clear();
					this->Internal_RecordInvalidation(theRgn);
					}
				if (!fXServer->CheckWindowEvent(fWindow, ExposureMask, tempEvent))
					break;

				ZDebugLogf(kDebug_X, ("There was another exposure message in the queue"));
				}
			break;
			}
		case ConfigureNotify:
			{
			this->Internal_RecordSizeChange(ZPoint(inEvent.xconfigure.width, inEvent.xconfigure.height));
			break;
			}
		case ClientMessage:
			{
			bool handledIt = false;
			if (inEvent.xclient.message_type == fXServer->InternAtom("WM_PROTOCOLS"))
				{
				if (inEvent.xclient.data.l[0] == fXServer->InternAtom("WM_DELETE_WINDOW"))
					{
					this->Internal_RecordCloseBoxHit();
					handledIt = true;
					}
				else if (inEvent.xclient.data.l[0] == fXServer->InternAtom("WM_TAKE_FOCUS"))
					{
					}
				else
					{
					}
				}

			if (!handledIt)
				{
				ZDebugLogf(kDebug_X, ("ZOSWindow_X, ClientMessage, message type %s, data[0] atom %s",
						fXServer->GetAtomName(inEvent.xclient.message_type).c_str(),
						fXServer->GetAtomName(inEvent.xclient.data.l[0]).c_str()));
				}
			break;
			}
		}
	}

void ZOSWindow_X::Internal_Tickle()
	{
	if (fCaptured)
		this->Internal_TickleMouse(ZTicks::sNow());
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZDCCanvas_X_Scratch

class ZDCCanvas_X_Scratch : public ZDCCanvas_X_NonWindow
	{
public:
	ZDCCanvas_X_Scratch(ZRef<ZXServer> inXServer);
	virtual ~ZDCCanvas_X_Scratch();
	};

ZDCCanvas_X_Scratch::ZDCCanvas_X_Scratch(ZRef<ZXServer> inXServer)
	{
	fXServer = inXServer;
	}

ZDCCanvas_X_Scratch::~ZDCCanvas_X_Scratch()
	{
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_X

ZOSApp_X* ZOSApp_X::sOSApp_X;

ZOSApp_X* ZOSApp_X::sGet()
	{ return sOSApp_X; }

ZOSApp_X::ZOSApp_X()
	{
	fSemaphore_IdleExited = nil;
	fSemaphore_EventLoopExited = nil;

	ZRef<ZXServer> theXServer;
	try
		{
		theXServer = new ZXServer(nil);
		}
	catch (...)
		{}

	if (!theXServer)
		theXServer = new ZXServer(":0");

	fVector_XServer.push_back(theXServer);

	ZAssert(sOSApp_X == nil);
	sOSApp_X = this;

	// Set the scratch DC
	ZDCScratch::sSet(new ZDCCanvas_X_Scratch(theXServer));

	// Fire off the OS event loop thread
	(new ZThreadSimple<ZOSApp_X*>(sRunEventLoop, this, "ZOSApp_X::sRunEventLoop"))->Start();

	// And the idle thread
	(new ZThreadSimple<ZOSApp_X*>(sIdleThread, this))->Start();
	}

ZOSApp_X::~ZOSApp_X()
	{
	ZAssertLocked(0, fMutex_MessageDispatch);

	ZMutexLocker structureLocker(fMutex_Structure, false);
	ZAssert(structureLocker.IsOkay());
	ZMessage theDieMessage;
	theDieMessage.SetString("what", "zoolib:DieDieDie");

	while (fVector_Windows.size() != 0)
		{
		for (size_t x = 0; x < fVector_Windows.size(); ++x)
			fVector_Windows[x]->QueueMessage(theDieMessage);
		fCondition_Structure.Wait(fMutex_Structure);
		}

	// Tell our idle thread to exit, and wait for it to do so.
	ZSemaphore semaphore_IdleExited;
	fSemaphore_IdleExited = &semaphore_IdleExited;
	semaphore_IdleExited.Wait(1);

	// Remove the scratch ZDC
	ZDCScratch::sSet(ZRef<ZDCCanvas>());

	// Close off the XServer(s)
	while (fVector_XServer.size())
		{
		fVector_XServer[0]->Close();
		fVector_XServer.erase(fVector_XServer.begin());
		}

	// Shut down the event loop by setting fSemaphore_EventLoopExited false and waking the process.
	ZSemaphore semaphore_EventLoopExited;
	fSemaphore_EventLoopExited = &semaphore_EventLoopExited;

	// Release the structure lock, which is also the low level event dispatch lock
	structureLocker.Release();

	// Wait for the event loop thread to exit
	semaphore_EventLoopExited.Wait(1);
	}

void ZOSApp_X::Run()
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:RunStarted");
	this->DispatchMessageToOwner(theMessage);

	ZOSApp_Std::Run();
	}

string ZOSApp_X::GetAppName()
	{
	return "Unknown app name";
	}

bool ZOSApp_X::HasUserAccessibleWindows()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (size_t x = 0; x < fVector_Windows.size(); ++x)
		{
		if (fVector_Windows[x]->Internal_GetShown())
			return true;
		}
	return false;
	}

bool ZOSApp_X::HasGlobalMenuBar()
	{
	return false;
	}

bool ZOSApp_X::Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
												const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
												bigtime_t inTimeout, ZOSWindow_Std*& outWindow)
	{
	outWindow = nil;
	bigtime_t endTime = ZTicks::sNow() + inTimeout;
	for (size_t x = 0; x < fVector_Windows.size(); ++x)
		{
		ZOSWindow_Std* currentWindow = fVector_Windows[x];
		if (!ZUtil_STL::sContains(inVector_VisitedWindows, currentWindow))
			{
			if (!ZUtil_STL::sContains(inVector_DroppedWindows, currentWindow))
				{
				// Not dropped and not visited, so try to lock it and return.
				ZThread::Error err = currentWindow->GetLock().Acquire(endTime - ZTicks::sNow());
				if (err == ZThread::errorTimeout)
					return false;
				if (err == ZThread::errorNone)
					{
					outWindow = currentWindow;
					return true;
					}
				}
			}
		}
	return true;
	}

void ZOSApp_X::BroadcastMessageToAllWindows(const ZMessage& inMessage)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	for (vector<ZOSWindow_X*>::iterator i = fVector_Windows.begin(); i != fVector_Windows.end(); ++i)
		(*i)->QueueMessage(inMessage);
	this->QueueMessage(inMessage);
	}

ZOSWindow* ZOSApp_X::CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes)
	{
	static int count = 0;
	ZRef<ZXServer> theXServer = fVector_XServer[count++ % fVector_XServer.size()];

	return new ZOSWindow_X(this, theXServer, inAttributes);
	}

void ZOSApp_X::AddOSWindow(ZOSWindow_X* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	fVector_Windows.push_back(inOSWindow);
	fCondition_Structure.Broadcast();
	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_X::RemoveOSWindow(ZOSWindow_X* inOSWindow)
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	fVector_Windows.erase(find(fVector_Windows.begin(), fVector_Windows.end(), inOSWindow));

	this->Internal_OSWindowRemoved(inOSWindow);

	fCondition_Structure.Broadcast();

	structureLocker.Release();

	this->Internal_RecordWindowRosterChange();
	}

void ZOSApp_X::RunEventLoop()
	{
	ZMutexLocker structureLocker(fMutex_Structure);

	while (!fSemaphore_EventLoopExited)
		{
		fd_set readSet;
		FD_ZERO(&readSet);
		int maxFD = -1;
		bool hadPending = false;
		for (size_t x = 0; x < fVector_XServer.size(); ++x)
			{
			if (fVector_XServer[x]->Pending())
				{
				hadPending = true;
				XEvent theXEvent;
				fVector_XServer[x]->NextEvent(theXEvent);
				fVector_XServer[x]->DispatchXEvent(theXEvent);
				}
			else
				{
				int theFD = ConnectionNumber(fVector_XServer[x]->GetDisplay());
				if (maxFD < theFD)
					maxFD = theFD;
				FD_SET(theFD, &readSet);
				}
			}
		if (!fSemaphore_EventLoopExited && !hadPending)
			{
			structureLocker.Release();
			if (maxFD >= 0)
				{
				struct timeval timeOut;
				timeOut.tv_sec = 1;
				timeOut.tv_usec = 0;
				int result = ::select(maxFD + 1, &readSet, nil, nil, &timeOut);
				}
			structureLocker.Acquire();
			}
		}
	fSemaphore_EventLoopExited->Signal(1);
	}

void ZOSApp_X::sRunEventLoop(ZOSApp_X* inOSApp)
	{ inOSApp->RunEventLoop(); }

void ZOSApp_X::IdleThread()
	{
	while (!fSemaphore_IdleExited)
		{
		ZMutexLocker structureLocker(fMutex_Structure);
		for (size_t x = 0; x < fVector_Windows.size(); ++x)
			{
			fVector_Windows[x]->Internal_Tickle();
			fVector_Windows[x]->Internal_RecordIdle();
			}

		this->Internal_RecordIdle();

		structureLocker.Release();

		ZThread::sSleep(50);
		}
	fSemaphore_IdleExited->Signal(1);
	}

void ZOSApp_X::sIdleThread(ZOSApp_X* inOSApp)
	{ inOSApp->IdleThread(); }

#endif // ZCONFIG(API_OSWindow, X)
