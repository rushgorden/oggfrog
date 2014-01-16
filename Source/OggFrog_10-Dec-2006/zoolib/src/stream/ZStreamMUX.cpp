static const char rcsid[] = "@(#) $Id: ZStreamMUX.cpp,v 1.14 2006/04/27 03:19:32 agreen Exp $";

/* ------------------------------------------------------------
Copyright (c) 2002 Andrew Green and Learning in Motion, Inc.
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

#include "ZStreamMUX.h"
#include "ZThreadSimple.h"
#include "ZMemory.h"

using std::deque;
using std::min;
using std::string;

#define kDebug_StreamMUX 1

/**
\class ZStreamMUX

\brief Allows one to multiplex independent sessions over a single ZStreamR/ZStreamW pair.

\note ZStreamMUX is based on some of the ideas from the moribund
<a href="http://www.w3.org/TR/WD-mux">WebMUX protocol</a>.

ZStreamMUX takes a ZStreamR and a ZStreamW and runs a lightweight protocol over
them to support multiple independent sessions. The overall interface is similar to
that provided by sockets:
\li ZStreamMUX::Listen causes the MUX to listen for sessions being opened by the
remote side. Note that we use textual names rather than numeric port IDs.
\li ZStreamMUX::Accept retrieves a session that's been opened by the other side, or
blocks waiting for such an open to happen.
\li ZStreamMUX::Open opens a session to the other side, using a name to identify which
of several ZStreamMUX::Listen/ZStreamMUX::Accept combos might be active.
\li ZStreamMUX::Close half-closes an existing session -- after this call no more data
can be sent to the other side, although the other side can continue to send data
and you can continue to receive it.
\li ZStreamMUX::Abort immediately tears down a session in both directions. Pending
receives and sends return with errorAbortLocal or errorAbortRemote (as appropriate).

Sessions are identified with a SessionID. ZStreamMUX supports up to 512 outbound sessions,
for a total of 1024 sessions at once. SessionIDs themselves are 16 bit unsigned integers
where bits 0-8 distinguish one session from another, bit 14 (0x4000) is always set and
bit 13 (0x2000) indicates whether the session was initiated locally (by calling
ZStreamMUX::Open) or remotely (by calling ZStreamMUX::Accept). So valid SessionIDs
range from 0x6000 to 0x61FF (for locally-initiated sessions) and from 0x4000 to 0x41FF
(for remotely-initiated sessions). Note that zero is never a valid sessionID.

\par Wire Protocol Summary

\verbatim
S = Session ID (full, possibly of limited range)
T = Session ID (near/far implied, again possibly with a limited range)
W = Near/Far session 1 == sender's notion of far (receiver's near)
                     0 == receiver's notion of far (sender's near)
L = Length of payload that follows
K = Length minus one of payload that follows
P = Listener ID
C = Credit (usually must be multiplied by some constant)
_ = Not determined

Payload
  11WS SLLL   .....
  11WS S111   KKKK KKKK   .....
  11WS S111   1111 1111   KKKK KKKK   KKKK KKKK .....

  101W SSSS   KKKK KKKK   .....
  101W SSSS   1111 1111   KKKK KKKK   KKKK KKKK .....

  100W SSSS   SSSS SLLL   .....
  100W SSSS   SSSS S111   KKKK KKKK   .....
  100W SSSS   SSSS S111   1111 1111   KKKK KKKK   KKKK KKKK   .....

Payload SYN
  01TT TTPP   PPPK KKKK   .....

Add credit
  0011 WTTT   TTCC CCCC
  0010 WTTT   TTTT TTCC   CCCC CCCC

SYN (restricted range for session and listener)
  0001 1TTT   TTPP PPPP

FIN
  0001 01WS   SSSS SSSS

RST
  0001 00WS   SSSS SSSS

SYN (unrestricted range)
  0000 111T   TTTT TTTT   PPPP PPPP

SYNACK
  0000 110T   TTTT TTTT

Define Listener
  0000 1011   PPPP PPPP   LLLL LLLL   .....
  0000 1010   PPPP PPPP   LLLL LLLL   LLLL LLLL   .....

UNUSED
  0000 100_ = 2 different sequences
  0000 0___ = 8 different sequences
  There are ten unused 8 bit sequences available for future use.

\endverbatim
*/

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamMUX

ZStreamMUX::ZStreamMUX(const ZStreamR& iStreamR, const ZStreamW& iStreamW)
:	fStreamR(iStreamR),
	fStreamW(iStreamW)
	{
	fSem_Disposed = nil;
	fExitedSend = true;
	fExitedReceive = true;

	// We assume the other side can accept up to 16K on
	// new sessions before we need a credit.
	fDefaultCredit = 16 * 1024;

	// This is the largest size fragment we'll send
	fMaxFragmentSize = 2048;

	fSessions_Head = nil;
	fSessions_Tail = nil;
	fSessionNextSend = nil;

	fPendingSYN_Head = nil;
	fPendingSYN_Tail = nil;

	fWaiters_Open_Head = nil;
	fWaiters_Open_Tail = nil;
	fWaiters_OpenSYNSent = nil;

	fWaiters_RST_Head = nil;
	fWaiters_RST_Tail = nil;

	fActiveListens = nil;
	fActiveListens_RST = nil;

	fWaiters_Accept_Head = nil;
	fWaiters_Accept_Tail = nil;

	fListenNames_Send = nil;
	fListenNames_Receive = nil;

	// Preallocate send listeners
	for (PrivID currentID = 0; currentID < 256; ++currentID)
		{
		ListenName* newListener = new ListenName;
		newListener->fID = currentID;
		newListener->fNext = fListenNames_Send;
		fListenNames_Send = newListener;
		}

	fListenNames_Receive = nil;

	fActiveListens = nil;
	}

ZStreamMUX::~ZStreamMUX()
	{
	ZAssertStop(kDebug_StreamMUX, fExitedSend && fExitedReceive);

	while (fListenNames_Send)
		{
		ListenName* nextListenName = fListenNames_Send->fNext;
		delete fListenNames_Send;
		fListenNames_Send = nextListenName;
		}
	}


/**
Establish a multiplex connection using fStreamR and fStreamW to communicate. The ZStreamR and
ZStreamW need only support the normal facilities, and will typically be the read/write interfaces
of a single ZNetEndpoint (which is of course a ZStreamerRW). However it will likely be
advantageous to wrap a ZStreamR_Buffered/ZStreamW_Buffered/ZStreamRW_FlushOnRead
combo around raw endpoint streams in order to maxmize the size of read/writes made to the OS.
*/

void ZStreamMUX::Startup()
	{
	ZAssertStop(kDebug_StreamMUX, fExitedSend && fExitedReceive);

	fExitedSend = false;
	fExitedReceive = false;

	(new ZThreadSimple<ZStreamMUX*>(sRunSend, this))->Start();
	(new ZThreadSimple<ZStreamMUX*>(sRunReceive, this))->Start();
	}


/**
Tear down the multiplexed connection. No exchange of information occurs, we
just stop work and dispose working storage. You'll need to use some external
technique to ensure that all sessions have finished.
*/

void ZStreamMUX::Shutdown()
	{	
	fMutex.Acquire();

	// Cause RunSend and RunReceive to exit, if they hadn't already done so unexpectedly.
	int semCount = 0;
	if (!fExitedSend)
		++semCount;
	if (!fExitedReceive)
		++semCount;
	if (semCount)
		{
		ZSemaphore semDisposed;
		fSem_Disposed = &semDisposed;
		fCondition_Send.Broadcast();
		fMutex.Release();
		// And wait for them to exit.
		semDisposed.Wait(semCount);
		fMutex.Acquire();
		fSem_Disposed = nil;
		}

	// Wake up all Waiter_RSTs
	while (Waiter_RST* theWaiter = fWaiters_RST_Head)
		{
		// Pull it off the head.
		if (theWaiter->fNext)
			{
			fWaiters_RST_Head = theWaiter->fNext;
			theWaiter->fNext = nil;
			}
		else
			{
			fWaiters_RST_Head = nil;
			fWaiters_RST_Tail = nil;
			}

		theWaiter->fDoneIt = true;
		theWaiter->fSession->fCondition_Send.Broadcast();
		}

	// Wake up all Waiter_Opens
	while (Waiter_Open* theWaiter_Open = fWaiters_Open_Head)
		{
		// Unlink it
		if (theWaiter_Open->fNext == nil)
			{
			fWaiters_Open_Head = nil;
			fWaiters_Open_Tail = nil;
			}
		else
			{
			fWaiters_Open_Head = theWaiter_Open->fNext;
			}
		theWaiter_Open->fResult = errorDisposed;
		}

	// Wake up all Waiter_Opens where SYN has been sent
	while (Waiter_Open* theWaiter_Open = fWaiters_OpenSYNSent)
		{
		fWaiters_OpenSYNSent = theWaiter_Open->fNext;
		theWaiter_Open->fNext = nil;
		theWaiter_Open->fResult = errorDisposed;
		}

	// Dispose all SYNReceived
	while (SYNReceived* theSYNReceived = fPendingSYN_Head)
		{
		fPendingSYN_Head = theSYNReceived->fNext;
		delete[] theSYNReceived->fBuffer;
		delete theSYNReceived;
		}
	fPendingSYN_Tail = nil;

	// Dispose listens
	while (ActiveListen* currListen = fActiveListens)
		{
		fActiveListens = currListen->fNext;
		while (Waiter_Accept* currWaiter = currListen->fWaiters_Head)
			{
			currListen->fWaiters_Head = currWaiter->fNext;
			currWaiter->fResult = errorDisposed;
			currWaiter->fNext = nil;
			}
		delete currListen;
		}

	// Dispose listens that got cancelled with pending sessions but have
	// not had their RSTs sent yet
	while (ActiveListen* currListen = fActiveListens_RST)
		{
		fActiveListens_RST = currListen->fNext;
		ZAssertStop(kDebug_StreamMUX, currListen->fWaiters_Head == nil);
		ZAssertStop(kDebug_StreamMUX, currListen->fWaiters_Tail == nil);
		for (deque<SessionCB*>::iterator i = currListen->fPendingSessions.begin();
			i != currListen->fPendingSessions.end(); ++i)
			{
			this->ReleaseSession(*i);
			}
		delete currListen;
		}

	// Abort all extant sessions
	while (SessionCB* theSession = fSessions_Head)
		{
		this->UseSession(theSession);
		this->AbortSession(theSession);
		this->ReleaseSession(theSession);
		}

	fCondition_Accept.Broadcast();
	fCondition_Send.Broadcast();
	fCondition_Open.Broadcast();
	fMutex.Release();

	while (fListenNames_Receive)
		{
		ListenName* nextListenName = fListenNames_Receive->fNext;
		delete fListenNames_Receive;
		fListenNames_Receive = nextListenName;
		}
	}


