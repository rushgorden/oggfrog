static const char rcsid[] =
"@(#) $Id: ZMessageLooperImpStd.cpp,v 1.6 2006/04/26 22:31:27 agreen Exp $";

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

#include "ZMessageLooperImpStd.h"

#include "ZTicks.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageLooperImpStd

ZMessageLooperImpStd::ZMessageLooperImpStd()
:	fMutex_MessageDispatch(nil, true)
	{
// We start life with our message dispatch mutex held, hence the ctor initializer above
	fMessageLoopNesting = 0;
	fTimeLastMessageDispatched = 0;
	fDispatchingMessage = false;

	fLooperExistsAddress = nil;
	fRunMessageLoop = false;
	}

ZMessageLooperImpStd::~ZMessageLooperImpStd()
	{
	ZAssertLocked(1, fMutex_MessageDispatch);

	if (sLooperForCurrentThread() == this)
		{
		/* If we're being called by ZMessageLooperImpStd::RunMessageLoop, potentially
		indirectly and from multiple nestings, then each caller of RunMessageLoop will
		have passed in the address of a bool, the last of which is now stored in
		fLooperExistsAddress. We set that false, and simply return from this method,
		which will return through each invoking RunMessageLoop, which will see that its
		bool is true, will set true the prior bool, and return without touching the
		this variable. In general, deleting ourselves is *not* a safe operation. ZooLib
		does so under very constrained circumstances, and you should also. However,
		sometime, somewhere, someone's going to PoseModal on some window, and then
		PoseModal on another window, and then delete the calling window before the
		calls to PoseModal have returned. And this all will handle that, although the
		rest of their code might not :) */
		ZAssertStop(1, fLooperExistsAddress);
		*fLooperExistsAddress = false;
		fLooperExistsAddress = nil;
		}
	else
		{
		// In the other case we're being deleted by some other thread, and can thus just
		// force the main message loop to exit by setting fRunMessageLoop false, broadcasting
		// fCondition_Structure and waiting for fMessageLoopNesting to hit zero.
		fMutex_Structure.Acquire();
		fRunMessageLoop = false;
		fCondition_Structure.Broadcast();

		while (fMessageLoopNesting > 0)
			{
			fMutex_Structure.Release();
			fCondition_MessageDispatch.Wait(fMutex_MessageDispatch);
			fMutex_Structure.Acquire();
			}
		fMutex_Structure.Release();
		}
	// The two branches above mean that ZMessageLooperImpStd is robust in the face
	// of destruction by calls from any source -- your own code may not be so
	// robust, so be careful how you trash message loopers.
	}

#ifdef __BEOS__
static ZThread::TLSKey_t sTLSKey_LooperForCurrentThread;
#else
// On BeOS this will call tlsalloc before tlsinit has been called (because our
// ZThread::InitHelper trick won't link on BeOS.) As the only current use for
// ZMessageLooperImpStd is to run window message loops, and BeOS has its own.
static ZThread::TLSKey_t sTLSKey_LooperForCurrentThread = ZThread::sTLSAllocate();
#endif

void ZMessageLooperImpStd::PreRunMessageLoop()
	{
	ZAssertStop(1, fMutex_MessageDispatch.IsLocked());
	ZAssertStop(1, fMessageLoopNesting >= 0);
	fMutex_Structure.Acquire();
	++fMessageLoopNesting;
	ZAssertStop(1, fRunMessageLoop == false);
	fRunMessageLoop = true;
	fMutex_Structure.Release();
	}

