/*  @(#) $Id: ZOSWindow_Mac.h,v 1.6 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZOSWindow_Mac__
#define __ZOSWindow_Mac__ 1
#include "zconfig.h"

#if ZCONFIG(API_OSWindow, Mac)
#include "ZOSWindow_Std.h"
#include "ZMenu.h"

#include <AEDataModel.h>
#include <Drag.h>

// By defining this a zlib user can change the window kind we're using -- gotta be careful
// that the right number of bits are used -- the bottom 4 bits are used for other purposes

#ifndef kZOSWindow_MacKind
#	define kZOSWindow_MacKind 160
#endif

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Mac

class ZOSApp_Mac;
class ZDCCanvas_QD_Window;

class ZOSWindow_Mac : public ZOSWindow_Std
	{
	friend class ZOSApp_Mac;

public:
	ZOSWindow_Mac(WindowRef inWindowRef);
	virtual ~ZOSWindow_Mac();

// From ZOSWindow via ZOSWIndow_Std
	virtual bool DispatchMessage(const ZMessage& inMessage);

	virtual void SetShownState(ZShownState inState);

	virtual void SetTitle(const string& inTitle);
	virtual string GetTitle();

	virtual void SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap, const ZDCPixmap& inMaskPixmap);
	virtual ZCoord GetTitleIconPreferredHeight();

	virtual void SetCursor(const ZCursor& inCursor);

	virtual void SetLocation(ZPoint inLocation);

	virtual void SetSize(ZPoint inSize);

	virtual void SetFrame(const ZRect& inFrame);

	virtual void SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize);

	virtual void Zoom();

	virtual void ForceOnScreen(bool inEnsureTitleBarAccessible, bool inEnsureSizeBoxAccessible);

	virtual void BringFront(bool inMakeVisible);

	virtual void GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();

	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void GetNative(void* outNative);

	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

	virtual ZDragInitiator* CreateDragInitiator();

	virtual int32 GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone);
	virtual int32 GetScreenIndex(const ZRect& inGlobalRect,  bool inReturnMainIfNone);
	virtual int32 GetMainScreenIndex();
	virtual int32 GetScreenCount();
	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly);
	virtual ZDCRgn GetDesktopRegion(bool inUsableSpaceOnly);

// From ZOSWindow_Std
	virtual ZDCRgn Internal_GetExcludeRgn();
	virtual void Internal_TitleIconClick(const ZMessage& inMessage);

// Our protocol
	void FinalizeCanvas(ZDCCanvas_QD_Window* inCanvas);

	void Internal_SetLocation(ZPoint inLocation);
	void Internal_SetSize(ZPoint inSize);

// Utility methods and internal implementation
	enum { kDummy, kSinker, kDocument, kFloater, kHiddenFloater, kDialog, kMenu, kHelp };
	enum { kKindBits = 0xFFF0, kCallDrawGrowIconBit = 0x8, kLayerBits = 0x7 };

	static short sCalcWindowKind(Layer inLayer, bool inCallDrawGrowIcon);
	static void sCalcWindowRefParams(const ZOSWindow::CreationAttributes& inAttributes, short& outProcID, bool& outCallDrawGrowIcon);
	static WindowRef sCreateWindowRef(const ZRect& inInitialFrame, short inProcID, ZOSWindow::Layer inWindowLayer, bool inCallDrawGrowIcon, bool inHasCloseBox);
	static WindowRef sGetBehindPtr(Layer inLayer);
	static ZOSWindow::Layer sGetLayer(WindowRef inWindowRef);
	static ZOSWindow_Mac* sFromWindowRef(WindowRef inWindowRef);

	static ZPoint sFromGlobal(ZOSWindow_Mac* inWindow, ZPoint inGlobalPoint);

protected:
	WindowRef fWindowRef;
	bool fWindowActive;
	ZCursor fCursor;
	ZMenuBar fMenuBar;

	ZDCCanvas_QD_Window* fCanvas;
	friend class ZDCCanvas_QD_Window;

	bool fForEachCalled;

	ZPoint fSize_Min;
	ZPoint fSize_Max;

	class DragInitiator;
	class DragInfo;
	class DragDropImpl;
	class Drag;
	class Drop;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Mac

class ZOSApp_Mac : public ZOSApp_Std
	{
public:
	static ZOSApp_Mac* sGet();

	ZOSApp_Mac();
	virtual ~ZOSApp_Mac();

// From ZOSApp via ZOSApp_Std
	virtual bool DispatchMessage(const ZMessage& inMessage);

	virtual string GetAppName();

	virtual bool HasUserAccessibleWindows();

	virtual bool HasGlobalMenuBar();
	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

	virtual void BroadcastMessageToAllWindows(const ZMessage& inMessage);

	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes);

// From ZOSApp_Std
	virtual bool Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
												const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
												bigtime_t inTimeout, ZOSWindow_Std*& outWindow);

// Our protocol
// Window management and OS event loop -- this is all internal, but marking it off as protected
// would make creating interesting derivations of ZOSWindow_Mac much harder.
	ZOSWindow_Mac* GetActiveOSWindow();

	void AddOSWindow(ZOSWindow_Mac* inOSWindow);
	void RemoveOSWindow(ZOSWindow_Mac* inOSWindow);

	void RunEventLoop();
	static void sRunEventLoop(void* inOSApp_Mac);

	void DispatchEvent(const EventRecord& inEvent);
	void UpdateAllWindows();

	void DispatchMouseDown(const EventRecord& inEvent, WindowRef initWindowRef, short inWindowPart);
	void DispatchMouseUp(const EventRecord& inEvent, WindowRef inHitWindowRef, short inWindowPart);
	void DispatchUpdate(const EventRecord& inEvent);
	void DispatchIdle(const EventRecord& inEvent);
	void DispatchSuspendResume(const EventRecord& inEvent);

	void HandleDragWindow(WindowRef inWindowRef, ZPoint inHitPoint, bool inBringFront);

	void ShowWindow(ZOSWindow_Mac* inOSWindow, bool inShowOrHide);
	void BringWindowFront(WindowRef inWindowRefToMove, bool inMakeVisible);
	void SendWindowBack(WindowRef inWindowRefToMove);

	bool IsFrontProcess();
	bool IsInBackground();
	bool IsTopModal();

	void PreDialog();
	void PostDialog();

	void SendWindowBehind(WindowRef windowToMove, WindowRef otherWindow);
	void FixupWindows();

	void AcquireCapture(ZOSWindow_Mac* inAcquireWindow);
	void ReleaseCapture(ZOSWindow_Mac* inReleaseWindow);
	void CancelCapture();

	void CursorChanged(ZOSWindow_Mac* inWindow);
	void MenuBarChanged(ZOSWindow_Mac* inWindow);
	ZRef<ZMenuItem> PostSetupAndHandleMenuBarClick(const ZMenuBar& inMenuBar, ZPoint inHitPoint);
	ZRef<ZMenuItem> PostSetupAndHandleCammandKey(const ZMenuBar& inMenuBar, const EventRecord& inEventRecord);

protected:
	ZOSWindow_Mac* Internal_GetActiveWindow();
	void Internal_SetCursor(const ZCursor& inCursor);
	void Internal_SetMenuBar(const ZMenuBar& inMenuBar);
	void Internal_RecordMouseDown(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse);
	void Internal_RecordMouseUp(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse);
	void Internal_RecordMouseContainment(const EventRecord& inEvent, ZOSWindow_Mac* inWindow_ContainingMouse, short inHitPart);
	void Internal_SetShown(WindowRef inWindowRef, bool inShown);
	void Internal_SetActive(WindowRef inWindowRef, bool inActive);
	void Internal_FixupWindows(WindowRef windowToShow, WindowRef windowToHide);

// AppleEvents
	static DEFINE_API(OSErr) sAppleEventHandler(const AppleEvent* inMessage, AppleEvent* outReply, long inRefCon);
	OSErr AppleEventHandler(const AppleEvent* inMessage, AppleEvent* outReply, unsigned long inRefCon);

// Drag and drop support
	static DEFINE_API(OSErr) sDragTrackingHandler(DragTrackingMessage inMessage, WindowRef inWindowRef, void* inRefCon, DragReference inDragReference);
	virtual OSErr DragTrackingHandler(DragTrackingMessage inMessage, WindowRef inWindowRef, void* inRefCon, DragReference inDragReference);

	static DEFINE_API(OSErr) sDragReceiveHandler(WindowRef inWindowRef, void* inRefCon, DragReference inDragReference);
	virtual OSErr DragReceiveHandler(WindowRef inWindowRef, void* inRefCon, DragReference inDragReference);

// Idling
	void IdleThread();
	static void sIdleThread(ZOSApp_Mac* inOSApp);

	ZSemaphore* fSemaphore_IdleExited;

	ZSemaphore* fSemaphore_EventLoopExited;

	bool fSinkerLayerActive;
	WindowRef fWindowRef_Dummy;
	WindowRef fWindowRef_DummyDialog;
	long fCount_DummyDialog;

	CCrsrHandle fCCrsrHandle;

	size_t fAppleMenuItemCount;

	ZOSWindow_Mac* fWindow_Active;
	ZOSWindow_Mac* fWindow_Capture;
	ZOSWindow_Mac* fWindow_ContainingMouse;
	ZOSWindow_Mac* fWindow_LiveResize;
	ZOSWindow_Mac* fWindow_LiveDrag;
	ZOSWindow_Mac* fWindow_SetCursor;
	bool fWindow_SetCursor_WasBusy;

	ZPoint fLocation_LiveResizeOrDrag;
	ZPoint fSize_LiveResize;

	bool fDragActive;
	bool fDropOccurred;
	ZPoint fLocation_LastDragDrop;

	ZMenuBar fMenuBar;

	bool fPostedRunStarted;

protected:
	short fAppResFile;
	FSSpec fAppFSSpec;
	static ZOSApp_Mac* sOSApp_Mac;
	};

// =================================================================================================
#endif // ZCONFIG(API_OSWindow, Mac)

#endif // __ZOSWindow_Mac__
