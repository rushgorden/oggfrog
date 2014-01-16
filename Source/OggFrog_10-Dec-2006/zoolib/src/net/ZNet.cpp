static const char rcsid[] = "@(#) $Id: ZNet.cpp,v 1.9 2006/04/14 20:59:05 agreen Exp $";

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

#include "ZNet.h"

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEx

ZNetEx::ZNetEx(ZNet::Error iError)
:	fError(iError),
	runtime_error("ZNetEx")
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetAddress

ZNetAddress::~ZNetAddress()
	{}

ZNetAddressLookup* ZNetAddress::CreateLookup(size_t iMaxNames) const
	{ return nil; };

ZRef<ZNetEndpoint> ZNetAddress::OpenRW() const
	{ return ZRef<ZNetEndpoint>(); }

// =================================================================================================
#pragma mark -
#pragma mark * ZNetAddressLookup

ZNetAddressLookup::~ZNetAddressLookup()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetName

ZNetName::~ZNetName()
	{}

ZNetNameLookup* ZNetName::CreateLookup(size_t iMaxAddresses) const
	{ return nil; };

ZRef<ZNetEndpoint> ZNetName::OpenRW() const
	{
	if (ZNetAddress* theAddress = this->Lookup())
		{
		ZRef<ZNetEndpoint> theEndpoint = theAddress->OpenRW();
		delete theAddress;
		return theEndpoint;
		}
	return ZRef<ZNetEndpoint>();
	}

ZNetAddress* ZNetName::Lookup() const
	{
	if (ZNetNameLookup* theLookup = this->CreateLookup(1))
		{
		theLookup->Start();
		if (!theLookup->Finished())
			{
			ZNetAddress* theAddress = theLookup->CurrentAddress().Clone();
			delete theLookup;
			return theAddress;
			}
		delete theLookup;
		}
	return nil;
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetNameLookup

ZNetNameLookup::~ZNetNameLookup()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener

ZNetListener::~ZNetListener()
	{}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpoint

ZNetEndpoint::ZNetEndpoint()
	{}

ZNetEndpoint::~ZNetEndpoint()
	{}

void ZNetEndpoint::Disconnect()
	{
	this->SendDisconnect();
	this->ReceiveDisconnect(-1);
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpointDG

ZNetEndpointDG::ZNetEndpointDG()
	{}

ZNetEndpointDG::~ZNetEndpointDG()
	{}

