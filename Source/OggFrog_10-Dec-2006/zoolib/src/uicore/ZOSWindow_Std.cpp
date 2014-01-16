static const char rcsid[] = "@(#) $Id: ZOSWindow_Std.cpp,v 1.10 2006/07/17 18:26:42 agreen Exp $";

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

#include "ZOSWindow_Std.h"
#include "ZThreadSimple.h"
#include "ZTicks.h"
#include "ZUtil_STL.h"

/* AG 2000-07-02.
ZOSWindow_Std Locking Scheme.
ZOSWindow_Std has two mutex/condition variable pairs. The first is
fMutex_MessageDispatch/fCondition_MessageDispatch. The mutex is exposed to the outside world by
ZOSWindow_Std's overload of ZOSWindow::GetLock. It is acquired and held when a message is pulled off
the message queue and dispatched to the window or its owner. It must be acquired by any method that
calls pretty much any method on the window (Get/SetPosition, Get/SetSize etc). This ensures that
any code that executes as a result of a message dispatch can depend on the window's state remaining
unchanged -- this behavior is a requirement of the ZOSWindow rules.

The second mutex/condiiton pair is fMutex_Structure/fCondition_Structure which is protects internal
data structures like the message queue itself and instance variables which hold the current state
of the window (recorded and reported). Subclasses of ZOSWindow_Std call methods like
Internal_RecordSizeChange to update state instance variables, no lock need be acquired for that to
occur, but the expectation is that some external rules will ensure that the window cannot be
disposed whilst the method call is executing. For example, on Windows, such state updates are
invoked from the single low-level message pump, as a result of a Win message being dispatched to a
WindowProc. Disposal of the window also occurs as a result of a message dispatch to a WindowProc
from the same message pump, hence the window cannot be disposed until the state change method and
thus the win message dispatch return. fMutex_Structure may be acquired at any time, but code that
does so must be very careful to limit which other locks it subsequently acquires, in order that
acquisition order is always consistent and hence deadlocks do not occur. 99% of the time you won't
have to worry about fMutex_Structure unless you are implementing some derived version of ZOSWindow.

ZOSApp_Std has the same set of variables, with similar rules, except that fMutex_Structure is also
often used by derived classes to guard things like low level event dispatch and the window list.
See notes in other files as to how that usage is made to be deadlock-free.
*/

#define ASSERTLOCKED() ZAssertStopf(1, this->GetLock().IsLocked(), ("You must lock the window"))

// =================================================================================================
#pragma mark -
#pragma mark * kDebug

#define kDebug_Std 1

static bool sFillUpdates = false;
static bool sFillInvalidates = false;

// =================================================================================================
#pragma mark -
#pragma mark * Static helper functions

static ZMessage sMouseStateToMessage(const ZOSWindow_Std::MouseState& inMouseState, bigtime_t inWhen)
	{
	ZMessage theMessage;
	theMessage.SetPoint("where", inMouseState.fLocation);
	theMessage.SetInt64("when", inWhen);

	theMessage.SetBool("contained", inMouseState.fContained);

	theMessage.SetBool("buttonLeft", inMouseState.fButtons[0]);
	theMessage.SetBool("buttonMiddle", inMouseState.fButtons[1]);
	theMessage.SetBool("buttonRight", inMouseState.fButtons[2]);
	theMessage.SetBool("button4", inMouseState.fButtons[3]);
	theMessage.SetBool("button5", inMouseState.fButtons[4]);

	theMessage.SetBool("command", inMouseState.fModifier_Command);
	theMessage.SetBool("option", inMouseState.fModifier_Option);
	theMessage.SetBool("shift", inMouseState.fModifier_Shift);
	theMessage.SetBool("control", inMouseState.fModifier_Control);
	theMessage.SetBool("capsLock", inMouseState.fModifier_CapsLock);
	return theMessage;
	}