/**
Start listening for sessions.
\param iListenName The name to listen for.
\return \li \c errorNone: Success, the other side can call Open with this listener name.
\return \li \c errorAlreadyListening: ZStreamMUX::Listen has already been called for
\a iListenName.
\return \li \c errorBadListenName: iListenName was illegal (zero length).
*/

ZStreamMUX::Error ZStreamMUX::Listen(const string& iListenName)
	{
	if (iListenName.size() == 0)
		return errorBadListenName;

	ZMutexLocker locker(fMutex);
	ActiveListen* currListen = fActiveListens;
	while (currListen && currListen->fListenName != iListenName)
		currListen = currListen->fNext;

	if (currListen)
		return errorAlreadyListening;

	ActiveListen* newListen = new ActiveListen;
	newListen->fListenName = iListenName;
	newListen->fWaiters_Head = nil;
	newListen->fWaiters_Tail = nil;
	newListen->fNext = fActiveListens;
	fActiveListens = newListen;
	return errorNone;
	}


/**
Stop listening for sessions.
\param iListenName The name of the Listener to stop listening for.

\return \li \c errorNone Success, any further attempt by the other side to connect
to this listener name will fail.
\return \li \c errorNotListening ZStreamMUX::Listen has not previously been called for
\a iListenName or ZStreamMUX::StopListen has since been called.
*/

ZStreamMUX::Error ZStreamMUX::StopListen(const string& iListenName)
	{
	if (iListenName.size() == 0)
		return errorBadListenName;

	ZMutexLocker locker(fMutex);

	ActiveListen* prevListen = nil;
	ActiveListen* currListen = fActiveListens;
	while (currListen && currListen->fListenName != iListenName)
		{
		prevListen = currListen;
		currListen = currListen->fNext;
		}

	if (!currListen)
		return errorNotListening;

	// Remove it from our list of listens
	if (prevListen)
		prevListen->fNext = currListen->fNext;
	else
		fActiveListens = currListen->fNext;

	// Have to cancel any waiting accepts.
	Waiter_Accept* currWaiter = currListen->fWaiters_Head;
	while (currWaiter)
		{
		currWaiter->fResult = errorNotListening;
		Waiter_Accept* nextWaiter = currWaiter->fNext;
		currWaiter->fResult = errorNotListening;
		currWaiter->fNext = nil;
		currWaiter = nextWaiter;
		}

	fCondition_Accept.Broadcast();

	// If we've got any pending sessions
	if (currListen->fPendingSessions.size())
		{
		for (deque<SessionCB*>::iterator i = currListen->fPendingSessions.begin();
			i != currListen->fPendingSessions.end(); ++i)
			{
			// Mark it as used (will be released by HandleSend_RST)
			this->UseSession(*i);
			// And mark it as aborted
			this->AbortSession(*i);
			}

		// And put the listen on list of that need to have RSTs sent for their pending sessions.
		currListen->fNext = fActiveListens_RST;
		fActiveListens_RST = currListen;
		}
	else
		{
		// Just dispose the listen
		delete currListen;
		}

	return errorNone;
	}


/**
Accept an incoming session, or block waiting for one.
\param iListenName Must be a name that has previously been passed to ZStreamMUX::Listen.
\param oSessionID The ID of the accepted session.

\return \li \c errorNone: The Accept succeeded, the session is in \a oSessionID.
\return \li \c errorNotListening: ZStreamMUX::Listen has not previously been called for
\a iListenName, or ZStreamMUX::StopListen has since been called.
*/

ZStreamMUX::Error ZStreamMUX::Accept(const string& iListenName, SessionID& oSessionID)
	{
	if (iListenName.size() == 0)
		return errorBadListenName;

	Error result = errorNotListening;

	// Find an active listen
	ZMutexLocker locker(fMutex);

	ActiveListen* currListen = fActiveListens;
	while (currListen && currListen->fListenName != iListenName)
		currListen = currListen->fNext;
	if (currListen)
		{
		SessionCB* theSession = nil;
		if (currListen->fPendingSessions.size())
			{
			theSession = currListen->fPendingSessions.front();
			currListen->fPendingSessions.pop_front();
			result = errorNone;
			}
		else
			{
			// Need to wait
			Waiter_Accept theWaiter;
			theWaiter.fSession = nil;
			theWaiter.fResult = waitPending;
			// Put ourselves on the list
			theWaiter.fNext = nil;
			if (currListen->fWaiters_Tail)
				currListen->fWaiters_Tail->fNext = &theWaiter;
			else
				currListen->fWaiters_Head = &theWaiter;
			currListen->fWaiters_Tail = &theWaiter;
			while (theWaiter.fResult == waitPending)
				fCondition_Accept.Wait(fMutex);

			result = theWaiter.fResult;

			theSession = theWaiter.fSession;
			}

		if (theSession)
			{
			oSessionID = sMakeSessionID(false, theSession->fID);
			this->ReleaseSession(theSession);
			}
		}
	return result;
	}


/**
Open a session to the listener named \a iListenName. Optionally also send
data with the open request -- at least some of this data is sent immediately and thus is not
subject to an additional round-trip delay.

\param iListenName The name to connect to.
\param iData If non-nil points to data to be sent as part of the session-establishment
fragment.
\param iCount Number of bytes at \a iData to send.
\param oCountSent Number of bytes that were succesfully sent with the session-establishment
fragment.
\param oSessionID The ID of the session, if open succeeds.

\return \li \c errorNone: The Open succeeded, the session is in \a oSessionID.
\return \li \c errorAbortRemote: The remote MUX was not listening for \a iListenName.
*/

ZStreamMUX::Error ZStreamMUX::Open(const string& iListenName, const void* iData, size_t iCount,
	size_t* oCountSent, SessionID& oSessionID)
	{
	ZMutexLocker locker(fMutex);

	Waiter_Open theWaiter;
	theWaiter.fListenName = iListenName;
	theWaiter.fSession = nil;
	theWaiter.fSource = reinterpret_cast<const uint8*>(iData);
	theWaiter.fCount = iCount;
	theWaiter.fResult = waitPending;
	theWaiter.fNext = nil;

	if (fWaiters_Open_Tail)
		fWaiters_Open_Tail->fNext = &theWaiter;
	else
		fWaiters_Open_Head = &theWaiter;
	fWaiters_Open_Tail = &theWaiter;

	// Wake up the send thread.
	fCondition_Send.Broadcast();

	// And wait till we're complete
	while (theWaiter.fResult == waitPending)
		fCondition_Open.Wait(fMutex);

	if (oCountSent)
		*oCountSent = iCount - theWaiter.fCount;

	if (theWaiter.fResult == errorNone)
		{
		oSessionID = sMakeSessionID(true, theWaiter.fSession->fID);
		this->ReleaseSession(theWaiter.fSession);
		}
	else
		{
		oSessionID = kInvalidSessionID;
		}

	return theWaiter.fResult;
	}


/**
Half-close a session.

\param iSessionID The session for which we no longer wish to send data. Any call to
ZStreamMUX::Send that's already blocked will send all its data. Any subsequent call
to Send will fail.

\return \li \c errorNone: All pending sends have completed and will return \a errorNone.
Any future calls to ZStreamMUX::Send will return \a errorSendClosed.
\return \li \c errorInvalidSession: The session \a iSessionID does not exist.
\return \li \c errorAbortLocal: The session was locally aborted before the half-close
could complete.
\return \li \c errorAbortRemote: The session was remotely aborted before the half-close
could complete.
*/

ZStreamMUX::Error ZStreamMUX::Close(SessionID iSessionID)
	{
	ZMutexLocker locker(fMutex);

	Error result = errorInvalidSession;
	bool theIsNear;
	PrivID thePrivID;
	sMakePrivID(iSessionID, theIsNear, thePrivID);
	if (SessionCB* theSession = this->UseSession(theIsNear, thePrivID))
		{
		if (!theSession->fOpen_Send)
			{
			result = errorSendClosed;
			while (!theSession->fAborted && !theSession->fSentFIN)
				theSession->fCondition_Send.Wait(fMutex);
			}
		else
			{
			result = errorNone;
			theSession->fOpen_Send = false;
			fCondition_Send.Broadcast();
			while (!theSession->fAborted && !theSession->fSentFIN)
				theSession->fCondition_Send.Wait(fMutex);
			}
		if (theSession->fAborted)
			result = errorAbortLocal;
		this->ReleaseSession(theSession);
		}
	return result;
	}


/**
Abort a session.

\param iSessionID The session which we want to shoot in the head. Any pending calls to
ZStreamMUX::Send, ZStreamMUX::Receive or ZStreamMUX::Close will return with \a errorAbortLocal.
All resources for the session are released.

\return \li \c errorNone: All pending sends and receives have been aborted and will
return \a errorAbortLocal.
\return \li \c errorInvalidSession: The session \a iSessionID does not exist.
*/

