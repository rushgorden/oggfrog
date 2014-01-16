/*  @(#) $Id: ZNet.h,v 1.14 2006/04/14 20:59:05 agreen Exp $ */

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

#ifndef __ZNet__
#define __ZNet__ 1
#include "zconfig.h"

#include "ZCompat_NonCopyable.h"
#include "ZStreamer.h"

#include <string>
#include <stdexcept> // For runtime_error

class ZNetAddressLookup;
class ZNetEndpoint;
class ZNetName;
class ZNetNameLookup;

// =================================================================================================
#pragma mark -
#pragma mark * ZNet

namespace ZNet
	{
	enum Error
		{
		errorNone,
		errorGeneric,
		errorBadState,
	
		errorCouldntConnect,
		errorLocalPortInUse,
		errorCantGetAmountUnread,
		errorBufferTooSmall
		};
	}

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEx

class ZNetEx : public std::runtime_error
	{
public:
	ZNetEx(ZNet::Error iError);
	ZNet::Error GetNetError() const
		{ return fError; }

private:
	ZNet::Error fError;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetAddress

/// Represents the physical address of a particular endpoint on a particular host.
class ZNetAddress : ZooLib::NonCopyable
	{
public:
	virtual ~ZNetAddress();
	virtual ZNetAddress* Clone() const = 0;

	virtual ZNetAddressLookup* CreateLookup(size_t iMaxNames) const;

	virtual ZRef<ZNetEndpoint> OpenRW() const;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetAddressLookup

/// Subclasses of this are returned by ZNetAddress instances when
/// ZNetAddress::CreateLookup is called. It's an iterator that returns zero or
/// more names that are considered to map to the address in question.
class ZNetAddressLookup : ZooLib::NonCopyable
	{
public:
	virtual ~ZNetAddressLookup();

	virtual void Start() = 0;
	virtual bool Finished() = 0;
	virtual void Next() = 0;
	virtual const ZNetName& CurrentName() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetName

/// Represents the abstract name of a port or service on a host or hosts.
class ZNetName : ZooLib::NonCopyable
	{
public:
	virtual ~ZNetName();
	virtual ZNetName* Clone() const = 0;
	virtual std::string AsString() const = 0;

	virtual ZNetNameLookup* CreateLookup(size_t iMaxAddresses) const;

	ZRef<ZNetEndpoint> OpenRW() const;

	ZNetAddress* Lookup() const;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetNameLookup

/// Subclasses of this are returned by ZNetName instances when
/// ZNetName::CreateLookup is called. It's an iterator that returns zero or
/// more addresses that the name maps to.
class ZNetNameLookup : ZooLib::NonCopyable
	{
public:
	virtual ~ZNetNameLookup();

	virtual void Start() = 0;
	virtual bool Finished() = 0;
	virtual void Next() = 0;
	virtual const ZNetAddress& CurrentAddress() = 0;
	virtual const ZNetName& CurrentName() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetListener

/// Subclasses of this return ZNetEndpoint instances as connections arrive.
class ZNetListener : public ZRefCountedWithFinalization, ZooLib::NonCopyable
	{
public:
	virtual ~ZNetListener();

	virtual ZRef<ZNetEndpoint> Listen() = 0;
	virtual void CancelListen() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpoint

/// Subclasses of this provide the interface to network endpoints. As a subclass of
/// ZStreamerRW it provides access to the endpoint's read and write streams, and has
/// an extended protocol to allow the closing and aborting of the endpoint.
class ZNetEndpoint : public ZStreamerRW, ZooLib::NonCopyable
	{
protected:
	ZNetEndpoint();

public:
	virtual ~ZNetEndpoint();

// Our protocol
	virtual bool WaitTillReadable(int iMilliseconds) = 0;
	virtual void SendDisconnect() = 0;
	virtual bool ReceiveDisconnect(int iMilliseconds) = 0;
	virtual void Disconnect();
	virtual void Abort() = 0;

	virtual ZNetAddress* GetRemoteAddress() = 0;
	};

// =================================================================================================
#pragma mark -
#pragma mark * ZNetEndpointDG

class ZNetEndpointDG : public ZRefCountedWithFinalization
	{
protected:
	ZNetEndpointDG();

public:
	virtual ~ZNetEndpointDG();

	virtual ZNet::Error Receive(void* iBuffer, size_t iBufferSize, size_t& oCountReceived, ZNetAddress*& oSourceAddress) = 0;
	virtual ZNet::Error Send(const void* iBuffer, size_t iCount, ZNetAddress* iDestAddress) = 0;
	};

#endif // __ZNet__