static bool sMouseStatesDiffer(const ZOSWindow_Std::MouseState& a, const ZOSWindow_Std::MouseState& b)
	{
	if (a.fLocation != b.fLocation)
		return true;
	for (int x = 0; x < 5; ++x)
		{
		if (a.fButtons[x] != b.fButtons[x])
			return true;
		}
	if (a.fModifier_Command != b.fModifier_Command)
		return true;
	if (a.fModifier_Option != b.fModifier_Option)
		return true;
	if (a.fModifier_Shift != b.fModifier_Shift)
		return true;
	if (a.fModifier_Control != b.fModifier_Control)
		return true;
	if (a.fModifier_CapsLock != b.fModifier_CapsLock)
		return true;
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindow_Std

ZOSWindow_Std::ZOSWindow_Std()
	{
	// When PoseModal is called on us, fPoseModal_CallerLooper holds a pointer to the looper from
	// whose message loop the call was made (ie *not* us). If the call was made from some other
	// thread, one not running a message loop, then fPoseModal_Semaphore holds the address of a
	// semaphore that thread is waiting on whilst we are waiting for EndPoseModal to be called. It
	// is always legal to delete an OSWindow if you hold the window's lock (ie its message dispatch
	// mutex). When some other thread, be it window or worker, is waiting for our EndPoseModal
	// method to be called, we have to be able to allow that thread to run, but to communicate to
	// it that the window on which it called PoseModal (us) no longer exists. Seeing as it is also
	// legal (but annoying) for a window that calls our PoseModal method to *itself* be deleted
	// before our EndPoseModal method is called we can't simply call a method or set a flag on the
	// calling window. So fPoseModal_WasDisposed holds the address of a bool allocated on the stack
	// by the window/thread calling PoseModal. It is then free to do what it likes with the
	// information.
	fPoseModal_CallerLooper = nil;
	fPoseModal_CallerLooperRunNestedMessageLoop = nil;
	fPoseModal_Semaphore = nil;
	fPoseModal_CalleeExistsAddress = nil;

	// Instance variables protected by fMutex_Structure
	fPendingIdle = false;

	fRecordedShownState = eZShownStateHidden;
	fReportedShownState = eZShownStateHidden;
	fPendingShownState = false;

	fReportedActive = false;
	fRecordedActive = false;
	fPendingActive = false;

	// Good defaults? No, but we'll need some API to get our app's current active
	// status in order that these fields values start life with the correct values.
	fReportedAppActive = true;
	fRecordedAppActive = true;
	fPendingAppActive = false;

	fReportedFrame = ZRect::sZero;
	fRecordedFrame = ZRect::sZero;
	fPendingFrame = false;

	fPreZoomFrameKnown = false;

	fPendingCloseBoxHit = false;
	fPendingZoomBoxHit = false;

	fRecordedMouseState.fLocation.h = 0;
	fRecordedMouseState.fLocation.v = 0;
	fRecordedMouseState.fContained = false;
	fRecordedMouseState.fButtons[0] = false;
	fRecordedMouseState.fButtons[1] = false;
	fRecordedMouseState.fButtons[2] = false;
	fRecordedMouseState.fButtons[3] = false;
	fRecordedMouseState.fButtons[4] = false;
	fRecordedMouseState.fModifier_Command = false;
	fRecordedMouseState.fModifier_Option = false;
	fRecordedMouseState.fModifier_Shift = false;
	fRecordedMouseState.fModifier_Control = false;
	fRecordedMouseState.fModifier_CapsLock = false;
	fRecordedMouseStateWhen = 0;

	fReportedMouseState = fRecordedMouseState;
	fReportedMouseStateWhen = fRecordedMouseStateWhen;
	fPendingMouseState = false;

	fClickCount = 0;
	fClickLocation = ZPoint::sZero;
	fTimeOfLastClick = 0;

	// Mark ourselves as committed to running the message loop
	this->PreRunMessageLoop();

	// And fire off our thread that will actually run the message loop
	(new ZThreadSimple<ZOSWindow_Std*>(sRunMessageLoop, this))->Start();
	}

ZOSWindow_Std::~ZOSWindow_Std()
	{
	ASSERTLOCKED();

	if (fPoseModal_CalleeExistsAddress)
		*fPoseModal_CalleeExistsAddress = false;

	if (fPoseModal_CallerLooperRunNestedMessageLoop)
		{
		ZMessageLooperImpStd* theCallerLooper = fPoseModal_CallerLooper;
		fPoseModal_CallerLooper = nil;
		theCallerLooper->ExitNestedMessageLoop(fPoseModal_CallerLooperRunNestedMessageLoop);
		fPoseModal_CallerLooperRunNestedMessageLoop = nil;
		}
	else if (fPoseModal_Semaphore)
		{
		fPoseModal_Semaphore->Signal(1);
		fPoseModal_Semaphore = nil;
		}

	while (fPoseModal_CalleeExistsAddress != nil)
		fCondition_MessageDispatch.Wait(fMutex_MessageDispatch);
	}

void ZOSWindow_Std::DispatchMessageImp(const ZMessage& inMessage)
	{ this->DispatchMessage(inMessage); }

ZMutexBase& ZOSWindow_Std::GetLock()
	{ return fMutex_MessageDispatch; }

void ZOSWindow_Std::QueueMessage(const ZMessage& inMessage)
	{ this->QueueMessageImp(inMessage); }

bool ZOSWindow_Std::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSWindow::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Idle")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingIdle)
				{
				fPendingIdle = false;
				locker.Release();
				this->DispatchMessageToOwner(inMessage);
				}
			return true;
			}
		else if (theWhat == "zoolib:Invalid")
			{
			this->Internal_ReportInvalidation();
			return true;
			}
		else if (theWhat == "zoolib:ShownState")
			{
			this->Internal_ReportShownStateChange();
			return true;
			}
		else if (theWhat == "zoolib:Activate")
			{
			this->Internal_ReportActiveChange();
			return true;
			}
		else if (theWhat == "zoolib:AppActivate")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingAppActive)
				{
				fPendingAppActive = false;
				if (fReportedAppActive != fRecordedAppActive)
					{
					fReportedAppActive = fRecordedAppActive;
					ZMessage theMessage = inMessage;
					theMessage.SetBool("active", fReportedAppActive);
					locker.Release();
					this->DispatchMessageToOwner(theMessage);
					}
				}
			return true;
			}
		else if (theWhat == "zoolib:FrameChange")
			{
			this->Internal_ReportFrameChange();
			return true;
			}
		else if (theWhat == "zoolib:Close")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingCloseBoxHit)
				{
				fPendingCloseBoxHit = false;
				locker.Release();
				this->DispatchMessageToOwner(inMessage);
				}
			return true;
			}
		else if (theWhat == "zoolib:Zoom")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingZoomBoxHit)
				{
				fPendingZoomBoxHit = false;
				locker.Release();
				this->DispatchMessageToOwner(inMessage);
				}
			return true;
			}
		else if (theWhat == "zoolib:MouseState")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingMouseState)
				{
				fPendingMouseState = false;
				// First, let's check for entry/exit which require their own special messages
				if (fRecordedMouseState.fContained != fReportedMouseState.fContained)
					{
					ZMessage entryExitMessage = sMouseStateToMessage(fRecordedMouseState,
						fRecordedMouseStateWhen);

					if (fRecordedMouseState.fContained)
						entryExitMessage.SetString("what", "zoolib:MouseEnter");
					else
						entryExitMessage.SetString("what", "zoolib:MouseExit");

					fReportedMouseState = fRecordedMouseState;
					fReportedMouseStateWhen = fRecordedMouseStateWhen;
					locker.Release();
					this->DispatchMessageToOwner(entryExitMessage);
					}
				else
					{
					// Let's see if there's been any other change, which get reported as generic
					// MouseChange messages
					bool anyChange = false;

					// If the mouse is captured, then we report the mouse state if the timestamps
					// are different. The rest of the contents could be the same, but we need the
					// continuous stream to have continuous tracking.
					if (fRecordedMouseState.fCaptured
						&& fRecordedMouseStateWhen > fReportedMouseStateWhen)
						{
						anyChange = true;
						}
					else
						{
						anyChange = sMouseStatesDiffer(fRecordedMouseState, fReportedMouseState);
						}

					fReportedMouseState = fRecordedMouseState;
					fReportedMouseStateWhen = fRecordedMouseStateWhen;
					if (anyChange)
						{
						ZMessage theMessage = sMouseStateToMessage(fRecordedMouseState,
							fRecordedMouseStateWhen);
						theMessage.SetString("what", "zoolib:MouseChange");
						locker.Release();
						this->DispatchMessageToOwner(theMessage);
						}
					}
				}
			return true;
			}
		else if (theWhat == "zoolib:TitleIcon")
			{
			if (!fOwner || !fOwner->OwnerTitleIconHit(inMessage.GetPoint("where")))
				this->Internal_TitleIconClick(inMessage);
			return true;
			}
		}
	return false;
	}