ZStreamMUX::Error ZStreamMUX::Abort(SessionID iSessionID)
	{
	ZMutexLocker locker(fMutex);
	Error result = errorInvalidSession;
	bool theIsNear;
	PrivID thePrivID;
	sMakePrivID(iSessionID, theIsNear, thePrivID);
	if (SessionCB* theSession = this->UseSession(theIsNear, thePrivID))
		{
		result = errorNone;
		if (!theSession->fSentFIN || theSession->fOpen_Send || theSession->fOpen_Receive)
			{
			this->AbortSession(theSession);
			Waiter_RST theWaiter;
			theWaiter.fSession = theSession;
			theWaiter.fNext = nil;
			theWaiter.fDoneIt = false;
			if (fWaiters_RST_Tail)
				{
				fWaiters_RST_Tail->fNext = &theWaiter;
				}
			else
				{
				fWaiters_RST_Head = &theWaiter;
				fWaiters_RST_Tail = &theWaiter;
				}
			fCondition_Send.Broadcast();

			while (!theWaiter.fDoneIt)
				theSession->fCondition_Send.Wait(fMutex);
			}
		this->ReleaseSession(theSession);
		}
	return result;
	}


/**
Send data to the remote side of the session.

\param iSessionID The session over which we wish to send data.
\param iSource A pointer to the data which we wish to send.
\param iCount The number of bytes to send.
\param oCountSent Optional output argument which will be updated with
the number of bytes actually sent. If an abort occurs before all data
has been sent this will hold the number that was sent prior to the abort.

\return \li \c errorNone: Data was sent, \a oCountSent holds some value between
0 and \a iCount (currently we always send everything).
\return \li \c errorInvalidSession: The session \a iSessionID does not exist.
\return \li \c errorSendClosed: ZStreamMUX::Close had previously been called for this session.
\return \li \c errorAbortLocal, \c errorAbortRemote: The session was aborted.
*/

ZStreamMUX::Error ZStreamMUX::Send(SessionID iSessionID,
	const void* iSource, size_t iCount, size_t* oCountSent)
	{
	ZMutexLocker locker(fMutex);

	if (oCountSent)
		*oCountSent = 0;

	Error result = errorInvalidSession;

	bool theIsNear;
	PrivID thePrivID;
	sMakePrivID(iSessionID, theIsNear, thePrivID);
	if (SessionCB* theSession = this->UseSession(theIsNear, thePrivID))
		{
		// We can only send if we're open in the send direction
		if (!theSession->fOpen_Send)
			{
			result = errorSendClosed;
			}
		else
			{
			if (iCount == 0)
				{
				result = errorNone;
				}
			else
				{
				// Put ourselves on the session's send queue
				Waiter_Send theWaiter;
				theWaiter.fSource = reinterpret_cast<const uint8*>(iSource);
				theWaiter.fCount = iCount;
				theWaiter.fResult = waitPending;
				theWaiter.fNext = nil;
				if (theSession->fWaiters_Send_Tail)
					{
					theSession->fWaiters_Send_Tail->fNext = &theWaiter;
					theSession->fWaiters_Send_Tail = &theWaiter;
					}
				else
					{
					theSession->fWaiters_Send_Head = &theWaiter;
					theSession->fWaiters_Send_Tail = &theWaiter;
					}

				fCondition_Send.Broadcast();
				while (theWaiter.fResult == waitPending)
					theSession->fCondition_Send.Wait(fMutex);

				if (oCountSent)
					*oCountSent += iCount - theWaiter.fCount;

				result = theWaiter.fResult;
				}
			}
		this->ReleaseSession(theSession);
		}
	return result;
	}


/**
Receive data from the remote side of the session.

\param iSessionID The session from  which we wish to receive data.
\param iDest A pointer to where received data should be placed.
\param iCount The maximum number of bytes to receive.
\param oCountReceived Optional output argument which will be updated with
the number of bytes actually received.

\return \li \c errorNone: The size_t referred to by \a oCountReceived holds
the number of bytes that were succesfully received. If less than iCount then it's
likely (but not certain) that the remote side has closed the session.
\return \li \c errorInvalidSession: The session \a iSessionID does not exist.
\return \li \c errorReceiveClosed: The remote side has called ZStreamMUX::Close,
and all data sent prior to that has already been received.
\return \li \c errorAbortLocal, \c errorAbortRemote: The session was aborted.
*/

ZStreamMUX::Error ZStreamMUX::Receive(SessionID iSessionID,
	void* iDest, size_t iCount, size_t* oCountReceived)
	{
	ZMutexLocker locker(fMutex);

	if (oCountReceived)
		*oCountReceived = 0;

	Error result = errorInvalidSession;
	bool theIsNear;
	PrivID thePrivID;
	sMakePrivID(iSessionID, theIsNear, thePrivID);
	if (SessionCB* theSession = this->UseSession(theIsNear, thePrivID))
		{
		// First try to drain the buffer.
		uint8* localDest = reinterpret_cast<uint8*>(iDest);
		size_t countRemaining = iCount;
		size_t countCopied;
		this->CopyDataOut(theSession, localDest, countRemaining, countCopied);
		if (countCopied)
			{
			localDest += countCopied;
			countRemaining -= countCopied;
			if (oCountReceived)
				*oCountReceived += countCopied;
			// Wake up our send thread so it has the opportunity to issue more credits.
			fCondition_Send.Broadcast(); 
			}

		if (!theSession->fOpen_Receive)
			{
			if (countCopied)
				result = errorNone;
			else
				result = errorReceiveClosed;
			}
		else
			{
			if (countRemaining == 0)
				{
				result = errorNone;
				}
			else
				{
				// Buffer *must* be empty if we're hooking on.
				ZAssertStop(kDebug_StreamMUX,
					theSession->fReceiveBufferFeedIn == theSession->fReceiveBufferFeedOut);

				Waiter_Receive theWaiter;
				theWaiter.fDest = localDest;
				theWaiter.fCount = countRemaining;
				theWaiter.fResult = waitPending;
				theWaiter.fNext = nil;
				if (theSession->fWaiters_Receive_Tail)
					{
					theSession->fWaiters_Receive_Tail->fNext = &theWaiter;
					theSession->fWaiters_Receive_Tail = &theWaiter;
					}
				else
					{
					theSession->fWaiters_Receive_Head = &theWaiter;
					theSession->fWaiters_Receive_Tail = &theWaiter;
					}

				while (theWaiter.fResult == waitPending)
					theSession->fCondition_Receive.Wait(fMutex);

				if (oCountReceived)
					*oCountReceived += countRemaining - theWaiter.fCount;

				result = theWaiter.fResult;
				}
			}
		this->ReleaseSession(theSession);
		}
	return result;
	}


/**
Get the number of bytes that can be read without blocking.

\param iSessionID The session from  which we wish to receive data.
\param oCountReceivable On success this will hold the number of bytes that
could be retrieved by a call to ZStreamMUX::Receive without blocking.

\return \li \c errorNone
\return \li \c errorInvalidSession: The session \a iSessionID does not exist.
\return \li \c errorReceiveClosed: The remote side has called ZStreamMUX::Close,
and all data sent prior to that has already been received.
*/

ZStreamMUX::Error ZStreamMUX::CountReceiveable(SessionID iSessionID, size_t& oCountReceivable)
	{
	ZMutexLocker locker(fMutex);
	oCountReceivable = 0;
	Error result = errorInvalidSession;
	bool theIsNear;
	PrivID thePrivID;
	sMakePrivID(iSessionID, theIsNear, thePrivID);
	if (SessionCB* theSession = this->UseSession(theIsNear, thePrivID))
		{
		oCountReceivable = theSession->fReceiveBufferFeedIn - theSession->fReceiveBufferFeedOut;
		if (oCountReceivable == 0 && !theSession->fOpen_Receive)
			result = errorReceiveClosed;
		else
			result = errorNone;
		this->ReleaseSession(theSession);
		}
	return result;
	}

void ZStreamMUX::SetMaximumFragmentSize(size_t iSize)
	{
	iSize = min(size_t(64), iSize);
	ZMutexLocker locker(fMutex);
	fMaxFragmentSize = iSize;
	}

void ZStreamMUX::RunSend()
	{
	ZMutexLocker locker(fMutex);
	try
		{
		while (!fSem_Disposed)
			{
			bool needsWait = true;
			if (this->HandleSend_Open(locker))
				needsWait = false;
			if (this->HandleSend_RST(locker))
				needsWait = false;
			if (this->HandleSend_SYNACK(locker))
				needsWait = false;
			if (this->HandleSend_Credit(locker))
				needsWait = false;
			if (this->HandleSend_Data(locker))
				needsWait = false;

			if (needsWait)
				{
				fStreamW.Flush();
				fCondition_Send.Wait(fMutex);
				}
			}
		fSem_Disposed->Signal(1);
		}
	catch (...)
		{
		}
	fExitedSend = true;
	}

void ZStreamMUX::sRunSend(ZStreamMUX* iStreamMUX)
	{ iStreamMUX->RunSend(); }

