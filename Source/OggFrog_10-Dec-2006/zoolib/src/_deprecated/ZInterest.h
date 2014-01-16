/*  @(#) $Id: ZInterest.h,v 1.1.1.1 2002/02/17 18:39:11 agreen Exp $ */

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

#ifndef __ZInterest__
#define __ZInterest__ 1
#include "zconfig.h"

#include "ZThread.h"

#include <vector>

/*
AG 97-01-03
ZInterest is a replacement for Host and Client from Zoom.
Why? Good question -- I was reading "Inside Taligent Technology" (Addison Wesley) and
came across the idea of a client registering an interest in some attribute of another object,
which is basically what Host and Client did. However, all the Client/Interest protocol
provides for is notification that whatever the interest is attached to has changed -- there's
no further information provided. If you need to distinguish different attributes of an object,
simply have that object own several interest objects, and provide accessors to get the
addresses of those objects. The advantage of this scheme is that it is *infinitely*
extensible, without having to define a namespace of shorts (or whatever) defining
each message.

I changed the names of the objects mainly to make it bloody obvious that Host and Client
have gone. We can of course reinstate Host and Client by making a Host object have several
interests, and determining which object tripped, then translating that into the old
eOpen, eClosed, eUpdate etc. messages.

Finally, the implementation of ZInterest and ZClient automatically unregister themselves,
so no blow-ups occur when we forget to unregister interest. The code can be changed to
trip an assertion if something goes away without unregistering all its interests.
*/

/*
AG 97-01-14
Well, I've found that the notification stuff works well, but for real world use
it's really handy to have a separate thread that sends out the notifications. So now
you can specify an async parameter in NotifyClients. Note that you must ensure
that there are no circular dependencies through the use of interests. We do not
do what MacApp does, which is to walk the tree marking objects needing
notification, and bailing if an object is already marked (which prevents
infinite loops).
*/

/*
AG 97-02-07
I found a case where I want to be able to delete a client whilst the interest
is still notifying stuff. So now we make a temporary copy of the list of clients
and walk that, obviating the problem of the client list being changed whilst
we're traversing it. In addition, the client list is now a multiset, so if a single
client registers with an interest several times it will receive only one notification.
Finally, we now dynamically allocate the vector, saving some space.
*/

/*
AG 97-06-29
All change! Back to simple vectors, but we keep track of when we're walking
a vector, and do index fixups as needed. Should be much safer, and simpler too.
*/
// ==================================================
class ZInterestClient;
class ZThread;
class ZInterest
	{
public:
	ZInterest() : fClients(nil), fCurrentIndex(nil) {}
	virtual ~ZInterest();
	
	void RegisterClient(ZInterestClient* inClient);
	void UnregisterClient(ZInterestClient* inClient);
	void NotifyClients(bool async = true);
	void NotifyClientsAsync();
	void NotifyClientsImmediate();

	long GetSpecificClientCount(ZInterestClient* inClient) const;
	long GetClientCount() const;

private:
	void Internal_NotifyClients();
	void Internal_RemoveClient(ZInterestClient* inClient);

	static void sDoAsyncNotifications();
	vector<ZInterestClient*>* fClients;
	long* fCurrentIndex;

	static ZThread* sNotificationThread;
	static vector<ZInterest*> sPendingNotifications;
	static ZMutex sMutex;

	friend class ZInterestClient;
	};

// ==================================================

class ZInterestClient
	{
public:
	ZInterestClient() : fCurrentIndex(nil) {}
	virtual ~ZInterestClient();
	
	virtual void InterestChanged_Full(ZInterest* inInterest, ZMutex& inMutex);
	virtual void InterestChanged(ZInterest* inInterest);

private:
// We keep a back list to aid in debugging etc. You can also just
// assume all your interests will be unregistered when you're destroyed.
	void Internal_InterestUnregistered(ZInterest* inInterest);
	void Internal_InterestRegistered(ZInterest* inInterest);
	void Internal_RemoveInterest(ZInterest* inInterest);

	vector<ZInterest*> fInterests;
	long* fCurrentIndex;

	friend class ZInterest;
	};

#endif // __ZInterest__