ZShownState ZOSWindow_Std::GetShownState()
	{
	ASSERTLOCKED();
	return fReportedShownState;
	}

ZPoint ZOSWindow_Std::GetLocation()
	{
	ASSERTLOCKED();
	return fReportedFrame.TopLeft();
	}

ZPoint ZOSWindow_Std::GetSize()
	{
	ASSERTLOCKED();
	return fReportedFrame.Size();
	}

ZRect ZOSWindow_Std::GetFrame()
	{
	ASSERTLOCKED();
	return fReportedFrame;
	}

template<typename T> T abs(T iVal) { return T(abs(int(iVal))); }

ZRect ZOSWindow_Std::GetFrameNonZoomed()
	{
	ASSERTLOCKED();

	ZRect myFrame = this->GetFrame();
	int32 screenIndex = this->GetScreenIndex(myFrame, true);
	ZRect screenFrame = this->GetScreenFrame(screenIndex, true);

	ZPoint topLeftInset, bottomRightInset;
	this->GetContentInsets(topLeftInset, bottomRightInset);
	screenFrame.left += topLeftInset.h;
	screenFrame.top += topLeftInset.v;
	screenFrame.right += bottomRightInset.h;
	screenFrame.bottom += bottomRightInset.v;

	// See if we're within 5 pixels of the screen on all edges
	static ZCoord kNear = 5;
	bool zoomedOut = true;
	if (abs(myFrame.left - screenFrame.left) > kNear)
		zoomedOut = false;
	if (abs(myFrame.top - screenFrame.top) > kNear)
		zoomedOut = false;
	if (abs(myFrame.right - screenFrame.right) > kNear)
		zoomedOut = false;
	if (abs(myFrame.bottom - screenFrame.bottom) > kNear)
		zoomedOut = false;
	if (zoomedOut)
		{
		if (fPreZoomFrameKnown)
			return fPreZoomFrame;
		}
	return myFrame;
	}

