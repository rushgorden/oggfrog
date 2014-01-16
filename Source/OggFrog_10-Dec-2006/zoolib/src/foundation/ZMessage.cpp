static const char rcsid[] = "@(#) $Id: ZMessage.cpp,v 1.8 2006/04/26 22:31:27 agreen Exp $";

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

#include "ZMessage.h"

#define kDebug_Message 2

static ZMutex sMutex_Messenger;

using std::string;

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageLooper

bool ZMessageLooper::DispatchMessage(const ZMessage& iMessage)
	{
	string theWhat = iMessage.GetString("what");
	if (theWhat == "zoolib:MessageReceiver")
		{
		ZMessageReceiver* theReceiver
			= static_cast<ZMessageReceiver*>(iMessage.GetPointer("messageReceiver"));
		theReceiver->ReceivedMessage(iMessage.GetTuple("message"));
		return true;
		}
	else if (theWhat == "zoolib:MessengerRep")
		{
		if (ZRef<ZMessengerRep> theRep
			= ZRefStaticCast<ZMessengerRep>(iMessage.GetRefCounted("messengerRep")))
			{
			theRep->DispatchMessage(iMessage.GetTuple("message"));
			}
		return true;
		}
	return false;
	}

void ZMessageLooper::QueueMessageForReceiver(const ZMessage& iMessage, ZMessageReceiver* iReceiver)
	{
	ZMessage envelope;
	envelope.SetString("what", "zoolib:MessageReceiver");
	envelope.SetPointer("messageReceiver", iReceiver);
	envelope.SetTuple("message", iMessage.AsTuple());
	this->QueueMessage(envelope);
	}

void ZMessageLooper::QueueMessageForMessenger(const ZMessage& iMessage,
	ZRef<ZMessengerRep> iMessengerRep)
	{
	ZMessage envelope;
	envelope.SetString("what", "zoolib:MessengerRep");
	envelope.SetRefCounted("messengerRep", iMessengerRep);
	envelope.SetTuple("message", iMessage.AsTuple());
	this->QueueMessage(envelope);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMessenger

ZMessenger::ZMessenger()
	{}

ZMessenger::ZMessenger(const ZMessenger& inOther)
:	fRep(inOther.fRep)
	{}

ZMessenger::ZMessenger(const ZRef<ZMessengerRep>& inRep)
:	fRep(inRep)
	{}

ZMessenger::~ZMessenger()
	{}

ZMessenger& ZMessenger::operator=(const ZMessenger& inOther)
	{
	fRep = inOther.fRep;
	return *this;
	}

bool ZMessenger::operator==(const ZMessenger& inOther) const
	{ return fRep == inOther.fRep; }

bool ZMessenger::PostMessage(const ZMessage& iMessage)
	{
	if (ZRef<ZMessengerRep> theRep = fRep)
		return theRep->PostMessage(iMessage);
	return false;
	}

bool ZMessenger::SendMessage(const ZMessage& iMessage, ZMessage* outReply)
	{
	if (ZRef<ZMessengerRep> theRep = fRep)
		return theRep->SendMessage(iMessage, outReply);
	return false;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageReceiver
ZMessengerRep::ZMessengerRep(ZMessageReceiver* iReceiver)
:	fReceiver(iReceiver)
	{}

ZMessengerRep::~ZMessengerRep()
	{
	ZMutexLocker locker(sMutex_Messenger);
	ZAssertStop(1, fReceiver == nil);
	}

bool ZMessengerRep::PostMessage(const ZMessage& iMessage)
	{
	ZMutexLocker locker(sMutex_Messenger);
	if (fReceiver)
		{
		ZAssertStop(1, fReceiver->GetLooper());
		fReceiver->GetLooper()->QueueMessageForMessenger(iMessage, this);
		return true;
		}
	return false;
	}

bool ZMessengerRep::SendMessage(const ZMessage& iMessage, ZMessage* outReply)
	{
	ZUnimplemented();
	return false;
	}

void ZMessengerRep::ReceiverDisposed(ZMessageReceiver* iReceiver)
	{
	ZMutexLocker locker(sMutex_Messenger);
	ZAssertStop(1, fReceiver && fReceiver == iReceiver);
	fReceiver = nil;
	}

void ZMessengerRep::DispatchMessage(const ZMessage& iMessage)
	{
	// fReceiver cannot go out of scope unexpectedly -- it's effectively
	// protected by its looper's lock.
	if (fReceiver)
		fReceiver->ReceivedMessage(iMessage);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZMessageReceiver

ZMessageReceiver::ZMessageReceiver(ZMessageLooper* iLooper)
	{
	ZAssertStop(kDebug_Message, iLooper);
	fLooper = iLooper;
	}

ZMessageReceiver::~ZMessageReceiver()
	{
	ZAssertStop(2, fLooper && fLooper->GetLock().IsLocked());
	if (fMessengerRep)
		fMessengerRep->ReceiverDisposed(this);
	}

void ZMessageReceiver::PostMessage(const ZMessage& iMessage)
	{
// Should we require that a ZMessenger be used for posting to an unlocked
// receiver? If so then this assert should be uncommented.
//	ZAssertStop(kDebug_Message, fLooper && fLooper->GetLock().IsLocked());
	fLooper->QueueMessageForReceiver(iMessage, this);
	}

void ZMessageReceiver::ReceivedMessage(const ZMessage& iMessage)
	{}

ZMessenger ZMessageReceiver::GetMessenger()
	{
	ZAssertStop(kDebug_Message, fLooper && fLooper->GetLock().IsLocked());
	if (!fMessengerRep)
		fMessengerRep = new ZMessengerRep(this);
	return ZMessenger(fMessengerRep);
	}
