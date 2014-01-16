static const char rcsid[] = "@(#) $Id: ZInterest.cpp,v 1.2 2002/11/02 18:56:04 agreen Exp $";

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

#include "ZInterest.h"
#include "ZDebug.h"
#include "ZThreadSimple.h"

#include "ZCompat_algorithm.h"


/* AG 98-01-27. ZInterest::sPendingNotifications was previously maintained as sorted list of
the addresses of ZInterest objects that need to be fired. This blows up any code which relies on
the order of interest firing being the same as the order in which the notification was made. So
now we just shove the things on the tail end of the list. If you fire async and sync notifications,
the sync notifications will probably (certainly) get fired before the async ones. So be careful. */

ZThread* ZInterest::sNotificationThread = nil;
vector<ZInterest*> ZInterest::sPendingNotifications;
ZMutex ZInterest::sMutex("ZInterest::sMutex");

// ==================================================

ZInterest::~ZInterest()
	{
// If notifications were posted asynchronously, it's possible that they
// will be lost if the owning object is destroyed before they're sent
	ZMutexLocker locker(sMutex);
	vector<ZInterest*>::iterator theIter = find(sPendingNotifications.begin(), sPendingNotifications.end(), this);
	if (theIter != sPendingNotifications.end())
		sPendingNotifications.erase(theIter);

// If we're in the middle of walking the list, terminate it by setting that index to v. large number 0x40000000;
	if (fCurrentIndex != nil)
		*fCurrentIndex = 0x40000000;
	long currentIndex = 0;
	fCurrentIndex = &currentIndex;
	for (; fClients != nil && currentIndex < (long)fClients->size(); ++currentIndex)
		{
		ZInterestClient* theClient = (*fClients)[currentIndex];
		theClient->Internal_RemoveInterest(this);
		}
	fCurrentIndex = nil;
	if (fClients)
		delete fClients;
	}

void ZInterest::RegisterClient(ZInterestClient* inClient)
	{
	ZMutexLocker locker(sMutex);
	if (fClients == nil)
		{
		fClients = new vector<ZInterestClient*>;
		fClients->push_back(inClient);
		}
	else
		{
		vector<ZInterestClient*>::iterator insertPos = lower_bound(fClients->begin(), fClients->end(), inClient);
		insertPos = fClients->insert(insertPos, inClient);
		long offset = insertPos-fClients->begin();
		if (fCurrentIndex != nil && offset <= *fCurrentIndex)
			++(*fCurrentIndex);
		}
	inClient->Internal_InterestRegistered(this);
	}

void ZInterest::UnregisterClient(ZInterestClient* inClient)
	{
	ZMutexLocker locker(sMutex);
	this->Internal_RemoveClient(inClient);
	inClient->Internal_InterestUnregistered(this);
	}

void ZInterest::NotifyClients(bool async)
	{
	ZMutexLocker locker(sMutex);
// Don't bother if we have no clients
	if (fClients == nil)
		return;
	if (async)
		{
		vector<ZInterest*>::iterator theIter = find(sPendingNotifications.begin(), sPendingNotifications.end(), this);
		if (theIter == sPendingNotifications.end())
			sPendingNotifications.push_back(this);
		if (sNotificationThread == nil)
			{
			sNotificationThread = new ZThreadSimple<>(ZInterest::sDoAsyncNotifications, "ZInterest::sDoAsyncNotifications");
			sNotificationThread->Start();
			}
		}
	else
		{
// Make sure we're pulled from the list of pending asynch notifications
		vector<ZInterest*>::iterator theIter = find(sPendingNotifications.begin(), sPendingNotifications.end(), this);
		if (theIter != sPendingNotifications.end())
			sPendingNotifications.erase(theIter);
		this->Internal_NotifyClients();
		}
	}

void ZInterest::NotifyClientsAsync()
	{ this->NotifyClients(true); }

void ZInterest::NotifyClientsImmediate()
	{ this->NotifyClients(false); }