void ZOSWindow_Std::Zoom()
	{
	ASSERTLOCKED();
	ZRect myFrame = this->GetFrame();
	int32 screenIndex = this->GetScreenIndex(myFrame, true);
	ZRect screenFrame = this->GetScreenFrame(screenIndex, true);

	ZPoint topLeftInset, bottomRightInset;
	this->GetContentInsets(topLeftInset, bottomRightInset);
	screenFrame.left += topLeftInset.h;
	screenFrame.top += topLeftInset.v;
	screenFrame.right += bottomRightInset.h;
	screenFrame.bottom += bottomRightInset.v;

	// See if we're within 5 pixels of the screen on all edges
	static ZCoord kNear = 5;
	bool zoomedOut = true;
	if (abs(myFrame.left - screenFrame.left) > kNear)
		zoomedOut = false;
	if (abs(myFrame.top - screenFrame.top) > kNear)
		zoomedOut = false;
	if (abs(myFrame.right - screenFrame.right) > kNear)
		zoomedOut = false;
	if (abs(myFrame.bottom - screenFrame.bottom) > kNear)
		zoomedOut = false;
	if (zoomedOut)
		{
		if (fPreZoomFrameKnown)
			this->SetFrame(fPreZoomFrame);
		}
	else
		{
		fPreZoomFrame = myFrame;
		fPreZoomFrameKnown = true;
		this->SetFrame(screenFrame);
		}
	}


bool ZOSWindow_Std::GetActive()
	{
	ASSERTLOCKED();
	return fReportedActive;
	}

ZPoint ZOSWindow_Std::GetOrigin()
	{
	ASSERTLOCKED();
	return ZPoint::sZero;
	}

void ZOSWindow_Std::PoseModal(bool inRunCallerMessageLoopNested, bool* outCallerExists,
	bool* outCalleeExists)
	{
	ASSERTLOCKED();

	// We return true in outCalleeExists if we still exist and are locked when this ends. We
	// return false if we got deleted before EndPoseModal was called.

	// It's an error to call PoseModal on a window more than once.
	ZAssertStop(1, fPoseModal_CallerLooper == nil
					&& fPoseModal_CallerLooperRunNestedMessageLoop == nil
					&& fPoseModal_Semaphore == nil);

	// If we were called from the thread for a looper, we need to recursively run *its* message
	// loop, but have control over exiting that loop be held by the window that has PoseModal
	// called on it (ie this window). In addition, it must also be possible to delete either or
	// both of the calling window and the PoseModal window.
	if (ZMessageLooperImpStd* looperForCurrentThread
		= ZMessageLooperImpStd::sLooperForCurrentThread())
		{
		// It is an error to call PoseModal on a window from that window's message loop thread.
		// And why would you need to?
		ZAssertStop(1, this != looperForCurrentThread);

		fPoseModal_CallerLooper = looperForCurrentThread;
		bool theRunFlag = true;
		fPoseModal_CallerLooperRunNestedMessageLoop = &theRunFlag;

		// We're about to release our message dispatch mutex. Remember the address of the local
		// variable calleeExists, so that if our main thread deletes us, we can know about it.
		bool calleeExists = true;
		fPoseModal_CalleeExistsAddress = &calleeExists;
		int32 ourMutexCount = fMutex_MessageDispatch.FullRelease();

		bool callerExists = looperForCurrentThread->RunNestedMessageLoop(&theRunFlag);
		if (outCallerExists)
			*outCallerExists = callerExists;
		if (outCalleeExists)
			*outCalleeExists = calleeExists;

		// Now, deal with this window, the one that had PoseModal called on it. We shoved the
		// address of our local variable calleeExists into our fPoseModal_CalleeExistsAddress
		// instance variable. 
		if (calleeExists)
			{
			if (fMutex_MessageDispatch.FullAcquire(ourMutexCount) == ZThread::errorNone)
				{
				fPoseModal_CalleeExistsAddress = nil;
				if (!callerExists)
					{
					fPoseModal_CallerLooperRunNestedMessageLoop = nil;
					fPoseModal_CallerLooper = nil;
					}
				// Let our destructor know we're done.
				fCondition_MessageDispatch.Broadcast();
				return;
				}
			ZDebugStopf(0, ("Should not get here"));
			}
		}
	else
		{
		// This version is similar in spirit to the above, but dramatically simpler in detail.
		bool calleeExists = true;
		fPoseModal_CalleeExistsAddress = &calleeExists;

		ZSemaphore thePoseModalSemaphore;
		fPoseModal_Semaphore = &thePoseModalSemaphore;

		int32 ourMutexCount = this->fMutex_MessageDispatch.FullRelease();

		thePoseModalSemaphore.Wait(1);

		if (outCallerExists)
			*outCallerExists = true;
		if (outCalleeExists)
			*outCalleeExists = calleeExists;

		if (calleeExists)
			{
			if (fMutex_MessageDispatch.FullAcquire(ourMutexCount) == ZThread::errorNone)
				{
				fPoseModal_CalleeExistsAddress = nil;
				// Let our destructor know we're done.
				fCondition_MessageDispatch.Broadcast();
				return;
				}
			ZDebugStopf(0, ("Should not get here"));
			}
		}
	}

