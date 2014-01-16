static const char rcsid[] = "@(#) $Id: ZOSWindow.cpp,v 1.7 2006/04/14 21:02:35 agreen Exp $";

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

#include "ZOSWindow.h"
#include "ZDCPixmap.h"

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow::CreationAttributes

// AG 2000-03-22. These constructors are here (1) to ensure that new fields that are added are
// set to sensible initial values (for code that doesn't get updated to explicitly set those
// values) and (2) to make it easy to create a document window with appropriate frame.

ZOSWindow::CreationAttributes::CreationAttributes()
	{
	fFrame.left = 100;
	fFrame.top = 100;
	fFrame.right = 200;
	fFrame.bottom = 200;
	fLook = lookDocument;
	fLayer = layerDocument;
	fResizable = true;
	fHasSizeBox = true;
	fHasCloseBox = true;
	fHasZoomBox = true;
	fHasMenuBar = true;
	fHasTitleIcon = false;
	}

ZOSWindow::CreationAttributes::CreationAttributes(const ZRect& inFrame)
:	fFrame(inFrame)
	{
	fLook = lookDocument;
	fLayer = layerDocument;
	fResizable = true;
	fHasSizeBox = true;
	fHasCloseBox = true;
	fHasZoomBox = true;
	fHasMenuBar = true;
	fHasTitleIcon = false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow

ZOSWindow::ZOSWindow()
:	fOwner(nil)
	{}

ZOSWindow::~ZOSWindow()
	{
	// The only legitimate way to delete a ZOSWindow is to delete fOwner, which will call
	// our OwnerDisposed method, which nils out fOwner and calls delete this
	ZAssert(fOwner == nil);
	}

void ZOSWindow::SetOwner(ZOSWindowOwner* inOwner)
	{
	ZAssert(fOwner == nil);
	fOwner = inOwner;
	}

void ZOSWindow::OwnerDisposed(ZOSWindowOwner* inOwner)
	{
	ZAssert(fOwner == inOwner);
	fOwner = nil;
	delete this;
	}

void ZOSWindow::QueueMessageForOwner(const ZMessage& inMessage)
	{
	ZMessage envelope;
	envelope.SetString("what", "zoolib:Owner");
	envelope.SetTuple("message", inMessage.AsTuple());
	this->QueueMessage(envelope);
	}

bool ZOSWindow::DispatchMessage(const ZMessage& inMessage)
	{
// Uncomment this to get a stream of pretty much every message sent to your window
//		if (inMessage.GetString("what") != "zoolib:Idle")
//			{
//			string messageString = inMessage.AsString();
//			ZDebugLogf(0, ("%08X %s", this, messageString.c_str()));
//			}

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:DieDieDie")
			{
			if (fOwner)
				fOwner->OSWindowMustDispose(this);
			else
				delete this;
			return true;
			}
		else if (theWhat == "zoolib:Owner")
			{
			this->DispatchMessageToOwner(ZMessage(inMessage.GetTuple("message")));
			return true;
			}
		}
	return false;
	}

void ZOSWindow::DispatchMessageToOwner(const ZMessage& iMessage)
	{
	if (fOwner)
		fOwner->OwnerDispatchMessage(iMessage);
	}

void ZOSWindow::SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap,
	const ZDCPixmap& inMaskPixmap)
	{}

ZCoord ZOSWindow::GetTitleIconPreferredHeight()
	{ return 0; }

ZRect ZOSWindow::GetFrame()
	{
	ZRect myFrame = this->GetSize();
	return myFrame + this->GetLocation();
	}

ZRect ZOSWindow::GetFrameNonZoomed()
	{ return this->GetFrame(); }