void ZInterest::Internal_NotifyClients()
	{
	ZAssert(sMutex.IsLocked());
// If we have no clients (not uncommon) then return
	if (fClients == nil)
		return;

// If someone is already walking the list, just return.
	if (fCurrentIndex)
		return;

// Optimize the simple case of a single client
	if (fClients->size() == 1)
		{
		(*fClients)[0]->InterestChanged_Full(this, sMutex);
		}
	else
		{
// Keep track of the lastClient, so we do one notification per client
		ZInterestClient* lastClient = nil;
		long currentIndex = 0;
		fCurrentIndex = &currentIndex;
		for (; currentIndex != 0x40000000 && fClients != nil && currentIndex < (long)fClients->size(); ++currentIndex)
			{
			ZInterestClient* currentClient = (*fClients)[currentIndex];
			if (currentClient != lastClient)
				{
				currentClient->InterestChanged_Full(this, sMutex);
				}
			lastClient = currentClient;
			if (currentIndex == 0x40000000)
				break;
			}
		if (currentIndex != 0x40000000)
			fCurrentIndex = nil;
		}
	}

void ZInterest::sDoAsyncNotifications()
	{
	ZAssert(!sMutex.IsLocked());
	ZMutexLocker locker(sMutex);
	while (sPendingNotifications.size() > 0)
		{
		ZInterest* anInterest = sPendingNotifications[0];
		sPendingNotifications.erase(sPendingNotifications.begin());
		anInterest->Internal_NotifyClients();
		}
	sNotificationThread = nil;
	}

long ZInterest::GetClientCount() const
	{
	ZMutexLocker locker(sMutex);
	if (fClients)
		return fClients->size();
	return 0;
	}

long ZInterest::GetSpecificClientCount(ZInterestClient* inClient) const
	{
	ZMutexLocker locker(sMutex);
	if (fClients)
		{
		long count = 0;
		for (vector<ZInterestClient*>::iterator i = fClients->begin(); i != fClients->end(); ++i)
			{
			if ((*i) == inClient)
				count++;
			}
		return count;
		}
	return 0;
	}

void ZInterest::Internal_RemoveClient(ZInterestClient* inClient)
	{
	ZAssert(sMutex.IsLocked());
	ZAssertStop(2, fClients);

	vector<ZInterestClient*>::iterator position = lower_bound(fClients->begin(), fClients->end(), inClient);
	ZAssertStop(2, *position == inClient);

	long offset = position - fClients->begin();
	if (fCurrentIndex != nil && offset <= *fCurrentIndex)
		--(*fCurrentIndex);
	fClients->erase(position);
	if (fClients->size() == 0)
		{
		delete fClients;
		fClients = nil;
		}
	}

// ==================================================

ZInterestClient::~ZInterestClient()
	{
	ZLocker locker(ZInterest::sMutex);
	ZAssertStop(2, fCurrentIndex == nil);
	long currentIndex = 0;
	fCurrentIndex = &currentIndex;
	for (; currentIndex < (long)fInterests.size(); ++currentIndex)
		{
		ZInterest* theInterest = fInterests[currentIndex];
		theInterest->Internal_RemoveClient(this);
		}
	fCurrentIndex = nil;
	}

void ZInterestClient::InterestChanged_Full(ZInterest* inInterest, ZMutex& inMutex)
	{
	inMutex.Release();
	ZAssert(!inMutex.IsLocked());
	try
		{
		this->InterestChanged(inInterest);
		}
	catch (...)
		{}
	inMutex.Acquire();
	}

void ZInterestClient::InterestChanged(ZInterest* inInterest)
	{}

void ZInterestClient::Internal_InterestRegistered(ZInterest* inInterest)
	{
	ZAssert(ZInterest::sMutex.IsLocked());
	ZAssertStop(2, fCurrentIndex == nil);
	fInterests.push_back(inInterest);
	}

void ZInterestClient::Internal_InterestUnregistered(ZInterest* inInterest)
	{
	ZAssert(ZInterest::sMutex.IsLocked());
	this->Internal_RemoveInterest(inInterest);
	}

void ZInterestClient::Internal_RemoveInterest(ZInterest* inInterest)
	{
	ZAssert(ZInterest::sMutex.IsLocked());
// AG 99-08-02. I don't think this should be called whilst fCurrentIndex is non-nil -- but make sure of the fact.
	ZAssertStop(2, fCurrentIndex == nil);
	for (long x = 0; x < (long)fInterests.size();)
		{
		if (fInterests[x] == inInterest)
			fInterests.erase(fInterests.begin()+x);
		else
			++x;
		}
	}