void ZOSWindow_Std::EndPoseModal()
	{
	ASSERTLOCKED();

	if (fPoseModal_CallerLooperRunNestedMessageLoop)
		{
		ZMessageLooperImpStd* theCallerLooper = fPoseModal_CallerLooper;
		fPoseModal_CallerLooper = nil;
		theCallerLooper->ExitNestedMessageLoop(fPoseModal_CallerLooperRunNestedMessageLoop);
		fPoseModal_CallerLooperRunNestedMessageLoop = nil;
		}
	else
		{
		ZAssertStop(1, fPoseModal_Semaphore);
		fPoseModal_Semaphore->Signal(1);
		fPoseModal_Semaphore = nil;
		}
	}

bool ZOSWindow_Std::WaitingForPoseModal()
	{
	ASSERTLOCKED();
	return this->GetMessageLoopNesting() > 1;
	}

ZPoint ZOSWindow_Std::ToGlobal(const ZPoint& inWindowPoint)
	{
	ASSERTLOCKED();
	return inWindowPoint + fReportedFrame.TopLeft();
	}

bool ZOSWindow_Std::WaitForMouse()
	{
	ASSERTLOCKED();
	// First determine if the mouse button was reported as being down
	ZMutexLocker lockerStructure(fMutex_Structure);
	bool buttonDown = false;
	for (size_t x = 0; x < 5; ++x)
		{
		if (fReportedMouseState.fButtons[x])
			buttonDown = true;
		}
	if (!buttonDown)
		return false;

	bigtime_t endTime = ZTicks::sNow() + ZTicks::sDoubleClick();

	while (ZTicks::sNow() < endTime)
		{
		// Look through the message queue to see if there is a pending
		// mouse up, if so thenreturn false.
		for (deque<ZMessage>::iterator i = fMessageQueue.begin(); i != fMessageQueue.end(); ++i)
			{
			if ((*i).GetString("what") == "zoolib:MouseUp")
				return false;
			}
		// Look through the message queue to see if there is a pending mouse motion, if so then
		// check to see whether our reported and recorded mouse states match.
		bool anyMouseMotion = false;
		for (deque<ZMessage>::iterator i = fMessageQueue.begin();
			i != fMessageQueue.end() && !anyMouseMotion; ++i)
			{
			if ((*i).GetString("what") == "zoolib:MouseState")
				anyMouseMotion = true;
			}
		if (anyMouseMotion && sMouseStatesDiffer(fRecordedMouseState, fReportedMouseState))
			return true;

		// Wait till the double click time has expired, or a message gets posted.
		fCondition_Structure.Wait(fMutex_Structure, endTime - ZTicks::sNow());
		}

	// We've made it past the double click time with no
	// mouse motion or mouse ups, so just return true.
	return true;
	}

void ZOSWindow_Std::Internal_Update(const ZDCRgn& inUpdateRgn)
	{
	if (fOwner)
		{
		ZDC theDC(this->GetCanvas(), ZPoint::sZero);
		theDC.SetClip(inUpdateRgn);
		if (sFillUpdates)
			{
			theDC.SetInk(ZRGBColor::sBlue);
			theDC.Fill(inUpdateRgn);
			}
		theDC.SetInk(ZDCInk());

		fOwner->OwnerUpdate(theDC);
		theDC.Flush();
		}
	}

void ZOSWindow_Std::Internal_TitleIconClick(const ZMessage& inMessage)
	{}

