/*  @(#) $Id: ZServer.h,v 1.7 2006/04/10 20:44:20 agreen Exp $ */

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

#ifndef __ZServer__
#define __ZServer__ 1
#include "zconfig.h"

#include "ZNet.h"
#include "ZThread.h"

#include <vector>

// =================================================================================================
#pragma mark -
#pragma mark * ZServerListener

class ZServerResponder;

class ZServerListener
	{
public:
	ZServerListener();
	virtual ~ZServerListener();

	ZRef<ZNetListener> GetNetListener();

	void StartWaitingForConnections(ZRef<ZNetListener> inListener);
	void StopWaitingForConnections();
	void KillAllConnections();

	void SetMaxResponders(size_t iCount);

protected:
	void ResponderFinished(ZServerResponder* inResponder);

	virtual ZServerResponder* CreateServerResponder() = 0;

private:
	void RunThread();
	static void sRunThread(ZServerListener* inListener);

private:
	ZMutex fMutex;
	ZCondition fCondition;
	ZSemaphore fSem;
	std::vector<ZServerResponder*> fResponders;

	ZRef<ZNetListener> fNetListener;
	size_t fMaxResponders;
	size_t fAcceptedCount;

	friend class ZServerResponder;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZServerResponder

class ZServerResponder
	{
public:
	ZServerResponder();
	virtual ~ZServerResponder();

	void AcceptIncomingFromListener(ZServerListener* inListener, ZRef<ZNetEndpoint> inEndpoint);
	void ListenerDeleted(ZServerListener* inListener);
	void KillConnection();

	virtual void DoRun(ZRef<ZNetEndpoint> inEndpoint) = 0;

protected:
	void RunThread();
	static void sRunThread(ZServerResponder* inListener);

	ZThread* fThread;
	ZServerListener* fServerListener;
	ZRef<ZNetEndpoint> fEndpoint;
	};

#endif // __ZServer__
