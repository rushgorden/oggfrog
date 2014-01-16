/*  @(#) $Id: ZWindow.h,v 1.6 2006/07/17 18:23:15 agreen Exp $ */

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

#ifndef __ZWindow__
#define __ZWindow__ 1
#include "zconfig.h"

#include "ZFakeWindow.h"
#include "ZOSWindow.h"
#include "ZUIInk.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowSupervisor

class ZWindowSupervisor
	{
public:
	virtual void WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall);
	virtual void WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup);

	virtual void WindowSupervisorPostMessage(const ZMessage& inMessage) = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZWindow

class ZWindowWatcher;

class ZWindow : protected ZOSWindowOwner,
			public ZMessageLooper,
			public ZMessageReceiver,
			public ZFakeWindow
	{
public:
	ZWindow(ZWindowSupervisor* inWindowSupervisor, ZOSWindow* inOSWindow);
	virtual ~ZWindow();

// From ZMessageLooper
	virtual ZMutexBase& GetLock();
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void QueueMessage(const ZMessage& inMessage);

// From ZMessageReceiver
	virtual void ReceivedMessage(const ZMessage& inMessage);

// Essential ZFakeWindow overrides
	virtual ZMutexBase& GetWindowLock();
	virtual ZPoint GetWindowOrigin();
	virtual ZPoint GetWindowSize();
	virtual ZDCRgn GetWindowVisibleContentRgn();
	virtual void InvalidateWindowRegion(const ZDCRgn& inWindowRegion);
	virtual ZPoint WindowToGlobal(const ZPoint& inWindowPoint);
	virtual void UpdateWindowNow();
	virtual ZRef<ZDCCanvas> GetWindowCanvas();
	virtual void GetWindowNative(void* outNativeWindow);
	virtual void InvalidateWindowCursor();

// Other non-essential ZFakeWindow overrides
	virtual void InvalidateWindowMenuBar();
	virtual ZDCInk GetWindowBackInk();
	virtual bool GetWindowActive();
	virtual void BringFront(bool inMakeVisible = true);
	virtual ZDragInitiator* CreateDragInitiator();
	virtual bool WaitForMouse();
	virtual void WindowCaptureAcquire();
	virtual void WindowCaptureRelease();

// From ZEventHr via ZFakeWindow
	virtual void HandleInstallMenus(ZMenuInstall& inMenuInstall, ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual void HandleSetupMenus(ZMenuSetup& inMenuSetup, ZEventHr* inOriginalTarget, bool inFirstCall);
	virtual bool HandleMenuMessage(const ZMessage& inMessage, ZEventHr* inOriginalTarget, bool inFirstCall);

// Our protocol
	virtual bool WindowQuitRequested();

	virtual void WindowShownState(ZShownState inShownState);

	virtual void WindowAppActivate(bool inActive);

	virtual void WindowCloseByUser();
	virtual void WindowZoomByUser();

	virtual void WindowFrameChange();

	virtual bool WindowMenuMessage(const ZMessage& inMessage);

	typedef void (*MessageProc)(ZWindow* inWindow, const ZTupleValue& inTV);
	void InvokeFunctionFromMessageLoop(MessageProc inProc, const ZTupleValue& inTV);

	void CloseAndDispose();

	void Show(bool inShown);
	bool GetShown();

	void SetShownState(ZShownState inState);
	ZShownState GetShownState();

	void SetTitle(const string& inTitle);
	string GetTitle() const;

	void SetLocation(ZPoint inLocation);
	ZPoint GetLocation() const;

	void SetSize(ZPoint newSize);
	ZPoint GetSize() const;

	void SetFrame(const ZRect& inFrame);
	ZRect GetFrame();
	ZRect GetFrameNonZoomed();

	void SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize);

	void PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists);
	void EndPoseModal();
	bool WaitingForPoseModal();

	void Center();
	void CenterDialog();

	void Zoom();

	void ForceOnScreen(bool ensureTitleBarAccessible = true, bool ensureSizeBoxAccessible = true);

	void SetBackInks(const ZRef<ZUIInk>& inBackInkActive, const ZRef<ZUIInk>& inBackInkInactive);
	void SetBackInk(const ZRef<ZUIInk>& inBackInk);

	void SetTitleIcon(const ZDCPixmapCombo& inPixmapCombo);
	void SetTitleIcon(const ZDCPixmap& colorPixmap, const ZDCPixmap& monoPixmap, const ZDCPixmap& maskPixmap);
	ZCoord GetTitleIconPreferredHeight();

// Open up access to GetOSWindow only.
	ZOSWindow* GetOSWindow()
		{ return ZOSWindowOwner::GetOSWindow(); }

protected:
	ZWindowSupervisor* fWindowSupervisor;

	ZPoint fLastKnownWindowSize;

	bool fPostedCursorInvalid;
	bool fPostedInvalidateMenuBar;

	ZRef<ZUIInk> fBackInkActive;
	ZRef<ZUIInk> fBackInkInactive;

	friend class ZWindowWatcher;
	ZWindowWatcher* fWatcher_Head;

protected:
// From ZOSWindowOwner. Note that our first base class is ZOSWindowOwner, but we use protected inheritance
// in order that no one (externally) will depend on ZWindow being a ZOSWindowOwner, and have placed these
// methods at the end of this declaration in order that they not appear too obvious and tempting.
	virtual void OwnerDispatchMessage(const ZMessage& inMessage);

	virtual void OwnerUpdate(const ZDC& inDC);

	virtual void OwnerDrag(ZPoint inWindowLocation, ZMouse::Motion inMotion, ZDrag& inDrag);
	virtual void OwnerDrop(ZPoint inWindowLocation, ZDrop& inDrop);

	virtual void OwnerSetupMenus(ZMenuSetup& inMenuSetup);

	virtual ZDCInk OwnerGetBackInk(bool inActive);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZWindowWatcher

class ZWindowWatcher
	{
	ZWindowWatcher(); // Not implemented
	ZWindowWatcher(const ZWindowWatcher& other); // Not implemented
	ZWindowWatcher& operator=(const ZWindowWatcher& inOther); // Not implemented

public:
	ZWindowWatcher(ZWindow* inWindow);
	~ZWindowWatcher();

	bool IsValid();

protected:
	ZWindowWatcher* fPrev;
	ZWindowWatcher* fNext;
	ZWindow* fWindow;
	friend class ZWindow;
	};

// =================================================================================================

#endif // __ZWindow__
