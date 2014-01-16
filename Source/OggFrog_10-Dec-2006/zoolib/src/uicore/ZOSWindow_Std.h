/*  @(#) $Id: ZOSWindow_Std.h,v 1.5 2006/04/10 20:18:34 agreen Exp $ */

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

#ifndef __ZOSWindow_Std__
#define __ZOSWindow_Std__ 1
#include "zconfig.h"

#include "ZOSWindow.h"
#include "ZMessage.h"
#include "ZMessageLooperImpStd.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Std

class ZOSWindow_Std : protected ZMessageLooperImpStd, public ZOSWindow
	{
protected:
	ZOSWindow_Std();
	virtual ~ZOSWindow_Std();

// From ZMessageLooperImpStd
	virtual void DispatchMessageImp(const ZMessage& inMessage);

public:
// From ZOSWindow
	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);

	virtual ZShownState GetShownState();
	virtual ZPoint GetLocation();
	virtual ZPoint GetSize();
	virtual ZRect GetFrame();
	virtual ZRect GetFrameNonZoomed();
	virtual void Zoom();
	virtual bool GetActive();
	virtual ZPoint GetOrigin();
	virtual void PoseModal(bool inRunCallerMessageLoopNested,
		bool* outCallerExists, bool* outCalleeExists);
	virtual void EndPoseModal();
	virtual bool WaitingForPoseModal();
	virtual ZPoint ToGlobal(const ZPoint& inWindowPoint);
	virtual bool WaitForMouse();

// Our protocol
	virtual void Internal_Update(const ZDCRgn& inUpdateRgn);

	// Needed for ZDCCanvas::Scroll and CopyBits implementations
	virtual ZDCRgn Internal_GetExcludeRgn() = 0;

	virtual void Internal_TitleIconClick(const ZMessage& inMessage);

	void Internal_RecordIdle();

	void Internal_RecordInvalidation(const ZDCRgn& inBadRgn);
	void Internal_ReportInvalidation();

	void Internal_RecordShownStateChange(ZShownState inNewShownState);
	void Internal_ReportShownStateChange();

	void Internal_RecordActiveChange(bool inNewActive);
	void Internal_ReportActiveChange();
	void Internal_RecordAppActiveChange(bool inNewActive);

	void Internal_RecordSizeChange(ZPoint inNewSize);
	void Internal_RecordLocationChange(ZPoint inNewLocation);
	void Internal_RecordFrameChange(const ZRect& inNewFrame);
	void Internal_ReportFrameChange();

	void Internal_RecordCloseBoxHit();
	void Internal_RecordZoomBoxHit();

	struct MouseState
		{
		ZPoint fLocation;
		bool fContained;
		bool fCaptured;
		bool fButtons[5];
		bool fModifier_Command;
		bool fModifier_Option;
		bool fModifier_Shift;
		bool fModifier_Control;
		bool fModifier_CapsLock;
		};

	void Internal_RecordMouseDown(int inButton, const MouseState& inMouseState, bigtime_t inWhen);
	void Internal_RecordMouseUp(const MouseState& inMouseState, bigtime_t inWhen);
	void Internal_RecordMouseState(const MouseState& inMouseState, bigtime_t inWhen);
	void Internal_TickleMouse(bigtime_t inWhen);

	bool Internal_GetReportedActive(); // Needed for Win32 WM_ERASEBACKGROUND

	ZDCRgn Internal_GetInvalidRgn();

private:
	static void sRunMessageLoop(ZOSWindow_Std* inOSWindow);

// ZMessageLooperImpStd has two mutexes, fMutex_MessageDispatch and fMutex_Structure. Each
// of our instance variables are protected by *one* of the mutexes.
// It is legal, and normal, to acquire fMutex_MessageDispatch (which is what the outside
// world sees as our window lock), and subsequently to acquire fMutex_Structure. Do *not*
// acquire them in the reverse order at any time. In a sense, fMutex_MessageDispatch mediates
// access to the window as a whole. However, in order for it to be possible for a locked window
// to still have messages posted to it by other threads, and to have low level events update
// the window's state, those portions are individually controlled by fMutex_Structure.
// ----------
// First group protected by fMutex_MessageDispatch
private:
	ZMessageLooperImpStd* fPoseModal_CallerLooper;
	bool* fPoseModal_CallerLooperRunNestedMessageLoop;
	ZSemaphore* fPoseModal_Semaphore;
	bool* fPoseModal_CalleeExistsAddress;

// ----------
// Second group protected by fMutex_Structure
private:
	bool fPendingIdle;

	ZDCRgn fInvalidRgn;

	ZShownState fRecordedShownState;
	ZShownState fReportedShownState;
	bool fPendingShownState;

	bool fRecordedActive;
	bool fReportedActive;
	bool fPendingActive;

	bool fRecordedAppActive;
	bool fReportedAppActive;
	bool fPendingAppActive;

	ZRect fRecordedFrame;
	ZRect fReportedFrame;
	bool fPendingFrame;

	ZRect fPreZoomFrame;
	bool fPreZoomFrameKnown;

	bool fPendingCloseBoxHit;
	bool fPendingZoomBoxHit;

// Mouse state
	MouseState fRecordedMouseState;
	MouseState fReportedMouseState;

	bigtime_t fRecordedMouseStateWhen;
	bigtime_t fReportedMouseStateWhen;
	bool fPendingMouseState;

	long fClickCount;
	ZPoint fClickLocation;
	bigtime_t fTimeOfLastClick;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Std

class ZOSWindowRoster_Std;

class ZOSApp_Std : protected ZMessageLooperImpStd, public ZOSApp
	{
protected:
	ZOSApp_Std();
	virtual ~ZOSApp_Std();

// From ZMessageLooperImpStd
	virtual void DispatchMessageImp(const ZMessage& inMessage);

public:
// From ZOSApp
	virtual ZMutexBase& GetLock();
	virtual void QueueMessage(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void Run();
	virtual bool IsRunning();
	virtual void ExitRun();
	virtual ZOSWindowRoster* CreateOSWindowRoster();

// Our protocol
	void Internal_RecordIdle();

	void Internal_RecordWindowRosterChange();

	void Internal_OSWindowRemoved(ZOSWindow_Std* inOSWindow);
	virtual bool Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
		const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
		bigtime_t inTimeout, ZOSWindow_Std*& outWindow) = 0;

private:
	ZOSWindowRoster_Std* fOSWindowRoster_Head;
	friend class ZOSWindowRoster_Std;

	bool fPendingIdle;
	bool fPendingWindowRosterChange;
	};

#endif // __ZOSWindow_Std__