void ZOSWindow_Std::Internal_RecordIdle()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingIdle)
		{
		fPendingIdle = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Idle");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_RecordInvalidation(const ZDCRgn& inBadRgn)
	{
	if (sFillInvalidates)
		{
		ZMutexLocker lockerDispatch(fMutex_MessageDispatch);
		ZDC theDC(this->GetCanvas(), ZPoint::sZero);
		theDC.SetClip(inBadRgn);
		theDC.SetInk(ZRGBColor::sRed);
		theDC.Fill(inBadRgn);
		}

	ZMutexLocker locker(fMutex_Structure);
	bool wasEmpty = !fInvalidRgn;
	fInvalidRgn |= inBadRgn;
	if (wasEmpty && fInvalidRgn)
		{
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Invalid");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_ReportInvalidation()
	{
	ASSERTLOCKED();
	ZMutexLocker locker(fMutex_Structure);
	if (fInvalidRgn)
		{
		ZDCRgn theUpdateRgn = fInvalidRgn;
		fInvalidRgn.Clear();
		locker.Release();
		this->Internal_Update(theUpdateRgn);
		}
	}

void ZOSWindow_Std::Internal_RecordShownStateChange(ZShownState inNewShownState)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedShownState = inNewShownState;
	if (!fPendingShownState)
		{
		fPendingShownState = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:ShownState");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_ReportShownStateChange()
	{
	ASSERTLOCKED();
	ZMutexLocker locker(fMutex_Structure);
	if (fPendingShownState)
		{
		fPendingShownState = false;
		if (fReportedShownState != fRecordedShownState)
			{
			fReportedShownState = fRecordedShownState;
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:ShownState");
			theMessage.SetInt32("shownState", fReportedShownState);
			locker.Release();
			this->DispatchMessageToOwner(theMessage);
			}
		}
	}

void ZOSWindow_Std::Internal_RecordActiveChange(bool inNewActive)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedActive = inNewActive;
	if (!fPendingActive)
		{
		fPendingActive = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Activate");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_ReportActiveChange()
	{
	ASSERTLOCKED();
	ZMutexLocker locker(fMutex_Structure);
	if (fPendingActive)
		{
		fPendingActive = false;
		if (fReportedActive != fRecordedActive)
			{
			fReportedActive = fRecordedActive;
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:Activate");
			theMessage.SetBool("active", fReportedActive);
			locker.Release();
			this->DispatchMessageToOwner(theMessage);
			}
		}
	}

void ZOSWindow_Std::Internal_RecordAppActiveChange(bool inNewActive)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedAppActive = inNewActive;
	if (!fPendingAppActive)
		{
		fPendingAppActive = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:AppActivate");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_RecordSizeChange(ZPoint inNewSize)
	{
	ZMutexLocker locker(fMutex_Structure);
	ZRect newFrame = inNewSize;
	newFrame += fRecordedFrame.TopLeft();
	this->Internal_RecordFrameChange(newFrame);
	}

void ZOSWindow_Std::Internal_RecordLocationChange(ZPoint inNewLocation)
	{
	ZMutexLocker locker(fMutex_Structure);
	ZRect newFrame = fRecordedFrame;
	newFrame += inNewLocation - fRecordedFrame.TopLeft();
	this->Internal_RecordFrameChange(newFrame);
	}

void ZOSWindow_Std::Internal_RecordFrameChange(const ZRect& inNewFrame)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedFrame = inNewFrame;
	if (!fPendingFrame)
		{
		fPendingFrame = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:FrameChange");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_ReportFrameChange()
	{
	ASSERTLOCKED();
	ZMutexLocker locker(fMutex_Structure);
	if (fPendingFrame)
		{
		fPendingFrame = false;
		if (fReportedFrame != fRecordedFrame)
			{
			fReportedFrame = fRecordedFrame;
			locker.Release();
			ZMessage theMessage;
			theMessage.SetString("what", "zoolib:FrameChange");
			this->DispatchMessageToOwner(theMessage);
			}
		}
	}

void ZOSWindow_Std::Internal_RecordCloseBoxHit()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingCloseBoxHit)
		{
		fPendingCloseBoxHit = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Close");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_RecordZoomBoxHit()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingZoomBoxHit)
		{
		fPendingZoomBoxHit = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Zoom");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_RecordMouseDown(int inButton,
	const MouseState& inMouseState, bigtime_t inWhen)
	{
	ZMutexLocker locker(fMutex_Structure);
	if (inWhen - fTimeOfLastClick > ZTicks::sDoubleClick()
		|| abs(fClickLocation.h - inMouseState.fLocation.h) > 3
		|| abs(fClickLocation.v - inMouseState.fLocation.v) > 3)
		fClickCount = 1;
	else
		++fClickCount;

	fTimeOfLastClick = inWhen;
	fClickLocation = inMouseState.fLocation;

	ZMessage theMessage = sMouseStateToMessage(inMouseState, inWhen);

	if (fRecordedMouseState.fContained != fReportedMouseState.fContained)
		{
		ZMessage entryExitMessage = theMessage;
		if (fRecordedMouseState.fContained)
			entryExitMessage.SetString("what", "zoolib:MouseEnter");
		else
			entryExitMessage.SetString("what", "zoolib:MouseExit");
		this->QueueMessageForOwner(entryExitMessage);
		}

	theMessage.SetString("what", "zoolib:MouseDown");

	theMessage.SetInt32("clicks", fClickCount);

	this->QueueMessageForOwner(theMessage);

	fRecordedMouseState = inMouseState;
	fReportedMouseState = inMouseState;
	fRecordedMouseStateWhen = inWhen;
	fReportedMouseStateWhen = inWhen;
	fPendingMouseState = false;
	}

void ZOSWindow_Std::Internal_RecordMouseUp(const MouseState& inMouseState, bigtime_t inWhen)
	{
	ZMutexLocker locker(fMutex_Structure);

	ZMessage theMessage = sMouseStateToMessage(inMouseState, inWhen);

	if (fRecordedMouseState.fContained != fReportedMouseState.fContained)
		{
		ZMessage entryExitMessage = theMessage;
		if (fRecordedMouseState.fContained)
			entryExitMessage.SetString("what", "zoolib:MouseEnter");
		else
			entryExitMessage.SetString("what", "zoolib:MouseExit");
		this->QueueMessageForOwner(entryExitMessage);
		}

	theMessage.SetString("what", "zoolib:MouseUp");

	this->QueueMessageForOwner(theMessage);

	fRecordedMouseState = inMouseState;
	fRecordedMouseStateWhen = inWhen;
	fReportedMouseState = fRecordedMouseState;
	fReportedMouseStateWhen = fRecordedMouseStateWhen;
	fPendingMouseState = false;
	}

void ZOSWindow_Std::Internal_RecordMouseState(const MouseState& inMouseState, bigtime_t inWhen)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedMouseState = inMouseState;
	fRecordedMouseStateWhen = inWhen;
	if (!fPendingMouseState)
		{
		fPendingMouseState = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:MouseState");
		this->QueueMessage(theMessage);
		}
	}

void ZOSWindow_Std::Internal_TickleMouse(bigtime_t inWhen)
	{
	ZMutexLocker locker(fMutex_Structure);
	fRecordedMouseState.fCaptured = true;
	fRecordedMouseStateWhen = inWhen;
	if (!fPendingMouseState)
		{
		fPendingMouseState = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:MouseState");
		this->QueueMessage(theMessage);
		}
	}

// Needed for Win32 WM_ERASEBACKGROUND
bool ZOSWindow_Std::Internal_GetReportedActive()
	{
	ZMutexLocker locker(fMutex_Structure);
	return fReportedActive;
	}

ZDCRgn ZOSWindow_Std::Internal_GetInvalidRgn()
	{
	ZAssertLocked(1, fMutex_Structure);
	return fInvalidRgn;
	}

void ZOSWindow_Std::sRunMessageLoop(ZOSWindow_Std* inOSWindow)
	{
	inOSWindow->RunMessageLoop(nil);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSWindowRoster_Std

class ZOSWindowRoster_Std : public ZOSWindowRoster
	{
public:
	ZOSWindowRoster_Std(ZOSApp_Std* inOSApp);
	virtual ~ZOSWindowRoster_Std();

	virtual void Reset();
	virtual bool GetNextOSWindow(bigtime_t inTimeout, ZOSWindow*& outOSWindow);
	virtual void DropCurrentOSWindow();
	virtual void UnlockCurrentOSWindow();

	void OSWindowRemoved(ZOSWindow_Std* inOSWindow);

protected:
	ZOSApp_Std* fOSApp;
	vector<ZOSWindow_Std*> fVector_OSWindowsVisited;
	vector<ZOSWindow_Std*> fVector_OSWindowsDropped;
	ZOSWindow_Std* fCurrentOSWindow;
	ZOSWindowRoster_Std* fNext;
	ZOSWindowRoster_Std* fPrev;
	friend class ZOSApp_Std;
	};

ZOSWindowRoster_Std::ZOSWindowRoster_Std(ZOSApp_Std* inOSApp)
	{
	fOSApp = inOSApp;
	fCurrentOSWindow = nil;

	if (fOSApp->fOSWindowRoster_Head)
		fOSApp->fOSWindowRoster_Head->fPrev = this;
	fNext = fOSApp->fOSWindowRoster_Head;
	fPrev = nil;
	fOSApp->fOSWindowRoster_Head = this;
	}

ZOSWindowRoster_Std::~ZOSWindowRoster_Std()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fNext)
				fNext->fPrev = fPrev;
			if (fPrev)
				fPrev->fNext = fNext;
			if (fOSApp->fOSWindowRoster_Head == this)
				fOSApp->fOSWindowRoster_Head = fNext;
			fNext = nil;
			fPrev = nil;
			}
		}
	}

void ZOSWindowRoster_Std::Reset()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			fVector_OSWindowsVisited.clear();
		}
	}

bool ZOSWindowRoster_Std::GetNextOSWindow(bigtime_t inTimeout, ZOSWindow*& outOSWindow)
	{
	outOSWindow = nil;
	if (fOSApp)
		{
		if (inTimeout < 0)
			inTimeout = 0;
		bigtime_t endTime = ZTicks::sNow() + inTimeout;
		while (true)
			{
			ZMutexLocker locker(fOSApp->fMutex_Structure, false);
			if (!locker.IsOkay())
				break;
			ZOSWindow_Std* theOSWindow_Std = nil;
			if (fOSApp->Internal_GetNextOSWindow(fVector_OSWindowsVisited,
				fVector_OSWindowsDropped, 10000, theOSWindow_Std))
				{
				if (theOSWindow_Std)
					{
					ZUtil_STL::sPushBackMustNotContain(1, fVector_OSWindowsVisited, theOSWindow_Std);
					fCurrentOSWindow = theOSWindow_Std;
					}
				outOSWindow = theOSWindow_Std;
				break;
				}
			if (ZTicks::sNow() >= endTime)
				return false;
			}
		}
	return true;
	}

void ZOSWindowRoster_Std::DropCurrentOSWindow()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fCurrentOSWindow)
				{
				// Ensure we're not double droppping a window
				ZUtil_STL::sPushBackMustNotContain(1, fVector_OSWindowsDropped, fCurrentOSWindow);
				}
			}
		}
	}

