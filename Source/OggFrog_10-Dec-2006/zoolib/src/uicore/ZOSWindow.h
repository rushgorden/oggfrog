/*  @(#) $Id: ZOSWindow.h,v 1.8 2006/07/23 22:04:58 agreen Exp $ */

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

#ifndef __ZOSWindow__
#define __ZOSWindow__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZCursor.h"
#include "ZDC.h"
#include "ZDCRgn.h"
#include "ZEvent.h"
#include "ZGeom.h"
#include "ZKeyboard.h"
#include "ZMessage.h"
#include "ZRGBColor.h"
#include "ZTypes.h"

class ZDCPixmap;
class ZDragInitiator;
class ZDrag;
class ZDrop;
class ZMenuSetup;
class ZMenuBar;
class ZMutexBase;

enum EZShownState { eZShownStateHidden, eZShownStateMinimized, eZShownStateShown };
typedef EZShownState ZShownState;

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow

class ZOSWindowOwner;

class ZOSWindow
	{
public:
	enum Layer { layerDummy, layerSinker, layerDocument, layerFloater,
		layerDialog, layerMenu, layerHelp,
		layerBottommost = layerDummy, layerTopmost = layerHelp };

	enum Look { lookDocument, lookPalette, lookModal, lookMovableModal, lookAlert,
		lookMovableAlert, lookThinBorder, lookMenu, lookHelp };

	struct CreationAttributes
		{
		CreationAttributes();
		explicit CreationAttributes(const ZRect& inFrame);

		ZRect fFrame;
		Look fLook;
		Layer fLayer;
		bool fResizable;
		bool fHasSizeBox;
		bool fHasCloseBox;
		bool fHasZoomBox;
		bool fHasMenuBar;
		bool fHasTitleIcon;
		};

protected:
	ZOSWindow();
	virtual ~ZOSWindow();

public:
	void SetOwner(ZOSWindowOwner* inOwner);
	ZOSWindowOwner* GetOwner()
		{ return fOwner; }
	void OwnerDisposed(ZOSWindowOwner* inOwner);

	virtual ZMutexBase& GetLock() = 0;
	virtual void QueueMessage(const ZMessage& inMessage) = 0;
	void QueueMessageForOwner(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);
	void DispatchMessageToOwner(const ZMessage& iMessage);

	virtual void SetShownState(ZShownState inState) = 0;
	virtual ZShownState GetShownState() = 0;

	virtual void SetTitle(const string& inTitle) = 0;
	virtual string GetTitle() = 0;

	virtual void SetTitleIcon(const ZDCPixmap& inColorPixmap, const ZDCPixmap& inMonoPixmap,
		const ZDCPixmap& inMaskPixmap);
	virtual ZCoord GetTitleIconPreferredHeight();

	virtual void SetCursor(const ZCursor& inCursor) = 0;

	virtual void SetLocation(ZPoint inLocation) = 0;
	virtual ZPoint GetLocation() = 0;

	virtual void SetSize(ZPoint newSize) = 0;
	virtual ZPoint GetSize() = 0;

	virtual void SetFrame(const ZRect& inFrame) = 0;
	virtual ZRect GetFrame();

	virtual ZRect GetFrameNonZoomed();

	virtual void SetSizeLimits(ZPoint inMinSize, ZPoint inMaxSize) = 0;

	virtual void PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists,
		bool* outCalleeExists) = 0;
	virtual void EndPoseModal() = 0;
	virtual bool WaitingForPoseModal() = 0;

	virtual void Center();
	virtual void CenterDialog();

	virtual void Zoom();

	virtual void ForceOnScreen(bool inEnsureTitleBarAccessible, bool inEnsureSizeBoxAccessible);

	virtual void BringFront(bool inMakeVisible) = 0;

	virtual bool GetActive() = 0;

	virtual void GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset) = 0;
	ZPoint GetContentTopLeftInset();

	virtual ZPoint GetOrigin();

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn() = 0;

	virtual ZPoint ToGlobal(const ZPoint& inWindowPoint) = 0;
	ZPoint FromGlobal(const ZPoint& inGlobalPoint);

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn) = 0;

	virtual void UpdateNow() = 0;

	virtual ZRef<ZDCCanvas> GetCanvas() = 0;

	virtual void AcquireCapture() = 0;
	virtual void ReleaseCapture() = 0;

	virtual void GetNative(void* outNative) = 0;

	virtual void SetMenuBar(const ZMenuBar& inMenuBar) = 0;

	virtual ZDragInitiator* CreateDragInitiator();

	virtual bool WaitForMouse();

