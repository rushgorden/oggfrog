/*  @(#) $Id: ZMessage.h,v 1.6 2006/04/10 20:44:20 agreen Exp $ */

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

#ifndef __ZMessage__
#define __ZMessage__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZTuple.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZMessage

class ZMessage : public ZTuple
	{
public:
	ZMessage() {}
	ZMessage(const ZMessage& iOther) : ZTuple(iOther) {}
	ZMessage(const ZTuple& iTuple) : ZTuple(iTuple) {}
	~ZMessage() {}


	ZTuple AsTuple() const { return *this; }	
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageLooper

class ZMessageReceiver;
class ZMessengerRep;

class ZMessageLooper : ZooLib::NonCopyable
	{
public:
	virtual ZMutexBase& GetLock() = 0;
	virtual bool DispatchMessage(const ZMessage& iMessage);
	virtual void QueueMessage(const ZMessage& iMessage)  = 0;
	void QueueMessageForReceiver(const ZMessage& iMessage, ZMessageReceiver* iReceiver);
	void QueueMessageForMessenger(const ZMessage& iMessage, ZRef<ZMessengerRep> iMessengerRep);
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMessengerRep

class ZMessengerRep : public ZRefCountedWithFinalization, ZooLib::NonCopyable
	{
public:
	virtual ~ZMessengerRep();

	friend class ZMessageLooper;
	friend class ZMessageReceiver;
	friend class ZMessenger;

	ZMessengerRep(ZMessageReceiver* iReceiver);

	bool PostMessage(const ZMessage& iMessage);
	bool SendMessage(const ZMessage& iMessage, ZMessage* oReply);

	void ReceiverDisposed(ZMessageReceiver* iReceiver);

	void DispatchMessage(const ZMessage& iMessage);

	ZMessageReceiver* fReceiver;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMessenger

class ZMessenger
	{
public:
	ZMessenger();
	ZMessenger(const ZMessenger& inOther);
	ZMessenger(const ZRef<ZMessengerRep>& inRep);
	~ZMessenger();

	ZMessenger& operator=(const ZMessenger& inOther);

	bool operator==(const ZMessenger& inOther) const;

	bool PostMessage(const ZMessage& iMessage);
	bool SendMessage(const ZMessage& iMessage, ZMessage* oReply);

private:
	ZRef<ZMessengerRep> fRep;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageReceiver

class ZMessageReceiver
	{
private:
	ZMessageReceiver() {}
	ZMessageReceiver(const ZMessageReceiver&) {}
	ZMessageReceiver& operator=(const ZMessageReceiver&) { return *this; }

protected:
	ZMessageReceiver(ZMessageLooper* iLooper);
	~ZMessageReceiver();

public:
	ZMessageLooper* GetLooper()
		{ return fLooper; }

	ZMessenger GetMessenger();

	void PostMessage(const ZMessage& iMessage);

	virtual void ReceivedMessage(const ZMessage& iMessage);

private:
	ZMessageLooper* fLooper;
	ZRef<ZMessengerRep> fMessengerRep;
	};

// =================================================================================================
#if 0
#pragma mark -
#pragma mark * ZMessenger Inlines

inlne ZMessenger::ZMessenger()
	{}

inline ZMessenger::ZMessenger(const ZMessenger& inOther)
:	fRep(inOther.fRep)
	{}

inline ZMessenger::~ZMessenger()
	{}

inline ZMessenger& ZMessenger::operator=(const ZMessenger& inOther)
	{
	fRep = inOther.fRep;
	return *this;
	}

inline bool ZMessenger::operator==(const ZMessenger& inOther) const
	{ return fRep == inOther.fRep; }

#endif
// =================================================================================================

#endif // __ZMessage__
