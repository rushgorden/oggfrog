static const char rcsid[] = "@(#) $Id: ZFakeWindow.cpp,v 1.5 2006/08/20 20:08:23 agreen Exp $";

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

#include "ZFakeWindow.h"
#include "ZDebug.h"
#include "ZPane.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZFakeWindow

ZFakeWindow::ZFakeWindow(ZEventHr* nextHandler)
:	ZEventHr(nextHandler),
	fWindowTarget(nil),
	fLastMousePane(nil),
	fLastDragPane(nil),
	fCaptureInput(nil)
	{}

ZFakeWindow::~ZFakeWindow()
	{
	if (fVector_WindowPanes.size() > 0)
		ZDebugStopf(1, ("ZFakeWindow::~ZFakeWindow, subclass did not call DeleteAllWindowPanes()"));
	}

void ZFakeWindow::SubHandlerBeingFreed(ZEventHr* inHandler)
	{
	if (inHandler == fWindowTarget)
		{
		ZEventHr* newHandler = inHandler->GetParentHandler();
		while (newHandler && !newHandler->WantsToBecomeTarget())
			newHandler = newHandler->GetParentHandler();
		fWindowTarget = newHandler;
		}
	if (inHandler == fLastMousePane)
		fLastMousePane = nil;
	if (fCaptureInput && fCaptureInput->GetPane() == inHandler)
		fCaptureInput->PaneBeingDeleted();
	ZEventHr::SubHandlerBeingFreed(inHandler);
	}

ZEventHr* ZFakeWindow::GetWindowTarget()
	{
	if (fWindowTarget)
		return fWindowTarget;
	return this;
	}

void ZFakeWindow::HandlerBecomeWindowTarget(ZEventHr* inHandler)
	{
	// Remember that this is to be our window target
	ZEventHr* oldTarget = this->GetWindowTarget();
	if (oldTarget != inHandler)
		{
		fWindowTarget = inHandler;
		ZEventHr* newTarget = this->GetWindowTarget();
		ZAssert(oldTarget);
		oldTarget->HandlerResignedWindowTarget(oldTarget, newTarget);

		ZAssert(newTarget);
		newTarget->HandlerBecameWindowTarget(oldTarget, newTarget);
		this->InvalidateWindowMenuBar();
		}
	}

void ZFakeWindow::InsertTabTargets(vector<ZEventHr*>& oTargets)
	{
	ZEventHr::InsertTabTargets(oTargets);
	// Walk through our window panes, asking them to insert their possible targets
	for (size_t x = 0; x < fVector_WindowPanes.size(); ++x)
		fVector_WindowPanes[x]->InsertTabTargets(oTargets);
	}

void ZFakeWindow::InvalidateWindowMenuBar()
	{}

ZDCInk ZFakeWindow::GetWindowBackInk()
	{ return ZDCInk::sWhite; }

bool ZFakeWindow::GetWindowActive()
	{ return true; }

void ZFakeWindow::BringFront(bool makeVisible)
	{
	// Does nothing here -- real windows will have a stacking order
	// and their overrides of this method will make the window frontmost
	}

ZDragInitiator* ZFakeWindow::CreateDragInitiator()
	{ return nil; }

bool ZFakeWindow::WaitForMouse()
	{ return true; }