bool ZStreamMUX::HandleSend_Open(ZMutexLocker& iLocker)
	{
	bool releasedMutex = false;
	SessionCB* newSession = nil;
	try
		{
		while (Waiter_Open* theWaiter_Open = fWaiters_Open_Head)
			{
			// Unlink it
			ZAssertStop(kDebug_StreamMUX,
				fWaiters_Open_Tail != fWaiters_Open_Head || theWaiter_Open->fNext == nil);

			if (theWaiter_Open->fNext == nil)
				{
				fWaiters_Open_Head = nil;
				fWaiters_Open_Tail = nil;
				}
			else
				{
				fWaiters_Open_Head = theWaiter_Open->fNext;
				}

			// We need to allocate a session
			PrivID sessionID = 0;
			SessionCB* currentSession = fSessions_Head;
			while (currentSession)
				{
				if (currentSession->fIsNear)
					{
					if (currentSession->fID == sessionID)
						sessionID = currentSession->fID + 1;
					else
						break;
					}
				currentSession = currentSession->fNext;
				}

			if (sessionID > kMaxPrivID)
				{
				// We couldn't allocate a new session, so just bail.
				// Re link the waiter back on the list.
				theWaiter_Open->fNext = fWaiters_Open_Head;
				fWaiters_Open_Head = theWaiter_Open;
				if (fWaiters_Open_Tail == nil)
					fWaiters_Open_Tail = theWaiter_Open;
				break;
				}
			else
				{
				releasedMutex = true;
				// Allocate the session, with an inuse count
				newSession = new SessionCB(true, sessionID, fDefaultCredit - 32, fDefaultCredit);
				if (currentSession)
					{
					// We ended up pointing at a session in the list which is near
					// and which has an ID greater than our candidate ID. So we need
					// to insert the session before it.
					if (fSessions_Head == currentSession)
						{
						fSessions_Head = newSession;
						newSession->fNext = currentSession;
						currentSession->fPrev = newSession;
						}
					else
						{
						newSession->fPrev = currentSession->fPrev;
						newSession->fNext = currentSession;
						currentSession->fPrev->fNext = newSession;
						currentSession->fPrev = newSession;
						}
					}
				else
					{
					// We ended up at the end of the list, so we need to append to it.
					newSession->fPrev = fSessions_Tail;
					if (fSessions_Tail)
						fSessions_Tail->fNext = newSession;
					else
						fSessions_Head = newSession;
					fSessions_Tail = newSession;
					}

				// Find what listener number to use, allocating or reusing one if necessary.
				ListenName* prevPrevListenName = nil;
				ListenName* prevListenName = nil;
				ListenName* currListenName = fListenNames_Send;
				while (currListenName && currListenName->fName != theWaiter_Open->fListenName)
					{
					prevPrevListenName = prevListenName;
					prevListenName = currListenName;
					currListenName = currListenName->fNext;
					}

				theWaiter_Open->fSession = newSession;

				// Put the waiter on our list waiting for SYNACK or RST.
				theWaiter_Open->fNext = fWaiters_OpenSYNSent;
				fWaiters_OpenSYNSent = theWaiter_Open;

				uint8 theListenerID;
				if (currListenName)
					{
					// Move it to the front of the list
					if (prevListenName)
						prevListenName->fNext = currListenName->fNext;
					currListenName->fNext = fListenNames_Send;
					fListenNames_Send = currListenName;
					theListenerID = currListenName->fID;
					ZAssert(releasedMutex);
					iLocker.Release();
					}
				else
					{
					// We didn't find it. prevListenName points to the last one on
					// the list, which we'll just have to reuse.
					if (prevPrevListenName)
						prevPrevListenName->fNext = currListenName;
					prevListenName->fNext = fListenNames_Send;
					fListenNames_Send = prevListenName;
					fListenNames_Send->fName = theWaiter_Open->fListenName;
					theListenerID = fListenNames_Send->fID;

					ZAssert(releasedMutex);
					iLocker.Release();
					this->WriteFragment_DefineListener(theListenerID,
						theWaiter_Open->fListenName);
					}

				if (theWaiter_Open->fCount && theWaiter_Open->fSource
					&& sessionID < 16 && theListenerID < 32)
					{
					// We can send a Payload SYN fragment
					// 01TT TTPP   PPPK KKKK   .....
					size_t countToSend = min(size_t(31), theWaiter_Open->fCount - 1);
					fStreamW.WriteUInt8(0x40 | (sessionID << 2) | (theListenerID >> 3));
					fStreamW.WriteUInt8((theListenerID << 5) | (countToSend - 1));
					fStreamW.Write(theWaiter_Open->fSource, countToSend);
					theWaiter_Open->fSource += countToSend;
					theWaiter_Open->fCount -= countToSend;
					}
				else
					{
					if (sessionID < 32 && theListenerID < 64)
						{
						// We can send a short SYN
						// 0001 1TTT   TTPP PPPP
						fStreamW.WriteUInt8(0x10 | 0x08 | (sessionID >> 2));
						fStreamW.WriteUInt8((sessionID << 6) | theListenerID);
						}
					else
						{
						// Have to use the long SYN
						// 0000 111T   TTTT TTTT   PPPP PPPP
						fStreamW.WriteUInt8(0x08 | 0x04 | 0x02 | (sessionID >> 8));
						fStreamW.WriteUInt8(sessionID);
						fStreamW.WriteUInt8(theListenerID);
						}
					}
				iLocker.Acquire();
				}
			}
		}
	catch (...)
		{
		delete newSession;
		throw;
		}
	return releasedMutex;
	}

bool ZStreamMUX::HandleSend_RST(ZMutexLocker& iLocker)
	{
	bool releasedMutex = false;
	while (Waiter_RST* theWaiter = fWaiters_RST_Head)
		{
		releasedMutex = true;

		// Pull it off the head.
		if (theWaiter->fNext)
			{
			fWaiters_RST_Head = theWaiter->fNext;
			theWaiter->fNext = nil;
			}
		else
			{
			fWaiters_RST_Head = nil;
			fWaiters_RST_Tail = nil;
			}

		iLocker.Release();
		this->WriteFragment_RST(theWaiter->fSession->fIsNear, theWaiter->fSession->fID);
		iLocker.Acquire();

		theWaiter->fDoneIt = true;
		theWaiter->fSession->fCondition_Send.Broadcast();
		fCondition_Send.Broadcast();
		}

	// Also send RSTs for any listens that were stopped with pending sessions
	while (ActiveListen* theListen = fActiveListens_RST)
		{
		releasedMutex = true;

		fActiveListens_RST = theListen->fNext;
		theListen->fNext = nil;

		// We should only have an ActiveListen on fActiveListens_RST if it has pending sessions.
		ZAssertStop(kDebug_StreamMUX, theListen->fPendingSessions.size());
		for (deque<SessionCB*>::iterator i = theListen->fPendingSessions.begin();
			i != theListen->fPendingSessions.end(); ++i)
			{
			ZAssertStop(kDebug_StreamMUX, !(*i)->fIsNear);
			iLocker.Release();
			this->WriteFragment_RST(false, (*i)->fID);
			iLocker.Acquire();
			// Release the session (undoing the pinning initiated by ZStreamMUX::StopListen);
			// Mark it as used (will be released by HandleSend_RST)
			this->ReleaseSession(*i);
			}
		delete theListen;
		}

	return releasedMutex;
	}

bool ZStreamMUX::HandleSend_SYNACK(ZMutexLocker& iLocker)
	{
	bool releasedMutex = false;
	SessionCB* newSession = nil;
	try
		{
		while (fPendingSYN_Head)
			{
			releasedMutex = true;

			// Pull it off the head.
			SYNReceived* theSYN = fPendingSYN_Head;
			fPendingSYN_Head = theSYN->fNext;
			if (!fPendingSYN_Head)
				fPendingSYN_Tail = nil;

			ActiveListen* currListen = nil;
			if (theSYN->fListenName.size())
				{
				currListen = fActiveListens;
				while (currListen && currListen->fListenName != theSYN->fListenName)
					currListen = currListen->fNext;
				}

			if (currListen == nil)
				{
				// Bad listener name.
				ZAssert(releasedMutex);
				iLocker.Release();
				this->WriteFragment_RST(false, theSYN->fID);
				delete[] theSYN->fBuffer;
				delete theSYN;
				iLocker.Acquire();
				}
			else
				{
				// We need a new session regardless of whether there's a pending accept.
				newSession = new SessionCB(false, theSYN->fID, fDefaultCredit, fDefaultCredit);

				if (theSYN->fBufferSize)
					{
					size_t countCopied;
					this->CopyDataIn(iLocker, newSession,
						theSYN->fBuffer, theSYN->fBufferSize, countCopied);

					ZAssertStop(kDebug_StreamMUX, countCopied == theSYN->fBufferSize);
					delete[] theSYN->fBuffer;
					theSYN->fBuffer = nil;
					}

				newSession->fPrev = fSessions_Tail;
				if (fSessions_Tail)
					fSessions_Tail->fNext = newSession;
				else
					fSessions_Head = newSession;
				fSessions_Tail = newSession;

				// See if we've got a waiting accept.
				if (Waiter_Accept* theWaiter = currListen->fWaiters_Head)
					{
					currListen->fWaiters_Head = theWaiter->fNext;
					if (currListen->fWaiters_Tail == theWaiter)
						currListen->fWaiters_Tail = nil;
					theWaiter->fNext = nil;
					theWaiter->fSession = newSession;
					theWaiter->fResult = errorNone;
					fCondition_Accept.Broadcast();
					}
				else
					{
					// Need to make it a pending session
					currListen->fPendingSessions.push_back(newSession);
					}
				ZAssert(releasedMutex);
				iLocker.Release();
				this->WriteFragment_SYNACK(theSYN->fID);
				delete theSYN;
				iLocker.Acquire();
				}
			}
		}
	catch (...)
		{
		delete newSession;
		throw;
		}
	return releasedMutex;
	}

bool ZStreamMUX::HandleSend_Credit(ZMutexLocker& iLocker)
	{
	// Walk through all sessions and generate credit messages for any whose receive credit
	// is less than half their buffer space.
	bool releasedMutex = false;
	while (true)
		{
		bool foundOne = false;
		bool isNear;
		PrivID thePrivID;
		size_t additionalCredit;
		SessionCB* theSession = fSessions_Head;
		while (theSession)
			{
			size_t spaceInBuffer = theSession->fReceiveBufferSize
				- (theSession->fReceiveBufferFeedIn - theSession->fReceiveBufferFeedOut);

			if (spaceInBuffer > 2 * theSession->fCredit_Receive)
				{
				additionalCredit = (spaceInBuffer - theSession->fCredit_Receive) & 0xFFFFFF00;
				if (additionalCredit)
					{
					additionalCredit = min(size_t(0x3FF00), additionalCredit);
					foundOne = true;
					isNear = theSession->fIsNear;
					thePrivID = theSession->fID;
					theSession->fCredit_Receive += additionalCredit;
					break;
					}
				break;
				}
			theSession = theSession->fNext;
			}
		if (!foundOne)
			break;

		releasedMutex = true;
		iLocker.Release();
		this->WriteFragment_Credit(isNear, thePrivID, additionalCredit >> 8);
		iLocker.Acquire();
		}
	return releasedMutex;
	}

