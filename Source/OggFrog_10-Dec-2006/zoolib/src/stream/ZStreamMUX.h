/*  @(#) $Id: ZStreamMUX.h,v 1.10 2006/07/23 21:58:20 agreen Exp $ */

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

#ifndef __ZStreamMUX__
#define __ZStreamMUX__ 1

#include "zconfig.h"
#include "ZStream.h"
#include "ZThread.h"

#include <deque>
#include <string>

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamMUX

class ZStreamMUX
	{
public:
	typedef uint16 SessionID;
	static const SessionID kInvalidSessionID = 0;

	enum Error
		{
		/// The operation succeeded.
		errorNone,

		/// The operation failed because Abort was called locally.
		errorAbortLocal,

		/// The operation failed because the remote end called Abort.
		errorAbortRemote,

		/// Data could not be sent because Close has already been called locally.
		errorSendClosed,

		/// The remote end has called Close and all data has been received.
		errorReceiveClosed,

		/// The SessionID is either illegal or refers to a non-existent session.
		errorInvalidSession,

		/// Attempted to listen with a name that is already being listened for.
		errorAlreadyListening,

		 /// Attempted to accept or stop listening for a name that was not being listened for.
 		errorNotListening,

		/// The listener name was illegal (currently zero length is the only illegal name).
		errorBadListenName,

		/// The ZStreamMUX's underlying connection disappeared.
		errorConnectionDied,

		/// The ZStreamMUX was disposed before the call could complete.
		errorDisposed,

		/// Used internally
		waitPending
		};

	class StreamRW;

	ZStreamMUX(const ZStreamR& iStreamR, const ZStreamW& iStreamW);
	~ZStreamMUX();

	void Startup();
	void Shutdown();

	Error Listen(const std::string& iListenName);
	Error StopListen(const std::string& iListenName);
	Error Accept(const std::string& iListenName, SessionID& oSessionID);

	Error Open(const std::string& iListenName, const void* iData, size_t iCount,
		size_t* oCountSent, SessionID& oSessionID);
	Error Close(SessionID iSessionID);
	Error Abort(SessionID iSessionID);

	Error Send(SessionID iSessionID, const void* iSource, size_t iCount, size_t* oCountSent);
	Error Receive(SessionID iSessionID, void* iDest, size_t iCount, size_t* oCountReceived);
	Error CountReceiveable(SessionID iSessionID, size_t& oCountReceivable);

	void SetMaximumFragmentSize(size_t iSize);

private:
	typedef uint16 PrivID;
	struct SessionCB;

	void RunSend();
	static void sRunSend(ZStreamMUX* iStreamMUX);

	bool HandleSend_Open(ZMutexLocker& iLocker);
	bool HandleSend_RST(ZMutexLocker& iLocker);
	bool HandleSend_SYNACK(ZMutexLocker& iLocker);
	bool HandleSend_Credit(ZMutexLocker& iLocker);
	bool HandleSend_Data(ZMutexLocker& iLocker);

	void RunReceive();
	static void sRunReceive(ZStreamMUX* iStreamMUX);

	void HandleReceive_Payload(ZMutexLocker& iLocker, SessionCB* iSession, size_t iPayloadLength);
	void HandleReceive_Credit(bool iIsNear, PrivID iPrivID, size_t iCredit);
	void HandleReceive_SYN(PrivID iPrivID, uint8 iListenerID, size_t iPayloadLength);
	void HandleReceive_SYNACK(PrivID iPrivID);
	void HandleReceive_FIN(bool iIsNear, PrivID iPrivID);
	void HandleReceive_RST(bool iIsNear, PrivID iPrivID);
	void HandleReceive_DefineListener(uint8 iListenerID, size_t iListenNameLength);

	void WriteFragment_SYNACK(PrivID iPrivID);
	void WriteFragment_FIN(bool iIsNear, PrivID iPrivID);
	void WriteFragment_RST(bool iIsNear, PrivID iPrivID);
	void WriteFragment_Credit(bool iIsNear, PrivID iPrivID, size_t iSentCredit);
	void WriteFragment_Payload(bool iIsNear, PrivID iPrivID, const void* iSource, size_t iLength);
	void WriteFragment_DefineListener(uint8 iListenerID, const std::string& iListenName);

	void CopyDataIn(ZMutexLocker& iLocker, SessionCB* iSession,
		const void* iSource, size_t iCount, size_t& oCount);
	void CopyDataOut(SessionCB* iSession, void* iDest, size_t iCount, size_t& oCount);

	void AbortSession(SessionCB* iSession);

	SessionCB* UseSession(bool iIsNear, PrivID iPrivID);
	SessionCB* UseSession(SessionCB* iSession);
	void ReleaseSession(SessionCB* iSession);

	static SessionID sMakeSessionID(bool iIsNear, PrivID iPrivID);
	static void sMakeSessionID(PrivID iPrivID, bool iIsNear);
	static void sMakePrivID(SessionID iSessionID, bool& oIsNear, PrivID& oPrivID);
	static const PrivID kMaxPrivID = 0x1FF;

	const ZStreamR& fStreamR;
	const ZStreamW& fStreamW;
	size_t fDefaultCredit;
	size_t fMaxFragmentSize;

	struct SessionCB;

	struct Waiter_Send
		{
		const uint8* fSource;
		size_t fCount;
		Error fResult;
		Waiter_Send* fNext;
		};

	struct Waiter_Receive
		{
		uint8* fDest;
		size_t fCount;
		Error fResult;
		Waiter_Receive* fNext;
		};

	struct Waiter_Open
		{
		std::string fListenName;
		SessionCB* fSession;
		const uint8* fSource;
		size_t fCount;
		Error fResult;
		Waiter_Open* fNext;
		};

	struct Waiter_RST
		{
		SessionCB* fSession;
		bool fDoneIt;
		Waiter_RST* fNext;
		};

	struct Waiter_Accept
		{
		SessionCB* fSession;
		Error fResult;
		Waiter_Accept* fNext;
		};

	struct SYNReceived
		{
		std::string fListenName;
		PrivID fID;
		uint8* fBuffer;
		size_t fBufferSize;
		SYNReceived* fNext;
		};

	struct ActiveListen
		{
		std::string fListenName;
		std::deque<SessionCB*> fPendingSessions;
		Waiter_Accept* fWaiters_Head;
		Waiter_Accept* fWaiters_Tail;
		ActiveListen* fNext;
		};

	/** Just in case the CB suffix is not obvious, this is the session Control Block,
	used to maintain state for an open session. */
	struct SessionCB
		{
		SessionCB(bool iIsNear, PrivID iPrivID, size_t iCreditSend, size_t iReceiveBufferSize);
		~SessionCB();

		bool fIsNear;
		PrivID fID;
		size_t fUseCount;

		SessionCB* fPrev;
		SessionCB* fNext;

		bool fAborted;

		size_t fCredit_Send;
		bool fOpen_Send;
		bool fSentFIN;
		Waiter_Send* fWaiters_Send_Head;
		Waiter_Send* fWaiters_Send_Tail;
		ZCondition fCondition_Send;

		size_t fCredit_Receive;
		bool fOpen_Receive;
		Waiter_Receive* fWaiters_Receive_Head;
		Waiter_Receive* fWaiters_Receive_Tail;
		ZCondition fCondition_Receive;

		uint8* fReceiveBuffer;
		size_t fReceiveBufferSize;
		size_t fReceiveBufferFeedIn;
		size_t fReceiveBufferFeedOut;

		size_t fTotalSent;
		size_t fTotalReceived;
		};

	struct ListenName
		{
		std::string fName;
		uint8 fID;
		ListenName* fNext;
		};

	ZCondition fCondition_Send;
	ZCondition fCondition_Open;
	ZCondition fCondition_Accept;

	ZMutex fMutex;

	SessionCB* fSessions_Head;
	SessionCB* fSessions_Tail;
	SessionCB* fSessionNextSend;

	SYNReceived* fPendingSYN_Head;
	SYNReceived* fPendingSYN_Tail;

	Waiter_Open* fWaiters_Open_Head;
	Waiter_Open* fWaiters_Open_Tail;
	Waiter_Open* fWaiters_OpenSYNSent;

	Waiter_RST* fWaiters_RST_Head;
	Waiter_RST* fWaiters_RST_Tail;

	ActiveListen* fActiveListens;

	ActiveListen* fActiveListens_RST;

	Waiter_Accept* fWaiters_Accept_Head;
	Waiter_Accept* fWaiters_Accept_Tail;

	ListenName* fListenNames_Send;
	ListenName* fListenNames_Receive;

	ZSemaphore* fSem_Disposed;
	bool fExitedSend;
	bool fExitedReceive;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZStreamMUX::StreamRW

class ZStreamMUX::StreamRW  : public ZStreamR, public ZStreamW
	{
public:
	StreamRW(ZStreamMUX& iStreamMUX, SessionID iSessionID);
	~StreamRW();

// From ZStreamR
	virtual void Imp_Read(void* iDest, size_t iCount, size_t* oCountRead);
	virtual size_t Imp_CountReadable();

// From ZStreamW
	virtual void Imp_Write(const void* iSource, size_t iCount, size_t* oCountWritten);

private:
	ZStreamMUX& fStreamMUX;
	SessionID fSessionID;
	};

#endif // __ZStreamMUX__
