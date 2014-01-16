/*  @(#) $Id: ZOSWindow_Be.h,v 1.5 2006/04/10 20:44:20 agreen Exp $ */

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

#ifndef __ZOSWindow_Be__
#define __ZOSWindow_Be__ 1
#include "zconfig.h"

#if ZCONFIG(API_OSWindow, Be)
#include "ZOSWindow.h"
#include "ZMenu.h"

#include <vector>

#include <interface/Window.h>
#include <app/Application.h>
#include <deque>

class BMenuBar;

class ZBMessageHelper;
class ZMutex_BLooper;
class ZMenuBar_Be;
class ZDCCanvas_Be_Window;

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Be

class ZOSWindow_Be : public ZOSWindow
	{
public:
	ZOSWindow_Be(const ZOSWindow::CreationAttributes& inAttributes);
	virtual ~ZOSWindow_Be();

// From ZOSWindow
	virtual ZMutexBase& GetLock();

	virtual void QueueMessage(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);

	virtual void SetShownState(ZShownState inState);
	virtual ZShownState GetShownState();

	virtual void SetTitle(const string& inTitle);
	virtual string GetTitle();

	virtual void SetCursor(const ZCursor& inCursor);

	virtual void SetLocation(ZPoint inLocation);
	virtual ZPoint GetLocation();

	virtual void SetSize(ZPoint inNewSize);
	virtual ZPoint GetSize();

	virtual void SetFrame(const ZRect& inFrame);

	virtual void SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize);

	virtual void PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists, bool* outCalleeExists);
	virtual void EndPoseModal();
	virtual bool WaitingForPoseModal();

	virtual void Zoom();

	virtual void BringFront(bool inMakeVisible);

	virtual bool GetActive();

	virtual void GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual ZPoint ToGlobal(const ZPoint& inWindowPoint);

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();

	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void GetNative(void* outNative);

	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

	virtual ZDragInitiator* CreateDragInitiator();

	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly);

// Our protocol
	void Internal_RecordIdle();

	static ZOSWindow_Be* sFromBWindow(BWindow* inBWindow);
	BWindow* GetBWindow();

// Calls forwarded from our BWindow
	void BWindow_DispatchMessage(BMessage* inMessage, BHandler* inTarget);
	void BWindow_MessageReceived(BMessage* inMessage);
	void BWindow_MenusBeginning();

// Calls forwarded from our BView
	void BView_Draw(BRect inUpdateRect);
	void BView_FrameMoved(BPoint inNewPosition);
	void BView_FrameResized(float inNewWidth, float inNewHeight);
	void BView_MouseDown(BPoint inPoint);
	void BView_MouseUp(BPoint inPoint);
	void BView_MouseMoved(BPoint inPoint, uint32 inTransit, const BMessage* inBMessage);
	void BView_KeyDown(const char* inBytes, int32 inNumBytes);
	void BView_WindowActivated(bool inActive);

protected:
// ----------
	ZMutex_BLooper* fMutex_Looper;

// ----------
	ZMutex fMutex_Structure;
	ZBMessageHelper* fMessageHelper;

	bool fPendingIdle;

	ZMenuBar fMenuBar;
	BMenuBar* fRealMenuBar;

	ZRef<ZDCCanvas_Be_Window> fCanvas;

	ZCursor fCursor;

	bool fCapturedMouse;
	bool fContainsMouse;

	class RealWindow;
	friend class RealWindow;
	RealWindow* fRealWindow;

	class RealView;
	friend class RealView;
	RealView* fRealView;

	friend class ZOSApp_Be;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Be

class ZOSWindowRoster_Be;

class ZOSApp_Be : public ZOSApp
	{
public:
	static ZOSApp_Be* sGet();

	ZOSApp_Be(const char* inSignature);
	virtual ~ZOSApp_Be();

// From ZOSApp
	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void Run();
	virtual bool IsRunning();
	virtual void ExitRun();

	virtual string GetAppName();

	virtual bool HasUserAccessibleWindows();

	virtual bool HasGlobalMenuBar();

	virtual void BroadcastMessageToAllWindows(const ZMessage& inMessage);

	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes);

	virtual ZOSWindowRoster* CreateOSWindowRoster();

// Our protocol
	void AddOSWindow(ZOSWindow_Be* inOSWindow);
	void RemoveOSWindow(ZOSWindow_Be* inOSWindow);

	void Internal_RecordIdle();

	void Internal_RecordWindowRosterChange();

// Calls forwarded from our BApp
	void BApp_DispatchMessage(BMessage* inMessage, BHandler* inTarget);

protected:
	void IdleThread();
	static void sIdleThread(ZOSApp_Be* inOSApp);

// ----------
	ZMutex_BLooper* fMutex_Looper;

// ----------
	ZMutex fMutex_Structure;
	ZBMessageHelper* fMessageHelper;
	ZCondition fCondition_Structure;

	ZOSWindowRoster_Be* fOSWindowRoster_Head;
	friend class ZOSWindowRoster_Be;

	vector<ZOSWindow_Be*> fVector_Windows;

	bool fIsRunning;

	bool fPendingIdle;
	bool fPendingWindowRosterChange;

	ZSemaphore* fSemaphore_IdleExited;

	class RealApp;
	friend class RealApp;
	RealApp* fRealApp;

	static ZOSApp_Be* sOSApp_Be;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMutex_BLooper

class ZMutex_BLooper : public ZMutexBase
	{
public:
	ZMutex_BLooper(BLooper* inLooper);
	virtual ~ZMutex_BLooper();

// From ZMutexBase
	virtual ZThread::Error Acquire();
	virtual ZThread::Error Acquire(bigtime_t inMicroseconds);
	virtual void Release();
	virtual bool IsLocked();

	virtual string* CheckForDeadlockImp(ZThread* iAcquiringThread);
protected:
	BLooper* fLooper;
	};


#endif // ZCONFIG(API_OSWindow, Be)

#endif // __ZOSWindow_Be__