bool ZStreamMUX::HandleSend_Data(ZMutexLocker& iLocker)
	{
	// We round robin through our sessions.
	bool releasedMutex = false;
	size_t fragmentsSent = 0;
	if (fSessionNextSend == nil)
		fSessionNextSend = fSessions_Head;

	while (fSessionNextSend && fragmentsSent < 4)
		{
		SessionCB* theSession = this->UseSession(fSessionNextSend);
		fSessionNextSend = fSessionNextSend->fNext;

		if (!theSession->fWaiters_Send_Head)
			{
			if (!theSession->fOpen_Send && !theSession->fSentFIN)
				{
				releasedMutex = true;
				theSession->fSentFIN = true;
				iLocker.Release();
				this->WriteFragment_FIN(theSession->fIsNear, theSession->fID);
				iLocker.Acquire();
				theSession->fCondition_Send.Broadcast();
				}
			}
		else
			{
			while (theSession->fWaiters_Send_Head && fragmentsSent < 4)
				{
				Waiter_Send* currWaiter = theSession->fWaiters_Send_Head;
				if (size_t countSendable = min(currWaiter->fCount, theSession->fCredit_Send))
					{
					countSendable = min(countSendable, fMaxFragmentSize);
					const uint8* source = currWaiter->fSource;
					theSession->fCredit_Send -= countSendable;
					currWaiter->fSource += countSendable;
					currWaiter->fCount -= countSendable;
					bool sendFIN = false;
					if (currWaiter->fCount == 0)
						{
						// Unlink it.
						if (currWaiter->fNext)
							{
							theSession->fWaiters_Send_Head = currWaiter->fNext;
							currWaiter->fNext = nil;
							}
						else
							{
							theSession->fWaiters_Send_Head = nil;
							theSession->fWaiters_Send_Tail = nil;
							}

						if (!theSession->fOpen_Send)
							{
							ZAssertStop(kDebug_StreamMUX, !theSession->fSentFIN);
							sendFIN = true;
							}
						}

					++fragmentsSent;
					releasedMutex = true;

					iLocker.Release();
					this->WriteFragment_Payload(theSession->fIsNear, theSession->fID,
						source, countSendable);

					theSession->fTotalSent += countSendable;
					if (sendFIN)
						this->WriteFragment_FIN(theSession->fIsNear, theSession->fID);
					iLocker.Acquire();

					if (sendFIN)
						{
						ZAssertStop(kDebug_StreamMUX, !theSession->fSentFIN);
						theSession->fSentFIN = true;
						theSession->fCondition_Send.Broadcast();
						}

					if (currWaiter->fCount == 0)
						{
						currWaiter->fResult = errorNone;
						theSession->fCondition_Send.Broadcast();
						}
					}
				else
					{
					break;
					}
				}
			}
		this->ReleaseSession(theSession);
		}
	return releasedMutex;
	}

void ZStreamMUX::RunReceive()
	{
	try
		{
		while (!fSem_Disposed)
			{
			uint8 secondByte;
			uint8 thirdByte;
			uint8 listenerID;
			PrivID thePrivID;
			bool isNear;
			// Decode the fragment header and dispatch to the appropriate handler
			uint8 firstByte = fStreamR.ReadUInt8();
			if (fSem_Disposed)
				break;
			if (firstByte & 0x80)
				{
				// 1___ ____
				// Payload.
				size_t lengthMinusOne;
				if (firstByte & 0x40)
					{
					// 11WS SLLL   .....
					// Short session ID (ie < 8)
					isNear = (firstByte & 0x20);
					thePrivID = (firstByte & 0x18) >> 3;
					lengthMinusOne = (firstByte & 0x07);
					// Overhead is one byte for fragments from 1 to 7 bytes (we
					// don't send zero sized fragments).
					if (lengthMinusOne == 7)
						{
						// 11WS S111   KKKK KKKK   .....
						// lengthMinusOne - 7 is in the subsequent byte, giving us an overhead
						// of two bytes for fragments from 8 to 262 bytes in length.
						lengthMinusOne = 7 + fStreamR.ReadUInt8();
						if (lengthMinusOne == 262)
							{
							// 11WS S111   1111 1111   KKKK KKKK   KKKK KKKK .....
							// lengthMinusOne is in the subsequent two bytes, giving us an
							// overhead of four bytes for fragments from 263 to 65536 in length.
							lengthMinusOne = fStreamR.ReadUInt16();
							}
						}
					}
				else if (firstByte & 0x20)
					{
					// 101W SSSS   KKKK KKKK   .....
					// Longer session ID, between 4 and 19.
					isNear = (firstByte & 0x10);
					thePrivID = 4 + (firstByte & 0x0F);
					lengthMinusOne = fStreamR.ReadUInt8();
					if (lengthMinusOne == 255)
						{
						// 101S SSSS   1111 1111   KKKK KKKK   KKKK KKKK .....
						lengthMinusOne = fStreamR.ReadUInt16();
						}
					}
				else
					{
					// 100W SSSS   SSSS SLLL   .....
					// Big PrivID, from 20 to 1024. Use 5 bits from firstByte
					// plus 5 bits from next byte.
					secondByte = fStreamR.ReadUInt8();
					isNear = (firstByte & 0x10);
					thePrivID = (((firstByte & 0x0F) << 5) | ((secondByte & 0xF8) >> 3));
					lengthMinusOne = (secondByte & 0x07);
					if (lengthMinusOne == 7)
						{
						// 100S SSSS   SSSS S111   KKKK KKKK   .....
						// lengthMinusOne - 7 is in the subsequent byte, giving us an overhead
						// of three bytes for fragments from 8 to 262 bytes in length.
						lengthMinusOne = 7 + fStreamR.ReadUInt8();
						if (lengthMinusOne == 262)
							{
							// 100S SSSS   SSSS S111   1111 1111   KKKK KKKK   KKKK KKKK   .....
							// length is in the subsequent two bytes, giving us an overhead of
							// five bytes for fragments from 263 to 65536 in length.
							lengthMinusOne = fStreamR.ReadUInt16();
							}
						}
					}

				ZMutexLocker locker(fMutex);
				if (SessionCB* theSession = this->UseSession(isNear, thePrivID))
					{
					this->HandleReceive_Payload(locker, theSession, lengthMinusOne + 1);
					theSession->fTotalReceived += lengthMinusOne + 1;
					this->ReleaseSession(theSession);
					}
				else
					{
					locker.Release();
					// Nead to suck up and ignore the payload
					fStreamR.Skip(lengthMinusOne + 1);
					}
				}
			else if (firstByte & 0x40)
				{
				// 01__ ____

				// 01TT TTPP   PPPK KKKK .....
				// Payload SYN. 
				secondByte = fStreamR.ReadUInt8();
				thePrivID = (firstByte & 0x3C) >> 2;

				listenerID = ((firstByte & 0x03) << 3) | ((secondByte & 0xE0) >> 5);

				size_t length = secondByte & 0x1F;
				this->HandleReceive_SYN(thePrivID, listenerID, length);
				}
			else if (firstByte & 0x20)
				{
				// 001_ ____
				// Add credit.
				secondByte = fStreamR.ReadUInt8();
				size_t credits;
				if (firstByte & 0x10)
					{
					// 0011 WTTT   TTCC CCCC
					// Short form.
					isNear = firstByte & 0x08;
					thePrivID = ((firstByte & 0x07) << 2) | ((secondByte & 0xC0) >> 6);

					// credit length is 6 bits * 256.
					credits = (secondByte & 0x3F) << 8;
					}
				else
					{
					// 0010 WTTT   TTTT TTCC   CCCC CCCC
					// Long form.
					isNear = firstByte & 0x08;
					thePrivID = ((firstByte & 0x07) << 6) | ((secondByte & 0xF8) >> 2);

					thirdByte = fStreamR.ReadUInt8();
					// credit length is 10 bits * 256.
					credits = ((secondByte & 0x03) << 16) | (thirdByte << 8);
					}
				this->HandleReceive_Credit(isNear, thePrivID, credits);
				}
			else if (firstByte & 0x10)
				{
				// 0001 ____
				if (firstByte & 0x08)
					{
					// 0001 1TTT   TTPP PPPP
					// Short SYN.
					secondByte = fStreamR.ReadUInt8();
					thePrivID = ((firstByte & 0x07) << 2) | ((secondByte & 0xC0) >> 6);

					listenerID = secondByte & 0x3F;
					this->HandleReceive_SYN(thePrivID, listenerID, 0);
					}
				else if (firstByte & 0x04)
					{
					// 0001 01WS   SSSS SSSS
					// FIN
					secondByte = fStreamR.ReadUInt8();
					isNear = firstByte & 0x2;
					thePrivID = ((firstByte & 0x1) << 8) | secondByte;
					this->HandleReceive_FIN(isNear, thePrivID);
					}
				else
					{
					// 0001 00WS   SSSS SSSS
					// RST
					secondByte = fStreamR.ReadUInt8();
					isNear = firstByte & 0x2;
					thePrivID = ((firstByte & 0x1) << 8) | secondByte;
					this->HandleReceive_RST(isNear, thePrivID);
					}
				}
			else if (firstByte & 0x08)
				{
				// 0000 1___
				if (firstByte & 0x04)
					{
					// 0000 11__
					if (firstByte & 0x02)
						{
						// 0000 111T   TTTT TTTT   PPPP PPPP
						// SYN (unrestricted)
						secondByte = fStreamR.ReadUInt8();
						thirdByte = fStreamR.ReadUInt8();
						thePrivID = ((firstByte & 0x1) << 8) | (secondByte);

						listenerID = thirdByte;
						this->HandleReceive_SYN(thePrivID, listenerID, 0);
						}
					else
						{
						// 0000 110T   TTTT TTTT
						// SYNACK
						secondByte = fStreamR.ReadUInt8();
						thePrivID = ((firstByte & 0x1) << 8) | (secondByte);
						this->HandleReceive_SYNACK(thePrivID);
						}
					}
				else  if (firstByte & 0x02)
					{
					// 0000 101_
					listenerID = fStreamR.ReadUInt8();
					size_t length;
					if (firstByte & 0x01)
						{
						// 0000 1011   PPPP PPPP   LLLL LLLL   .....
						// Define listener (short)
						length = fStreamR.ReadUInt8();
						}
					else
						{
						// 0000 1010   PPPP PPPP   LLLL LLLL   LLLL LLLL   .....
						// Define listener (long)
						length = fStreamR.ReadUInt16();
						}
					this->HandleReceive_DefineListener(listenerID, length);
					}
				else
					{
					// 0000 100_
					// Unused (illegal?)
					}
				}
			else
				{
				// 0000 0___
				// Unused
				}
			}
		fSem_Disposed->Signal(1);
		}
	catch (...)
		{
		}
	fExitedReceive = true;
	}