static void sCenter(ZOSWindow* inOSWindow, bool inHorizontal, bool inVertical,
	bool inForDialog, long inScreenIndex)
	{
	// Get the insets needed to find the structure region
	ZPoint topLeftInset, bottomRightInset;
	inOSWindow->GetContentInsets(topLeftInset, bottomRightInset);

	// Figure out how big the window is including the structure region
	ZPoint windowSize = inOSWindow->GetSize();

	windowSize += topLeftInset;
	windowSize -= bottomRightInset;

	// Find which screen we're using
	ZRect screenRect;
	if (inScreenIndex == -1)
		screenRect = inOSWindow->GetMainScreenFrame(true);
	else
		screenRect = inOSWindow->GetScreenFrame(inScreenIndex, true);

	ZPoint location(ZPoint::sZero);
	if (inHorizontal)
		location.h = (screenRect.Width() - windowSize.h)/2;
	if (inVertical)
		{
		// For a dialog, we have 2/3 of the space below the window and 1/3 above it.
		// For normal centering it's half and half
		if (inForDialog)
			location.v = (screenRect.Height() - windowSize.v)/3;
		else
			location.v = (screenRect.Height() - windowSize.v)/2;
		}
	// Put back the menu bar height
	location.v += screenRect.top + topLeftInset.v;
	location.h += topLeftInset.h;
	inOSWindow->SetLocation(location);
	}

void ZOSWindow::Center()
	{
	ASSERTLOCKED();
	::sCenter(this, true, true, false, -1);
	}

void ZOSWindow::CenterDialog()
	{
	ASSERTLOCKED();
	::sCenter(this, true, true, true, -1);
	}

void ZOSWindow::Zoom()
	{
	ASSERTLOCKED();
	}

static ZRect sMoveFrameMinimal(const ZRect& inFrameToMove,
	const ZRect& inFrameOther, ZPoint inOverlap)
	{
	ZRect newFrame = inFrameToMove;
	if (inOverlap.h)
		{
		if (newFrame.right < inFrameOther.left + inOverlap.h)
			newFrame += ZPoint(inFrameOther.left + inOverlap.h - newFrame.right, 0);
		if (newFrame.left > inFrameOther.right - inOverlap.h)
			newFrame += ZPoint(inFrameOther.right - inOverlap.h - newFrame.left, 0);
		}
	else
		{
		if (newFrame.left < inFrameOther.left)
			newFrame += ZPoint(inFrameOther.left - newFrame.left, 0);
		if (newFrame.right > inFrameOther.right)
			newFrame += ZPoint(inFrameOther.right - newFrame.right, 0);
		}

	if (inOverlap.v)
		{
		if (newFrame.bottom < inFrameOther.top + inOverlap.v)
			newFrame += ZPoint(0, inFrameOther.top + inOverlap.v - newFrame.bottom);
		if (newFrame.top > inFrameOther.bottom - inOverlap.v)
			newFrame += ZPoint(0, inFrameOther.bottom - inOverlap.v - newFrame.top);
		}
	else
		{
		if (newFrame.top < inFrameOther.top)
			newFrame += ZPoint(0, inFrameOther.top - newFrame.top);
		if (newFrame.bottom > inFrameOther.bottom)
			newFrame += ZPoint(0, inFrameOther.bottom - newFrame.bottom);
		}
	return newFrame;
	}