bool ZMessageLooperImpStd::RunMessageLoop(bool* inRunFlag)
	{
	ZAssertStop(1, !fMutex_MessageDispatch.IsLocked());

	ZMessageLooperImpStd* priorLooperForThread =
		reinterpret_cast<ZMessageLooperImpStd*>(ZThread::sTLSGet(sTLSKey_LooperForCurrentThread));
	ZAssertStop(1, priorLooperForThread == nil || priorLooperForThread == this);
	ZThread::sTLSSet(sTLSKey_LooperForCurrentThread, reinterpret_cast<ZThread::TLSData_t>(this));

	try
		{
		fMutex_Structure.Acquire();
		ZAssertStop(1, fMessageLoopNesting > 0);

		bool* priorLooperExistsAddress = fLooperExistsAddress;
		bool looperExists = true;
		fLooperExistsAddress = &looperExists;

		while ((inRunFlag == nil || *inRunFlag) && fRunMessageLoop)
			{
			while (fMessageQueue.size() == 0
				&& (inRunFlag == nil || *inRunFlag) && fRunMessageLoop)
				{
				fCondition_Structure.Wait(fMutex_Structure);
				}

			if (fMessageQueue.size() > 0
				&& (inRunFlag == nil || *inRunFlag) && fRunMessageLoop)
				{
				fDispatchingMessage = true;
				fTimeLastMessageDispatched = ZTicks::sNow();
				fMutex_Structure.Release();
				fMutex_MessageDispatch.Acquire();
				fMutex_Structure.Acquire();
				if (fMessageQueue.size() > 0
					&& (inRunFlag == nil || *inRunFlag) && fRunMessageLoop)
					{
					ZMessage theMessage = fMessageQueue.front();
					fMessageQueue.pop_front();
					fMutex_Structure.Release();

					try
						{
						this->DispatchMessageImp(theMessage);
						}
					catch (...)
						{}

					if (!looperExists)
						{
						if (priorLooperExistsAddress)
							*priorLooperExistsAddress = false;
						return false;
						}

					fMutex_Structure.Acquire();
					fDispatchingMessage = false;
					}
				fMutex_MessageDispatch.Release();
				}
			}

		fLooperExistsAddress = priorLooperExistsAddress;

		--fMessageLoopNesting;

		fMutex_Structure.Release();
		}
	catch (...)
		{
		ZDebugStopf(0, ("Serious problem ... uncaught exception"));
		}
	
	fMutex_MessageDispatch.Acquire();
	fCondition_MessageDispatch.Broadcast();
	fMutex_MessageDispatch.Release();

	if (priorLooperForThread == nil)
		ZThread::sTLSSet(sTLSKey_LooperForCurrentThread, nil);
	return true;
	}

void ZMessageLooperImpStd::ExitMessageLoop()
	{
	ZAssertLocked(1, fMutex_MessageDispatch);
	ZMutexLocker locker(fMutex_Structure);
	ZAssertStop(1, fRunMessageLoop);
	fRunMessageLoop = false;
	fCondition_Structure.Broadcast();
	}

void ZMessageLooperImpStd::QueueMessageImp(const ZMessage& inMessage)
	{
	if (ZThread::errorNone == fMutex_Structure.Acquire())
		{
		fMessageQueue.push_back(inMessage);
		fCondition_Structure.Broadcast();
		fMutex_Structure.Release();
		}
	}

void ZMessageLooperImpStd::DispatchMessageImp(const ZMessage& inMessage)
	{
	ZDebugLogf(0, ("ZMessageLooperImpStd::DispatchMessageImp called"));
	}

int32 ZMessageLooperImpStd::GetMessageLoopNesting()
	{ return fMessageLoopNesting; }

bool ZMessageLooperImpStd::RunNestedMessageLoop(bool* inRunFlag)
	{
	ZAssertStop(1, fMutex_MessageDispatch.IsLocked());
	ZAssertStop(1, fMessageLoopNesting > 0);
	fMutex_Structure.Acquire();
	++fMessageLoopNesting;
	fMutex_Structure.Release();

	int32 oldMutexCount = fMutex_MessageDispatch.FullRelease();
	if (!this->RunMessageLoop(inRunFlag))
		return false;
	fMutex_MessageDispatch.FullAcquire(oldMutexCount);
	return true;
	}

void ZMessageLooperImpStd::ExitNestedMessageLoop(bool* inRunFlag)
	{
	fMutex_Structure.Acquire();
	ZAssertStop(1, inRunFlag != nil);
	*inRunFlag = false;
	fCondition_Structure.Broadcast();
	fMutex_Structure.Release();
	}

bool ZMessageLooperImpStd::IsBusy()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fDispatchingMessage)
		return false;
	// If it's been a second since we dispatched the message then we're considered busy
	if (ZTicks::sNow() - fTimeLastMessageDispatched < ZTicks::sPerSecond())
		return false;
	return true;
	}

ZMessageLooperImpStd* ZMessageLooperImpStd::sLooperForCurrentThread()
	{
	return reinterpret_cast<ZMessageLooperImpStd*>(
		ZThread::sTLSGet(sTLSKey_LooperForCurrentThread));
	}
