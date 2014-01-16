/*  @(#) $Id: ZFakeWindow.h,v 1.4 2006/04/10 20:44:22 agreen Exp $ */

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

#ifndef __ZFakeWindow__
#define __ZFakeWindow__ 1
#include "zconfig.h"

#include "ZDC.h"
#include "ZPane.h"
#include "ZTypes.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZFakeWindow

class ZDragInitiator;
class ZCaptureInput;
class ZWindowPane;

class ZFakeWindow : public ZEventHr
	{
public:
	ZFakeWindow(ZEventHr* nextHandler);
	virtual ~ZFakeWindow();

// From ZEventHr
	virtual void SubHandlerBeingFreed(ZEventHr* iHandler);
	virtual ZEventHr* GetWindowTarget();
	virtual void HandlerBecomeWindowTarget(ZEventHr* iHandler);
	virtual void InsertTabTargets(vector<ZEventHr*>& oTargets);

// The essential protocol -- you must provide overrides of these methods
	virtual ZMutexBase& GetWindowLock() = 0;
	virtual ZPoint GetWindowOrigin() = 0;
	virtual ZPoint GetWindowSize() = 0;
	virtual ZDCRgn GetWindowVisibleContentRgn() = 0;
	virtual void InvalidateWindowRegion(const ZDCRgn& iWindowRegion) = 0;
	virtual ZPoint WindowToGlobal(const ZPoint& iWindowPoint) = 0;
	virtual void UpdateWindowNow() = 0;
	virtual ZRef<ZDCCanvas> GetWindowCanvas() = 0;
	virtual void GetWindowNative(void* oNativeWindow) = 0;
	virtual void InvalidateWindowCursor() = 0;

// The rest of the overrideable ZFakeWindow protocol
	virtual void InvalidateWindowMenuBar();
	virtual ZDCInk GetWindowBackInk();
	virtual bool GetWindowActive();
	virtual void BringFront(bool iMakeVisible = true);
	virtual ZDragInitiator* CreateDragInitiator();
	virtual bool WaitForMouse();

	virtual void WindowMessage_Mouse(const ZMessage& iMessage);
	virtual void WindowMessage_Key(const ZMessage& iMessage);

	virtual void WindowCaptureAcquire();
	virtual void WindowCaptureRelease();

	virtual void WindowIdle();

	virtual void WindowActivate(bool iActive);

// Non-overrideable protocol
	void WindowCaptureInstall(ZCaptureInput* iCaptureInput);
	void WindowCaptureRemove(ZCaptureInput* iCaptureInput);
	void WindowCaptureCancelled();

	void WindowPaneAdded(ZWindowPane* iPane);
	void WindowPaneRemoved(ZWindowPane* iPane);
	void DeleteAllWindowPanes();

	void DoUpdate(const ZDC& iDC);

	void RegisterIdlePane(ZSubPane* iSubPane);
	void UnregisterIdlePane(ZSubPane* iSubPane);

	ZPoint ToGlobal(const ZPoint& iWindowPoint);
	ZPoint FromGlobal(const ZPoint& iGlobalPoint);
	ZRect ToGlobal(const ZRect& iWindowRect);
	ZRect FromGlobal(const ZRect& iGlobalRect);
	ZDCRgn ToGlobal(const ZDCRgn& iWindowRgn);
	ZDCRgn FromGlobal(const ZDCRgn& iGlobalRgn);

protected:
	vector<ZWindowPane*> fVector_WindowPanes;
	ZEventHr* fWindowTarget;
	vector<ZSubPane*> fVector_IdlePanes;

	ZSubPane* fLastMousePane;
	ZPoint fLastMouseLocation;
	ZSubPane* fLastDragPane;
	ZCaptureInput* fCaptureInput;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowPane

class ZWindowPane : public ZSuperPane
	{
public:
	ZWindowPane(ZFakeWindow* iOwnerWindow, ZPaneLocator* iPaneLocator);
	virtual ~ZWindowPane();

// From ZSubPane
	virtual ZFakeWindow* GetWindow();
	virtual ZPoint GetSize();
	virtual ZPoint GetCumulativeTranslation();
	virtual ZDCInk GetBackInk(const ZDC& iDC);
	virtual ZDCRgn CalcVisibleBoundsRgn();
	virtual ZPoint ToSuper(const ZPoint& iPanePoint);
	virtual ZPoint ToWindow(const ZPoint& iPanePoint);
	virtual ZPoint ToGlobal(const ZPoint& iPanePoint);

protected:
	ZFakeWindow* fFakeWindow;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZCaptureInput

class ZCaptureInput
	{
protected:
	ZCaptureInput(ZSubPane* iPane);
	virtual ~ZCaptureInput();

public:
	void Install();
	void Finish();

	ZSubPane* GetPane() { return fPane; }
	void PaneBeingDeleted();

	void IncrementUseCount();
	void DecrementUseCount();

	void InvalidateCursor();

	virtual void Installed();
	virtual void Removed();
	virtual void Cancelled();

	virtual bool HandleMessage(const ZMessage& iMessage);
	virtual bool GetCursor(ZPoint iPanePoint, ZCursor& oCursor);

protected:
	ZSubPane* fPane;
	long fUseCount;
	};

// =================================================================================================

#endif // __ZFakeWindow__