// AG 2000-07-14. I'm not in love with this API. It does not address workspace/desktop issues, and
// its notion of what constitutes a screen only makes sense on MacOS and Win32 (with the Memphis
// multi-screen extensions). Expect this API to change radically at some time in the future (ie
// when UNIX/BeOS people start screaming).
	virtual int32 GetScreenIndex(const ZPoint& inGlobalLocation, bool inReturnMainIfNone);
	virtual int32 GetScreenIndex(const ZRect& inGlobalRect,  bool inReturnMainIfNone);
	virtual int32 GetMainScreenIndex();
	virtual int32 GetScreenCount();
	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly) = 0;
	virtual ZDCRgn GetDesktopRegion(bool inUsableSpaceOnly);

// This is a shortcut method, and hence not overridable.
	ZRect GetMainScreenFrame(bool inUsableSpaceOnly);

// Do not predicate any application code on SetDebugLabel -- it's here
// strictly to aid in debuggng and may well disappear in the future.
	void SetDebugLabel(const string& inDebugLabel)
		{ fDebugLabel = inDebugLabel; }

protected:
	string fDebugLabel;

	ZOSWindowOwner* fOwner;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindowRoster

class ZOSWindowRoster : ZooLib::NonCopyable
	{
public:
	virtual ~ZOSWindowRoster() {}

	virtual void Reset() = 0;
	virtual bool GetNextOSWindow(bigtime_t inTimeout, ZOSWindow*& outOSWindow) = 0;
	virtual void DropCurrentOSWindow() = 0;
	virtual void UnlockCurrentOSWindow() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindowOwner

class ZOSWindowOwner
	{
protected:
	ZOSWindowOwner(ZOSWindow* inOSWindow);
	virtual ~ZOSWindowOwner();

public:
	virtual void OwnerDispatchMessage(const ZMessage& inMessage);

	virtual void OwnerUpdate(const ZDC& inDC);

	virtual void OwnerDrag(ZPoint inWindowLocation, ZMouse::Motion inMotion, ZDrag& inDrag);
	virtual void OwnerDrop(ZPoint inWindowLocation, ZDrop& inDrop);

	virtual void OwnerSetupMenus(ZMenuSetup& inMenuSetup);

	virtual bool OwnerTitleIconHit(ZPoint inHitPoint);

// Warning OwnerGetBackInk may (will) be called at a time when the window lock is not held.
	virtual ZDCInk OwnerGetBackInk(bool inActive);

	ZOSWindow* GetOSWindow()
		{ return fOSWindow; }

	void OSWindowMustDispose(ZOSWindow* inOSWindow);

protected:
	ZOSWindow* fOSWindow;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp

class ZOSAppOwner;

class ZOSApp
	{
public:
	ZOSApp();
	virtual ~ZOSApp();

	void SetOwner(ZOSAppOwner* inOwner);
	void OwnerDisposed(ZOSAppOwner* inOwner);

	virtual ZMutexBase& GetLock() = 0;
	virtual void QueueMessage(const ZMessage& inMessage) = 0;
	void QueueMessageForOwner(const ZMessage& inMessage);
	virtual bool DispatchMessage(const ZMessage& inMessage);
	void DispatchMessageToOwner(const ZMessage& iMessage);

	virtual void Run() = 0;
	virtual bool IsRunning() = 0;
	virtual void ExitRun() = 0;

	virtual string GetAppName() = 0;

	virtual bool HasUserAccessibleWindows() = 0;

	virtual bool HasGlobalMenuBar() = 0;
	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

	virtual void BroadcastMessageToAllWindows(const ZMessage& inMessage) = 0;

	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes) = 0;

	virtual ZOSWindowRoster* CreateOSWindowRoster() = 0;

protected:
	ZOSAppOwner* fOwner;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSAppOwner

class ZOSAppOwner
	{
protected:
	ZOSAppOwner(ZOSApp* inOSApp);
	virtual ~ZOSAppOwner();

public:
	virtual void OwnerDispatchMessage(const ZMessage& inMessage);

// This is used only on Mac, and only when there are no targettable windows open.
	virtual void OwnerSetupMenus(ZMenuSetup& inMenuSetup);

	ZOSApp* GetOSApp() { return fOSApp; }

protected:
	ZOSApp* fOSApp;
	};

// =================================================================================================

#endif // __ZOSWindow__
