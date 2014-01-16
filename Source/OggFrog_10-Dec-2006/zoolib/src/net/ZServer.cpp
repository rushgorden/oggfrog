static const char rcsid[] = "@(#) $Id: ZServer.cpp,v 1.9 2006/04/10 20:44:20 agreen Exp $";

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

#include "ZServer.h"
#include "ZMemory.h"
#include "ZThreadSimple.h"

#include "ZCompat_algorithm.h"

using std::vector;

// =================================================================================================
#pragma mark -
#pragma mark * ZServerListener

ZServerListener::ZServerListener()
:	fMutex("ZServerListener::fMutex"),
	fCondition("ZServerListener::fCondition"),
	fSem(0, "ZServerListener::fSem"),
	fMaxResponders(0),
	fAcceptedCount(0)
	{}

ZServerListener::~ZServerListener()
	{
	ZMutexLocker locker(fMutex);

	// StopWaitingForConnections must be called before we're deleted.
	ZAssert(!fNetListener);

	// If we get deleted before our responders, then tell them we've gone
	for (vector<ZServerResponder*>::iterator i = fResponders.begin(); i != fResponders.end(); ++i)
		(*i)->ListenerDeleted(this);
	fResponders.erase(fResponders.begin(), fResponders.end());
	locker.Release();
	}

ZRef<ZNetListener> ZServerListener::GetNetListener()
	{ return fNetListener; }

void ZServerListener::StartWaitingForConnections(ZRef<ZNetListener> inListener)
	{
	ZMutexLocker locker(fMutex);
	ZAssert(!fNetListener);

	fNetListener = inListener;

	(new ZThreadSimple<ZServerListener*>(ZServerListener::sRunThread, this, "ZServerListener::sRunThread"))->Start();

	// Wait for the thread to start before we return.
	fSem.Wait(1);
	}

void ZServerListener::StopWaitingForConnections()
	{
	ZMutexLocker locker(fMutex);
	if (fNetListener)
		{
		fNetListener->CancelListen();
		fNetListener.Clear();
		fCondition.Broadcast();
		locker.Release();

		// Wait for listen thread to exit before we return
		fSem.Wait(1);
		}
	}

void ZServerListener::KillAllConnections()
	{
	// First of all, cancel our listener thread, if any.
	this->StopWaitingForConnections();

	// Now, tell all our responders to bail
	ZMutexLocker locker(fMutex);
	while (!fResponders.empty())
		{
		ZServerResponder* aResponder = *(fResponders.begin());
		aResponder->KillConnection();
		fCondition.Wait(fMutex);
		}
	}

void ZServerListener::SetMaxResponders(size_t iCount)
	{
	ZMutexLocker locker(fMutex);
	fMaxResponders = iCount;
	fCondition.Broadcast();
	}

void ZServerListener::ResponderFinished(ZServerResponder* inResponder)
	{
	ZMutexLocker locker(fMutex);

	vector<ZServerResponder*>::iterator theIter = find(fResponders.begin(), fResponders.end(), inResponder);
	ZAssert(theIter != fResponders.end());
	fResponders.erase(theIter);
	fCondition.Broadcast();
	}

void ZServerListener::RunThread()
	{
	// AG 98-02-05. consecutiveErrorCount is a preemptive strike against nasty situations that
	// may arise where the networking subsystem starts failing consistently. If we're running
	// at deferred task time on the Mac, just Yielding is not enough -- we have to sleep or be blocked
	// in some other way if we're ever gonna leave Deferred Task time.
	int consecutiveErrorCount = 0;

	fSem.Signal(1);
	ZMutexLocker locker(fMutex);

	for (;;)
		{
		while (fNetListener && fMaxResponders && fResponders.size() >= fMaxResponders)
			fCondition.Wait(fMutex);

		if (!fNetListener)
			break;

		ZRef<ZNetListener> theListener = fNetListener;
		locker.Release();

		if (ZRef<ZNetEndpoint> theEndpoint = theListener->Listen())
			{
			if (ZServerResponder* theResponder = this->CreateServerResponder())
				{
				locker.Acquire();
				fResponders.push_back(theResponder);
				fCondition.Broadcast();
				++fAcceptedCount;
				locker.Release();

				theResponder->AcceptIncomingFromListener(this, theEndpoint);
				}
			}
		else
			{
			if (++consecutiveErrorCount == 10)
				{
				consecutiveErrorCount = 0;
				ZThread::sSleep(1000);
				}
			}
		locker.Acquire();
		}
	fSem.Signal(1);
	}

void ZServerListener::sRunThread(ZServerListener* inListener)
	{ inListener->RunThread(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZServerResponder

ZServerResponder::ZServerResponder()
	{
	fServerListener = nil;
	fEndpoint = nil;
	fThread = nil;
	}

ZServerResponder::~ZServerResponder()
	{
	ZAssert(fThread == nil);
	}

void ZServerResponder::AcceptIncomingFromListener(ZServerListener* inListener, ZRef<ZNetEndpoint> inEndpoint)
	{
	ZAssert(inListener);
	ZAssert(inEndpoint);
	ZAssert(fServerListener == nil);
	ZAssert(!fEndpoint);

	fServerListener = inListener;
	fEndpoint = inEndpoint;

	// Create and start the thread that will actually service the socket
	fThread = new ZThreadSimple<ZServerResponder*>(ZServerResponder::sRunThread, this, "ZServerResponder::sRunThread");
	fThread->Start();
	}

void ZServerResponder::ListenerDeleted(ZServerListener* inListener)
	{
	ZAssert(inListener == fServerListener);
	fServerListener = nil;
	}

void ZServerResponder::KillConnection()
	{
	// This will cause any pending comms to abort, returning an error
	// to our thread which is reading/writing the endpoint. So any code
	// which gets such an error should unwind explicitly or by throwing an
	// exception.
	fEndpoint->Abort();
	}

void ZServerResponder::RunThread()
	{
	ZAssert(fEndpoint);
	try
		{
		this->DoRun(fEndpoint);
		// If our DoRun cleanly sent and received disconnects then Abort
		// will have no effect. If it didn't then we'll get the endpoint's
		// underlying connection torn down immediately.
		fEndpoint->Abort();
		}
	catch (...)
		{}

	if (fServerListener)
		fServerListener->ResponderFinished(this);

	fThread = nil;

	delete this;
	}

void ZServerResponder::sRunThread(ZServerResponder* inResponder)
	{ inResponder->RunThread(); }

// ==================================================