void ZOSWindow::ForceOnScreen(bool inEnsureTitleBarAccessible, bool inEnsureSizeBoxAccessible)
	{
	ASSERTLOCKED();

	ZRect oldContentFrame = this->GetFrame();

	ZPoint topLeftInset, bottomRightInset;
	this->GetContentInsets(topLeftInset, bottomRightInset);

	ZRect newStructureFrame;
	newStructureFrame.left = oldContentFrame.left - topLeftInset.h;
	newStructureFrame.top = oldContentFrame.top - topLeftInset.v;
	newStructureFrame.right = oldContentFrame.right - bottomRightInset.h;
	newStructureFrame.bottom = oldContentFrame.bottom - bottomRightInset.v;

	// Find the screen containing the largest area of window, or the main screen if none
	int32 screenIndex = this->GetScreenIndex(newStructureFrame, true);
	ZRect screenFrame = this->GetScreenFrame(screenIndex, true);

	ZDCRgn desktopRgn = this->GetDesktopRegion(true);

	if (inEnsureTitleBarAccessible)
		{
		ZRect titleFrame = newStructureFrame;
		titleFrame.bottom = titleFrame.top + 15;
		if ((desktopRgn & titleFrame).IsEmpty())
			{
			ZRect newTitleFrame = sMoveFrameMinimal(titleFrame, screenFrame, ZPoint(15, 15));
			newStructureFrame += newTitleFrame.TopLeft() - titleFrame.TopLeft();
			}
		}

	if (inEnsureSizeBoxAccessible)
		{
		ZRect sizeBoxFrame = newStructureFrame;
		sizeBoxFrame.left = sizeBoxFrame.right - 15;
		sizeBoxFrame.top = sizeBoxFrame.bottom - 15;
		if ((desktopRgn & sizeBoxFrame).IsEmpty())
			{
			ZRect newSizeBoxFrame = sMoveFrameMinimal(sizeBoxFrame, screenFrame, ZPoint(15, 15));
			newStructureFrame += newSizeBoxFrame.TopLeft() - sizeBoxFrame.TopLeft();
			}
		}

	if (inEnsureTitleBarAccessible)
		{
		ZRect titleFrame = newStructureFrame;
		titleFrame.bottom = titleFrame.top + 15;
		if ((desktopRgn & titleFrame).IsEmpty())
			{
			ZRect newTitleFrame = sMoveFrameMinimal(titleFrame, screenFrame, ZPoint(15, 15));
			newStructureFrame.left += newTitleFrame.left - titleFrame.left;
			newStructureFrame.top += newTitleFrame.top - titleFrame.top;
			}
		}

	if (inEnsureSizeBoxAccessible)
		{
		ZRect sizeBoxFrame = newStructureFrame;
		sizeBoxFrame.left = sizeBoxFrame.right - 15;
		sizeBoxFrame.top = sizeBoxFrame.bottom - 15;
		if ((desktopRgn & sizeBoxFrame).IsEmpty())
			{
			ZRect newSizeBoxFrame = sMoveFrameMinimal(sizeBoxFrame, screenFrame, ZPoint(15, 15));
			newStructureFrame.right += newSizeBoxFrame.right - sizeBoxFrame.right;
			newStructureFrame.bottom += newSizeBoxFrame.bottom - sizeBoxFrame.bottom;
			}
		}

	if ((desktopRgn & newStructureFrame).IsEmpty())
		newStructureFrame = sMoveFrameMinimal(newStructureFrame, screenFrame, ZPoint(15, 15));

	ZRect newContentFrame;
	newContentFrame.left = newStructureFrame.left + topLeftInset.h;
	newContentFrame.top = newStructureFrame.top + topLeftInset.v;
	newContentFrame.right = newStructureFrame.right + bottomRightInset.h;
	newContentFrame.bottom = newStructureFrame.bottom + bottomRightInset.v;

	if (newContentFrame != oldContentFrame)
		this->SetFrame(newContentFrame);
	}

ZPoint ZOSWindow::GetContentTopLeftInset()
	{
	// Used as a shorthand to get the distance from the top left of the OSWindow's
	// content up and over to the OSWindow's structure top left. Useful when
	// initially positioning a window.
	ZPoint topLeftInset, bottomRightInset;
	this->GetContentInsets(topLeftInset, bottomRightInset);
	return topLeftInset;
	}

ZPoint ZOSWindow::GetOrigin()
	{ return ZPoint::sZero; }

bool ZOSWindow::GetSizeBox(ZPoint& outSizeBoxSize)
	{
	ASSERTLOCKED();
	return false;
	}

ZPoint ZOSWindow::FromGlobal(const ZPoint& inGlobalPoint)
	{ return inGlobalPoint - this->ToGlobal(ZPoint::sZero); }

ZDragInitiator* ZOSWindow::CreateDragInitiator()
	{ return nil; }

bool ZOSWindow::WaitForMouse()
	{ return true; }

int32 ZOSWindow::GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone)
	{
	ZRect mainScreenRect = this->GetMainScreenFrame(false);
	if (mainScreenRect.Contains(inGlobalLocation))
		return 0;
	if (inReturnMainIfNone)
		return 0;
	return -1;
	}

int32 ZOSWindow::GetScreenIndex(const ZRect& inGlobalRect,  bool inReturnMainIfNone)
	{
	ZRect mainScreenRect = this->GetMainScreenFrame(false);
	if (!(inGlobalRect & mainScreenRect).IsEmpty())
		return 0;
	if (inReturnMainIfNone)
		return 0;
	return -1;
	}

int32 ZOSWindow::GetMainScreenIndex()
	{
	return 0;
	}

int32 ZOSWindow::GetScreenCount()
	{
	return 1;
	}