void ZOSWindowRoster_Std::UnlockCurrentOSWindow()
	{
	if (fOSApp)
		{
		ZMutexLocker locker(fOSApp->fMutex_Structure, false);
		if (locker.IsOkay())
			{
			if (fCurrentOSWindow)
				fCurrentOSWindow->GetLock().Release();
			}
		}
	}

void ZOSWindowRoster_Std::OSWindowRemoved(ZOSWindow_Std* inOSWindow)
	{
	vector<ZOSWindow_Std*>::iterator theIter;

	ZUtil_STL::sEraseIfContains(fVector_OSWindowsVisited, inOSWindow);
	ZUtil_STL::sEraseIfContains(fVector_OSWindowsDropped, inOSWindow);

	if (inOSWindow == fCurrentOSWindow)
		fCurrentOSWindow = nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZOSApp_Std

ZOSApp_Std::ZOSApp_Std()
	{
	fOSWindowRoster_Head = nil;

	fPendingIdle = false;
	fPendingWindowRosterChange = false;
	}

ZOSApp_Std::~ZOSApp_Std()
	{
	// We must end life with our message dispatch mutex held
	ZAssertLocked(1, fMutex_MessageDispatch);

	ZMutexLocker structureLocker(fMutex_Structure);
	while (fOSWindowRoster_Head)
		{
		fOSWindowRoster_Head->fOSApp = nil;
		fOSWindowRoster_Head = fOSWindowRoster_Head->fNext;
		}
	}

void ZOSApp_Std::DispatchMessageImp(const ZMessage& inMessage)
	{ this->DispatchMessage(inMessage); }

ZMutexBase& ZOSApp_Std::GetLock()
	{ return fMutex_MessageDispatch; }

void ZOSApp_Std::QueueMessage(const ZMessage& inMessage)
	{ this->QueueMessageImp(inMessage); }

bool ZOSApp_Std::DispatchMessage(const ZMessage& inMessage)
	{
	if (ZOSApp::DispatchMessage(inMessage))
		return true;

	if (inMessage.Has("what"))
		{
		string theWhat = inMessage.GetString("what");
		if (theWhat == "zoolib:Idle")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingIdle)
				{
				fPendingIdle = false;
				locker.Release();
				this->DispatchMessageToOwner(inMessage);
				}
			return true;
			}
		else if (theWhat == "zoolib:WindowRosterChange")
			{
			ZMutexLocker locker(fMutex_Structure);
			if (fPendingWindowRosterChange)
				{
				fPendingWindowRosterChange = false;
				locker.Release();
				this->DispatchMessageToOwner(inMessage);
				}
			return true;
			}
		}
	return false;
	}

