static const char rcsid[] = "@(#) $Id: ZApp.cpp,v 1.12 2006/08/20 20:02:06 agreen Exp $";

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

#include "ZApp.h"
#include "ZDebug.h"
#include "ZOSWindow_Be.h"
#include "ZOSWindow_Carbon.h"
#include "ZOSWindow_Mac.h"
#include "ZOSWindow_Win.h"
#include "ZOSWindow_X.h"

// =================================================================================================

ZApp* ZApp::sApp = nil;

ZApp* ZApp::sGet()
	{
	ZAssert(sApp);
	return sApp;
	}

static ZOSApp* sCreateOSApp(const char* inSignature)
	{
	ZOSApp* theOSApp = nil;

	#if 0
	#elif ZCONFIG(API_OSWindow, Be)

		theOSApp = new ZOSApp_Be(inSignature);

	#elif ZCONFIG(API_OSWindow, Carbon)

		theOSApp = new ZOSApp_Carbon;

	#elif ZCONFIG(API_OSWindow, Mac)

		theOSApp = new ZOSApp_Mac;

	#elif ZCONFIG(API_OSWindow, Win32)

		theOSApp = new ZOSApp_Win(inSignature);

	#elif ZCONFIG(API_OSWindow, X)

		theOSApp = new ZOSApp_X;

	#endif

	ZAssert(theOSApp);
	return theOSApp;
	}

ZApp::ZApp(const char* inSignature)
:	ZOSAppOwner(sCreateOSApp(inSignature)),
	ZMessageReceiver(this)
	{
	ZAssert(sApp == nil);
	sApp = this;
	fPostedInvalidateMenuBar = false;
	this->InvalidateMenuBar();
	}

ZApp::ZApp(ZOSApp* inOSApp)
:	ZOSAppOwner(inOSApp),
	ZMessageReceiver(this)
	{
	ZAssert(sApp == nil);
	sApp = this;
	fPostedInvalidateMenuBar = false;
	this->InvalidateMenuBar();
	}

ZApp::~ZApp()
	{
	ZAssert(sApp == this);
	sApp = nil;
	}

ZMutexBase& ZApp::GetLock()
	{ return fOSApp->GetLock(); }

