/*  @(#) $Id: ZOSWindow_Carbon.h,v 1.11 2006/10/13 20:34:03 agreen Exp $ */

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

#ifndef __ZOSWindow_Carbon__
#define __ZOSWindow_Carbon__ 1
#include "zconfig.h"

#if ZCONFIG(API_OSWindow, Carbon)

#include "ZOSWindow.h"
#include "ZMenu.h"

#if ZCONFIG(OS, MacOSX)
#	include <HIToolbox/CarbonEvents.h>
#	include <AE/AEDataModel.h>
#else
#	include <CarbonEvents.h>
#	include <AEDataModel.h>
#endif
//#include <Drag.h>

#include <set>

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Carbon

class ZOSApp_Carbon;
class ZDCCanvas_QD_CarbonWindow;

class ZOSWindow_Carbon : public ZOSWindow
	{
	friend class ZOSApp_Carbon;
public:
	static WindowRef sCreateWindowRef(const ZOSWindow::CreationAttributes& iAttributes);

	ZOSWindow_Carbon(WindowRef iWindowRef);
	virtual ~ZOSWindow_Carbon();

// From ZOSWindow
	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& iMessage);
	virtual bool DispatchMessage(const ZMessage& iMessage);

	virtual void SetShownState(ZShownState iState);
	virtual ZShownState GetShownState();

	virtual void SetTitle(const string& iTitle);
	virtual string GetTitle();

	virtual void SetCursor(const ZCursor& iCursor);

	virtual void SetLocation(ZPoint iLocation);
	virtual ZPoint GetLocation();

	virtual void SetSize(ZPoint newSize);
	virtual ZPoint GetSize();

	virtual void SetFrame(const ZRect& iFrame);

	virtual void SetSizeLimits(ZPoint iMinSize, ZPoint iMaxSize);

	virtual void PoseModal(bool iRunCallerMessageLoopNested, bool* oCallerExists,
		bool* oCalleeExists);
	virtual void EndPoseModal();
	virtual bool WaitingForPoseModal();

	virtual void BringFront(bool iMakeVisible);

	virtual bool GetActive();

	virtual void GetContentInsets(ZPoint& oTopLeftInset, ZPoint& oBottomRightInset);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual ZPoint ToGlobal(const ZPoint& iWindowPoint);

	virtual void InvalidateRegion(const ZDCRgn& iBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();
	
	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void GetNative(void* oNative);

	virtual void SetMenuBar(const ZMenuBar& iMenuBar);

	virtual int32 GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone);
	virtual int32 GetScreenIndex(const ZRect& inGlobalRect,  bool inReturnMainIfNone);
	virtual int32 GetMainScreenIndex();
	virtual int32 GetScreenCount();
	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly);
	virtual ZDCRgn GetDesktopRegion(bool inUsableSpaceOnly);

protected:
	void InvalSizeBox();

	ZDCRgn Internal_GetExcludeRgn();
	void FinalizeCanvas(ZDCCanvas_QD_CarbonWindow* iCanvas);

	static EventHandlerUPP sEventHandlerUPP_Window;
	static pascal OSStatus sEventHandler_Window(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon);
	OSStatus EventHandler_Window(EventHandlerCallRef iCallRef, EventRef iEvent);

	static EventHandlerUPP sEventHandlerUPP_Content;
	static pascal OSStatus sEventHandler_Content(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon);
	OSStatus EventHandler_Content(EventHandlerCallRef iCallRef, EventRef iEvent);

	static EventLoopTimerUPP sEventLoopTimerUPP_Idle;
	static pascal void sEventLoopTimer_Idle(EventLoopTimerRef iTimer, void* iRefcon);
	void EventLoopTimer_Idle(EventLoopTimerRef iTimer);

	ZMutex fMutex;

	#if ZCONFIG(OS, MacOSX)
		bool fCompositing;
		ZDCRgn fInvalidRgn;
	#endif

	WindowRef fWindowRef;
	EventTargetRef fEventTargetRef_Window;
	EventHandlerRef fEventHandlerRef_Window;

	EventHandlerRef fEventHandlerRef_Content;

    EventLoopTimerRef fEventLoopTimerRef_Idle;

	ZDCCanvas_QD_CarbonWindow* fCanvas;
	friend class ZDCCanvas_QD_CarbonWindow;

	ZMenuBar fMenuBar;
	CCrsrHandle fCCrsrHandle;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Carbon