ZDCRgn ZOSWindow::GetDesktopRegion(bool inUsableSpaceOnly)
	{
	return this->GetMainScreenFrame(inUsableSpaceOnly);
	}

ZRect ZOSWindow::GetMainScreenFrame(bool inUsableSpaceOnly)
	{
	// Shortcut routine to get the main screen rect
	return this->GetScreenFrame(this->GetMainScreenIndex(), inUsableSpaceOnly);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindowOwner

ZOSWindowOwner::ZOSWindowOwner(ZOSWindow* inOSWindow)
:	fOSWindow(inOSWindow)
	{
	if (!fOSWindow)
		throw runtime_error("ZOSWindowOwner, no ZOSWindow passed");
	fOSWindow->SetOwner(this);
	}

ZOSWindowOwner::~ZOSWindowOwner()
	{
	ZOSWindow* theOSWindow = fOSWindow;
	fOSWindow = nil;
	theOSWindow->OwnerDisposed(this);
	}

void ZOSWindowOwner::OwnerDispatchMessage(const ZMessage& inMessage)
	{}

void ZOSWindowOwner::OwnerUpdate(const ZDC& inDC)
	{}

void ZOSWindowOwner::OwnerDrag(ZPoint inPoint, ZMouse::Motion inMotion, ZDrag& inDrag)
	{}

void ZOSWindowOwner::OwnerDrop(ZPoint inPoint, ZDrop& inDrop)
	{}

void ZOSWindowOwner::OwnerSetupMenus(ZMenuSetup& inMenuSetup)
	{}

bool ZOSWindowOwner::OwnerTitleIconHit(ZPoint inHitPoint)
	{ return false; }

ZDCInk ZOSWindowOwner::OwnerGetBackInk(bool inActive)
	{ return ZRGBColor::sWhite; }

void ZOSWindowOwner::OSWindowMustDispose(ZOSWindow* inOSWindow)
	{
	// This method is only called when an OSWindow is getting deleted by the application as a result
	// of the app being deleted. Normally windows get some kind of QuitRequested notification, or
	// are manually taken down in ZApp's overridden RunFinished method or overridden destructor.
	ZAssert(fOSWindow == inOSWindow);
	delete this;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp

ZOSApp::ZOSApp()
:	fOwner(nil)
	{}

ZOSApp::~ZOSApp()
	{
	ZAssert(fOwner == nil);
	}

void ZOSApp::SetOwner(ZOSAppOwner* inOwner)
	{
	ZAssert(fOwner == nil);
	fOwner = inOwner;
	}

void ZOSApp::OwnerDisposed(ZOSAppOwner* inOwner)
	{
	ZAssert(this->GetLock().IsLocked());
	ZAssert(fOwner == inOwner);
	fOwner = nil;
	delete this;
	}

void ZOSApp::QueueMessageForOwner(const ZMessage& inMessage)
	{
	ZMessage envelope;
	envelope.SetString("what", "zoolib:Owner");
	envelope.SetTuple("message", inMessage.AsTuple());
	this->QueueMessage(envelope);
	}

bool ZOSApp::DispatchMessage(const ZMessage& inMessage)
	{
	if (inMessage.GetString("what") == "zoolib:Owner")
		{
		this->DispatchMessageToOwner(ZMessage(inMessage.GetTuple("message")));
		return true;
		}
	return false;
	}

void ZOSApp::DispatchMessageToOwner(const ZMessage& iMessage)
	{
	if (fOwner)
		fOwner->OwnerDispatchMessage(iMessage);
	}

void ZOSApp::SetMenuBar(const ZMenuBar& inMenuBar)
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSAppOwner

ZOSAppOwner::ZOSAppOwner(ZOSApp* inOSApp)
	{
	fOSApp = inOSApp;
	fOSApp->SetOwner(this);
	}

ZOSAppOwner::~ZOSAppOwner()
	{
	fOSApp->OwnerDisposed(this);
	}

void ZOSAppOwner::OwnerDispatchMessage(const ZMessage& inMessage)
	{
	// By default just drop the message on the floor
	}

void ZOSAppOwner::OwnerSetupMenus(ZMenuSetup& inMenuSetup)
	{}

// =================================================================================================