void ZStreamMUX::sRunReceive(ZStreamMUX* iStreamMUX)
	{ iStreamMUX->RunReceive(); }

void ZStreamMUX::HandleReceive_Payload(ZMutexLocker& iLocker,
	SessionCB* iSession, size_t iPayloadLength)
	{
	if (!iSession->fOpen_Receive)
		{
		fStreamR.Skip(iPayloadLength);
		return;
		}

	// Check and update the sessions receive credit.
	ZAssertStop(kDebug_StreamMUX, iSession->fCredit_Receive >= iPayloadLength);

	size_t priorSpaceInBuffer = iSession->fReceiveBufferSize
		- (iSession->fReceiveBufferFeedIn - iSession->fReceiveBufferFeedOut);

	bool priorSendCredit = (priorSpaceInBuffer > 2 * iSession->fCredit_Receive);

	iSession->fCredit_Receive -= iPayloadLength;

	while (iPayloadLength)
		{
		// Try to disperse the data to waiters.
		if (Waiter_Receive* theWaiter = iSession->fWaiters_Receive_Head)
			{
			// If we've got a waiter then our buffer *must* be empty
			ZAssertStop(kDebug_StreamMUX,
				iSession->fReceiveBufferFeedOut == iSession->fReceiveBufferFeedIn);

			if (iPayloadLength >= theWaiter->fCount)
				{
				// We're gonna fill the waiter, so pull it off our list
				if (theWaiter->fNext)
					{
					iSession->fWaiters_Receive_Head = theWaiter->fNext;
					}
				else
					{
					iSession->fWaiters_Receive_Head = nil;
					iSession->fWaiters_Receive_Tail = nil;
					}
				theWaiter->fNext = nil;

				iLocker.Release();
				fStreamR.Read(theWaiter->fDest, theWaiter->fCount);
				iLocker.Acquire();

				iPayloadLength -= theWaiter->fCount;
				theWaiter->fDest += theWaiter->fCount;
				theWaiter->fCount = 0;
				theWaiter->fResult = errorNone;

				iSession->fCondition_Receive.Broadcast();
				}
			else
				{
				// We won't fill the waiter, so we only need to manipulate
				// its state -- it stays on the list.
				iLocker.Release();
				fStreamR.Read(theWaiter->fDest, iPayloadLength);
				iLocker.Acquire();

				theWaiter->fDest += iPayloadLength;
				theWaiter->fCount -= iPayloadLength;
				iPayloadLength = 0;
				}
			}
		else
			{

			// We weren't able to parcel out all the received data
			// to waiters, so we'll have to buffer it.
			size_t feedOutOffset = iSession->fReceiveBufferFeedOut % iSession->fReceiveBufferSize;
			size_t feedInOffset = iSession->fReceiveBufferFeedIn % iSession->fReceiveBufferSize;
			if (feedInOffset < feedOutOffset)
				{
				// The feed in is to the left of the feed out -- we can read
				// the payload in a single chunk.
				ZAssertStop(kDebug_StreamMUX, feedOutOffset - feedInOffset >= iPayloadLength);
				iLocker.Release();
				fStreamR.Read(iSession->fReceiveBuffer + feedInOffset, iPayloadLength);
				iLocker.Acquire();
				}
			else
				{
				// May need to read in two chunks because the free space wraps around
				size_t countToReceive
					= min(iPayloadLength, iSession->fReceiveBufferSize - feedInOffset);

				iLocker.Release();
				fStreamR.Read(iSession->fReceiveBuffer + feedInOffset, countToReceive);
				if (size_t extra = iPayloadLength - countToReceive)
					fStreamR.Read(iSession->fReceiveBuffer, extra);
				iLocker.Acquire();
				}
			iSession->fReceiveBufferFeedIn += iPayloadLength;
			iPayloadLength = 0;
			// If any waiters queued up whilst we were reading, distribute some data to them.
			while (Waiter_Receive* theWaiter = iSession->fWaiters_Receive_Head)
				{
				size_t countCopied;
				this->CopyDataOut(iSession, theWaiter->fDest, theWaiter->fCount, countCopied);
				theWaiter->fDest += countCopied;
				theWaiter->fCount -= countCopied;
				if (theWaiter->fCount != 0)
					{
					// Bail out if we couldn't satisfy the first on the list.
					break;
					}
				else
					{
					if (theWaiter->fNext)
						{
						iSession->fWaiters_Receive_Head = theWaiter->fNext;
						}
					else
						{
						iSession->fWaiters_Receive_Head = nil;
						iSession->fWaiters_Receive_Tail = nil;
						}
					theWaiter->fNext = nil;
					theWaiter->fResult = errorNone;
					iSession->fCondition_Receive.Broadcast();
					}
				}
			}
		}
	size_t subsequentSpaceInBuffer = iSession->fReceiveBufferSize
		- (iSession->fReceiveBufferFeedIn - iSession->fReceiveBufferFeedOut);

	bool subsequentSendCredit = (priorSpaceInBuffer > 2 * iSession->fCredit_Receive);
	if (!priorSendCredit && subsequentSendCredit)
		fCondition_Send.Broadcast();
	}

void ZStreamMUX::HandleReceive_Credit(bool iIsNear, PrivID iPrivID, size_t iCredit)
	{
	ZMutexLocker locker(fMutex);
	if (SessionCB* theSession = this->UseSession(iIsNear, iPrivID))
		{
		theSession->fCredit_Send += iCredit;
		fCondition_Send.Broadcast();
		this->ReleaseSession(theSession);
		}
	}

void ZStreamMUX::HandleReceive_SYN(PrivID iPrivID, uint8 iListenerID, size_t iPayloadLength)
	{
	// Try to find a matching listener name
	ZMutexLocker locker(fMutex);
	SYNReceived* theSYN = nil;
	try
		{
		ListenName* prevListener = nil;
		ListenName* currListener = fListenNames_Receive;
		while (currListener && currListener->fID != iListenerID)
			{
			prevListener = currListener;
			currListener = currListener->fNext;
			}

		theSYN = new SYNReceived;
		theSYN->fID = iPrivID;
		theSYN->fBuffer = nil;
		theSYN->fBufferSize = 0;
		if (currListener)
			{
			// Move the listener to the front of the list.
			if (prevListener)
				{
				prevListener->fNext = currListener->fNext;
				currListener->fNext = fListenNames_Receive;
				fListenNames_Receive = currListener;
				}
			// Remember the name (so we don't have to keep the listener list locked down).
			theSYN->fListenName = currListener->fName;

			if (iPayloadLength)
				{
				locker.Release();
				theSYN->fBuffer = new uint8[iPayloadLength];
				theSYN->fBufferSize = iPayloadLength;
				// Suck off the payload
				fStreamR.Read(theSYN->fBuffer, iPayloadLength);
				locker.Acquire();
				}
			}
		else
			{
			// The listener was not valid, which we indicate by leaving the SYNReceived's
			// listener name empty. HandleSend_SYNACK will send a RST when it sees this SYN.
			// Ignore any payload
			if (iPayloadLength)
				{
				locker.Release();
				fStreamR.Skip(iPayloadLength);
				locker.Acquire();
				}
			}

		// Enqueue the SYN
		theSYN->fNext = nil;
		if (fPendingSYN_Tail)
			fPendingSYN_Tail->fNext = theSYN;
		else
			fPendingSYN_Head = theSYN; 
		fPendingSYN_Tail = theSYN;
		fCondition_Send.Broadcast();
		}
	catch (...)
		{
		if (theSYN)
			delete[] theSYN->fBuffer;
		delete theSYN;
		throw;
		}
	}

void ZStreamMUX::HandleReceive_SYNACK(PrivID iPrivID)
	{
	ZMutexLocker locker(fMutex);
	// Find a Waiter_Open
	Waiter_Open* prevWaiter = nil;
	Waiter_Open* currWaiter = fWaiters_OpenSYNSent;
	while (currWaiter && currWaiter->fSession && currWaiter->fSession->fID != iPrivID)
		{
		ZAssertStop(kDebug_StreamMUX, currWaiter->fSession->fIsNear);
		prevWaiter = currWaiter;
		currWaiter = currWaiter->fNext;
		}

	if (currWaiter)
		{
		// Unlink it
		if (prevWaiter)
			prevWaiter->fNext = currWaiter->fNext;
		else
			fWaiters_OpenSYNSent = currWaiter->fNext;
		currWaiter->fNext = nil;
		// Wake up the thread that's waiting.
		currWaiter->fResult = errorNone;
		fCondition_Open.Broadcast();
		}
	}

void ZStreamMUX::HandleReceive_FIN(bool iIsNear, PrivID iPrivID)
	{
	// Switch states
	ZMutexLocker locker(fMutex);
	if (SessionCB* theSession = this->UseSession(iIsNear, iPrivID))
		{
		// Notify all Waiter_Receives
		Waiter_Receive* currWaiter = theSession->fWaiters_Receive_Head;
		while (currWaiter)
			{
			currWaiter->fResult = errorNone;
			Waiter_Receive* nextWaiter = currWaiter->fNext;
			currWaiter->fNext = nil;
			currWaiter = nextWaiter;
			}

		theSession->fOpen_Receive = false;
		theSession->fWaiters_Receive_Head = nil;
		theSession->fWaiters_Receive_Tail = nil;
		theSession->fCondition_Receive.Broadcast();

		this->ReleaseSession(theSession);
		}
	else
		{
		// This shouldn't be able to happen.
		ZUnimplemented();
		}
	}

