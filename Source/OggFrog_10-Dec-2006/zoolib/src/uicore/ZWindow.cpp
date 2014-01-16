static const char rcsid[] = "@(#) $Id: ZWindow.cpp,v 1.10 2006/07/30 21:14:41 agreen Exp $";

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

#include "ZWindow.h"

#include "ZDebug.h"
#include "ZDragClip.h"

#ifndef kDebug_Window
#	define kDebug_Window 1
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowSupervisor

void ZWindowSupervisor::WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall)
	{
	}

void ZWindowSupervisor::WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup)
	{
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindow

static ZMutexNR sMutex_WindowWatcher;

ZWindow::ZWindow(ZWindowSupervisor* inWindowSupervisor, ZOSWindow* inOSWindow)
:	ZOSWindowOwner(inOSWindow),
	ZMessageReceiver(this),
	ZFakeWindow(nil),
	fWindowSupervisor(inWindowSupervisor),
	fLastKnownWindowSize(ZPoint::sZero),
	fPostedCursorInvalid(false),
	fPostedInvalidateMenuBar(false),
	fWatcher_Head(nil)
	{
	fLastKnownWindowSize = fOSWindow->GetSize();
	this->InvalidateWindowMenuBar();
	}

ZWindow::~ZWindow()
	{
	this->WindowCaptureRemove(fCaptureInput);

	sMutex_WindowWatcher.Acquire();
	while (fWatcher_Head)
		{
		ZWindowWatcher* nextWatcher = fWatcher_Head->fNext;
		fWatcher_Head->fWindow = nil;
		fWatcher_Head->fPrev = nil;
		fWatcher_Head->fNext = nil;
		fWatcher_Head = nextWatcher;
		}
	sMutex_WindowWatcher.Release();

	// By setting the target to nil we short circuit a lot of crap in ZFakeWindow::SubHandlerBeingFreed
	fWindowTarget = nil;
	this->DeleteAllWindowPanes();
	}

// ==================================================
#pragma mark -
#pragma mark From ZMessageLooper

ZMutexBase& ZWindow::GetLock()
	{ return fOSWindow->GetLock(); }

bool ZWindow::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZMessageLooper::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "Dispose")
			{
			delete this;
			return true;
			}
		else if (theWhat == "zoolib:ScreenLayoutChanged")
			{
			this->ForceOnScreen(true, true);
			return true;
			}
		else if (theWhat == "InvalidCursor")
			{
			fPostedCursorInvalid = false;
			ZCursor theCursor;
			bool gotIt = false;
			if (ZCaptureInput* theCaptureInput = fCaptureInput)
				{
				theCaptureInput->IncrementUseCount();
				if (ZSubPane* thePane = theCaptureInput->GetPane())
					gotIt = theCaptureInput->GetCursor(thePane->FromWindow(fLastMouseLocation), theCursor);
				theCaptureInput->DecrementUseCount();
				}
			else
				{
				ZPoint paneHitPoint = fLastMouseLocation - this->GetWindowOrigin();
				for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
					{
					if ((*i)->GetVisible() && (*i)->CalcFrameRgn().Contains(paneHitPoint))
						{
						if (ZSubPane* hitPane = (*i)->FindPane(paneHitPoint - (*i)->GetLocation()))
							gotIt = hitPane->GetCursor(hitPane->FromWindow(paneHitPoint), theCursor);
						}
					}
				}
			// If neither the mouse tracker nor the enclosing pane set the cursor, ensure it's reset
			// to an arrow. If false is returned by the GetCursor methods they're supposed not to have
			// touched theCursor, but let's make sure and set it to be an arrow.
			if (!gotIt)
				theCursor.Set(ZCursor::eCursorTypeArrow);
			fOSWindow->SetCursor(theCursor);
			return true;
			}
		else if (theWhat == "zoolib:InvalidateMenuBar")
			{
			if (fPostedInvalidateMenuBar)
				{
				fPostedInvalidateMenuBar = false;
				ZMenuBar theMenuBar;
				this->GetWindowTarget()->DispatchInstallMenus(theMenuBar);
				fOSWindow->SetMenuBar(theMenuBar);
				}
			return true;
			}
		else if (theWhat == "zoolib:InvalidateAllMenuBars")
			{
			this->InvalidateWindowMenuBar();
			return true;
			}
		else if (theWhat == "zoolib:Close")
			{
			this->WindowCloseByUser();
			return true;
			}
		else if (theWhat == "zoolib:Zoom")
			{
			this->WindowZoomByUser();
			return true;
			}
		else if (theWhat == "zoolib:MouseDown")
			{
			this->WindowMessage_Mouse(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:MouseUp")
			{
			this->WindowMessage_Mouse(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:MouseEnter" || theWhat == "zoolib:MouseChange" || theWhat == "zoolib:MouseExit")
			{
			this->WindowMessage_Mouse(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:CaptureCancelled")
			{
			this->WindowCaptureCancelled();
			return true;
			}
		else if (theWhat == "zoolib:FrameChange")
			{
			this->WindowFrameChange();
			return true;
			}
		else if (theWhat == "zoolib:ShownState")
			{
			this->WindowShownState((ZShownState)inMessage.GetInt32("shownState"));
			return true;
			}
		else if (theWhat == "zoolib:Activate")
			{
			if (fBackInkActive && fBackInkInactive && (fBackInkActive != fBackInkInactive))
				{
				if (!fBackInkActive->GetInk().IsSameAs(fBackInkInactive->GetInk()))
					this->InvalidateWindowRegion(ZDCRgn(this->GetSize()));
				}
			this->WindowActivate(inMessage.GetBool("active"));
			return true;
			}
		else if (theWhat == "zoolib:AppActivate")
			{
			this->WindowAppActivate(inMessage.GetBool("active"));
			return true;
			}
		else if (theWhat == "zoolib:Menu")
			{
			if (!this->WindowMenuMessage(inMessage) && fWindowSupervisor)
				fWindowSupervisor->WindowSupervisorPostMessage(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:KeyDown")
			{
			this->WindowMessage_Key(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:Idle")
			{
			this->WindowIdle();
			return true;
			}
		else if (theWhat == "zoolib:InvokeFunction")
			{
			if (MessageProc theProc = reinterpret_cast<MessageProc>(inMessage.GetPointer("function")))
				theProc(this, inMessage.GetValue("param"));
			return true;
			}
		}

	return false;
	}

void ZWindow::QueueMessage(const ZMessage& inMessage)
	{
	fOSWindow->QueueMessageForOwner(inMessage);
	}

// ==================================================
#pragma mark -
#pragma mark From ZMessageReceiver

void ZWindow::ReceivedMessage(const ZMessage& inMessage)
	{}

// ==================================================
#pragma mark -
#pragma mark From ZFakeWindow

ZMutexBase& ZWindow::GetWindowLock()
	{ return fOSWindow->GetLock(); }

ZPoint ZWindow::GetWindowOrigin()
	{ return fOSWindow->GetOrigin(); }

ZPoint ZWindow::GetWindowSize()
	{ return fLastKnownWindowSize; }

ZDCRgn ZWindow::GetWindowVisibleContentRgn()
	{ return fOSWindow->GetVisibleContentRgn(); }

void ZWindow::InvalidateWindowRegion(const ZDCRgn& inWindowRegion)
	{ fOSWindow->InvalidateRegion(inWindowRegion); }

ZPoint ZWindow::WindowToGlobal(const ZPoint& inWindowPoint)
	{ return fOSWindow->ToGlobal(inWindowPoint); }

void ZWindow::UpdateWindowNow()
	{ fOSWindow->UpdateNow(); }

ZRef<ZDCCanvas> ZWindow::GetWindowCanvas()
	{ return fOSWindow->GetCanvas(); }

void ZWindow::GetWindowNative(void* outNativeWindow)
	{ fOSWindow->GetNative(outNativeWindow); }

void ZWindow::InvalidateWindowCursor()
	{
	if (!fPostedCursorInvalid)
		{
		fPostedCursorInvalid = true;
		ZMessage theMessage;
		theMessage.SetString("what", "InvalidCursor");
		this->QueueMessage(theMessage);
		}
	}

void ZWindow::InvalidateWindowMenuBar()
	{
	ZAssertLocked(kDebug_Window, this->GetLock());
	if (!fPostedInvalidateMenuBar)
		{
		fPostedInvalidateMenuBar = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:InvalidateMenuBar");
		this->QueueMessage(theMessage);
		}
	}

ZDCInk ZWindow::GetWindowBackInk()
	{
	if (this->GetWindowActive())
		{
		if (fBackInkActive)
			return fBackInkActive->GetInk();
		}
	if (fBackInkInactive)
		return fBackInkInactive->GetInk();
	return ZRGBColor::sWhite;
	}

bool ZWindow::GetWindowActive()
	{ return fOSWindow->GetActive(); }

void ZWindow::BringFront(bool inMakeVisible)
	{
	if (fPostedInvalidateMenuBar)
		{
		try
			{
			fPostedInvalidateMenuBar = false;
			ZMenuBar theMenuBar;
			this->GetWindowTarget()->DispatchInstallMenus(theMenuBar);
			fOSWindow->SetMenuBar(theMenuBar);
			}
		catch (...)
			{}
		}
	fOSWindow->BringFront(inMakeVisible);
	if (inMakeVisible)
		this->UpdateWindowNow();
	}

ZDragInitiator* ZWindow::CreateDragInitiator()
	{ return fOSWindow->CreateDragInitiator(); }

bool ZWindow::WaitForMouse()
	{ return fOSWindow->WaitForMouse(); }

void ZWindow::WindowCaptureAcquire()
	{
	fOSWindow->AcquireCapture();
	}

void ZWindow::WindowCaptureRelease()
	{
	fOSWindow->ReleaseCapture();
	}

// ==================================================
#pragma mark -
#pragma mark From ZEventHr via ZFakeWindow

void ZWindow::HandleInstallMenus(ZMenuInstall& inMenuInstall, ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fWindowSupervisor)
			fWindowSupervisor->WindowSupervisorInstallMenus(inMenuInstall);
		}

	ZEventHr::HandleInstallMenus(inMenuInstall, inOriginalTarget, inFirstCall);
	}

void ZWindow::HandleSetupMenus(ZMenuSetup& inMenuSetup, ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (inFirstCall)
		{
		if (fWindowSupervisor)
			fWindowSupervisor->WindowSupervisorSetupMenus(inMenuSetup);
		}

	ZEventHr::HandleSetupMenus(inMenuSetup, inOriginalTarget, inFirstCall);
	}

bool ZWindow::HandleMenuMessage(const ZMessage& inMessage, ZEventHr* inOriginalTarget, bool inFirstCall)
	{
	if (ZEventHr::HandleMenuMessage(inMessage, inOriginalTarget, inFirstCall))
		return true;
	if (inFirstCall)
		return false;
	if (fWindowSupervisor)
		fWindowSupervisor->WindowSupervisorPostMessage(inMessage);
	return true;
	}

// ==================================================
#pragma mark -
#pragma mark * ZWindow Protocol

bool ZWindow::WindowQuitRequested()
	{
	if (this->WaitingForPoseModal())
		return false;
	delete this;
	return true;
	}

void ZWindow::WindowShownState(ZShownState inShownState)
	{}

void ZWindow::WindowAppActivate(bool inActive)
	{}

void ZWindow::WindowCloseByUser()
	{ this->CloseAndDispose(); }

void ZWindow::WindowZoomByUser()
	{ this->Zoom(); }

void ZWindow::WindowFrameChange()
	{
	if (fLastKnownWindowSize != fOSWindow->GetSize())
		{
		for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
			(*i)->FramePreChange();

		// Figure out the difference between our old and new regions
		ZDCRgn invalidRgn = ZRect(fLastKnownWindowSize);

		fLastKnownWindowSize = fOSWindow->GetSize();

		invalidRgn ^= ZRect(fLastKnownWindowSize);
		invalidRgn += this->GetWindowOrigin();
		this->InvalidateWindowRegion(invalidRgn);

		for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
			(*i)->FramePostChange();
		}
	}

bool ZWindow::WindowMenuMessage(const ZMessage& inMessage)
	{ return this->GetWindowTarget()->DispatchMenuMessage(inMessage); }

void ZWindow::InvokeFunctionFromMessageLoop(MessageProc inProc, const ZTupleValue& inTV)
	{
	ZMessage wrapperMessage;
	wrapperMessage.SetString("what", "zoolib:InvokeFunction");
	wrapperMessage.SetPointer("function", reinterpret_cast<void*>(inProc));
	wrapperMessage.SetValue("param", inTV);
	this->QueueMessage(wrapperMessage);
	}

void ZWindow::CloseAndDispose()
	{
	fOSWindow->SetShownState(eZShownStateHidden);
	ZMessage theMessage;
	theMessage.SetString("what", "Dispose");
	this->QueueMessage(theMessage);
	}

void ZWindow::Show(bool inShown)
	{
	switch (fOSWindow->GetShownState())
		{
		case eZShownStateHidden:
			{
			if (inShown)
				fOSWindow->SetShownState(eZShownStateShown);
			break;
			}
		case eZShownStateMinimized:
			{
			if (!inShown)
				fOSWindow->SetShownState(eZShownStateHidden);
			break;
			}
		case eZShownStateShown:
			{
			if (!inShown)
				fOSWindow->SetShownState(eZShownStateHidden);
			break;
			}
		default:
			ZUnimplemented();
		}
	}

bool ZWindow::GetShown()
	{
	switch (fOSWindow->GetShownState())
		{
		case eZShownStateHidden:
			return false;
		case eZShownStateMinimized:
		case eZShownStateShown:
			return true;
		default:
			ZUnimplemented();
		}
	return false;
	}

void ZWindow::SetShownState(ZShownState inState)
	{ fOSWindow->SetShownState(inState); }

ZShownState ZWindow::GetShownState()
	{ return fOSWindow->GetShownState(); }

void ZWindow::SetTitle(const string& inTitle)
	{ fOSWindow->SetTitle(inTitle); }

string ZWindow::GetTitle() const
	{ return fOSWindow->GetTitle(); }

void ZWindow::SetLocation(ZPoint inLocation)
	{ fOSWindow->SetLocation(inLocation); }

ZPoint ZWindow::GetLocation() const
	{ return fOSWindow->GetLocation(); }

void ZWindow::SetSize(ZPoint newSize)
	{ fOSWindow->SetSize(newSize); }

ZPoint ZWindow::GetSize() const
	{ return fLastKnownWindowSize; }

void ZWindow::SetFrame(const ZRect& inFrame)
	{ fOSWindow->SetFrame(inFrame); }

ZRect ZWindow::GetFrame()
	{ return fOSWindow->GetFrame(); }

ZRect ZWindow::GetFrameNonZoomed()
	{ return fOSWindow->GetFrameNonZoomed(); }

void ZWindow::SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize)
	{ fOSWindow->SetSizeLimits(inMinSize, inMaxSize); }

void ZWindow::PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists)
	{ fOSWindow->PoseModal(inRunCallerMessageLoopNested, outCallerExists, outCalleeExists); }

void ZWindow::EndPoseModal()
	{ fOSWindow->EndPoseModal(); }

bool ZWindow::WaitingForPoseModal()
	{ return fOSWindow->WaitingForPoseModal(); }

void ZWindow::Center()
	{ fOSWindow->Center(); }

void ZWindow::CenterDialog()
	{ fOSWindow->CenterDialog(); }

void ZWindow::Zoom()
	{ fOSWindow->Zoom(); }

void ZWindow::ForceOnScreen(bool ensureTitleBarAccessible, bool ensureSizeBoxAccessible)
	{ fOSWindow->ForceOnScreen(ensureTitleBarAccessible, ensureSizeBoxAccessible); }

void ZWindow::SetBackInks(const ZRef<ZUIInk>& inBackInkActive, const ZRef<ZUIInk>& inBackInkInactive)
	{
	if ((fBackInkActive != inBackInkActive) || (fBackInkInactive != inBackInkInactive))
		{
		fBackInkActive = inBackInkActive;
		fBackInkInactive = inBackInkInactive;
		this->InvalidateWindowRegion(ZDCRgn(this->GetSize()));
		}
	}

void ZWindow::SetBackInk(const ZRef<ZUIInk>& inBackInk)
	{
	if ((fBackInkActive != inBackInk) || (fBackInkInactive != inBackInk))
		{
		fBackInkActive = inBackInk;
		fBackInkInactive = inBackInk;
		this->InvalidateWindowRegion(ZDCRgn(this->GetSize()));
		}
	}

void ZWindow::SetTitleIcon(const ZDCPixmapCombo& inPixmapCombo)
	{ fOSWindow->SetTitleIcon(inPixmapCombo.GetColor(), inPixmapCombo.GetMono(), inPixmapCombo.GetMask()); }

void ZWindow::SetTitleIcon(const ZDCPixmap& colorPixmap, const ZDCPixmap& monoPixmap, const ZDCPixmap& maskPixmap)
	{ fOSWindow->SetTitleIcon(colorPixmap, monoPixmap, maskPixmap); }

ZCoord ZWindow::GetTitleIconPreferredHeight()
	{ return fOSWindow->GetTitleIconPreferredHeight(); }

// ==================================================
#pragma mark -
#pragma mark From ZOSWindowOwner

void ZWindow::OwnerDispatchMessage(const ZMessage& inMessage)
	{ this->DispatchMessage(inMessage); }

void ZWindow::OwnerUpdate(const ZDC& inDC)
	{
	// Call ZFakeWindow's DoUpdate method
	this->DoUpdate(inDC);
	}

void ZWindow::OwnerDrag(ZPoint inWindowLocation, ZMouse::Motion inMotion, ZDrag& inDrag)
	{
	ZPoint paneHitPoint = inWindowLocation - this->GetWindowOrigin();
	if (inMotion == ZMouse::eMotionLeave)
		{
		if (fLastDragPane)
			{
			if (ZDragDropHandler* theDragDropHandler = fLastDragPane->GetDragDropHandler())
				{
				try
					{ theDragDropHandler->HandleDrag(fLastDragPane, fLastDragPane->FromWindow(inWindowLocation), ZMouse::eMotionLeave, inDrag); }
				catch (...)
					{}
				}
			fLastDragPane = nil;
			}
		}
	else
		{
		// Find the current pane
		ZSubPane* dragPane = nil;
		for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end() && dragPane == nil; ++i)
			{
			if ((*i)->GetVisible() && (*i)->CalcFrameRgn().Contains(paneHitPoint))
				dragPane = (*i)->FindPane(paneHitPoint - (*i)->GetLocation());
			}

		ZMouse::Motion realMotion = inMotion;
		if (fLastDragPane != dragPane)
			{
			if (fLastDragPane)
				{
				if (ZDragDropHandler* lastDragDropHandler = fLastDragPane->GetDragDropHandler())
					{
					try
						{ lastDragDropHandler->HandleDrag(fLastDragPane, fLastDragPane->FromWindow(inWindowLocation), ZMouse::eMotionLeave, inDrag); }
					catch (...)
						{}
					}
				}
			fLastDragPane = dragPane;
			if (fLastDragPane)
				{
				if (ZDragDropHandler* currentDragDropHandler = fLastDragPane->GetDragDropHandler())
					{
					try
						{ currentDragDropHandler->HandleDrag(fLastDragPane, fLastDragPane->FromWindow(inWindowLocation), ZMouse::eMotionEnter, inDrag); }
					catch (...)
						{}
					}
				}
			realMotion = ZMouse::eMotionLinger;
			}
		if (fLastDragPane)
			{
			if (ZDragDropHandler* currentDragDropHandler = fLastDragPane->GetDragDropHandler())
				{
				try
					{ currentDragDropHandler->HandleDrag(fLastDragPane, fLastDragPane->FromWindow(inWindowLocation), realMotion, inDrag); }
				catch (...)
					{}
				}
			}
		}
	}

void ZWindow::OwnerDrop(ZPoint inWindowLocation, ZDrop& inDrop)
	{
	ZPoint paneHitPoint = inWindowLocation - this->GetWindowOrigin();
	for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
		{
		if ((*i)->GetVisible() && (*i)->CalcFrameRgn().Contains(paneHitPoint))
			{
			if (ZSubPane* dragPane = (*i)->FindPane(paneHitPoint - (*i)->GetLocation()))
				{
				if (ZDragDropHandler* theDragDropHandler = dragPane->GetDragDropHandler())
					{
					try
						{ theDragDropHandler->HandleDrop(dragPane, dragPane->FromWindow(inWindowLocation), inDrop); }
					catch (...)
						{}
					}
				break;
				}
			}
		}
	}

void ZWindow::OwnerSetupMenus(ZMenuSetup& inMenuSetup)
	{ this->GetWindowTarget()->DispatchSetupMenus(inMenuSetup); }

ZDCInk ZWindow::OwnerGetBackInk(bool inActive)
	{
	// Because OwnerGetBackInk can be called at any time (ie its the only OwnerXXX method that is not called under the
	// protection of our looper lock), we take a local copy of our instance variables
	// and then ask the local copy for its ink. ZRef<> is thread-safe, so this ensures we don't make
	// a method call on an object whose only reference is our instance variable.
	if (inActive)
		{
		if (ZRef<ZUIInk> theInk = fBackInkActive)
			return theInk->GetInk();
		}
	if (ZRef<ZUIInk> theInk = fBackInkInactive)
		return theInk->GetInk();
	return ZOSWindowOwner::OwnerGetBackInk(inActive);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowWatcher

ZWindowWatcher::ZWindowWatcher(ZWindow* inWindow)
:	fWindow(inWindow)
	{
	fPrev = nil;
	fNext = nil;
	if (inWindow)
		{
		// You must hold the lock on a window at the time a watcher is created. You do not
		// have to do so when it goes out of scope (which is the whole point -- to be able
		// to know reliably when a window we *knew* about is still in existence)
		ZAssertLocked(kDebug_Window, inWindow->GetLock());

		sMutex_WindowWatcher.Acquire();
		fWindow = inWindow;
		fNext = fWindow->fWatcher_Head;
		if (fWindow->fWatcher_Head)
			fWindow->fWatcher_Head->fPrev = this;
		fWindow->fWatcher_Head = this;
		sMutex_WindowWatcher.Release();
		}
	}

ZWindowWatcher::~ZWindowWatcher()
	{
	if (fWindow)
		{
		sMutex_WindowWatcher.Acquire();
		if (fWindow)
			{
			if (fPrev)
				fPrev->fNext = fNext;
			if (fNext)
				fNext->fPrev = fPrev;
			if (fWindow->fWatcher_Head == this)
				fWindow->fWatcher_Head = fNext;
			}
		sMutex_WindowWatcher.Release();
		}
	}

bool ZWindowWatcher::IsValid()
	{ return fWindow != nil; }
