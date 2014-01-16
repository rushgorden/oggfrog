/*  @(#) $Id: ZOSWindow_X.h,v 1.6 2006/04/10 20:44:21 agreen Exp $ */

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

#ifndef __ZOSWindow_X__
#define __ZOSWindow_X__ 1
#include "zconfig.h"

#if ZCONFIG(API_OSWindow, X)
#include "ZOSWindow_Std.h"

#include "ZXServer.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_X

class ZOSApp_X;
class ZDCCanvas_X_Window;

class ZOSWindow_X : public ZOSWindow_Std, public ZXWindow
	{
protected:
	friend class ZOSApp_X;

	ZOSWindow_X(ZOSApp_X* inApp, ZRef<ZXServer> inXServer, const ZOSWindow::CreationAttributes& inAttributes);
	virtual ~ZOSWindow_X();

public:
// From ZOSWindow via ZOSWIndow_Std
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

	virtual void BringFront(bool inMakeVisible);

	virtual void GetContentInsets(ZPoint& outTopLeftInset, ZPoint& outBottomRightInset);

	virtual bool GetSizeBox(ZPoint& outSizeBoxSize);

	virtual ZDCRgn GetVisibleContentRgn();

	virtual void InvalidateRegion(const ZDCRgn& inBadRgn);

	virtual void UpdateNow();

	virtual ZRef<ZDCCanvas> GetCanvas();

	virtual void AcquireCapture();
	virtual void ReleaseCapture();

	virtual void SetMenuBar(const ZMenuBar& inMenuBar);

	virtual void GetNative(void* outNative);

	virtual ZDragInitiator* CreateDragInitiator();

	virtual ZRect GetScreenFrame(int32 inScreenIndex, bool inUsableSpaceOnly);

// From ZOSWindow_Std
	virtual ZDCRgn Internal_GetExcludeRgn();

// From ZXWindow
	virtual void HandleXEvent(const XEvent& inEvent);

// Our protocol
	bool Internal_GetShown() { return fShown; }

protected:
	void Internal_Tickle();

	ZOSApp_X* fApp;
	bool fShown;
	string fTitle;
	Cursor fXCursor;
	ZDCRgn fPartialInvalRgn;
	bool fCaptured;
	ZRef<ZDCCanvas_X_Window> fCanvas;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_X

class ZDCCanvas_X;

class ZOSApp_X : public ZOSApp_Std
	{
public:
	static ZOSApp_X* sGet();

	ZOSApp_X();
	virtual ~ZOSApp_X();

// From ZOSApp via ZOSApp_Std
	virtual void Run();

	virtual string GetAppName();

	virtual bool HasUserAccessibleWindows();

	virtual bool HasGlobalMenuBar();

	virtual void BroadcastMessageToAllWindows(const ZMessage& inMessage);

	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes);

// From ZOSApp_Std
	virtual bool Internal_GetNextOSWindow(const vector<ZOSWindow_Std*>& inVector_VisitedWindows,
												const vector<ZOSWindow_Std*>& inVector_DroppedWindows,
												bigtime_t inTimeout, ZOSWindow_Std*& outWindow);

// Our protocol
	void AddOSWindow(ZOSWindow_X* inOSWindow);
	void RemoveOSWindow(ZOSWindow_X* inOSWindow);

protected:
	void RunEventLoop();
	static void sRunEventLoop(ZOSApp_X* inOSApp);

	void IdleThread();
	static void sIdleThread(ZOSApp_X* inOSApp);

	static ZOSApp_X* sOSApp_X;

	ZSemaphore* fSemaphore_IdleExited;

	ZSemaphore* fSemaphore_EventLoopExited;

	vector<ZRef<ZXServer> > fVector_XServer;

	vector<ZOSWindow_X*> fVector_Windows;
	};

#endif // ZCONFIG(API_OSWindow, X)


#endif // __ZOSWindow_X__