void ZStreamMUX::HandleReceive_RST(bool iIsNear, PrivID iPrivID)
	{
	ZMutexLocker locker(fMutex);
	if (iIsNear)
		{
		// Handle Waiter_Opens
		Waiter_Open* prevWaiter = nil;
		Waiter_Open* currWaiter = fWaiters_OpenSYNSent;
		while (currWaiter && currWaiter->fSession && currWaiter->fSession->fID != iPrivID)
			{
			ZAssertStop(kDebug_StreamMUX, currWaiter->fSession->fIsNear);
			prevWaiter = currWaiter;
			currWaiter = currWaiter->fNext;
			}
		if (currWaiter)
			{
			// Unlink it
			if (prevWaiter)
				prevWaiter->fNext = currWaiter->fNext;
			else
				fWaiters_OpenSYNSent = currWaiter->fNext;
			currWaiter->fNext = nil;
			// Wake up the thread that's waiting.
			currWaiter->fResult = errorAbortRemote;
			this->ReleaseSession(currWaiter->fSession);
			currWaiter->fSession = nil;
			fCondition_Send.Broadcast();
			}
		}

	if (SessionCB* theSession = this->UseSession(iIsNear, iPrivID))
		{
		this->AbortSession(theSession);
		this->ReleaseSession(theSession);
		}
	}

void ZStreamMUX::HandleReceive_DefineListener(uint8 iListenerID, size_t iListenNameLength)
	{
	// Read the listener name.
	string newName;
	if (iListenNameLength)
		{
		newName.resize(iListenNameLength);
		fStreamR.Read(&newName[0], iListenNameLength);
		}

	ZMutexLocker locker(fMutex);

	ListenName* prevListener = nil;
	ListenName* currListener = fListenNames_Receive;
	while (currListener && currListener->fID == iListenerID)
		{
		prevListener = currListener;
		currListener = currListener->fNext;
		}

	if (currListener)
		{
		// Move the listener to the front of the list
		if (prevListener)
			prevListener->fNext = currListener->fNext;
		currListener->fNext = fListenNames_Receive;
		fListenNames_Receive = currListener;
		}
	else
		{
		// We didn't find the listener already created.
		currListener = new ListenName;
		currListener->fID = iListenerID;
		currListener->fNext = fListenNames_Receive;
		fListenNames_Receive = currListener;
		}

	currListener->fName = newName;
	}

void ZStreamMUX::WriteFragment_SYNACK(PrivID iPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, iPrivID <= kMaxPrivID);

	// 0000 110T   TTTT TTTT
	fStreamW.WriteUInt8(0x08 | 0x04 | (iPrivID >> 8));
	fStreamW.WriteUInt8(iPrivID);
	}

void ZStreamMUX::WriteFragment_FIN(bool iIsNear, PrivID iPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, iPrivID <= kMaxPrivID);

	// 0001 01WS   SSSS SSSS
	if (iIsNear)
		fStreamW.WriteUInt8(0x10 | 0x04 | (iPrivID >> 8));
	else
		fStreamW.WriteUInt8(0x10 | 0x04 | 0x02 | (iPrivID >> 8));
	fStreamW.WriteUInt8(iPrivID);
	}

void ZStreamMUX::WriteFragment_RST(bool iIsNear, PrivID iPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, iPrivID <= kMaxPrivID);

	// 0001 00WS   SSSS SSSS
	if (iIsNear)
		fStreamW.WriteUInt8(0x10 | (iPrivID >> 8));
	else
		fStreamW.WriteUInt8(0x10 | 0x02 | (iPrivID >> 8));
	fStreamW.WriteUInt8(iPrivID);
	}

void ZStreamMUX::WriteFragment_Credit(bool iIsNear, PrivID iPrivID, size_t iSentCredit)
	{
	ZAssertStop(kDebug_StreamMUX, iPrivID <= kMaxPrivID);
	ZAssertStop(kDebug_StreamMUX, iSentCredit < 0x400);
	// We send credits in chunks of 256 bytes, capped at 2^18 (256K).
	if (iSentCredit)
		{
		if (iPrivID < 32 && iSentCredit < 64)
			{
			// 0011 WTTT   TTCC CCCC
			if (iIsNear)
				fStreamW.WriteUInt8(0x20 | 0x10 | (iPrivID >> 2));
			else
				fStreamW.WriteUInt8(0x20 | 0x10 | 0x08 | (iPrivID >> 2));

			fStreamW.WriteUInt8((iPrivID << 6) | iSentCredit);
			}
		else
			{
			// 0010 WTTT   TTTT TTCC   CCCC CCCC
			if (iIsNear)
				fStreamW.WriteUInt8(0x20 | (iPrivID >> 6));
			else
				fStreamW.WriteUInt8(0x20 | 0x08 | (iPrivID >> 6));

			fStreamW.WriteUInt8((iPrivID << 2) | (iSentCredit >> 8));
			fStreamW.WriteUInt8(iSentCredit);
			}
		}
	}

void ZStreamMUX::WriteFragment_Payload(bool iIsNear, PrivID iPrivID,
	const void* iSource, size_t iLength)
	{
	ZAssertStop(kDebug_StreamMUX, iLength > 0);
	size_t lengthMinusOne = iLength - 1;
	if (iPrivID < 4)
		{
		if (lengthMinusOne < 7)
			{
			// 11WS SLLL   .....
			if (iIsNear)
				fStreamW.WriteUInt8(0x80 | 0x40 | (iPrivID << 3) | lengthMinusOne);
			else
				fStreamW.WriteUInt8(0x80 | 0x40 | 0x20 | (iPrivID << 3) | lengthMinusOne);
			}
		else if (lengthMinusOne < 263)
			{
			// 11WS S111   KKKK KKKK   .....
			if (iIsNear)
				fStreamW.WriteUInt8(0x80 | 0x40 | (iPrivID << 3) | 7);
			else
				fStreamW.WriteUInt8(0x80 | 0x40 | 0x20 | (iPrivID << 3) | 7);

			fStreamW.WriteUInt8(lengthMinusOne - 7);
			}
		else
			{
			// 11WS S111   1111 1111   KKKK KKKK   KKKK KKKK .....
			if (iIsNear)
				fStreamW.WriteUInt8(0x80 | 0x40 | (iPrivID << 3) | 7);
			else
				fStreamW.WriteUInt8(0x80 | 0x40 | 0x20 | (iPrivID << 3) | 7);

			fStreamW.WriteUInt8(0xFF);
			fStreamW.WriteUInt16(lengthMinusOne);
			}
		}
	else
		{
		if (iPrivID < 20)
			{
			if (iIsNear)
				fStreamW.WriteUInt8(0x80 | 0x20 | iPrivID - 4);
			else
				fStreamW.WriteUInt8(0x80 | 0x20 | 0x10 | iPrivID - 4);
			if (lengthMinusOne < 255)
				{
				// 101W SSSS   KKKK KKKK   .....
				fStreamW.WriteUInt8(lengthMinusOne);
				}
			else
				{
				// 101W SSSS   1111 1111   KKKK KKKK   KKKK KKKK .....
				fStreamW.WriteUInt8(255);
				fStreamW.WriteUInt16(lengthMinusOne);
				}
			}
		else
			{
			if (iIsNear)
				fStreamW.WriteUInt8(0x80 | (iPrivID >> 5));
			else
				fStreamW.WriteUInt8(0x80 | 0x10 | (iPrivID >> 5));
			if (lengthMinusOne < 7)
				{
				// 100W SSSS   SSSS SLLL   .....
				fStreamW.WriteUInt8((iPrivID << 3) | lengthMinusOne);
				}
			else
				{
				fStreamW.WriteUInt8((iPrivID << 3) | 7);
				if (lengthMinusOne < 262)
					{
					// 100W SSSS   SSSS S111   KKKK KKKK   .....
					fStreamW.WriteUInt8(lengthMinusOne - 7);
					}
				else
					{
					// 100W SSSS   SSSS S111   1111 1111   KKKK KKKK   KKKK KKKK   .....
					fStreamW.WriteUInt8(255);
					fStreamW.WriteUInt16(lengthMinusOne);
					}
				}
			}
		}
	fStreamW.Write(iSource, iLength);
	}

void ZStreamMUX::WriteFragment_DefineListener(uint8 iListenerID, const string& iListenName)
	{
	if (iListenName.size() < 256)
		{
		// 0000 1011   PPPP PPPP   LLLL LLLL   .....
		fStreamW.WriteUInt8(0x08 | 0x02 | 0x01);
		fStreamW.WriteUInt8(iListenerID);
		fStreamW.WriteUInt8(iListenName.size());
		}
	else
		{
		// 0000 1010   PPPP PPPP   LLLL LLLL   LLLL LLLL   .....
		fStreamW.WriteUInt8(0x08 | 0x02);
		fStreamW.WriteUInt8(iListenerID);
		fStreamW.WriteUInt16(iListenName.size());
		}
	fStreamW.Write(iListenName.data(), iListenName.size());
	}

void ZStreamMUX::CopyDataIn(ZMutexLocker& iLocker, SessionCB* iSession,
	const void* iSource, size_t iCount, size_t& oCount)
	{
	oCount = min(iCount, iSession->fReceiveBufferFeedOut - iSession->fReceiveBufferFeedIn);
	size_t feedOutOffset = iSession->fReceiveBufferFeedOut % iSession->fReceiveBufferSize;
	size_t feedInOffset = iSession->fReceiveBufferFeedIn % iSession->fReceiveBufferSize;
	if (feedInOffset < feedOutOffset)
		{
		// The feed in is to the left of the feed out -- we can read
		// the payload in a single chunk.
		ZAssertStop(kDebug_StreamMUX, feedOutOffset - feedInOffset >= oCount);
		iLocker.Release();
		fStreamR.Read(iSession->fReceiveBuffer + feedInOffset, oCount);
		iLocker.Acquire();
		}
	else
		{
		// May need to read in two chunks because the free space wraps around
		size_t countToReceive = min(oCount, iSession->fReceiveBufferSize - feedInOffset);
		iLocker.Release();
		fStreamR.Read(iSession->fReceiveBuffer + feedInOffset, countToReceive);
		if (oCount > countToReceive)
			fStreamR.Read(iSession->fReceiveBuffer, oCount - countToReceive);
		iLocker.Acquire();
		}
	iSession->fReceiveBufferFeedIn += oCount;
	}