void ZFakeWindow::WindowMessage_Mouse(const ZMessage& iMessage)
	{
	string theWhat = iMessage.GetString("what");
	if (theWhat == "zoolib:MouseDown")
		{
		bool handledMouseDown = false;
		if (ZCaptureInput* theCaptureInput = fCaptureInput)
			{
			theCaptureInput->IncrementUseCount();
			if (theCaptureInput->HandleMessage(iMessage))
				handledMouseDown = true;
			theCaptureInput->DecrementUseCount();
			}

		if (!handledMouseDown)
			{
			ZEvent_Mouse theEvent(iMessage);
			ZPoint paneHitPoint = theEvent.GetLocation() - this->GetWindowOrigin();
			for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
				{
				if ((*i)->GetVisible() && (*i)->CalcFrame().Contains(paneHitPoint))
					{
					ZEvent_Mouse theEvent(iMessage);
					if ((*i)->HandleClick(paneHitPoint - (*i)->GetLocation(), theEvent))
						break;
					}
				}
			}
		}
	else if (theWhat == "zoolib:MouseUp")
		{
		bool handledMouseUp = false;
		if (ZCaptureInput* theCaptureInput = fCaptureInput)
			{
			theCaptureInput->IncrementUseCount();
			if (theCaptureInput->HandleMessage(iMessage))
				handledMouseUp = true;
			theCaptureInput->DecrementUseCount();
			}
		}
	else if (theWhat == "zoolib:MouseEnter" || theWhat == "zoolib:MouseChange" || theWhat == "zoolib:MouseExit")
		{
		bool handledMouseOther = false;
		if (ZCaptureInput* theCaptureInput = fCaptureInput)
			{
			theCaptureInput->IncrementUseCount();
			if (theCaptureInput->HandleMessage(iMessage))
				handledMouseOther = true;
			theCaptureInput->DecrementUseCount();
			}

		if (!handledMouseOther)
			{
			ZPoint windowLocation = iMessage.GetPoint("where");

			if (theWhat == "zoolib:MouseExit")
				{
				if (fLastMousePane)
					fLastMousePane->MouseMotion(fLastMousePane->FromWindow(windowLocation), ZMouse::eMotionLeave);
				fLastMousePane = nil;
				}
			else
				{
				// Find the current pane
				ZPoint paneHitPoint = windowLocation - this->GetWindowOrigin();
				ZSubPane* currentMousePane = nil;
				for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); currentMousePane == nil && i != fVector_WindowPanes.end(); ++i)
					{
					if ((*i)->GetVisible() && (*i)->CalcFrameRgn().Contains(paneHitPoint))
						currentMousePane = (*i)->FindPane(paneHitPoint - (*i)->GetLocation());
					}
				bool changedPane = false;
				if (fLastMousePane != currentMousePane)
					{
					if (fLastMousePane)
						fLastMousePane->MouseMotion(fLastMousePane->FromWindow(windowLocation), ZMouse::eMotionLeave);
					fLastMousePane = currentMousePane;
					if (fLastMousePane)
						fLastMousePane->MouseMotion(fLastMousePane->FromWindow(windowLocation), ZMouse::eMotionEnter);
					changedPane = true;
					}
				fLastMouseLocation = windowLocation;
				if (fLastMousePane)
					{
					ZPoint currentLocation = fLastMousePane->FromWindow(windowLocation);
					fLastMousePane->MouseMotion(currentLocation, ((fLastMouseLocation != currentLocation) || changedPane) ? ZMouse::eMotionMove : ZMouse::eMotionLinger);
					}
				}
			}
		}
	}

void ZFakeWindow::WindowMessage_Key(const ZMessage& iMessage)
	{
	if (iMessage.GetString("what") == "zoolib:KeyDown")
		{
		ZEvent_Key theEvent(iMessage);
		this->GetWindowTarget()->DispatchKeyDown(theEvent);
		}
	}

void ZFakeWindow::WindowCaptureAcquire()
	{}

void ZFakeWindow::WindowCaptureRelease()
	{}

void ZFakeWindow::WindowIdle()
	{
	for (size_t x = 0; x < fVector_IdlePanes.size(); ++x)
		fVector_IdlePanes[x]->DoIdle();
	}

void ZFakeWindow::WindowActivate(bool iActive)
	{
	for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
		(*i)->Activate(iActive);
	}

void ZFakeWindow::WindowCaptureInstall(ZCaptureInput* iCaptureInput)
	{
// Detach the existing mouse tracker, if any
	if (ZCaptureInput* oldCaptureInput = fCaptureInput)
		{
		fCaptureInput = nil;

		oldCaptureInput->IncrementUseCount();

		this->WindowCaptureRelease();

		oldCaptureInput->Removed();
		oldCaptureInput->DecrementUseCount();

// Balance the increment that occurred when it was installed
		oldCaptureInput->DecrementUseCount();
		}

// Increment the use count because we'll be keeping it around
	if (iCaptureInput)
		{
		fCaptureInput = iCaptureInput;
		iCaptureInput->IncrementUseCount();

		this->WindowCaptureAcquire();

		iCaptureInput->IncrementUseCount();

		iCaptureInput->Installed();

		iCaptureInput->DecrementUseCount();
		}
	}