class ZOSApp_Carbon : public ZOSApp
	{
	friend class ZOSWindow_Carbon;
public:
	static ZOSApp_Carbon* sGet();

	ZOSApp_Carbon();
	virtual ~ZOSApp_Carbon();

// From ZOSApp
	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& iMessage);
	virtual bool DispatchMessage(const ZMessage& iMessage);

	virtual void Run();
	virtual bool IsRunning();
	virtual void ExitRun();

	virtual string GetAppName();

	virtual bool HasUserAccessibleWindows();

	virtual bool HasGlobalMenuBar();
	virtual void SetMenuBar(const ZMenuBar& iMenuBar);

	virtual void BroadcastMessageToAllWindows(const ZMessage& iMessage);

	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& iAttributes);

	virtual ZOSWindowRoster* CreateOSWindowRoster();

// Our protocol
	ZOSWindow_Carbon* GetActiveOSWindow();

protected:
// Called by ZOSWindow_Carbon
	void Disposed(ZOSWindow_Carbon* iOSWindow);

	void AcquireCapture(WindowRef iWindowRef);
	void ReleaseCapture(WindowRef iWindowRef);
	void CancelCapture();
	bool IsCaptured(WindowRef iWindowRef);

	void MenuBarChanged(ZOSWindow_Carbon* iWindow);

	ZOSWindow_Carbon* pGetActiveWindow();
	void pSetMenuBar(const ZMenuBar& iMenuBar);
	
// Event handlers
	static EventHandlerUPP sEventHandlerUPP_Dispatcher;
	static pascal OSStatus sEventHandler_Dispatcher(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon);
	OSStatus EventHandler_Dispatcher(EventHandlerCallRef iCallRef, EventRef iEvent);

	static EventHandlerUPP sEventHandlerUPP_Application;
	static pascal OSStatus sEventHandler_Application(EventHandlerCallRef iCallRef, EventRef iEvent, void* iRefcon);
	OSStatus EventHandler_Application(EventHandlerCallRef iCallRef, EventRef iEvent);

	static AEEventHandlerUPP sAppleEventHandlerUPP;
	static DEFINE_API(OSErr) sAppleEventHandler(const AppleEvent* iMessage, AppleEvent* oReply, long iRefcon);
	OSErr AppleEventHandler(const AppleEvent* iMessage, AppleEvent* oReply, unsigned long iRefcon);

	static EventLoopTimerUPP sEventLoopTimerUPP_Idle;
	static pascal void sEventLoopTimer_Idle(EventLoopTimerRef iTimer, void* iRefcon);
	void EventLoopTimer_Idle(EventLoopTimerRef iTimer);

//
	ZMutex fMutex;
	bool fIsRunning;
	bool fPostedRunStarted;

	EventHandlerRef fEventHandlerRef_Dispatcher;

	EventHandlerRef fEventHandlerRef_Application;

    EventLoopTimerRef fEventLoopTimerRef_Idle;

	ZMenuBar fMenuBar;

	WindowRef fWindowRef_Capture;

	short fAppResFile;
	FSSpec fAppFSSpec;
	static ZOSApp_Carbon* sOSApp_Carbon;

	// OSWindowRoster stuff
	class OSWindowRoster;
	friend class OSWindowRoster;
	OSWindowRoster* fOSWindowRoster_Head;

	void OSWindowRoster_Disposed(OSWindowRoster* iRoster);
	void OSWindowRoster_Reset(OSWindowRoster* iRoster);
	bool OSWindowRoster_GetNextOSWindow(OSWindowRoster* iRoster, bigtime_t iTimeout, ZOSWindow*& oOSWindow);
	void OSWindowRoster_DropCurrentOSWindow(OSWindowRoster* iRoster);
	void OSWindowRoster_UnlockCurrentOSWindow(OSWindowRoster* iRoster);
	};

// =================================================================================================
#endif // ZCONFIG(API_OSWindow, Carbon)

#endif // __ZOSWindow_Carbon__