void ZStreamMUX::CopyDataOut(SessionCB* iSession, void* iDest, size_t iCount, size_t& oCount)
	{
	oCount = 0;
	// Try to pull some data from the buffer
	if (iSession->fReceiveBufferFeedIn > iSession->fReceiveBufferFeedOut)
		{
		size_t feedOutOffset = iSession->fReceiveBufferFeedOut % iSession->fReceiveBufferSize;
		size_t feedInOffset = iSession->fReceiveBufferFeedIn % iSession->fReceiveBufferSize;
		if (feedInOffset > feedOutOffset)
			{
			// The feed out is to the left of the feed in -- copy as many bytes
			// in between as possible and advance the feed out.
			size_t countToMove = min(feedInOffset - feedOutOffset, iCount);
			ZBlockMove(iSession->fReceiveBuffer + feedOutOffset, iDest, countToMove);
			if (ZCONFIG_Debug >= kDebug_StreamMUX)
				ZBlockSet(iSession->fReceiveBuffer + feedOutOffset, countToMove, 0x55);
			iSession->fReceiveBufferFeedOut += countToMove;
			oCount = countToMove;
			}
		else
			{
			// The feed out is to the right of the feed in. Copy from the feed out to
			// the end of the buffer.
			uint8* localDest = reinterpret_cast<uint8*>(iDest);
			size_t countRemaining = iCount;
			size_t countToMove
				= min(iSession->fReceiveBufferSize - feedOutOffset, countRemaining);

			ZBlockMove(iSession->fReceiveBuffer + feedOutOffset, localDest, countToMove);
			if (ZCONFIG_Debug >= kDebug_StreamMUX)
				ZBlockSet(iSession->fReceiveBuffer + feedOutOffset, countToMove, 0x55);
			iSession->fReceiveBufferFeedOut += countToMove;
			localDest += countToMove;
			countRemaining -= countToMove;
			oCount += countToMove;

			// Then copy from the beginning of the buffer to the feed in.
			countToMove = min(feedInOffset, countRemaining);
			ZBlockMove(iSession->fReceiveBuffer, localDest, countToMove);
			if (ZCONFIG_Debug >= kDebug_StreamMUX)
				ZBlockSet(iSession->fReceiveBuffer, countToMove, 0x77);
			iSession->fReceiveBufferFeedOut += countToMove;
			oCount += countToMove;
			}
		}
	}

void ZStreamMUX::AbortSession(SessionCB* iSession)
	{
	ZAssertStop(kDebug_StreamMUX, fMutex.IsLocked());

	iSession->fOpen_Send = false;
	iSession->fSentFIN = true;
	iSession->fOpen_Receive = false;

	// Notify all Waiter_Receives
	Waiter_Receive* currWaiterReceive = iSession->fWaiters_Receive_Head;
	while (currWaiterReceive)
		{
		currWaiterReceive->fResult = errorAbortRemote;
		Waiter_Receive* nextWaiter = currWaiterReceive->fNext;
		currWaiterReceive->fNext = nil;
		currWaiterReceive = nextWaiter;
		}

	// Notify all Waiter_Sends
	Waiter_Send* currWaiterSend = iSession->fWaiters_Send_Head;
	while (currWaiterSend)
		{
		currWaiterSend->fResult = errorAbortRemote;
		Waiter_Send* nextWaiter = currWaiterSend->fNext;
		currWaiterSend->fNext = nil;
		currWaiterSend = nextWaiter;
		}

	iSession->fOpen_Send = false;
	iSession->fSentFIN = true;
	iSession->fOpen_Receive = false;
	iSession->fWaiters_Receive_Head = nil;
	iSession->fWaiters_Receive_Tail = nil;
	iSession->fCondition_Receive.Broadcast();
	iSession->fCondition_Send.Broadcast();
	}

ZStreamMUX::SessionCB* ZStreamMUX::UseSession(bool iIsNear, PrivID iPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, fMutex.IsLocked());
	SessionCB* currSession = fSessions_Head;
	while (currSession)
		{
		if (currSession->fIsNear == iIsNear && currSession->fID == iPrivID)
			{
			++currSession->fUseCount;
			return currSession;
			}
		currSession = currSession->fNext;
		}
	return nil;
	}

ZStreamMUX::SessionCB* ZStreamMUX::UseSession(SessionCB* iSession)
	{
	ZAssertStop(kDebug_StreamMUX, fMutex.IsLocked());
	++iSession->fUseCount;
	return iSession;
	}

void ZStreamMUX::ReleaseSession(SessionCB* iSession)
	{
	ZAssertStop(kDebug_StreamMUX, fMutex.IsLocked());
	ZAssertStop(kDebug_StreamMUX, iSession->fUseCount);
	if (--iSession->fUseCount == 0)
		{
		if (!iSession->fOpen_Send && iSession->fSentFIN && !iSession->fOpen_Receive)
			{
			if (fSessionNextSend == iSession)
				fSessionNextSend = iSession->fNext;
			if (iSession->fPrev)
				iSession->fPrev->fNext = iSession->fNext;
			if (iSession->fNext)
				iSession->fNext->fPrev = iSession->fPrev;
			if (fSessions_Head == iSession)
				fSessions_Head = iSession->fNext;
			if (fSessions_Tail == iSession)
				fSessions_Tail = iSession->fPrev;
			iSession->fNext = nil;
			iSession->fPrev = nil;
			delete iSession;
			}
		}
	}

ZStreamMUX::SessionID ZStreamMUX::sMakeSessionID(bool iIsNear, ZStreamMUX::PrivID iPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, iPrivID <= kMaxPrivID);
	ZStreamMUX::SessionID result = 0x4000 | iPrivID;
	if (iIsNear)
		result |= 0x2000;
	return result;
	}

void ZStreamMUX::sMakeSessionID(ZStreamMUX::PrivID iPrivID, bool iIsNear)
	{
	// Calling the wrong sMakeSessionID -- your ID and near flag are swapped.
	ZUnimplemented();
	}

void ZStreamMUX::sMakePrivID(ZStreamMUX::SessionID iSessionID,
	bool& oIsNear, ZStreamMUX::PrivID& oPrivID)
	{
	ZAssertStop(kDebug_StreamMUX, iSessionID & 0x4000);
	oPrivID = iSessionID & 0x01FF;
	oIsNear = iSessionID & 0x2000;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamMUX::SessionCB

ZStreamMUX::SessionCB::SessionCB(bool iIsNear, PrivID iPrivID,
	size_t iCreditSend, size_t iReceiveBufferSize)
	{
	fIsNear = iIsNear;
	fID = iPrivID;
	fUseCount = 1;

	fPrev = nil;
	fNext = nil;

	fAborted = false;

	fCredit_Send = iCreditSend;
	fOpen_Send = true;
	fSentFIN = false;
	fWaiters_Send_Head = nil;
	fWaiters_Send_Tail = nil;

	fCredit_Receive = iReceiveBufferSize;
	fOpen_Receive = true;
	fWaiters_Receive_Head = nil;
	fWaiters_Receive_Tail = nil;

	fReceiveBuffer = new uint8[iReceiveBufferSize];
	if (ZCONFIG_Debug >= kDebug_StreamMUX)
		ZBlockSet(fReceiveBuffer, iReceiveBufferSize, 0xAA);
	fReceiveBufferSize = iReceiveBufferSize;
	fReceiveBufferFeedIn = 0;
	fReceiveBufferFeedOut = 0;

	fTotalSent = 0;
	fTotalReceived = 0;
	}

ZStreamMUX::SessionCB::~SessionCB()
	{
	ZAssertStop(kDebug_StreamMUX, fUseCount == 0);
	ZAssertStop(kDebug_StreamMUX, fPrev == nil);
	ZAssertStop(kDebug_StreamMUX, fNext == nil);
	ZAssertStop(kDebug_StreamMUX, fOpen_Send == false);
	ZAssertStop(kDebug_StreamMUX, fSentFIN == true);
	ZAssertStop(kDebug_StreamMUX, fPrev == nil);
	ZAssertStop(kDebug_StreamMUX, fNext == nil);
	ZAssertStop(kDebug_StreamMUX, fWaiters_Send_Head == nil);
	ZAssertStop(kDebug_StreamMUX, fWaiters_Send_Tail == nil);
	ZAssertStop(kDebug_StreamMUX, fOpen_Receive == false);
	ZAssertStop(kDebug_StreamMUX, fWaiters_Receive_Head == nil);
	ZAssertStop(kDebug_StreamMUX, fWaiters_Receive_Tail == nil);
	delete[] fReceiveBuffer;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamMUX::StreamRW

ZStreamMUX::StreamRW::StreamRW(ZStreamMUX& iStreamMUX, SessionID iSessionID)
:	fStreamMUX(iStreamMUX), fSessionID(iSessionID)
	{}

ZStreamMUX::StreamRW::~StreamRW()
	{}

void ZStreamMUX::StreamRW::Imp_Read(void* iDest, size_t iCount, size_t* oCountRead)
	{
	uint8* localDest = static_cast<uint8*>(iDest);
	while (iCount)
		{
		size_t countRead;
		ZStreamMUX::Error result = fStreamMUX.Receive(fSessionID, localDest, iCount, &countRead);
		if (result != ZStreamMUX::errorNone)
			break;
		localDest += countRead;
		iCount -= countRead;
		}
	if (oCountRead)
		*oCountRead = localDest - static_cast<uint8*>(iDest);
	}

size_t ZStreamMUX::StreamRW::Imp_CountReadable()
	{
	size_t count;
	fStreamMUX.CountReceiveable(fSessionID, count);
	return count;
	}

void ZStreamMUX::StreamRW::Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten)
	{
	const uint8* localSource = static_cast<const uint8*>(iSource);
	while (iCount)
		{
		size_t countWritten;
		ZStreamMUX::Error result = fStreamMUX.Send(fSessionID,
			localSource, iCount, &countWritten);

		if (result != ZStreamMUX::errorNone)
			break;
		localSource += countWritten;
		iCount -= countWritten;
		}
	if (oCountWritten)
		*oCountWritten = localSource - static_cast<const uint8*>(iSource);
	}