void ZOSApp_Std::Run()
	{
	// We must be locked when run is called
	ZAssertStop(1, fMutex_MessageDispatch.IsLocked());

	this->PreRunMessageLoop();
	fMutex_MessageDispatch.Release();
	this->RunMessageLoop(nil);
	fMutex_MessageDispatch.Acquire();

	ZMessage runFinishedMessage;
	runFinishedMessage.SetString("what", "zoolib:RunFinished");
	this->DispatchMessageToOwner(runFinishedMessage);
	}

bool ZOSApp_Std::IsRunning()
	{
	// As IsRunning is used by ZApp when it is setting up menus in WindowSupervisorSetupMenus,
	// we don't require that we be locked -- WindowSupervisorSetupMenus is called without the
	// app's lock held, to avoid likely deadlock.
	return this->GetMessageLoopNesting() > 0;
	}

void ZOSApp_Std::ExitRun()
	{
	this->ExitMessageLoop();
	}

void ZOSApp_Std::Internal_RecordIdle()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingIdle)
		{
		fPendingIdle = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:Idle");
		this->QueueMessage(theMessage);
		}
	}

void ZOSApp_Std::Internal_RecordWindowRosterChange()
	{
	ZMutexLocker locker(fMutex_Structure);
	if (!fPendingWindowRosterChange)
		{
		fPendingWindowRosterChange = true;
		ZMessage theMessage;
		theMessage.SetString("what", "zoolib:WindowRosterChange");
		this->QueueMessage(theMessage);
		}
	}

ZOSWindowRoster* ZOSApp_Std::CreateOSWindowRoster()
	{
	ZMutexLocker structureLocker(fMutex_Structure);
	return new ZOSWindowRoster_Std(this);
	}

void ZOSApp_Std::Internal_OSWindowRemoved(ZOSWindow_Std* inOSWindow)
	{
	ZAssertLocked(1, fMutex_Structure);

	ZOSWindowRoster_Std* theRoster = fOSWindowRoster_Head;
	while (theRoster)
		{
		theRoster->OSWindowRemoved(inOSWindow);
		theRoster = theRoster->fNext;
		}
	}
