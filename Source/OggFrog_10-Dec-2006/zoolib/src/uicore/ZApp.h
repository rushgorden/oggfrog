/*  @(#) $Id: ZApp.h,v 1.7 2006/04/14 21:02:35 agreen Exp $ */

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

#ifndef __ZApp__
#define __ZApp__ 1
#include "zconfig.h"

#include "ZWindow.h"

// ==================================================

class ZApp : public ZOSAppOwner,
			public ZMessageLooper,
			public ZMessageReceiver,
			public ZWindowSupervisor
	{
public:
	static ZApp* sGet();

	ZApp(const char* inSignature = nil);
	ZApp(ZOSApp* inOSApp);
	virtual ~ZApp();

// From ZMessageLooper
	virtual ZMutexBase& GetLock();
	virtual bool DispatchMessage(const ZMessage& inMessage);
	virtual void QueueMessage(const ZMessage& inMessage);

// From ZMessageReceiver
	virtual void ReceivedMessage(const ZMessage& inMessage);

// From ZWindowSupervisor
	virtual void WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall);
	virtual void WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup);

	virtual void WindowSupervisorPostMessage(const ZMessage& inMessage);

// Our protocol
	virtual ZOSWindow* CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes);

	void Run();
	void ExitRun();
	bool IsRunning();
	void QueueRequestQuitMessage();

	string GetAppName();

	bool HasUserAccessibleWindows();

	bool HasGlobalMenuBar();
	void InvalidateMenuBar();

	void InvalidateAllMenuBars();

// Hook functions called by DispatchMessage
	virtual void CommandLine(const ZMessage& inMessage);
	virtual void OpenDocument(const ZMessage& inMessage);
	virtual void PrintDocument(const ZMessage& inMessage);

	virtual void RunStarted();
	virtual bool QuitRequested();
	virtual void RunFinished();

	virtual void WindowRosterChanged();

	virtual void Idle();

protected:
	static ZApp* sApp;

	bool fPostedInvalidateMenuBar;

private:
// From ZOSAppOwner. Note that we use private inheritance, mark these
// methods as private and place them down here in order that they not appear
// too obvious and tempting to try to override. That ZApp is a ZOSAppOwner
// is really an implementation detail.
	virtual void OwnerDispatchMessage(const ZMessage& inMessage);
	virtual void OwnerSetupMenus(ZMenuSetup& inMenuSetup);
	};


// ==================================================

#endif // __ZApp__