bool ZApp::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZMessageLooper::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Idle")
			{
			this->Idle();
			return true;
			}
		else if (theWhat == "zoolib:CommandLine")
			{
			this->CommandLine(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:RunStarted")
			{
			this->RunStarted();
			return true;
			}
		else if (theWhat == "zoolib:RunFinished")
			{
			this->RunFinished();
			return true;
			}
		else if (theWhat == "zoolib:OpenDocument")
			{
			this->OpenDocument(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:PrintDocument")
			{
			this->PrintDocument(inMessage);
			return true;
			}
		else if (theWhat == "zoolib:WindowRosterChange")
			{
			this->WindowRosterChanged();
			return true;
			}
		else if (theWhat == "zoolib:InvalidateMenuBar")
			{
			if (fPostedInvalidateMenuBar)
				{
				fPostedInvalidateMenuBar = false;
				ZMenuBar theMenuBar;
				this->WindowSupervisorInstallMenus(theMenuBar);
				fOSApp->SetMenuBar(theMenuBar);
				}
			return true;
			}
		else if (theWhat == "zoolib:InvalidateAllMenuBars")
			{
			this->InvalidateMenuBar();
			return true;
			}
		else if (theWhat == "zoolib:RequestQuit")
			{
			if (this->QuitRequested())
				this->ExitRun();
			return true;
			}
		}
	return false;
	}

void ZApp::QueueMessage(const ZMessage& inMessage)
	{
	fOSApp->QueueMessageForOwner(inMessage);
	}

void ZApp::ReceivedMessage(const ZMessage& inMessage)
	{
	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Menu")
			{
			switch (inMessage.GetInt32("menuCommand"))
				{
				case mcQuit:
					{
					this->QueueRequestQuitMessage();
					break;
					}
				default:
					{
					break;
					}
				}
			}
		}
	}

void ZApp::WindowSupervisorInstallMenus(ZMenuInstall& inMenuInstall)
	{
	}

void ZApp::WindowSupervisorSetupMenus(ZMenuSetup& inMenuSetup)
	{
	if (this->IsRunning())
		inMenuSetup.EnableItem(mcQuit);
	inMenuSetup.EnableItem(mcAbout);
	}

void ZApp::WindowSupervisorPostMessage(const ZMessage& inMessage)
	{ this->PostMessage(inMessage); }

ZOSWindow* ZApp::CreateOSWindow(const ZOSWindow::CreationAttributes& inAttributes)
	{ return fOSApp->CreateOSWindow(inAttributes); }

void ZApp::Run()
	{ fOSApp->Run(); }

void ZApp::ExitRun()
	{ fOSApp->ExitRun(); }

bool ZApp::IsRunning()
	{ return fOSApp->IsRunning(); }

void ZApp::QueueRequestQuitMessage()
	{
	ZMessage theMessage;
	theMessage.SetString("what", "zoolib:RequestQuit");
	this->QueueMessage(theMessage);
	}

string ZApp::GetAppName()
	{ return fOSApp->GetAppName(); }

bool ZApp::HasUserAccessibleWindows()
	{ return fOSApp->HasUserAccessibleWindows(); }

bool ZApp::HasGlobalMenuBar()
	{ return fOSApp->HasGlobalMenuBar(); }

void ZApp::InvalidateMenuBar()
	{
	ZAssertLocked(1, this->GetLock());
	if (!fPostedInvalidateMenuBar)
		{
		fPostedInvalidateMenuBar = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:InvalidateMenuBar");
		this->QueueMessage(theMessage);
		}
	}

void ZApp::InvalidateAllMenuBars()
	{
	// We don't need to be locked to broadcast this message
	ZTuple theTuple;
	theTuple.SetString("what", "zoolib:InvalidateAllMenuBars");

	ZMessage envelope;
	envelope.SetString("what", "zoolib:Owner");
	envelope.SetTuple("message", theTuple);

	fOSApp->BroadcastMessageToAllWindows(envelope);
	}

void ZApp::CommandLine(const ZMessage& inMessage)
	{
	}

void ZApp::OpenDocument(const ZMessage& inMessage)
	{
	}

void ZApp::PrintDocument(const ZMessage& inMessage)
	{
	}

void ZApp::RunStarted()
	{
	}

bool ZApp::QuitRequested()
	{
	auto_ptr<ZOSWindowRoster> theRoster(fOSApp->CreateOSWindowRoster());
	bool allOkay = true;
	while (allOkay)
		{
		bool askedAnyWindows = false;
		// First work through every window and ask those that are active and visible (tautological, I know) if it's okay to quit
		while (allOkay)
			{
			ZOSWindow* theOSWindow;
			if (theRoster->GetNextOSWindow(2000000, theOSWindow))
				{
				if (!theOSWindow)
					break;
				if (ZWindow* theWindow = dynamic_cast<ZWindow*>(theOSWindow->GetOwner()))
					{
					askedAnyWindows = true;
					if (theWindow->GetWindowActive() && theWindow->GetShown())
						{
						theRoster->DropCurrentOSWindow();
						allOkay = theWindow->WindowQuitRequested();
						}
					}
				theRoster->UnlockCurrentOSWindow();
				}
			else
				{
				// If we timed out, treat it as if the user had explicitly cancelled.
				allOkay = false;
				}
			}
		// Reset the roster, which empties the list of visited windows but remembers the dropped windows.
		theRoster->Reset();

		// Now work through every window and ask those that are at least visible
		while (allOkay)
			{
			ZOSWindow* theOSWindow;
			if (theRoster->GetNextOSWindow(2000000, theOSWindow))
				{
				if (!theOSWindow)
					break;
				if (ZWindow* theWindow = dynamic_cast<ZWindow*>(theOSWindow->GetOwner()))
					{
					askedAnyWindows = true;
					if (theWindow->GetShown())
						{
						theRoster->DropCurrentOSWindow();
						allOkay = theWindow->WindowQuitRequested();
						}
					}
				theRoster->UnlockCurrentOSWindow();
				}
			else
				{
				allOkay = false;
				}
			}
		// Once more, reset the roster
		theRoster->Reset();

		// And work through every window, regardless of whether it's visible or active.
		while (allOkay)
			{
			ZOSWindow* theOSWindow;
			if (theRoster->GetNextOSWindow(2000000, theOSWindow))
				{
				if (!theOSWindow)
					break;
				if (ZWindow* theWindow = dynamic_cast<ZWindow*>(theOSWindow->GetOwner()))
					{
					askedAnyWindows = true;
					theRoster->DropCurrentOSWindow();
					allOkay = theWindow->WindowQuitRequested();
					}
				theRoster->UnlockCurrentOSWindow();
				}
			else
				{
				allOkay = false;
				}
			}
		if (!askedAnyWindows)
			break;
		}
	return allOkay;
	}

void ZApp::RunFinished()
	{}

void ZApp::WindowRosterChanged()
	{
	if (!this->HasGlobalMenuBar() && !this->HasUserAccessibleWindows())
		{
		if (this->QuitRequested())
			fOSApp->ExitRun();
		}
	}

void ZApp::Idle()
	{}

void ZApp::OwnerDispatchMessage(const ZMessage& inMessage)
	{
	// Forward the dispatch to our MessageLooper overridable API
	this->DispatchMessage(inMessage);
	}

// This only ever gets called on Mac (because of the global menu
// bar) and simply forwards to the WindowSupervisor API
void ZApp::OwnerSetupMenus(ZMenuSetup& inMenuSetup)
	{ this->WindowSupervisorSetupMenus(inMenuSetup); }