void ZFakeWindow::WindowCaptureRemove(ZCaptureInput* iCaptureInput)
	{
// Detach the existing mouse tracker, if any
	if (ZCaptureInput* oldCaptureInput = fCaptureInput)
		{
		fCaptureInput = nil;

		oldCaptureInput->IncrementUseCount();

		this->WindowCaptureRelease();

		oldCaptureInput->Removed();
		oldCaptureInput->DecrementUseCount();

// Balance the increment that occurred when it was installed
		oldCaptureInput->DecrementUseCount();
		}
	}

void ZFakeWindow::WindowCaptureCancelled()
	{
	if (ZCaptureInput* oldCaptureInput = fCaptureInput)
		{
		oldCaptureInput->IncrementUseCount();
		fCaptureInput = nil;
		// We used to call ZOSWindow::ReleaseCapture -- but I don't think it's necessary.
		// fOSWindow->ReleaseCapture();
		oldCaptureInput->Cancelled();
		oldCaptureInput->DecrementUseCount();
		// Balance the increment that occurred when it was installed
		oldCaptureInput->DecrementUseCount();
		}
	}

void ZFakeWindow::WindowPaneAdded(ZWindowPane* iPane)
	{ fVector_WindowPanes.push_back(iPane); }

void ZFakeWindow::WindowPaneRemoved(ZWindowPane* iPane)
	{ fVector_WindowPanes.erase(find(fVector_WindowPanes.begin(), fVector_WindowPanes.end(), iPane)); }

void ZFakeWindow::DeleteAllWindowPanes()
	{
	while (fVector_WindowPanes.size() > 0)
		delete *fVector_WindowPanes.begin();
	}

void ZFakeWindow::DoUpdate(const ZDC& iDC)
	{
	ZDC localDC(iDC);
	for (vector<ZWindowPane*>::iterator i = fVector_WindowPanes.begin(); i != fVector_WindowPanes.end(); ++i)
		{
		if ((*i)->GetVisible())
			{
			ZPoint location = (*i)->GetLocation();
			localDC.OffsetOrigin(location);
			(*i)->HandleDraw(localDC);
			localDC.OffsetOrigin(location);
			}
		}
	}

void ZFakeWindow::RegisterIdlePane(ZSubPane* iSubPane)
	{ fVector_IdlePanes.push_back(iSubPane); }

void ZFakeWindow::UnregisterIdlePane(ZSubPane* iSubPane)
	{ fVector_IdlePanes.erase(find(fVector_IdlePanes.begin(), fVector_IdlePanes.end(), iSubPane)); }

ZPoint ZFakeWindow::ToGlobal(const ZPoint& iWindowPoint)
	{ return iWindowPoint + this->WindowToGlobal(ZPoint::sZero); }

ZPoint ZFakeWindow::FromGlobal(const ZPoint& iGlobalPoint)
	{ return iGlobalPoint - this->WindowToGlobal(ZPoint::sZero); }

ZRect ZFakeWindow::ToGlobal(const ZRect& iWindowRect)
	{ return iWindowRect + this->WindowToGlobal(ZPoint::sZero); }

ZRect ZFakeWindow::FromGlobal(const ZRect& iGlobalRect)
	{ return iGlobalRect - this->WindowToGlobal(ZPoint::sZero); }

ZDCRgn ZFakeWindow::ToGlobal(const ZDCRgn& iWindowRgn)
	{ return iWindowRgn + this->WindowToGlobal(ZPoint::sZero); }

ZDCRgn ZFakeWindow::FromGlobal(const ZDCRgn& iGlobalRgn)
	{ return iGlobalRgn - this->WindowToGlobal(ZPoint::sZero); }

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowPane

ZWindowPane::ZWindowPane(ZFakeWindow* iOwnerWindow, ZPaneLocator* iPaneLocator)
:	ZSuperPane(iOwnerWindow, iPaneLocator)
	{
	ZAssertStop(2, iOwnerWindow);
	fFakeWindow = iOwnerWindow;
	fFakeWindow->WindowPaneAdded(this);
	}

ZWindowPane::~ZWindowPane()
	{
// Delete all our sub panes here, so anything which needs to access its window in its destructor
// will still be able to do so.
	while (fSubPanes.size())
		delete fSubPanes.back();
	fFakeWindow->WindowPaneRemoved(this);
	}

ZFakeWindow* ZWindowPane::GetWindow()
	{ return fFakeWindow; }

ZPoint ZWindowPane::GetSize()
	{
	// AG 98-07-03. Work a little harder to make the window pane fill the window. Previously we'd call
	// ZSubPane::GetSize() which returns ZPoint::sZero if the pane locator for this pane doesn't know how
	// big it should be. Instead we check with our pane locator directly, so we can call in to our window
	// if the pane locator doesn't know our size.
	ZPoint theSize;
	if (fPaneLocator && fPaneLocator->GetPaneSize(this, theSize))
		return theSize;
	return fFakeWindow->GetWindowSize();
	}

ZPoint ZWindowPane::GetCumulativeTranslation()
	{ return ZSuperPane::GetCumulativeTranslation() - fFakeWindow->GetWindowOrigin(); }

ZDCInk ZWindowPane::GetBackInk(const ZDC& iDC)
	{
	ZDCInk theInk;
	if (fPaneLocator && fPaneLocator->GetPaneBackInk(this, iDC, theInk))
		return theInk;
	return fFakeWindow->GetWindowBackInk();
	}

ZDCRgn ZWindowPane::CalcVisibleBoundsRgn()
	{
	// Calculate our normal bounds rgn
	ZDCRgn myBoundsRgn = this->CalcBoundsRgn();
	// But then clip it to our window's visible content rgn (which will have the update
	// rgn removed, if we're not updating, or the non-update rgn removed if we are
	// updating). It also ensures that if we're explicitly managing our clip rgn (like in ZWindoid)
	// that everything works.
	return myBoundsRgn & this->FromWindow(fFakeWindow->GetWindowVisibleContentRgn());
	}

ZPoint ZWindowPane::ToSuper(const ZPoint& iPanePoint)
	{ return iPanePoint + this->GetLocation(); }

ZPoint ZWindowPane::ToWindow(const ZPoint& iPanePoint)
	{ return iPanePoint + this->GetLocation() + fFakeWindow->GetWindowOrigin(); }

ZPoint ZWindowPane::ToGlobal(const ZPoint& iPanePoint)
	{ return fFakeWindow->WindowToGlobal(this->ToWindow(iPanePoint)); }

// =================================================================================================
#pragma mark -
#pragma mark * ZCaptureInput

ZCaptureInput::ZCaptureInput(ZSubPane* iPane)
	{
	fPane = iPane;
	fUseCount = 0;
	}

ZCaptureInput::~ZCaptureInput()
	{
	ZAssert(fUseCount == 0);
	}

void ZCaptureInput::Install()
	{
	this->InvalidateCursor();
	fPane->GetWindow()->WindowCaptureInstall(this);
	}

void ZCaptureInput::Finish()
	{
	if (fPane)
		fPane->GetWindow()->WindowCaptureRemove(this);
	}

void ZCaptureInput::PaneBeingDeleted()
	{
	ZAssert(fPane != nil);
	fPane = nil;
	}

void ZCaptureInput::IncrementUseCount()
	{ ++fUseCount; }

void ZCaptureInput::DecrementUseCount()
	{
	if (--fUseCount == 0)
		delete this;
	}

void ZCaptureInput::InvalidateCursor()
	{ fPane->InvalidateCursor(); }

void ZCaptureInput::Installed()
	{
	}

void ZCaptureInput::Removed()
	{
	}

void ZCaptureInput::Cancelled()
	{
	}

bool ZCaptureInput::HandleMessage(const ZMessage& iMessage)
	{
	return false;
	}

bool ZCaptureInput::GetCursor(ZPoint iPanePoint, ZCursor& oCursor)
	{
	if (fPane)
		return fPane->GetCursor(iPanePoint, oCursor);
	return false;
	}

